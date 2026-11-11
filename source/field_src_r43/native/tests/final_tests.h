#pragma once
static void run_final_tests() {
    engage();normal_event(0,1);mock_strategy_pause=true;update();
    at<uint16_t>(armies[1],0x16)=0;update();
    assert(proxies[0].pose==POSE_IDLE && !live_combat_target(proxies[0]));
    unsigned cycles=proxies[0].combat_cycles;for(unsigned i=0;i<100;++i)update();
    assert(proxies[0].combat_cycles==cycles);
    pass("R23 zero-troop target cannot cause either active or paused air swings even with stale native engagement");
    at<uint16_t>(armies[1],0x16)=100;update();assert(live_combat_target(proxies[0]));
    at<uint16_t>(battle_pairs[0],8)=3;at<uint8_t>(armies[1],0x10)=0;update();
    assert(live_combat_target(proxies[0]) && proxies[0].pose==POSE_ATTACK);
    at<int>(battle_pairs[0],0xc)=99;proxies[0].opponent_serial=0;update();assert(proxies[0].pose==POSE_IDLE);
    pass("R23 another native battle-pair target keeps combat alive; invalid pair kinds cannot invent an opponent");

    fixture(1);settle();Proxy &p=proxies[0];
    alignas(16) LocalBonePose bones[2]={};unsigned char instance[32]={},skeleton[16]={},descriptor[16]={};
    at<void *>(models[0][0],0x150)=instance;at<void *>(instance,0)=skeleton;at<void *>(skeleton,0)=descriptor;
    at<int16_t>(descriptor,4)=2;at<LocalBonePose *>(instance,0x10)=bones;
    for(unsigned i=0;i<2;++i){bones[i].parent=int(i)-1;bones[i].rotation[3]=1;bones[i].flags=0xaabb1234;for(float &v:bones[i].scale)v=1;}
    set_pose(p,POSE_ATTACK,0,true);bones[1].position[0]=20;presentation_ms=1000;
    set_pose(p,POSE_BATTLE_IDLE,0);assert(p.blend_count==2 && at<int>(p.duel,0xf8)==1);
    bones[1].position[0]=0;presentation_ms=1090;blend_attack_recovery(p,models[0][0]);
    assert(fabsf(bones[1].position[0]-10)<0.00001f && bones[1].flags==0xaabb1234 && bones[1].parent==0);
    for(auto &bone:bones){float norm=0;for(float v:bone.rotation)norm+=v*v;assert(fabsf(norm-1)<0.00001f);}
    bones[1].parent=99;auto copy=bones[0];blend_attack_recovery(p,models[0][0]);assert(!p.blend_count&&!memcmp(&copy,&bones[0],sizeof copy));
    at<void *>(models[0][0],0x150)=NULL;
    pass("R23 attack recovery blends only the two original private poses, keeps weapon hand and normalizes rotation; mismatched bones produce no partial write");

    fixture(1);settle();Proxy &fall=proxies[0];fall.defeated=fall.captured=true;set_pose(fall,POSE_FALL,0,true);
    at<void *>(models[0][0],0x1c0)=NULL;bone_available=true;fall.source.x=10;fall.source.z=20;
    for(float distance:{0.0f,800.0f,2400.0f,4000.0f,10000.0f}){
        bone_position[0]=10+distance;bone_position[1]=345;bone_position[2]=20;
        float origin[3]={10,234,20};set_position(models[0][0],origin);
        float scale=at<float>(models[0][0],0x34);shorten_captured_fall(fall,models[0][0]);
        float result=distance+at<float>(models[0][0],0x18)-10;
        assert(result>=-0.001 && result<=810.61f && at<float>(models[0][0],0x1c)==234 && at<float>(models[0][0],0x34)==scale);
    }
    unsigned before=fall_distance_corrections;fall.captured=false;shorten_captured_fall(fall,models[0][0]);assert(before==fall_distance_corrections);
    pass("R23 captured rider translation is quartered and capped at one native hex; height and scale stay original and uncaptured defeat is unchanged");

    fixture(1);settle();unsigned char root_layout[0x280]={},normal[0x270]={},bg[0x120]={},text[0x120]={},bg_t[0x40]={},text_t[0x40]={};void *entries[0x40]={};
    at<uintptr_t>(root_layout,0)=base+FL_INFO_VTABLE;at<void *>(root_layout,0x1f0)=armies[0];at<void *>(root_layout,0x260)=normal;
    at<uintptr_t>(normal,0)=base+0x1398150;at<void *>(normal,0x138)=entries;at<int>(normal,0x140)=0x40;entries[0x31]=bg;entries[0x32]=text;
    at<void *>(bg,0xe8)=bg_t;at<void *>(text,0xe8)=text_t;
    ui_size(bg,120,40);ui_position(bg,0,0);ui_size(text,90,28);ui_position(text,15,6);
    auto font_before=std::vector<unsigned char>(text,text+sizeof text);
    compact_info_padding(root_layout,true);assert(at<int16_t>(bg,0xf0)==94 && at<int16_t>(bg,0xf2)==30);
    assert(at<float>(bg_t,0x30)+47==at<float>(text_t,0x30)+45 && at<float>(bg_t,0x34)+15==at<float>(text_t,0x34)+14);
    assert(!memcmp(text,font_before.data(),sizeof text));unsigned queries=ui_queries,writes=ui_size_writes;
    for(unsigned i=0;i<500;++i)compact_info_padding(root_layout,true);
    assert(ui_queries==queries && ui_size_writes==writes);
    compact_info_padding(root_layout,false);assert(at<int16_t>(bg,0xf0)==120 && at<int16_t>(bg,0xf2)==40);
    assert(!info_padding_entry(root_layout,false));
    compact_info_padding(root_layout,true);FL_HookInfoDestroy(root_layout,1);assert(!info_padding_entry(root_layout,false));
    pass("R23 troop padding is symmetric in both axes, font object untouched, 500 unchanged updates query no native layout, and removal/destruction restores the original panel");

    at<int>(field[0],0x740)=2;update();assert(!bound_army(armies[0]) && info_modified_unit(root_layout));
    unsigned audio_before=native_sound_calls;uint32_t sound=0;assert(sound_call(true,&sound)&&native_sound_calls==audio_before+1);
    mock_field_emit=true;FL_HookArmyUpdate(field[0],engine,app);assert(motion_effect_calls==1);
    assert(FL_HookAmbientEnqueue(manager,armies[0],12,0xa5)==27);
    root(FL_SOUND_MANAGER,sound_manager_memory);void *ctrl=sound_manager_memory+0x2a0;at<int>(ctrl,0x118)=1;
    ambient_source(0x9e);FL_HookAmbientStart(ctrl,0x9e,11,22,33);assert(ambient_percent[7]==100);
    FL_HookAmbientVolume(ctrl,7,75);assert(ambient_percent[7]==75);
    at<void *>(root_layout,0x260)=NULL;
    pass("R23 siege retains original models, movement/combat sounds and effects while sharing badge positioning/padding");

    fixture(2);settle();at<int>(field[0],0x740)=2;
    assert(!bound_army(armies[0]));unsigned count=skill_notifications;tactic(1);assert(skill_notifications==count);
    update();draw_frame();assert(!proxies[0].constructed && original_visuals[0][6]==1 && proxies[1].ready);
    at<int>(field[0],0x740)=0;settle();assert(bound_army(armies[0])&&constructions==3);
    pass("R23 mode changes immediately exclude siege from notifications before the next update and restore a fresh mounted instance on land return");
}
