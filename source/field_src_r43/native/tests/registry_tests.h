#pragma once
static unsigned registry_cleanup_calls;
static void *registry_cleanup_mock(void *p,unsigned flags) {
    assert(!state_lock.depth&&flags==1);++registry_cleanup_calls;return info_destroy(p,flags);
}
static void registry_map(unsigned slot,int model) {
    unsigned id=at<uint16_t>(people[slot],0x10);
    FL_Registry.snapshot.active=1;
    for(auto &v:FL_Registry.snapshot.allowed)v=1;
    auto &r=FL_Registry.snapshot.rows[id];r.person=(uintptr_t)people[slot];r.model=model;
    memcpy(r.identity,people[slot]+0x12,28);FL_Registry.sequence+=2;
}
static void run_registry_tests() {
    for(int body=0;body<126;++body) {
        fixture(1);registry_map(0,body);settle();
        assert(proxies[0].ready&&proxies[0].body==body&&!blocked);
        bool common=(body>=11&&body<=16)||(body>=22&&body<=24);
        assert(proxies[0].color==(common?0:-1));
        assert(private_actor_vtable[1+0x20/8]==base+FL_ACTOR_UPDATE);
        assert(private_actor_vtable[1+0x48/8]==base+FL_ACTOR_CREATE);
        for(Pose pose:{POSE_ATTACK,POSE_STRONG,POSE_HIT,POSE_GUARD}) {
            set_pose(proxies[0],pose,0,true);at<float>(settings[0][0],0x24)=1000;
            observe_clip(proxies[0]);assert(proxies[0].pose_done);
        }
    }
    pass("R24 all 25 original IDs and 101 catalog-approved extended IDs create without calling real-duel wrappers");
    fixture(1);settle();unsigned char *old=proxies[0].actor;registry_map(0,125);
    fl_test_now_ms+=2100;color_hold_mask=2;update();
    assert(proxies[0].actor==old&&proxies[0].refresh_loading&&!blocked);
    color_hold_mask=0;update();assert(proxies[0].body==125&&proxies[0].actor!=old&&proxies[0].ready);
    FL_Registry.snapshot={};FL_Registry.sequence+=2;fl_test_now_ms+=2100;update();update();
    assert(proxies[0].body==select_body(proxies[0].duel)&&!blocked);
    pass("R24 live registration swaps after readiness and clearing the mapping restores the current native appearance");
    fixture(1);registry_map(0,2);FL_Registry.snapshot.rows[at<uint16_t>(people[0],0x10)].person=123;settle();
    assert(proxies[0].body==select_body(proxies[0].duel));
    registry_map(0,2);people[0][0x12]=7;fl_test_now_ms+=2100;update();assert(proxies[0].body!=2);
    pass("R24 reused officer addresses and renamed officer identities cannot inherit a stale saved mapping");
    fixture(2);FL_Registry.sequence=1;update();assert(!blocked&&!proxies[0].constructed);
    FL_Registry.sequence=2;fl_test_now_ms+=2100;settle();assert(proxies[0].ready&&proxies[1].ready);
    registry_map(0,125);FL_Registry.snapshot.allowed[125]=0;fl_test_now_ms+=2100;update();assert(proxies[0].body!=125&&!blocked);
    pass("R24 incomplete mailbox writes defer selection and unavailable catalog IDs cannot disable other units");
    fixture(1);registry_map(0,25);never_ready_id=at<uint16_t>(people[0],0x10);update();
    FL_Status.frame+=3601;update();assert(!blocked&&!proxies[0].constructed);
    pass("R24 unavailable extended resources time out locally without stopping all field models");
    fixture(1);at<unsigned>(field[0],0x740)=2;registry_map(0,125);update();assert(!proxies[0].constructed&&FL_Status.target_count==0);
    pass("R24 registry mappings do not convert siege units");
    fixture(1);unsigned char layout[0x270]={};
    registry_cleanup_calls=0;info_destroy_next=(uintptr_t)&registry_cleanup_mock;
    assert(FL_HookInfoDestroy(layout,1)==layout&&registry_cleanup_calls==1);
    info_destroy_next=0;assert(FL_HookInfoDestroy(layout,1)==layout&&registry_cleanup_calls==1);
    pass("R24 chained integrated UI cleanup runs once with original arguments and native destruction still works without the mod");
}
