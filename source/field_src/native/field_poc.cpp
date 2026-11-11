#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "poc_contract.h"
#include "registry_bridge.h"
#ifndef FL_FOG_VARIANT
#define FL_FOG_VARIANT 0
#endif

/* Experimental all-land field submission. Buffers belong to this DLL, never to
   ArmyData, the game's duel manager or its unit pools. No gameplay tick is
   invoked. All engine calls run inside the original field driver's thread. */
extern "C" {
    __declspec(dllexport) FLStatus FL_Status = {};
    __declspec(dllexport) volatile LONG FL_StatusSequence = 0;
    __declspec(dllexport) FLRosterAudit FL_RosterAudit = {};
    __declspec(dllexport) fl_registry::Mailbox FL_Registry = {};
}
static uintptr_t base;
static uintptr_t info_destroy_next;
static HANDLE log_file = INVALID_HANDLE_VALUE;
static volatile LONG stop_requested;
static bool configured, blocked, ownership_lost, draw_current_frame;
static void *owner_manager, *owner_data, *owner_context, *owner_engine;
static SRWLOCK state_lock = SRWLOCK_INIT;
static unsigned log_bytes;
static uint64_t last_log_flush_ms, presentation_ms;
static unsigned log_write_count, log_flush_count;
static char pending_log[16384];
static unsigned pending_log_bytes;
static void write_pending_log() {
    if (!pending_log_bytes || log_file==INVALID_HANDLE_VALUE) return;
    if(log_bytes+pending_log_bytes>8*1024*1024) {
        LARGE_INTEGER zero={};
        if(SetFilePointerEx(log_file,zero,NULL,FILE_BEGIN) && SetEndOfFile(log_file)) log_bytes=0;
        else { pending_log_bytes=0; return; }
    }
    DWORD written=0;
    WriteFile(log_file,pending_log,pending_log_bytes,&written,NULL);
    log_bytes+=written; ++log_write_count; last_log_flush_ms=GetTickCount64();
    if(written<pending_log_bytes) memmove(pending_log,pending_log+written,pending_log_bytes-written);
    pending_log_bytes-=written;
}
static unsigned engine_calls, thread_handoffs;
static DWORD engine_call_thread;
/* SoundDopeSheet evaluates synchronously inside the private actor update. */
static thread_local unsigned private_presentation_depth;
struct Proxy;
static thread_local Proxy *private_sound_proxy;
static thread_local bool private_staging;
static uint32_t suppressed_motion_sounds;

/* The lock protects private resources. The sequence also lets the external
   launcher reject a read taken partway through a frame's status update. */
static void lock_status() {
    AcquireSRWLockExclusive(&state_lock);
    InterlockedIncrement(&FL_StatusSequence);
}
static void unlock_status() {
    InterlockedIncrement(&FL_StatusSequence);
    ReleaseSRWLockExclusive(&state_lock);
}

template<class T> static T &at(void *p, size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(p) + offset);
}
#ifdef FL_PORTABLE_TEST
extern uintptr_t fl_test_function(uint32_t rva);
template<class T> static T fn(uint32_t rva) { return reinterpret_cast<T>(fl_test_function(rva)); }
#else
template<class T> static T fn(uint32_t rva) { return reinterpret_cast<T>(base + rva); }
#endif
static void *global(uint32_t rva) { return *reinterpret_cast<void **>(base + rva); }
typedef void (*Call1)(void *);
typedef void (*Call2)(void *, void *);
typedef void (*Call3)(void *, void *, void *);
typedef int (*SelectBody)(void *);
typedef void (*RequestColor)(void *, int, int);
typedef bool (*EventSound)(void *,void *,void *,void *,uint32_t,uint32_t,void *,uint32_t *,void *);
static bool event_sound(uint32_t rva,void *a,void *b,void *c,void *d,uint32_t e,uint32_t f,
                        void *params,uint32_t *out,void *bank);
extern "C" __declspec(dllexport) bool FL_HookEventSound2D(void *a,void *b,void *c,void *d,
        uint32_t e,uint32_t f,void *params,uint32_t *out,void *bank) {
    return event_sound(FL_EVENT_SOUND_2D,a,b,c,d,e,f,params,out,bank);
}
extern "C" __declspec(dllexport) bool FL_HookEventSound3D(void *a,void *b,void *c,void *d,
        uint32_t e,uint32_t f,void *params,uint32_t *out,void *bank) {
    return event_sound(FL_EVENT_SOUND_3D,a,b,c,d,e,f,params,out,bank);
}

static void log_line(const char *format, ...) {
    if (log_file == INVALID_HANDLE_VALUE) return;
    char b[1024];
    int n = snprintf(b, sizeof b, "frame=%lu tid=%lu ",
                     (unsigned long)FL_Status.frame, (unsigned long)GetCurrentThreadId());
    va_list args; va_start(args, format);
    int count = vsnprintf(b + n, sizeof(b) - n - 3, format, args);
    va_end(args);
    if (count < 0) return;
    size_t length = strlen(b);
    if (length > sizeof(b) - 3) length = sizeof(b) - 3;
    b[length++] = '\r'; b[length++] = '\n';
    /* One OS-cached write per callback/buffer, no synchronous disk flush on
       the field thread. Records remain readable by the single GUI collector. */
    if(pending_log_bytes+length>sizeof pending_log) write_pending_log();
    if(pending_log_bytes+length<=sizeof pending_log) {
        memcpy(pending_log+pending_log_bytes,b,length); pending_log_bytes+=(unsigned)length;
    }
}

static void log_motion_samples();
#include "presentation_perf.h"

/* Track each WHOLE native update/draw/teardown call, including the original
   function outside state_lock. A sequential callback may use a new worker;
   overlapping or reentrant callbacks still quarantine the private handles.
   No extra lock is held while calling the original engine. */
static bool begin_engine_call(const char *stage) {
    lock_status();
    bool exclusive=engine_calls++==0;
    if (exclusive) engine_call_thread=GetCurrentThreadId();
    else if (configured && !ownership_lost) {
        log_line("BLOCKED overlapping engine calls stage=%s active_tid=%lu incoming_tid=%lu",stage,
            (unsigned long)engine_call_thread,(unsigned long)GetCurrentThreadId());
        blocked=true; ownership_lost=true; draw_current_frame=false;
        FL_Status.phase=FL_BLOCKED; FL_Status.reason=FL_OVERLAPPING_CALL;
    }
    unlock_status();
    return exclusive;
}
static void end_engine_call() {
    lock_status();
    if (--engine_calls==0) engine_call_thread=0;
    performance_summary(FL_Status.ready_count,FL_Status.target_count);
    write_pending_log();
    unlock_status();
}

struct Unit {
    void *actor, *army, *person;
    uint32_t id, mode;
    float x, y, z, yaw;
    int action;
    bool engaged;
};
enum Pose { POSE_IDLE, POSE_MOVE, POSE_ATTACK, POSE_STRONG, POSE_HIT, POSE_BATTLE_IDLE, POSE_GUARD, POSE_FALL };
#define FL_IDLE_COPY_METRICS
#include "idle_seam_core.h"
using LocalBonePose=IdleSeam::Pose;
static_assert(sizeof(LocalBonePose)==48,"Native SRT stride");
struct MotionSample {
    void *setting,*model;
    int action; uint32_t frame;
    float before,after,clip_rate,period,evaluated,native_after;
};
struct alignas(16) Proxy {
    /* Native actor destructor reports 0x190 bytes, excluding pool links. */
    alignas(16) unsigned char actor_storage[2][0x190];
    /* Presentation-only CDuelUnit fields. No native duel constructor/tick. */
    alignas(16) unsigned char duel_storage[2][0x1c0];
    unsigned char *actor,*duel;
    unsigned active_bank;
    bool refresh_loading;
    uint32_t refresh_born_frame;
    Unit source;
    bool constructed, ready, submitted;
    bool replacement_logged, color_ready_logged;
    bool fog_hidden, draw_visible_latch;
    unsigned deferred_mask;
    uint32_t deferred_generation, replaced_generation;
    void *model_cache_actor, *model_cache[3];
    float loop_offset; bool loop_phase_pending,loop_horse_pending;
    MotionSample motion_sample[2];
    uint32_t distance_generation, visibility_generation;
    float camera_distance; bool camera_visible;
    uint32_t born_frame;
    uint32_t last_snapshot_frame;
    uint32_t motion_sample_frame, motion_transitions;
    uint64_t last_displacement_ms;
    bool running;
    int body, horse, weapon, color, horse_color;
    uint64_t serial, skill_pending_ms, hit_pending_ms, last_hit_ms;
    uint32_t last_draw_frame, skill_events, hit_events;
    Pose pose;
    float clip_frame, hoof_gain;
    bool pose_done, skill_pending, hit_pending, ambient_removed;
    bool combat_engaged;
    uint32_t combat_cycles;
    uint64_t guard_pending_ms, last_guard_ms, tactic_guard_ms;
    bool guard_pending;
    uint32_t guard_draws, guard_wins, guard_plays;
    unsigned opponent_slot;
    uint64_t opponent_serial;
    float opponent_chance;
    bool normal_blocked, normal_contact_done;
    bool normal_pending;
    uint64_t normal_pending_ms;
    // 0 unknown, 1 opponent not stronger, 2 opponent stronger. Event snapshots,
    // not recomputed per frame and never substituted for the game's damage.
    unsigned char opponent_relation, opponent_target_relation, hit_relation, guard_relation, pose_relation;
    bool hit_sound_handled, hit_pending_sound_handled;
    uint64_t skill_token, reaction_token, reaction_source_serial, reaction_deadline_ms;
    unsigned reaction_source_slot;
    bool reaction_waiting, reaction_blocked, strong_contact_done;
    unsigned char reaction_relation;
    uint64_t tactic_targets[8];
    uint64_t tactic_decided_ms;
    uint32_t last_combat_sound_frame;
    float combat_gain;
    void *ambient_collector;
    bool defeated, captured, terminal_started, terminal_done, source_detached;
    uint64_t terminal_ms, appearance_check_ms;
    bool scale_body_ok, scale_weapon_ok, native_effects_trimmed;
    uint32_t attachment_frame;
    uint64_t pose_began_ms, blend_began_ms;
    unsigned paused_wait_ms;
    void *blend_model;
    unsigned blend_count;
    LocalBonePose blend_from[256];
    void *blend_horse_model;
    unsigned blend_horse_count,blend_duration_ms;
    float blend_facing_from;
    LocalBonePose blend_horse_from[256];
    // Private presentation cache only; no shared/game object layout changes.
    bool static_valid, visual_deferred;
    Unit evaluated_source;
    unsigned char *evaluated_actor;
    // HF5 presentation scheduling only: screen-edge wake zone and the 120Hz
    // half-rate hold of stationary combat loops. Never shared with the game.
    bool view_awake, half_skipped;
    uint32_t view_generation, eval_frame, eval_transitions;
    uint64_t eval_tick;
};
static Proxy proxies[FL_PROXY_COUNT];
static uint64_t next_proxy_serial;
static Unit candidates[FL_PROXY_COUNT];
static unsigned candidate_count;

