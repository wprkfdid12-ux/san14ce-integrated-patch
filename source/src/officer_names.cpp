#include "patch27.h"
#include "module_hooks.h"
// The native 0x4a0 record remains byte-for-byte the same size. Immutable names
// are referenced by a positive version tag; phonetics/courtesy/biography stay native.
struct NamePair{wchar_t surname[5],given[5];};
struct DiskName{NamePair pair;U32 crc;};
struct NameHeader{char magic[8];U32 version,entry_size,reserved[2];};
static_assert(sizeof(NamePair)==20&&sizeof(DiskName)==24&&sizeof(NameHeader)==24,"wire size");
static NamePair dictionary[16384];static bool usable[16384];static volatile LONG entries;static bool writable;
static wchar_t path[1024];static SRWLOCK lock=SRWLOCK_INIT;static DWORD tls=TLS_OUT_OF_INDEXES;
static void*trampolines[5];static unsigned allocated;
using Setter=void(*)(U8*,const wchar_t*);using Build=UPTR(*)(U8*,void*,UPTR,UPTR);using Result=void(*)(U8*,U8*);using Commit=void(*)(U8*);
static Setter native_surname,native_given;static Build native_build;static Result native_result;static Commit native_commit;
struct Edit{U8*owner;U8*record;NamePair pair;struct {wchar_t*text;U32 capacity;} input;};
static Edit edits[8];static unsigned next_edit;
struct NameScope{bool building;Edit*committing;NameScope*previous;bool active;
 NameScope(bool b,Edit*e):building(b),committing(e),previous((NameScope*)TlsGetValue(tls)),active(TlsSetValue(tls,this)!=0){}
 ~NameScope(){if(active)TlsSetValue(tls,previous);}
};
static U32 crc32(const void*ptr,size_t n){U32 crc=~0u;const U8*b=(const U8*)ptr;for(size_t i=0;i<n;i++){crc^=b[i];for(int j=0;j<8;j++)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}return ~crc;}
static unsigned length(const wchar_t*s){if(!s)return 5;for(unsigned i=0;i<=4;i++){if(!core27->Readable(s+i,2))return 5;if(!s[i])return i;}return 5;}
static bool editor(U8*r){return core27->Readable(r,0x4a0)&&(at<U8*>(r,0)==game27+0x13a7260||at<U8*>(r,0)==game27+0x13a7298);}
// R3: name getters run for every officer name the game reads. Compare the object's
// first word with the two editor vtables before asking the OS about the memory, so
// ordinary objects no longer pay a system call per name.
static bool editor_vtable(U8*v){return v==game27+0x13a7260||v==game27+0x13a7298;}
static bool fast_editor_object(U8*r){ // r is a live object passed to a member getter
 if(UPTR(r)<0x10000||(UPTR(r)&7))return false;
 return editor_vtable(at<U8*>(r,0))&&core27->Readable(r,0x4a0);
}
static bool fast_editor_before(U8*r,const void*text){ // r precedes a string the game is reading
 if(UPTR(r)<0x10000||(UPTR(r)&7))return false;
 if((UPTR(r)>>12)!=(UPTR(text)>>12)&&!core27->Readable(r,8))return false;
 return editor_vtable(at<U8*>(r,0))&&core27->Readable(r,0x4a0);
}
static void prefix(wchar_t*out,const wchar_t*in){out[0]=in[0];out[1]=in[0]?in[1]:0;out[2]=0;}
static bool prefixes(U8*r,const NamePair&p){wchar_t s[3],g[3];prefix(s,p.surname);prefix(g,p.given);return !memcmp(r+0xc,s,6)&&!memcmp(r+0x12,g,6);}
static const NamePair*full_known(U8*r){
 unsigned tag=at<U16>(r,0xa);if(tag<0x4000||tag>0x7fff)return nullptr;unsigned n=tag-0x4000;
 if(n>=U32(entries)||!usable[n]||!prefixes(r,dictionary[n]))return nullptr;return dictionary+n;
}
static const NamePair*full(U8*r){return editor(r)?full_known(r):nullptr;}
static const NamePair*full_fast(U8*r){return fast_editor_object(r)?full_known(r):nullptr;}
static const wchar_t*resolve(const wchar_t*raw){
 U8*p=(U8*)raw;
 for(int field=0;field<2;field++){U8*r=p-(field?0x12:0xc);if(fast_editor_before(r,raw))if(const NamePair*n=full_known(r))return field?n->given:n->surname;}
 return raw;
}
static NamePair current(U8*r){NamePair p{};if(const NamePair*n=full(r))return *n;memcpy(p.surname,r+0xc,6);memcpy(p.given,r+0x12,6);return p;}
static bool exact_io(HANDLE f,void*p,DWORD size,bool write){DWORD n=0;return (write?WriteFile(f,p,size,&n,nullptr):ReadFile(f,p,size,&n,nullptr))&&n==size;}
static bool file_name(){DWORD n=GetModuleFileNameW(nullptr,path,1024);if(!n||n>=950)return false;while(n&&path[n-1]!=L'\\'&&path[n-1]!=L'/')n--;if(!n)return false;wcscpy(path+n,L"SAN14Mods\\Patch27\\OfficerNames.bin");return true;}
extern "C" bool NamesInit(){
 if(!file_name())return false;tls=TlsAlloc();if(tls==TLS_OUT_OF_INDEXES)return false;
 writable=true;HANDLE f=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 if(f==INVALID_HANDLE_VALUE)f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 if(f==INVALID_HANDLE_VALUE)return true;
 LARGE_INTEGER size{};NameHeader h{};
 if(!GetFileSizeEx(f,&size)||size.QuadPart<24||!exact_io(f,&h,24,false)||memcmp(h.magic,"S14NAME1",8)||h.version!=1||h.entry_size!=24||h.reserved[0]||h.reserved[1]){
  // R40 A15: unreadable journal no longer freezes 4-character names forever. Keep it aside and start a new one.
  // Old saves' tags are prefix-checked against the dictionary, so a fresh dictionary never shows a wrong name.
  CloseHandle(f);wchar_t bad[1100];wcscpy(bad,path);{size_t n=0;while(bad[n])n++;bad[n]=L'.';bad[n+1]=L'b';bad[n+2]=L'a';bad[n+3]=L'd';bad[n+4]=0;}MoveFileExW(path,bad,MOVEFILE_REPLACE_EXISTING);
  if(core27->Log)core27->Log("Patch27 R40 names: unreadable name journal kept as .bad; a new journal starts.");return true;}
 if(size.QuadPart>24+16384*24)size.QuadPart=24+16384*24;
 unsigned total=unsigned((size.QuadPart-24)/24);
 if((size.QuadPart-24)%24||size.QuadPart>24+LONGLONG(total)*24){ // R40 A15: half-written last record (interrupted write) -> cut it off and continue
  LARGE_INTEGER cut{};cut.QuadPart=24+LONGLONG(total)*24;if(SetFilePointerEx(f,cut,nullptr,FILE_BEGIN))SetEndOfFile(f);
  if(core27->Log)core27->Log("Patch27 R40 names: half-written last record removed; journal continues.");
  LARGE_INTEGER back{};back.QuadPart=24;SetFilePointerEx(f,back,nullptr,FILE_BEGIN);}
 for(unsigned i=0;i<total;i++){DiskName disk{};if(!exact_io(f,&disk,sizeof(disk),false)){total=i;break;}
  dictionary[i]=disk.pair;usable[i]=disk.crc==crc32(&disk.pair,sizeof(disk.pair))&&length(disk.pair.surname)<=4&&length(disk.pair.given)<=4;entries=LONG(i+1);
 }
 CloseHandle(f);return true;
}
extern "C" void NamesShutdown(){for(unsigned i=0;i<allocated;i++)VirtualFree(trampolines[i],0,MEM_RELEASE);allocated=0;if(tls!=TLS_OUT_OF_INDEXES)TlsFree(tls);tls=TLS_OUT_OF_INDEXES;}
static int find_or_append(const NamePair&p){
 for(unsigned i=0;i<U32(entries);i++)if(usable[i]&&!memcmp(&dictionary[i],&p,sizeof(p)))return int(i);
 if(!writable||entries>=16384)return -1;
 HANDLE f=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,0,nullptr);if(f==INVALID_HANDLE_VALUE)return -1;
 LARGE_INTEGER size{};bool ok=GetFileSizeEx(f,&size)!=0;
 if(ok&&size.QuadPart==0&&entries==0){NameHeader h{{'S','1','4','N','A','M','E','1'},1,24,{0,0}};ok=exact_io(f,&h,sizeof(h),true);size.QuadPart=24;}
 if(ok&&size.QuadPart>24+LONGLONG(entries)*24){LARGE_INTEGER cut{};cut.QuadPart=24+LONGLONG(entries)*24;ok=SetFilePointerEx(f,cut,nullptr,FILE_BEGIN)&&SetEndOfFile(f);size=cut;} // R40 A15
 if(ok&&size.QuadPart!=24+LONGLONG(entries)*24)ok=false;
 if(ok){LARGE_INTEGER end{};ok=SetFilePointerEx(f,end,nullptr,FILE_END)!=0;}
 DiskName disk{p,crc32(&p,sizeof(p))};if(ok)ok=exact_io(f,&disk,sizeof(disk),true)&&FlushFileBuffers(f);
 CloseHandle(f);if(!ok)return -1; // R40 A15: no permanent latch; the next name save tries again
 unsigned i=unsigned(entries);dictionary[i]=p;usable[i]=true;InterlockedExchange(&entries,LONG(i+1));return int(i);
}
static bool set_pair(U8*r,const NamePair&p){
 if(!editor(r)||length(p.surname)>4||length(p.given)>4)return false;
 AcquireSRWLockExclusive(&lock);
 int tag=-1;bool longer=length(p.surname)>2||length(p.given)>2;
 if(longer)tag=find_or_append(p);
 if(!longer||tag>=0){prefix((wchar_t*)(r+0xc),p.surname);prefix((wchar_t*)(r+0x12),p.given);at<U16>(r,0xa)=U16(longer?0x4000+tag:4);}
 ReleaseSRWLockExclusive(&lock);return !longer||tag>=0;
}
static Edit*edit(U8*owner,bool reset=false){
 if(!core27->Readable(owner,0x600)||!editor(at<U8*>(owner,0x198)))return nullptr;
 U8*r=at<U8*>(owner,0x198);
 for(auto&e:edits)if(e.owner==owner&&e.record==r){if(reset)e.pair=current(r);return &e;}
 Edit&e=edits[next_edit++%8];e={};e.owner=owner;e.record=r;e.pair=current(r);return &e;
}
static void failure(){if(core27->Log)core27->Log("Patch27 names: name journal could not be saved; officer record retained.");MessageBoxW(nullptr,L"이름 자료를 저장하지 못해 기존 무장 정보를 유지했습니다. 게임 폴더의 쓰기 권한과 남은 공간을 확인해 주세요.",L"신무장 이름 저장",MB_OK|MB_ICONERROR);}
extern "C" {
 UPTR NamesVersionReturn,NamesInputReturn;
 void NamesVersionBridge();void NamesInputBridge();
 __declspec(dllexport) const wchar_t*NamesSurname(U8*r){auto*s=(NameScope*)TlsGetValue(tls);const NamePair*p=(!s||!s->building)?full_fast(r):nullptr;return p?p->surname:(wchar_t*)(r+0xc);}
 __declspec(dllexport) const wchar_t*NamesGiven(U8*r){auto*s=(NameScope*)TlsGetValue(tls);const NamePair*p=(!s||!s->building)?full_fast(r):nullptr;return p?p->given:(wchar_t*)(r+0x12);}
 __declspec(dllexport) const wchar_t*NamesShortSurname(U8*r){return (wchar_t*)(r+0xc);}
 static void set_name(U8*r,const wchar_t*value,int field){
  if(!editor(r)){(field?native_given:native_surname)(r,value);return;}
  auto*s=(NameScope*)TlsGetValue(tls);if(s&&s->committing&&s->committing->record==r)return;
  const wchar_t*resolved=resolve(value);unsigned n=length(resolved);
  if(n>4)return;
  NamePair p=current(r);wchar_t*out=field?p.given:p.surname;memset(out,0,10);memcpy(out,resolved,n*2);if(!set_pair(r,p)&&core27->Log)core27->Log("Patch27 names: setter retained old record after persistence failure.");
 }
 __declspec(dllexport) void NamesSetSurname(U8*r,const wchar_t*s){set_name(r,s,0);}
 __declspec(dllexport) void NamesSetGiven(U8*r,const wchar_t*s){set_name(r,s,1);}
 __declspec(dllexport) void NamesVersion(U8*r){if(!full(r))at<U16>(r,0xa)=4;}
 __declspec(dllexport) UPTR NamesFormat(UPTR a,void*b,UPTR c,const wchar_t*d,const wchar_t*s,const wchar_t*g){return fn<UPTR(*)(UPTR,void*,UPTR,const wchar_t*,const wchar_t*,const wchar_t*)>(game27,0x197e50)(a,b,c,d,resolve(s),resolve(g));}
 __declspec(dllexport) UPTR NamesTransferString(void*out,const wchar_t*raw,UPTR count){const wchar_t*resolved=resolve(raw);if(resolved!=raw)count=length(resolved);return fn<UPTR(*)(void*,const wchar_t*,UPTR)>(game27,0x21fad0)(out,resolved,count);}
 __declspec(dllexport) UPTR NamesBuild(U8*owner,void*params,UPTR c,UPTR d){
  UPTR result;{NameScope scope(true,nullptr);if(!scope.active)return 0;result=native_build(owner,params,c,d);}if(!result)return result;
  Edit*e=edit(owner,true);if(!e)return result;
  for(int field=0;field<2;field++){
   int id=at<int>(owner,0x180+field*4);U8*w=fn<U8*(*)(void*,int)>(game27,0x92f390)(owner+0x130,id);const wchar_t*name=field?e->pair.given:e->pair.surname;
   if(w&&core27->Readable(w,8)){U8*vt=at<U8*>(w,0);if(core27->Readable(vt,0x1e0))reinterpret_cast<void(*)(void*,const wchar_t*)>(at<void*>(vt,0x1d8))(w,name);}
   U8*panel=at<U8*>(owner,0x5f0);
   if(w&&core27->Readable(panel,0x188)){
    void*report=game27+0x1a26440;fn<void(*)(void*)>(game27,0x30c380)(report);
    fn<void(*)(void*,int,int)>(game27,0x30fdb0)(report,4,0);
    void*help=fn<void*(*)(void*)>(game27,0x5c1550)(at<void*>(panel,0x180));fn<void(*)(void*,void*,int,int)>(game27,0x89d340)(help,w,0x2830,0);
   }
  }
  return result;
 }
 __declspec(dllexport) void*NamesInput(U8*owner,int field){
  Edit*e=edit(owner);if(!e||field<0||field>1)return nullptr;
  wchar_t*text=field?e->pair.given:e->pair.surname;
  e->input={text,5};return &e->input;
 }
 __declspec(dllexport) void NamesInputResult(U8*owner,U8*event){
  Edit*e=edit(owner);U8*w=core27->Readable(owner,0x600)?at<U8*>(owner,0x5e8):nullptr;
  if(!e||!core27->Readable(w,0x88)||!core27->Readable(event,12)){native_result(owner,event);return;}
  int field=at<int>(w,0x80);if(field<0||field>1){native_result(owner,event);return;}
  wchar_t converted[6]={};
  fn<void(*)(void*,wchar_t*,int,int,int)>(game27,0x207660)(at<void*>(owner,0x5e0),converted,6,0,1);
  wchar_t*dest=field?e->pair.given:e->pair.surname;memset(dest,0,10);memcpy(dest,converted,8);
  prefix((wchar_t*)(owner+0x1a0+field*6),dest);
  U8*vt=at<U8*>(w,0);if(core27->Readable(vt,0x1e0))reinterpret_cast<void(*)(void*,const wchar_t*)>(at<void*>(vt,0x1d8))(w,dest);
  at<U8*>(owner,0x5e8)=nullptr;
  if(at<U32>(event,8)==0x7ffffffd){int next=fn<int(*)(void*,int)>(game27,0x5c0d50)(owner,field);if(next>=0&&next<5){U8*button=fn<U8*(*)(void*,int)>(game27,0x92f390)(owner+0x130,at<int>(owner,0x180+next*4));fn<void(*)(void*,void*)>(game27,0x62f7e0)(owner,button);}}
 }
 __declspec(dllexport) void NamesCommit(U8*owner){
  Edit*e=edit(owner);if(!e){native_commit(owner);return;}
  NameScope scope(false,e);if(!scope.active||!set_pair(e->record,e->pair)){failure();return;}
  native_commit(owner);
 }
}
static void*trampoline(U32 rva,unsigned n){
 if(allocated==5)return nullptr;U8*p=(U8*)VirtualAlloc(nullptr,64,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!p)return nullptr;
 trampolines[allocated++]=p;memcpy(p,game27+rva,n);ModuleTransaction::abs_jump(p+n,game27+rva+n);if(!FlushInstructionCache(GetCurrentProcess(),p,n+14))return nullptr;return p;
}
extern "C" bool NamesInstall(const S14_CoreAPI*a){
 if(!NamesInit()){r39::fail(25,"SAN14_Patch27.dll","names_init","NamesInstall -> NamesInit() (names data file open / TlsAlloc)","names file opened","open failed or TLS_OUT_OF_INDEXES",L"무장 이름 확장(25번)이 안 됨 → 게임 기본 이름");return false;}
 native_surname=(Setter)trampoline(0x21d7f0,8);native_given=(Setter)trampoline(0x21d150,8);native_build=(Build)trampoline(0x5df050,6);native_result=(Result)trampoline(0x62a2f0,6);native_commit=(Commit)trampoline(0x622ba0,6);
 if(!native_surname||!native_given||!native_build||!native_result||!native_commit){r39::fail(25,"SAN14_Patch27.dll","trampoline","NamesInstall -> trampoline(0x21d7f0/0x21d150/0x5df050/0x62a2f0/0x622ba0)","5 trampolines (near exec memory)","one or more NULL (alloc failed or entry bytes not relocatable)",L"무장 이름 확장(25번)이 안 됨 → 게임 기본 이름");NamesShutdown();return false;}
 NamesVersionReturn=(UPTR)(game27+0x21afaf);NamesInputReturn=(UPTR)(game27+0x62f896);
 HookPlan p(a);
 const U8 get_s[]={0x48,0x8d,0x41,0x0c,0xc3},get_g[]={0x48,0x8d,0x41,0x12,0xc3},set[]={0x40,0x53,0x55,0x57,0x48,0x83,0xec,0x20},version[]={0x66,0x44,0x89,0x61,0x0a};
 const U8 build[]={0x40,0x55,0x56,0x57,0x41,0x54},result[]={0x40,0x57,0x48,0x83,0xec,0x30},commit[]={0x40,0x53,0x48,0x83,0xec,0x20};
 const U8 input_s[]={0x4c,0x8d,0xbe,0xa0,1,0,0,0x41,0xbe,3,0,0,0},input_g[]={0x4c,0x8d,0xbe,0xa6,1,0,0,0x41,0xbe,3,0,0,0};
 p.jump(0x32dc20,get_s,5,(void*)NamesSurname);p.jump(0x35d1a0,get_g,5,(void*)NamesGiven);
 p.jump(0x21d7f0,set,8,(void*)NamesSetSurname);p.jump(0x21d150,set,8,(void*)NamesSetGiven);p.jump(0x21afaa,version,5,(void*)NamesVersionBridge);
 p.jump(0x5df050,build,6,(void*)NamesBuild);p.jump(0x62a2f0,result,6,(void*)NamesInputResult);p.jump(0x622ba0,commit,6,(void*)NamesCommit);
 p.jump(0x62f84d,input_s,13,(void*)NamesInputBridge);p.jump(0x62f85c,input_g,13,(void*)NamesInputBridge);
 p.call(0x2101bb,0x197e50,(void*)NamesFormat);p.call(0x47ad1c,0x32dc20,(void*)NamesShortSurname);
 p.call(0x20bc91,0x21fad0,(void*)NamesTransferString);p.call(0x20bce1,0x21fad0,(void*)NamesTransferString);p.call(0x20bd81,0x21fad0,(void*)NamesTransferString);   // R43: 새 판은 같은 함수 안에 전달 호출이 3곳(옛 0x20a8d3/0x20a921 → 0x20bc91/0x20bce1 + 새 블록 0x20bd81)
 bool ok=p.install("SAN14CE.Patch27.OfficerNames");if(!ok){r39::fail_plan(25,"SAN14_Patch27.dll",p,"officer names (get 0x32dc20/0x35d1a0, set 0x21d7f0/0x21d150, version 0x21afaa, build 0x5df050, result 0x62a2f0, commit 0x622ba0, input 0x62f84d/0x62f85c, calls)",L"무장 이름 확장(25번)이 안 됨 → 게임 기본 이름");NamesShutdown();}return ok;
}
