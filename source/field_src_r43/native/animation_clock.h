#pragma once
#include "frame_step_core.h"
// Measured once per displayed field update; native gameplay clocks stay intact.
static uint64_t animation_last_tick;
static float animation_rate=60.0f;
// HF5: smoothed update interval (seconds); 0 = unknown. Only gates the 120Hz hold.
static double animation_interval_avg_s;
static void advance_animation_clock() {
    uint64_t tick=performance_tick();
    if(tick && animation_last_tick && tick>animation_last_tick && performance_frequency) {
        double seconds=double(tick-animation_last_tick)/double(performance_frequency);
        // Do not fast-forward after loading, debugger stops or returning to map.
        if(seconds>0 && seconds<0.25) {
            animation_rate=float(1.0/seconds);
            animation_interval_avg_s=animation_interval_avg_s>0?animation_interval_avg_s*0.875+seconds*0.125:seconds;
        } else { animation_rate=60.0f; animation_interval_avg_s=0; }
    } else { animation_rate=60.0f; animation_interval_avg_s=0; }
    animation_last_tick=tick;
}
// HF5: a unit whose previous update was a 120Hz half-rate hold advances by its
// OWN elapsed time on the next evaluation, so clips keep real-time speed.
// Zero means "use the shared field clock" (every pre-HF5 path is unchanged).
static thread_local float private_clock_rate;
static float presentation_clock_rate() {
    return private_clock_rate>0 ? private_clock_rate : animation_rate;
}
extern "C" __declspec(dllexport) float FL_HookAnimationRate(void *clock) {
    if(private_presentation_depth && private_sound_proxy) return private_sound_proxy->pose==POSE_MOVE ? presentation_clock_rate()/0.75f : presentation_clock_rate();
    return fn<float (*)(void *)>(0x937a00)(clock);
}

// Hooks only the two guarded E8 calls inside 37a720. No model asset, native
// duel clock, original caller's stack layout or renderer entry is replaced.
struct MotionEvaluation { Proxy *proxy; void *model; float seconds; bool valid; };
static thread_local MotionEvaluation motion_evaluation;
using MotionEvaluate=unsigned char (*)(void *,void *,void *,int,float,void *,float *);
extern "C" __declspec(dllexport) unsigned char FL_HookMotionEvaluate(void *model,void *frame,void *engine,
        int action,float time,void *options,float *out) {
    Proxy *p=private_presentation_depth?private_sound_proxy:NULL;
    if(p && p->pose==POSE_IDLE && !p->defeated) time=0.0f;
    // A successful fallback resource path does not always initialize out[0].
    if(p && out)out[0]=-1.0f;
    unsigned char result=fn<MotionEvaluate>(FL_MOTION_EVALUATE)(model,frame,engine,action,time,options,out);
    if(p) motion_evaluation={p,model,out?out[0]:NAN,result!=0 && out && isfinite(out[0]) && out[0]>=0};
    return result;
}
extern "C" __declspec(dllexport) void FL_HookMotionCommit(void *setting,float native_next) {
    Proxy *p=private_presentation_depth?private_sound_proxy:NULL;
    MotionEvaluation evaluated=motion_evaluation;
    if(p && evaluated.proxy==p) {
        motion_evaluation={};
        // Match the exact live setting and model of the current private bank.
        for(unsigned part=0;part<2;++part) {
            void *handle=at<void *>(p->actor,0x68+part*0x30);
            if(!handle || at<void *>(handle,0x20)!=setting)continue;
            void *child=at<void *>(setting,0x30);
            if(!child || at<void *>(child,0x20)!=evaluated.model)break;
            float previous=at<float>(setting,0x24),rate=at<float>(setting,0x28);
            auto result=FrameStep::advance(previous,evaluated.valid?evaluated.seconds:NAN,rate,
                p->pose==POSE_MOVE?presentation_clock_rate()/0.75f:presentation_clock_rate());
            ++perf_motion_commits;
            if(!result.valid) {
                ++perf_motion_invalid;
                native_next=isfinite(previous)&&previous>=0?previous:0;
            } else {
                float original_next=native_next;
                if(fabsf(native_next-result.next)>0.0001f)++perf_motion_corrected;
                if(result.wrapped)++perf_motion_wraps;
                native_next=result.next;
                if(!private_staging) {
                    auto &sample=p->motion_sample[part];
                    int action=at<int>(p->duel,0x28+part*4);
                    if(sample.setting!=setting || sample.model!=evaluated.model || sample.action!=action)sample={};
                    sample.setting=setting;sample.model=evaluated.model;sample.action=action;
                    sample.before=previous;sample.after=result.next;sample.clip_rate=rate;
                    sample.evaluated=evaluated.seconds*rate;sample.native_after=original_next;
                    sample.frame=FL_Status.frame;
                    // Learn only observed full cycles, not clamped one-shot ends.
                    if(result.wrapped && result.period>=16 && result.period<=10000)
                        sample.period=result.period;
                }
            }
            if(p->pose==POSE_IDLE && !p->defeated) native_next=0.0f;
            break;
        }
    }
    fn<void (*)(void *,float)>(FL_MOTION_SET_FRAME)(setting,native_next);
}
static void log_motion_samples() {
    // At most four live units per five-second summary, never a per-frame log.
    unsigned logged=0;
    for(unsigned slot=0;slot<FL_PROXY_COUNT && logged<4;++slot) {
        const auto &p=proxies[slot];if(!p.ready || !p.constructed)continue;
        bool current=false;
        for(unsigned part=0;part<2;++part) {
            const auto &s=p.motion_sample[part];if(!s.setting || s.frame!=FL_Status.frame)continue;
            current=true;
            log_line("MOTION_SAMPLE slot=%u officer=%u part=%u action=%d tick=%u before=%.6f evaluated=%.6f native_next=%.6f corrected_next=%.6f clip_rate=%.3f observed_period=%.6f",
                slot,p.source.id,part,s.action,s.frame,s.before,s.evaluated,s.native_after,s.after,s.clip_rate,s.period);
        }
        if(current)++logged;
    }
}