/* Bounded open-addressing indexes avoid scanning every proxy for every native
   draw call. Both indexes are rebuilt in the owning field update under lock. */
struct ActorEntry { uintptr_t actor; unsigned slot; uint32_t generation; };
struct ActorIndex { ActorEntry rows[1024]; uint32_t generation; };
static void clear_index(ActorIndex &index) { if(!++index.generation) { memset(index.rows,0,sizeof index.rows); index.generation=1; } }
static constexpr unsigned INDEX_SIZE=1024;
static ActorIndex candidate_index, proxy_index, army_index;
static uint32_t draw_generation;
static unsigned active_slots[FL_PROXY_COUNT], active_count;
static unsigned index_start(uintptr_t actor) { return unsigned(((actor>>4)^(actor>>14))*2654435761u)&(INDEX_SIZE-1); }
static int index_find(const ActorIndex &table,uintptr_t actor) {
    auto *index=table.rows;
    unsigned h=index_start(actor);
    for(unsigned n=0;n<INDEX_SIZE;++n,h=(h+1)&(INDEX_SIZE-1)) {
        if (index[h].generation!=table.generation) return -1;
        if (index[h].actor==actor) return (int)index[h].slot;
    }
    return -1;
}
static bool index_insert(ActorIndex &table,uintptr_t actor,unsigned slot) {
    auto *index=table.rows;
    unsigned h=index_start(actor);
    for(unsigned n=0;n<INDEX_SIZE;++n,h=(h+1)&(INDEX_SIZE-1)) {
        if (index[h].generation==table.generation && index[h].actor==actor) return false;
        if (index[h].generation!=table.generation) { index[h]={actor,slot,table.generation}; return true; }
    }
    return false;
}
static bool current_source(const Proxy &p) {
    if (!p.constructed || p.defeated || p.source_detached) return false;
    void *actor=p.source.actor, *army=at<void *>(actor,0x40);
    return at<uintptr_t>(actor,0)==base+FL_ARMY_VTABLE &&
        at<uint32_t>(actor,0x740)==p.source.mode && fl_land_mode(p.source.mode) &&
        at<uint32_t>(actor,0x748)!=2 && army && army==p.source.army &&
        at<uint8_t>(army,0x10) && at<uint16_t>(army,0x12)==p.source.id &&
        at<void *>(owner_data,0x158+p.source.id*8)==p.source.person;
}

static void trim_native_particles(Proxy &p,void *actor);
static void bind_private_vtable(Proxy &p);
static bool terminal_source(const Proxy &p,void *actor);
static void *character_model(Proxy &p,unsigned part);
static void capture_attack_recovery(Proxy &p);
static void capture_pose_transition(Proxy &p,Pose next);
static float pose_facing_offset(const Proxy &p);
static bool live_combat_target(const Proxy &p);
static bool current_owners(void *manager,void *engine) {
    void *app=global(FL_APP);
    return manager && engine && manager==owner_manager && engine==owner_engine &&
        manager==global(FL_UNIT_MANAGER) && owner_data==global(FL_DATA_MANAGER) &&
        owner_context==global(FL_RESOURCE_CONTEXT) && app && at<void *>(app,0x210)==engine;
}
static bool replacement_draw_eligible(Proxy &p) {
    if(!FL_FOG_VARIANT) return p.defeated || current_source(p);
    const bool current=p.deferred_generation==draw_generation && p.deferred_mask!=0;
    if(!p.defeated) {
        p.draw_visible_latch=current;p.fog_hidden=!current;
        return current && current_source(p);
    }
    if(current) p.draw_visible_latch=true;
    else if(!p.source_detached) p.draw_visible_latch=false;
    p.fog_hidden=!p.draw_visible_latch;
    return p.draw_visible_latch;
}

static float shared_camera_distance(Proxy &p) {
    if(p.distance_generation!=FL_Status.frame) {
        alignas(16) float world[4]={p.source.x,p.source.y,p.source.z,1};
        p.camera_distance=fn<float (*)(const float *)>(FL_CAMERA_DISTANCE)(world);
        p.distance_generation=FL_Status.frame;
    } return p.camera_distance;
}
static bool shared_camera_visible(Proxy &p) {
    if(p.visibility_generation!=FL_Status.frame) {
        alignas(16) float world[4]={p.source.x,p.source.y,p.source.z,1};
        p.camera_visible=fn<int (*)(const float *,float)>(FL_CAMERA_VISIBLE)(world,1.0f)!=0;
        p.visibility_generation=FL_Status.frame;
    } return p.camera_visible;
}
/* HF5 wake zone. 1c5690(point, 1.0) is the exact screen test (1.0 is the NDC
   bound; widening it would also stretch the depth range past the far plane, so
   it is never widened). Instead a unit counts as "near the screen" when a point
   one model-length away from its centre, left/right/front/back or above, is on
   screen. Units on screen always wake at once. Units only near the edge are
   prepared before they appear, a few per update so a fast pan or zoom never
   wakes hundreds at once. An awake unit sleeps only beyond a wider ring, so
   edge jitter never flips it. Audio keeps its own exact centre test. */
static constexpr float FL_VIEW_WAKE_RADIUS=1500.0f, FL_VIEW_SLEEP_RADIUS=2600.0f;
static constexpr unsigned FL_PREWARM_PER_UPDATE=12;
static unsigned prewarm_budget=FL_PREWARM_PER_UPDATE;
static bool view_point(float x,float y,float z) {
    alignas(16) float world[4]={x,y,z,1};
    return fn<int (*)(const float *,float)>(FL_CAMERA_VISIBLE)(world,1.0f)!=0;
}
static bool view_near(const Proxy &p,float r) {
    const float x=p.source.x,y=p.source.y,z=p.source.z;
    return view_point(x+r,y,z) || view_point(x-r,y,z) || view_point(x,y,z+r) ||
           view_point(x,y,z-r) || view_point(x,y+r*0.5f,z);
}
static bool presentation_awake(Proxy &p,unsigned slot) {
    if(p.view_generation==FL_Status.frame && p.view_generation) return p.view_awake;
    p.view_generation=FL_Status.frame;
    bool awake;
    if(shared_camera_visible(p)) awake=true;
    else if(p.view_awake) {
        awake=view_near(p,FL_VIEW_SLEEP_RADIUS);
        if(!awake) ++perf_view_sleeps;
    } else if(((FL_Status.frame^slot)&1) && view_near(p,FL_VIEW_WAKE_RADIUS)) {
        // Sleeping off-screen units look at the edge ring every other update.
        awake=prewarm_budget!=0;
        if(awake) { --prewarm_budget; ++perf_prewarm_wakes; } else ++perf_prewarm_deferred;
    } else awake=false;
    p.view_awake=awake;
    return awake;
}
#include "combat_audio.h"
#include "animation_clock.h"

/* HF5 120Hz half-rate hold. Only a READY, VISIBLE, STANDING unit repeating an
   ordinary combat loop (normal swing, battle pause, guard) may keep its last
   pose for one update, and only when its next evaluation still lands within
   one 60Hz animation step. Movement, turning, any pose change or restart,
   blends, tactics, hits, defeat and re-entry always evaluate at once. The
   draw is never skipped, so nothing flickers; the next evaluation advances by
   the unit's own elapsed time (private_clock_rate). At 60FPS this never runs. */
static constexpr double FL_HALF_RATE_MAX_STEP_S=1.06/60.0, FL_HALF_RATE_FRAME_S=0.0105;
static bool half_rate_hold(const Proxy &p,const Unit &u,unsigned slot) {
    if(!p.ready || p.defeated || p.visual_deferred || p.running || p.refresh_loading || p.loop_phase_pending) return false;
    if(p.pose!=POSE_ATTACK && p.pose!=POSE_BATTLE_IDLE && p.pose!=POSE_GUARD) return false;
    if(p.evaluated_actor!=p.actor || p.eval_frame+1!=FL_Status.frame || p.eval_transitions!=p.motion_transitions) return false;
    if(p.blend_duration_ms && presentation_ms>=p.blend_began_ms && presentation_ms-p.blend_began_ms<p.blend_duration_ms) return false;
    const Unit &old=p.evaluated_source;
    if(old.x!=u.x || old.y!=u.y || old.z!=u.z || old.yaw!=u.yaw || old.mode!=u.mode) return false;
    // Only at a SUSTAINED high frame rate: both this interval and the recent
    // average must be short, so an uneven 60FPS never triggers a hold.
    if(!(animation_rate>0) || !performance_frequency || !p.eval_tick || animation_last_tick<=p.eval_tick) return false;
    const double frame_s=1.0/animation_rate;
    if(frame_s>FL_HALF_RATE_FRAME_S || !(animation_interval_avg_s>0) || animation_interval_avg_s>FL_HALF_RATE_FRAME_S) return false;
    double elapsed=double(animation_last_tick-p.eval_tick)/double(performance_frequency);
    if(elapsed+frame_s>FL_HALF_RATE_MAX_STEP_S) return false;
    // Alternate units so each update evaluates about half of them, not all at once.
    return ((FL_Status.frame^slot)&1)!=0;
}

