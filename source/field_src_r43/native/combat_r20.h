#pragma once
/* Cosmetic decisions only. No damage output, native RNG, shared animation
   resource or ArmyData is modified. Included after bound_army(). */
static uint64_t guard_rng;
static bool strategy_pause;
static unsigned paused_guard_trials;
static unsigned suppressed_normal_effects, guard_trials, guard_successes, tactic_pairs;

static unsigned char compare_strength(float self_a,float self_d,float other_a,float other_d) {
    if (!isfinite(self_a) || !isfinite(self_d) || !isfinite(other_a) || !isfinite(other_d) ||
        self_a<0 || self_d<0 || other_a<0 || other_d<0 || self_a+self_d<=0 ||
        other_a+other_d<=0 || self_a+self_d>1000000 || other_a+other_d>1000000) return 0;
    return other_a+other_d>self_a+self_d ? 2 : 1;
}
static float clamp_guard(float x,float lo,float hi) { return x<lo?lo:x>hi?hi:x; }
static float guard_probability(float aa,float ad,float da,float dd,float factor=1.0f) {
    if (!isfinite(aa) || !isfinite(ad) || !isfinite(da) || !isfinite(dd) ||
        aa<0 || ad<0 || da<0 || dd<0 || aa+ad<=0 || da+dd<=0 ||
        aa+ad>1000000 || da+dd>1000000 || !isfinite(factor) || factor<=0) return -1;
    const float ratio=(da+dd)/(aa+ad);
    const float x[]={0.50f,0.65f,0.80f,1.0f,1.25f,1.50f,1.75f};
    const float y[]={0.15f,0.25f,0.40f,0.55f,0.65f,0.75f,0.80f};
    float probability=y[0];
    for(unsigned i=1;i<7;++i) {
        if(ratio>=x[i]) probability=y[i];
        else if(ratio>x[i-1]) {
            probability=y[i-1]+(y[i]-y[i-1])*(ratio-x[i-1])/(x[i]-x[i-1]); break;
        } else break;
    }
    probability+=clamp_guard(0.2f*(dd/(da+dd)-ad/(aa+ad)),-0.05f,0.05f);
    probability+=clamp_guard(0.10f*(1.0f-factor),-0.10f,0.10f);
    return clamp_guard(probability,0.15f,0.80f);
}
static float guard_random() {
    /* Separate SplitMix64 stream. Neither successes nor preceding failures
       enter this state. One draw per eligible incoming attack or paused visual contact. */
    if(!guard_rng) guard_rng=GetTickCount64()^uint64_t(base)^0x94781a2635d0c6bfull;
    uint64_t z=(guard_rng+=0x9e3779b97f4a7c15ull);
    z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;
    z=(z^(z>>27))*0x94d049bb133111ebull;
    return float((z^(z>>31))>>40)*(1.0f/16777216.0f);
}
static bool queue_guard(Proxy &p,float probability,bool tactic,bool enqueue=true,unsigned char relation=0) {
    if(probability<0 || !p.ready) return false;
    uint64_t now=GetTickCount64();
    ++p.guard_draws; ++guard_trials;
    bool win=guard_random()<probability;
    if(win) {
        ++p.guard_wins; ++guard_successes;
        if(enqueue) { p.guard_pending=true; p.guard_pending_ms=now; p.guard_relation=relation; }
        if(tactic) p.tactic_guard_ms=now;
    }
    if(p.guard_draws<=4 || !(p.guard_draws&(p.guard_draws-1)))
        log_line("GUARD_DRAW officer=%u tactic=%d chance=%.4f win=%d trials=%u wins=%u independent=1 gameplay_damage_unchanged=1",
            p.source.id,tactic,probability,win,p.guard_draws,p.guard_wins);
    return win;
}
typedef void (*NormalDamage)(void *,float,float,void *,float,float,int *);
extern "C" __declspec(dllexport) void FL_HookNormalDamage(void *source,float aa,float ad,
        void *target,float da,float dd,int *damage) {
    fn<NormalDamage>(FL_NORMAL_DAMAGE)(source,aa,ad,target,da,dd,damage);
    lock_status();
    {
        PresentationTimer cost(perf_guard);
        Proxy *attacker=bound_army(source), *defender=bound_army(target);
        if(attacker && defender && attacker!=defender && attacker->ready && defender->ready && damage && *damage>0) {
            float chance=guard_probability(aa,ad,da,dd);
            attacker->normal_pending=true; attacker->normal_pending_ms=GetTickCount64();
            attacker->opponent_slot=unsigned(defender-proxies);
            attacker->opponent_serial=defender->serial; attacker->opponent_chance=chance;
            attacker->opponent_relation=compare_strength(aa,ad,da,dd);
            defender->opponent_slot=unsigned(attacker-proxies);
            defender->opponent_serial=attacker->serial;
            defender->opponent_chance=guard_probability(da,dd,aa,ad);
            defender->opponent_relation=compare_strength(da,dd,aa,ad);
            attacker->opponent_target_relation=defender->opponent_relation;
            defender->opponent_target_relation=attacker->opponent_relation;
            attacker->normal_blocked=queue_guard(*defender,chance,false,true,defender->opponent_relation);
        }
    }
    unlock_status();
}

