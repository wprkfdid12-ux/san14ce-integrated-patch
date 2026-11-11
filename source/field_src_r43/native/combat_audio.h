#pragma once
/* Included after Proxy and owner helpers. All mutable presentation state is
   protected by state_lock. Hooks observe native events; original calls and
   their arguments/results are preserved. No gameplay object is written. */
static unsigned skill_notifications, damage_notifications, ambient_omissions;
static unsigned hoof_events, hoof_budget_rejections, hoof_selected;
static unsigned gait_probe_count,gait_probe_moving;
static constexpr unsigned HOOF_SOURCES=8, HOOF_CANDIDATES=32, HOOF_VOICES=16;
static constexpr float HOOF_TOTAL_GAIN=1.5f;
struct HoofVoice { uint64_t until; float gain; };
static HoofVoice hoof_voices[HOOF_VOICES];

static bool presentation_owner_live() {
    return configured && !blocked && !ownership_lost && !stop_requested && owner_manager &&
        owner_data==global(FL_DATA_MANAGER) && owner_context==global(FL_RESOURCE_CONTEXT) &&
        owner_manager==global(FL_UNIT_MANAGER) && current_owners(owner_manager,owner_engine);
}
static Proxy *bound_army(void *army) {
    /* The argument belongs to the current original callback. Never use an old
       source pointer to look up an army during a notification or sound pass. */
    if (!army || !presentation_owner_live()) return NULL;
    int slot=index_find(army_index,(uintptr_t)army);
    if (slot<0 || !at<uint8_t>(army,0x10)) return NULL;
    uint32_t id=at<uint16_t>(army,0x12);
    if (id>FL_MAX_OFFICER) return NULL;
    if (slot>=0) {
        Proxy &p=proxies[slot];
        if (p.constructed && !p.defeated && !p.source_detached && p.source.army==army && p.source.id==id &&
            p.source.person==at<void *>(owner_data,0x158+id*8) && current_source(p)) return &p;
    }
    return NULL;
}

#include "combat_r20.h"
#include "combat_ambient.h"

static void start_tactic(void *value,bool child) {
    lock_status();
    if (value && (!child || at<void *>(value,0)) && presentation_owner_live() && at<int>(value,8)==0x1b) {
        int id=at<int>(value,0xc);
        if (id>=0 && id<=500) {
            Proxy *p=bound_army(at<void *>(owner_data,0x7e838+size_t(id)*8));
            if (p && p->ready) {
                p->skill_pending=true; p->skill_pending_ms=GetTickCount64();
                ++p->skill_token; memset(p->tactic_targets,0,sizeof p->tactic_targets);
                ++p->skill_events; ++skill_notifications;
                if (p->skill_events<=4 || !(p->skill_events&(p->skill_events-1)))
                    log_line("TACTIC_EFFECT officer=%u events=%u cutscene=%d native_effect_stage=1",
                        p->source.id,p->skill_events,at<int>((void *)base,FL_TACTIC_CUTSCENE_ACTIVE)!=0);
            }
        }
    }
    unlock_status();
    tactic_guards(value);
}
extern "C" __declspec(dllexport) void FL_HookTacticEffect(void *value,void *numbers,void *other) {
    start_tactic(value,false);
    fn<Call3>(FL_TACTIC_EFFECT)(value,numbers,other);
}
extern "C" __declspec(dllexport) void FL_HookTacticChild(void *value,void *numbers,void *other) {
    start_tactic(value,true);
    fn<Call3>(FL_TACTIC_EFFECT)(value,numbers,other);
}
extern "C" __declspec(dllexport) void FL_HookTacticDamage(void *ui,int amount,void *army,int style) {
    lock_status();
    Proxy *p=bound_army(army);
    /* This exact call belongs exclusively to native tactic presentation.
       171520 places effect kind 1 in bucket 0 (style 34); kind 20 is bucket 1
       (style 2e). Native 177110 heals +16 using wounded +18, proving that the
       second bucket is recovery. Normal attacks never call this hook. */
    if (p && amount>0 && style==0x34) {
        uint64_t now=GetTickCount64();
        bool awaiting_contact=p->reaction_waiting && now<=p->reaction_deadline_ms;
        // A delayed number notification must not fabricate HIT while a known
        // immutable block/hit outcome still awaits the original attack contact.
        if (!awaiting_contact &&
            (!p->tactic_decided_ms || now<p->tactic_decided_ms || now-p->tactic_decided_ms>250)) {
            p->hit_pending=true; p->hit_pending_ms=now;
            p->hit_relation=0; p->hit_pending_sound_handled=false;
        }
        ++damage_notifications;
    }
    unlock_status();
    typedef void (*NotifyDamage)(void *,int,void *,int);
    fn<NotifyDamage>(FL_TACTIC_DAMAGE)(ui,amount,army,style);
}