/* Only the current original manager draw may defer these presentation calls.
   The source actor and ArmyData are never changed. If this pass cannot append
   the replacement, replay every deferred call before returning to the driver. */
struct DeferredDraw { void *object; uint32_t function; unsigned slot; };
static thread_local DeferredDraw deferred_draws[FL_PROXY_COUNT*FL_DRAWS_PER_UNIT];
static thread_local unsigned deferred_count;
static thread_local void *replacement_callback;
static thread_local DWORD replacement_thread;

#include "draw_lookup.h"
static bool defer_original_draw(void *object,void *engine,void *callback,bool action) {
    // Map membership is TLS-only. Owner/lifetime/stop gates are rechecked below.
    const bool mapped=draw_lookup_armed && replacement_callback==callback && callback &&
                      replacement_thread==GetCurrentThreadId();
    const DrawLookupEntry *entry=mapped?find_draw_lookup(object,action):NULL;

    bool deferred=false;
    AcquireSRWLockExclusive(&state_lock);
    ++(action?perf_action_hooks:perf_group_hooks);
    if(mapped) ++perf_mapped_hooks;
    // Accepted defeat is terminal: even a later presentation failure must
    // never replay soldiers. Only dereference the live actor from THIS call.
    if(configured && !ownership_lost && engine==owner_engine && current_owners(owner_manager,engine)) {
        const unsigned offsets[]={0x750,0x778,0x7c8,0x7f0,0x580,0x7a0,0x840};
        for(unsigned part=mapped?(entry?entry->part:7):0;part<7;part=mapped?7:part+1) {
            if(action!=(part==4) || (uintptr_t)object<offsets[part]) continue;
            void *actor=(void *)((uintptr_t)object-offsets[part]);
            int slot=mapped?int(entry->slot):index_find(proxy_index,(uintptr_t)actor);
            if(slot>=0 && terminal_source(proxies[slot],actor)) {
                ReleaseSRWLockExclusive(&state_lock); return true;
            }
        }
    }
    if (configured && !blocked && !ownership_lost && !stop_requested && draw_current_frame &&
        replacement_thread==GetCurrentThreadId() && replacement_callback==callback && callback &&
        engine==owner_engine && owner_manager==global(FL_UNIT_MANAGER) &&
        owner_data==global(FL_DATA_MANAGER) && owner_context==global(FL_RESOURCE_CONTEXT) &&
        !at<int>(owner_manager,0x498) && deferred_count<FL_PROXY_COUNT*FL_DRAWS_PER_UNIT) {
        const unsigned offsets[]={0x750,0x778,0x7c8,0x7f0,0x580,0x7a0,0x840};
        for(unsigned part=mapped?(entry?entry->part:7):0;part<7 && !deferred;part=mapped?7:part+1) {
            if (action!=(part==4) || (uintptr_t)object<offsets[part]) continue;
            int found=mapped?int(entry->slot):index_find(proxy_index,(uintptr_t)object-offsets[part]);
            if(found<0) continue;
            unsigned i=(unsigned)found;
            Proxy &p=proxies[i];
            if (!p.constructed || (!p.ready && !p.defeated)) continue;
            uint32_t mode=p.source.mode;
            bool known=mode==0 ? part<=4 : mode==2 ? part==3 || part==5 : mode==3 && part!=4;
            if(p.deferred_generation!=draw_generation) { p.deferred_generation=draw_generation; p.deferred_mask=0; }
            if (!known || (p.deferred_mask&(1u<<part))) continue;
            /* The exact argument match above binds this to an actor currently
               being drawn by the original engine, before dereferencing it. */
            if (!terminal_source(p,(void *)((uintptr_t)object-offsets[part])) && !current_source(p)) continue;
            p.deferred_mask|=1u<<part;
            deferred_draws[deferred_count++]={object,action?FL_FIELD_ACTION_DRAW:FL_FIELD_GROUP_DRAW,i};
            deferred=true;
        }
    }
    ReleaseSRWLockExclusive(&state_lock);
    return deferred;
}
extern "C" __declspec(dllexport) void FL_HookFieldGroupDraw(void *object,void *engine,void *callback) {
    if (!defer_original_draw(object,engine,callback,false))
        fn<Call3>(FL_FIELD_GROUP_DRAW)(object,engine,callback);
}
extern "C" __declspec(dllexport) void FL_HookFieldActionDraw(void *object,void *engine,void *callback) {
    if (!defer_original_draw(object,engine,callback,true))
        fn<Call3>(FL_FIELD_ACTION_DRAW)(object,engine,callback);
}

static bool on_owner_thread(const char *stage) {
    DWORD tid = GetCurrentThreadId();
    if (engine_calls!=1 || engine_call_thread!=tid) {
        if (!blocked) log_line("BLOCKED engine call outside exclusive callback stage=%s",stage);
        blocked=true; ownership_lost=true;
        FL_Status.phase=FL_BLOCKED; FL_Status.reason=FL_OTHER_THREAD;
        return false;
    }
    if (FL_Status.thread_id && tid!=FL_Status.thread_id) {
        ++thread_handoffs;
        /* A normal update/draw worker split must not cause per-frame I/O. */
        if (thread_handoffs<=8 || (thread_handoffs&(thread_handoffs-1))==0)
            log_line("FIELD_THREAD_HANDOFF stage=%s from=%lu to=%lu count=%u owners_verified=1 exclusive_native_call=1",
                stage,(unsigned long)FL_Status.thread_id,(unsigned long)tid,thread_handoffs);
    }
    FL_Status.thread_id=tid;
    return true;
}

static void release_proxy(Proxy &p, unsigned slot, const char *reason) {
    if (!p.constructed) return;
    log_line("RELEASE_BEGIN slot=%u officer=%u reason=%s", slot, p.source.id, reason);
    /* Mirrors original manager release then non-deleting destructor. The
       deleting destructor must NOT free our static aligned storage. */
    fn<Call1>(FL_ACTOR_RELEASE)(p.actor);
    fn<Call1>(FL_ACTOR_DTOR)(p.actor);
    if(p.refresh_loading) {
        fn<Call1>(FL_ACTOR_RELEASE)(p.actor_storage[1-p.active_bank]);
        fn<Call1>(FL_ACTOR_DTOR)(p.actor_storage[1-p.active_bank]);
    }
    memset(&p, 0, sizeof p);
    ++FL_Status.releases;
    FL_Status.officer[slot] = UINT32_MAX; FL_Status.appearance[slot] = -1;
    FL_Status.body_handle[slot] = FL_Status.horse_handle[slot] = FL_Status.weapon_handle[slot] = 0;
    log_line("RELEASE_END slot=%u", slot);
}
static void release_all(const char *reason) {
    sync_combat_ambient(true);
    for (unsigned i = 0; i < FL_PROXY_COUNT; ++i) release_proxy(proxies[i], i, reason);
    clear_index(proxy_index);
    clear_index(army_index);
    memset(hoof_voices,0,sizeof hoof_voices);
    memset(combat_voices,0,sizeof combat_voices); strategy_pause=false;
    FL_Status.ready_count = FL_Status.target_count = FL_Status.pending_count = 0;
}