/* Original EffectDopeSheet creation call has 18 arguments: the last twelve
   floats occupy stack slots +30..+88. NULL is its native checked failure path.
   TLS is set only around our private actor update, including weapon sheets. */
typedef void *(*MotionEffect)(void *,uint32_t,void *,void *,void *,uint32_t,
    float,float,float,float,float,float,float,float,float,float,float,float);
struct FieldEffectScope { Proxy *proxy; uint64_t serial; void *actor; };
static thread_local FieldEffectScope field_effect_scope;
static unsigned suppressed_stationary_dust, suppressed_stationary_field_effects;
extern "C" __declspec(dllexport) void FL_HookArmyUpdate(void *actor,void *engine,void *frame) {
    FieldEffectScope saved=field_effect_scope; field_effect_scope={};
    lock_status();
    if(actor && engine==owner_engine && presentation_owner_live()) {
        int slot=index_find(proxy_index,(uintptr_t)actor);
        if(slot>=0) {
            Proxy &p=proxies[slot];
            if(p.ready && p.submitted && FL_Status.frame-p.last_draw_frame<=1 && current_source(p))
                field_effect_scope={&p,p.serial,actor};
        }
    }
    unlock_status();
    // This is the original army actor's graphics update, not an army AI tick.
    // Its vtable entry is guarded as an eight-byte relocated pointer. The
    // original function still runs exactly once with every argument intact.
    // Sample one in 64 timings, count all calls. Keep QPC overhead bounded.
    static thread_local uint64_t army_sample;
    const uint64_t started=(++army_sample&63)==1?performance_tick():0;
    fn<Call3>(FL_ARMY_UPDATE)(actor,engine,frame);
    lock_status();
    ++perf_army_calls;
    if(started) performance_record(perf_army,started);
    if(field_effect_scope.proxy && presentation_owner_live() &&
       field_effect_scope.proxy->serial==field_effect_scope.serial)
        trim_native_particles(*field_effect_scope.proxy,actor);
    unlock_status();
    field_effect_scope=saved;
}
static bool suppress_field_effect(uint32_t display) {
    if(!field_effect_scope.proxy) return false;
    bool omit=false;
    lock_status();
    Proxy &p=*field_effect_scope.proxy;
    if(p.serial==field_effect_scope.serial && p.source.actor==field_effect_scope.actor &&
       p.ready && presentation_owner_live() && current_source(p)) {
        // The native update may have moved the actor already, so compare the
        // fresh position with the last completed frame instead of stale pose.
        double x=double(at<float>(field_effect_scope.actor,0x10))-p.source.x;
        double z=double(at<float>(field_effect_scope.actor,0x18))-p.source.z;
        double d=x*x+z*z;
        bool moving=isfinite(d) && d>0.0001 && d<=4096.0*4096.0;
        omit=!moving && (display==0x7363ee3a || (p.pose!=POSE_STRONG && !p.skill_pending));
        if(omit) ++suppressed_stationary_field_effects;
    }
    unlock_status();
    return omit;
}
extern "C" __declspec(dllexport) void *FL_HookMotionEffect(void *a,uint32_t b,void *c,
        void *d,void *e,uint32_t f,float g,float h,float i,float j,float k,float l,
        float m,float n,float o,float p,float q,float r) {
    if(private_presentation_depth && private_sound_proxy) {
        if(private_staging || private_sound_proxy->visual_deferred || (FL_FOG_VARIANT && private_sound_proxy->fog_hidden)) return NULL;
        const Proxy &actor=*private_sound_proxy;
        if(b==0x7363ee3a && !actor.running) { ++suppressed_stationary_dust; return NULL; }
        if(actor.pose==POSE_ATTACK) { ++suppressed_normal_effects; return NULL; }
        if(actor.pose!=POSE_MOVE && actor.pose!=POSE_STRONG &&
           !(actor.pose==POSE_HIT && b==0xca81df19)) return NULL;
    } else if(suppress_field_effect(b)) {
        return NULL;
    }
    return fn<MotionEffect>(FL_MOTION_EFFECT)(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r);
}