struct AmbientHandoff { uint64_t serial; uint32_t frame; };
static thread_local AmbientHandoff ambient_pending[FL_PROXY_COUNT];
static thread_local void *pending_collector;
static thread_local uint32_t pending_frame;
static bool movement_sound_id(uint32_t id) {
    return (id>=0x99 && id<=0x9d) || (id>=0xa5 && id<=0xa7);
}
extern "C" __declspec(dllexport) int FL_HookAmbientEnqueue(void *collector,void *army,int tile,uint32_t sound) {
    bool omit=false;
    lock_status();
    Proxy *p=bound_army(army);
    if (p && movement_sound_id(sound)) {
        omit=p->ready && p->submitted && draw_current_frame && !at<int>(owner_manager,0x498) &&
             FL_Status.frame-p->last_draw_frame<=1;
        if (omit) {
            if (pending_collector!=collector || pending_frame!=FL_Status.frame) {
                memset(ambient_pending,0,sizeof ambient_pending);
                pending_collector=collector; pending_frame=FL_Status.frame;
            }
            ambient_pending[p-proxies]={p->serial,FL_Status.frame};
            if (p->ambient_collector!=collector) p->ambient_removed=false;
            p->ambient_collector=collector;
            ++ambient_omissions;
        } else { p->ambient_removed=false; p->hoof_gain=0; }
    }
    unlock_status();
    /* Success means that this source was handled, not that a native queue
       node/voice was created. Combat 9e/9f/a0 and water ab are not these IDs. */
    if (omit) return 1;
    typedef int (*Enqueue)(void *,void *,int,uint32_t);
    return fn<Enqueue>(FL_AMBIENT_ENQUEUE)(collector,army,tile,sound);
}
extern "C" __declspec(dllexport) void FL_HookAmbientCleanup(void *collector) {
    /* 405920 has already matched retained old/new entries. 404da0 removes
       ONLY its unmatched old list, including the omitted movement entries.
       Do not permit hoof events until this original cleanup has returned. */
    fn<Call1>(FL_AMBIENT_CLEANUP)(collector);
    lock_status();
    if(presentation_owner_live()) classify_combat_ambient(collector);
    if (pending_collector==collector) {
        if (presentation_owner_live()) for (unsigned i=0;i<FL_PROXY_COUNT;++i) {
            Proxy &p=proxies[i]; const AmbientHandoff &h=ambient_pending[i];
            if (h.serial && p.constructed && p.serial==h.serial &&
                p.ambient_collector==collector && FL_Status.frame-h.frame<=1)
                p.ambient_removed=true;
        }
        memset(ambient_pending,0,sizeof ambient_pending); pending_collector=NULL;
    }
    unlock_status();
}

