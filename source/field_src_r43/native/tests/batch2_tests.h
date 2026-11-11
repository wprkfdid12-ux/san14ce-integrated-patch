#pragma once
static void fresh_batch2(unsigned count=2) {
    fixture(count); settle();
    field_effect_calls=field_resets=pair_resets=defeat_commits=army_detaches=0;
    mock_child_visible=false;
}
static void defeat_test(unsigned slot,bool captured) {
    at<uint8_t>(people[slot],0xe6)=captured?6:2;
    FL_HookDefeatCommit(data,armies[slot]);
    at<uint8_t>(armies[slot],0x10)=0;
}
static void run_batch2_tests() {
    fresh_batch2();
    for(int id:{2,0x48,0x61}) {
        assert(!FL_HookGroundEffect(manager,id,app,field[0],17));
        proxies[0].running=true;
        assert(!FL_HookGroundEffect(manager,id,app,field[0],17));
    }
    assert(!field_effect_calls && native_dust_removed==6);
    assert(FL_HookGroundEffect(manager,0x53,app,field[0],17)==callback);
    assert(last_field_id==0x53 && last_field_owner==field[0] && last_field_transform==app && last_field_flags==17);
    assert(FL_HookGroundEffect(manager,2,app,field[2],17)==callback);
    pass("R22 removes all hidden troop dust variants but preserves status effects and unrelated native actors with unchanged arguments");

    fresh_batch2(); alignas(16) unsigned char dust[0xa0]={},fire[0xa0]={};
    at<int>(dust,0x1c)=2;at<int>(fire,0x1c)=0x53;
    at<void *>(field[0],0x890)=dust;at<void *>(field[0],0x898)=fire;
    FL_HookArmyUpdate(field[0],engine,app);
    assert(field_resets==1 && !at<void *>(field[0],0x890) && at<void *>(field[0],0x898)==fire && at<int>(fire,0x1c)==0x53);
    pass("R22 stops already-running stationary troop dust without stopping fire/status particles");

    fresh_batch2(); unsigned char descriptor[0x20]={},pair[0x40]={};
    at<void *>(pair,0)=descriptor;at<int>(descriptor,4)=0x1b;at<uint16_t>(descriptor,0)=1;
    for(int id:{1,0x51}) assert(!FL_HookNormalFieldEffect(manager,id,app,pair,23));
    assert(!field_effect_calls);
    at<uint16_t>(descriptor,0)=500;
    assert(FL_HookNormalFieldEffect(manager,1,app,pair,23)==callback);
    assert(last_field_owner==pair && last_field_flags==23);
    private_presentation_depth=1;private_sound_proxy=&proxies[0];proxies[0].pose=POSE_STRONG;
    assert(test_effect(0x78bdc0d7)==callback);
    private_sound_proxy=NULL;private_presentation_depth=0;
    pass("R22 ordinary field arcs are omitted independently of strong-attack VFX; original duel and nonreplacement calls still pass through");

    fresh_batch2(); unsigned char marker[16]={},node[16]={},pool[16]={},arc[0xa0]={};
    at<void *>(pool,0)=node;at<void *>(node,0)=pair;at<void *>(pair,0x28)=arc;at<int>(arc,0x1c)=1;
    root(0x27d10e8,pool);root(0x27d10f0,pool);at<unsigned>(image.data(),0x27d1120)=1;
    at<void *>(field[0],0x8d0)=marker;proxies[0].native_effects_trimmed=false;
    update();assert(pair_resets==1 && !at<void *>(pair,0x28));
    update();assert(pair_resets==1);
    at<void *>(field[0],0x8d0)=NULL;root(0x27d10e8,NULL);root(0x27d10f0,NULL);
    pass("R22 retires arcs created before attachment once, using their original owner setter and without recreating combat events");

    fresh_batch2(); unsigned char layout[0x270]={},child[0x200]={};InfoPoint out;
    at<uintptr_t>(layout,0)=base+FL_INFO_VTABLE;at<void *>(layout,0x1f0)=armies[0];
    at<int>(layout,0x28)=100;at<int>(layout,0x2c)=80;
    const uint64_t point=uint64_t(600)<<32|400;
    auto before=std::vector<unsigned char>(layout,layout+sizeof layout);
    assert(FL_HookInfoAnchor(layout,&out,point)==&out && out.x==350 && out.y==552);
    assert(!memcmp(before.data(),layout,sizeof layout));
    at<void *>(layout,0x258)=child;at<int>(child,0x28)=120;at<int>(child,0x2c)=100;mock_child_visible=true;
    FL_HookInfoAnchor(layout,&out,point);assert(out.y==542 && out.x==350);
    at<void *>(layout,0x1f0)=armies[2];FL_HookInfoAnchor(layout,&out,point);assert(out.y==500);
    pass("R22 centres the whole native info block at the same unit foot anchor, retains X, respects expanded size, and never moves unrelated labels");

    fresh_batch2();unsigned char army_before[sizeof armies[0]],person_before[sizeof people[0]];
    at<uint8_t>(people[0],0xe6)=6;
    memcpy(army_before,armies[0],sizeof army_before);memcpy(person_before,people[0],sizeof person_before);
    FL_HookDefeatCommit(data,armies[0]);
    assert(defeat_commits==1 && last_defeat_data==data && last_defeat_army==armies[0] && proxies[0].captured);
    assert(!memcmp(army_before,armies[0],sizeof army_before) && !memcmp(person_before,people[0],sizeof person_before));
    FL_HookDefeatCommit(data,armies[0]);assert(defeat_commits==2 && proxies[0].defeated);
    pass("R22 observes accepted capture without writing gameplay/people and forwards each original deletion-queue call exactly once");

    for(unsigned state=0;state<=9;++state) {
        fresh_batch2(1);at<uint8_t>(people[0],0xe6)=(unsigned char)state;
        FL_HookDefeatCommit(data,armies[0]);assert(proxies[0].captured==(state==6));
    }
    pass("R22 only native person status6 selects the captured branch; other statuses never invent a capture");

    fresh_batch2();defeat_test(0,false);update();draw_frame();
    assert(proxies[0].constructed && !proxies[0].captured && proxies[0].pose==POSE_IDLE && at<int>(proxies[0].duel,0xfc)==1);
    native_land(0,0);fl_test_now_ms+=301;update();draw_frame();
    assert(proxies[0].terminal_done && proxies[0].constructed);native_land(0,0);
    blocked=true;draw_frame();native_land(0,0);
    pass("R22 uncaptured defeat disappears mounted and cannot flash original troops after completion or a later presentation failure");

    for(bool weapon_in_attack_hand:{false,true}) {
        fresh_batch2();Proxy &p=proxies[0];
        at<int>(p.duel,0xf8)=weapon_in_attack_hand;
        defeat_test(0,true);update();draw_frame();
        assert(p.pose==POSE_FALL && at<int>(p.duel,0x28)==(weapon_in_attack_hand?46:47) && !at<int>(p.duel,0xfc));
        assert(!at<void *>(p.duel,0xb0) && !p.running);native_land(0,0);
        FL_HookArmyDetach(field[0]);at<void *>(manager,0x18)=field[1];
        p.source.army=(void *)0x10;p.source.person=(void *)0x20; // departed pointers must never be read
        for(unsigned f=0;f<117;++f) {update();draw_frame();assert(p.constructed && !p.terminal_done);}
        update();draw_frame();assert(p.terminal_done);
        update();assert(!p.constructed && army_detaches==1);
    }
    pass("R22 both original 120-frame fall variants finish after source teardown without dereferencing departed game objects");

    fresh_batch2();at<uint8_t>(armies[0],0x10)=0;update();assert(!proxies[0].constructed && !defeat_commits);
    fresh_batch2();at<unsigned>(field[0],0x740)=1;update();assert(!proxies[0].constructed && !defeat_commits);
    pass("R22 returning and water-transition units are not confused with accepted annihilation/capture");

    fresh_batch2();defeat_test(0,true);update();
    FL_HookArmyDetach(field[0]);at<void *>(field[0],0x40)=armies[2];set_id(2,999);
    at<uint8_t>(armies[2],0x10)=1;update();draw_frame();
    assert(proxies[0].source_detached && proxies[0].defeated);
    int replacement=index_find(proxy_index,(uintptr_t)field[0]);
    assert(replacement>=0 && replacement!=0 && proxies[replacement].source.id==999);
    pass("R22 a recycled original actor receives a new replacement while the old captured visual finishes independently");

    fresh_batch2(1);Proxy &p=proxies[0];unsigned char *old_actor=p.actor;
    override_appearance(0,12,2,1,3);color_hold_mask=3;fl_test_now_ms+=2100;
    update();draw_frame();assert(p.refresh_loading && p.actor==old_actor && p.ready);native_land(0,0);
    assert(at<void *>(old_actor,0x68)==handles[0][0] && at<void *>(p.actor_storage[1],0x68)==alt_handles[0][0]);
    settle(5);assert(p.actor==old_actor && p.refresh_loading);native_land(0,0);
    color_hold_mask=0;update();draw_frame();
    assert(!p.refresh_loading && p.active_bank==1 && p.body==12 && p.color==2 && p.horse_color==1 && p.weapon==3);
    assert(p.actor==p.actor_storage[1] && at<void *>(p.actor,0x40)==p.duel);native_land(0,0);
    override_appearance(0,22,4,2,1);fl_test_now_ms+=2100;update();draw_frame();
    assert(p.active_bank==0 && p.body==22 && p.color==4 && p.horse_color==2 && p.weapon==1 && appearance_refreshes==2);
    native_land(0,0);
    pass("R22 live body, parts, weapon and horse colors swap only after the new resources are ready; two bank swaps keep native addresses stable and soldiers hidden");

    fresh_batch2(1);override_appearance(0,12,2,1,3);color_hold_mask=3;fl_test_now_ms+=2100;update();
    assert(proxies[0].refresh_loading);unsigned old_removals=removals;
    FL_HookManagerRelease(manager,engine);assert(removals==old_removals+2 && !proxies[0].constructed);
    pass("R22 loading appearance refreshes release both resource banks exactly once on save reload/owner cleanup");

    fixture(500);settle(260);unsigned before_cost=optimized_attachments,before_updates=unsigned(std::count(events.begin(),events.end(),14));
    unsigned old_body_calls=body_attachment_calls,old_weapon_calls=weapon_attachment_calls;
    update();draw_frame();
    assert(body_attachment_calls-old_body_calls==1000 && weapon_attachment_calls-old_weapon_calls==500);
    assert(optimized_attachments-before_cost==500 && unsigned(std::count(events.begin(),events.end(),14))-before_updates==500);
    for(unsigned i=0;i<500;++i)assert(proxies[i].ready && at<uint32_t>(callback,0x1c)>=1500);
    pass("R22 500 units retain full native animation/update/submission rate while eliminating one duplicate hand attachment per unit per frame");
}