static bool collect_units(void *manager, void *data) {
    candidate_count = 0;
    clear_index(candidate_index);
    FL_Status.water_count=FL_Status.excluded_count=0;
    void *actor = at<void *>(manager, 0x18);
    unsigned nodes = 0;
    while (actor) {
        if (++nodes > FL_PROXY_COUNT || at<uintptr_t>(actor, 0) != base + FL_ARMY_VTABLE) return false;
        void *army = at<void *>(actor, 0x40);
        uint32_t mode=at<uint32_t>(actor,0x740);
        if (army && at<uint32_t>(actor, 0x748) != 2 && at<uint8_t>(army, 0x10)) {
            if(mode==1) { ++FL_Status.water_count; actor=at<void *>(actor,0x8e0); continue; }
            bool accepted=false;
            uint32_t id = at<uint16_t>(army, 0x12);
            /* Native person lookup admits 0..0x1770 inclusive. Officer zero
               is valid; it is not an empty-slot sentinel. */
            if (fl_land_mode(mode) && id <= FL_MAX_OFFICER) {
                void *person = at<void *>(data, 0x158 + id * 8);
                if (person && at<uintptr_t>(person, 0) == base + FL_PERSON_VTABLE &&
                    at<uint16_t>(person, 0x10) == id) {
                    Unit u = {actor, army, person, id, mode, at<float>(actor, 0x10),
                              at<float>(actor, 0x14), at<float>(actor, 0x18), at<float>(actor, 0x744),
                              at<int>(actor,0x60), bool((at<uint8_t>(army,0x26)&1) && !at<uint8_t>(army,0x23))};
                    if (isfinite(u.x) && isfinite(u.y) && isfinite(u.z) && isfinite(u.yaw)) {
                        if (!index_insert(candidate_index,(uintptr_t)actor,candidate_count)) return false;
                        candidates[candidate_count++] = u;
                        accepted=true;
                    }
                }
            }
            if(!accepted) ++FL_Status.excluded_count;
        }
        actor = at<void *>(actor, 0x8e0);
    }
    return true;
}
static bool matches(const Proxy &p, const Unit &u) {
    return p.constructed && fl_same_unit((uintptr_t)p.source.actor, (uintptr_t)p.source.army,
        (uintptr_t)p.source.person, p.source.id, (uintptr_t)u.actor, (uintptr_t)u.army,
        (uintptr_t)u.person, u.id);
}
static uint64_t appearance_retry_after[fl_registry::officer_count];
static bool resolve_field_appearance(unsigned char *duel,const Unit &u) {
    fn<Call1>(FL_CURRENT_APPEARANCE)(duel);
    uint32_t before=__atomic_load_n(&FL_Registry.sequence,__ATOMIC_SEQ_CST);
    if(before&1)return false;
    fl_registry::Row row=FL_Registry.snapshot.rows[u.id];
    bool active=FL_Registry.snapshot.active!=0;
    int body=at<int>(duel,0x14),color=at<int>(duel,0x20);
    if(active && row.person==(uintptr_t)u.person && !memcmp(row.identity,static_cast<unsigned char *>(u.person)+0x12,28) &&
       row.model>=0 && row.model<int(fl_registry::model_count) && FL_Registry.snapshot.allowed[row.model])
        fl_registry::override_model(row.model,body,color);
    bool extended=active && body>=25 && body<int(fl_registry::model_count) && FL_Registry.snapshot.allowed[body];
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if(__atomic_load_n(&FL_Registry.sequence,__ATOMIC_SEQ_CST)!=before)return false;
    if(!fl_registry::appearance_valid(body,at<int>(duel,0x18),at<int>(duel,0x1c),color,at<int>(duel,0x24),extended))return false;
    at<int>(duel,0x14)=body;at<int>(duel,0x20)=color;
    return true;
}
static void update_private_actor(unsigned char *actor,unsigned char *duel,void *engine,void *frame) {
    motion_evaluation={};
    if(at<int>(duel,0x14)>=25) {
        // Same extended-body material policy as the verified registry update
        // wrapper, which otherwise intentionally ignores private descriptors.
        at<int>(duel,0x20)=-1;at<int>(actor,0x7c)=-1;at<int>(actor,0x84)=0;
    }
    ++perf_actor_calls;
    uint64_t started=(perf_actor_calls&63)==1?performance_tick():0;
    fn<Call3>(FL_ACTOR_UPDATE)(actor,engine,frame);
    motion_evaluation={};
    if(started) performance_record(perf_actor,started);
}
static void init_duel(unsigned char *d, const Unit &u) {
    memset(d, 0, 0x1c0);
    /* Field values consumed by 371f60, 1c4ce0, 1cad00 and their leaf getters.
       Initial visibility and motion IDs match 372e80/373000 (timeline 0).
       373000 itself is NOT called: it registers a real pooled duel actor. */
    /* 372e80 initializes BOTH colors to -1; 371f60 overwrites only common
       force-color cases. A unique body has no color rows, so requesting row 0
       would keep 3717c0 pending forever rather than preserve its base material. */
    at<int>(d, 0x10) = at<int>(d, 0x14) = at<int>(d, 0x20) = at<int>(d, 0x24) = -1;
    at<uint32_t>(d, 0xa8) = u.id; at<void *>(d, 0xb0) = u.person;
    at<int>(d, 0x28) = 55; at<int>(d, 0x2c) = 0;
    at<int>(d, 0xfc) = 1;  /* Mounted body attachment. */
    at<int>(d, 0x108) = at<int>(d, 0x10c) = at<int>(d, 0x110) = 1;
    at<float>(d, 0xc0) = u.x; at<float>(d, 0xc4) = u.y;
    at<float>(d, 0xc8) = u.z; at<float>(d, 0xcc) = 1.0f;
    at<float>(d, 0xf0) = u.yaw;
}
static bool create_proxy(Proxy &p, const Unit &u, unsigned slot) {
    memset(&p, 0, sizeof p); p.source = u;
    p.actor=p.actor_storage[0]; p.duel=p.duel_storage[0];
    p.appearance_check_ms=GetTickCount64();
    p.serial=++next_proxy_serial;
    uint32_t h=(u.id+1)*0x9e3779b9u; h^=h>>16;h*=0x7feb352du;h^=h>>15;
    p.loop_offset=1.0f+float(h&0xffffu)*(59.0f/65536.0f);
    p.loop_phase_pending=p.loop_horse_pending=true;
    init_duel(p.duel, u);
    log_line("SELECT_BEGIN slot=%u officer=%u", slot, u.id);
    /* Same original path for every officer and monarch, including effective
       stats, faction colour and equipped-item traversal. No model hash table. */
    if(!resolve_field_appearance(p.duel,u)) {
        appearance_retry_after[u.id]=GetTickCount64()+2000;
        log_line("R24_SELECTION_DEFERRED officer=%u other_units_continue=1",u.id);return false;
    }
    p.body = at<int>(p.duel, 0x14); p.horse = at<int>(p.duel, 0x18);
    p.weapon = at<int>(p.duel, 0x1c); p.color = at<int>(p.duel, 0x20);
    p.horse_color = at<int>(p.duel, 0x24);
    log_line("CREATE_BEGIN slot=%u officer=%u body=%d horse=%d weapon=%d colour=%d horse_colour=%d",
             slot,u.id,p.body,p.horse,p.weapon,p.color,p.horse_color);
    fn<Call1>(FL_ACTOR_CTOR)(p.actor);
    p.constructed = true; p.born_frame = FL_Status.frame;
    fn<Call2>(FL_ACTOR_CREATE)(p.actor, p.duel);
    fn<Call2>(FL_ACTOR_BIND)(p.actor, p.duel);
    bind_private_vtable(p);
    /* Original duel controller 379780 queues BOTH requests after create/bind.
       Do this before the first asynchronous loader poll. The native function
       chooses the body's secondary material slot; horse selection is independent
       of unique/common body class. -1 preserves the original base horse.
       No shared material is edited and no real duel controller is invoked. */
    fn<RequestColor>(FL_REQUEST_COLOR)(p.actor, 0, p.color);
    fn<RequestColor>(FL_REQUEST_COLOR)(p.actor, 1, p.horse_color);
    FLRosterRow &record=FL_RosterAudit.rows[u.id];
    record.body=(int16_t)p.body; record.weapon=(int16_t)p.weapon;
    record.body_color=(int16_t)p.color; record.horse_color=(int16_t)p.horse_color;
    log_line("COLOR_QUEUED slot=%u officer=%u body=%d body_colour=%d horse_colour=%d body_secondary=%d horse_secondary=%d",
             slot,u.id,p.body,p.color,p.horse_color,at<int>(p.actor,0x84),at<int>(p.actor,0xb4));
    ++FL_Status.creates;
    FL_Status.officer[slot] = u.id; FL_Status.appearance[slot] = p.body;
    log_line("CREATE_END slot=%u waiting for original asynchronous loaders",slot);
    return true;
}
static void reset_roster_audit() {
    uint32_t generation=FL_RosterAudit.generation+1;
    memset(&FL_RosterAudit,0,sizeof FL_RosterAudit);
    FL_RosterAudit.protocol=FL_PROTOCOL; FL_RosterAudit.generation=generation;
}
static void audit_roster_step(void *data) {
    if (FL_RosterAudit.complete) return;
    /* Scan the verified native person-pointer domain, in a sole owner callback.
       At most 32 slots and four existing persons per frame. No model allocation,
       gameplay constructor, save write, per-person disk I/O or roster mutation. */
    unsigned slots=0,persons=0;
    while (FL_RosterAudit.scanned_slots<=FL_MAX_OFFICER && slots<32 && persons<4) {
        unsigned id=FL_RosterAudit.scanned_slots++; ++slots;
        void *person=at<void *>(data,0x158+id*8);
        if (!person || at<uintptr_t>(person,0)!=base+FL_PERSON_VTABLE || at<uint16_t>(person,0x10)!=id) continue;
        ++persons; ++FL_RosterAudit.registered;
        Unit u={NULL,NULL,person,id,0,0,0,0,0,0,false};
        alignas(16) unsigned char duel[0x1c0]; init_duel(duel,u);
        bool resolved=resolve_field_appearance(duel,u);
        FLRosterRow &row=FL_RosterAudit.rows[id];
        row.body=(int16_t)at<int>(duel,0x14); row.weapon=(int16_t)at<int>(duel,0x1c);
        row.body_color=(int16_t)at<int>(duel,0x20); row.horse_color=(int16_t)at<int>(duel,0x24);
        row.flags|=FL_ROSTER_PRESENT;
        if (resolved) {
            row.flags|=FL_ROSTER_RESOLVED; ++FL_RosterAudit.resolved;
        } else { row.flags|=FL_ROSTER_UNRESOLVED; ++FL_RosterAudit.unresolved; }
    }
    if (FL_RosterAudit.scanned_slots>FL_MAX_OFFICER) {
        FL_RosterAudit.complete=1;
        log_line("ROSTER_SELECTION_DONE generation=%u scanned_slots=%u present=%u resolved=%u unresolved=%u visual_verified=0",
                 FL_RosterAudit.generation,FL_RosterAudit.scanned_slots,FL_RosterAudit.registered,
                 FL_RosterAudit.resolved,FL_RosterAudit.unresolved);
    }
}
static bool slot_ready(Proxy &p, unsigned model) {
    void *s = p.actor + 0x58 + model * 0x30;
    if (!at<int>(s, 0x18) || (model < 2 && at<int>(s, 0x28))) return false;
    void *h = at<void *>(s, 0x10);
    if (!h) return false;
    void *setting = at<void *>(h, 0x20);
    if (!setting) return false;
    void *child = at<void *>(setting, 0x30);
    return child && at<void *>(child, 0x20);
}
static void *character_model(Proxy &p, unsigned part) {
    if(part>=3)return NULL;
    if(p.model_cache_actor==p.actor && p.model_cache[part])return p.model_cache[part];
    if (!slot_ready(p,part)) return NULL;
    void *h=at<void *>(p.actor,0x68+part*0x30);
    void *setting=at<void *>(h,0x20), *child=at<void *>(setting,0x30);
    void *model=at<void *>(child,0x20);
    if(at<uintptr_t>(model,0)!=base+FL_CHARACTER_VTABLE)return NULL;
    if(p.model_cache_actor!=p.actor){memset(p.model_cache,0,sizeof p.model_cache);p.model_cache_actor=p.actor;}
    p.model_cache[part]=model;return model;
}
#include "final_motion.h"
#include "batch2_presentation.h"
static bool apply_field_scale(Proxy &p,unsigned,void *) {
    return p.attachment_frame==FL_Status.frame && p.scale_body_ok && p.scale_weapon_ok;
}
static void snapshot_proxy(Proxy &p, unsigned slot) {
    log_line("R22_PRESENTATION slot=%u dust_removed=%u field_arcs_removed=%u info_anchor_moves=%u hand_rebuilds_avoided=%u appearance_swaps=%u captured=%d terminal=%d",
        slot,native_dust_removed,native_arcs_removed,info_anchor_moves,optimized_attachments,appearance_refreshes,p.captured,p.defeated);
    log_line("COMBAT_FACING slot=%u officer=%u engaged=%d pose=%d cycles=%u source_yaw=%.4f display_yaw=%.4f body_frame=%.2f",
        slot,p.source.id,p.combat_engaged,int(p.pose),p.combat_cycles,p.source.yaw,at<float>(p.duel,0xf0),p.clip_frame);
    log_line("MOTION_SAMPLE slot=%u officer=%u moving=%d body_action=%d horse_action=%d transitions=%u extra_sound_events_blocked=%u",
        slot,p.source.id,p.running,at<int>(p.duel,0x28),at<int>(p.duel,0x2c),p.motion_transitions,suppressed_motion_sounds);
    log_line("COMBAT_AUDIO slot=%u officer=%u pose=%d skills=%u hits=%u hoof_gain=%.4f handoff=%d selected=%u played=%u capped=%u original_movement_omissions=%u",
        slot,p.source.id,int(p.pose),p.skill_events,p.hit_events,p.hoof_gain,p.ambient_removed,
        hoof_selected,hoof_events,hoof_budget_rejections,ambient_omissions);
    log_line("R21_COMBAT slot=%u officer=%u guard_trials=%u guard_wins=%u guard_plays=%u paused_trials=%u normal_vfx_suppressed=%u sounds_normal=%u guard=%u tactic_block=%u tactic_hit=%u sound_capped=%u",
        slot,p.source.id,p.guard_draws,p.guard_wins,p.guard_plays,paused_guard_trials,suppressed_normal_effects,
        normal_audio,guard_audio,tactic_block_audio,tactic_hit_audio,combat_audio_capped);
    log_line("R21_VISUAL slot=%u officer=%u weapon_bone=%d opponent_relation=%u pose_relation=%u stationary_dust_suppressed=%u stationary_field_effects_suppressed=%u native_combat_gain=0.40 ambience_updates=%u",
        slot,p.source.id,at<int>(p.duel,0xf8)?23:24,unsigned(p.opponent_relation),unsigned(p.pose_relation),
        suppressed_stationary_dust,suppressed_stationary_field_effects,ambient_attenuations);
    log_line("TRANSFORM slot=%u officer=%u source=%.4f,%.4f,%.4f actor=%.4f,%.4f,%.4f transform_enabled=%d",
        slot,p.source.id,p.source.x,p.source.y,p.source.z,at<float>(p.actor,0x10),
        at<float>(p.actor,0x14),at<float>(p.actor,0x18),at<int>(p.actor,0x54));
    for (unsigned k=0;k<3;++k) {
        if (!slot_ready(p,k)) continue;
        void *h=at<void *>(p.actor,0x68+k*0x30);
        void *setting=at<void *>(h,0x20), *child=at<void *>(setting,0x30);
        void *model=at<void *>(child,0x20);
        uintptr_t vt=at<uintptr_t>(model,0);
        if (vt!=base+FL_CHARACTER_VTABLE) {
            log_line("MODEL_OTHER slot=%u part=%u vtable_rva=%llx",slot,k,(unsigned long long)(vt-base));
            continue;
        }
        alignas(16) float position[4]={}, scale[4]={};
        fn<Call2>(FL_MODEL_GET_POSITION)(model,position);
        fn<Call2>(FL_MODEL_GET_SCALE)(model,scale);
        log_line("MODEL slot=%u part=%u handle=%llx payload=%llx world=%.4f,%.4f,%.4f scale=%.4f,%.4f,%.4f flags=%08x group_mask=%08x parent=%llx displayset=%llx",
            slot,k,(unsigned long long)(uintptr_t)child,(unsigned long long)(uintptr_t)model,
            position[0],position[1],position[2],scale[0],scale[1],scale[2],at<uint32_t>(model,0x44),
            at<uint32_t>(model,0x78),(unsigned long long)at<uintptr_t>(model,0x1c0),
            (unsigned long long)at<uintptr_t>(model,0x148));
    }
}
static void update_motion(Proxy &p,const Unit &u,unsigned slot) {
    const uint64_t now=presentation_ms;
    const double dx=double(u.x)-p.source.x,dz=double(u.z)-p.source.z;
    const double distance2=dx*dx+dz*dz;
    /* Sample world position, not camera motion, yaw or height-only settling.
       Native field 1c9640 sets actor+60=1 while interpolating a route. Slow
       field updates skip interpolation frames (actor+8dc), so retain an
       already observed run for at most 120ms with that native hint. An order
       alone cannot start a run. A map/view gap or a >4096-unit jump resets it.
       The time bound is independent of FPS and also bounds pause settling. */
    bool continuous=p.motion_sample_frame && FL_Status.frame-p.motion_sample_frame==1 &&
                    p.source.mode==u.mode && isfinite(distance2) && distance2<=4096.0*4096.0;
    bool displaced=continuous && distance2>0.01*0.01;
    bool moving=p.ready && continuous && (displaced ||
        (p.running && u.action==1 && now>=p.last_displacement_ms && now-p.last_displacement_ms<120));
    if (displaced) p.last_displacement_ms=now;
    p.running=moving;
    update_combat(p,u,slot,continuous);
    p.motion_sample_frame=FL_Status.frame;
}
static void update_proxy(Proxy &p, const Unit &u, unsigned slot, void *engine, void *frame) {
    if (!matches(p, u)) { release_proxy(p,slot,"unit identity changed"); if (!create_proxy(p,u,slot)) return; }
    if (!p.defeated && p.motion_sample_frame!=FL_Status.frame) update_motion(p,u,slot);
    p.source = u;
    if(p.ready && !presentation_awake(p,slot)) {
        p.visual_deferred=true;p.static_valid=false;p.half_skipped=false;
        ++perf_offscreen_skips;return;
    }
    if(p.visual_deferred && !p.defeated && p.pose!=POSE_IDLE && p.pose!=POSE_MOVE &&
       p.pose_began_ms!=presentation_ms) {
        // Do not replay an old offscreen tactic/reaction on camera re-entry.
        set_pose(p,POSE_IDLE,slot,true);
        p.pose_done=true;
        update_combat(p,u,slot,true);
        p.blend_count=p.blend_horse_count=p.blend_duration_ms=0;
    }
    const Unit &old=p.evaluated_source;
    if(p.ready && !p.defeated && p.pose==POSE_IDLE && p.static_valid &&
       !p.visual_deferred && p.evaluated_actor==p.actor &&
       old.x==u.x && old.y==u.y && old.z==u.z && old.yaw==u.yaw && old.mode==u.mode) {
        ++perf_static_skips;return;
    }
    if(half_rate_hold(p,u,slot)) {
        p.half_skipped=true;++perf_half_rate_skips;return;
    }
    p.visual_deferred=false;p.static_valid=false;
    at<float>(p.duel, 0xc0) = u.x; at<float>(p.duel, 0xc4) = u.y;
    at<float>(p.duel, 0xc8) = u.z;
    if(!p.defeated) at<float>(p.duel, 0xf0) = presentation_yaw(p,u);
    // Hold the ORIGINAL sweep's ready pose during the interval. Keeping the
    // same animation and weapon hand avoids the old 13 -> 55 -> 13 snap.
    if(p.pose==POSE_BATTLE_IDLE) at<float>(p.duel,0x30)=1.0f;
    if(p.ready && p.loop_phase_pending && (p.pose==POSE_IDLE || p.pose==POSE_MOVE)) {
        float phase=p.loop_offset;
        if(!p.loop_horse_pending) {
            void *handle=at<void *>(p.actor,0x98);
            void *setting=handle?at<void *>(handle,0x20):NULL;
            float current=setting?at<float>(setting,0x24):phase;
            if(isfinite(current) && current>=0.0f)phase=current;
        }
        at<float>(p.duel,0x30)=phase;
        if(p.loop_horse_pending)at<float>(p.duel,0x34)=phase;
        p.loop_phase_pending=false;
    }
    if(!p.defeated && p.pose==POSE_IDLE) {
        // Evaluate one standing frame, never an old attack or a partial blend.
        at<float>(p.duel,0x30)=at<float>(p.duel,0x34)=1.0f;
        p.loop_phase_pending=false;
        p.blend_count=p.blend_horse_count=p.blend_duration_ms=0;
        at<float>(p.duel,0xf0)=presentation_yaw(p,u);
    }
    ++private_presentation_depth;
    private_sound_proxy=&p;
    memset(p.model_cache,0,sizeof p.model_cache);
    private_clock_rate=0;
    // Catch up only the one held update of the SAME clip; never after a gap in
    // field updates (map/view change) and never for a clip that just restarted.
    if(p.half_skipped && p.eval_frame+2==FL_Status.frame && p.eval_transitions==p.motion_transitions &&
       p.eval_tick && animation_last_tick>p.eval_tick && performance_frequency) {
        double seconds=double(animation_last_tick-p.eval_tick)/double(performance_frequency);
        if(seconds>0 && seconds<0.25) private_clock_rate=float(1.0/seconds);
    }
    p.half_skipped=false;
    update_private_actor(p.actor,p.duel,engine,frame);
    private_clock_rate=0;
    p.eval_tick=animation_last_tick;p.eval_frame=FL_Status.frame;p.eval_transitions=p.motion_transitions;
    private_sound_proxy=NULL;
    --private_presentation_depth;
    p.ready = slot_ready(p,0) && slot_ready(p,1) && slot_ready(p,2);
    if(!p.defeated) observe_clip(p);
    if (p.ready && !p.color_ready_logged) {
        /* Read back each private wrapper after the engine has consumed its
           request. Completion is a native readiness observation, not a claim
           about pixel colors or the contents of a GPU material. */
        if (at<int>(p.actor,0x7c)!=p.color || at<int>(p.actor,0xac)!=p.horse_color) {
            if(p.body>=25) {
                appearance_retry_after[u.id]=GetTickCount64()+5000;
                release_proxy(p,slot,"extended material unavailable; other units continue");return;
            }
            p.ready=false; blocked=true; FL_Status.phase=FL_BLOCKED;
            FL_Status.reason=FL_INVALID_APPEARANCE;
            log_line("BLOCKED color selection changed slot=%u",slot); return;
        }
        p.color_ready_logged=true;
        FL_RosterAudit.rows[p.source.id].flags|=FL_ROSTER_COLOR_READY;
        log_line("COLOR_READY slot=%u officer=%u body=%d body_colour=%d horse_colour=%d native_pending=0 visual_verified=0",
                 slot,u.id,p.body,p.color,p.horse_color);
    }
    if (p.ready && !apply_field_scale(p,slot,engine)) {
        if(p.body>=25) {
            appearance_retry_after[u.id]=GetTickCount64()+5000;
            release_proxy(p,slot,"extended attachment unavailable; other units continue");return;
        }
        p.ready=false; blocked=true; FL_Status.phase=FL_BLOCKED; FL_Status.reason=FL_SCALE_FAILED;
        log_line("BLOCKED field scale/attachment contract slot=%u",slot);
        return;
    }
    if (p.ready && !p.last_snapshot_frame) {
        if (slot<FL_SNAPSHOT_SLOTS) snapshot_proxy(p,slot);
        p.last_snapshot_frame=FL_Status.frame;
    }
    FL_Status.body_handle[slot] = at<uint64_t>(p.actor, 0x58 + 0x10);
    FL_Status.horse_handle[slot] = at<uint64_t>(p.actor, 0x88 + 0x10);
    FL_Status.weapon_handle[slot] = at<uint64_t>(p.actor, 0xb8 + 0x10);
    if (!p.ready && FL_Status.frame - p.born_frame > 3600) {
        if(p.body>=25) {
            appearance_retry_after[u.id]=GetTickCount64()+5000;
            release_proxy(p,slot,"extended resource unavailable; other units continue");return;
        }
        log_line("BLOCKED model readiness timeout slot=%u",slot);
        blocked = true; FL_Status.phase = FL_BLOCKED; FL_Status.reason = FL_LOAD_TIMEOUT;
    }
    p.static_valid=p.ready && !p.defeated && p.pose==POSE_IDLE;
    p.evaluated_source=u;p.evaluated_actor=p.actor;
}

