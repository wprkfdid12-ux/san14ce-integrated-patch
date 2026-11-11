#pragma once
static void tactic(unsigned index=1) {
    alignas(16) unsigned char value[0x310]={};
    at<int>(value,8)=0x1b; at<int>(value,0xc)=int(index);
    FL_HookTacticEffect(value,manager,app);
}
static void handoff(unsigned index=0,void *collector=app) {
    assert(FL_HookAmbientEnqueue(collector,armies[index],123,0x99)==1);
    FL_HookAmbientCleanup(collector);
}
static bool private_hoof(unsigned slot=0,bool spatial=true) {
    lock_status(); private_presentation_depth=1; private_sound_proxy=&proxies[slot];
    uint32_t id=0;
    bool ok=(spatial?FL_HookEventSound3D:FL_HookEventSound2D)
        (manager,app,engine,context,17,29,hoof_params,&id,callback_handle);
    private_sound_proxy=NULL; private_presentation_depth=0; unlock_status();
    return ok;
}
static void walk(unsigned count=1) {
    fl_test_now_ms+=16;
    for(unsigned i=0;i<count;++i) { at<int>(field[i],0x60)=1; at<float>(field[i],0x10)+=3; }
    update(); draw_frame();
}
static void run_batch1_tests() {
    fixture(1); settle(); at<int>(field[0],0x60)=2; update();
    assert(proxies[0].pose==POSE_ATTACK && at<int>(proxies[0].duel,0x28)==13 && at<int>(proxies[0].duel,0x2c)==0);
    unsigned transitions=proxies[0].motion_transitions;
    for(unsigned n=0;n<20;++n) update();
    assert(proxies[0].motion_transitions==transitions && motion_override_seen[0][0]==0);
    pass("native attack state selects the original normal duel clip without per-frame restarting");

    at<uint16_t>(armies[0],0x16)=10000; at<uint8_t>(armies[0],0x26)=1;
    for(unsigned n=0;n<30;++n) { at<uint16_t>(armies[0],0x16)-=50; update(); }
    assert(proxies[0].pose==POSE_ATTACK && !proxies[0].hit_events && !proxies[0].hit_pending);
    pass("ordinary combat losses never trigger a hit pose, as explicitly requested by the user");

    unsigned char army_before[sizeof armies[0]],actor_before[sizeof field[0]],person_before[sizeof people[0]];
    memcpy(army_before,armies[0],sizeof army_before); memcpy(actor_before,field[0],sizeof actor_before);
    memcpy(person_before,people[0],sizeof person_before);
    tactic(); assert(tactic_calls==1 && proxies[0].skill_pending); update();
    assert(proxies[0].pose==POSE_STRONG && at<int>(proxies[0].duel,0x28)==19);
    assert(!memcmp(army_before,armies[0],sizeof army_before) && !memcmp(actor_before,field[0],sizeof actor_before) &&
           !memcmp(person_before,people[0],sizeof person_before));
    pass("native tactic effect notification starts strong attack and leaves original unit, army and person bytes untouched");

    transitions=proxies[0].motion_transitions;
    FL_HookTacticDamage(app,500,armies[0],0x34); update();
    assert(damage_calls==1 && proxies[0].pose==POSE_STRONG && proxies[0].motion_transitions==transitions);
    for(unsigned n=0;n<190;++n) update();
    assert(proxies[0].pose==POSE_ATTACK && proxies[0].hit_events==0);
    pass("a tactic's strong attack finishes at the native clip end and is not interrupted by a stale hit reaction");

    for(int style:{0x2e,0x35}) { FL_HookTacticDamage(app,300,armies[0],style); update(); }
    FL_HookTacticDamage(app,0,armies[0],0x34); update();
    FL_HookTacticDamage(app,-10,armies[0],0x34); update();
    assert(!proxies[0].hit_events);
    pass("recovery, other number styles, zero damage and negative amounts do not produce tactic hit reactions");

    FL_HookTacticDamage(app,300,armies[0],0x34); update();
    assert(proxies[0].pose==POSE_HIT && at<int>(proxies[0].duel,0x28)==39 && proxies[0].hit_events==1);
    transitions=proxies[0].motion_transitions;
    for(unsigned n=0;n<20;++n) { FL_HookTacticDamage(app,100,armies[0],0x34); update(); }
    assert(proxies[0].motion_transitions==transitions && proxies[0].clip_frame>10);
    for(unsigned n=0;n<75;++n) update();
    assert(proxies[0].pose==POSE_ATTACK && proxies[0].hit_events==1);
    pass("only positive native attack-tactic damage plays the original hit clip once; repeated notifications cannot freeze it");

    for(int body=0;body<25;++body) {
        fixture(1); override_appearance(0,body,-1,-1,body%6); settle();
        tactic(); update(); assert(proxies[0].pose==POSE_STRONG);
        at<float>(settings[0][0],0x24)=178; update(); assert(proxies[0].pose_done);
        update(); assert(proxies[0].pose==POSE_IDLE);
        FL_HookTacticDamage(app,10,armies[0],0x34); update(); assert(proxies[0].pose==POSE_HIT);
        unsigned end=(body==1 || body==2 || body==6 || (body>=17 && body<=20))?80:60;
        at<float>(settings[0][0],0x24)=float(end-2); update(); assert(proxies[0].pose_done);
        update(); assert(proxies[0].pose==POSE_IDLE && constructions==1 && color_requests==2);
    }
    pass("all 25 body classes finish their original 180-frame strong and 60/80-frame hit clips without recreating appearance resources");

    fixture(1); settle(); tactic(); update(); at<float>(settings[0][0],0x24)=100; update();
    at<float>(settings[0][0],0x24)=0; update(); assert(proxies[0].pose_done); update(); assert(proxies[0].pose==POSE_IDLE);
    pass("native frame wrap ends a one-shot even when a low frame rate crosses its last frame");

    fixture(2); settle(); at<int>(image.data(),FL_TACTIC_CUTSCENE_ACTIVE)=1;
    tactic(2); update(); assert(proxies[0].pose==POSE_IDLE && proxies[1].pose==POSE_STRONG);
    at<int>(image.data(),FL_TACTIC_CUTSCENE_ACTIVE)=0;
    pass("cutscene and no-cutscene notifications share the effect-stage hook and target only the emitting army");

    fixture(1); settle(); FL_HookTacticDamage(app,50,armies[0],0x34);
    fl_test_now_ms+=1001; update(); assert(proxies[0].pose!=POSE_HIT);
    tactic(); fl_test_now_ms+=2001; update(); assert(proxies[0].pose!=POSE_STRONG);
    pass("expired tactic and hit notifications do not replay after a long view or loading interruption");

    tactic(); FL_HookManagerRelease(manager,engine); update(); assert(!proxies[0].skill_pending && proxies[0].pose==POSE_IDLE);
    at<int>(field[0],0x740)=1; update(); tactic(); assert(!proxies[0].constructed);
    pass("reload and water transitions discard pending presentation events rather than applying them to a new occupant");

    fixture(1); settle(); old_movement_voice=true; emit_hoof_sound=true;
    assert(FL_HookAmbientEnqueue(app,armies[0],123,0x99)==1);
    walk(); assert(!hoof_events && !proxies[0].ambient_removed && old_movement_voice);
    FL_HookAmbientCleanup(app); walk();
    assert(hoof_events==1 && proxies[0].ambient_removed && !old_movement_voice);
    pass("hoof playback cannot overlap the old movement voice: it begins only after native unmatched-voice cleanup returns");

    assert(last_native_gain>0 && last_native_gain<at<float>(hoof_params,8));
    for(unsigned i=0;i<52;++i) if(i<8 || i>=12) assert(last_native_params[i]==hoof_params[i]);
    assert(at<float>(hoof_params,8)==0.8f);
    pass("hoof attenuation copies the native request and changes only linear gain while preserving all nine sound arguments");

    unsigned before=ambient_calls;
    for(unsigned id:{0x9eu,0x9fu,0xa0u,0xaau,0xabu}) assert(FL_HookAmbientEnqueue(app,armies[0],123,id)==27);
    assert(ambient_calls==before+5);
    for(unsigned id:{0x99u,0x9au,0x9bu,0x9cu,0x9du,0xa5u,0xa6u,0xa7u})
        assert(FL_HookAmbientEnqueue(app,armies[0],123,id)==1);
    pass("only replacement-unit land movement profile IDs are omitted; native combat, attack and ship sounds pass through");

    fixture(1); load_ready=false; settle(); assert(FL_HookAmbientEnqueue(app,armies[0],123,0x99)==27);
    load_ready=true; settle(); handoff(); walk(); assert(proxies[0].hoof_gain>0);
    FL_RequestStop(NULL); assert(FL_HookAmbientEnqueue(app,armies[0],123,0x99)==27);
    assert(!private_hoof()); update(); assert(!proxies[0].constructed);
    pass("pending models retain original audio; Stop forwards native movement again and rejects all additional hoof starts");

    fixture(2); settle(); handoff(0); handoff(1); at<float>(field[1],0x10)=10000; walk(2);
    assert(proxies[0].hoof_gain>0 && proxies[1].hoof_gain==0); // Teleport resets movement first.
    walk(2); assert(proxies[1].hoof_gain>0 && proxies[0].hoof_gain>proxies[1].hoof_gain);
    float many_gain=proxies[0].hoof_gain;
    at<int>(field[1],0x60)=0; walk(1); assert(hoof_selected==1 && proxies[0].hoof_gain>many_gain);
    pass("distance attenuates farther movers and the per-source gain decreases as the moving group grows");

    at<float>(field[0],0x18)=-1; walk(); assert(!proxies[0].hoof_gain);
    at<float>(field[0],0x18)=20; walk(); assert(proxies[0].hoof_gain>0);
    at<int>(field[0],0x60)=0; update(); assert(!proxies[0].hoof_gain && !private_hoof());
    pass("onscreen projection and actual displacement gate sound; offscreen or stopped units emit no new footsteps");

    fixture(1); settle(); handoff(); walk();
    for(uint32_t id:{0xe3c22636u,0xc5540415u,0xa6e5e1f4u,0x8877bfd3u}) {
        fl_test_now_ms+=800; at<uint32_t>(hoof_params,0)=id; assert(private_hoof());
    }
    for(uint32_t id:{0u,0xba1e4b8bu,0xabcdef01u}) { at<uint32_t>(hoof_params,0)=id; assert(!private_hoof()); }
    pass("only the four original gait programs are admitted; neighs, weapon sounds and other private audio stay suppressed");

    at<uint32_t>(hoof_params,0)=0xe3c22636; fl_test_now_ms+=800;
    at<int>(hoof_params,4)=1; assert(!private_hoof()); at<int>(hoof_params,4)=0;
    at<float>(hoof_params,8)=NAN; assert(!private_hoof()); at<float>(hoof_params,8)=0.8f;
    sound_failure=true; unsigned played=hoof_events; assert(!private_hoof() && hoof_events==played);
    sound_failure=false; assert(private_hoof(0,false));
    pass("malformed or unknown pitch/gain is rejected, failed native playback consumes no voice budget, and the 2D path also works");

    fixture(1); settle(); handoff(); walk(); proxies[0].hoof_gain=0.01f;
    for(unsigned n=0;n<HOOF_VOICES;++n) assert(private_hoof());
    assert(!private_hoof() && hoof_events==HOOF_VOICES);
    fl_test_now_ms+=451; assert(private_hoof());
    pass("the 16-voice reservation cap applies even to quiet sources and releases only after the original finite sample plus tail allowance");

    memset(hoof_voices,0,sizeof hoof_voices); proxies[0].hoof_gain=1;
    unsigned admitted=0; for(unsigned n=0;n<20;++n) if(private_hoof())++admitted;
    float total=0; for(auto &voice:hoof_voices) if(voice.until>fl_test_now_ms) total+=voice.gain;
    assert(admitted==2 && total<=HOOF_TOTAL_GAIN+0.00001f);
    pass("the aggregate linear-gain ceiling limits simultaneous loud footsteps independently of the voice count cap");

    fixture(100); settle(51); for(unsigned i=0;i<100;++i) handoff(i);
    visible_checks=distance_checks=0; walk(100);
    assert(hoof_selected==8 && distance_checks==100 && visible_checks<=32);
    unsigned audible=0; for(auto &p:proxies) if(p.hoof_gain>0)++audible; assert(audible==8);
    pass("large moving groups have at most eight audible sources and 32 native view tests per field update");

    fixture(1); settle(); assert(FL_HookAmbientEnqueue(app,armies[0],123,0x99)==1);
    uint64_t serial=proxies[0].serial;
    set_id(0,501); update(); draw_frame(); assert(proxies[0].serial!=serial);
    FL_HookAmbientCleanup(app); assert(!proxies[0].ambient_removed);
    pass("audio cleanup acknowledgements are bound to private instance serials and cannot unlock a reused field slot");

    handoff(); walk(); assert(proxies[0].hoof_gain>0);
    at<int>(manager,0x498)=1; update(); assert(!proxies[0].hoof_gain && !private_hoof());
    at<int>(manager,0x498)=0; update(); draw_frame(); walk();
    root(FL_DATA_MANAGER,NULL); update(); assert(!private_hoof()); root(FL_DATA_MANAGER,data);
    FL_HookManagerRelease(manager,engine);
    for(auto &v:hoof_voices) assert(v.until==0);
    pass("other views, missing map owners and native teardown stop new hoof events and clear private audio budgets");
}
