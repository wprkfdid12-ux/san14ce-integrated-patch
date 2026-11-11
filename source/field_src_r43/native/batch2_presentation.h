#pragma once
/* R22: private presentation only. Native combat, capture, AI, RNG and save
   results are never changed. Included after character_model(). */
static unsigned native_dust_removed,native_arcs_removed,info_anchor_moves;
static unsigned optimized_attachments,appearance_refreshes,appearance_cursor;
static uintptr_t private_actor_vtable[16]; // RTTI locator followed by 15 entries

static Proxy *private_actor(void *actor) {
    uintptr_t value=(uintptr_t)actor,first=(uintptr_t)proxies;
    if(value<first || value>=first+sizeof proxies) return NULL;
    Proxy &p=proxies[(value-first)/sizeof(Proxy)];
    return p.constructed && actor==p.actor ? &p : NULL;
}
#include "idle_correction.h"
static int scaled_weapon_attach(void *actor,void *engine) {
    Proxy *p=private_actor(actor);
    if(!p) return fn<int (*)(void *,void *)>(FL_WEAPON_ATTACH)(actor,engine);
    apply_idle_correction(*p);
    p->attachment_frame=FL_Status.frame;
    p->scale_body_ok=p->scale_weapon_ok=false;
    void *body=character_model(*p,0),*horse=character_model(*p,1),*weapon=character_model(*p,2);
    if(!body || !horse || !weapon) return fn<int (*)(void *,void *)>(FL_WEAPON_ATTACH)(actor,engine);
    blend_attack_recovery(*p,body);
    alignas(16) float scale[4]={},horse_scale[4]={FL_FIELD_SCALE,FL_FIELD_SCALE,FL_FIELD_SCALE,0};
    fn<Call2>(FL_MODEL_GET_SCALE)(body,scale);
    for(unsigned i=0;i<3;++i) {
        if(!isfinite(scale[i]) || scale[i]<=0 || scale[i]>10) return 0;
        scale[i]*=FL_FIELD_SCALE;
    }
    if(at<void *>(horse,0x1c0)) return 0;
    fn<Call2>(FL_MODEL_SET_SCALE)(horse,horse_scale);
    /* Same final order as R21, AFTER both native animation updates: saddle,
       body scale, hand. We intercept the first weapon attach instead of
       performing it unscaled and immediately repeating it at field scale.
       The original early saddle attach stays in place for effect evaluation. */
    p->scale_body_ok=!at<int>(p->duel,0xfc) || fn<int (*)(void *,void *)>(FL_BODY_ATTACH)(actor,engine)!=0;
    if(!p->scale_body_ok) return 0;
    fn<Call2>(FL_MODEL_SET_SCALE)(body,scale);
    shorten_captured_fall(*p,body);
    int result=fn<int (*)(void *,void *)>(FL_WEAPON_ATTACH)(actor,engine);
    p->scale_weapon_ok=result!=0;
    if(result) ++optimized_attachments;
    return result;
}
static void bind_private_vtable(Proxy &p) {
    if(!private_actor_vtable[1]) {
        memcpy(private_actor_vtable,(void *)(base+FL_PRIVATE_ACTOR_VTABLE-8),sizeof private_actor_vtable);
        // Real-duel registry hooks keep their original vtable untouched. Our
        // descriptors use the already-resolved model and the native functions.
        private_actor_vtable[1+0x20/8]=base+FL_ACTOR_UPDATE;
        private_actor_vtable[1+0x48/8]=base+FL_ACTOR_CREATE;
        private_actor_vtable[1+0x68/8]=(uintptr_t)&scaled_weapon_attach;
    }
    at<uintptr_t>(p.actor,0)=(uintptr_t)(private_actor_vtable+1);
}

