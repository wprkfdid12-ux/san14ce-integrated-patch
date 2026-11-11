#pragma once
static void finish_body(float last) {
    if(proxies[0].pose==POSE_BATTLE_IDLE) fl_test_now_ms+=751;
    at<float>(settings[0][0],0x24)=last-2; update();
    assert(proxies[0].pose_done); update();
}
static bool same_angle(float a,float b) { return fabsf(remainderf(a-b,6.28318530718f))<0.0001f; }
static void run_combat_r19_tests() {
    fixture(1); settle(); at<uint8_t>(armies[0],0x26)=1; update();
    assert(proxies[0].pose==POSE_ATTACK && proxies[0].combat_cycles==1);
    float horse_frame=at<float>(settings[0][1],0x24);
    finish_body(60);
    assert(proxies[0].pose==POSE_BATTLE_IDLE && at<int>(proxies[0].duel,0x28)==13);
    assert(at<int>(proxies[0].duel,0x2c)==0 && motion_override_seen[0][1]==0);
    finish_body(45);
    assert(proxies[0].pose==POSE_ATTACK && proxies[0].combat_cycles==2);
    assert(motion_override_seen[0][1]==0 && at<float>(settings[0][1],0x24)>horse_frame);
    pass("engaged idle units alternate original attack and short original idle without restarting the standing horse clock");

    at<uint8_t>(armies[0],0x26)=0; update(); assert(proxies[0].pose==POSE_IDLE);
    at<uint8_t>(armies[0],0x26)=1; at<uint8_t>(armies[0],0x23)=1;
    settle(210); assert(proxies[0].pose==POSE_IDLE && proxies[0].combat_cycles==2);
    at<uint8_t>(armies[0],0x23)=0; walk(); assert(proxies[0].pose==POSE_MOVE);
    fixture(2); settle(210); assert(proxies[0].combat_cycles==0 && proxies[1].combat_cycles==0);
    pass("peaceful nearby units never swing; native engagement cancellation and actual route movement end cosmetic battle cycles");

    at<float>(field[0],0x744)=0; at<float>(field[1],0x744)=3.14159265359f;
    at<int>(field[0],0x60)=at<int>(field[1],0x60)=2;
    std::vector<unsigned char> native_before((unsigned char *)field,(unsigned char *)field+sizeof field);
    update();
    assert(same_angle(at<float>(proxies[0].duel,0xf0),0));
    assert(same_angle(at<float>(proxies[1].duel,0xf0),3.14159265359f));
    fl_test_now_ms+=100;update();
    assert(same_angle(at<float>(proxies[0].duel,0xf0),-1.57079632679f));
    assert(same_angle(at<float>(proxies[1].duel,0xf0),1.57079632679f));
    assert(!memcmp(native_before.data(),field,sizeof field));
    for(unsigned i=0;i<2;++i) for(unsigned axis=0;axis<3;++axis)
        assert(at<float>(proxies[i].duel,0xc0+axis*4)==at<float>(field[i],0x10+axis*4));
    pass("opposing field bearings become opposing private quarter-turn battle bearings without moving or rotating gameplay actors");

    tactic(); update();
    assert(proxies[0].pose==POSE_STRONG && at<int>(proxies[0].duel,0x2c)==1);
    assert(same_angle(at<float>(proxies[0].duel,0xf0),at<float>(field[0],0x744)));
    assert(proxies[1].pose==POSE_ATTACK && at<int>(proxies[1].duel,0x2c)==0);
    finish_body(180);
    assert(proxies[0].pose==POSE_ATTACK && at<int>(proxies[0].duel,0x2c)==0);
    assert(same_angle(at<float>(proxies[0].duel,0xf0),-1.57079632679f));
    update(); assert(motion_override_seen[0][1]==0);
    pass("a tactic alone switches to forward-facing original strong attack and gallop, then restores standing sideways combat at native completion");

    fixture(1); settle(); walk(); tactic(); walk(); assert(proxies[0].pose==POSE_STRONG);
    at<float>(settings[0][0],0x24)=178; walk(); assert(proxies[0].pose_done);
    walk(); assert(proxies[0].pose==POSE_MOVE && at<int>(proxies[0].duel,0x2c)==1);
    assert(motion_override_seen[0][1]==0);
    at<int>(field[0],0x60)=0; update(); assert(proxies[0].pose==POSE_IDLE && at<int>(proxies[0].duel,0x2c)==0);
    pass("a finished tactic resumes genuine route movement with continuous horse time and stops when actual movement ends");

    at<uint8_t>(armies[0],0x26)=1; update(); assert(proxies[0].pose==POSE_ATTACK);
    at<int>(manager,0x498)=1; update(); at<int>(manager,0x498)=0;
    at<uint8_t>(armies[0],0x26)=0; update();
    fl_test_now_ms+=150;update();
    assert(proxies[0].pose==POSE_IDLE && same_angle(at<float>(proxies[0].duel,0xf0),at<float>(field[0],0x744)));
    FL_HookManagerRelease(manager,engine); settle(); assert(!proxies[0].combat_cycles);
    pass("view changes and observed reloads discard stale battle cycles and restore current native facing");

    for(int body=0;body<25;++body) {
        fixture(1); override_appearance(0,body,-1,2,body%6); settle();
        at<int>(field[0],0x60)=2; update(); finish_body(60); finish_body(45);
        assert(proxies[0].pose==POSE_ATTACK && constructions==1 && color_requests==2);
        assert(applied_color[0][0]==-1 && applied_color[0][1]==2 && at<int>(proxies[0].duel,0x2c)==0);
    }
    pass("all 25 body classes retain the same original appearance and horse color through periodic standing battle transitions");

    fixture(500); settle(251);
    for(unsigned i=0;i<500;++i) assert(bound_army(armies[i])==&proxies[i]);
    assert(!bound_army(armies[500]));
    pass("the indexed army lookup finds every one of 500 live bindings and rejects an unbound army");
    events.clear(); unsigned before=submissions,models_before=at<uint32_t>(callback,0x1c);
    for(unsigned i=0;i<500;++i) at<uint8_t>(armies[i],0x26)=1;
    update(); draw_frame();
    assert(std::count(events.begin(),events.end(),14)==500 && submissions-before==500);
    assert(at<uint32_t>(callback,0x1c)-models_before==1500);
    for(unsigned i=0;i<500;++i) assert(proxies[i].pose==POSE_ATTACK && proxies[i].ready && at<int>(proxies[i].duel,0x2c)==0);
    pass("500 engaged units still receive one full native actor update and three native model submissions each per frame");

    at<uint16_t>(armies[0],0x12)=500; assert(!bound_army(armies[0]));
    at<uint16_t>(armies[0],0x12)=596; assert(bound_army(armies[0]));
    at<void *>(data,0x158+596*8)=people[500]; assert(!bound_army(armies[0]));
    at<void *>(data,0x158+596*8)=people[0]; at<uint8_t>(armies[0],0x10)=0; assert(!bound_army(armies[0]));
    FL_HookManagerRelease(manager,engine); assert(index_find(army_index,(uintptr_t)armies[499])<0);
    pass("indexed notifications retain officer, person, active-state and generation checks instead of accepting recycled army slots");

    fixture(0); log_file=(HANDLE)1;
    for(unsigned n=0;n<100;++n)log_line("SAMPLE n=%u",n);
    assert(log_write_count==0 && log_flush_count==0 && pending_log_bytes>0);
    write_pending_log(); assert(log_write_count==1 && !pending_log_bytes && log_bytes>0);
    unsigned first_bytes=log_bytes;
    for(const char *tag:{"CREATE_BEGIN","RELEASE_BEGIN","RELEASE_END","BLOCKED","CONFIGURED"}) log_line("%s",tag);
    assert(log_write_count==1 && !log_flush_count);
    write_pending_log(); assert(log_write_count==2 && log_bytes>first_bytes && !pending_log_bytes);
    log_file=INVALID_HANDLE_VALUE;
    pass("diagnostic bursts batch OS-cached writes without synchronous disk flushes or lost records");

    fixture(1); settle(300);
    assert(perf_update.count==300 && perf_draw.count==300 && perf_update.total>0 && perf_draw.total>0);
    assert(perf_update.maximum<=perf_update.total && perf_draw.maximum<=perf_draw.total);
    pass("runtime cost samples cover private update and draw work without claiming GPU timing or changing animation frequency");
}
