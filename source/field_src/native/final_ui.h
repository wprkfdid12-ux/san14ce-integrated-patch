#pragma once
struct InfoPoint { int x,y; };
struct InfoPadding {
    void *layout,*background,*numbers;
    InfoPoint original_position,original_size;
    InfoPoint last_number_position,last_number_size;
    unsigned last_troops;
    uint32_t number_dimensions,background_dimensions;
    float number_x,number_y;
};
static InfoPadding info_padding[1024];
static unsigned compact_info_changes;
static InfoPadding *info_padding_entry(void *layout,bool create) {
    unsigned start=index_start((uintptr_t)layout),empty=1024;
    for(unsigned n=0;n<1024;++n) {
        unsigned i=(start+n)&1023;
        if(info_padding[i].layout==layout) return &info_padding[i];
        if(info_padding[i].layout==(void *)1 && empty==1024) empty=i;
        if(!info_padding[i].layout) { if(empty==1024) empty=i; break; }
    }
    if(!create || empty==1024) return NULL;
    info_padding[empty]={}; info_padding[empty].layout=layout; return &info_padding[empty];
}
static void *info_element(void *normal,int id) {
    if(!normal || at<uintptr_t>(normal,0)!=base+0x13981a0) return NULL;
    void *table=at<void *>(normal,0x138);
    int count=at<int>(normal,0x140);
    return table && count>id && count<=256 ? at<void *>(table,size_t(id)*8):NULL;
}
static void restore_info_padding(void *layout,bool remove=false) {
    InfoPadding *cache=info_padding_entry(layout,false);
    if(!cache) return;
    void *normal=at<void *>(layout,0x260);
    // Never dereference a cached, possibly replaced CUIObject. Resolve it again
    // from this live layout; deletion invalidates the weak entry below.
    void *bg=info_element(normal,0x31);
    if(bg && bg==cache->background) {
        fn<void (*)(void *,int,int)>(FL_UI_ELEMENT_SIZE)(bg,cache->original_size.x,cache->original_size.y);
        fn<void (*)(void *,int,int)>(FL_UI_ELEMENT_POSITION)(bg,cache->original_position.x,cache->original_position.y);
    }
    if(remove || cache->background) { *cache={}; cache->layout=(void *)1; }
}
static bool info_modified_unit(void *layout) {
    if(!presentation_owner_live() || at<uintptr_t>(layout,0)!=base+FL_INFO_VTABLE) return false;
    void *army=at<void *>(layout,0x1f0);
    Proxy *p=bound_army(army);
    if(p && p->ready && p->submitted && current_source(*p)) return true;
    // Siege models/motions/effects/sounds stay native; only their badge is shared.
    if(!army || !at<uint8_t>(army,0x10)) return false;
    void *actor=at<void *>(army,0x148);
    return actor && at<uintptr_t>(actor,0)==base+FL_ARMY_VTABLE &&
        at<void *>(actor,0x40)==army && at<unsigned>(actor,0x740)==2 && at<unsigned>(actor,0x748)!=2;
}
static void compact_info_padding(void *layout,bool enabled) {
    if(!enabled) { restore_info_padding(layout); return; }
    void *normal=at<void *>(layout,0x260);
    void *bg=info_element(normal,0x31),*numbers=info_element(normal,0x32);
    if(!bg || !numbers || !at<void *>(bg,0xe8) || !at<void *>(numbers,0xe8)) return;
    InfoPadding *cache=info_padding_entry(layout,true);
    if(!cache) return;
    void *army=at<void *>(layout,0x1f0),*transform=at<void *>(numbers,0xe8);
    unsigned troops=army?at<uint16_t>(army,0x16):0;
    uint32_t nd=at<uint32_t>(numbers,0xf0),bd=at<uint32_t>(bg,0xf0);
    float nx=at<float>(transform,0x30),ny=at<float>(transform,0x34);
    if(cache->background==bg && cache->numbers==numbers && cache->last_troops==troops &&
       cache->number_dimensions==nd && cache->background_dimensions==bd &&
       cache->number_x==nx && cache->number_y==ny) return;
    // 1a6650 writes the troop count to element32. Element31 is its background.
    // Fit the background to the native text bounds; do not scale or rewrite text.
    InfoPoint np={},ns={},bp={},bs={};
    void *repo=(unsigned char *)normal+0x130;
    typedef InfoPoint *(*Query)(void *,InfoPoint *,int);
    fn<Query>(FL_UI_POSITION)(repo,&np,0x32); fn<Query>(FL_UI_SIZE)(repo,&ns,0x32);
    fn<Query>(FL_UI_POSITION)(repo,&bp,0x31); fn<Query>(FL_UI_SIZE)(repo,&bs,0x31);
    if(ns.x<1 || ns.x>512 || ns.y<1 || ns.y>128 || bs.x<1 || bs.x>1024 || bs.y<1 || bs.y>256) return;
    if(cache->background!=bg || cache->numbers!=numbers) {
        *cache={}; cache->layout=layout; cache->background=bg; cache->numbers=numbers;
        cache->original_position=bp; cache->original_size=bs;
    }
    // Symmetric padding puts the text bounds at the exact box centre. Never
    // shrink the font, portrait, morale gauge, trait dots or formation icon.
    int width=ns.x+4,height=ns.y+2;
    if(width>cache->original_size.x || height>cache->original_size.y) {
        cache->last_troops=troops;cache->number_dimensions=nd;cache->background_dimensions=bd;
        cache->number_x=nx;cache->number_y=ny;return;
    }
    fn<void (*)(void *,int,int)>(FL_UI_ELEMENT_SIZE)(bg,width,height);
    fn<void (*)(void *,int,int)>(FL_UI_ELEMENT_POSITION)(bg,np.x-2,np.y-1);
    cache->last_number_position=np; cache->last_number_size=ns;
    cache->last_troops=troops; cache->number_dimensions=nd;
    cache->background_dimensions=at<uint32_t>(bg,0xf0); cache->number_x=nx; cache->number_y=ny;
    ++compact_info_changes;
}
extern "C" __declspec(dllexport) void *FL_HookInfoDestroy(void *layout,unsigned flags) {
    lock_status();
    restore_info_padding(layout,true);
    unlock_status();
    // Retain the verified integrated patch's own UI-cache cleanup before its
    // tailcall to the native destructor. Never bypass it or call it twice.
    if(info_destroy_next)return reinterpret_cast<void *(*)(void *,unsigned)>(info_destroy_next)(layout,flags);
    return fn<void *(*)(void *,unsigned)>(FL_INFO_DESTROY)(layout,flags);
}