static bool terminal_source(const Proxy &p,void *actor) {
    /* The caller has a live original actor. Do not dereference the departed
       ArmyData or PersonData. Detach invalidates the old binding before reuse. */
    return p.constructed && p.defeated && !p.source_detached && p.source.actor==actor &&
        at<uintptr_t>(actor,0)==base+FL_ARMY_VTABLE && fl_land_mode(at<unsigned>(actor,0x740)) && at<void *>(actor,0x40)==p.source.army;
}
static bool replacement_for_actor(void *actor) {
    if(!actor || !presentation_owner_live()) return false;
    int slot=index_find(proxy_index,(uintptr_t)actor);
    if(slot<0) return false;
    Proxy &p=proxies[slot];
    return terminal_source(p,actor) || (p.ready && p.submitted && current_source(p));
}
static void trim_native_particles(Proxy &p,void *actor) {
    if(!p.ready || (!current_source(p) && !terminal_source(p,actor))) return;
    // Only slot0 is troop ground dust; status/fire slots1..6 remain original.
    void *dust=at<void *>(actor,0x890);
    if(dust) {
        int id=at<int>(dust,0x1c);
        if(id==2 || id==0x48 || id==0x61) {
            fn<Call1>(FL_FIELD_EFFECT_RESET)(dust);
            at<void *>(actor,0x890)=NULL; ++native_dust_removed;
        }
    }
    if(p.native_effects_trimmed) return;
    /* Existing normal-attack pairs predate attachment of this DLL. Their
       original pool list uses an index marker at +8d0, then data/next nodes.
       Do this once when ready; subsequent creation is intercepted below. */
    void *marker=at<void *>(actor,0x8d0),*table=global(0x27d4140);
    if(marker && table && global(0x27d4138)) {
        unsigned index=at<unsigned>(marker,0),capacity=at<unsigned>((void *)base,0x27d4170);
        void *node=index<capacity ? at<void *>(table,size_t(index)*8):NULL;
        for(unsigned n=0;node && n<64;++n,node=at<void *>(node,8)) {
            void *pair=at<void *>(node,0);
            void *effect=pair?at<void *>(pair,0x28):NULL;
            if(effect && (at<int>(effect,0x1c)==1 || at<int>(effect,0x1c)==0x51)) {
                fn<Call2>(FL_PAIR_EFFECT_SET)(pair,NULL); ++native_arcs_removed;
            }
        }
    }
    p.native_effects_trimmed=true;
}
typedef void *(*FieldEffectCreate)(void *,int,void *,void *,int);
extern "C" __declspec(dllexport) void *FL_HookGroundEffect(void *manager,int id,void *transform,void *actor,int flags) {
    lock_status();
    bool omit=(id==2 || id==0x48 || id==0x61) && replacement_for_actor(actor);
    if(omit) ++native_dust_removed;
    unlock_status();
    // Private moving horse supplies the original hoof dust. No duplicate
    // hidden-soldier dust, including the stationary native slot0 effect2.
    return omit?NULL:fn<FieldEffectCreate>(FL_FIELD_EFFECT_CREATE)(manager,id,transform,actor,flags);
}
extern "C" __declspec(dllexport) void *FL_HookNormalFieldEffect(void *manager,int id,void *transform,void *pair,int flags) {
    lock_status();
    bool omit=false;
    if((id==1 || id==0x51) && pair && presentation_owner_live()) {
        void *descriptor=at<void *>(pair,0);
        if(descriptor && at<int>(descriptor,4)==0x1b) {
            unsigned army_id=at<uint16_t>(descriptor,0);
            Proxy *p=army_id<=500?bound_army(at<void *>(owner_data,0x7e838+army_id*8)):NULL;
            omit=p && p->ready && p->submitted;
        }
    }
    if(omit) ++native_arcs_removed;
    unlock_status();
    // Only 1c1240's ordinary field attack. Strong duel EffectDopeSheet has
    // a different call path and remains untouched by this filter.
    return omit?NULL:fn<FieldEffectCreate>(FL_FIELD_EFFECT_CREATE)(manager,id,transform,pair,flags);
}
extern "C" __declspec(dllexport) void FL_HookDefeatCommit(void *data,void *army) {
    lock_status();
    if(army && data==owner_data && presentation_owner_live()) {
        int slot=index_find(army_index,(uintptr_t)army);
        if(slot>=0) {
            Proxy &p=proxies[slot];
            if(p.constructed && !p.defeated && p.source.army==army) {
                void *person=at<void *>(data,0x158+size_t(p.source.id)*8);
                p.captured=person && person==p.source.person &&
                    at<uintptr_t>(person,0)==base+FL_PERSON_VTABLE && at<uint8_t>(person,0xe6)==6;
                p.defeated=true; p.terminal_ms=GetTickCount64();
                p.running=p.combat_engaged=false;
                p.skill_pending=p.hit_pending=p.guard_pending=p.normal_pending=p.reaction_waiting=false;
                p.hoof_gain=p.combat_gain=0;
                log_line("R22_DEFEAT officer=%u captured=%d native_result_only=1",p.source.id,p.captured);
            }
        }
    }
    unlock_status();
    fn<Call2>(FL_DEFEAT_COMMIT)(data,army); // exactly once, unchanged ordering/result
}
extern "C" __declspec(dllexport) void FL_HookArmyDetach(void *actor) {
    lock_status();
    if(actor && presentation_owner_live()) {
        int slot=index_find(proxy_index,(uintptr_t)actor);
        if(slot>=0 && proxies[slot].source.actor==actor) proxies[slot].source_detached=true;
    }
    unlock_status();
    fn<Call1>(FL_ARMY_DETACH)(actor);
}

#include "final_ui.h"
extern "C" __declspec(dllexport) InfoPoint *FL_HookInfoAnchor(void *layout,InfoPoint *out,uint64_t point) {
    InfoPoint *result=fn<InfoPoint *(*)(void *,InfoPoint *,uint64_t)>(FL_INFO_ANCHOR)(layout,out,point);
    if(!layout || !out || result!=out) return result;
    lock_status();
    bool enabled=info_modified_unit(layout);
    compact_info_padding(layout,enabled);
    if(enabled) {
        // Same X, same whole native layout (portrait/count/icons/hit rectangle).
        // Remove the stock high offset, centre just above the projected foot
        // point. This keeps the badge among the legs, within its own tile.
        int height=at<int>(layout,0x2c);
        void *child=at<void *>(layout,0x258);
        if(child && fn<int (*)(void *)>(FL_UI_VISIBLE)(child)) {
            InfoPoint size={}; fn<Call2>(FL_INFO_SIZE)(child,&size); height=size.y;
        }
        int y=(int32_t)(point>>32);
        if(height>0 && height<1024 && y>-32768 && y<32768) {
            out->y=y-height/2-8;
            ++info_anchor_moves;
        }
    }
    unlock_status();
    return result;
}
