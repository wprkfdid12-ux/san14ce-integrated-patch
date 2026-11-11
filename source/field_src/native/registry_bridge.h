#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <vector>
// Read-only adapter for the exact 20260924 R2 integrated registry. No writes to
// its mappings, vtables, controller or game resources. All remote reads execute
// in the existing external helper, never on the game's render thread.
namespace fl_registry {
constexpr unsigned model_count=174, officer_count=6001; // R35: 0~173 (25~173 확장 모델)
constexpr uint32_t update_slot=0x13a05e0, create_slot=0x13a0608;
constexpr uint32_t update_hook=0x6d460, create_hook=0x3b910;
constexpr uint32_t info_slot=0x1398a58,info_hook=0x4940;
template<class Read> uintptr_t info_cleanup(Read read,uintptr_t game,uintptr_t module) {
    uintptr_t owner=0,target=0;
    static const unsigned char code[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x8b,0xda,0x48,0x8b,0xf9,0xe8,0xdc,0x33,0x03,0x00,0x48,0x8b,0x05,0x05,0xca,0x1b,0x00,0x8b,0xd3,0x48,0x05,0x20,0xb0,0x19,0x00,0x48,0x8b,0xcf,0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f,0x48,0xff,0xe0};
    unsigned char actual[sizeof code];
    if(!module||!read(module+0x1c1360,&owner,8)||owner!=game||!read(game+info_slot,&target,8)||target!=module+info_hook||
       !read(target,actual,sizeof actual)||memcmp(code,actual,sizeof code))return 0;
    return target;
}
static const unsigned char module_sha256[32]={0xd3,0xb0,0x88,0x6a,0x6c,0x89,0xaf,0xb5,0x3f,0x0c,0xf2,0x83,0xde,0xee,0x93,0xac,0x78,0x85,0xdd,0x9c,0xc7,0x36,0x4e,0xbb,0x99,0xd0,0x98,0x2d,0x61,0x3f,0x0a,0x08};
static const unsigned char module_sha256_r8[32]={0x07,0x45,0xf8,0xa3,0x4b,0x9e,0x90,0xdf,0xbd,0xa0,0xe6,0x44,0xc4,0x74,0x4e,0x01,0x5a,0x7c,0x6b,0x18,0xd9,0xd3,0x0f,0x55,0xe2,0x65,0xba,0x62,0x81,0x4c,0x69,0x51};
// P1 dialog validation repair: registry layout and hooks are unchanged.
static const unsigned char module_sha256_p1[32]={0xfa,0x07,0xcd,0x5f,0xdf,0x5c,0x26,0xab,0x6d,0xec,0xce,0xb9,0x33,0xf6,0x25,0x6a,0x73,0xd2,0xd0,0xfc,0xf0,0x5e,0x95,0x10,0x2f,0x82,0x81,0x2f,0x34,0x15,0xda,0x0d};
// CE 1.0.1 port IC
static const unsigned char module_sha256_ce101[32]={0x13,0xd1,0x6c,0x04,0xb6,0x46,0x19,0x97,0x30,0x7d,0x80,0x32,0xa7,0x10,0xc5,0x9e,0xe0,0xb0,0xd1,0x6e,0xb2,0x73,0xb2,0x8a,0x8c,0x20,0x87,0x6a,0x86,0x85,0xcd,0x42};
// R39 (ce101 + ruler-at-gate home-city cave at 0x208470)
static const unsigned char module_sha256_r39[32]={0xbb,0x68,0xbe,0x42,0xa4,0xdb,0x9b,0x37,0x33,0xe5,0xbc,0x98,0xba,0x2f,0xa8,0x1e,0x62,0x49,0x64,0x41,0x58,0x2c,0x92,0x16,0x7a,0x2c,0x59,0xd1,0x58,0x7f,0xaa,0xde};
// R40 (r39 + AI mod compat: global check accepts other mods' entry detours, cave 0x2084e0). Missing it turned the 3D registry bridge off (A19).
static const unsigned char module_sha256_r40[32]={0xae,0x36,0x5e,0xab,0x73,0x19,0x5e,0x23,0xcb,0x0e,0x3c,0xc3,0x86,0x1c,0x2b,0x8b,0x9c,0x90,0x97,0x50,0x5c,0xbf,0xb8,0x46,0x88,0x1e,0xc6,0xfb,0xbb,0x49,0x83,0xb7};
inline bool module_known(const unsigned char hash[32]) {return !memcmp(hash,module_sha256_r40,32)||!memcmp(hash,module_sha256_r39,32)||!memcmp(hash,module_sha256_ce101,32)||!memcmp(hash,module_sha256,32)||!memcmp(hash,module_sha256_r8,32)||!memcmp(hash,module_sha256_p1,32);}

struct Row { uint64_t person; int32_t model; unsigned char identity[28]; };
static_assert(sizeof(Row)==40,"fixed remote row ABI");
struct Snapshot { uint32_t active; unsigned char allowed[model_count]; unsigned char padding[6]; Row rows[officer_count]; };
static_assert(offsetof(Snapshot,rows)==184,"fixed remote snapshot ABI (R35 174 models)");
struct Mailbox { uint32_t sequence,padding; Snapshot snapshot; };
struct Lea { uint32_t site,offset; unsigned char opcode[3]; };
static const Lea leas[]={{0x1c5528,0,{0x48,0x8d,0x0d}},{0x1cb67b,0x40,{0x48,0x8d,0x15}},{0x376d02,0x40,{0x48,0x8d,0x05}}};
template<class Read> bool installed(Read read,uintptr_t game,uintptr_t module,uintptr_t &table) {
    uint8_t flag=0;uintptr_t owner=0,update=0,create=0;
    if(!module || !read(module+0x1c1f8d,&flag,1) || flag!=1 ||
       !read(module+0x1ec360,&owner,8) || owner!=game ||
       !read(module+0x1eca38,&table,8) || !table ||
       !read(game+update_slot,&update,8) || update!=module+update_hook ||
       !read(game+create_slot,&create,8) || create!=module+create_hook) return false;
    for(const Lea &l:leas) {
        unsigned char code[7];int32_t displacement;
        if(!read(game+l.site,code,7) || memcmp(code,l.opcode,3))return false;
        memcpy(&displacement,code+3,4);
        if(game+l.site+7+intptr_t(displacement)!=table+l.offset)return false;
    }
    // Original rows are retained byte for byte in the expanded table.
    unsigned char original[25*68],expanded[25*68];
    return read(game+0x1a3e290,original,sizeof original) && read(table,expanded,sizeof expanded) &&
           !memcmp(original,expanded,sizeof original);
}
inline bool code_matches(uintptr_t game,uintptr_t table,uint32_t rva,const unsigned char *expected,const unsigned char *actual,unsigned size) {
    for(unsigned i=0;i<size;++i) {
        unsigned char value=expected[i];
        for(const Lea &l:leas) if(table && rva+i>=l.site+3 && rva+i<l.site+7) {
            int64_t delta=int64_t(table+l.offset)-int64_t(game+l.site+7);
            if(delta<INT32_MIN || delta>INT32_MAX)return false;
            uint32_t bits=uint32_t(int32_t(delta)); value=uint8_t(bits>>(8*(rva+i-l.site-3)));
        }
        if(actual[i]!=value)return false;
    }
    return true;
}
struct String { uint64_t storage[2],length,capacity; };
struct Record { int32_t id,model; String name; };
static_assert(sizeof(Record)==40,"MSVC registry record ABI");
template<class Read> bool string_read(Read read,const String &s,uint16_t out[65]) {
    memset(out,0,130);
    if(s.length>64 || s.capacity<s.length || (s.capacity<=7 && s.length>7))return false;
    if(s.capacity<=7)memcpy(out,s.storage,size_t(s.length)*2);
    else if(!s.storage[0] || !read(s.storage[0],out,size_t(s.length)*2))return false;
    return true;
}
template<class Read> bool records(Read read,uintptr_t address,std::vector<Record> &out,uintptr_t bounds[2]) {
    if(!read(address,bounds,16) || bounds[1]<bounds[0] || (bounds[1]-bounds[0])%40 ||
       (bounds[1]-bounds[0])/40>officer_count || (!bounds[0] && bounds[1]))return false;
    out.resize((bounds[1]-bounds[0])/40);
    return out.empty() || read(bounds[0],out.data(),out.size()*40);
}
template<class Read> bool current_name(Read read,uintptr_t module,unsigned id,const unsigned char raw[28],uint16_t out[65]) {
    memset(out,0,130);
    if((id>=3001&&id<=4000)||(id>=5101&&id<=5400)) {
        String s={};return read(module+0x16e5c0+id*32,&s,32)&&string_read(read,s,out);
    }
    unsigned n=0;
    for(unsigned part=0;part<2;++part)for(unsigned j=0;j<7;++j) {
        uint16_t c;memcpy(&c,raw+part*14+j*2,2);if(!c)break;out[n++]=c;
    }
    bool nonspace=false,valid=n!=0;
    for(unsigned j=0;j<n;++j){if(out[j]<32||out[j]==127)valid=false;if(out[j]!=32)nonspace=true;}
    if(n==3 && ((out[0]==45&&out[1]==45&&out[2]==45)||(out[0]==0xff0d&&out[1]==0xff0d&&out[2]==0xff0d)))valid=false;
    if(!valid||!nonspace)memset(out,0,130);
    return true;
}
template<class Read> bool collect(Read read,uintptr_t game,uintptr_t module,uintptr_t table,uint32_t data_rva,uint32_t person_vtable,Snapshot &out) {
    memset(&out,0,sizeof out);out.active=1;
    for(unsigned i=0;i<25;++i)out.allowed[i]=1;
    std::vector<Record> catalog,bindings,check;uintptr_t cb[2],bb[2],after[2];
    if(!records(read,module+0x1ec330,catalog,cb)||!records(read,module+0x1ec348,bindings,bb))return false;
    for(const Record &r:catalog)if(r.id>=25&&r.id<int(model_count)) {
        char resource[64];if(!read(table+unsigned(r.id)*68,resource,64))return false;
        if(resource[0] && memchr(resource,0,64))out.allowed[r.id]=1;
    }
    uintptr_t data=0;if(!read(game+data_rva,&data,8)||!data)return false;
    for(const Record &r:bindings) {
        if(r.id<0||r.id>=int(officer_count)||r.model<-1||r.model>=int(model_count)||(r.model>=0&&!out.allowed[r.model])||out.rows[r.id].person)continue;
        uint16_t saved[65],name[65];uintptr_t person=0,vt=0;uint16_t id=0;unsigned char raw[28];
        if(!string_read(read,r.name,saved))return false;
        if(!read(data+0x158+unsigned(r.id)*8,&person,8)||!person||!read(person,&vt,8)||vt!=game+person_vtable||
           !read(person+0x10,&id,2)||id!=r.id||!read(person+0x12,raw,28))continue;
        if(!current_name(read,module,id,raw,name))return false;
        if(memcmp(name,saved,sizeof name))continue;
        // Re-read name storage and current identity before committing a row.
        uint16_t saved2[65];unsigned char raw2[28];
        if(!string_read(read,r.name,saved2)||memcmp(saved,saved2,sizeof saved)||!read(person+0x12,raw2,28)||memcmp(raw,raw2,28))return false;
        Row &row=out.rows[r.id];row.person=person;row.model=r.model;memcpy(row.identity,raw,28);
    }
    // Vector reallocations/in-place edits are never observed as a mixed snapshot.
    if(!records(read,module+0x1ec330,check,after)||memcmp(cb,after,16)||check.size()!=catalog.size()||
       (!check.empty()&&memcmp(check.data(),catalog.data(),check.size()*40)))return false;
    if(!records(read,module+0x1ec348,check,after)||memcmp(bb,after,16)||check.size()!=bindings.size()||
       (!check.empty()&&memcmp(check.data(),bindings.data(),check.size()*40)))return false;
    return true;
}
inline bool appearance_valid(int body,int horse,int weapon,int color,int horse_color,bool extended) {
    return body>=0 && body<(extended?int(model_count):25) && horse==0 && weapon>=0 && weapon<=5 &&
           color>=-1 && color<=4 && horse_color>=-1 && horse_color<=2;
}
inline void override_model(int model,int &body,int &color) {
    body=model;
    color=((model>=11&&model<=16)||(model>=22&&model<=24)) ? (color>=0&&color<=4?color:0) : -1;
}
}