static void set_pose(Proxy &p,Pose pose,unsigned slot,bool restart=false) {
    if (p.pose==pose && !restart) return;
    if(p.pose==POSE_ATTACK && pose==POSE_BATTLE_IDLE) capture_attack_recovery(p);
    else if(p.pose!=pose) capture_pose_transition(p,pose);
    else if(pose!=POSE_BATTLE_IDLE) p.blend_count=p.blend_horse_count=p.blend_duration_ms=0;
    p.pose=pose; p.pose_done=false; p.clip_frame=0;
    p.loop_phase_pending=false;
    p.pose_began_ms=presentation_ms;
    if(pose==POSE_BATTLE_IDLE) {
        uint32_t phase=(p.source.id+1)*0x9e3779b9u; phase^=phase>>16;
        // Mean 1500 ms (old wait 750); identity offset uses no gameplay RNG.
        p.paused_wait_ms=1350u+phase%301u;
    }
    if(restart) ++perf_pose_restarts;
    if(pose==POSE_ATTACK) p.normal_contact_done=false;
    if(pose==POSE_STRONG) p.strong_contact_done=false;
    ++p.motion_transitions;
    const int actions[]={55,3,13,19,39,13,35,at<int>(p.duel,0xf8)?46:47};
    const int horse=(pose==POSE_MOVE || pose==POSE_STRONG || pose==POSE_FALL) ? 1 : 0;
    p.loop_phase_pending=(pose==POSE_IDLE || pose==POSE_MOVE);
    p.loop_horse_pending=at<int>(p.duel,0x2c)!=horse;
    at<int>(p.duel,0x28)=actions[pose]; at<float>(p.duel,0x30)=1.0f;
    /* All 25 original ModelDopeSheets attach weapon display 48b2c677 to
       bone23 for 13/19/35/39 and bone24 for 3/55. 1c19c0 selects those bones
       using CDuelUnit+f8. Only this private duel instance is changed. */
    if(pose!=POSE_FALL) at<int>(p.duel,0xf8)=(pose==POSE_ATTACK || pose==POSE_BATTLE_IDLE || pose==POSE_STRONG || pose==POSE_HIT || pose==POSE_GUARD);
    /* Normal attacks and battle pauses keep the SAME idle horse clock.
       Do not restart the horse whenever only the rider changes action. */
    if (at<int>(p.duel,0x2c)!=horse) {
        at<int>(p.duel,0x2c)=horse; at<float>(p.duel,0x34)=1.0f;
    }
    if (pose==POSE_ATTACK) ++p.combat_cycles;
    if (pose==POSE_GUARD) ++p.guard_plays;
    unsigned n=p.motion_transitions;
    if (n<=12 || !(n&(n-1)))
        log_line("MOTION_CHANGE slot=%u officer=%u moving=%d pose=%d body=%d horse=%d transitions=%u",
            slot,p.source.id,p.running,int(pose),actions[pose],at<int>(p.duel,0x2c),n);
}
static int pose_priority(Pose pose,unsigned char relation) {
    if(pose==POSE_STRONG) return 5;
    if(pose==POSE_GUARD) return 2;
    if(pose==POSE_HIT) return relation==2 ? 3 : relation==1 ? 1 : 3;
    if(pose==POSE_ATTACK) return relation==2 ? 1 : relation==1 ? 3 : 1;
    return 0;
}
static void update_combat(Proxy &p,const Unit &u,unsigned slot,bool continuous) {
    uint64_t now=presentation_ms;
    if(strategy_pause && !p.defeated) {
        p.running=p.combat_engaged=false;
        p.skill_pending=p.hit_pending=p.guard_pending=p.normal_pending=false;
        p.reaction_waiting=false;p.opponent_serial=0;
        set_pose(p,POSE_IDLE,slot);return;
    }
    /* 1c9640 derives native attack state2 from Army+26 bit0 and +23==0.
       Retain that actual engagement during idle intervals; proximity alone
       is never interpreted as combat, and real travel remains movement. */
    p.combat_engaged=p.ready && continuous && (u.action==2 || (!p.running && u.engaged)) && live_combat_target(p);
    if(!continuous || (!p.combat_engaged && !p.reaction_waiting)) {
        p.opponent_serial=0; p.opponent_relation=p.opponent_target_relation=0;
    }
    if(p.reaction_waiting && now>p.reaction_deadline_ms) p.reaction_waiting=false;
    /* User amendment: hit pose ONLY for damaging tactics. No soldier-count
       polling, ordinary-attack reaction or periodic cosmetic hit. */
    bool hit=p.hit_pending && now>=p.hit_pending_ms && now-p.hit_pending_ms<=1000;
    p.hit_pending=false;
    bool guard=p.guard_pending && now>=p.guard_pending_ms && now-p.guard_pending_ms<=1000;
    p.guard_pending=false;
    bool normal=p.normal_pending && now>=p.normal_pending_ms && now-p.normal_pending_ms<=1000;
    p.normal_pending=false;
    bool skill=p.skill_pending && p.ready && now>=p.skill_pending_ms && now-p.skill_pending_ms<=2000;
    if (p.ready || now-p.skill_pending_ms>2000) p.skill_pending=false;
    // A fresh own tactic always begins NOW, even while the weaker side reacts.
    if(skill) { p.pose_relation=0; set_pose(p,POSE_STRONG,slot,true); return; }
    bool active=continuous && !p.pose_done;
    if(active && p.pose==POSE_STRONG) return;

    Pose next=p.running?POSE_MOVE:POSE_IDLE;
    unsigned char relation=p.opponent_relation;
    if(!relation) relation=hit?p.hit_relation:guard?p.guard_relation:0;
    if(p.combat_engaged) {
        if(p.pose==POSE_ATTACK) next=active?POSE_ATTACK:POSE_BATTLE_IDLE;
        else if(p.pose==POSE_BATTLE_IDLE && active) next=POSE_BATTLE_IDLE;
        else next=POSE_ATTACK;
    }
    // An unfinished reaction is a candidate, not an uninterruptible lock.
    if(active && (p.pose==POSE_HIT || p.pose==POSE_GUARD)) {
        next=p.pose; relation=p.pose_relation;
    }
    int rank=pose_priority(next,relation);
    if(normal && p.combat_engaged && pose_priority(POSE_ATTACK,p.opponent_relation)>rank) {
        next=POSE_ATTACK; relation=p.opponent_relation; rank=pose_priority(next,relation);
    }
    bool new_guard=guard && p.ready && (!p.last_guard_ms || now-p.last_guard_ms>=1600) &&
                   !(active && p.pose==POSE_GUARD);
    bool new_hit=hit && p.ready && (!p.last_hit_ms || now-p.last_hit_ms>=1600) &&
                 !(active && p.pose==POSE_HIT);
    // Against an ordinary swing compare the actual incoming source's totals,
    // which may differ from the remembered outgoing target in a multi-unit fight.
    int guard_threshold=next==POSE_ATTACK && p.guard_relation ? pose_priority(POSE_ATTACK,p.guard_relation):rank;
    if(new_guard && pose_priority(POSE_GUARD,p.guard_relation)>guard_threshold) {
        next=POSE_GUARD; relation=p.guard_relation; rank=pose_priority(next,relation);
    }
    int hit_threshold=next==POSE_ATTACK && p.hit_relation ? pose_priority(POSE_ATTACK,p.hit_relation):rank;
    if(new_hit && pose_priority(POSE_HIT,p.hit_relation)>hit_threshold) {
        next=POSE_HIT; relation=p.hit_relation;
    }
    bool starting=next!=p.pose || !active;
    if(starting && next==POSE_GUARD) p.last_guard_ms=now;
    if(starting && next==POSE_HIT) {
        p.last_hit_ms=now; ++p.hit_events; p.hit_sound_handled=p.hit_pending_sound_handled;
    }
    p.pose_relation=relation;
    set_pose(p,next,slot,starting && next==p.pose && next!=POSE_IDLE && next!=POSE_MOVE);
}
static float pose_facing_offset(const Proxy &p) {
    bool side=p.pose==POSE_ATTACK || p.pose==POSE_BATTLE_IDLE ||
              ((p.pose==POSE_HIT || p.pose==POSE_GUARD) && p.combat_engaged);
    float target=side?-1.5707963267948966f:0;
    if(p.blend_duration_ms && presentation_ms>=p.blend_began_ms) {
        uint64_t elapsed=presentation_ms-p.blend_began_ms;
        if(elapsed<p.blend_duration_ms) {
            float t=float(elapsed)/p.blend_duration_ms;t=t*t*(3-2*t);
            return p.blend_facing_from+(target-p.blend_facing_from)*t;
        }
    }
    return target;
}
static float presentation_yaw(const Proxy &p,const Unit &u) {
    /* The game already supplies the active field facing at actor+744.
       Original mounted yaw is in radians (373000 writes pi or zero).
       Put native body13's lateral sweep on that bearing by turning only our
       mounted presentation a quarter turn. Strong/movement use field-facing
       directly. Never turn ArmyData, pick a nearby ally, or move the unit. */
    float yaw=u.yaw+pose_facing_offset(p);
    return remainderf(yaw,6.2831853071795865f);
}
static void observe_clip(Proxy &p) {
    if (!p.ready || (p.pose!=POSE_STRONG && p.pose!=POSE_HIT &&
                     p.pose!=POSE_ATTACK && p.pose!=POSE_BATTLE_IDLE && p.pose!=POSE_GUARD)) return;
    if(p.pose==POSE_BATTLE_IDLE) {
        p.pose_done=presentation_ms-p.pose_began_ms>=(strategy_pause?p.paused_wait_ms:750u);
        return;
    }
    void *handle=at<void *>(p.actor,0x68);
    if (!handle) return;
    void *setting=at<void *>(handle,0x20);
    if (!setting) return;
    /* Exact bcc780 getter: frame is setting+24. Read after the original
       update, not a wall-clock approximation or a synthetic animation tick. */
    float frame=at<float>(setting,0x24);
    if (!isfinite(frame) || frame<0) { p.pose_done=true; return; }
    const unsigned hit80=(1u<<1)|(1u<<2)|(1u<<6)|(1u<<17)|(1u<<18)|(1u<<19)|(1u<<20);
    bool long_hit=p.body>=0 && p.body<25 && (hit80&(1u<<unsigned(p.body)));
    float end=p.pose==POSE_STRONG ? 180.0f : (p.pose==POSE_ATTACK || p.pose==POSE_GUARD) ? 60.0f :
              p.pose==POSE_BATTLE_IDLE ? 45.0f : long_hit ? 80.0f : 60.0f;
    if (frame>=end-1 || (p.clip_frame>1 && frame<p.clip_frame)) p.pose_done=true;
    p.clip_frame=frame;
}

