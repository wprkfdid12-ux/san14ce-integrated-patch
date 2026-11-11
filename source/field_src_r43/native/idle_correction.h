#pragma once
// Exact placement of the former R4.4 IdleBridge: before final field scaling
// and saddle/weapon attachment. No second helper is installed in this build.
static IdleSeam::State idle_states[FL_PROXY_COUNT][2];
static uint64_t idle_sample_counter[2];
static void reset_idle_state(IdleSeam::State &s,unsigned part,unsigned reason) {
    if(s.active) { ++perf_idle_resets[part];++perf_idle_reset_reason[reason]; }
    IdleSeam::reset(s);
}
static void apply_idle_correction(Proxy &p) {
    // Staging gets its own first pose at swap; do not destroy the displayed
    // bank's history while a second bank is still loading.
    if(private_staging) return;
    auto *states=idle_states[unsigned(&p-proxies)];
    // HF4 has a fixed standing pose, so loop-seam history is unnecessary.
    if(p.pose==POSE_IDLE) {
        reset_idle_state(states[0],0,0);reset_idle_state(states[1],1,0);return;
    }
    if(p.pose!=POSE_IDLE || p.running || p.combat_engaged || p.defeated) {
        reset_idle_state(states[0],0,0);reset_idle_state(states[1],1,0);return;
    }
    for(unsigned part=0;part<2;++part) {
        auto &s=states[part];
        ++perf_idle_calls[part];
        uint64_t start=(++idle_sample_counter[part]&63)==1?performance_tick():0;
        struct EndSample { unsigned part; uint64_t start; ~EndSample(){if(start)performance_record(part?perf_horse:perf_body,start);} } timer{part,start};
        const size_t off=0x58+part*0x30;
        if(!at<int>(p.actor,off+0x18)||at<int>(p.actor,off+0x28)) {reset_idle_state(s,part,3);continue;}
        void *handle=at<void *>(p.actor,off+0x10);
        void *setting=handle?at<void *>(handle,0x20):NULL;
        void *child=setting?at<void *>(setting,0x30):NULL;
        void *model=child?at<void *>(child,0x20):NULL;
        unsigned count=0;auto *bones=private_bones(model,count);
        if(!bones || ((uintptr_t)bones&15)) {reset_idle_state(s,part,3);continue;}
        IdleSeam::Key key{(uintptr_t)p.actor,(uintptr_t)setting,(uintptr_t)model,(uintptr_t)bones,p.serial,at<int>(p.duel,0x28+part*4)};
        float frame=at<float>(setting,0x24);
        if(s.active && (s.count!=count || !IdleSeam::same(s.key,key))) reset_idle_state(s,part,1);
        if(s.active && (presentation_ms<s.last_ms || presentation_ms-s.last_ms>250)) reset_idle_state(s,part,2);
        if(s.active && frame+0.001f<s.frame)++perf_idle_wraps[part];
        bool was_active=s.active;uint64_t before=IdleSeam::copied_bones;
        uint64_t skipped=IdleSeam::skipped_copies;
        auto &sample=p.motion_sample[part];
        bool observed=sample.frame==FL_Status.frame && sample.setting==setting && sample.model==model && sample.action==key.action;
        IdleSeam::sample_copy=start!=0;IdleSeam::sample_part=part;
        IdleSeam::apply(s,key,bones,count,frame,FL_Status.frame,presentation_ms,
            observed?sample.period:0,observed?sample.clip_rate*0.25f:0);
        IdleSeam::sample_copy=false;
        perf_idle_bones[part]+=IdleSeam::copied_bones-before;
        perf_idle_copy_avoided+=IdleSeam::skipped_copies-skipped;
        if(was_active && !s.active) {++perf_idle_resets[part];++perf_idle_reset_reason[4];}
    }
}
