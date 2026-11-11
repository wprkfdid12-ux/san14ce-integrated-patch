#pragma once
static float r48_hook_step(unsigned slot,unsigned part,float previous,float evaluated,float hz,float native_result) {
    auto &p=proxies[slot];private_presentation_depth=1;private_sound_proxy=&p;animation_rate=hz;
    void *h=at<void *>(p.actor,0x68+part*0x30),*setting=at<void *>(h,0x20);
    r48_evaluation_model=at<void *>(at<void *>(setting,0x30),0x20);
    at<float>(setting,0x24)=previous;at<float>(setting,0x28)=60;
    r48_input_seconds=previous/60;r48_evaluated_seconds=evaluated/60;float out=99;
    unsigned before=r48_evaluations,commits=r48_commits;
    assert(FL_HookMotionEvaluate(r48_evaluation_model,frame_arg,engine,55,r48_input_seconds,app,&out)==r48_evaluator_result);
    FL_HookMotionCommit(setting,native_result);
    assert(r48_evaluations==before+1 && r48_commits==commits+1 && !motion_evaluation.proxy);
    private_sound_proxy=NULL;private_presentation_depth=0;
    return at<float>(setting,0x24);
}
struct R48Skeleton {
    alignas(16) LocalBonePose bones[2]={};
    unsigned char instance[32]={},skeleton[16]={},descriptor[16]={};
    explicit R48Skeleton(void *model) {
        at<void *>(model,0x150)=instance;at<void *>(instance,0)=skeleton;at<void *>(skeleton,0)=descriptor;
        at<int16_t>(descriptor,4)=2;at<LocalBonePose *>(instance,0x10)=bones;raw(0);
    }
    void raw(float x) {r47_raw(bones,2,1);bones[1].position[0]=x;}
};
static void run_r48_tests() {
    fixture(2);settle();
    assert(r48_hook_step(0,0,10.25f,10.25f,60,0)==11.25f);
    proxies[0].pose=POSE_MOVE;
    assert(r48_hook_step(0,1,1,1,120,1.375f)==1.375f);
    assert(r48_hook_step(0,1,1.375f,1.375f,120,-0.625f)==1.75f);
    assert(r48_hook_step(0,1,120.25f,0.25f,120,-0.625f)==0.625f);
    assert(proxies[0].motion_sample[1].period==120 && perf_motion_wraps==1);
    pass("R4.8 production evaluator/commit hooks preserve subframes, movement increments and genuine loop overshoot with exactly one native evaluation");

    r48_evaluator_writes=false;
    assert(r48_hook_step(0,0,20.25f,99,60,-0.25f)==20.25f && perf_motion_invalid==1);
    r48_evaluator_writes=true;
    assert(r48_hook_step(0,0,21.25f,NAN,60,-0.25f)==21.25f && perf_motion_invalid==2);
    assert(r48_hook_step(0,0,21.25f,21.25f,0,-0.25f)==21.25f && perf_motion_invalid==3);
    pass("R4.8 missing evaluator output, NaN and invalid clock hold the last valid private frame without propagating negative time");

    float out=77;r48_input_seconds=0.5f;r48_evaluation_model=models[0][0];r48_evaluator_writes=false;
    FL_HookMotionEvaluate(models[0][0],frame_arg,engine,55,.5f,app,&out);
    assert(out==77 && !motion_evaluation.proxy);
    FL_HookMotionCommit(settings[0][0],-0.625f);assert(at<float>(settings[0][0],0x24)==-0.625f);
    private_presentation_depth=1;private_sound_proxy=&proxies[0];r48_evaluator_writes=true;r48_evaluated_seconds=.5f;
    FL_HookMotionEvaluate(models[0][0],frame_arg,engine,55,.5f,app,&out);
    FL_HookMotionCommit(settings[1][0],77);assert(at<float>(settings[1][0],0x24)==77);
    private_presentation_depth=0;private_sound_proxy=NULL;
    pass("R4.8 real duel/out-of-scope calls and mismatched private settings retain original arguments, output and commit values");

    auto old_sample=proxies[0].motion_sample[1];private_staging=true;
    assert(r48_hook_step(0,1,120.75f,.75f,120,0)==1.125f);
    private_staging=false;assert(!memcmp(&old_sample,&proxies[0].motion_sample[1],sizeof old_sample));
    pass("R4.8 staging updates correct their own clock without overwriting live-bank cycle evidence");

    fixture(20);settle(20);
    float phases[20],offsets[20];for(unsigned i=0;i<20;++i)phases[i]=offsets[i]=proxies[i].loop_offset;
    const float rates[]={60,61,59,90,120,37,55};
    for(unsigned n=0;n<3000;++n)for(unsigned i=0;i<20;++i)
        phases[i]=r48_hook_step(i,0,phases[i],fmodf(phases[i],120),rates[n%7],0);
    for(unsigned i=1;i<20;++i){
        assert(fabsf(remainderf((phases[i]-phases[0])-(offsets[i]-offsets[0]),120))<.015f);
        for(unsigned j=0;j<i;++j)assert(fabsf(remainderf(phases[i]-phases[j],120))>.001f);
    }
    pass("R4.8 20 independently seeded units retain all 20 phases across 3000 variable-rate updates and repeated real loops");

    fixture(1);settle();auto &p=proxies[0];
    R48Skeleton body(models[0][0]),horse(models[0][1]);body.raw(20);horse.raw(30);
    presentation_ms=2000;set_pose(p,POSE_MOVE,0);assert(p.blend_count==2 && p.blend_horse_count==2);
    body.raw(0);horse.raw(10);blend_attack_recovery(p,models[0][0]);
    assert(body.bones[1].position[0]==20 && horse.bones[1].position[0]==30);
    body.raw(0);horse.raw(10);presentation_ms=2050;blend_attack_recovery(p,models[0][0]);
    assert(body.bones[1].position[0]==10 && horse.bones[1].position[0]==20);
    body.raw(0);horse.raw(10);presentation_ms=2100;blend_attack_recovery(p,models[0][0]);
    assert(body.bones[1].position[0]==0 && horse.bones[1].position[0]==10 && !p.blend_duration_ms);
    pass("R4.8 standing-to-move transitions blend rider and horse together, reach the new pose in100ms and add no native animation call");

    presentation_ms=2200;set_pose(p,POSE_IDLE,0);assert(p.blend_duration_ms==140 && p.blend_horse_count==2);
    presentation_ms=2400;set_pose(p,POSE_ATTACK,0);assert(p.blend_duration_ms==90 && !p.blend_horse_count);
    assert(fabsf(pose_facing_offset(p))<.0001f);
    presentation_ms=2445;assert(fabsf(pose_facing_offset(p)+.7853981634f)<.0001f);
    float facing=pose_facing_offset(p);set_pose(p,POSE_IDLE,0);
    assert(fabsf(pose_facing_offset(p)-facing)<.0001f);
    presentation_ms=2585;assert(fabsf(pose_facing_offset(p))<.0001f);
    set_pose(p,POSE_ATTACK,0);float before=body.bones[1].position[0];private_staging=true;
    blend_attack_recovery(p,alt_models[0][0]);private_staging=false;
    assert(p.blend_count==2 && body.bones[1].position[0]==before);
    set_pose(p,POSE_HIT,0);assert(!p.blend_duration_ms && !p.blend_count && !p.blend_horse_count);
    pass("R4.8 movement stop and attack facing blend continuously through interrupted transitions; standing combat preserves the horse clock; hits stay immediate");

    uint64_t full_copies=0,reduced_copies=0,misses=IdleSeam::history_misses;
    for(float period:{60.0f,120.0f,151.0f}) {
        IdleSeam::State full={},reduced={};IdleSeam::Key key{1,2,3,4,5,55};
        alignas(16) IdleSeam::Pose x[32],y[32];float frame=0;uint64_t ms=1000;bool learned=false;
        for(unsigned n=1;n<4001;++n) {
            float step=n%113==0?14.5f:60/rates[n%7];float next=fmodf(frame+step,period);
            if(next<frame)learned=true;frame=next;ms+=uint64_t(step*1000/60);
            r47_raw(x,32,frame);memcpy(y,x,sizeof x);
            auto copies=IdleSeam::copied_bones;IdleSeam::apply(full,key,x,32,frame,n,ms);
            full_copies+=IdleSeam::copied_bones-copies;
            copies=IdleSeam::copied_bones;IdleSeam::apply(reduced,key,y,32,frame,n,ms,learned?period:0,15);
            reduced_copies+=IdleSeam::copied_bones-copies;assert(!memcmp(x,y,sizeof x));
        }
    }
    assert(reduced_copies<full_copies/2 && IdleSeam::history_misses==misses);
    printf("METRIC idle_history full_copied_bones=%llu reduced_copied_bones=%llu\n",(unsigned long long)full_copies,(unsigned long long)reduced_copies);
    pass("R4.8 learned-cycle history eliminates over half the copied bones with byte-identical output across12000 variable-rate updates, real loops and long permitted steps");
}