static void prepare_hoof_mix() {
    struct Nearby { unsigned slot; float distance; } ranked[HOOF_CANDIDATES];
    unsigned count=0;
    hoof_selected=0;
    for (unsigned n=0;n<active_count;++n) {
        unsigned i=active_slots[n];
        Proxy &p=proxies[i]; p.hoof_gain=0;
        if (!p.constructed || !p.ready || !p.running || p.pose!=POSE_MOVE || !p.ambient_removed ||
            !p.submitted || FL_Status.frame-p.last_draw_frame>1) continue;
        float distance=shared_camera_distance(p);
        if (!isfinite(distance) || distance<0) continue;
        unsigned at=0;
        while (at<count && ranked[at].distance<=distance) ++at;
        if (at>=HOOF_CANDIDATES) continue;
        if (count<HOOF_CANDIDATES) ++count;
        for (unsigned j=count-1;j>at;--j) ranked[j]=ranked[j-1];
        ranked[at]={i,distance};
    }
    for (unsigned n=0;n<count && hoof_selected<HOOF_SOURCES;++n) {
        Proxy &p=proxies[ranked[n].slot];
        /* 113e0 compares projected x/y with +/- this bound and z with 0..bound.
           It is a clip-space extent, NOT added padding: zero rejects all. */
        if (!shared_camera_visible(p)) continue;
        float d=ranked[n].distance/8000.0f;
        p.hoof_gain=0.65f/(1.0f+d*d); ++hoof_selected;
    }
    float density=hoof_selected ? 1.0f/sqrtf(float(hoof_selected)) : 0;
    for(unsigned n=0;n<active_count;++n) proxies[active_slots[n]].hoof_gain*=density;
}
static unsigned hoof_duration(uint32_t program) {
    /* Original finite samples with pitch 0. Conservative reservations include
       an extra 150ms for dispatch/tail. IDs remain owned by SoundDopeSheet;
       retaining or stopping recycled native IDs would be unsafe. */
    switch(program) {
    case 0xe3c22636:return 450;
    case 0xc5540415:return 708;
    case 0xa6e5e1f4:return 711;
    case 0x8877bfd3:return 622;
    default:return 0;
    }
}
/* Original ASRS 11de6328: 273f6ffd selects the collision sample family.
   A member (5ebb7e22) matches the supplied duel's guard contact at 5.428s.
   Guard35 has no own SoundDopeSheet: reuse the attacker's fresh callback,
   with the same collision program for normal attacks and guards.
   6fd54d22 is the original hit39 event. No native callback pointer is cached. */
