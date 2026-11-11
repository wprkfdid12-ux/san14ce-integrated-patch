#pragma once
#include "r44_idle_reference.h"
static void r47_raw(IdleSeam::Pose *bones,unsigned count,float phase) {
    for(unsigned i=0;i<count;++i) {
        auto &b=bones[i];b={};b.parent=i?int(i-1):-1;b.flags=0x12345678;
        b.rotation[3]=1;for(unsigned k=0;k<3;++k){b.scale[k]=1;b.position[k]=(phase<1?-4.0f:4.0f)+float(i)*0.25f;}
    }
}
static void run_r47_tests() {
    // Compare with the exact former overlay algorithm, not expected outputs
    // copied from the new implementation. Both curves and flags must agree.
    for(unsigned fps:{60u,90u,120u}) {
        IdleSeam::State a={};IdleReference::State b={};
        alignas(16) IdleSeam::Pose x[8];alignas(16) IdleReference::Pose y[8];
        IdleSeam::Key ka{1,2,3,4,5,55};IdleReference::Key kb{1,2,3,4,5,55};
        for(unsigned tick=1;tick<fps*25;++tick) {
            float frame=fmodf(float(tick)*60.0f/fps,120.0f);
            r47_raw(x,8,frame);memcpy(y,x,sizeof x);
            IdleSeam::apply(a,ka,x,8,frame,tick,1000+uint64_t(tick)*1000/fps);
            IdleReference::apply(b,kb,y,8,frame,tick);
            assert(!memcmp(x,y,sizeof x));
        }
    }
    pass("R4.7 integrated idle seam is byte-identical to R4.4 poses over repeated 60/90/120Hz loops");
    {
        IdleSeam::State s={};IdleSeam::Key key{1,2,3,4,5,55};alignas(16) IdleSeam::Pose x[2];
        r47_raw(x,2,119);IdleSeam::apply(s,key,x,2,119,1,1000);
        r47_raw(x,2,0);IdleSeam::apply(s,key,x,2,0,3,1017);
        assert(x[0].position[0]==4); // normal 60Hz evaluation in a 120Hz driver
        r47_raw(x,2,119);IdleSeam::apply(s,key,x,2,119,4,1034);
        r47_raw(x,2,0);IdleSeam::apply(s,key,x,2,0,8,2000);
        assert(x[0].position[0]==-4); // real long gap resets
        ++key.serial;r47_raw(x,2,0);IdleSeam::apply(s,key,x,2,0,9,2017);
        assert(!s.blending);
    }
    pass("R4.7 idle continuity uses elapsed time: skipped update IDs retain seams; long gaps and new identities reset");
    fixture(1);settle();
    auto &p=proxies[0];
    private_presentation_depth=1;private_sound_proxy=&p;
    for(unsigned fps:{60u,90u,120u}) for(Pose pose:{POSE_MOVE,POSE_IDLE,POSE_ATTACK,POSE_HIT,POSE_GUARD,POSE_STRONG,POSE_FALL}) {
        p.pose=pose;performance_frequency=180000;animation_last_tick=0;fl_test_qpc_override=180000;
        advance_animation_clock();double advance=0;
        for(unsigned n=0;n<fps*10;++n){fl_test_qpc_override+=180000/fps;advance_animation_clock();advance+=60.0/FL_HookAnimationRate(NULL);}
        double expected=pose==POSE_MOVE?450:600;
        assert(fabs(advance-expected)<0.002);
    }
    private_presentation_depth=0;private_sound_proxy=NULL;fl_test_qpc_override=0;
    pass("R4.7 only MOVE uses 0.75 time scale at 60/90/120Hz; idle/combat/fall durations and original clocks stay unchanged");
    fixture(2);settle();engage();mock_strategy_pause=true;update();
    set_pose(proxies[0],POSE_BATTLE_IDLE,0,true);set_pose(proxies[1],POSE_BATTLE_IDLE,1,true);
    unsigned wait=proxies[0].paused_wait_ms;
    assert(wait>=1350 && wait<=1650 && wait!=proxies[1].paused_wait_ms);
    fl_test_now_ms+=751;update();assert(!proxies[0].pose_done && proxies[0].pose==POSE_BATTLE_IDLE);
    normal_event(0,1);update();assert(proxies[0].pose==POSE_ATTACK);
    set_pose(proxies[0],POSE_BATTLE_IDLE,0,true);mock_strategy_pause=false;fl_test_now_ms+=751;update();assert(proxies[0].pose_done);
    pass("R4.7 paused waits are identity-staggered around 1500ms; real attack events bypass the wait; running-turn waits remain 750ms");
    fixture(2);settle();engage();tactic();update();
    // Return to MOVE on same horse action must preserve that horse clock.
    set_pose(proxies[0],POSE_STRONG,0,true);at<float>(settings[0][1],0x24)=37;
    set_pose(proxies[0],POSE_MOVE,0);assert(!proxies[0].loop_horse_pending);
    update_proxy(proxies[0],proxies[0].source,0,engine,frame_arg);
    assert(motion_override_seen[0][1]==0);
    pass("R4.7 rider phase alignment preserves a continuing horse clock after a tactic");
    fixture(500);for(unsigned i=0;i<260;++i)update();
    lock_status();prepare_draw_lookup();
    for(unsigned i=0;i<500;++i)for(unsigned part=0;part<7;++part) {
        auto *row=find_draw_lookup(field[i]+draw_offsets[part],part==4);
        assert(row && proxies[row->slot].source.actor==field[i] && row->part==part);
    }
    assert(!find_draw_lookup(field[0]+0x818,false));
    draw_lookup_armed=false;unlock_status();
    pass("R4.7 3500 draw-map entries resolve exact live actors and parts; unsupported geometry stays native");
    fixture(1);settle();draw_frame();auto generation=proxies[0].replaced_generation;
    append_ok=false;draw_frame();assert(blocked && proxies[0].replaced_generation==generation);native_land(0,1);
    pass("R4.7 an earlier successful submission never hides native fallback after a failed current pass");
    fixture(2);settle();append_limit=submissions+1;draw_frame();
    assert(blocked);native_land(0,0);native_land(1,1);
    pass("R4.7 a later append failure preserves the earlier current-pass success without duplicating original troops");
    fixture(1);settle();
    performance_frequency=120000;performance_window=120000;performance_last_draw=0;performance_windows=0;
    fl_test_qpc_override=120000;log_file=(HANDLE)1;log_bytes=0;pending_log_bytes=0;
    for(unsigned i=0;i<120*1800;++i) {
        fl_test_qpc_override+=1000;++perf_update_hooks;performance_draw_sample();
        performance_record(perf_update,fl_test_qpc_override-100);
        performance_summary(1,1);
        if(pending_log_bytes)write_pending_log();
    }
    assert(performance_windows==360 && log_write_count>=360);
    assert(perf_update.count==0 && log_bytes>300000 && log_bytes<8*1024*1024);
    log_bytes=8*1024*1024;log_line("R47_RING_TEST");write_pending_log();assert(log_bytes<1024);
    log_file=INVALID_HANDLE_VALUE;fl_test_qpc_override=0;performance_window=performance_last_draw=0;
    pass("R4.7 simulated 30-minute 120Hz run emits 360 five-second windows beyond sample300 and rotates at8MiB");
}
