#define FL_PORTABLE_TEST
#include "../field_poc.cpp"
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <thread>
#include <condition_variable>

/* Exercise the actual DLL's control logic with a mocked original engine.
   This deliberately does not pretend to execute the game's graphics engine. */
static std::vector<unsigned char> image(FL_IMAGE_SIZE);
alignas(16) static unsigned char manager[0x4a0], data[0x80000], context[0x1a0], app[0x220], engine[0x200];
alignas(16) static unsigned char field[FL_PROXY_COUNT+1][0x8f0], armies[FL_PROXY_COUNT+1][0x200], people[FL_PROXY_COUNT+1][0x1f0];
alignas(16) static unsigned char handles[FL_PROXY_COUNT][5][0x40], settings[FL_PROXY_COUNT][5][0x70], children[FL_PROXY_COUNT][5][0x30];
alignas(16) static unsigned char models[FL_PROXY_COUNT][5][0x370];
alignas(16) static unsigned char attachments[FL_PROXY_COUNT][2][0x40];
alignas(16) static unsigned char alt_handles[FL_PROXY_COUNT][5][0x40],alt_settings[FL_PROXY_COUNT][5][0x70],alt_children[FL_PROXY_COUNT][5][0x30];
alignas(16) static unsigned char alt_models[FL_PROXY_COUNT][5][0x370],alt_attachments[FL_PROXY_COUNT][2][0x40];
static unsigned char callback[0x30], callback_handle[0x40];
static std::vector<int> events;
static bool load_ready;
static bool append_ok=true;
static unsigned append_limit=UINT32_MAX;
static bool attachment_ok=true;
static unsigned position_reads,scale_reads,body_attachment_calls,weapon_attachment_calls;
static unsigned selections, constructions, destructions, removals, submissions;
static unsigned inventory_selections, color_requests, color_hold_mask;
static bool appearance_override, delayed_color;
static int applied_color[FL_PROXY_COUNT][2];
static unsigned original_visuals[FL_PROXY_COUNT+1][8], other_visuals;
static unsigned field_count, never_ready_id=UINT32_MAX;
static bool stop_during_draw, nested_update, nested_cleanup, hold_native_draw;
static std::mutex rendezvous;
static std::condition_variable rendezvous_cv;
static bool native_draw_waiting, resume_native_draw;
static unsigned char flags=4;
static void *frame_arg=(void *)0x12340000;
static bool emit_motion_sound;
static unsigned native_sound_calls;
static bool emit_hoof_sound, sound_failure;
static bool mock_strategy_pause;
static unsigned visible_checks, distance_checks, ambient_calls, ambient_cleans, tactic_calls, damage_calls;
static bool old_movement_voice;
static float last_native_gain;
static unsigned char last_native_params[52];
static float mock_motion_step=1;
static float mock_seen_rate[FL_PROXY_COUNT]; static unsigned mock_tick_calls[FL_PROXY_COUNT];
static unsigned char hoof_params[52];
static float motion_override_seen[FL_PROXY_COUNT][2];
static bool native_sound(void *a,void *b,void *c,void *d,uint32_t e,uint32_t f,
                         void *params,uint32_t *out,void *bank) {
    assert(a==manager && b==app && c==engine && d==context && e==17 && f==29);
    assert(bank==callback_handle && out);
    if(params!=callback) {
        assert(!old_movement_voice);
        memcpy(last_native_params,params,52);
        last_native_gain=at<float>(params,8);
    }
    ++native_sound_calls; *out=12345; return !sound_failure;
}
static bool sound_call(bool positional,uint32_t *out) {
    return (positional?FL_HookEventSound3D:FL_HookEventSound2D)
        (manager,app,engine,context,17,29,callback,out,callback_handle);
}