static void tactic_guards(void *value) {
    /* Actual effect stage, including child/linked tactics. Iterate only this
       native event's two effect vectors, never the field army list. */
    lock_status();
    bool live=value && presentation_owner_live() && at<int>(value,8)==0x1b && at<void *>(value,0);
    int source_id=live?at<int>(value,0xc):-1;
    void *data_now=owner_data;
    Proxy *source=live && source_id>=0 && source_id<=500 ?
        bound_army(at<void *>(data_now,0x7e838+size_t(source_id)*8)):NULL;
    bool source_ready=source && source->ready;
    uint64_t source_serial=source?source->serial:0;
    void *source_army=source?source->source.army:NULL;
    uint64_t begin=performance_tick();
    unlock_status();
    if(!source_ready) return;
    bool seen[501]={};
    for(unsigned effect=0;effect<2;++effect) {
        void *tactic=at<void *>(value,0);
        if(fn<int (*)(void *,int)>(FL_TACTIC_KIND)(tactic,int(effect))!=1) continue;
        size_t off=0x10+effect*0x18;
        uintptr_t first=at<uintptr_t>(value,off),end=at<uintptr_t>(value,off+8);
        if(!first || end<first || (end-first)%16 || (end-first)/16>500) continue;
        for(uintptr_t row=first;row<end;row+=16) {
            void *record=(void *)row;
            int target_id=at<int>(record,4);
            if(at<int>(record,0)!=0x1b || target_id<0 || target_id>500 ||
               target_id==source_id || !at<uint16_t>(record,8) || seen[target_id]) continue;
            seen[target_id]=true;
            lock_status();
            Proxy *attacker=bound_army(source_army);
            Proxy *defender=presentation_owner_live() && owner_data==data_now ?
                bound_army(at<void *>(data_now,0x7e838+size_t(target_id)*8)):NULL;
            bool valid=attacker && attacker->serial==source_serial && defender && defender->ready;
            uint64_t serial=valid?defender->serial:0;
            void *target_army=valid?defender->source.army:NULL;
            if(!valid) { unlock_status(); continue; }
            /* Native stat readers operate on private scratch descriptors.
               Unit totals use one target, not a fabricated damage formula.
               The private-owner lock spans these read-only native calls so
               teardown cannot recycle a proxy halfway through a decision.
               Read once per actual tactic target; normal attacks reuse their
               already-computed four operands directly. */
            alignas(16) unsigned char source_ref[0x20]={},pair[0x30]={},ctx[0x38]={};
            at<uint16_t>(source_ref,0)=uint16_t(source_id); at<int>(source_ref,4)=0x1b;
            at<void *>(pair,0)=source_ref;
            at<uint16_t>(pair,8)=uint16_t(target_id); at<int>(pair,0xc)=0x1b;
            float scores[10]={};
            typedef float (*Power)(void *,void *,int,int);
            scores[0]=fn<Power>(FL_BATTLE_POWER)(source_army,NULL,1,1);
            scores[5]=fn<Power>(FL_BATTLE_POWER)(target_army,NULL,1,1);
            fn<Call2>(FL_PAIR_ATTACK)(pair,scores);
            fn<Call2>(FL_PAIR_DEFENSE)(pair,scores);
            at<void *>(ctx,0)=tactic; at<void *>(ctx,0x10)=source_army;
            float factor=fn<float (*)(void *,void *,void *)>(FL_TACTIC_FACTOR)(ctx,target_army,NULL);
            float chance=guard_probability(scores[1],scores[2],scores[6],scores[7],factor);
            attacker=bound_army(source_army); defender=bound_army(target_army);
            if(attacker && attacker->serial==source_serial && defender && defender->serial==serial) {
                ++tactic_pairs;
                bool blocked_result=queue_guard(*defender,chance,true,false);
                defender->tactic_decided_ms=GetTickCount64();
                defender->reaction_source_slot=unsigned(attacker-proxies);
                defender->reaction_source_serial=attacker->serial;
                defender->reaction_token=attacker->skill_token;
                defender->reaction_deadline_ms=GetTickCount64()+5000;
                defender->reaction_blocked=blocked_result; defender->reaction_waiting=true;
                defender->reaction_relation=compare_strength(scores[6],scores[7],scores[1],scores[2]);
                unsigned slot=unsigned(defender-proxies);
                attacker->tactic_targets[slot/64]|=uint64_t(1)<<(slot%64);
            }
            unlock_status();
        }
    }
    lock_status(); performance_record(perf_guard,begin); unlock_status();
}

