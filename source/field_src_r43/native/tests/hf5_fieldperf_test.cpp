#define FL_HF4_MAIN_NAME preserved_hf4_main
#include "hf4_presentation_test.cpp"
/* HF5 FieldPerf: screen-edge wake zone with hysteresis and the 120Hz half-rate
   hold. Synthetic native resources and a scripted performance counter; this
   proves control flow and timing arithmetic, NOT live-game FPS. */
static uint64_t hf5_qpc=5000000000ull;
static void clock_step(double ms){ hf5_qpc+=uint64_t(ms*1000000.0); fl_test_qpc_override=hf5_qpc; fl_test_now_ms+=uint64_t(ms+0.5); }
static void frame_at(double ms){ clock_step(ms); update(); draw_frame(); }
static unsigned slot_of(unsigned unit){ for(unsigned i=0;i<FL_PROXY_COUNT;++i) if(proxies[i].constructed && proxies[i].source.actor==field[unit]) return i; return UINT32_MAX; }
int main() {
    preserved_hf4_main();
    const double hz120=1000.0/120.0, hz60=1000.0/60.0;
    // 1) 60FPS: never held, every engaged unit evaluates every update.
    fl_test_qpc_override=hf5_qpc; engage(2);
    for(int i=0;i<10;++i) frame_at(hz60);
    perf_half_rate_skips=0; memset(mock_tick_calls,0,sizeof mock_tick_calls);
    for(int i=0;i<120;++i) frame_at(hz60);
    assert(perf_half_rate_skips==0 && !blocked);
    for(unsigned u=0;u<2;++u) assert(mock_tick_calls[slot_of(u)]==120);
    pass("60FPS keeps every update evaluated (HF5 hold is inactive)");
    // 2) 120FPS standing combat loops: about half held, never two holds in a row,
    //    drawn every frame, and the evaluation after a hold advances by ~1/60 s.
    for(int i=0;i<10;++i) frame_at(hz120);
    perf_half_rate_skips=0; memset(mock_tick_calls,0,sizeof mock_tick_calls);
    unsigned submitted_before=submissions; unsigned held_runs=0; bool last_held[2]={};
    unsigned rate_checks=0;
    for(int i=0;i<240;++i) {
        uint64_t before[2]; for(unsigned u=0;u<2;++u) before[u]=mock_tick_calls[slot_of(u)];
        frame_at(hz120);
        for(unsigned u=0;u<2;++u) {
            unsigned s=slot_of(u); bool held=mock_tick_calls[s]==before[u];
            Pose pose=proxies[s].pose;
            if(held) { assert(pose==POSE_ATTACK||pose==POSE_BATTLE_IDLE||pose==POSE_GUARD); assert(!last_held[u]); ++held_runs; }
            else if(last_held[u]) { assert(fabsf(mock_seen_rate[s]-60.0f)<1.0f); ++rate_checks; }
            else if(i>0 && fabsf(mock_seen_rate[s]-120.0f)>=2.0f){ printf("DBG i=%d u=%u rate=%f pose=%d anim=%f\n",i,u,mock_seen_rate[s],int(pose),animation_rate); fflush(stdout); assert(0);}
            last_held[u]=held;
        }
    }
    assert(held_runs>120 && held_runs<=240 && rate_checks>100 && !blocked);
    assert(submissions-submitted_before==480); // both units drawn on all 240 frames
    printf("INFO 120FPS holds=%u of 480 unit-updates; post-hold rate checks=%u\n",held_runs,rate_checks);
    pass("120FPS standing combat loops evaluate at ~60Hz with real-time clip speed and are drawn every frame");
    // 3) Movement or turning is never held.
    unsigned s0=slot_of(0); perf_half_rate_skips=0; memset(mock_tick_calls,0,sizeof mock_tick_calls);
    for(int i=0;i<60;++i){ at<float>(field[0],0x10)+=2.0f; frame_at(hz120); }
    assert(mock_tick_calls[s0]==60);
    for(int i=0;i<30;++i){ at<float>(field[0],0x744)+=0.01f; frame_at(hz120); }
    assert(mock_tick_calls[s0]==90);
    pass("moving or turning units evaluate on every 120FPS update");
    // 4) A pose change or restart is evaluated immediately, not held.
    for(int i=0;i<20;++i) frame_at(hz120);
    unsigned s1=slot_of(1); unsigned changes=0;
    for(int i=0;i<200;++i){
        uint32_t t=proxies[s1].motion_transitions; uint64_t c=mock_tick_calls[s1];
        frame_at(hz120);
        if(proxies[s1].motion_transitions!=t){ assert(mock_tick_calls[s1]==c+1); ++changes; }
    }
    assert(changes>0);
    pass("every combat pose change or restart is evaluated on the same update");
    // 5) Paused battle: standing units still use the HF4 zero-update cache.
    mock_strategy_pause=true; frame_at(hz120); frame_at(hz120);
    memset(mock_tick_calls,0,sizeof mock_tick_calls);
    for(int i=0;i<60;++i) frame_at(hz120);
    assert(mock_tick_calls[slot_of(0)]==0 && mock_tick_calls[slot_of(1)]==0);
    mock_strategy_pause=false;
    pass("paused battle standing cache is unchanged at 120FPS");
    // 6) Edge ring and hysteresis. Mock screen = world z>=0, exact bound 1.0 only.
    //    Wake ring: a point 1500 away is on screen (z>=-1500); sleep ring 2600.
    fl_test_qpc_override=hf5_qpc; fixture(1); settle(); unsigned w=slot_of(0);
    at<float>(field[0],0x18)=-9000; frame_at(hz60); assert(proxies[w].visual_deferred && !proxies[w].view_awake);
    uint64_t c=mock_tick_calls[w];
    at<float>(field[0],0x18)=-1000; frame_at(hz60); frame_at(hz60);
    assert(proxies[w].view_awake && !proxies[w].visual_deferred && mock_tick_calls[w]>c);
    pass("a unit just outside the screen edge is prepared before it becomes visible");
    at<float>(field[0],0x18)=-2000; c=mock_tick_calls[w]; at<float>(field[0],0x10)+=1; frame_at(hz60);
    assert(proxies[w].view_awake && mock_tick_calls[w]==c+1);
    at<float>(field[0],0x18)=-3000; frame_at(hz60); assert(!proxies[w].view_awake && proxies[w].visual_deferred);
    at<float>(field[0],0x18)=-2000; c=mock_tick_calls[w]; for(int i=0;i<4;++i){ at<float>(field[0],0x10)+=1; frame_at(hz60); }
    assert(!proxies[w].view_awake && mock_tick_calls[w]==c);
    at<float>(field[0],0x18)=20; at<float>(field[0],0x10)+=1; frame_at(hz60);
    assert(proxies[w].view_awake && mock_tick_calls[w]==c+1 && !proxies[w].visual_deferred);
    pass("edge hysteresis: stays prepared inside the outer ring, sleeps beyond it, wakes at once on screen");
    for(unsigned i=0;i<8;++i){ visible_checks=0; frame_at(hz60); }
    pass("every camera test uses the exact 1.0 screen bound (mock asserts it)");
    // 7) Budget: many units entering only the outer ring wake a few per update;
    //    units that are actually on screen always wake at once.
    fixture(200); settle(110);
    for(unsigned u=0;u<200;++u) at<float>(field[u],0x18)=-9000;
    frame_at(hz60); frame_at(hz60);
    for(unsigned u=0;u<200;++u) assert(!proxies[slot_of(u)].view_awake);
    for(unsigned u=0;u<100;++u) at<float>(field[u],0x18)=-1000;   // edge ring only
    for(unsigned u=100;u<200;++u) at<float>(field[u],0x18)=20;    // on screen
    perf_prewarm_wakes=perf_prewarm_deferred=0; frame_at(hz60);
    unsigned ring=0,screen=0;
    for(unsigned u=0;u<100;++u) ring+=proxies[slot_of(u)].view_awake;
    for(unsigned u=100;u<200;++u) screen+=proxies[slot_of(u)].view_awake;
    assert(screen==100 && ring==FL_PREWARM_PER_UPDATE);
    for(int i=0;i<24;++i) frame_at(hz60);
    ring=0; for(unsigned u=0;u<100;++u) ring+=proxies[slot_of(u)].view_awake;
    assert(ring==100 && !blocked);
    pass("outer-ring preparation is spread over updates; on-screen units are never delayed");
    // 8) 500 units at 120FPS in combat: bounded, no failure, draws complete.
    fl_test_qpc_override=hf5_qpc; engage(500);
    for(int i=0;i<10;++i) frame_at(hz120);
    perf_half_rate_skips=0; unsigned before_sub=submissions;
    for(int i=0;i<60;++i) frame_at(hz120);
    assert(!blocked && FL_Status.ready_count==500 && submissions-before_sub==500u*60u && perf_half_rate_skips>0);
    printf("INFO 500 units x 60 updates at 120FPS: holds=%llu\n",(unsigned long long)perf_half_rate_skips);
    pass("500 engaged units at 120FPS keep every model drawn while about half the loop updates are held");
    // 9) Uneven 60FPS (alternating long/short frames, and hitch + catch-up) never holds.
    fl_test_qpc_override=hf5_qpc; engage(2);
    for(int i=0;i<10;++i) frame_at(hz60);
    perf_half_rate_skips=0;
    for(int i=0;i<240;++i) frame_at((i&1)?8.333:25.0);
    for(int i=0;i<60;++i) frame_at(i%5==3?30.0:i%5==4?3.4:16.667);
    assert(perf_half_rate_skips==0);
    pass("uneven 60FPS frame pacing never triggers a hold");
    // 10) No catch-up after a gap in field updates, and none for a restarted clip.
    for(int i=0;i<30;++i) frame_at(hz120);
    unsigned g=slot_of(0); bool tested=false;
    for(int i=0;i<40 && !tested;++i){ frame_at(hz120); if(proxies[g].half_skipped){ at<int>(manager,0x498)=1; for(int k=0;k<12;++k) frame_at(hz120); at<int>(manager,0x498)=0; uint64_t t=mock_tick_calls[g]; frame_at(hz120); assert(mock_tick_calls[g]==t+1 && fabsf(mock_seen_rate[g]-120.0f)<2.0f); tested=true; } }
    assert(tested);
    pass("after a pause in field updates a held unit advances one normal frame, never a fast-forward");
    tested=false;
    for(int i=0;i<200 && !tested;++i){
        frame_at(hz120);
        if(proxies[g].half_skipped){
            uint64_t t=mock_tick_calls[g];
            lock_status(); presentation_ms=fl_test_now_ms; set_pose(proxies[g],POSE_STRONG,g,true); unlock_status();
            frame_at(hz120);
            if(mock_tick_calls[g]==t+1){ assert(fabsf(mock_seen_rate[g]-120.0f)<2.0f); tested=true; }
        }
    }
    assert(tested);
    pass("a clip that restarts right after a hold starts on a normal single-frame step");
    fl_test_qpc_override=0;
    puts("SUCCESS HF5 FieldPerf tests; synthetic native resources, scripted clock, no live game/FPS measurement");
}
