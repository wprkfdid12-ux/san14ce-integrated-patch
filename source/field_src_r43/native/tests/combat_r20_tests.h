#pragma once
static void seed_result(float chance,bool result) {
    for(uint64_t s=1;s<1000;++s) { guard_rng=s; bool win=guard_random()<chance; if(win==result) {guard_rng=s;return;} }
    assert(false);
}
static void engage(unsigned count=2) {
    fixture(count); settle(count/2+3);
    for(unsigned i=0;i<count;++i) at<int>(field[i],0x60)=2;
    update(); draw_frame(); update();
}
static int normal_event(unsigned a=0,unsigned d=1,float aa=100,float ad=100,float da=100,float dd=100) {
    int damage=-1; FL_HookNormalDamage(armies[a],aa,ad,armies[d],da,dd,&damage); return damage;
}
static bool combat_sound(unsigned slot,uint32_t program) {
    /* Real frames include a draw between updates. These manually injected
       contact events refresh the same visibility mix after a current draw. */
    draw_frame(); lock_status(); prepare_combat_mix(); unlock_status();
    at<uint32_t>(hoof_params,0)=program; return private_hoof(slot);
}
static void damaging_tactic(unsigned a=0,unsigned d=1,bool child=false,bool duplicate=false) {
    alignas(16) unsigned char value[0x40]={},tactic_data[0x80]={},record[16]={};
    at<int>(tactic_data,0)=1; at<int>(tactic_data,4)=duplicate?1:0;
    at<void *>(value,0)=tactic_data; at<int>(value,8)=0x1b; at<int>(value,0xc)=int(a+1);
    at<void *>(value,0x10)=record; at<void *>(value,0x18)=record+16;
    at<int>(record,0)=0x1b; at<int>(record,4)=int(d+1); at<uint16_t>(record,8)=300;
    if(duplicate) {at<void *>(value,0x28)=record; at<void *>(value,0x30)=record+16;}
    (child?FL_HookTacticChild:FL_HookTacticEffect)(value,manager,app);
    FL_HookTacticDamage(app,300,armies[d],0x34);
}
static void run_combat_r20_tests() {
    assert(fabsf(guard_probability(100,100,100,100)-0.55f)<1e-6f);
    assert(guard_probability(50,50,100,100)>guard_probability(100,100,100,100));
    assert(guard_probability(150,150,100,100)<guard_probability(100,100,100,100));
    for(float r:{0.001f,0.5f,0.65f,0.8f,1.0f,1.25f,1.5f,1.75f,100.f}) {
        float p=guard_probability(100,100,r*100,r*100); assert(p>=0.15f && p<=0.80f);
    }
    assert(guard_probability(NAN,1,1,1)<0 && guard_probability(0,0,10,10)<0);
    assert(guard_probability(100,100,100,100,2)<guard_probability(100,100,100,100,1));
    pass("guard chance follows current attack-plus-defense totals with bounded minor corrections and rejects invalid values");

    engage(); seed_result(0.55f,true);
    std::vector<unsigned char> before((unsigned char *)armies,(unsigned char *)armies+sizeof armies);
    int damage=normal_event(0,1,101,102,103,104);
    assert(damage==123 && normal_damage_calls==1 && normal_source==armies[0] && normal_target==armies[1]);
    for(unsigned i=0;i<4;++i) assert(normal_operands[i]==101+i);
    assert(!memcmp(before.data(),armies,sizeof armies) && proxies[1].guard_draws==1);
    assert(!proxies[1].hit_pending && proxies[0].opponent_serial==proxies[1].serial);
    mock_damage=0; normal_event(); assert(proxies[1].guard_draws==1);
    mock_damage=123; normal_event(0,0); assert(proxies[0].guard_draws==0);
    pass("actual normal damage receives all seven original arguments once, leaves damage and armies untouched, and never queues hit39");

    engage(); guard_rng=7; unsigned wins=0;
    for(unsigned n=0;n<20000;++n) wins+=queue_guard(proxies[1],0.55f,false,false);
    assert(wins>10700 && wins<11300);
    guard_rng=71; proxies[1].guard_wins=0; bool first=queue_guard(proxies[1],0.55f,false,false);
    guard_rng=71; proxies[1].guard_wins=999999; bool again=queue_guard(proxies[1],0.55f,false,false);
    assert(first==again);
    pass("independent cosmetic RNG has the expected sampled rate and does not consult success streaks or game RNG");

    engage(); set_pose(proxies[1],POSE_BATTLE_IDLE,1,true); seed_result(0.55f,true); normal_event(); update();
    assert(proxies[1].pose==POSE_GUARD && at<int>(proxies[1].duel,0x28)==35 && at<int>(proxies[1].duel,0x2c)==0);
    unsigned transitions=proxies[1].motion_transitions;
    for(unsigned n=0;n<10;++n) {seed_result(0.55f,true);normal_event();update();}
    assert(proxies[1].motion_transitions==transitions && proxies[1].clip_frame>5);
    at<float>(settings[1][0],0x24)=58; update(); update(); assert(proxies[1].pose==POSE_ATTACK);
    pass("native guard35 completes and returns to combat; repeated successful rolls cannot freeze or restart the same guard");

    engage(); seed_result(0.55f,false);normal_event(); update();
    unsigned trials=guard_trials;
    combat_sound(0,0x762497dc); assert(guard_trials==trials && paused_guard_trials==0);
    proxies[0].normal_contact_done=false; mock_strategy_pause=true; update();
    set_pose(proxies[1],POSE_BATTLE_IDLE,1,true); seed_result(0.55f,true); combat_sound(0,0x762497dc);
    assert(guard_trials==trials+1 && paused_guard_trials==1 && proxies[1].guard_pending);
    combat_sound(0,0x762497dc); assert(guard_trials==trials+1);
    update(); assert(proxies[1].pose==POSE_GUARD);
    at<float>(settings[1][0],0x24)=58;update();update();assert(proxies[1].pose==POSE_ATTACK);
    assert(normal_damage_calls==1);
    pass("only the original strategy-pause state rolls once per visual contact from cached opponent scores, with no gameplay damage call or frozen guard");

    uint64_t old=proxies[1].serial;set_id(1,1234);update();draw_frame();update();
    assert(proxies[1].serial!=old);proxies[0].normal_contact_done=false;trials=guard_trials;
    combat_sound(0,0x762497dc);assert(guard_trials==trials);
    at<int>(field[0],0x60)=0;at<uint8_t>(armies[0],0x26)=0;update();assert(!proxies[0].opponent_serial);
    pass("paused presentation rejects a recycled opponent and clears the remembered pair when combat ends");

    engage(); private_sound_proxy=&proxies[0];private_presentation_depth=1;
    auto effect=[](){return FL_HookMotionEffect(manager,23,app,engine,context,67,1,2,3,4,5,6,7,8,9,10,11,12);};
    assert(effect()==NULL && motion_effect_calls==0 && suppressed_normal_effects==1);
    proxies[0].pose=POSE_STRONG;assert(effect()==callback && motion_effect_calls==1);
    proxies[0].pose=POSE_ATTACK;private_presentation_depth=0;assert(effect()==callback && motion_effect_calls==2);
    private_sound_proxy=NULL;
    pass("only private normal-attack VFX creation is suppressed; strong effects and the original duel forward all 18 arguments unchanged");

    engage(); seed_result(0.55f,true);damaging_tactic(0,1,false,true);
    assert(proxies[1].guard_draws==1 && proxies[1].reaction_waiting && !proxies[1].hit_pending);
    update(); assert(proxies[0].pose==POSE_STRONG && proxies[1].pose!=POSE_GUARD);
    assert(!combat_sound(0,0x762497dc) && !proxies[0].strong_contact_done);
    set_pose(proxies[1],POSE_BATTLE_IDLE,1,true); assert(combat_sound(0,0xdd622565)); update();
    assert(proxies[1].pose==POSE_GUARD && tactic_block_audio==1 && !tactic_hit_audio);
    assert(at<uint32_t>(last_native_params,0)==CLASH_PROGRAM);
    assert(!combat_sound(0,0xdd622565) && tactic_block_audio==1);
    pass("a tactic rolls once per target, including duplicate effect rows, and starts guard plus louder collision only on original strong-contact audio timing");

    engage();mock_scores[0][0]=mock_scores[0][1]=200;seed_result(0.55f,false);damaging_tactic();update();
    assert(combat_sound(0,0xdd622565));update();
    assert(proxies[1].pose==POSE_HIT && !tactic_block_audio);
    assert(!combat_sound(1,HIT_PROGRAM) && tactic_hit_audio==1 && !tactic_block_audio);
    assert(at<uint32_t>(last_native_params,0)==HIT_PROGRAM);
    const float hit_gain=last_native_gain;
    engage();assert(combat_sound(0,0x762497dc));assert(hit_gain>last_native_gain*4);
    pass("unblocked tactic contact emits one loud original hit independently of motion; the victim clip cannot duplicate it");

    engage();seed_result(0.55f,true);damaging_tactic(0,1,true);update();
    assert(proxies[0].pose==POSE_STRONG && tactic_pairs==1);
    set_id(1,1234);update();draw_frame();update();assert(!combat_sound(0,0xdd622565));
    assert(!proxies[1].guard_pending && !proxies[1].hit_pending);
    pass("linked tactic children use the same presentation and delayed contacts never hit a recycled defender");

    engage();seed_result(0.55f,true);damaging_tactic();update();
    fl_test_now_ms+=5001;assert(!combat_sound(0,0xdd622565));
    assert(!proxies[1].guard_pending && !proxies[1].hit_pending);
    FL_HookManagerRelease(manager,engine);for(auto &v:combat_voices)assert(v.until==0);
    pass("expired or torn-down tactic contacts and combat audio reservations are discarded");

    engage(100);draw_frame();visible_checks=distance_checks=0;update();
    unsigned audible=0;for(auto &p:proxies)if(p.combat_gain>0)++audible;
    assert(audible==COMBAT_SOURCES && distance_checks==100 && visible_checks<=32);
    float saved=proxies[0].combat_gain;
    for(unsigned i=0;i<100;++i) combat_sound(i,0x762497dc);
    assert(normal_audio<=NORMAL_VOICES && native_sound_calls<=NORMAL_VOICES);
    float sum=0;for(auto &v:combat_voices)if(v.until>fl_test_now_ms)sum+=v.gain;assert(sum<=0.80001f);
    proxies[0].pose=POSE_HIT;assert(combat_sound(0,HIT_PROGRAM));
    assert(proxies[0].combat_gain==saved && tactic_hit_audio==1);
    pass("crowds bound visible sound sources, view checks, normal voice count and total gain while reserving room for louder tactic audio");

    engage();at<float>(field[0],0x18)=-10;update();assert(!combat_sound(0,0x762497dc));
    assert(!native_sound_calls);uint32_t out=0;assert(sound_call(true,&out) && out==12345);
    engage();at<float>(hoof_params,8)=0;assert(!combat_sound(0,0x762497dc) && !native_sound_calls);
    pass("offscreen or natively muted private combat is silent and original non-private audio remains untouched");
}
