#pragma once
static void run_combat_r21_tests() {
    const Pose equal_expected[]={POSE_BATTLE_IDLE,POSE_ATTACK,POSE_GUARD,POSE_ATTACK,POSE_HIT,POSE_ATTACK,POSE_GUARD,POSE_ATTACK};
    const Pose stronger_expected[]={POSE_BATTLE_IDLE,POSE_ATTACK,POSE_GUARD,POSE_GUARD,POSE_HIT,POSE_HIT,POSE_HIT,POSE_HIT};
    for(unsigned relation:{1u,2u}) for(unsigned mask=0;mask<8;++mask) for(bool own_skill:{false,true}) {
        engage(); Proxy &p=proxies[1];
        set_pose(p,(mask&1)?POSE_ATTACK:POSE_BATTLE_IDLE,1,true);
        p.opponent_relation=(unsigned char)relation;
        p.hit_relation=p.guard_relation=(unsigned char)relation;
        p.hit_pending=mask&4; p.guard_pending=mask&2;
        p.hit_pending_ms=p.guard_pending_ms=fl_test_now_ms;
        if(own_skill) tactic(2);
        update();
        assert(p.pose==(own_skill?POSE_STRONG:(relation==1?equal_expected:stronger_expected)[mask]));
        assert(!p.hit_pending && !p.guard_pending && !p.skill_pending);
    }
    pass("32 overlap cases honor STRONG first and NORMAL-GUARD-HIT versus HIT-GUARD-NORMAL according to current totals");

    assert(compare_strength(100,100,100,100)==1);
    assert(compare_strength(100,100,100,101)==2);
    assert(compare_strength(100,100,100,99)==1);
    for(float invalid:{NAN,-1.0f,2000000.0f}) assert(!compare_strength(invalid,100,100,100));
    engage(); seed_result(.55f,false);normal_event(0,1,80,120,100,100);
    assert(proxies[0].opponent_relation==1 && proxies[1].opponent_relation==1);
    normal_event(0,1,200,201,100,100);
    assert(proxies[0].opponent_relation==1 && proxies[1].opponent_relation==2);
    normal_event(0,1,50,60,100,100);
    assert(proxies[0].opponent_relation==2 && proxies[1].opponent_relation==1);
    pass("ties use the not-stronger branch, live native stat operands can reverse the relation, and invalid totals are not invented");

    engage(); Proxy &p=proxies[0];
    set_pose(p,POSE_GUARD,0,true); p.pose_relation=1;
    seed_result(.55f,false);normal_event(0,1,150,150,100,100); update();
    assert(p.pose==POSE_ATTACK);
    set_pose(p,POSE_HIT,0,true);p.pose_relation=2; unsigned transitions=p.motion_transitions;
    normal_event(0,1,50,50,100,100);update();assert(p.pose==POSE_HIT && p.motion_transitions==transitions);
    tactic(); update(); assert(p.pose==POSE_STRONG);
    transitions=p.motion_transitions;
    for(unsigned n=0;n<10;++n) {
        p.hit_pending=p.guard_pending=true; p.hit_pending_ms=p.guard_pending_ms=fl_test_now_ms;
        p.hit_relation=p.guard_relation=2;update();
    }
    assert(p.pose==POSE_STRONG && p.motion_transitions==transitions);
    pass("a fresh ordinary strike can outrank a weaker reaction, and a weaker unit's own tactic immediately interrupts reactions and finishes");

    engage(); set_pose(proxies[1],POSE_BATTLE_IDLE,1,true);
    proxies[1].opponent_relation=1;
    proxies[1].guard_pending=true;proxies[1].guard_pending_ms=fl_test_now_ms;proxies[1].guard_relation=1;
    update();assert(proxies[1].pose==POSE_GUARD);
    unsigned plays=proxies[1].guard_plays;
    for(unsigned i=0;i<20;++i) {
        proxies[1].guard_pending=true;proxies[1].guard_pending_ms=fl_test_now_ms;update();
    }
    assert(proxies[1].guard_plays==plays && proxies[1].clip_frame>10);
    pass("periodic battle pauses remain real guard opportunities; engagement alone neither forces NORMAL forever nor restarts reactions");

    engage();seed_result(.55f,false);damaging_tactic();tactic(2);update();
    assert(proxies[0].pose==POSE_STRONG && proxies[1].pose==POSE_STRONG);
    assert(combat_sound(0,0xdd622565) && tactic_hit_audio==1 && !tactic_block_audio);
    update();assert(proxies[1].pose==POSE_STRONG);
    assert(!combat_sound(0,0xdd622565) && tactic_hit_audio==1);
    pass("a tactic victim performing its own strong attack still receives exactly one hit sound without losing its strong pose");

    engage();seed_result(.55f,true);damaging_tactic();tactic(2);update();
    assert(combat_sound(0,0xdd622565)); update();
    assert(tactic_block_audio==1 && !tactic_hit_audio && !proxies[1].hit_pending && proxies[1].pose==POSE_STRONG);
    pass("an immutable blocked tactic produces loud block audio even if STRONG wins; it cannot become HIT through arbitration");

    for(bool block:{false,true}) {
        engage();seed_result(.55f,block);damaging_tactic();
        fl_test_now_ms+=500;FL_HookTacticDamage(app,300,armies[1],0x34);
        assert(damage_calls==2 && proxies[1].reaction_waiting && !proxies[1].hit_pending);
        update();assert(proxies[1].pose!=POSE_HIT && !tactic_hit_audio && !tactic_block_audio);
        assert(combat_sound(0,0xdd622565));
        assert(tactic_block_audio==unsigned(block) && tactic_hit_audio==unsigned(!block));
    }
    pass("slow native damage-number notifications cannot fabricate an early HIT or turn a pending block into a hit");

    engage(3);mock_scores[0][0]=mock_scores[0][1]=200;
    seed_result(.25f,false);damaging_tactic(0,1);
    seed_result(.55f,true);normal_event(2,1);update();
    assert(combat_sound(0,0xdd622565));update();
    assert(proxies[1].pose==POSE_HIT && tactic_hit_audio==1);
    pass("a different normal guard cannot erase an already decided unblocked tactic from a stronger source");

    for(unsigned body=0;body<25;++body) {
        fixture(1);override_appearance(0,int(body),-1,2,int(body%6));settle();
        for(Pose pose:{POSE_IDLE,POSE_MOVE,POSE_ATTACK,POSE_STRONG,POSE_HIT,POSE_GUARD,POSE_BATTLE_IDLE}) {
            set_pose(proxies[0],pose,0,true);
            assert(at<int>(proxies[0].duel,0xf8)==(pose==POSE_ATTACK||pose==POSE_BATTLE_IDLE||pose==POSE_STRONG||pose==POSE_HIT||pose==POSE_GUARD));
        }
        assert(proxies[0].body==int(body) && applied_color[0][1]==2 && constructions==1);
    }
    pass("all 175 body-pose combinations use the original corresponding weapon hand and preserve original appearance resources");

    engage();private_presentation_depth=1;private_sound_proxy=&proxies[0];
    for(Pose pose:{POSE_IDLE,POSE_BATTLE_IDLE,POSE_ATTACK,POSE_GUARD,POSE_HIT,POSE_STRONG}) {
        proxies[0].pose=pose;proxies[0].running=false;
        assert(!test_effect(0x7363ee3a));
    }
    assert(motion_effect_calls==0 && suppressed_stationary_dust==6);
    proxies[0].pose=POSE_MOVE;proxies[0].running=true;assert(test_effect(0x7363ee3a)==callback);
    proxies[0].running=false;proxies[0].pose=POSE_ATTACK;assert(!test_effect(0x78bdc0d7));
    proxies[0].pose=POSE_STRONG;assert(test_effect(0x78bdc0d7)==callback);
    proxies[0].pose=POSE_HIT;assert(test_effect(0xca81df19)==callback);
    private_presentation_depth=0;private_sound_proxy=NULL;
    assert(test_effect(0x7363ee3a)==callback);
    pass("native hoof dust requires actual movement, normal sweep effects are hidden, and original duel effects pass through");

    engage();mock_field_emit=true;
    FL_HookArmyUpdate(field[0],engine,app);
    assert(army_update_calls==1 && motion_effect_calls==0 && suppressed_stationary_field_effects==1 && !field_effect_scope.proxy);
    mock_field_move=true;FL_HookArmyUpdate(field[0],engine,app);
    assert(army_update_calls==2 && motion_effect_calls==1);
    mock_field_move=false; update(); draw_frame();proxies[0].pose=POSE_STRONG;
    FL_HookArmyUpdate(field[0],engine,app);assert(motion_effect_calls==2);
    proxies[0].pose=POSE_ATTACK;proxies[0].submitted=false;
    FL_HookArmyUpdate(field[0],engine,app);assert(motion_effect_calls==3);
    assert(test_effect()==callback && motion_effect_calls==4);
    pass("hidden original troop effects are filtered only inside a current replacement actor; movement, tactics, fallback and external effects forward");

    engage();mock_field_emit=true;proxies[0].source.id=1234;
    FL_HookArmyUpdate(field[0],engine,app);assert(motion_effect_calls==1);
    engage();mock_field_emit=true;stop_requested=1;
    FL_HookArmyUpdate(field[0],engine,app);assert(motion_effect_calls==1 && !field_effect_scope.proxy);
    pass("field effect scopes reject identity changes and Stop, preserve the original call and leave no dangling TLS owner");

    engage();root(FL_SOUND_MANAGER,sound_manager_memory);
    void *ctrl=sound_manager_memory+0x2a0;at<int>(ctrl,0x118)=1;
    for(int id:{0x9e,0x9f,0xa0}) {
        ambient_source(id); assert(FL_HookAmbientStart(ctrl,id,11,22,33)==7 && ambient_percent[7]==40);
    }
    unsigned calls=ambient_volume_calls;
    for(int id:{0x99,0xa5,0xab,0x10}) assert(FL_HookAmbientStart(ctrl,id,11,22,33)==7 && ambient_percent[7]==100);
    assert(ambient_volume_calls==calls && ambient_start_calls==7);
    ambient_start_slot=-1;assert(FL_HookAmbientStart(ctrl,0x9e,11,22,33)==-1 && ambient_volume_calls==calls);
    pass("only native troop combat ambience starts at 40 percent; movement, water, unrelated sounds and failed starts retain native behavior");

    ambient_start_slot=7;ambient_source(0x9e);FL_HookAmbientStart(ctrl,0x9e,11,22,33);
    for(unsigned n=0;n<5;++n) { assert(FL_HookAmbientVolume(ctrl,7,60)==1); assert(ambient_percent[7]==24); }
    lock_status();sync_combat_ambient(false);unlock_status();assert(ambient_percent[7]==24);
    stop_requested=1;lock_status();sync_combat_ambient(true);unlock_status();assert(ambient_percent[7]==60);
    assert(FL_HookAmbientVolume(ctrl,7,70)==1 && ambient_percent[7]==70);
    pass("native volume changes are multiplied once, do not compound across frames, and Stop restores the latest original level");

    engage();root(FL_SOUND_MANAGER,sound_manager_memory);ctrl=sound_manager_memory+0x2a0;at<int>(ctrl,0x118)=1;
    ambient_source(0x9f);FL_HookAmbientStart(ctrl,0x9f,11,22,33);calls=ambient_volume_calls;
    at<int>(ctrl,0x228+7*4)=9001;at<int>(ctrl,8+7*4)=0xab;
    lock_status();sync_combat_ambient(true);unlock_status();assert(ambient_volume_calls==calls);
    root(FL_SOUND_MANAGER,NULL);lock_status();sync_combat_ambient(true);unlock_status();
    assert(!ambient_level_owner && ambient_volume_calls==calls);
    assert(FL_HookAmbientVolume(ctrl,-1,20)==0 && last_ambient_percent==20);
    pass("ambience restoration cannot change recycled noncombat voices or dereference a destroyed sound manager");

    unsigned char before[8]={},after[8]={};
    for(uint64_t base_address:{0x140000000ull,0x7ff600000000ull}) {
        FLPatchSpec pointer={FL_ARMY_UPDATE_VTABLE,FL_ARMY_UPDATE};
        assert(fl_patch_width(pointer)==8 && fl_encode_patch(pointer,base_address,0x123456789abcull,before,after));
        uint64_t b=0,a=0;memcpy(&b,before,8);memcpy(&a,after,8);
        assert(b==base_address+FL_ARMY_UPDATE && a==0x123456789abcull);
        const auto &call=fl_patch_specs[0];
        assert(fl_patch_width(call)==5 && fl_encode_patch(call,base_address,base_address+0x10000,before,after));
        int32_t old_rel=0,new_rel=0;memcpy(&old_rel,before+1,4);memcpy(&new_rel,after+1,4);
        assert(before[0]==0xe8 && after[0]==0xe8 && base_address+call.site+5+old_rel==base_address+call.original);
        assert(base_address+call.site+5+new_rel==base_address+0x10000);
        assert(!fl_encode_patch(call,base_address,0xf000000000000000ull,before,after));
    }
    pass("graphics vtable patches use complete relocated 64-bit pointers; E8 patches preserve call targets and reject unreachable stubs");
}