static constexpr uint32_t CLASH_PROGRAM=0x273f6ffd, HIT_PROGRAM=0x6fd54d22;
static constexpr unsigned COMBAT_VOICES=24, NORMAL_VOICES=12, COMBAT_SOURCES=16;
static HoofVoice combat_voices[COMBAT_VOICES];
static unsigned normal_audio,guard_audio,tactic_block_audio,tactic_hit_audio,combat_audio_capped;
static void prepare_combat_mix() {
    struct Near { unsigned slot; float distance; } ranked[32];
    unsigned count=0,selected=0;
    for(unsigned n=0;n<active_count;++n) {
        unsigned i=active_slots[n];
        Proxy &p=proxies[i]; p.combat_gain=0;
        if(!p.constructed || !p.ready || !p.submitted || FL_Status.frame-p.last_draw_frame>1 ||
           (!p.combat_engaged && p.pose!=POSE_STRONG && p.pose!=POSE_HIT && !p.reaction_waiting)) continue;
        float d=shared_camera_distance(p);
        if(!isfinite(d) || d<0) continue;
        unsigned at=0; while(at<count && ranked[at].distance<=d) ++at;
        if(at>=32) continue;
        if(count<32) ++count;
        for(unsigned k=count-1;k>at;--k) ranked[k]=ranked[k-1];
        ranked[at]={i,d};
    }
    for(unsigned n=0;n<count && selected<COMBAT_SOURCES;++n) {
        Proxy &p=proxies[ranked[n].slot];
        if(!shared_camera_visible(p)) continue;
        float d=ranked[n].distance/8000.0f;
        p.combat_gain=1.0f/(1.0f+d*d); ++selected;
    }
    const float density=selected?1.0f/sqrtf(float(selected)):0;
    for(unsigned n=0;n<active_count;++n) proxies[active_slots[n]].combat_gain*=density;
}
static bool play_combat_sound(uint32_t rva,void *a,void *b,void *c,void *d,uint32_t e,uint32_t f,
        void *params,uint32_t *out,void *bank,Proxy &p,uint32_t program,float level,bool loud) {
    if(!p.ready || !out || p.combat_gain<=0 || !presentation_owner_live()) return false;
    const uint64_t now=GetTickCount64();
    float used=0,normal_used=0; int free_slot=-1;
    for(unsigned i=0;i<COMBAT_VOICES;++i) {
        if(combat_voices[i].until<=now) {
            if(free_slot<0 && (loud || i<NORMAL_VOICES)) free_slot=int(i);
        } else {
            used+=combat_voices[i].gain;
            if(i<NORMAL_VOICES) normal_used+=combat_voices[i].gain;
        }
    }
    float original_gain=at<float>(params,8);
    if(!isfinite(original_gain) || original_gain<=0 || original_gain>4) return false;
    float gain=original_gain*level*p.combat_gain;
    gain=fminf(gain,2.4f-used);
    if(!loud) gain=fminf(gain,0.8f-normal_used);
    if(free_slot<0 || !isfinite(gain) || gain<0.005f) { ++combat_audio_capped; return false; }
    alignas(16) unsigned char local[52]={};
    memcpy(local,params,rva==FL_EVENT_SOUND_3D?52:44);
    /* e5002c/e5036c prove program at +0, pitch integer at +4 and gain at +8.
       All native position/bank/dispatch fields remain exactly as received. */
    at<uint32_t>(local,0)=program; at<int>(local,4)=0; at<float>(local,8)=gain;
    bool played=fn<EventSound>(rva)(a,b,c,d,e,f,local,out,bank);
    if(played) {
        combat_voices[free_slot]={now+(program==CLASH_PROGRAM?4500u:900u),gain};
        p.last_combat_sound_frame=FL_Status.frame;
    }
    return played;
}
static bool event_sound(uint32_t rva,void *a,void *b,void *c,void *d,uint32_t e,uint32_t f,
                        void *params,uint32_t *out,void *bank) {
    if (!private_presentation_depth) return fn<EventSound>(rva)(a,b,c,d,e,f,params,out,bank);
    Proxy *p=private_sound_proxy;
    if(private_staging || (p && (p->visual_deferred || (FL_FOG_VARIANT && p->fog_hidden)))) { if(out) *out=0; return false; }
    if(p && params && p->ready) {
        uint32_t program=at<uint32_t>(params,0);
        bool contact=false,loud=false; uint32_t replacement=0; float level=0;
        unsigned *counter=NULL;
        if(p->pose==POSE_ATTACK && program==0x762497dc && !p->normal_contact_done) {
            normal_contact(*p); contact=true; replacement=CLASH_PROGRAM;
            level=p->normal_blocked?0.20f:0.14f;
            counter=p->normal_blocked?&guard_audio:&normal_audio;
        } else if(p->pose==POSE_STRONG && program==0xdd622565 && !p->strong_contact_done) {
            unsigned contacts[FL_PROXY_COUNT];
            unsigned count=strong_contact(*p,contacts); bool any=false;
            if(out) *out=0;
            for(unsigned k=0;k<count;++k) {
                Proxy &target=proxies[contacts[k]];
                uint32_t voice=0;
                uint32_t sound=target.reaction_blocked?CLASH_PROGRAM:HIT_PROGRAM;
                // Contact timing belongs to the original attack event. A victim
                // may simultaneously perform its own strong attack; sound must
                // not depend on whether its reaction won motion arbitration.
                if(play_combat_sound(rva,a,b,c,d,e,f,params,&voice,bank,target,sound,0.85f,true)) {
                    if(target.reaction_blocked) ++tactic_block_audio; else ++tactic_hit_audio;
                    if(!any && out) *out=voice;
                    any=true;
                }
            }
            return any;
        } else if(p->pose==POSE_HIT && program==HIT_PROGRAM && !p->hit_sound_handled) {
            p->hit_sound_handled=true;
            contact=true; loud=true; replacement=HIT_PROGRAM; level=0.85f; counter=&tactic_hit_audio;
        }
        if(contact) {
            bool played=replacement && play_combat_sound(rva,a,b,c,d,e,f,params,out,bank,*p,replacement,level,loud);
            if(played && counter) ++*counter;
            if(!played && out) *out=0;
            return played;
        }
    }
    unsigned duration=params ? hoof_duration(at<uint32_t>(params,0)) : 0;
    /* Bounded diagnostics include the corrected native program/pitch layout. */
    if (p && params) {
        unsigned &samples=(p->running && p->pose==POSE_MOVE)?gait_probe_moving:gait_probe_count;
        if (samples<16) {
            ++samples;
            log_line("GAIT_EVENT officer=%u moving=%d pose=%d program=%08x gain=%.4f pitch=%d allowed_ms=%u mix=%.4f cleanup=%d",
                p->source.id,p->running,int(p->pose),at<uint32_t>(params,0),at<float>(params,8),
                at<int>(params,4),duration,p->hoof_gain,p->ambient_removed);
        }
    }
    if (p && duration && params && out && p->hoof_gain>0 && p->running && p->pose==POSE_MOVE &&
        p->ready && p->ambient_removed && presentation_owner_live()) {
        uint64_t now=GetTickCount64(); float used=0; int free_slot=-1;
        for (unsigned i=0;i<HOOF_VOICES;++i) {
            if (hoof_voices[i].until<=now) { if(free_slot<0) free_slot=int(i); }
            else used+=hoof_voices[i].gain;
        }
        float original=at<float>(params,8), gain=original*p->hoof_gain;
        if (gain>HOOF_TOTAL_GAIN-used) gain=HOOF_TOTAL_GAIN-used;
        /* Native pitch is zero in these exact gait sheets. Unknown pitch or
           malformed gain is not admitted into a duration-bounded voice pool. */
        if (free_slot>=0 && isfinite(original) && original>=0 && original<=1 &&
            at<int>(params,4)==0 && gain>0.005f) {
            alignas(16) unsigned char local[52]={};
            memcpy(local,params,rva==FL_EVENT_SOUND_3D?52:44);
            at<float>(local,8)=gain;
            bool played=fn<EventSound>(rva)(a,b,c,d,e,f,local,out,bank);
            if (played) {
                hoof_voices[free_slot]={now+duration,gain}; ++hoof_events;
            }
            return played;
        }
        ++hoof_budget_rejections;
    }
    if (out) *out=0;
    ++suppressed_motion_sounds;
    return false; // Original false branch skips registration of a sound ID.
}
