// SAN14_PrisonerReturn.dll R4 — 22번 포로 관직 유지 · 원래 소속 도시 귀환
// 포획·해방 동작은 R1(기존 모듈)과 같다. 보존 방식만 바꿨다:
//  R1: 무장 6000명 직렬화마다 시스템콜·SHA-256을 돌려 세이브 내용 해시를 만들고, 저장마다 포로마다 <해시>.bin 을 새로 만들었다(지우지 않음).
//  R2: 세이브 이름을 게임 파일 계층의 CreateFileW(.s14ko)에서 받으려 했으나, 이 실행 파일은 세이브를 스팀 원격 저장소
//      (ISteamRemoteStorage FileWrite/FileRead, 0x3f6e42/0x3f95b4)로 쓰고 읽는다. .s14ko 는 신무장 편집 파일이라
//      실제 세이브에서는 이름을 한 번도 받지 못했다(불러올 때마다 "이름 모름 → 표 비움").
//  R3: 인물 직렬화(이미 걸려 있던 CPersonData vtable 칸)가 받는 스트림 객체에 게임이 세이브 이름을 직접 들고 있다
//      (0x3f9450 이 이름을 스트림 +0 std::string 에 복사, +0x20 = 1 읽기 / 그 밖 쓰기). 새 스트림의 첫 무장에서
//      이름이 세이브(svdexce*/autosdexce* .s14)이면 쓰기 = 사이드카 저장, 읽기 = 사이드카로 표 교체.
//      파일 열기 훅(0x940357)은 뺐다 — 게임의 모든 파일 열기마다 끼던 호출이 없어진다. 무장마다 드는 비용은 포인터 비교 하나.
//  R4 (턴 진행 지연 줄이기): 포획·해방마다 기록 파일을 열고(CreateFileW) 크기를 묻고 닫던 것을 없앴다. 파일은 한 번만 열어 두고
//      WriteFile 한 번만 한다(운영체제 캐시에 쓰고 끝; 디스크 동기화 없음). 1MB 넘으면 그때만 닫고 .prev.log 로 돌린 뒤 다시 연다.
//      포획·해방 처리 자체(관직 유지·귀환 도시·사이드카 내용)는 R3 과 같다.
#include "common.h"
#include "r39_record.h"
namespace{
const S14_CoreAPI*core;U8*game;
wchar_t root[1024];unsigned rootLen;wchar_t dir[1024],logPath[1024],prevLogPath[1024];
SRWLOCK lock=SRWLOCK_INIT;
HANDLE logFile=INVALID_HANDLE_VALUE;ULONGLONG logBytes;   // R4: 기록 파일은 열어 둔다 (lock 아래에서만 만진다)
struct Rec{U8 valid,rank,force,pad;U16 id,home;wchar_t name[14];};   // 36바이트
static_assert(sizeof(Rec)==36,"record size");
Rec recs[0x1771];
volatile LONG loadsApplied,sidecarsSaved;unsigned installed;
U8*lastStream;ULONGLONG lastStreamTick;int lastStreamMode=-1;

// ---------------------------------------------------------------- 문자열·파일
struct WBuf{wchar_t b[400];unsigned n=0;WBuf(){b[0]=0;}
 void add(const wchar_t*s){if(!s)return;while(*s&&n<399)b[n++]=*s++;b[n]=0;}
 void addn(const wchar_t*s,unsigned max){if(!s)return;for(unsigned i=0;i<max&&s[i]&&n<399;i++)b[n++]=s[i];b[n]=0;}
 void num(long long v){wchar_t t[24];unsigned k=0;bool neg=v<0;unsigned long long u=neg?0ull-(unsigned long long)v:(unsigned long long)v;do{t[k++]=wchar_t(L'0'+u%10);u/=10;}while(u);if(neg)t[k++]=L'-';while(k&&n<399)b[n++]=t[--k];b[n]=0;}
 const wchar_t*c()const{return b;}
};
unsigned wlen(const wchar_t*s){unsigned n=0;while(s&&s[n])n++;return n;}
wchar_t lower(wchar_t c){return (c>=L'A'&&c<=L'Z')?wchar_t(c+32):c;}
bool pathJoin(wchar_t*out,const wchar_t*rel){if(rootLen+wlen(rel)+1>1000)return false;memcpy(out,root,rootLen*2);wcscpy(out+rootLen,rel);return true;}
// R4: 열어 둔 핸들에 덧붙이기만 한다. 열기·크기 조회·닫기는 처음 한 번과 1MB 교체 때만.
bool openLog(){
 logFile=CreateFileW(logPath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_ALWAYS,0,nullptr);
 if(logFile==INVALID_HANDLE_VALUE)return false;
 LARGE_INTEGER size{};logBytes=GetFileSizeEx(logFile,&size)?(ULONGLONG)size.QuadPart:0;return true;
}
void logLine(const wchar_t*text,bool alsoCore=false){
 char buf[1200];int n=WideCharToMultiByte(CP_UTF8,0,text,-1,buf,sizeof(buf)-4,nullptr,nullptr);if(n<=0)return;n--;
 if(alsoCore&&core&&core->Log)core->Log(buf);
 buf[n++]='\r';buf[n++]='\n';
 AcquireSRWLockExclusive(&lock);
 if(logFile==INVALID_HANDLE_VALUE)openLog();
 if(logFile!=INVALID_HANDLE_VALUE&&logBytes+(ULONGLONG)n>(1u<<20)){CloseHandle(logFile);logFile=INVALID_HANDLE_VALUE;MoveFileExW(logPath,prevLogPath,MOVEFILE_REPLACE_EXISTING);openLog();}
 if(logFile!=INVALID_HANDLE_VALUE){DWORD w=0;if(WriteFile(logFile,buf,(DWORD)n,&w,nullptr))logBytes+=w;else{CloseHandle(logFile);logFile=INVALID_HANDLE_VALUE;}}
 ReleaseSRWLockExclusive(&lock);
}

// ---------------------------------------------------------------- 게임 접근
inline bool rd(const void*p,size_t n){return p&&(UPTR)p>0x10000&&core->Readable(p,n);}
inline U8*gm(){return at<U8*>(game,0x2780f60);}
inline bool personValid(U8*p){return rd(p,0x1e8)&&at<U8*>(p,0)==game+0x13a9bc8&&at<U16>(p,0x10)<=0x1770;}
// 인물표(gm+0x158)는 게임이 늘 유효한 객체로 채워 두므로(게임 코드도 그대로 역참조) 시스템콜 없이 vtable 만 본다.
inline U8*personAt(unsigned id){U8*g=gm();if(!g||id>0x1770)return nullptr;U8*p=at<U8*>(g,0x158+8*id);return (p&&(UPTR)p>0x10000&&at<U8*>(p,0)==game+0x13a9bc8)?p:nullptr;}
// R1의 0x26f0: 군단(+0xe0) → 군단표(gm+0xe0d0)[d]+0x10 = 세력 번호. 없으면 0.
inline int forceOf(U8*p){U8*g=gm();if(!g)return 0;unsigned d=at<U8>(p,0xe0);if(d>=0x34)return 0;U8*dist=at<U8*>(g,0xe0d0+8*d);return (dist&&(UPTR)dist>0x10000)?at<U8>(dist,0x10):0;}   // 군단표는 게임이 늘 채워 둔 정적 객체
inline U8*cityById(int id){U8*g=gm();if(!g||id<=0)return nullptr;U8*c=fn<U8*(*)(U8*,int)>(game,0x32e3b0)(g,id);return (rd(c,0x164)&&at<U8*>(c,0)==game+0x13a96d0)?c:nullptr;}
inline int vcall(U8*o,size_t slot){return reinterpret_cast<int(*)(void*)>(at<void*>(at<U8*>(o,0),slot))(o);}
void nameOf(U8*p,WBuf&b){b.addn((const wchar_t*)(p+0x12),6);b.addn((const wchar_t*)(p+0x20),6);}

// ---------------------------------------------------------------- 사이드카 (세이브 슬롯당 1개)
struct Header{char magic[4];U32 count;};
bool sidecarPath(const wchar_t*name,wchar_t*out){WBuf b;b.add(L"SAN14Mods\\PrisonerReturn\\");b.add(name);b.add(L".prison");return pathJoin(out,b.c());}
// 지금 포로(상태 6)이고 이름이 맞는 기록만 쓴다 → 쓸 때마다 묵은 기록이 걸러진다.
void saveSidecar(const wchar_t*name){
 static Rec out[0x1771];unsigned n=0;
 AcquireSRWLockExclusive(&lock);
 for(unsigned id=1;id<=0x1770;id++){Rec&r=recs[id];if(!r.valid)continue;U8*p=personAt(id);if(!p||at<U8>(p,0xe6)!=6||memcmp(r.name,p+0x12,28))continue;out[n++]=r;}
 ReleaseSRWLockExclusive(&lock);
 wchar_t path[1024];if(!sidecarPath(name,path))return;
 HANDLE f=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);
 if(f==INVALID_HANDLE_VALUE){WBuf b;b.add(L"sidecar write failed: ");b.add(name);logLine(b.c());return;}
 Header h{{'S','P','R','2'},n};DWORD w=0;WriteFile(f,&h,sizeof h,&w,nullptr);if(n)WriteFile(f,out,(DWORD)(n*sizeof(Rec)),&w,nullptr);CloseHandle(f);
 __atomic_add_fetch(&sidecarsSaved,1,__ATOMIC_SEQ_CST);
 WBuf b;b.add(L"sidecar saved: ");b.add(name);b.add(L" prisoners=");b.num(n);logLine(b.c());
}
// 표를 비우고 사이드카로 다시 채운다. 파일이 없으면 비운 채로 둔다(그 세이브에는 기록이 없는 것).
void loadSidecar(const wchar_t*name){
 static Rec in[0x1771];unsigned n=0;bool found=false;
 wchar_t path[1024];
 if(sidecarPath(name,path)){HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
  if(f!=INVALID_HANDLE_VALUE){Header h{};DWORD got=0;found=true;
   if(ReadFile(f,&h,sizeof h,&got,nullptr)&&got==sizeof h&&!memcmp(h.magic,"SPR2",4)&&h.count<=0x1770){DWORD want=(DWORD)(h.count*sizeof(Rec));if(ReadFile(f,in,want,&got,nullptr)&&got==want)n=h.count;}
   CloseHandle(f);}}
 unsigned kept=0;
 AcquireSRWLockExclusive(&lock);
 memset(recs,0,sizeof recs);
 for(unsigned i=0;i<n;i++){Rec&r=in[i];if(!r.valid||r.id<1||r.id>0x1770||r.home<1||r.home>0x33||r.force<1||r.force>0x33||r.rank>0x49)continue;recs[r.id]=r;kept++;}
 ReleaseSRWLockExclusive(&lock);
 __atomic_add_fetch(&loadsApplied,1,__ATOMIC_SEQ_CST);
 WBuf b;b.add(L"sidecar loaded: ");b.add(name);b.add(found?L" prisoners=":L" (없음) prisoners=");b.num(kept);logLine(b.c());
}
// 직렬화 스트림 +0 의 std::string(MSVC: 16바이트 내장 또는 포인터, +0x10 길이, +0x18 용량)에서 세이브 파일 이름을 꺼낸다.
// 세이브 슬롯 이름(0x32c050 형식: svdexceKO%02d.s14 / svdexceccKO%02d.s14 / autosdexceKO.. / autosdexceccKO..)만 받는다.
bool streamName(U8*stream,wchar_t*name){
 if(!rd(stream,0x28))return false;
 UPTR size=at<UPTR>(stream,0x10),cap=at<UPTR>(stream,0x18);if(size<8||size>60||cap<size)return false;
 const char*s=cap>=0x10?at<const char*>(stream,0):(const char*)stream;if(cap>=0x10&&!rd(s,size))return false;
 unsigned b=0;for(unsigned i=0;i<size;i++){char c=s[i];if(c=='/'||c=='\\')b=i+1;else if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'))return false;}
 unsigned n=unsigned(size)-b;if(n<8||n>60)return false;
 auto low=[&](unsigned i){char c=s[b+i];return (c>='A'&&c<='Z')?char(c+32):c;};
 if(low(n-4)!='.'||low(n-3)!='s'||low(n-2)!='1'||low(n-1)!='4')return false;
 static const char*prefixes[]={"svdexce","autosdexce"};bool ok=false;
 for(const char*p:prefixes){unsigned k=0;while(p[k]&&k<n&&low(k)==p[k])k++;if(!p[k]){ok=true;break;}}
 if(!ok)return false;
 for(unsigned i=0;i<n;i++)name[i]=wchar_t((unsigned char)s[b+i]);name[n]=0;return true;
}
}
extern "C" {
UPTR Cont2e0000;
int Orig2e0000(U8*,void*,int,int,void*,void*);
// ---------------------------------------------------------------- 포획 (0x2e0000 진입 5바이트)
__declspec(dllexport) int CaptureHook(U8*p,void*army,int flag,int r9,void*force,void*captor){
 bool ok=personValid(p);int st=ok?at<U8>(p,0xe6):0;ok=ok&&st>=1&&st<=4;
 U8 rank0=0,force0=0;U16 city0=0;wchar_t name[14];
 if(ok){rank0=at<U8>(p,0xe7);city0=at<U16>(p,0xe2);force0=(U8)forceOf(p);memcpy(name,p+0x12,28);}
 int result=Orig2e0000(p,army,flag,r9,force,captor);   // 원본 반환값은 그대로 돌려준다
 if(!ok||at<U8>(p,0xe6)!=6||(U8)forceOf(p)!=force0)return result;
 if(city0<1||city0>0x33||force0<1||force0>0x33)return result;
 unsigned id=at<U16>(p,0x10);
 AcquireSRWLockExclusive(&lock);Rec&r=recs[id];r.valid=1;r.rank=rank0;r.force=force0;r.pad=0;r.id=(U16)id;r.home=city0;memcpy(r.name,name,28);ReleaseSRWLockExclusive(&lock);
 if(rank0<=0x48)at<U8>(p,0xe7)=rank0;
 WBuf b;b.add(L"captured; rank retained: id=");b.num(id);b.add(L" home=");b.num(city0);b.add(L" rank=");b.num(rank0);b.add(L" force=");b.num(force0);b.add(L" ");nameOf(p,b);logLine(b.c());
 return result;
}
// ---------------------------------------------------------------- 해방 (0x2e6d2f 의 call 0x2e99e0 자리)
__declspec(dllexport) int ReleaseHook(U8*p,int a,int c){
 auto native=fn<int(*)(void*,int,int)>(game,0x2eca90);
 if(!personValid(p)||at<U8>(p,0xe6)!=4)return native(p,a,c);
 unsigned id=at<U16>(p,0x10);Rec r;
 AcquireSRWLockExclusive(&lock);r=recs[id];recs[id].valid=0;ReleaseSRWLockExclusive(&lock);
 if(!r.valid||r.id!=id||memcmp(r.name,p+0x12,28)||r.force!=(U8)forceOf(p))return native(p,a,c);
 WBuf who;nameOf(p,who);
 U8*city=cityById(r.home);
 if(city&&at<U16>(city,0x4e)==r.home){int owner=vcall(city,0xb0),district=vcall(city,0x78);
  if(owner==r.force&&district>=1&&district<=0x33){fn<void(*)(void*,int,int)>(game,0x242650)(p,district,0);at<U16>(p,0xe2)=r.home;
   WBuf b;b.add(L"release route redirected: id=");b.num(id);b.add(L" home=");b.num(r.home);b.add(L" ");b.add(who.c());logLine(b.c());}
  else{WBuf b;b.add(L"home lost; native destination kept: id=");b.num(id);b.add(L" home=");b.num(r.home);b.add(L" owner=");b.num(owner);b.add(L" ");b.add(who.c());logLine(b.c());}
 }
 // 관직 복원·중복 방지: 같은 세력에 같은 관직을 가진 다른 무장이 있으면 관직 없음(0x49)으로.
 if(r.rank<=0x48&&gm()){bool occupied=false;
  for(unsigned i=1;i<=0x1770&&!occupied;i++){U8*q=personAt(i);if(!q||q==p)continue;int st=at<U8>(q,0xe6);if(!((st>=1&&st<=4)||st==6))continue;if(at<U8>(q,0xe7)!=r.rank||forceOf(q)!=r.force)continue;occupied=true;}
  at<U8>(p,0xe7)=occupied?0x49:r.rank;
  if(occupied){WBuf b;b.add(L"rank occupied; duplicate prevented: id=");b.num(id);b.add(L" rank=");b.num(r.rank);b.add(L" ");b.add(who.c());logLine(b.c());}
 }
 return native(p,a,c);
}
// ---------------------------------------------------------------- 인물 직렬화 (CPersonData vtable +0x28). 불러오기(스트림 모드 1)가 시작되면 표를 그 세이브의 사이드카로 바꾼다.
__declspec(dllexport) UPTR SerializeHook(U8*p,U8*stream,UPTR a,UPTR b){
 UPTR result=fn<UPTR(*)(void*,void*,UPTR,UPTR)>(game,0x23aa60)(p,stream,a,b);   // 반환값·여분 인자 그대로
 if(!stream)return result;
 ULONGLONG now=GetTickCount64();
 int mode=at<int>(stream,0x20);
 if(stream==lastStream&&mode==lastStreamMode&&now-lastStreamTick<3000){lastStreamTick=now;return result;}  // 같은 저장/불러오기 안의 다음 무장
 lastStream=stream;lastStreamTick=now;lastStreamMode=mode;
 bool reading=mode==1;wchar_t name[64]={};bool have=streamName(stream,name);
 if(have){if(reading)loadSidecar(name);else saveSidecar(name);}
 else if(reading){AcquireSRWLockExclusive(&lock);memset(recs,0,sizeof recs);ReleaseSRWLockExclusive(&lock);logLine(L"load without save-slot name (new game or other data): prisoner table cleared");}
 return result;
}
// ---------------------------------------------------------------- 질의 (자동확인 시험 모듈용)
__declspec(dllexport) int PrisonerReturnRecord(unsigned id,int*home,int*rank,int*force){
 if(id>0x1770)return 0;AcquireSRWLockShared(&lock);Rec r=recs[id];ReleaseSRWLockShared(&lock);
 if(!r.valid)return 0;if(home)*home=r.home;if(rank)*rank=r.rank;if(force)*force=r.force;return 1;
}
__declspec(dllexport) int PrisonerReturnLoads(){return loadsApplied;}
__declspec(dllexport) int PrisonerReturnSaves(){return sidecarsSaved;}
__declspec(dllexport) int PrisonerReturnVersion(){return 4;}
__declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,21,"SAN14_PrisonerReturn.dll"))return 0;core=api;game=api->resolved->image;
 DWORD n=GetModuleFileNameW(nullptr,root,1024);if(!n||n>=900||[&]{while(n&&root[n-1]!=L'\\'&&root[n-1]!=L'/')n--;return n==0;}()){r39::fail(21,"SAN14_PrisonerReturn.dll","root","SAN14_ModInitV1 -> GetModuleFileNameW(game exe)","game folder path (<900 chars)","failed or too long",L"포로 원래 도시 복귀(21번)가 이번 실행에서 동작 안 함");return 0;}root[n]=0;rootLen=n;
 pathJoin(dir,L"SAN14Mods");CreateDirectoryW(dir,nullptr);pathJoin(dir,L"SAN14Mods\\PrisonerReturn");CreateDirectoryW(dir,nullptr);
 pathJoin(logPath,L"SAN14Mods\\PrisonerReturn\\PrisonerReturn.log");pathJoin(prevLogPath,L"SAN14Mods\\PrisonerReturn\\PrisonerReturn.prev.log");
 // R1이 저장마다 남긴 <해시>.bin 정리 (R2·R3는 쓰지 않음). 관직은 세이브 안에 있으므로 잃는 것은 '지금 포로인 무장의 원래 도시'만.
 {wchar_t pat[1024];pathJoin(pat,L"SAN14Mods\\PrisonerReturn\\*.bin");WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);unsigned removed=0;
  if(h!=INVALID_HANDLE_VALUE){do{if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;wchar_t f[1024];pathJoin(f,L"SAN14Mods\\PrisonerReturn\\");wcscpy(f+wlen(f),fd.cFileName);if(DeleteFileW(f))removed++;}while(FindNextFileW(h,&fd));FindClose(h);}
  if(removed){WBuf b;b.add(L"R3: removed ");b.num(removed);b.add(L" legacy per-save .bin sidecars");logLine(b.c());}}
 Cont2e0000=(UPTR)(game+0x2e3025);
 {HookPlan p(api);const U8 e[]={0x44,0x89,0x4c,0x24,0x20};p.jump(0x2e3020,e,5,(void*)CaptureHook);p.call(0x2e9ddf,0x2eca90,(void*)ReleaseHook);if(p.install("SAN14CE.PrisonerReturn"))installed|=1;
  else r39::fail_plan(21,"SAN14_PrisonerReturn.dll",p,"capture exe+0x2e3020 / release call exe+0x2e9ddf",L"포로 원래 도시 복귀(21번)가 동작 안 함 → 게임 기본(석방 시 가까운 도시). 다른 기능은 그대로");}
 {S14_Hook vt{};vt.target=game+0x13a9bf0;vt.length=8;UPTR old=(UPTR)(game+0x23aa60),next=(UPTR)&SerializeHook;memcpy(vt.expected,&old,8);memcpy(vt.replacement,&next,8);
  // 인물 직렬화 vtable 칸 하나로 저장·불러오기를 모두 처리한다(세이브 이름은 스트림에 있음).
  if(api->InstallHooks("SAN14CE.PrisonerReturn.SaveSlot",&vt,1))installed|=2;
  else{char ex[40],ac[40];ex[0]=ac[0]=0;r39::sbytes(ex,sizeof ex,vt.expected,8);if(api->Readable(vt.target,8))r39::sbytes(ac,sizeof ac,vt.target,8);else r39::scat(ac,sizeof ac,"unreadable");
   r39::fail(21,"SAN14_PrisonerReturn.dll","vtable","CPersonData serialize vtable slot exe+0x13a9bf0 (orig fn exe+0x23aa60)",ex,ac,L"포로 원래 도시가 세이브에 같이 저장·복원되지 않음(이번 실행 안에서는 동작). 다른 모드가 인물 직렬화 칸을 먼저 바꿈");}}
 WBuf b;b.add(L"PrisonerReturn R4 loaded: capture rank retained; friendly original-home release");b.add((installed&1)?L" [hooks on]":L" [hooks NOT installed]");b.add((installed&2)?L"; per-save-slot sidecar from the save stream name (no per-person hashing)":L"; save-slot sidecar NOT installed (person-serialize slot taken) — records live for this session only");
 logLine(b.c(),true);
 return (installed&1)?1:0;
}
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