static void update_terminal(Proxy &p,unsigned slot,void *engine,void *frame) {
    if(p.terminal_done) return;
    if(!p.terminal_started) {
        p.terminal_started=true; p.terminal_ms=presentation_ms;
        p.running=p.combat_engaged=false;
        // All copied presentation fields remain private after native deletion.
        at<void *>(p.duel,0xb0)=NULL;
        if(p.captured) {
            // Original duel state6: action46/47, copy horse origin to rider
            // origin, stop saddle attachment. No new movement or animation.
            memcpy(p.duel+0xd0,p.duel+0xc0,16);
            at<int>(p.duel,0xfc)=0;
            set_pose(p,POSE_FALL,slot,true);
        } else set_pose(p,POSE_IDLE,slot,true);
    }
    if(!p.ready) { p.terminal_done=true; return; }
    update_proxy(p,p.source,slot,engine,frame);
    uint64_t elapsed=presentation_ms-p.terminal_ms;
    if(p.captured) {
        void *handle=at<void *>(p.actor,0x68),*setting=handle?at<void *>(handle,0x20):NULL;
        float frame_now=setting?at<float>(setting,0x24):0;
        bool wrapped=p.clip_frame>1 && frame_now<p.clip_frame;
        // All 50 original body46/47 clips were decoded: exactly120 frames.
        // A view/clock stall timeout prevents a departed actor leaking forever.
        p.terminal_done=(isfinite(frame_now) && frame_now>=119) || wrapped || elapsed>=6000;
        p.clip_frame=frame_now;
    } else p.terminal_done=elapsed>=300;
    if(p.terminal_done) log_line("R22_DEFEAT_DISPLAY_END officer=%u captured=%d original_troop_fallback=0",p.source.id,p.captured);
}

