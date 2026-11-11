#pragma once
static unsigned field_effect_calls,field_resets,pair_resets,defeat_commits,army_detaches;
static void *last_field_owner,*last_field_transform,*last_defeat_data,*last_defeat_army;
static int last_field_id,last_field_flags;
static bool mock_child_visible;
static void *field_effect_create(void *m,int id,void *transform,void *owner,int flags) {
    assert(!state_lock.depth && m==manager);
    ++field_effect_calls; last_field_owner=owner; last_field_transform=transform;
    last_field_id=id; last_field_flags=flags; return callback;
}
static void field_effect_reset(void *effect) {
    ++field_resets; at<int>(effect,0x1c)=-1;
}
static void pair_effect_set(void *pair,void *effect) {
    assert(effect==NULL); ++pair_resets;
    field_effect_reset(at<void *>(pair,0x28)); at<void *>(pair,0x28)=NULL;
}
static void defeat_commit(void *d,void *army) {
    assert(!state_lock.depth); ++defeat_commits; last_defeat_data=d; last_defeat_army=army;
}
static void army_detach(void *actor) {
    assert(!state_lock.depth); ++army_detaches; at<void *>(actor,0x40)=NULL;
}
static InfoPoint *info_anchor(void *layout,InfoPoint *out,uint64_t point) {
    assert(!state_lock.depth);
    out->x=int32_t(point)-at<int>(layout,0x28)/2;
    out->y=int32_t(point>>32)-100;
    return out;
}
static int ui_visible(void *) { return mock_child_visible; }
static void info_size(void *child,void *out) {
    memcpy(out,(unsigned char *)child+0x28,8);
}