static void original_update(void *,void *,void *) { assert(state_lock.depth==0); events.push_back(1); }
static void original_visual(void *object,void *,void *) {
    assert(state_lock.depth==0);
    const unsigned offsets[]={0x750,0x778,0x7c8,0x7f0,0x580,0x818,0x7a0,0x840};
    uintptr_t diff=(uintptr_t)object-(uintptr_t)field;
    unsigned i=(unsigned)(diff/sizeof field[0]), offset=(unsigned)(diff%sizeof field[0]);
    if(i<field_count) for(unsigned j=0;j<8;++j) if(offset==offsets[j]) {
        ++original_visuals[i][j]; return;
    }
    ++other_visuals;
}
static void original_draw(void *m,void *e,void *cb) {
    assert(state_lock.depth==0); events.push_back(2);
    memset(original_visuals,0,sizeof original_visuals); other_visuals=0;
    if (!(flags&4) || at<int>(m,0x498)) return;
    void *a=at<void *>(m,0x18); unsigned nodes=0;
    while(a && ++nodes<=FL_PROXY_COUNT+1) {
        unsigned mode=at<unsigned>(a,0x740);
        auto group=[&](unsigned offset) { FL_HookFieldGroupDraw((unsigned char *)a+offset,e,cb); };
        if(mode==0 || mode==3) {
            if(mode==3) group(0x840);
            group(0x750); group(0x778); group(0x7c8);
        }
        if(mode==1) group(0x818);
        if(mode==2 || mode==3) group(0x7a0);
        if(at<int>(a,0x748)) group(0x7f0);
        if(mode==0) FL_HookFieldActionDraw((unsigned char *)a+0x580,e,cb);
        a=at<void *>(a,0x8e0);
    }
    if (hold_native_draw) {
        std::unique_lock<std::mutex> lock(rendezvous);
        native_draw_waiting=true; rendezvous_cv.notify_all();
        rendezvous_cv.wait(lock,[]{return resume_native_draw;});
    }
    if (nested_update) { nested_update=false; FL_HookUpdate(manager,engine,frame_arg); }
    if (nested_cleanup) { nested_cleanup=false; FL_HookManagerRelease(manager,engine); }
    if(stop_during_draw) FL_RequestStop(NULL);
}
static void original_release(void *,void *) { assert(state_lock.depth==0); events.push_back(3); }
static int select_body(void *d) {
    void *p=at<void *>(d,0xb0); assert(p); assert(at<uint32_t>(d,0xa8)==at<uint16_t>(p,0x10));
    return at<uint16_t>(p,0x10)==596?3:at<uint8_t>(p,0xe6)==1?11:12;
}
static void current_appearance(void *d) {
    if ((uintptr_t)d>=(uintptr_t)proxies && (uintptr_t)d<(uintptr_t)(proxies+FL_PROXY_COUNT)) {
        ++selections; events.push_back(10);
    } else ++inventory_selections;
    at<int>(d,0x14)=select_body(d); at<int>(d,0x18)=0; at<int>(d,0x1c)=2;
    if(appearance_override) {
        void *p=at<void *>(d,0xb0);
        at<int>(d,0x14)=at<int>(p,0x150); at<int>(d,0x20)=at<int>(p,0x154);
        at<int>(d,0x24)=at<int>(p,0x158); at<int>(d,0x1c)=at<int>(p,0x15c);
    }
}
static unsigned actor_slot(void *a) {
    uintptr_t diff=(uintptr_t)a-(uintptr_t)proxies;
    unsigned slot=(unsigned)(diff/sizeof proxies[0]);
    assert(slot<FL_PROXY_COUNT && (a==proxies[slot].actor_storage[0] || a==proxies[slot].actor_storage[1])); return slot;
}
static void construct(void *a) { assert((uintptr_t)a%16==0); ++constructions; events.push_back(11); }
static void create(void *a,void *d) {
    Proxy &p=proxies[actor_slot(a)]; unsigned bank=a==p.actor_storage[1];
    assert(d==p.duel_storage[bank]); events.push_back(12);
    assert(at<int>(d,0x28)==55 && at<int>(d,0x2c)==0);
    assert(at<int>(d,0xfc)==1 && at<int>(d,0x108)==1 && at<int>(d,0x110)==1 && at<int>(d,0x114)==0);
    at<void *>(a,0x40)=d;
}
static void bind(void *,void *) { events.push_back(13); }
static void request_color(void *a,int part,int color) {
    assert(part==0 || part==1); ++color_requests; events.push_back(18+part);
    void *d=at<void *>(a,0x40); assert(d==proxies[actor_slot(a)].duel_storage[a==proxies[actor_slot(a)].actor_storage[1]]);
    int body=at<int>(d,0x14);
    int secondary=part?0:body==14?5:body>=0 && body<=24 && ((0x180b000u>>body)&1)?4:0;
    void *w=(unsigned char *)a+0x58+part*0x30;
    at<int>(w,0x24)=color; at<int>(w,0x2c)=secondary; at<int>(w,0x28)=1;
}
static void get_model_scale(void *m,void *out) {
    void *parent=at<void *>(m,0x1c0);
    for(unsigned i=0;i<3;++i)
        ((float *)out)[i]=at<float>(m,0x34+4*i)*(parent?at<float>(parent,0x10+4*i):1.0f);
}
static void get_model_position(void *m,void *out) {
    void *parent=at<void *>(m,0x1c0);
    for(unsigned i=0;i<3;++i)
        ((float *)out)[i]=parent?at<float>(parent,0x30+4*i)+at<float>(m,0x18+4*i)*at<float>(parent,0x10+4*i):at<float>(m,0x18+4*i);
}
static void set_model_scale(void *m,void *value) {
    void *parent=at<void *>(m,0x1c0);
    for(unsigned i=0;i<3;++i)
        at<float>(m,0x34+4*i)=((float *)value)[i]/(parent?at<float>(parent,0x10+4*i):1.0f);
}
static int attach_part(void *a,void *e,unsigned child,unsigned parent_part,unsigned attachment,const float *joint) {
    unsigned bank=a==proxies[actor_slot(a)].actor_storage[1];
    auto &models=bank?alt_models: ::models;
    auto &attachments=bank?alt_attachments: ::attachments;
    assert(e==engine);
    if (!attachment_ok) return 0;
    unsigned s=actor_slot(a); float scale[3],position[3];
    get_model_scale(models[s][parent_part],scale); get_model_position(models[s][parent_part],position);
    void *p=attachments[s][attachment];
    at<int>(p,8)=-1;
    for(unsigned i=0;i<3;++i) { at<float>(p,0x10+4*i)=scale[i]; at<float>(p,0x30+4*i)=position[i]+joint[i]*scale[i]; }
    at<void *>(models[s][child],0x1c0)=p; return 1;
}
static int attach_body(void *a,void *e) {
    ++body_attachment_calls;
    const float saddle[3]={0,205,43}; return attach_part(a,e,0,1,0,saddle);
}
static int attach_weapon(void *a,void *e) {
    ++weapon_attachment_calls;
    const float hand[3]={-64,-5,-22}; return attach_part(a,e,2,0,1,hand);
}
static void tick(void *a,void *e,void *f) {
    unsigned bank=a==proxies[actor_slot(a)].actor_storage[1];
    auto &handles=bank?alt_handles: ::handles;
    auto &settings=bank?alt_settings: ::settings;
    auto &children=bank?alt_children: ::children;
    auto &models=bank?alt_models: ::models;
    assert(e==engine && f==frame_arg); events.push_back(14);
    unsigned slot=actor_slot(a);
    assert(private_presentation_depth==1);
    mock_seen_rate[slot]=FL_HookAnimationRate(NULL); ++mock_tick_calls[slot];
    for(unsigned k=0;k<2;++k) {
        motion_override_seen[slot][k]=at<float>(proxies[slot].duel,0x30+4*k);
        if (motion_override_seen[slot][k]>0) at<float>(settings[slot][k],0x24)=motion_override_seen[slot][k];
        else at<float>(settings[slot][k],0x24)+=mock_motion_step;
        at<float>(proxies[slot].duel,0x30+4*k)=0; // Original update consumes it.
    }
    if(emit_motion_sound) {
        uint32_t id=99; assert(!sound_call(false,&id) && id==0);
        id=99; assert(!sound_call(true,&id) && id==0);
    }
    if(emit_hoof_sound) {
        uint32_t id=99;
        FL_HookEventSound3D(manager,app,engine,context,17,29,hoof_params,&id,callback_handle);
    }
    for (unsigned k=0;k<5;++k) {
        unsigned char *s=(unsigned char *)a+0x58+k*0x30;
        at<int>(s,0x18)=load_ready && proxies[slot].source.id!=never_ready_id;
        at<void *>(s,0x10)=handles[slot][k];
        at<void *>(handles[slot][k],0x20)=settings[slot][k];
        at<void *>(settings[slot][k],0x30)=children[slot][k];
        at<void *>(children[slot][k],0x20)=models[slot][k];
        if (k<2 && at<int>(s,0x18) && at<int>(s,0x28)) {
            bool hold=(color_hold_mask&(1u<<k))!=0;
            int body=at<int>(proxies[slot].duel,0x14);
            bool common=(body>=11 && body<=16) || body>=22;
            // Original 3717c0 cannot resolve row zero when a fixed body has
            // no variation rows. Base preservation must retain sentinel -1.
            if (k==0 && !common && at<int>(s,0x24)>=0) hold=true;
            if (!hold) { applied_color[slot][k]=at<int>(s,0x24); at<int>(s,0x28)=0; }
            if (hold || delayed_color) at<int>(s,0x18)=0;
        }
    }
    if (load_ready && proxies[slot].source.id!=never_ready_id) {
        memcpy(models[slot][1]+0x18,proxies[slot].duel+0xc0,12);
        if(at<int>(proxies[slot].duel,0xfc)) attach_body(a,e);
        else {
            at<void *>(models[slot][0],0x1c0)=NULL;
            memcpy(models[slot][0]+0x18,proxies[slot].duel+0xd0,12);
        }
        float original_scale=slot?0.95f:1.0f;
        float s[3]={original_scale,original_scale,original_scale};
        set_model_scale(models[slot][0],s);
        auto vt=at<uintptr_t *>(a,0);
        reinterpret_cast<int (*)(void *,void *)>(vt[0x68/8])(a,e);
    }
}
static void draw(void *a,void *e,void *cb) {
    assert(e==engine && cb==callback); assert(proxies[actor_slot(a)].ready);
    ++submissions; events.push_back(15); if(append_ok && submissions<=append_limit)at<uint32_t>(cb,0x1c)+=3;
}
static void model_position(void *m,void *out) { ++position_reads; get_model_position(m,out); }
static void model_scale(void *m,void *out) { ++scale_reads; get_model_scale(m,out); }
static void release(void *a) { actor_slot(a); ++removals; events.push_back(16); }
static void destroy(void *a) { actor_slot(a); ++destructions; events.push_back(17); }
static unsigned char *draw_flags() { return &flags; }
static float camera_distance(const float *p) { ++distance_checks; return fabsf(p[0]); }
static int camera_visible(const float *p,float bound) { ++visible_checks; assert(bound==1.0f); return p[2]>=0; }
static int ambient_enqueue(void *,void *,int,uint32_t) { assert(state_lock.depth==0); ++ambient_calls; return 27; }
static void ambient_cleanup(void *) { assert(state_lock.depth==0); ++ambient_cleans; old_movement_voice=false; }
static void tactic_effect(void *,void *numbers,void *other) {
    assert(state_lock.depth==0 && numbers==manager && other==app); ++tactic_calls;
}
static void tactic_damage(void *ui,int,void *,int) { assert(state_lock.depth==0 && ui==app); ++damage_calls; }
static int top_state(void *m,const char *name) {
    assert(m==(void *)(base+FL_STATE_MANAGER) && !strcmp(name,"CUserStrategyState")); return mock_strategy_pause;
}
#include "combat_r20_mocks.h"
#include "combat_r21_mocks.h"
#include "batch2_mocks.h"
#include "final_mocks.h"
#include "r48_mocks.h"
uintptr_t fl_test_function(uint32_t rva) {
    switch(rva) {
    case FL_MOTION_EVALUATE:return(uintptr_t)&r48_evaluate;
    case FL_MOTION_SET_FRAME:return(uintptr_t)&r48_commit;
    case FL_NATIVE_VALID:return(uintptr_t)&native_valid;
    case FL_MODEL_BONE_TRANSFORM:return(uintptr_t)&bone_transform;
    case FL_MODEL_SET_POSITION:return(uintptr_t)&set_position;
    case FL_INFO_DESTROY:return(uintptr_t)&info_destroy;
    case FL_UI_SIZE:return(uintptr_t)&ui_query_size;
    case FL_UI_POSITION:return(uintptr_t)&ui_query_position;
    case FL_UI_ELEMENT_SIZE:return(uintptr_t)&ui_size;
    case FL_UI_ELEMENT_POSITION:return(uintptr_t)&ui_position;
    case FL_MANAGER_UPDATE:return(uintptr_t)&original_update;
    case FL_MANAGER_DRAW:return(uintptr_t)&original_draw;
    case FL_FIELD_GROUP_DRAW:case FL_FIELD_ACTION_DRAW:return(uintptr_t)&original_visual;
    case FL_MANAGER_RELEASE:case FL_RESOURCES_RELEASE:return(uintptr_t)&original_release;
    case FL_BODY_SELECTOR:return(uintptr_t)&select_body;
    case FL_CURRENT_APPEARANCE:return(uintptr_t)&current_appearance;
    case FL_REQUEST_COLOR:return(uintptr_t)&request_color;
    case FL_ACTOR_CTOR:return(uintptr_t)&construct;
    case FL_ACTOR_CREATE:return(uintptr_t)&create;
    case FL_ACTOR_BIND:return(uintptr_t)&bind;
    case FL_ACTOR_UPDATE:return(uintptr_t)&tick;
    case FL_ACTOR_DRAW:return(uintptr_t)&draw;
    case FL_ACTOR_RELEASE:return(uintptr_t)&release;
    case FL_ACTOR_DTOR:return(uintptr_t)&destroy;
    case FL_DRAW_FLAGS:return(uintptr_t)&draw_flags;
    case FL_ANIMATION_RATE:return (uintptr_t)+[](void*){return 60.0f;};
    case FL_MODEL_GET_POSITION:return(uintptr_t)&model_position;
    case FL_MODEL_GET_SCALE:return(uintptr_t)&model_scale;
    case FL_MODEL_SET_SCALE:return(uintptr_t)&set_model_scale;
    case FL_BODY_ATTACH:return(uintptr_t)&attach_body;
    case FL_WEAPON_ATTACH:return(uintptr_t)&attach_weapon;
    case FL_EVENT_SOUND_2D:case FL_EVENT_SOUND_3D:return(uintptr_t)&native_sound;
    case FL_CAMERA_DISTANCE:return(uintptr_t)&camera_distance;
    case FL_CAMERA_VISIBLE:return(uintptr_t)&camera_visible;
    case FL_AMBIENT_ENQUEUE:return(uintptr_t)&ambient_enqueue;
    case FL_AMBIENT_CLEANUP:return(uintptr_t)&ambient_cleanup;
    case FL_AMBIENT_START:return(uintptr_t)&ambient_start;
    case FL_AMBIENT_VOLUME:return(uintptr_t)&ambient_volume;
    case FL_ARMY_UPDATE:return(uintptr_t)&army_update;
    case FL_FIELD_EFFECT_CREATE:return(uintptr_t)&field_effect_create;
    case FL_FIELD_EFFECT_RESET:return(uintptr_t)&field_effect_reset;
    case FL_PAIR_EFFECT_SET:return(uintptr_t)&pair_effect_set;
    case FL_DEFEAT_COMMIT:return(uintptr_t)&defeat_commit;
    case FL_ARMY_DETACH:return(uintptr_t)&army_detach;
    case FL_INFO_ANCHOR:return(uintptr_t)&info_anchor;
    case FL_INFO_SIZE:return(uintptr_t)&info_size;
    case FL_UI_VISIBLE:return(uintptr_t)&ui_visible;
    case FL_TACTIC_EFFECT:return(uintptr_t)&tactic_effect;
    case FL_TACTIC_DAMAGE:return(uintptr_t)&tactic_damage;
    case FL_TOP_STATE:return(uintptr_t)&top_state;
    case FL_NORMAL_DAMAGE:return(uintptr_t)&normal_damage;
    case FL_MOTION_EFFECT:return(uintptr_t)&motion_effect;
    case FL_TACTIC_KIND:return(uintptr_t)&tactic_kind;
    case FL_BATTLE_POWER:return(uintptr_t)&battle_power;
    case FL_PAIR_ATTACK:return(uintptr_t)&pair_attack;
    case FL_PAIR_DEFENSE:return(uintptr_t)&pair_defense;
    case FL_TACTIC_FACTOR:return(uintptr_t)&tactic_factor;
    default:assert(false);return 0;
    }
}
static void root(uint32_t rva,void *p) { memcpy(image.data()+rva,&p,sizeof p); }
static void fixture(unsigned count=2) {
    info_destroy_next=0;
    memset(info_padding,0,sizeof info_padding); bone_available=false;
    ambient_start_cursor=NULL;memset(ambient_owned,0,sizeof ambient_owned);
    assert(count<=FL_PROXY_COUNT+1); field_count=count; never_ready_id=UINT32_MAX;
    memset(private_actor_vtable,0,sizeof private_actor_vtable);
    pending_log_bytes=0; private_staging=false; appearance_cursor=appearance_refreshes=0;
    native_dust_removed=native_arcs_removed=info_anchor_moves=optimized_attachments=0;
    memset(proxies,0,sizeof proxies); memset(&FL_Status,0,sizeof FL_Status);
    memset(&FL_RosterAudit,0,sizeof FL_RosterAudit);
    memset(&FL_Registry,0,sizeof FL_Registry);memset(appearance_retry_after,0,sizeof appearance_retry_after);
    inventory_selections=color_requests=color_hold_mask=0; appearance_override=delayed_color=false;
    emit_motion_sound=false; native_sound_calls=suppressed_motion_sounds=0;
    private_presentation_depth=0; fl_test_now_ms=1000;
    motion_evaluation={};r48_evaluations=r48_commits=0;r48_evaluator_writes=true;r48_evaluator_result=1;
    perf_motion_commits=perf_motion_corrected=perf_motion_invalid=perf_motion_wraps=0;
    last_log_flush_ms=presentation_ms=0; log_bytes=log_write_count=log_flush_count=0;
    perf_update={0,0,0,"private_update"}; perf_draw={0,0,0,"private_draw"};
    gait_probe_count=gait_probe_moving=0;
    mock_strategy_pause=strategy_pause=false; guard_rng=1;
    normal_damage_calls=motion_effect_calls=0; mock_damage=123; mock_factor=1;
    for(auto &s:mock_scores) s[0]=s[1]=100;
    expected_effect_hash=23; field_effect_scope={};
    suppressed_stationary_dust=suppressed_stationary_field_effects=0;
    ambient_start_slot=7; ambient_start_calls=ambient_volume_calls=army_update_calls=0;
    mock_field_emit=mock_field_move=false; ambient_attenuations=0; ambient_level_owner=NULL;
    memset(ambient_levels,0,sizeof ambient_levels); memset(sound_manager_memory,0,sizeof sound_manager_memory);
    memset(ambient_percent,0,sizeof ambient_percent); root(FL_SOUND_MANAGER,NULL);
    suppressed_normal_effects=guard_trials=guard_successes=tactic_pairs=paused_guard_trials=0;
    normal_audio=guard_audio=tactic_block_audio=tactic_hit_audio=combat_audio_capped=0;
    memset(combat_voices,0,sizeof combat_voices);
    private_sound_proxy=NULL; pending_collector=NULL; next_proxy_serial=0;
    memset(ambient_pending,0,sizeof ambient_pending); memset(hoof_voices,0,sizeof hoof_voices);
    memset(settings,0,sizeof settings); memset(alt_settings,0,sizeof alt_settings);
    memset(hoof_params,0,sizeof hoof_params);
    at<uint32_t>(hoof_params,0)=0xe3c22636; at<float>(hoof_params,8)=0.8f;
    at<int>(image.data(),FL_TACTIC_CUTSCENE_ACTIVE)=0;
    emit_hoof_sound=sound_failure=old_movement_voice=false;
    visible_checks=distance_checks=ambient_calls=ambient_cleans=tactic_calls=damage_calls=0;
    skill_notifications=damage_notifications=ambient_omissions=hoof_events=hoof_budget_rejections=hoof_selected=0;
    mock_motion_step=1;
    memset(motion_override_seen,0,sizeof motion_override_seen);
    memset(applied_color,0xff,sizeof applied_color);
    FL_Status.protocol=FL_PROTOCOL; FL_StatusSequence=0; engine_calls=thread_handoffs=0; engine_call_thread=0;
    nested_update=nested_cleanup=hold_native_draw=native_draw_waiting=resume_native_draw=false;
    memset(manager,0,sizeof manager); memset(data,0,sizeof data); memset(context,0,sizeof context);
    memset(app,0,sizeof app); memset(field,0,sizeof field); memset(armies,0,sizeof armies); memset(people,0,sizeof people);
    memset(models,0,sizeof models); memset(attachments,0,sizeof attachments); memset(callback,0,sizeof callback);
    base=(uintptr_t)image.data(); configured=true; blocked=false; ownership_lost=false; draw_current_frame=false;
    owner_manager=owner_data=owner_context=owner_engine=NULL; stop_requested=0; fl_test_thread=77;
    log_file=INVALID_HANDLE_VALUE; flags=4; load_ready=true;
    append_limit=UINT32_MAX;
    append_ok=attachment_ok=true; position_reads=scale_reads=body_attachment_calls=weapon_attachment_calls=0;
    deferred_count=0; replacement_callback=NULL; replacement_thread=0; stop_during_draw=false;
    memset(original_visuals,0,sizeof original_visuals); other_visuals=0;
    for(unsigned i=0;i<FL_PROXY_COUNT;++i)for(unsigned k=0;k<5;++k) {
        at<uintptr_t>(models[i][k],0)=base+FL_CHARACTER_VTABLE;
        for(unsigned j=0;j<3;++j)at<float>(models[i][k],0x34+4*j)=1.0f;
    }
    memcpy(alt_models,models,sizeof models); memset(alt_attachments,0,sizeof alt_attachments);
    selections=constructions=destructions=removals=submissions=0; events.clear();
    root(FL_UNIT_MANAGER,manager); root(FL_DATA_MANAGER,data); root(FL_RESOURCE_CONTEXT,context); root(FL_APP,app);
    at<void *>(app,0x210)=engine; at<void *>(context,0x78)=callback_handle;
    at<void *>(context,0x98)=callback_handle; at<void *>(callback_handle,0x20)=callback;
    at<uintptr_t>(callback,0)=base+0x16914f8;
    for (unsigned i=0;i<FL_PROXY_COUNT+1;++i) {
        unsigned id=i==0?596:i==1?500:i==2?501:1000+i;
        at<uintptr_t>(field[i],0)=base+FL_ARMY_VTABLE;
        at<void *>(field[i],0x40)=armies[i];
        at<void *>(field[i],0x8e0)=i+1<count?field[i+1]:NULL;
        at<float>(field[i],0x10)=10.0f+(float)i;
        at<float>(field[i],0x18)=20.0f;
        at<float>(field[i],0x744)=1.25f;
        at<uint8_t>(armies[i],0x10)=1; at<uint16_t>(armies[i],0x12)=id;
        at<void *>(armies[i],0x148)=field[i]; at<uint16_t>(armies[i],0x16)=3000;
        at<unsigned>(pair_markers[i],0)=i;at<void *>(field[i],0x8d0)=pair_markers[i];
        at<uint16_t>(battle_pairs[i],8)=i==0?2:1;at<int>(battle_pairs[i],0xc)=0x1b;
        at<void *>(battle_nodes[i],0)=battle_pairs[i];at<void *>(battle_nodes[i],8)=NULL;
        battle_table[i]=battle_nodes[i];
        if(i<500) at<void *>(data,0x7e838+(i+1)*8)=armies[i];
        at<uintptr_t>(people[i],0)=base+FL_PERSON_VTABLE; at<uint16_t>(people[i],0x10)=id;
        at<void *>(data,0x158+id*8)=people[i];
    }
    root(0x27d10f0,battle_table);root(0x27d10e8,manager);at<unsigned>(image.data(),0x27d1120)=501;
    at<void *>(manager,0x18)=count?field[0]:NULL;
    clear_index(candidate_index); clear_index(proxy_index);
    clear_index(army_index);
    for(unsigned i=0;i<FL_PROXY_COUNT;++i) { FL_Status.officer[i]=UINT32_MAX; FL_Status.appearance[i]=-1; }
}
static void update() { FL_HookUpdate(manager,engine,frame_arg); }
static void draw_frame() { FL_HookDraw(manager,engine,callback); }
static void pass(const char *s) { printf("PASS %s\n",s); }
static void set_id(unsigned unit,unsigned id) {
    unsigned old=at<uint16_t>(people[unit],0x10);
    if(at<void *>(data,0x158+old*8)==people[unit]) at<void *>(data,0x158+old*8)=NULL;
    at<uint16_t>(people[unit],0x10)=(uint16_t)id; at<uint16_t>(armies[unit],0x12)=(uint16_t)id;
    if(id<=FL_MAX_OFFICER) at<void *>(data,0x158+id*8)=people[unit];
}
static void settle(unsigned frames=4) { for(unsigned f=0;f<frames;++f) { update(); draw_frame(); } }
static void native_land(unsigned unit,unsigned expected) {
    for(unsigned part : {0u,1u,2u,4u}) assert(original_visuals[unit][part]==expected);
}
static void override_appearance(unsigned unit,int body,int color,int horse,int weapon=0) {
    appearance_override=true;
    at<int>(people[unit],0x150)=body; at<int>(people[unit],0x154)=color;
    at<int>(people[unit],0x158)=horse; at<int>(people[unit],0x15c)=weapon;
}
#include "batch1_tests.h"
#include "combat_r19_tests.h"
#include "combat_r20_tests.h"
#include "combat_r21_tests.h"
#include "batch2_tests.h"
#include "final_tests.h"
#include "registry_tests.h"
#include "r47_tests.h"
#include "r48_tests.h"
int main() {
    static_assert(sizeof(FLStatus)==16064 && sizeof(FLConfig)==2072,"Remote ABI layout changed");
    static_assert(sizeof(FLRosterRow)==12 && sizeof(FLRosterAudit)==72044,"Roster export ABI changed");
    fixture(); load_ready=false; update(); draw_frame();
    assert(constructions==2 && selections==2 && submissions==0 && events[0]==1 && FL_Status.pending_count==2);
    pass("every officer uses the original selector/create path; pending models keep original visuals");
    load_ready=true; events.clear(); update(); draw_frame();
    assert(constructions==2 && submissions==2 && FL_Status.officer[1]==500 && events.front()==1);
    assert(std::find(events.begin(),events.end(),2)<std::find(events.begin(),events.end(),15));
    pass("original update and draw precede private actors without per-frame duplicate creation");
    at<float>(field[0],0x10)=42.5f; at<float>(field[0],0x744)=2.0f; update();
    assert(at<float>(proxies[0].duel,0xc0)==42.5f && at<float>(proxies[0].duel,0xf0)==2.0f);
    pass("source position and facing track the original unit");
    unsigned before=submissions; root(FL_DATA_MANAGER,NULL); update(); draw_frame(); assert(submissions==before);
    root(FL_DATA_MANAGER,data); update(); assert(constructions==2);
    pass("temporary map-data gaps suppress stale draws without duplicate resources");
    at<int>(field[0],0x740)=1; update(); draw_frame();
    assert(removals==1 && destructions==1 && FL_Status.officer[0]==UINT32_MAX && FL_Status.ready_count==1);
    assert(original_visuals[0][5]==1); native_land(1,0); update(); assert(removals==1);
    pass("only the unit entering water is released; another officer remains visible");
    at<int>(field[0],0x740)=0; update(); assert(constructions==3 && FL_Status.ready_count==2);
    pass("returning to land recreates through the same original appearance path");
    events.clear(); FL_HookManagerRelease(manager,engine);
    assert(removals==3 && destructions==3 && events.back()==3 && !owner_manager);
    FL_HookResourceRelease(context,engine); assert(removals==3);
    pass("private resource release precedes native teardown with no overlapping double-free");
    update(); assert(constructions==5 && FL_Status.ready_count==2);
    pass("an observed new owner generation starts cleanly");
    FL_RequestStop(NULL); update(); before=submissions; draw_frame();
    assert(FL_Status.stopped && removals==5 && submissions==before); update(); assert(removals==5);
    pass("remote stop requests cleanup in the field callback and releases once");
    fixture(); set_id(0,1001); at<uint8_t>(people[1],0xe6)=1; update(); draw_frame();
    assert(constructions==2 && FL_Status.appearance[1]==11 && submissions==2);
    pass("ordinary and monarch appearances work without Zhao Yun being present");
    fixture(); update(); events.clear();
    at<void *>(field[1],0x40)=armies[2]; update();
    assert(removals==1 && constructions==3 && FL_Status.officer[1]==501);
    assert(std::find(events.begin(),events.end(),16)<std::find(events.begin(),events.end(),11));
    pass("a reused actor address releases the old army identity before creating the replacement");
    fixture(); update(); flags=0; draw_frame(); assert(submissions==0);
    flags=4; at<int>(manager,0x498)=1; update(); draw_frame(); assert(submissions==0);
    pass("original draw flags and non-field view gates remain effective");
    fixture(); update(); at<void *>(callback_handle,0x20)=NULL; draw_frame();
    assert(blocked && FL_Status.reason==FL_CALLBACK_CHANGED && submissions==0);
    pass("a changed render callback is rejected");
    fixture(); update(); fl_test_thread=99; update(); draw_frame();
    assert(!blocked && !ownership_lost && submissions==2 && FL_Status.thread_id==99 && thread_handoffs==1);
    assert(constructions==2 && !removals);
    pass("a sequential worker handoff with unchanged owners keeps existing models");
    fixture(); update(); root(FL_RESOURCE_CONTEXT,app); at<void *>(app,0x78)=callback_handle;
    at<void *>(app,0x98)=callback_handle; update(); FL_RequestStop(NULL); update();
    assert(ownership_lost && blocked && removals==0 && FL_Status.reason==FL_CONTEXT_CHANGED);
    pass("unexpected owner replacement quarantines stale handles");
    fixture(); update(); fl_test_thread=99; events.clear(); FL_HookManagerRelease(manager,engine);
    assert(!blocked && removals==2 && destructions==2 && events.back()==3 && !FL_Status.thread_id);
    update(); draw_frame(); assert(constructions==4 && submissions==2 && FL_Status.thread_id==99);
    pass("matching manager teardown permits a new field worker after cleanup");
    fixture(); update(); fl_test_thread=99; FL_HookResourceRelease(context,engine);
    assert(!blocked && removals==2 && !owner_context && !FL_Status.thread_id);
    FL_HookManagerRelease(manager,engine); assert(removals==2);
    pass("matching resource teardown on another worker releases exactly once");
    fixture(); update(); fl_test_thread=99; FL_HookManagerRelease(app,engine);
    assert(blocked && ownership_lost && removals==0 && FL_Status.reason==FL_CONTEXT_CHANGED);
    pass("a mismatched cleanup owner cannot release live resources");
    fixture(); update(); draw_frame(); assert(position_reads==6 && scale_reads==8);
    for(unsigned i=0;i<10;++i){update();draw_frame();assert(FL_Status.ready_count==2);}
    assert(position_reads==6 && scale_reads==28 && at<uint32_t>(callback,0x1c)==66);
    pass("model diagnostics are periodic while all three parts continue to submit");
    fixture(); update(); append_ok=false; draw_frame();
    assert(blocked && FL_Status.reason==FL_APPEND_FAILED && !FL_Status.draw_frames);
    native_land(0,1); native_land(1,1);
    pass("failed array insertion restores deferred originals before pooled rendering");
    fixture(); for(unsigned k=0;k<5;++k)at<uintptr_t>(models[0][k],0)=base+0x123456;
    update(); draw_frame(); assert(position_reads==0 && scale_reads==0 && blocked && submissions==0);
    pass("unknown model classes cannot reach Character getters or scaling");
    fixture(); root(FL_DATA_MANAGER,NULL); update(); assert(!FL_Status.thread_id);
    fl_test_thread=99; root(FL_DATA_MANAGER,data); update(); assert(!blocked && FL_Status.thread_id==99);
    pass("an empty loading screen does not pin the eventual field thread");
    fixture(); std::vector<unsigned char> original_armies((unsigned char *)armies,(unsigned char *)armies+sizeof armies);
    for(unsigned frame=0;frame<600;++frame) {
        at<float>(field[0],0x10)=10.0f+frame; update(); draw_frame();
        for(unsigned slot=0;slot<2;++slot) {
            float body_size=slot?0.95f:1.0f,hs[3],bs[3],ws[3],hp[3],bp[3],wp[3];
            get_model_scale(models[slot][0],bs); get_model_scale(models[slot][1],hs); get_model_scale(models[slot][2],ws);
            get_model_position(models[slot][0],bp); get_model_position(models[slot][1],hp); get_model_position(models[slot][2],wp);
            for(unsigned i=0;i<3;++i) {
                assert(fabsf(hs[i]-3.0f)<0.0001f && fabsf(bs[i]-3.0f*body_size)<0.0001f);
                assert(fabsf(ws[i]-bs[i])<0.0001f);
                assert(fabsf(hp[i]-at<float>(proxies[slot].duel,0xc0+4*i))<0.0001f);
            }
            assert(fabsf(bp[1]-hp[1]-615.0f)<0.001f && fabsf(bp[2]-hp[2]-129.0f)<0.001f);
            assert(fabsf(wp[0]-bp[0]+64.0f*body_size*3.0f)<0.001f);
        }
    }
    assert(constructions==2 && !blocked && memcmp(original_armies.data(),armies,sizeof armies)==0);
    pass("600 moving frames preserve mounted proportions and attachments without accumulated scaling or army writes");
    fixture(); attachment_ok=false; update(); draw_frame();
    assert(blocked && FL_Status.reason==FL_SCALE_FAILED && !submissions);
    FL_RequestStop(NULL); update(); assert(removals==constructions && removals==1 && FL_Status.stopped);
    pass("attachment failure suppresses partial models and still permits cleanup");
    fixture(5); for(unsigned i=0;i<5;++i) at<int>(field[i],0x748)=1;
    at<int>(field[1],0x740)=2; at<int>(field[2],0x740)=3; at<int>(field[4],0x740)=1;
    std::vector<unsigned char> original_fields((unsigned char *)field,(unsigned char *)field+sizeof field);
    original_armies.assign((unsigned char *)armies,(unsigned char *)armies+sizeof armies); settle();
    assert(FL_Status.ready_count==3 && FL_Status.water_count==1 && constructions==3);
    for(unsigned i : {0u,2u,3u}) for(unsigned j=0;j<8;++j) assert(original_visuals[i][j]==0);
    assert(original_visuals[1][6]==1 && original_visuals[1][3]==1 && !bound_army(armies[1]));
    assert(original_visuals[4][5]==1 && original_visuals[4][3]==1);
    assert(memcmp(original_fields.data(),field,sizeof field)==0 && memcmp(original_armies.data(),armies,sizeof armies)==0);
    pass("land and elephant groups are replaced; siege, siege action flags and ships remain original");
    fixture(); load_ready=false; update(); draw_frame(); native_land(0,1); native_land(1,1);
    load_ready=true; never_ready_id=500; update(); draw_frame(); native_land(0,0); native_land(1,1);
    assert(FL_Status.ready_count==1 && FL_Status.pending_count==1);
    pass("per-unit readiness leaves pending troops visible without delaying ready officers");
    fixture(); update(); stop_during_draw=true; draw_frame();
    assert(!submissions && !deferred_count); native_land(0,1); native_land(1,1);
    pass("stop during the native draw replays every deferred original immediately");
    fixture(); update(); draw_frame(); FL_RequestStop(NULL); update(); draw_frame();
    native_land(0,1); native_land(1,1); assert(removals==2);
    pass("stopping restores native troops without a restore patch");
    fixture(); update(); at<void *>(field[0],0x40)=armies[2]; draw_frame();
    native_land(0,1); assert(submissions==1);
    FL_HookFieldGroupDraw(field[1]+0x750,engine,callback);
    FL_HookFieldActionDraw(field[1]+0x580,engine,callback);
    assert(original_visuals[1][0]==1 && original_visuals[1][4]==1);
    pass("changed source identities cannot append stale proxies; out-of-scope calls pass through");
    fixture(); update(); at<int>(field[0],0x740)=1; draw_frame();
    assert(submissions==1 && original_visuals[0][5]==1); native_land(1,0);
    pass("a water transition between update and draw cannot submit a stale mounted proxy");
    fixture(); update(); draw_frame(); FL_HookManagerRelease(manager,engine);
    load_ready=false; update(); draw_frame(); native_land(0,1); native_land(1,1);
    load_ready=true; update(); draw_frame(); native_land(0,0); native_land(1,0);
    assert(constructions==4 && removals==2 && !deferred_count);
    pass("observed reload retains originals during loading and reapplies replacements once");
    fixture(4); set_id(0,0); set_id(1,FL_MAX_OFFICER); set_id(2,2500); at<uint8_t>(people[2],0xe6)=1;
    settle(); assert(constructions==4 && selections==4 && FL_Status.ready_count==4);
    assert(FL_Status.officer[0]==0 && FL_Status.officer[1]==FL_MAX_OFFICER && FL_Status.appearance[2]==11);
    pass("officer zero, high custom IDs and monarchs share the native appearance resolver");
    fixture(FL_PROXY_COUNT); load_ready=false;
    for(unsigned f=0;f<10;++f) { unsigned old=constructions; update(); draw_frame(); assert(constructions-old<=FL_CREATE_PER_FRAME); }
    assert(constructions==FL_MAX_LOADING && !FL_Status.ready_count && FL_Status.pending_count==FL_PROXY_COUNT);
    for(unsigned i=0;i<FL_PROXY_COUNT;++i) native_land(i,1);
    load_ready=true;
    for(unsigned f=0;f<FL_PROXY_COUNT/FL_CREATE_PER_FRAME;++f) {
        unsigned old=constructions; update(); draw_frame(); assert(constructions-old<=FL_CREATE_PER_FRAME && !blocked);
    }
    assert(constructions==FL_PROXY_COUNT && selections==FL_PROXY_COUNT && FL_Status.ready_count==FL_PROXY_COUNT);
    for(unsigned i=0;i<FL_PROXY_COUNT;++i) native_land(i,0);
    assert(position_reads<=3*FL_SNAPSHOT_SLOTS*2);
    settle(3); assert(constructions==FL_PROXY_COUNT);
    FL_RequestStop(NULL); update(); assert(removals==FL_PROXY_COUNT && destructions==FL_PROXY_COUNT);
    pass("500 units obey creation and pending-load budgets, retain queued visuals, bound snapshots and release once");
    fixture(FL_PROXY_COUNT); for(unsigned i=0;i<FL_PROXY_COUNT;++i) { at<int>(field[i],0x740)=3; at<int>(field[i],0x748)=1; }
    settle(FL_PROXY_COUNT/FL_CREATE_PER_FRAME); assert(FL_Status.ready_count==FL_PROXY_COUNT);
    append_ok=false; draw_frame(); assert(blocked && !deferred_count);
    for(unsigned i=0;i<FL_PROXY_COUNT;++i) for(unsigned j : {0u,1u,2u,3u,6u,7u}) assert(original_visuals[i][j]==1);
    pass("full-capacity failure replays all 3000 deferred elephant-mode calls without overflow");
    fixture(); at<void *>(field[1],0x8e0)=field[0]; update();
    assert(blocked && FL_Status.reason==FL_BAD_LIST && !constructions);
    pass("a cyclic active list fails before resource creation");
    fixture(FL_PROXY_COUNT+1); update(); assert(blocked && FL_Status.reason==FL_BAD_LIST && !constructions);
    pass("a field list beyond the verified capacity cannot overrun candidate storage");
    fixture(8); settle(); assert(constructions==8);
    at<void *>(manager,0x18)=field[7]; for(unsigned i=0;i<8;++i) at<void *>(field[i],0x8e0)=i?field[i-1]:NULL;
    settle(); assert(constructions==8 && !removals && FL_Status.ready_count==8);
    pass("linked-list reordering preserves existing proxy identities without churn");
    fixture(0); update(); draw_frame(); assert(!blocked && !constructions && FL_Status.reason==FL_NO_LAND);
    field_count=1; at<void *>(manager,0x18)=field[0]; at<void *>(field[0],0x8e0)=NULL; update(); draw_frame();
    assert(constructions==1 && submissions==1);
    pass("an empty field automatically handles the first new deployment");
    fixture(4); set_id(0,65535); at<uintptr_t>(people[1],0)=base+123; at<float>(field[2],0x10)=NAN;
    settle(); assert(constructions==1 && FL_Status.excluded_count==3 && !blocked);
    native_land(0,1); native_land(1,1); native_land(2,1); native_land(3,0);
    pass("invalid IDs, person types and nonfinite transforms retain originals without table overreads");
    fixture(12); load_ready=false; settle(4); assert(constructions==8);
    at<uint8_t>(armies[0],0x10)=0; at<int>(field[1],0x748)=2; update();
    assert(removals==2 && constructions==10 && FL_Status.target_count==10);
    load_ready=true; settle(); assert(FL_Status.ready_count==10 && constructions==12);
    pass("removing pending units frees loading capacity for queued new deployments");
    fixture(); update();
    memcpy(people[2],people[0],sizeof people[0]); at<void *>(data,0x158+596*8)=people[2]; update();
    assert(constructions==3 && removals==1 && proxies[0].source.person==people[2]);
    pass("a person-pointer generation change recreates even when officer ID is unchanged");
    fixture(); at<int>(field[0],0x740)=99; at<int>(field[0],0x748)=1; update(); draw_frame();
    assert(constructions==1 && FL_Status.excluded_count==1 && original_visuals[0][3]==1);
    pass("unknown future presentation modes remain native");
    fixture(); update(); fl_test_thread=99; draw_frame();
    assert(!blocked && submissions==2 && thread_handoffs==1); native_land(0,0); native_land(1,0);
    pass("a draw-worker handoff arms replacement before original drawing without doubled troops");
    fixture(); settle();
    for(unsigned f=0;f<120;++f) {
        fl_test_thread=(f%2)?91:92; at<float>(field[0],0x10)=100.0f+f; update();
        fl_test_thread=93; draw_frame();
        assert(!blocked && at<float>(proxies[0].duel,0xc0)==100.0f+f);
        native_land(0,0); native_land(1,0);
    }
    assert(constructions==2 && removals==0 && thread_handoffs==240);
    pass("repeated update/draw worker alternation tracks movement without resource churn");
    fixture(); settle(); FL_RequestStop(NULL); fl_test_thread=99; update(); draw_frame();
    assert(FL_Status.stopped && removals==2 && !blocked); native_land(0,1); native_land(1,1);
    pass("stop cleans up on a valid successor worker instead of remaining stuck");
    fixture(); update(); fl_test_thread=99; root(FL_DATA_MANAGER,app); update();
    assert(blocked && ownership_lost && FL_Status.reason==FL_CONTEXT_CHANGED && !removals);
    pass("worker changes do not excuse a changed data owner");
    fixture(); update(); fl_test_thread=99; at<void *>(app,0x210)=data; draw_frame();
    assert(!submissions); native_land(0,1); native_land(1,1);
    pass("a changed application engine cannot use a worker handoff to render stale models");
    fixture(); update(); nested_update=true; draw_frame();
    assert(blocked && ownership_lost && FL_Status.reason==FL_OVERLAPPING_CALL && !submissions && !engine_calls);
    assert(constructions==2 && removals==0 && !deferred_count);
    draw_frame(); native_land(0,1); native_land(1,1);
    pass("same-thread reentry quarantines handles and later native frames pass through");
    fixture(); update(); nested_cleanup=true; draw_frame();
    assert(blocked && ownership_lost && FL_Status.reason==FL_OVERLAPPING_CALL && !submissions && !removals);
    assert(!deferred_count && !engine_calls);
    pass("teardown overlapping a draw never releases or replays possibly stale private or source handles");
    fixture(); update(); hold_native_draw=true;
    std::thread worker([]{fl_test_thread=77; draw_frame();});
    { std::unique_lock<std::mutex> lock(rendezvous); rendezvous_cv.wait(lock,[]{return native_draw_waiting;}); }
    fl_test_thread=99; update();
    { std::lock_guard<std::mutex> lock(rendezvous); resume_native_draw=true; }
    rendezvous_cv.notify_all(); worker.join(); hold_native_draw=false;
    assert(blocked && ownership_lost && FL_Status.reason==FL_OVERLAPPING_CALL && !submissions && !engine_calls);
    assert(!removals && !deferred_count && !(FL_StatusSequence&1));
    draw_frame(); native_land(0,1); native_land(1,1);
    pass("two real host threads overlapping original native calls are quarantined without deadlock");
    fixture(); lock_status(); update_mod(manager,engine,frame_arg); unlock_status();
    assert(blocked && ownership_lost && FL_Status.reason==FL_OTHER_THREAD && !constructions);
    pass("private engine work outside an admitted native callback remains rejected");
    fixture(); settle(); assert(!(FL_StatusSequence&1) && FL_Status.ready_count==FL_Status.target_count);
    before=FL_StatusSequence; FL_HookManagerRelease(manager,engine);
    assert(!(FL_StatusSequence&1) && FL_StatusSequence>before && !FL_Status.ready_count);
    pass("status publication closes its sequence after normal frames and owner cleanup");
    fixture(); delayed_color=true; override_appearance(0,3,-1,2); override_appearance(1,16,4,1);
    update(); draw_frame(); assert(!submissions && color_requests==4);
    native_land(0,1); native_land(1,1);
    assert(std::find(events.begin(),events.end(),13)<std::find(events.begin(),events.end(),18));
    assert(std::find(events.begin(),events.end(),19)<std::find(events.begin(),events.end(),14));
    update(); draw_frame(); assert(submissions==2 && color_requests==4);
    assert(applied_color[0][1]==2 && applied_color[1][0]==4 && applied_color[1][1]==1);
    pass("body and horse color requests follow bind and precede polling; final readiness waits another native poll");
    fixture(1); override_appearance(0,1,-1,2); color_hold_mask=2;
    settle(); assert(!submissions && !proxies[0].ready); native_land(0,1);
    color_hold_mask=0; settle(); assert(proxies[0].ready && applied_color[0][1]==2);
    pass("a unique officer's pending horse color keeps native troops until that horse is ready");
    fixture(1); override_appearance(0,14,3,-1); color_hold_mask=1;
    settle(); assert(!submissions); native_land(0,1);
    color_hold_mask=0; settle(); assert(proxies[0].ready && at<int>(proxies[0].actor,0x84)==5);
    pass("a common civil officer waits for body color including the native secondary material slot");
    fixture(); override_appearance(0,16,1,2); override_appearance(1,16,4,1); settle();
    assert(applied_color[0][0]==1 && applied_color[1][0]==4 && applied_color[0][1]==2 && applied_color[1][1]==1);
    assert(at<int>(proxies[0].actor,0x7c)==1 && at<int>(proxies[1].actor,0x7c)==4);
    pass("two instances sharing a common body retain independent body and horse selections");
    fixture(1); override_appearance(0,6,-1,-1); settle();
    assert(proxies[0].ready && applied_color[0][1]==-1);
    pass("the original unchanged horse sentinel is preserved for unique appearances");
    unsigned appearance_cases=0;
    for(int body=0;body<25;++body) {
        bool common=(body>=11 && body<=16) || body>=22;
        for(int color=0;color<(common?5:1);++color)for(int horse=-1;horse<=2;++horse)for(int weapon=0;weapon<6;++weapon) {
            int requested_color=common?color:-1;
            fixture(1); override_appearance(0,body,requested_color,horse,weapon); delayed_color=true;
            settle(2);
            assert(!blocked && proxies[0].ready && submissions==1 && color_requests==2);
            assert(applied_color[0][0]==requested_color && applied_color[0][1]==horse);
            assert(proxies[0].body==body && proxies[0].weapon==weapon);
            assert(at<int>(proxies[0].actor,0x84)==(body==14?5:((0x180b000u>>body)&1)?4:0));
            FL_RequestStop(NULL); update(); assert(removals==1 && destructions==1);
            ++appearance_cases;
        }
    }
    assert(appearance_cases==1464);
    pass("all 1464 body-color/horse-state/weapon combinations use the full private create-ready-submit-release path");
    fixture(); color_hold_mask=3; settle(); FL_RequestStop(NULL); update();
    assert(removals==2 && destructions==2 && !submissions);
    pass("stop during material loading releases both pending private actors once");
    fixture(1); override_appearance(0,12,2,1); settle();
    at<uint32_t>(field[0],0x740)=1; update(); assert(removals==1);
    override_appearance(0,12,4,2); at<uint32_t>(field[0],0x740)=0; settle();
    assert(constructions==2 && applied_color[0][0]==4 && applied_color[0][1]==2);
    pass("returning from water re-resolves current body and horse colors without stale wrapper state");
    fixture(0); appearance_override=true;
    std::vector<unsigned char> roster_people((FL_MAX_OFFICER+1)*0x1f0);
    for(unsigned id=0;id<=FL_MAX_OFFICER;++id) {
        void *p=roster_people.data()+id*0x1f0;
        at<uintptr_t>(p,0)=base+FL_PERSON_VTABLE; at<uint16_t>(p,0x10)=(uint16_t)id;
        int b=id%25; at<int>(p,0x150)=b; at<int>(p,0x154)=((b>=11 && b<=16) || b>=22)?(int)(id%5):-1;
        at<int>(p,0x158)=(int)(id%4)-1; at<int>(p,0x15c)=id%6;
        at<void *>(data,0x158+id*8)=p;
    }
    const auto unchanged_people=roster_people;
    for(unsigned frame=0;!FL_RosterAudit.complete && frame<2000;++frame) {
        unsigned prev_calls=inventory_selections,prev_slots=FL_RosterAudit.scanned_slots;
        update(); assert(inventory_selections-prev_calls<=4 && FL_RosterAudit.scanned_slots-prev_slots<=32);
    }
    assert(FL_RosterAudit.complete && FL_RosterAudit.registered==6001 && FL_RosterAudit.resolved==6001 && !FL_RosterAudit.unresolved);
    assert(!constructions && roster_people==unchanged_people);
    for(unsigned id=0;id<=FL_MAX_OFFICER;++id) {
        assert(FL_RosterAudit.rows[id].body==(int)(id%25));
        assert(FL_RosterAudit.rows[id].flags==(FL_ROSTER_PRESENT|FL_ROSTER_RESOLVED));
    }
    pass("bounded all-person audit covers IDs zero through 6000 without creating actors or modifying person data");
    before=inventory_selections; settle(20); assert(inventory_selections==before);
    pass("a completed roster scan stops querying and never claims field or pixel verification");
    unsigned generation=FL_RosterAudit.generation; FL_HookManagerRelease(manager,engine); update();
    assert(FL_RosterAudit.generation==generation+1 && !FL_RosterAudit.complete && FL_RosterAudit.registered==4);
    pass("an observed owner reload starts a new bounded roster generation");
    fixture(0); appearance_override=true; at<int>(people[0],0x150)=-1;
    at<uintptr_t>(people[1],0)=0; at<void *>(data,0x158+501*8)=NULL;
    for(unsigned f=0;!FL_RosterAudit.complete && f<1000;++f)update();
    assert(!blocked && FL_RosterAudit.complete && FL_RosterAudit.unresolved==1 && !constructions);
    assert(FL_RosterAudit.rows[596].flags&FL_ROSTER_UNRESOLVED);
    pass("missing and unresolved roster entries are reported without fabricated appearances or stopping the field");
    fixture(1); delayed_color=true; settle(2);
    assert(proxies[0].body==3 && proxies[0].color==-1 && proxies[0].ready && applied_color[0][0]==-1);
    pass("native initialization preserves fixed unique body material with minus-one rather than nonexistent row zero");
    fixture(1); override_appearance(0,16,-1,2); delayed_color=true; settle(2);
    assert(proxies[0].ready && applied_color[0][0]==-1 && applied_color[0][1]==2);
    pass("a common body without a force color preserves base materials while applying its original horse variant");
    fixture(1); at<int>(field[0],0x60)=1; settle();
    assert(!proxies[0].running && at<int>(proxies[0].duel,0x28)==55 && at<int>(proxies[0].duel,0x2c)==0);
    pass("a movement order or native interpolation hint cannot start running without observed displacement");
    at<float>(field[0],0x10)+=12; fl_test_now_ms+=16; update();
    assert(proxies[0].running && at<int>(proxies[0].duel,0x28)==3 && at<int>(proxies[0].duel,0x2c)==1);
    assert(motion_override_seen[0][0]==proxies[0].loop_offset && motion_override_seen[0][1]==proxies[0].loop_offset);
    assert(constructions==1 && color_requests==2 && at<float>(models[0][1],0x34)==FL_FIELD_SCALE);
    pass("actual horizontal travel starts the original paired rider and horse actions at the same native frame");
    for(unsigned n=0;n<200;++n) {
        at<float>(field[0],0x18)+=3; fl_test_now_ms+=16; update();
        assert(proxies[0].running && motion_override_seen[0][0]==0 && motion_override_seen[0][1]==0);
    }
    assert(proxies[0].motion_transitions==1);
    pass("continued travel leaves native animation advancement in control instead of restarting the clip each frame");
    fl_test_now_ms+=80; update(); assert(proxies[0].running);
    fl_test_now_ms+=41; update(); assert(!proxies[0].running && motion_override_seen[0][1]==proxies[0].loop_offset);
    assert(at<int>(proxies[0].duel,0x28)==55 && at<int>(proxies[0].duel,0x2c)==0);
    pass("skipped native position frames are tolerated but a stationary pause settles to idle within 120ms plus one update");
    at<float>(field[0],0x10)+=1; update(); assert(proxies[0].running);
    at<int>(field[0],0x60)=0; update(); assert(!proxies[0].running);
    pass("a native stop hint ends an observed run on the first unchanged-position update");
    for(unsigned n=0;n<5;++n) {
        at<float>(field[0],0x744)+=0.3f; at<float>(field[0],0x14)+=2;
        at<float>(field[0],0x10)+=0.001f; update();
        assert(!proxies[0].running);
    }
    pass("yaw-only movement, terrain-height settling and sub-threshold jitter do not start a run");
    at<float>(field[0],0x10)+=5; update(); assert(proxies[0].running);
    at<int>(manager,0x498)=1; update(); at<float>(field[0],0x18)+=30;
    at<int>(manager,0x498)=0; update(); assert(!proxies[0].running);
    at<float>(field[0],0x18)+=2; update(); assert(proxies[0].running);
    root(FL_DATA_MANAGER,NULL); update(); root(FL_DATA_MANAGER,data); update(); assert(!proxies[0].running);
    pass("returning after another view or a map-data gap resets movement history without stale running");
    at<float>(field[0],0x10)+=10000; update(); assert(!proxies[0].running);
    at<float>(field[0],0x10)+=5; update(); assert(proxies[0].running);
    at<int>(field[0],0x740)=3; at<float>(field[0],0x10)+=5; update(); assert(!proxies[0].running);
    at<float>(field[0],0x10)+=5; update(); assert(proxies[0].running);
    pass("teleport-sized discontinuities and land-mode changes restart sampling before normal travel resumes");
    fixture(2); settle(); at<float>(field[0],0x10)+=5; update();
    assert(proxies[0].running && !proxies[1].running);
    at<int>(field[0],0x740)=1; update(); assert(!proxies[0].constructed && !proxies[0].running);
    at<int>(field[0],0x740)=0; update(); assert(proxies[0].constructed && !proxies[0].running);
    at<float>(field[0],0x10)+=5; update(); assert(proxies[0].running);
    FL_HookManagerRelease(manager,engine); update(); assert(!proxies[0].running);
    FL_RequestStop(NULL); update(); assert(!proxies[0].constructed && FL_Status.stopped);
    pass("movement is per unit and is cleared by water, observed reload and user stop without affecting another unit");
    for(int body=0;body<25;++body) {
        fixture(1); bool common=(body>=11 && body<=16) || body>=22;
        override_appearance(0,body,common?4:-1,2,body%6); settle();
        at<float>(field[0],0x18)+=5; update(); draw_frame();
        assert(proxies[0].running && proxies[0].ready && applied_color[0][1]==2 && submissions>0);
        assert(applied_color[0][0]==(common?4:-1) && constructions==1 && color_requests==2);
    }
    pass("all 25 body classes retain original body, horse color and weapon selection while entering mounted movement");
    fixture(500); settle(251);
    for(unsigned i=0;i<500;++i)at<float>(field[i],0x10)+=5;
    update(); draw_frame();
    for(unsigned i=0;i<500;++i)assert(proxies[i].running);
    assert(constructions==500 && FL_Status.ready_count==500 && !blocked);
    pass("the 500-unit capacity can enter movement without reallocating appearances or weakening draw limits");
    fixture(1); emit_motion_sound=true; update();
    assert(native_sound_calls==0 && suppressed_motion_sounds==2 && private_presentation_depth==0);
    uint32_t sound_id=0; assert(sound_call(false,&sound_id) && sound_id==12345);
    sound_id=0; assert(sound_call(true,&sound_id) && sound_id==12345 && native_sound_calls==2);
    pass("private native sound events are rejected in both branches while external calls forward all nine arguments and results");
    private_presentation_depth=1;
    std::thread sound_worker([] { uint32_t id=0; assert(private_presentation_depth==0 && sound_call(true,&id) && id==12345); });
    sound_worker.join(); private_presentation_depth=0;
    assert(native_sound_calls==3);
    pass("the private sound gate is thread-local and does not mute another worker's original sound call");
    run_batch1_tests();
    run_combat_r19_tests();
    run_combat_r20_tests();
    run_combat_r21_tests();
    run_batch2_tests();
    run_final_tests();
    run_registry_tests();
    fixture(4);settle();
    for(unsigned i=0;i<4;++i){assert(proxies[i].loop_offset>=1 && proxies[i].loop_offset<60);for(unsigned j=0;j<i;++j)assert(proxies[i].loop_offset!=proxies[j].loop_offset);}
    pass("visual idle and move offsets differ by identity without game RNG");
    ActorIndex check_index={};clear_index(check_index);
    for(unsigned i=0;i<500;++i)assert(index_insert(check_index,0x1000+i*0x1000,i));
    for(unsigned i=0;i<500;++i)assert(index_find(check_index,0x1000+i*0x1000)==int(i));
    clear_index(check_index);for(unsigned i=0;i<500;++i)assert(index_find(check_index,0x1000+i*0x1000)==-1);
    check_index.generation=UINT32_MAX;clear_index(check_index);assert(check_index.generation==1 && index_find(check_index,0x1000)==-1);
    pass("actor generation table preserves 500 colliding lookups and clears stale entries including wrap");
    double travel[2]={};
    for(unsigned scenario=0;scenario<2;++scenario){
        unsigned fps=scenario?120:60;performance_frequency=120000;animation_last_tick=0;fl_test_qpc_override=120000;advance_animation_clock();
        private_presentation_depth=1;private_sound_proxy=&proxies[0];
        for(unsigned frame=0;frame<fps*10;++frame){fl_test_qpc_override+=120000/fps;advance_animation_clock();travel[scenario]+=60.0/FL_HookAnimationRate(NULL);}
        assert(fabs(travel[scenario]-600)<0.001);
    }
    pass("same ten real seconds advance 600 native animation frames at both 60 and 120 display FPS");
    private_presentation_depth=0;private_sound_proxy=NULL;assert(FL_HookAnimationRate(NULL)==60.0f);
    pass("original duel and unrelated actors keep the native animation rate");
    fl_test_qpc_override+=120000;advance_animation_clock();assert(animation_rate==60.0f);fl_test_qpc_override=0;
    pass("load or long stalls do not fast-forward private animation");
    run_r47_tests();
    run_r48_tests();
    puts("All scenarios passed; mocked engine, not a game rendering result.");
}