static unsigned refresh_budget,refresh_start_budget,refresh_active,refresh_started;
static void update_appearance(Proxy &p,unsigned slot,void *engine,void *frame) {
    if(p.defeated || !p.ready || !current_source(p)) return;
    unsigned bank=1-p.active_bank;
    unsigned char *next_actor=p.actor_storage[bank],*next_duel=p.duel_storage[bank];
    if(!p.refresh_loading) {
        if(!refresh_budget || !refresh_start_budget || refresh_active>=2 ||
           presentation_ms-p.appearance_check_ms<2000) return;
        --refresh_budget; p.appearance_check_ms=presentation_ms;
        init_duel(next_duel,p.source);
        if(!resolve_field_appearance(next_duel,p.source))return;
        int body=at<int>(next_duel,0x14),horse=at<int>(next_duel,0x18),weapon=at<int>(next_duel,0x1c);
        int color=at<int>(next_duel,0x20),horse_color=at<int>(next_duel,0x24);
        if(body==p.body && horse==p.horse && weapon==p.weapon && color==p.color && horse_color==p.horse_color) return;
        fn<Call1>(FL_ACTOR_CTOR)(next_actor);
        fn<Call2>(FL_ACTOR_CREATE)(next_actor,next_duel);
        fn<Call2>(FL_ACTOR_BIND)(next_actor,next_duel);
        at<uintptr_t>(next_actor,0)=(uintptr_t)(private_actor_vtable+1);
        fn<RequestColor>(FL_REQUEST_COLOR)(next_actor,0,color);
        fn<RequestColor>(FL_REQUEST_COLOR)(next_actor,1,horse_color);
        p.refresh_loading=true; p.refresh_born_frame=FL_Status.frame;
        --refresh_start_budget; ++refresh_active; ++refresh_started;
        log_line("R22_APPEARANCE_LOADING officer=%u old_model_retained=1",p.source.id);
    }
    // Two stable storage banks: NEVER memcpy a constructed native actor.
    // Old model remains the displayed/interactive replacement until ready.
    unsigned char *old_actor=p.actor,*old_duel=p.duel;
    bool old_body_ok=p.scale_body_ok,old_weapon_ok=p.scale_weapon_ok;
    uint32_t old_attach_frame=p.attachment_frame;
    memcpy(next_duel+0xc0,old_duel+0xc0,0x54);
    at<int>(next_duel,0x28)=at<int>(old_duel,0x28);
    at<int>(next_duel,0x2c)=at<int>(old_duel,0x2c);
    // Preserve each live native clock through an appearance bank swap.
    for(unsigned part=0;part<2;++part) {
        void *h=at<void *>(old_actor,0x68+part*0x30),*setting=h?at<void *>(h,0x20):NULL;
        float phase=setting?at<float>(setting,0x24):1.0f;
        // The native override uses >0 as its presence flag; keep subframes.
        at<float>(next_duel,0x30+part*4)=isfinite(phase)&&phase>=0?(phase>0?phase:0.000001f):1.0f;
    }
    p.actor=next_actor; p.duel=next_duel;
    ++private_presentation_depth; private_sound_proxy=&p; private_staging=true;
    memset(p.model_cache,0,sizeof p.model_cache);
    update_private_actor(next_actor,next_duel,engine,frame);
    private_staging=false; private_sound_proxy=NULL; --private_presentation_depth;
    bool done=slot_ready(p,0) && slot_ready(p,1) && slot_ready(p,2) && apply_field_scale(p,slot,engine) &&
        at<int>(next_actor,0x7c)==at<int>(next_duel,0x20) && at<int>(next_actor,0xac)==at<int>(next_duel,0x24);
    if(done) {
        p.active_bank=bank; p.refresh_loading=false; --refresh_active; ++appearance_refreshes;
        p.body=at<int>(next_duel,0x14); p.horse=at<int>(next_duel,0x18); p.weapon=at<int>(next_duel,0x1c);
        p.color=at<int>(next_duel,0x20); p.horse_color=at<int>(next_duel,0x24);
        p.color_ready_logged=false;
        fn<Call1>(FL_ACTOR_RELEASE)(old_actor); fn<Call1>(FL_ACTOR_DTOR)(old_actor);
        FL_Status.appearance[slot]=p.body;
        FLRosterRow &row=FL_RosterAudit.rows[p.source.id];
        row.body=(int16_t)p.body; row.weapon=(int16_t)p.weapon;
        row.body_color=(int16_t)p.color; row.horse_color=(int16_t)p.horse_color;
        log_line("R22_APPEARANCE_SWAPPED officer=%u body=%d color=%d horse_color=%d no_original_troop_flash=1",p.source.id,p.body,p.color,p.horse_color);
    } else {
        p.actor=old_actor; p.duel=old_duel;
        p.scale_body_ok=old_body_ok; p.scale_weapon_ok=old_weapon_ok; p.attachment_frame=old_attach_frame;
        if(FL_Status.frame-p.refresh_born_frame>3600) {
            fn<Call1>(FL_ACTOR_RELEASE)(next_actor); fn<Call1>(FL_ACTOR_DTOR)(next_actor);
            p.refresh_loading=false; --refresh_active;
            log_line("R22_APPEARANCE_RETAINED officer=%u load_timeout=1",p.source.id);
        }
    }
    // A failed staging pass must not leave alternate-bank models cached for
    // the next live pose transition, which runs before the actor update.
    memset(p.model_cache,0,sizeof p.model_cache);
}

