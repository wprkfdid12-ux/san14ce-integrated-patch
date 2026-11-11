#pragma once
// Native field combat ambience IDs; not a master-volume or duel-bank change.
static constexpr float FIELD_COMBAT_GAIN=0.40f;
struct AmbientLevel { uint32_t voice, sound; float percent; bool changed; };
static AmbientLevel ambient_levels[62];
static void *ambient_level_owner;
static unsigned ambient_attenuations;
static bool ambient_owned[62];
static thread_local void *ambient_start_cursor;
typedef int (*AmbientStart)(void *,int,float,float,float);
typedef int (*AmbientVolume)(void *,int,float);
static bool combat_ambient_id(uint32_t sound) { return sound>=0x9e && sound<=0xa0; }
static void *current_sound_controller() {
    void *manager=global(FL_SOUND_MANAGER);
    return manager?static_cast<unsigned char *>(manager)+0x2a0:NULL;
}
static void sync_combat_ambient(bool restore) {
    // Resolve the current manager anew; never dereference a saved voice owner.
    void *controller=current_sound_controller();
    if(controller!=ambient_level_owner) {
        memset(ambient_levels,0,sizeof ambient_levels); memset(ambient_owned,0,sizeof ambient_owned);
        ambient_level_owner=controller;
    }
    if(!controller || !at<int>(controller,0x118)) return;
    for(unsigned slot=0;slot<62;++slot) {
        AmbientLevel &v=ambient_levels[slot];
        uint32_t sound=at<uint32_t>(controller,8+slot*4),voice=at<uint32_t>(controller,0x228+slot*4);
        if(!voice || !at<int>(controller,0x518+slot*4) || !combat_ambient_id(sound)) { v={}; continue; }
        bool same=v.changed && v.voice==voice && v.sound==sound;
        if(restore || !ambient_owned[slot]) {
            if(same) fn<AmbientVolume>(FL_AMBIENT_VOLUME)(controller,int(slot),v.percent);
            v={};
        } else if(!same) {
            if(fn<AmbientVolume>(FL_AMBIENT_VOLUME)(controller,int(slot),100.0f*FIELD_COMBAT_GAIN)) {
                v={voice,sound,100.0f,true}; ++ambient_attenuations;
            }
        }
    }
}
static void classify_combat_ambient(void *collector) {
    // Original 403c90/405920 nodes: army+0, tile+8, sound+c, voice slot+10,
    // next+18. Only this live callback's bounded native queue is inspected.
    void *controller=current_sound_controller();
    if(controller!=ambient_level_owner) {
        memset(ambient_levels,0,sizeof ambient_levels); ambient_level_owner=controller;
    }
    memset(ambient_owned,0,sizeof ambient_owned);
    ambient_start_cursor=at<void *>(collector,0xb0);
    void *node=ambient_start_cursor;
    for(unsigned n=0;node && n<42;++n,node=at<void *>(node,0x18)) {
        int slot=at<int>(node,0x10);
        Proxy *p=bound_army(at<void *>(node,0));
        if(slot>=0 && slot<62 && p && p->ready && current_source(*p) &&
           combat_ambient_id(at<uint32_t>(node,0xc))) ambient_owned[slot]=true;
    }
    sync_combat_ambient(false);
}
// R35: 게임 원본(0x408e68 'cmp edx,imm32')과 같은 칸 상한을 씀 — 맵 확장이 이 값을 넓혀도(0xbd0f → 셀 수-1) 걸음이 어긋나지 않게
static uint32_t ambient_tile_limit() {
#ifdef FL_PORTABLE_TEST
    return 0xbd0f;
#else
    uint32_t v=*reinterpret_cast<const uint32_t *>(base+0x408e6a);
    return v>=0xbd0f && v<0x80000000u ? v : 0xbd0f;
#endif
}
extern "C" __declspec(dllexport) int FL_HookAmbientStart(void *controller,int sound,float x,float y,float z) {
    bool owned=false;
    lock_status();
    for(unsigned n=0;ambient_start_cursor && n<42;++n) {
        void *node=ambient_start_cursor; ambient_start_cursor=at<void *>(node,0x18);
        if(at<int>(node,0x10)!=-1 || at<uint32_t>(node,8)>ambient_tile_limit()) continue;
        if(at<int>(node,0xc)==sound) {
            Proxy *p=bound_army(at<void *>(node,0));
            owned=p && p->ready && current_source(*p);
        }
        break;
    }
    unlock_status();
    int slot=fn<AmbientStart>(FL_AMBIENT_START)(controller,sound,x,y,z);
    lock_status();
    if(slot>=0 && slot<62 && combat_ambient_id(unsigned(sound)) &&
       controller==current_sound_controller() && presentation_owner_live()) {
        ambient_owned[slot]=owned;
        sync_combat_ambient(false);
    }
    unlock_status();
    return slot;
}
extern "C" __declspec(dllexport) int FL_HookAmbientVolume(void *controller,int slot,float percent) {
    float gain=percent;
    lock_status();
    if(controller && controller==current_sound_controller() && unsigned(slot)<62 && isfinite(percent) &&
       ambient_level_owner==controller && ambient_owned[slot] && presentation_owner_live() && combat_ambient_id(at<uint32_t>(controller,8+size_t(slot)*4))) {
        if(ambient_level_owner!=controller) {
            memset(ambient_levels,0,sizeof ambient_levels); ambient_level_owner=controller;
        }
        ambient_levels[slot]={at<uint32_t>(controller,0x228+size_t(slot)*4),
            at<uint32_t>(controller,8+size_t(slot)*4),percent,true};
        gain=percent*FIELD_COMBAT_GAIN; ++ambient_attenuations;
    } else if(controller==ambient_level_owner && unsigned(slot)<62) {
        // A fresh unattenuated request after Stop supersedes the saved level.
        ambient_levels[slot]={};
    }
    unlock_status();
    // Scale each fresh native request, never the previously scaled value.
    return fn<AmbientVolume>(FL_AMBIENT_VOLUME)(controller,slot,gain);
}
