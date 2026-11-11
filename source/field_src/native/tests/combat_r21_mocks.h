#pragma once
static unsigned char sound_manager_memory[0x2000];
static int ambient_start_slot=7;
static unsigned ambient_start_calls,ambient_volume_calls,army_update_calls;
static float last_ambient_percent,ambient_percent[62];
static bool mock_field_emit,mock_field_move;
static void *test_effect(uint32_t hash=23) {
    expected_effect_hash=hash;
    return FL_HookMotionEffect(manager,hash,app,engine,context,67,1,2,3,4,5,6,7,8,9,10,11,12);
}
static int ambient_start(void *controller,int sound,float x,float y,float z) {
    assert(!state_lock.depth && controller==sound_manager_memory+0x2a0 && x==11 && y==22 && z==33);
    ++ambient_start_calls;
    if(ambient_start_slot>=0 && ambient_start_slot<62) {
        at<int>(controller,8+ambient_start_slot*4)=sound;
        at<int>(controller,0x228+ambient_start_slot*4)=1000+ambient_start_calls;
        at<int>(controller,0x518+ambient_start_slot*4)=1;
        ambient_percent[ambient_start_slot]=100;
    }
    return ambient_start_slot;
}
static int ambient_volume(void *controller,int slot,float percent) {
    assert(controller==sound_manager_memory+0x2a0);
    ++ambient_volume_calls; last_ambient_percent=percent;
    if(unsigned(slot)>=62) return 0;
    ambient_percent[slot]=percent; return 1;
}
static void army_update(void *actor,void *e,void *frame) {
    assert(!state_lock.depth && actor && e==engine && frame==app); ++army_update_calls;
    if(mock_field_move) at<float>(actor,0x10)+=2;
    if(mock_field_emit) test_effect();
}