static void update_mod(void *manager, void *engine, void *frame) {
    PresentationTimer timer(perf_update);
    presentation_ms=GetTickCount64();
    advance_animation_clock();
    draw_current_frame = false;
    hoof_selected=0;
    for(unsigned n=0;n<active_count;++n) { Proxy &p=proxies[active_slots[n]]; p.hoof_gain=0; p.combat_gain=0; }
    if (!configured) return;
    ++FL_Status.frame;
    if (ownership_lost) return;
    void *data = global(FL_DATA_MANAGER), *context = global(FL_RESOURCE_CONTEXT);
    void *app = global(FL_APP);
    if (!manager || !engine || !data || !context || !app || manager != global(FL_UNIT_MANAGER) ||
        at<void *>(app,0x210) != engine || !at<void *>(context,0x78) || !at<void *>(context,0x98)) {
        FL_Status.ready_count=0; FL_Status.reason = FL_NO_MAP; return;
    }
    if (owner_manager && (owner_manager != manager || owner_data != data || owner_context != context || owner_engine != engine)) {
        /* A teardown not covered by the observed hooks: stop without touching
           handles through a potentially destroyed owner. Restart required. */
        log_line("BLOCKED unexpected owner generation change");
        blocked = true; ownership_lost = true;
        FL_Status.phase = FL_BLOCKED; FL_Status.reason = FL_CONTEXT_CHANGED; return;
    }
    if (!on_owner_thread("field_update")) return;
    if (stop_requested) {
        release_all("user stopped test"); FL_Status.phase = FL_STOPPED;
        FL_Status.stopped = 1; return;
    }
    if (blocked) return;
    if (!owner_manager) reset_roster_audit();
    owner_manager = manager; owner_data = data; owner_context = context; owner_engine = engine;
    if (at<int>(manager,0x498)) { FL_Status.reason = FL_DUEL_OR_OTHER_VIEW; return; }
    if (!collect_units(manager,data)) {
        blocked = true; FL_Status.phase=FL_BLOCKED; FL_Status.reason=FL_BAD_LIST;
        log_line("BLOCKED invalid active unit list"); return;
    }
    /* Native top-state query, once per field update. A menu/cutscene is not
       treated as a paused battle merely because no damage arrived recently. */
    strategy_pause=fn<int (*)(void *,const char *)>(FL_TOP_STATE)
        ((void *)(base+FL_STATE_MANAGER),"CUserStrategyState")!=0;
    refresh_budget=4; refresh_start_budget=1; refresh_active=refresh_started=0;
    prewarm_budget=FL_PREWARM_PER_UPDATE;
    for(const Proxy &p:proxies) if(p.refresh_loading) ++refresh_active;
    bool retained[FL_PROXY_COUNT]={};
    int retained_candidate[FL_PROXY_COUNT];
    unsigned free_slots[FL_PROXY_COUNT], free_count=0, loading=0;
    clear_index(proxy_index);
    clear_index(army_index);
    active_count=0;
    FL_Status.target_count=candidate_count;
    FL_Status.ready_count=0;
    for(unsigned i=0;i<FL_PROXY_COUNT;++i) {
        Proxy &p=proxies[i];
        int candidate=p.constructed ? index_find(candidate_index,(uintptr_t)p.source.actor) : -1;
        retained_candidate[i]=-1;
        if(p.constructed && p.defeated) {
            if(candidate>=0 && matches(p,candidates[candidate]) && !p.source_detached) retained[candidate]=true;
            if(candidate>=0 && !matches(p,candidates[candidate])) p.source_detached=true;
            if(p.terminal_done && p.source_detached) release_proxy(p,i,"defeat animation finished and original detached");
            else {
                active_slots[active_count++]=i;
                if(!p.source_detached) index_insert(proxy_index,(uintptr_t)p.source.actor,i);
                continue;
            }
        }
        if(candidate<0 || !matches(p,candidates[candidate]))
            release_proxy(p,i,"unit absent, water, return or identity changed");
        if(p.constructed && !blocked) {
            active_slots[active_count++]=i;
            retained[candidate]=true;
            retained_candidate[i]=candidate;
            update_motion(p,candidates[candidate],i);
            p.source=candidates[candidate];
            index_insert(proxy_index,(uintptr_t)p.source.actor,i);
            index_insert(army_index,(uintptr_t)p.source.army,i);
        } else if(!p.constructed) free_slots[free_count++]=i;
        if(blocked) return;
    }
    /* Compute actual movement and the same global audio budget for every
       retained unit before ANY private actor can emit a hoof event. */
    prepare_hoof_mix();
    prepare_combat_mix();
    for(unsigned n=0;n<active_count;++n) {
        unsigned i=active_slots[n]; if(retained_candidate[i]<0) continue;
        Proxy &p=proxies[i];
        update_proxy(p,candidates[retained_candidate[i]],i,engine,frame);
        if(p.ready) trim_native_particles(p,p.source.actor);
        if(p.ready) ++FL_Status.ready_count; else ++loading;
        if(blocked) return;
    }
    // Round-robin keeps all 500 officers eligible without a per-frame rescan
    // through every appearance selector. No displayed model is dropped.
    if(loading+refresh_active>=FL_MAX_LOADING) refresh_start_budget=0;
    for(unsigned n=0;n<active_count;++n) {
        unsigned i=active_slots[(appearance_cursor+n)%active_count]; Proxy &p=proxies[i];
        if(p.constructed && p.defeated) update_terminal(p,i,engine,frame);
        else if(p.constructed) update_appearance(p,i,engine,frame);
        if(blocked) return;
    }
    appearance_cursor=active_count?(appearance_cursor+4)%active_count:0;
    unsigned created=0;
    for(unsigned c=0;c<candidate_count && created+refresh_started<FL_CREATE_PER_FRAME && loading+refresh_active<FL_MAX_LOADING && !blocked;++c) {
        if(retained[c] || presentation_ms<appearance_retry_after[candidates[c].id]) continue;
        if(created>=free_count) break; // Bounded final-death visuals can temporarily occupy a slot.
        unsigned slot=free_slots[created++];
        update_proxy(proxies[slot],candidates[c],slot,engine,frame);
        if(proxies[slot].ready) ++FL_Status.ready_count; else ++loading;
        if(proxies[slot].constructed) {
            active_slots[active_count++]=slot;
            index_insert(proxy_index,(uintptr_t)candidates[c].actor,slot);
            index_insert(army_index,(uintptr_t)candidates[c].army,slot);
        }
    }
    FL_Status.pending_count=candidate_count>FL_Status.ready_count?candidate_count-FL_Status.ready_count:0;
    if (!blocked) {
        audit_roster_step(data);
        FL_Status.reason = !candidate_count ? FL_NO_LAND : FL_Status.pending_count ? FL_NOT_READY : FL_OK;
        FL_Status.phase = FL_Status.ready_count && FL_Status.draw_frames ? FL_SUBMITTED : candidate_count ? FL_LOADING : FL_WAITING;
        draw_current_frame = true;
    }
}

