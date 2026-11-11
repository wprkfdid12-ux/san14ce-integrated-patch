#pragma once
/* R23 private model poses only. Layout derived from original bd76a0/dac1d0:
   CCharacter+150 -> skeleton instance, +10 -> 48-byte local SRT array;
   descriptor +4 = bone count, +6 = logical joint count, +c = mapping. */
static LocalBonePose *private_bones(void *model,unsigned &count) {
    count=0;
    if(!model || at<uintptr_t>(model,0)!=base+FL_CHARACTER_VTABLE) return NULL;
    void *instance=at<void *>(model,0x150);
    void *skeleton=instance?at<void *>(instance,0):NULL;
    void *descriptor=skeleton?at<void *>(skeleton,0):NULL;
    if(!descriptor) return NULL;
    int n=at<int16_t>(descriptor,4);
    if(n<1 || n>256) return NULL;
    auto *bones=at<LocalBonePose *>(instance,0x10);
    if(!bones) return NULL;
    count=unsigned(n); return bones;
}
static bool capture_local_pose(Proxy &p,unsigned part,void *&model,unsigned &count,LocalBonePose *out) {
    count=0;model=character_model(p,part);unsigned n=0;
    auto *bones=private_bones(model,n);
    if(!bones || !IdleSeam::valid(bones,n))return false;
    memcpy(out,bones,n*sizeof *bones);count=n;return true;
}
static bool transition_pose(Pose pose) {
    return pose==POSE_IDLE || pose==POSE_MOVE || pose==POSE_ATTACK || pose==POSE_BATTLE_IDLE;
}
static void capture_pose_transition(Proxy &p,Pose next) {
    float facing=pose_facing_offset(p);
    p.blend_count=p.blend_horse_count=p.blend_duration_ms=0;
    if(!p.ready || p.defeated || !transition_pose(p.pose) || !transition_pose(next))return;
    p.blend_facing_from=facing;p.blend_began_ms=presentation_ms;
    p.blend_duration_ms=p.pose==POSE_ATTACK && next==POSE_BATTLE_IDLE?180u:
        next==POSE_ATTACK?90u:next==POSE_MOVE?100u:140u;
    capture_local_pose(p,0,p.blend_model,p.blend_count,p.blend_from);
    // Same horse action continues on its original clock. Capture a second
    // skeleton only for an actual idle/run change, never every combat swing.
    bool old_move=at<int>(p.duel,0x2c)==1,new_move=next==POSE_MOVE;
    if(old_move!=new_move)
        capture_local_pose(p,1,p.blend_horse_model,p.blend_horse_count,p.blend_horse_from);
    ++perf_transition_captures;
}
static void capture_attack_recovery(Proxy &p) { capture_pose_transition(p,POSE_BATTLE_IDLE); }
static void blend_local_transition(void *model,void *expected,unsigned &saved_count,
        const LocalBonePose *from,float t) {
    if(!saved_count || model!=expected) {saved_count=0;return;}
    unsigned count=0;auto *bones=private_bones(model,count);
    if(!bones || count!=saved_count || !IdleSeam::valid(bones,count)) {saved_count=0;return;}
    for(unsigned i=0;i<count;++i)if(bones[i].parent!=from[i].parent){saved_count=0;return;}
    for(unsigned i=0;i<count;++i) {
        LocalBonePose &to=bones[i];const LocalBonePose &old=from[i];
        float dot=0,norm=0,q[4];
        for(unsigned k=0;k<4;++k)dot+=old.rotation[k]*to.rotation[k];
        for(unsigned k=0;k<4;++k){q[k]=old.rotation[k]*(1-t)+(dot<0?-1:1)*to.rotation[k]*t;norm+=q[k]*q[k];}
        if(isfinite(norm)&&norm>0.000001f){float inv=1/sqrtf(norm);for(unsigned k=0;k<4;++k)to.rotation[k]=q[k]*inv;}
        for(unsigned k=0;k<3;++k){to.scale[k]=old.scale[k]*(1-t)+to.scale[k]*t;to.position[k]=old.position[k]*(1-t)+to.position[k]*t;}
    }
}
static void blend_attack_recovery(Proxy &p,void *model) {
    if(private_staging || !p.blend_duration_ms)return;
    if(presentation_ms<p.blend_began_ms || presentation_ms-p.blend_began_ms>=p.blend_duration_ms) {
        p.blend_count=p.blend_horse_count=p.blend_duration_ms=0;return;
    }
    float t=float(presentation_ms-p.blend_began_ms)/p.blend_duration_ms;t=t*t*(3-2*t);
    blend_local_transition(model,p.blend_model,p.blend_count,p.blend_from,t);
    if(p.blend_horse_count)
        blend_local_transition(character_model(p,1),p.blend_horse_model,p.blend_horse_count,p.blend_horse_from,t);
    // Native scale, saddle and hand attachment run afterwards on these same
    // bones, invalidating transform caches without a second animation pass.
}
static unsigned fall_distance_corrections;
static void shorten_captured_fall(Proxy &p,void *body) {
    if(!p.defeated || !p.captured || p.pose!=POSE_FALL) return;
    // Logical joint1 is the pelvis (root child), including animated translation.
    alignas(16) unsigned char bone[48]={};
    if(!fn<bool (*)(void *,int,void *)>(FL_MODEL_BONE_TRANSFORM)(body,1,bone)) return;
    // dac1d0 packs scale at0, quaternion at+c, WORLD POSITION at+1c.
    double dx=double(at<float>(bone,0x1c))-p.source.x;
    double dz=double(at<float>(bone,0x24))-p.source.z;
    double distance=sqrt(dx*dx+dz*dz);
    if(!isfinite(distance) || distance<0.001 || distance>100000) return;
    // Native grid 187f50: column step702; row step468*sqrt(3).
    const double one_hex=468.0*1.7320508075688772935;
    double factor=0.25;
    if(distance*factor>one_hex) factor=one_hex/distance;
    alignas(16) float position[4]={};
    fn<Call2>(FL_MODEL_GET_POSITION)(body,position);
    if(!isfinite(position[0]) || !isfinite(position[1]) || !isfinite(position[2])) return;
    position[0]-=float(dx*(1-factor)); position[2]-=float(dz*(1-factor));
    fn<Call2>(FL_MODEL_SET_POSITION)(body,position);
    ++fall_distance_corrections;
}
static bool live_combat_target(const Proxy &p) {
    if(p.opponent_serial && p.opponent_slot<FL_PROXY_COUNT) {
        const Proxy &other=proxies[p.opponent_slot];
        if(other.serial==p.opponent_serial && current_source(other) &&
           at<uint16_t>(other.source.army,0x16)>0) return true;
    }
    // Native outgoing battle pairs, also usable before the first damage event.
    // Bound local list only: no distance search, no all-army scan, no stale flag.
    void *marker=at<void *>(p.source.actor,0x8d0),*table=global(0x27d10f0);
    if(!marker || !table || !global(0x27d10e8)) return false;
    unsigned index=at<unsigned>(marker,0),capacity=at<unsigned>((void *)base,0x27d1120);
    void *node=index<capacity?at<void *>(table,size_t(index)*8):NULL;
    for(unsigned n=0;node && n<64;++n,node=at<void *>(node,8)) {
        void *pair=at<void *>(node,0);
        if(!pair) continue;
        unsigned id=at<uint16_t>(pair,8); int kind=at<int>(pair,0xc);
        size_t offset=0; unsigned limit=0;
        // Same object lookup cases as original 16dfb0.
        switch(kind) {
        case 0x1b:offset=0x7e838;limit=500;break;
        case 5:offset=0xdd38;limit=51;break;
        case 6:offset=0xded8;limit=10;break;
        case 11:offset=0x6da98;limit=3000;break;
        default:continue;
        }
        if(id>limit) continue;
        void *target=at<void *>(owner_data,offset+size_t(id)*8);
        if(!target || target==p.source.army || !fn<int (*)(void *)>(FL_NATIVE_VALID)(target)) continue;
        if(kind==0x1b && !at<uint16_t>(target,0x16)) continue;
        return true;
    }
    return false;
}
