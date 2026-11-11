#pragma once
// One immutable address map per owner draw pass. Mutable lifetime checks and
// deferred/replay decisions still use state_lock; no lock-free native pointers.
struct DrawLookupEntry { uintptr_t object; uint32_t generation; unsigned slot, part; };
static thread_local DrawLookupEntry draw_lookup[8192];
static thread_local uint32_t draw_lookup_generation;
static thread_local bool draw_lookup_armed;
static constexpr unsigned draw_offsets[]={0x750,0x778,0x7c8,0x7f0,0x580,0x7a0,0x840};
static unsigned draw_lookup_hash(uintptr_t object) {
    return unsigned(((object>>3)^(object>>17))*2654435761u)&8191;
}
static void prepare_draw_lookup() {
    if(!++draw_lookup_generation) {
        memset(draw_lookup,0,sizeof draw_lookup); ++draw_lookup_generation;
    }
    for(unsigned n=0;n<active_count;++n) {
        unsigned slot=active_slots[n]; const Proxy &p=proxies[slot];
        if(!p.constructed) continue;
        for(unsigned part=0;part<7;++part) {
            uintptr_t object=(uintptr_t)p.source.actor+draw_offsets[part];
            unsigned h=draw_lookup_hash(object);
            while(draw_lookup[h].generation==draw_lookup_generation) h=(h+1)&8191;
            draw_lookup[h]={object,draw_lookup_generation,slot,part};
        }
    }
    draw_lookup_armed=true;
}
static const DrawLookupEntry *find_draw_lookup(void *object,bool action) {
    unsigned h=draw_lookup_hash((uintptr_t)object);
    for(unsigned n=0;n<8192;++n,h=(h+1)&8191) {
        const auto &r=draw_lookup[h];
        if(r.generation!=draw_lookup_generation) return NULL;
        if(r.object==(uintptr_t)object) return action==(r.part==4)?&r:NULL;
    }
    return NULL;
}