extern "C" __declspec(dllexport) void FL_HookUpdate(void *manager,void *engine,void *frame) {
    bool exclusive=begin_engine_call("field_update");
    const uint64_t original_begin=performance_tick();
    fn<Call3>(FL_MANAGER_UPDATE)(manager,engine,frame);
    lock_status();
    ++perf_update_hooks;
    performance_record(perf_original_update,original_begin);
    if (exclusive) update_mod(manager,engine,frame);
    unlock_status();
    end_engine_call();
}
extern "C" __declspec(dllexport) void FL_HookDraw(void *manager,void *engine,void *callback) {
    if (!begin_engine_call("field_draw")) {
        fn<Call3>(FL_MANAGER_DRAW)(manager,engine,callback);
        end_engine_call(); return;
    }
    lock_status();
    deferred_count=0; replacement_callback=NULL; replacement_thread=0; draw_lookup_armed=false;
    if(!++draw_generation) { for(Proxy &p:proxies) { p.deferred_generation=0; p.replaced_generation=0; } ++draw_generation; }
    if (configured && !blocked && !ownership_lost && !stop_requested && draw_current_frame &&
        current_owners(manager,engine) && callback) {
        void *context=global(FL_RESOURCE_CONTEXT);
        void *handle=context ? at<void *>(context,0x98) : NULL;
        if (context==owner_context && handle && at<void *>(handle,0x20)==callback &&
            at<uintptr_t>(callback,0)==base+FL_ARRAY_VTABLE && on_owner_thread("field_draw")) {
            replacement_callback=callback; replacement_thread=GetCurrentThreadId();
            { PresentationTimer map_timer(perf_draw_setup);prepare_draw_lookup(); }
        }
    }
    unlock_status();
    const uint64_t original_begin=performance_tick();
    fn<Call3>(FL_MANAGER_DRAW)(manager,engine,callback);
    lock_status();
    performance_draw_sample();
    performance_record(perf_original_draw,original_begin);
    const uint64_t private_draw_begin=performance_tick();
    replacement_callback=NULL; replacement_thread=0; draw_lookup_armed=false;
    if (configured && !blocked && !stop_requested && draw_current_frame && current_owners(manager,engine) &&
        callback && !at<int>(manager,0x498) &&
        (*fn<unsigned char *(*)()>(FL_DRAW_FLAGS)() & 4)) {
        void *context=global(FL_RESOURCE_CONTEXT);
        void *handle=context ? at<void *>(context,0x98) : NULL;
        if (!handle || at<void *>(handle,0x20)!=callback || at<uintptr_t>(callback,0)!=base+FL_ARRAY_VTABLE) {
            blocked=true; FL_Status.phase=FL_BLOCKED; FL_Status.reason=FL_CALLBACK_CHANGED;
            log_line("BLOCKED draw callback no longer matches original field resource");
        } else {
            if (!FL_Status.callback_vtable_rva) {
                FL_Status.callback_vtable_rva=at<uintptr_t>(callback,0)-base;
                log_line("FIELD_CALLBACK vtable_rva=%llx",(unsigned long long)FL_Status.callback_vtable_rva);
            }
            bool submitted=false;
            for (unsigned n=0;n<active_count;++n) {
                unsigned i=active_slots[n]; Proxy &p=proxies[i];
                if(p.defeated) p.replaced_generation=draw_generation; // Never replay original troops after accepted defeat.
                if (!p.ready || p.terminal_done || !replacement_draw_eligible(p)) continue;
                if(p.visual_deferred) {
                    // A camera change after update must never draw a stale pose.
                    // Resume on the next owner update with its live frame object.
                    p.replaced_generation=draw_generation;continue;
                }
                uint32_t before=at<uint32_t>(callback,0x1c);
                ++perf_submissions;
                fn<Call3>(FL_ACTOR_DRAW)(p.actor,engine,callback);
                uint32_t after=at<uint32_t>(callback,0x1c);
                if (after<before || after-before!=3) {
                    ++perf_submit_failures;
                    log_line("BLOCKED append count slot=%u before=%u after=%u expected=3",i,before,after);
                    blocked=true; FL_Status.phase=FL_BLOCKED; FL_Status.reason=FL_APPEND_FAILED; break;
                }
                submitted=true;
                p.replaced_generation=draw_generation;
                p.last_draw_frame=FL_Status.frame;
                FL_RosterAudit.rows[p.source.id].flags|=FL_ROSTER_SUBMITTED;
                if (!p.submitted) {
                    p.submitted=true;
                    log_line("ARRAY_APPEND slot=%u before=%u after=%u",i,before,after);
                    log_line("SUBMITTED slot=%u officer=%u body=%d xyz=%.4f,%.4f,%.4f yaw=%.4f (not a screenshot assertion)",
                             i,p.source.id,p.body,p.source.x,p.source.y,p.source.z,p.source.yaw);
                }
            }
            if (submitted) {
                ++FL_Status.draw_frames;
                if (!blocked && FL_Status.ready_count>0) FL_Status.phase=FL_SUBMITTED;
            }
        }
    }
    DeferredDraw replay[FL_PROXY_COUNT*FL_DRAWS_PER_UNIT]; unsigned replay_count=0;
    for(unsigned i=0;i<deferred_count;++i) {
        const DeferredDraw &d=deferred_draws[i];
        /* After an overlapping teardown or lost generation, even a native
           source pointer may be stale. Quarantine it instead of replaying it. */
        if (ownership_lost || !current_owners(manager,engine)) continue;
        if (proxies[d.slot].defeated || (proxies[d.slot].replaced_generation==draw_generation)) {
            Proxy &p=proxies[d.slot];
            if(!p.replacement_logged){p.replacement_logged=true;log_line("TROOP_REPLACED slot=%u officer=%u original_visual_hidden=1",d.slot,p.source.id);}
        }
        else { replay[replay_count++]=d; ++perf_fallback_draws; }
    }
    deferred_count=0;
    performance_record(perf_draw,private_draw_begin);
    unlock_status();
    for(unsigned i=0;i<replay_count;++i)
        fn<Call3>(replay[i].function)(replay[i].object,engine,callback);
    end_engine_call();
}
static void before_release(const char *reason,void *object,void *engine,bool resources) {
    lock_status();
    draw_current_frame=false;
    /* These exact original teardown sites own destruction. Their worker TID
       may differ from the field driver. Exact owner identity and the whole
       native-call exclusion decide; a lifetime-fixed TID is not required. */
    bool has_owner=owner_manager!=NULL;
    bool same_owner=has_owner && engine==owner_engine &&
        object==(resources?owner_context:owner_manager);
    if (configured && has_owner && !same_owner && !ownership_lost) {
        blocked=true; ownership_lost=true; FL_Status.phase=FL_BLOCKED;
        FL_Status.reason=FL_CONTEXT_CHANGED;
        log_line("BLOCKED mismatched cleanup owner stage=%s",reason);
    }
    if (configured && same_owner && !ownership_lost) {
        log_line("OWNER_RELEASE_BEGIN stage=%s field_tid=%lu cleanup_tid=%lu",reason,
            (unsigned long)FL_Status.thread_id,(unsigned long)GetCurrentThreadId());
        release_all(reason);
        owner_manager=owner_data=owner_context=owner_engine=NULL;
        FL_Status.thread_id=0;
        if (stop_requested) { FL_Status.phase=FL_STOPPED; FL_Status.stopped=1; }
        else if (!blocked) { FL_Status.phase=FL_WAITING; FL_Status.reason=FL_NO_MAP; }
        log_line("OWNER_RELEASE %s",reason);
    }
    unlock_status();
}
extern "C" __declspec(dllexport) void FL_HookManagerRelease(void *manager,void *engine) {
    begin_engine_call("manager_cleanup");
    before_release("unit manager cleanup",manager,engine,false);
    fn<Call2>(FL_MANAGER_RELEASE)(manager,engine);
    end_engine_call();
}
extern "C" __declspec(dllexport) void FL_HookResourceRelease(void *context,void *engine) {
    begin_engine_call("resource_cleanup");
    before_release("application resources cleanup",context,engine,true);
    fn<Call2>(FL_RESOURCES_RELEASE)(context,engine);
    end_engine_call();
}
extern "C" __declspec(dllexport) DWORD WINAPI FL_Configure(void *parameter) {
    const FLConfig *c=static_cast<const FLConfig *>(parameter);
    if (configured || !c || c->protocol!=FL_PROTOCOL || !c->log_path[0] || c->log_path[1023]) return 0;
    base=(uintptr_t)GetModuleHandleW(NULL);
    if (base!=c->module_base) return 0;
    if(c->info_destroy_next) {
        uintptr_t module=(uintptr_t)GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
        if(!module||c->info_destroy_next!=module+fl_registry::info_hook||
           *reinterpret_cast<uintptr_t *>(base+fl_registry::info_slot)!=c->info_destroy_next)return 0;
    }
    info_destroy_next=c->info_destroy_next;
    log_file=CreateFileW(reinterpret_cast<const wchar_t *>(c->log_path),GENERIC_WRITE,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (log_file==INVALID_HANDLE_VALUE) return 0;
    FL_Status.protocol=FL_PROTOCOL; FL_Status.phase=FL_WAITING;
    for(unsigned i=0;i<FL_PROXY_COUNT;++i) { FL_Status.appearance[i]=-1; FL_Status.officer[i]=UINT32_MAX; }
    configured=true;
    log_line("HF4 static_idle=1 pause_standing=1 offscreen_animation_skip=1 fog_variant=%d shadows_unchanged=1 gameplay_unchanged=1 present_fps_measured=0",FL_FOG_VARIANT);
    log_line("HF5 FIELDPERF wake_radius=%.0f sleep_radius=%.0f ndc_bound=1.00 prewarm_per_update=%u half_rate_loops=attack,battle_idle,guard half_rate_max_step_ms=%.3f half_rate_min_sustained_fps=%.1f draw_skip=0 model_quality_unchanged=1",
        FL_VIEW_WAKE_RADIUS,FL_VIEW_SLEEP_RADIUS,FL_PREWARM_PER_UPDATE,FL_HALF_RATE_MAX_STEP_S*1000.0,1.0/FL_HALF_RATE_FRAME_S);
    log_line("CONFIGURED R24 registry_bridge=1 integrated_ui_cleanup_chain=1 batch2: normal_field_arcs_removed=1 native_troop_dust_removed=1 strong_vfx_preserved=1 info=between_legs capture_based_fall=1 staged_appearance_refresh=1 attachments_per_frame=3 disk_sync_flushes=0; normal=13 no_motion_vfx horse_idle=0; strong=19; independent_cosmetic_guard=35; tactic_hit=39 only; paused=original_top_state; contact_audio original_clash_or_hit; full visual/update quality retained; scale=3 capacity=500 create_budget=2 loading_budget=8");
    write_pending_log();
    return 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI FL_RequestStop(void *) {
    InterlockedExchange(&stop_requested,1); return 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID) { return TRUE; }
