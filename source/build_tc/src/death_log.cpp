#include "patch27.h"
#include "module_hooks.h"
// Exact native history content, never pointers or a new bit in a game save.
// R5 (turn-progress delay): the journal file stays open; each death appends one record with a single
// WriteFile into the OS cache. The per-death open / FlushFileBuffers (a real disk sync, tens of ms on a
// hard disk) / close are gone. Content and format (S14DLOG1 v1, 304-byte receipts) are unchanged.
// Title/body readability is checked once per 4 KiB page instead of once per character.
struct LogKey{U16 date;U8 district,kind;U16 cell,speaker;wchar_t title[55],body[85];};
struct Receipt{uint64_t sequence;LogKey key;U32 crc,reserved;};
struct Header{char magic[8];U32 version,entry_size,reserved[2];};
static_assert(sizeof(LogKey)==288&&sizeof(Receipt)==304&&sizeof(Header)==24,"history format");
static Receipt records[8192];static int heads[16384],links[8192];static uint64_t sequence;
static bool ready,writable=true,attached,attach_failed,warned;static wchar_t path[1024];
static SRWLOCK lock=SRWLOCK_INIT;static DWORD tls=TLS_OUT_OF_INDEXES;static U8(*old_filter)(U8*);
static HANDLE journal=INVALID_HANDLE_VALUE;   // R5: opened once under `lock`, never per death
// R4: the fog log-feed filter we chain runs every frame for every visible history row.
// A 64 Kbit set of short keys (date, district, kind, cell, speaker) lets ordinary rows
// go straight to the fog filter without reading their text or asking the OS anything.
static volatile U8 quick_bits[8192];static volatile LONG pending_count;
static unsigned quick_hash(U16 date,U8 district,U8 kind,U16 cell,U16 speaker){
 U32 h=date*0x9e37u^(district*0x85ebu)^(kind*0xc2b2u)^(cell*0x27d4u)^(speaker*0x1656u);h^=h>>15;h*=0x2c1b3c6du;h^=h>>12;return h&0xffff;
}
static void quick_add(const struct LogKey&k);
static bool quick_maybe(U8*r){
 return quick_bits[quick_hash(at<U16>(r,0x10),at<U8>(r,0x12),at<U8>(r,0x86),at<U16>(r,0x84),at<U16>(r,0x88))>>3]&(1u<<(quick_hash(at<U16>(r,0x10),at<U8>(r,0x12),at<U8>(r,0x86),at<U16>(r,0x84),at<U16>(r,0x88))&7));
}
struct Pending{LogKey key;Pending*previous;bool active;Pending(const LogKey&k):key(k),previous((Pending*)TlsGetValue(tls)),active(TlsSetValue(tls,this)!=0){}~Pending(){if(active)TlsSetValue(tls,previous);}};
extern "C" int DeathScopeActive();
static U32 crc32(const void*p,size_t n){U32 c=~0u;auto*b=(const U8*)p;for(size_t i=0;i<n;i++){c^=b[i];for(int j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return ~c;}
static unsigned bucket(const LogKey&k){return crc32(&k,sizeof(k))&16383;}
static bool same(const LogKey&a,const LogKey&b){return !memcmp(&a,&b,sizeof(a));}
static bool contains(const LogKey&k){for(int i=heads[bucket(k)];i>=0;i=links[i])if(same(records[i].key,k))return true;return false;}
static void quick_add(const LogKey&k){unsigned h=quick_hash(k.date,k.district,k.kind,k.cell,k.speaker);__atomic_or_fetch(&quick_bits[h>>3],U8(1u<<(h&7)),__ATOMIC_RELEASE);}
static void index(unsigned slot){unsigned b=bucket(records[slot].key);links[slot]=heads[b];heads[b]=int(slot);quick_add(records[slot].key);}
static void unindex(unsigned slot){if(!records[slot].sequence)return;int*p=&heads[bucket(records[slot].key)];while(*p>=0){if(unsigned(*p)==slot){*p=links[slot];return;}p=&links[*p];}}
static unsigned clamp(int v,unsigned hi){return v<0?0:unsigned(v)>hi?hi:unsigned(v);}
template<size_t N>static bool copy_text(wchar_t(&out)[N],const wchar_t*in){
 // R5: one readability query per 4 KiB page touched (a character may straddle two pages).
 bool end=false;UPTR checked=0;
 for(size_t i=0;i<N;i++){
  if(!end){UPTR a=UPTR(in+i),last=(a+1)>>12;if(!checked||last!=checked-1){if(!core27->Readable(in+i,2))return false;checked=last+1;}
   out[i]=in[i];if(!out[i])end=true;}
  else out[i]=0;
 }
 return true;
}
template<size_t N>static void copy_inside(wchar_t(&out)[N],const wchar_t*in){bool end=false;for(size_t i=0;i<N;i++){if(!end){out[i]=in[i];if(!out[i])end=true;}else out[i]=0;}}
static bool from_row(U8*r,LogKey&k){
 if(!core27->Readable(r,0x140)||at<U8*>(r,0)!=game27+0x13a75b8)return false;
 k={};k.date=at<U16>(r,0x10);k.district=at<U8>(r,0x12);k.kind=at<U8>(r,0x86);k.cell=at<U16>(r,0x84);k.speaker=at<U16>(r,0x88);
 // Title (0x14..0x82) and body (0x8a..0x134) lie inside the row checked above.
 copy_inside(k.title,(const wchar_t*)(r+0x14));copy_inside(k.body,(const wchar_t*)(r+0x8a));return true;
}
static bool io(HANDLE f,void*p,DWORD size,bool writing){DWORD n=0;return (writing?WriteFile(f,p,size,&n,nullptr):ReadFile(f,p,size,&n,nullptr))&&n==size;}
extern "C" bool DeathLogsInit(){
 if(ready)return true;DWORD n=GetModuleFileNameW(nullptr,path,1024);if(!n||n>=950)return false;while(n&&path[n-1]!=L'\\'&&path[n-1]!=L'/')n--;if(!n)return false;wcscpy(path+n,L"SAN14Mods\\Patch27\\DeathLogs.bin");
 tls=TlsAlloc();if(tls==TLS_OUT_OF_INDEXES)return false;for(auto&h:heads)h=-1;for(auto&l:links)l=-1;ready=true;
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return true;
 LARGE_INTEGER size{};Header h{};
 if(!GetFileSizeEx(f,&size)||size.QuadPart<24||size.QuadPart>24+8192*304||!io(f,&h,24,false)||memcmp(h.magic,"S14DLOG1",8)||h.version!=1||h.entry_size!=304||h.reserved[0]||h.reserved[1]){writable=false;CloseHandle(f);return true;}
 if((size.QuadPart-24)%304)writable=false;
 for(unsigned i=0;i<unsigned((size.QuadPart-24)/304);i++){Receipt r{};if(!io(f,&r,sizeof(r),false)){writable=false;break;}
  if(r.sequence&&r.sequence<UINT64_MAX&&r.reserved==0&&r.crc==crc32(&r,296)){records[i]=r;index(i);if(r.sequence>sequence)sequence=r.sequence;}
 }
 CloseHandle(f);return true;
}
// Under `lock`. Opens the journal once; writes the header when the file is new.
static bool journal_open(){
 if(journal!=INVALID_HANDLE_VALUE)return true;
 HANDLE f=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,0,nullptr);if(f==INVALID_HANDLE_VALUE)return false;
 LARGE_INTEGER size{};bool ok=GetFileSizeEx(f,&size)!=0;
 if(ok&&!size.QuadPart){Header h{{'S','1','4','D','L','O','G','1'},1,304,{0,0}};LARGE_INTEGER zero{};ok=SetFilePointerEx(f,zero,nullptr,FILE_BEGIN)&&io(f,&h,24,true);}
 if(!ok){CloseHandle(f);return false;}
 journal=f;return true;
}
static void remember(const LogKey&key){
 AcquireSRWLockExclusive(&lock);if(contains(key)){ReleaseSRWLockExclusive(&lock);return;}
 if(sequence==UINT64_MAX-1){ReleaseSRWLockExclusive(&lock);return;}
 uint64_t next=++sequence;unsigned slot=unsigned((next-1)%8192);unindex(slot);Receipt r{next,key,0,0};r.crc=crc32(&r,296);records[slot]=r;index(slot);
 bool ok=writable&&journal_open();
 if(ok){LARGE_INTEGER pos{};pos.QuadPart=24+LONGLONG(slot)*304;
  ok=SetFilePointerEx(journal,pos,nullptr,FILE_BEGIN)&&io(journal,&r,sizeof(r),true);   // R5: OS-cached write, no per-death disk sync
  if(!ok){CloseHandle(journal);journal=INVALID_HANDLE_VALUE;}
 }
 if(!ok){writable=false;if(!warned&&core27->Log){warned=true;core27->Log("Patch27: death history journal unavailable; current-session history remains visible.");}}
 ReleaseSRWLockExclusive(&lock);
}
extern "C" __declspec(dllexport) U8 DeathHistoryVisible(U8*row){
 // Fast path: the fog engine passes live history rows; it reads them directly as well.
 if(!ready||!row||(UPTR(row)&7)||at<U8*>(row,0)!=game27+0x13a75b8||(!pending_count&&!quick_maybe(row)))
  return old_filter?old_filter(row):0;
 LogKey key{};if(from_row(row,key)){
  for(auto*p=(Pending*)TlsGetValue(tls);p;p=p->previous)if(same(p->key,key))return 1;
  AcquireSRWLockShared(&lock);bool known=contains(key);ReleaseSRWLockShared(&lock);if(known)return 1;
 }
 return old_filter?old_filter(row):0;
}
extern "C" void DeathLogsAttach(){
 if(attached||attach_failed||!ready)return;HMODULE fog=GetModuleHandleW(L"SAN14_FogEngine121.dll");if(!fog)return;
 static const U8 base[]={0xb0,0xe,0x46,0x49,0x7e,0xd1,0x6d,0x89,0xa2,0xc2,0x10,0x31,0x15,0x22,0x57,0xe9,0xa7,0x12,0xc1,0x4,0x8b,0xc8,0x58,0x9b,0x1d,0xcb,0x81,0xbb,0xb3,0xfc,0x81,0x0};
 static const U8 fixed[]={0x60,0x37,0x35,0xd2,0xaf,0xc3,0x9e,0x26,0x04,0x6e,0x65,0x81,0xc6,0xc9,0x69,0x0a,0x9f,0x96,0x7f,0xd3,0xfe,0xe4,0x10,0x55,0xe4,0x68,0xfc,0xe2,0x64,0x88,0xb3,0xb8};
 // Rollback fog + the four-byte weather collector correction only.
 if(!module_sha(fog,base)&&!module_sha(fog,fixed)){attach_failed=true;if(core27->Log)core27->Log("Patch27: unknown fog module; visibility hook not overwritten.");return;}
 const U8 expected[]={0x56,0x57,0x53,0x48,0x83,0xec,0x20};ModuleTransaction tx;old_filter=(U8(*)(U8*))tx.detour((U8*)fog+0x32040,expected,7,(void*)DeathHistoryVisible);
 attached=tx.install();if(!attached){old_filter=nullptr;attach_failed=true;}
}
extern "C" __declspec(dllexport) U8* DeathHistory(void*manager,int kind,int district,int cell,int flag,int date,const wchar_t*title,const wchar_t*body,int speaker,int a10,int a11,int a12,int a13){
 using Native=U8*(*)(void*,int,int,int,int,int,const wchar_t*,const wchar_t*,int,int,int,int,int);
 auto native=fn<Native>(game27,0x9338c0);LogKey key{};
 bool track=ready&&DeathScopeActive();key.date=U16(clamp(date,65535));key.kind=U8(clamp(kind,6));key.district=U8(clamp(district,51));key.cell=U16(clamp(cell,0xbd0f));key.speaker=U16(speaker<1?1:clamp(speaker,6000));
 if(track)track=copy_text(key.title,title)&&copy_text(key.body,body);
 if(!track)return native(manager,kind,district,cell,flag,date,title,body,speaker,a10,a11,a12,a13);
 DeathLogsAttach();quick_add(key);__atomic_add_fetch(&pending_count,1,__ATOMIC_SEQ_CST);Pending pending(key);
 U8*row=native(manager,kind,district,cell,flag,date,title,body,speaker,a10,a11,a12,a13);
 LogKey actual{};if(pending.active&&from_row(row,actual)&&same(actual,key))remember(actual);
 __atomic_sub_fetch(&pending_count,1,__ATOMIC_SEQ_CST);
 return row;
}
extern "C" bool DeathLogsPlan(HookPlan*p){if(!DeathLogsInit())return false;p->call(0x2955f2,0x9338c0,(void*)DeathHistory);p->call(0x295853,0x9338c0,(void*)DeathHistory);return true;}
