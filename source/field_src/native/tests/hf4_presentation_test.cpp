#define main preserved_legacy_main
#include "poc_lifecycle_test.cpp"
#undef main
#ifndef FL_HF4_MAIN_NAME
#define FL_HF4_MAIN_NAME main
#endif
int FL_HF4_MAIN_NAME() {
    fixture(2);settle();
    auto c=perf_actor_calls;auto idle=perf_idle_calls[0]+perf_idle_calls[1];
    for(int i=0;i<120;++i){fl_test_now_ms+=16;update();draw_frame();}
    assert(perf_actor_calls==c && perf_idle_calls[0]+perf_idle_calls[1]==idle && !blocked);
    pass("stationary mounted units reuse their pose for120 updates without actor/bone-history evaluation");
    at<float>(field[0],0x10)+=5;update();
    assert(proxies[0].pose==POSE_MOVE && perf_actor_calls==c+1 && proxies[1].static_valid);
    pass("moving visible unit resumes while the other standing unit stays cached");
    engage();assert(proxies[0].pose==POSE_ATTACK);
    mock_strategy_pause=true;update();
    for(unsigned i=0;i<2;++i){auto &p=proxies[i];assert(p.pose==POSE_IDLE && !p.running && !p.combat_engaged && at<int>(p.duel,0x28)==55 && at<int>(p.duel,0x2c)==0 && p.static_valid && fabsf(at<float>(p.duel,0xf0)-p.source.yaw)<0.0001f);}
    c=perf_actor_calls;for(int i=0;i<120;++i){fl_test_now_ms+=16;update();}
    assert(perf_actor_calls==c);
    pass("paused battle changes both mounted actors to standing and stops repeated animation work");
    mock_strategy_pause=false;update();assert(proxies[0].pose==POSE_ATTACK && perf_actor_calls>c);
    pass("unpause derives active battle behavior from the current native state");
    fixture(1);settle();auto serial=proxies[0].serial;c=perf_actor_calls;
    at<float>(field[0],0x18)=-9000; // HF5: beyond the edge-preparation ring
   at<int>(field[0],0x60)=1;
    for(int i=0;i<120;++i){at<float>(field[0],0x10)+=1;fl_test_now_ms+=16;update();draw_frame();}
    assert(perf_actor_calls==c && proxies[0].visual_deferred && constructions==1 && destructions==0 && proxies[0].source.x==at<float>(field[0],0x10));
    pass("offscreen motion skips120 actor updates but retains current source state and allocated model");
    at<float>(field[0],0x18)=20;update();draw_frame();
    assert(perf_actor_calls==c+1 && !proxies[0].visual_deferred && proxies[0].serial==serial && !blocked);
    assert(at<float>(proxies[0].duel,0xc0)==at<float>(field[0],0x10) && at<float>(models[0][1],0x18)==at<float>(field[0],0x10));
    pass("return to view updates the existing model at its current position before drawing");
    at<float>(field[0],0x18)=-9000;update();c=perf_actor_calls;mock_strategy_pause=true;update();
    at<float>(field[0],0x18)=20;update();assert(proxies[0].pose==POSE_IDLE && proxies[0].static_valid && perf_actor_calls==c+1);
    pass("return while paused evaluates a standing frame once");
    at<float>(field[0],0x744)+=0.4f;c=perf_actor_calls;update();assert(perf_actor_calls==c+1);
    c=perf_actor_calls;update();assert(perf_actor_calls==c);
    pass("standing yaw or transform changes invalidate the pose cache exactly once");
    at<float>(field[0],0x18)=-9000;update();at<int>(field[0],0x740)=1;update();assert(!proxies[0].constructed && destructions==1);
    pass("offscreen water/lifetime changes still release owned resources");
    fixture(1);load_ready=false;for(int i=0;i<3;++i)update();assert(!proxies[0].ready);
    load_ready=true;settle();assert(proxies[0].ready && proxies[0].static_valid);
    pass("not-ready resources continue loading and become cacheable only after readiness and attachment checks");
    fixture(500);settle(251);c=perf_actor_calls;update();draw_frame();assert(perf_actor_calls==c && FL_Status.ready_count==500 && !blocked);
    pass("500 stationary mounted models keep drawing with zero repeated actor updates");
    FL_HookManagerRelease(manager,engine);assert(destructions==500);
    pass("owner teardown releases all cached models");
    fixture(1);settle();auto &p=proxies[0];
    p.deferred_generation=draw_generation;p.deferred_mask=0;
    if(FL_FOG_VARIANT){assert(!replacement_draw_eligible(p) && p.fog_hidden);p.deferred_mask=1;assert(replacement_draw_eligible(p) && !p.fog_hidden);}
    else assert(replacement_draw_eligible(p));
    pass("normal/Fog draw eligibility keeps its original variant policy");
    fixture(1);settle();auto &q=proxies[0];
    private_presentation_depth=1;private_sound_proxy=&q;
    r48_evaluation_model=models[0][0];r48_input_seconds=0;r48_evaluated_seconds=1;
    float result=-1;
    assert(FL_HookMotionEvaluate(models[0][0],frame_arg,engine,55,12.0f,app,&result));
    FL_HookMotionCommit(settings[0][0],123.0f);
    assert(at<float>(settings[0][0],0x24)==0.0f);
    private_presentation_depth=0;private_sound_proxy=NULL;r48_input_seconds=12.0f;
    assert(FL_HookMotionEvaluate(models[0][0],frame_arg,engine,55,12.0f,app,&result));
    FL_HookMotionCommit(settings[0][0],123.0f);assert(at<float>(settings[0][0],0x24)==123.0f);
    pass("standing evaluation fixes time only for private field models; original duel evaluation and commit forward unchanged");
    fixture(1);settle();auto &t=proxies[0];set_pose(t,POSE_STRONG,0,true);
    at<float>(field[0],0x18)=-3000;update();fl_test_now_ms+=5000;
    at<int>(field[0],0x60)=1;at<float>(field[0],0x10)+=5;at<float>(field[0],0x18)=20;update();
    assert(t.pose==POSE_MOVE && !t.visual_deferred);
    pass("camera re-entry discards an old suspended tactic and resumes actual current movement");
    fixture(1);settle();events.clear();c=perf_actor_calls;
    for(int i=0;i<100;++i)update();
    assert(perf_actor_calls==c && std::count(events.begin(),events.end(),1)==100);
    pass("native manager updates keep running while private standing animation work is skipped");
    run_registry_tests();
    puts("SUCCESS HF4 presentation lifecycle tests; synthetic native resources, no live game/FPS measurement");
    return 0;
}
