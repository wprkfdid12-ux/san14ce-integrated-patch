#pragma once
// CPU/callback measurements, never claimed as GPU Present FPS. Per-actor
// timings sample 1/64 calls; total call counts and copied bones are exact.
struct PresentationCost { uint64_t total, maximum; unsigned count; const char *name; };
static PresentationCost perf_update={0,0,0,"private_update"},perf_draw={0,0,0,"private_draw"};
static PresentationCost perf_original_update={0,0,0,"original_manager_update"},perf_original_draw={0,0,0,"original_manager_draw"};
static PresentationCost perf_actor={0,0,0,"actor_update_sampled64"},perf_army={0,0,0,"army_graphics_sampled64"};
static PresentationCost perf_body={0,0,0,"idle_body_sampled64"},perf_horse={0,0,0,"idle_horse_sampled64"};
static PresentationCost perf_draw_setup={0,0,0,"draw_map_build"};
static PresentationCost perf_body_copy={0,0,0,"body_copy_sampled64"},perf_horse_copy={0,0,0,"horse_copy_sampled64"};
static PresentationCost perf_guard={0,0,0,"combat_event"};
static uint64_t perf_update_hooks,perf_draw_hooks,perf_actor_calls,perf_army_calls;
static uint64_t perf_group_hooks,perf_action_hooks,perf_mapped_hooks,perf_submissions;
static uint64_t perf_submit_failures,perf_fallback_draws,perf_pose_restarts;
static uint64_t perf_idle_calls[2],perf_idle_bones[2],perf_idle_resets[2],perf_idle_wraps[2];
static uint64_t perf_idle_reset_reason[6]; // pose, identity, gap, resource, invalid, transition
static uint64_t perf_motion_commits,perf_motion_corrected,perf_motion_invalid,perf_motion_wraps;
static uint64_t perf_idle_copy_avoided,perf_transition_captures;
static uint64_t perf_static_skips,perf_offscreen_skips;
static uint64_t perf_half_rate_skips,perf_prewarm_wakes,perf_prewarm_deferred,perf_view_sleeps;
static uint64_t performance_frequency,performance_window,performance_last_draw,performance_windows;
static uint64_t perf_frame_total,perf_frame_max;
static unsigned perf_frame_count,perf_frame_histogram[4096]; // 0.25ms bins
static uint64_t performance_tick() {
    LARGE_INTEGER value;
    if (!performance_frequency) {
        if (!QueryPerformanceFrequency(&value) || value.QuadPart<=0) return 0;
        performance_frequency=uint64_t(value.QuadPart);
    }
    return QueryPerformanceCounter(&value) ? uint64_t(value.QuadPart) : 0;
}
static void performance_record(PresentationCost &cost,uint64_t begin) {
    if(!begin) return;
    uint64_t end=performance_tick();
    if(end<begin || !performance_frequency) return;
    uint64_t elapsed=end-begin;
    cost.total+=elapsed; if(elapsed>cost.maximum)cost.maximum=elapsed; ++cost.count;
}
static void performance_draw_sample() {
    ++perf_draw_hooks;
    uint64_t now=performance_tick();
    if(performance_last_draw && now>performance_last_draw && performance_frequency) {
        uint64_t dt=now-performance_last_draw;
        double milliseconds=double(dt)*1000.0/double(performance_frequency);
        unsigned bin=milliseconds>=1023.75?4095:unsigned(milliseconds*4.0);
        ++perf_frame_histogram[bin]; ++perf_frame_count; perf_frame_total+=dt;
        if(dt>perf_frame_max)perf_frame_max=dt;
    }
    performance_last_draw=now;
}
static double performance_percentile(unsigned percent) {
    if(!perf_frame_count)return 0;
    unsigned target=(perf_frame_count*percent+99)/100,seen=0;
    for(unsigned i=0;i<4096;++i)if((seen+=perf_frame_histogram[i])>=target)return (i+1)*0.25;
    return 1024;
}
static void performance_summary(unsigned ready,unsigned targets) {
    uint64_t now=performance_tick();
    if(!performance_window || now<performance_window) {performance_window=now;return;}
    if(!performance_frequency || now-performance_window<5*performance_frequency)return;
    double seconds=double(now-performance_window)/double(performance_frequency);
    double factor=1000.0/double(performance_frequency);
    ++performance_windows;
    log_line("HF4_SKIPS window=%llu static_pose=%llu offscreen=%llu",
        (unsigned long long)performance_windows,(unsigned long long)perf_static_skips,(unsigned long long)perf_offscreen_skips);
    log_line("HF5_FIELDPERF window=%llu half_rate_holds=%llu prewarm_wakes=%llu prewarm_deferred=%llu view_sleeps=%llu",
        (unsigned long long)performance_windows,(unsigned long long)perf_half_rate_skips,(unsigned long long)perf_prewarm_wakes,
        (unsigned long long)perf_prewarm_deferred,(unsigned long long)perf_view_sleeps);
    perf_half_rate_skips=perf_prewarm_wakes=perf_prewarm_deferred=perf_view_sleeps=0;
    perf_static_skips=perf_offscreen_skips=0;
    log_line("PERF_WINDOW n=%llu span_s=%.3f ready=%u targets=%u update_hz=%.3f draw_callback_hz=%.3f actor_calls_s=%.3f army_calls_s=%.3f group_calls_s=%.3f action_calls_s=%.3f mapped_calls_s=%.3f submissions_s=%.3f submit_failures=%llu fallback_draws=%llu restarts=%llu present_fps_measured=0",
        (unsigned long long)performance_windows,seconds,ready,targets,perf_update_hooks/seconds,perf_draw_hooks/seconds,
        perf_actor_calls/seconds,perf_army_calls/seconds,perf_group_hooks/seconds,perf_action_hooks/seconds,
        perf_mapped_hooks/seconds,perf_submissions/seconds,(unsigned long long)perf_submit_failures,
        (unsigned long long)perf_fallback_draws,(unsigned long long)perf_pose_restarts);
    PresentationCost *costs[]={&perf_update,&perf_draw,&perf_original_update,&perf_original_draw,&perf_actor,&perf_army,&perf_body,&perf_horse,&perf_body_copy,&perf_horse_copy,&perf_draw_setup,&perf_guard};
    for(auto *c:costs) {
        log_line("PERFORMANCE window=%llu scope=%s samples=%u mean_ms=%.6f max_sample_ms=%.6f total_sample_ms=%.6f",
            (unsigned long long)performance_windows,c->name,c->count,c->count?double(c->total)*factor/c->count:0,
            double(c->maximum)*factor,double(c->total)*factor);
        c->total=c->maximum=0;c->count=0;
    }
    log_line("DRAW_INTERVAL window=%llu samples=%u mean_ms=%.4f max_ms=%.4f p95_upper_ms=%.2f p99_upper_ms=%.2f gpu_present_or_1pct_low=0",
        (unsigned long long)performance_windows,perf_frame_count,perf_frame_count?perf_frame_total*factor/perf_frame_count:0,
        perf_frame_max*factor,performance_percentile(95),performance_percentile(99));
    for(unsigned i=0;i<2;++i) {
        log_line("IDLE_COST window=%llu part=%s calls_s=%.3f copied_bones_s=%.3f resets=%llu wraps=%llu timing_sampling=64",
            (unsigned long long)performance_windows,i?"horse":"body",perf_idle_calls[i]/seconds,
            perf_idle_bones[i]/seconds,(unsigned long long)perf_idle_resets[i],(unsigned long long)perf_idle_wraps[i]);
        perf_idle_calls[i]=perf_idle_bones[i]=perf_idle_resets[i]=perf_idle_wraps[i]=0;
    }
    log_line("IDLE_RESET window=%llu pose=%llu identity=%llu gap=%llu resource=%llu invalid=%llu transition=%llu",
        (unsigned long long)performance_windows,(unsigned long long)perf_idle_reset_reason[0],(unsigned long long)perf_idle_reset_reason[1],
        (unsigned long long)perf_idle_reset_reason[2],(unsigned long long)perf_idle_reset_reason[3],(unsigned long long)perf_idle_reset_reason[4],(unsigned long long)perf_idle_reset_reason[5]);
    log_line("LOG_IO writes=%u synchronous_flushes=%u",log_write_count,log_flush_count);
    log_line("MOTION_CLOCK commits=%llu corrected_native_rounding=%llu true_wraps=%llu invalid_samples=%llu idle_history_copies_avoided=%llu transition_captures=%llu",
        (unsigned long long)perf_motion_commits,(unsigned long long)perf_motion_corrected,
        (unsigned long long)perf_motion_wraps,(unsigned long long)perf_motion_invalid,
        (unsigned long long)perf_idle_copy_avoided,(unsigned long long)perf_transition_captures);
    perf_motion_commits=perf_motion_corrected=perf_motion_wraps=perf_motion_invalid=0;
    log_motion_samples();
    perf_idle_copy_avoided=perf_transition_captures=0;
    memset(perf_idle_reset_reason,0,sizeof perf_idle_reset_reason);
    perf_update_hooks=perf_draw_hooks=perf_actor_calls=perf_army_calls=0;
    perf_group_hooks=perf_action_hooks=perf_mapped_hooks=perf_submissions=0;
    perf_submit_failures=perf_fallback_draws=perf_pose_restarts=0;
    perf_frame_count=0;perf_frame_total=perf_frame_max=0;memset(perf_frame_histogram,0,sizeof perf_frame_histogram);
    performance_window=now;
}
struct PresentationTimer {
    PresentationCost &cost; uint64_t begin;
    explicit PresentationTimer(PresentationCost &c):cost(c),begin(performance_tick()){}
    ~PresentationTimer(){performance_record(cost,begin);}
};