/* Only serials/slots and cosmetic scores survive an event; no native event
   pointer or audio callback argument is retained. Caller holds state_lock. */
static Proxy *cached_opponent(Proxy &p) {
    if(!p.opponent_serial || p.opponent_slot>=FL_PROXY_COUNT) return NULL;
    Proxy &other=proxies[p.opponent_slot];
    if(!other.constructed || !other.ready || other.serial!=p.opponent_serial ||
       !other.combat_engaged || !p.combat_engaged || other.running || p.running ||
       !current_source(other) || !current_source(p)) return NULL;
    return &other;
}
static void normal_contact(Proxy &p) {
    if(p.normal_contact_done) return;
    p.normal_contact_done=true;
    if(!strategy_pause) return;
    Proxy *other=cached_opponent(p);
    if(!other || p.opponent_chance<0) return;
    ++paused_guard_trials;
    p.normal_blocked=queue_guard(*other,p.opponent_chance,false,true,p.opponent_target_relation);
}
static unsigned strong_contact(Proxy &p,unsigned *contacts) {
    if(p.strong_contact_done) return 0;
    p.strong_contact_done=true;
    unsigned count=0;
    for(unsigned word=0;word<8;++word) {
        uint64_t bits=p.tactic_targets[word]; p.tactic_targets[word]=0;
        while(bits) {
            unsigned bit=unsigned(__builtin_ctzll(bits)),slot=word*64+bit;
            bits&=bits-1;
            if(slot>=FL_PROXY_COUNT) continue;
            Proxy &target=proxies[slot];
            if(!target.reaction_waiting || target.reaction_source_serial!=p.serial ||
               target.reaction_source_slot!=unsigned(&p-proxies) || target.reaction_token!=p.skill_token) continue;
            target.reaction_waiting=false;
            if(!target.ready || !target.constructed || !current_source(target) ||
               GetTickCount64()>target.reaction_deadline_ms) continue;
            if(target.reaction_blocked) {
                target.guard_pending=true; target.guard_pending_ms=GetTickCount64();
                target.guard_relation=target.reaction_relation;
            } else {
                target.hit_pending=true; target.hit_pending_ms=GetTickCount64();
                target.hit_relation=target.reaction_relation;
                target.hit_pending_sound_handled=true;
            }
            // A blocked result never creates HIT; a different incoming attack
            // may still have queued HIT. Motion arbitration cannot alter either outcome.
            contacts[count++]=slot;
        }
    }
    return count;
}
