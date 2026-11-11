// 2번 자동논공 새 엔진(AutoMerit R42i4) — 리눅스 네이티브 시험(에뮬 없이)
//  가짜 게임 메모리(실행 가능)에 PE 머리·함수 지문·무장 표·세력·월드를 만들고, 게임 함수 자리에 시험용 함수로 가는 점프를 둬서
//  모듈 소스를 그대로 돌린다. 게임 논공 확정 함수 흉내는 MSVC std::map 순회(다음 칸 찾기)·찾기 방식 그대로 표를 읽는다.
//  빌드·실행: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_auto_merit_native.cpp -o /tmp/am && /tmp/am
#include "common.h"
#include <sys/mman.h>
#include <string>
#include <map>
#include <vector>
#include <cassert>
typedef void*HWND;
typedef struct{void*p;}SRWLOCK;
#define SRWLOCK_INIT {0}
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
static inline void AcquireSRWLockExclusive(SRWLOCK*){}
static inline void ReleaseSRWLockExclusive(SRWLOCK*){}
static int threads_started;
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*)(void*),void*,DWORD,DWORD*){threads_started++;return (HANDLE)1;}
extern "C" void Sleep(DWORD){}
static DWORD cur_thread=111,win_thread_id=111;
extern "C" DWORD GetCurrentThreadId(void){return cur_thread;}
extern "C" DWORD GetCurrentProcessId(void){return 4242;}
static bool merit13_file=false;
extern "C" DWORD GetFileAttributesW(const wchar_t*p){if(merit13_file){std::wstring s(p);if(s.find(L"MeritTraits")!=std::wstring::npos)return 0x20;}return INVALID_FILE_ATTRIBUTES;}
#include "../src/auto_merit.cpp"
// ---- 기록 파일·설정 흉내
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*o,DWORD cap){std::string p="/tmp/am_root\\x.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)o[n]=(wchar_t)p[n];o[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string c="mkdir -p '"+narrow(p)+"'";if(system(c.c_str())){}return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE&&h!=(HANDLE)1)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=4;}
static std::map<std::wstring,std::wstring> ini;
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*k,const wchar_t*v,const wchar_t*){ini[k]=v;return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*k,int d,const wchar_t*){auto i=ini.find(k);if(i==ini.end())return (unsigned)d;return (unsigned)wcstol(i->second.c_str(),nullptr,10);}
static int boxes;
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){boxes++;return 1;}
// user32 흉내(게임 창 하나, 스레드 win_thread_id)
static BOOL f_enum(BOOL(*cb)(HWND,UPTR),UPTR l){cb((HWND)0x77,l);return 1;}
static DWORD f_tpid(HWND,DWORD*pid){if(pid)*pid=4242;return win_thread_id;}
static BOOL f_vis(HWND){return 1;}
static HWND f_getw(HWND,unsigned){return nullptr;}
static bool merit13_loaded_flag=false;
static U8*G_exe; // R43: GetModuleHandleW(0) = 가짜 게임 실행 파일(PE 머리 있음)
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){if(!n)return (HMODULE)G_exe;std::wstring s(n);if(s==L"user32.dll")return (HMODULE)0x5555;if(s==L"SAN14_MeritTraits.dll"&&merit13_loaded_flag)return (HMODULE)0x6666;return nullptr;}
extern "C" void*GetProcAddress(HMODULE m,const char*n){if(m!=(HMODULE)0x5555)return nullptr;std::string s(n);
 if(s=="EnumWindows")return (void*)f_enum;if(s=="GetWindowThreadProcessId")return (void*)f_tpid;if(s=="IsWindowVisible")return (void*)f_vis;if(s=="GetWindow")return (void*)f_getw;return nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
// ---- 가짜 게임
static U8*G;static U8*D;static U8*W;
static I32 t_readable(const void*p,UPTR){return p!=nullptr;}
static I32 t_install(const char*,const S14_Hook*,U32){return 1;}
static U8*t_relay(const void*,const void*){return nullptr;}
static void t_free(void*){}
static S14_Resolved res;static S14_CoreAPI api;
static void jmp_to(U8*at,void*dest){at[0]=0x48;at[1]=0xb8;memcpy(at+2,&dest,8);at[10]=0xff;at[11]=0xe0;}
static int s_valid(void*p){return p!=nullptr;}
static void*s_alloc(U64 n){return calloc(1,n);}
static U8*s_force(U8*p){return *(U8**)(p+0x100);}   // 무장 → 세력 (게임 가상 함수 +0x80 흉내)
// 논공 확정 흉내: MSVC 방식으로 표를 읽고(다음 칸·찾기), 칸 값대로 무장 칸을 바꿈
static int commit_ret=1;static bool commit_apply=true;static int commits;
static std::vector<std::pair<U32,U8>> seen;static U8*seen_giver;static U8 next_rank=2;
static bool isnil(U8*n){return n[0x19]!=0;}
static U8*succ(U8*n){U8*r=*(U8**)(n+0x10);if(!isnil(r)){n=r;while(!isnil(*(U8**)n))n=*(U8**)n;return n;}U8*p=*(U8**)(n+8);while(!isnil(p)&&n==*(U8**)(p+0x10)){n=p;p=*(U8**)(p+8);}return p;}
static U8*findk(U8*head,U32 k){U8*x=*(U8**)(head+8),*best=head;while(!isnil(x)){if(*(U32*)(x+0x1c)<k)x=*(U8**)(x+0x10);else{best=x;x=*(U8**)x;}}return (!isnil(best)&&*(U32*)(best+0x1c)==k)?best:nullptr;}
static U8*person_at(U32 id){return *(U8**)(D+0x158+(size_t)id*8);}
static int s_commit(void*bp){
 commits++;U8*b=(U8*)bp;U8*head=*(U8**)b;U64 size=*(U64*)(b+8);seen_giver=*(U8**)(b+0x10);seen.clear();
 if(!size)return 0;
 for(U8*n=*(U8**)head;n!=head;n=succ(n)){seen.push_back({*(U32*)(n+0x1c),n[0x20]});assert(findk(head,*(U32*)(n+0x1c))==n);}
 assert(seen.size()==size);
 if(!commit_apply)return commit_ret;
 for(auto&s:seen){U8*o=person_at(s.first);o[0x1c2]=s.second&15;o[0x1c3]=s.second>>4;o[0x1bc]=next_rank;*(U16*)(o+0x15e)|=0x8000;}
 return commit_ret;
}
static U8*mkperson(U32 id,U8*force,U8 ruler,U8 up,U8 lo,U8 rank){
 U8*p=(U8*)calloc(1,0x200);*(U8**)p=G+0x200000;*(U16*)(p+0x10)=(U16)id;p[0xe6]=ruler;p[0x1c2]=up;p[0x1c3]=lo;p[0x1bc]=rank;*(U8**)(p+0x100)=force;
 *(U8**)(D+0x158+(size_t)id*8)=p;return p;
}
// IC 계획 vector(항목 0x28)
struct Vec{U8*b,*e;};
static Vec mkvec(std::vector<std::tuple<U8*,U32,U32,U32>> v){U8*b=(U8*)calloc(v.size()?v.size():1,0x28);U8*e=b;for(auto&t:v){*(U8**)e=std::get<0>(t);*(U32*)(e+0x10)=std::get<1>(t);*(U32*)(e+0x1c)=std::get<2>(t);*(U32*)(e+0x20)=std::get<3>(t);e+=0x28;}return {b,e};}
static int ic_calls;
static U64 ic_orig(U64,U64 vec,U64,U64){ic_calls++;Vec*v=(Vec*)vec;for(U8*e=v->b;e<v->e;e+=0x28){U8*o=*(U8**)e;o[0x1bc]=(U8)*(U32*)(e+0x10);o[0x1c2]=(U8)*(U32*)(e+0x1c);o[0x1c3]=(U8)*(U32*)(e+0x20);*(U16*)(o+0x15e)|=0x8000;}return 1;}
static std::string logtext(){FILE*f=fopen("/tmp/am_root/SAN14_\xEA\xB8\xB0\xEB\xA1\x9D/\xEC\xA0\x81\xEC\x9A\xA9\xED\x99\x95\xEC\x9D\xB8.log","rb");if(!f){std::string s;FILE*g=popen("cat /tmp/am_root/*/*.log 2>/dev/null","r");char b[4096];size_t n;while(g&&(n=fread(b,1,sizeof b,g))>0)s.append(b,n);if(g)pclose(g);return s;}std::string s;char b[4096];size_t n;while((n=fread(b,1,sizeof b,f))>0)s.append(b,n);fclose(f);return s;}
static int pass,fail;
#define CHECK(c,msg) do{if(c){pass++;printf("통과 %s\n",msg);}else{fail++;printf("실패 %s\n",msg);}}while(0)
static U64 run_exec(Vec&v){return SAN14_AutoMeritExec(0,(U64)&v,0,0,ic_orig);}
int main(){
 if(system("rm -rf /tmp/am_root && mkdir -p /tmp/am_root")){}
 G=(U8*)mmap(0,0x2a02000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE,-1,0);assert(G!=MAP_FAILED);G_exe=G;
 // PE 머리·지문
 G[0]=0x4d;G[1]=0x5a;*(U32*)(G+0x3c)=0x100;*(U32*)(G+0x100)=0x4550;*(U32*)(G+0x108)=am::STAMP;*(U32*)(G+0x150)=am::IMAGE;
 memcpy(G+am::RVA_COMMIT,am::G_COMMIT,16);memcpy(G+am::RVA_VALID,am::G_VALID,16);
 // 게임 DB·월드
 D=(U8*)calloc(1,0x90000);*(U8**)(G+am::RVA_DB)=D;W=(U8*)calloc(1,0x100);*(U8**)(D+am::DB_WORLD)=W;*(short*)(W+0x34)=200;W[0x36]=5;
 // 가상 함수표(이미지 안) → 세력 함수
 jmp_to(G+0x100000,(void*)s_force);*(U8**)(G+0x200000+0x80)=G+0x100000;
 res.size=sizeof res;res.image=G;res.image_size=S14_GAME_IMAGE;
 api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=t_readable;api.InstallHooks=t_install;api.MakeRelay=t_relay;api.FreeRelay=t_free;api.Log=nullptr;
 U8*F1=(U8*)calloc(1,0x40),*F2=(U8*)calloc(1,0x40);
 U8*ruler1=mkperson(10,F1,1,0,0,6),*a=mkperson(55,F1,0,0,0,1),*b=mkperson(23,F1,0,1,0,1),*c=mkperson(301,F1,0,0,1,1);
 U8*ruler2=mkperson(11,F2,1,0,0,6),*x2=mkperson(77,F2,0,0,0,1);
 // ---- 0) 순수: 표 만들기 → MSVC 순회·찾기로 그대로 읽힘
 jmp_to(G+am::RVA_ALLOC,(void*)s_alloc);core=&api;game=G;
 {Batch bt{};U32 ids[5]={3,8,15,400,5999};U8 vals[5]={1,0x10,0x12,3,0x30};CHECK(build_map(&bt,ids,vals,5),"표 만들기");
  std::vector<U32> got;for(U8*n=*(U8**)bt.head;n!=bt.head;n=succ(n))got.push_back(*(U32*)(n+0x1c));
  CHECK(got==std::vector<U32>({3,8,15,400,5999}),"게임 순회 방식으로 번호 오름차순 그대로");
  bool f=true;for(int i=0;i<5;i++){U8*n=findk(bt.head,ids[i]);f=f&&n&&n[0x20]==vals[i];}CHECK(f&&!findk(bt.head,9),"게임 찾기 방식으로 각 무장 값 찾힘(없는 번호는 없음)");
  CHECK(bt.size==5&&*(U8**)(bt.head+0x10)==findk(bt.head,5999),"크기·맨 오른쪽 칸 맞음");}
 // ---- 시작 (지문 맞음, 13번 없음, 아직 확인 전)
 int r0=SAN14_ModInitV1(&api);
 jmp_to(G+am::RVA_COMMIT,(void*)s_commit);jmp_to(G+am::RVA_VALID,(void*)s_valid);   // 지문 검사 뒤 게임 함수 자리를 시험 함수로
 CHECK(r0==1&&ver_ok==1&&!merit13,"시작: 실행 파일 지문 OK, 13번 없음");
 CHECK(r0==1&&threads_started>=1,"시작: 불러온 뒤 감소 감시 스레드 1개(게임 코드는 안 바꿈)");
 cur_thread=win_thread_id;   // 게임 창 스레드 = 게임 처리
 CHECK(SAN14_AutoMeritState(0)==W_READY,"R42i2: 지켜보기 단계 없이 첫 턴부터 새 엔진 준비됨");
 int ic0=ic_calls,c0=commits;U64 r=0;
 // ---- 3) 새 엔진: IC 계획 → 게임 논공 확정 함수 (번호 오름차순·값 묶음·주는 사람=플레이어 군주)
 a[0x1c2]=0;a[0x1c3]=0;c[0x1c2]=0;c[0x1c3]=1;*(U16*)(a+0x15e)=0;*(U16*)(c+0x15e)=0;next_rank=2;
 Vec v2=mkvec({{c,2,0,2},{a,2,1,0},{b,2,1,2}});ic0=ic_calls;c0=commits;r=run_exec(v2);
 CHECK(commits==c0+1&&ic_calls==ic0,"새 엔진: 게임 논공 확정 함수 한 번, IC 원래 처리 안 부름");
 CHECK(seen.size()==3&&seen[0].first==23&&seen[1].first==55&&seen[2].first==301,"표: 무장 번호 오름차순(23,55,301)");
 CHECK(seen[0].second==0x21&&seen[1].second==0x01&&seen[2].second==0x20,"표 값: 상위 | 내정<<4 (23=상1·내2, 55=상1, 301=내2)");
 CHECK(seen_giver==ruler1,"논공 주는 사람 = 플레이어 세력 군주");
 CHECK(r==1&&a[0x1c2]==1&&c[0x1c3]==2&&b[0x1c3]==2,"게임 결과가 IC 예상과 같으면 성공(1)");
 // ---- 4) 게임 결과가 IC 예상과 다르면 0 (ICGuard 가 그 무장만 6턴 빼게)
 next_rank=3;a[0x1c2]=1;a[0x1c3]=0;Vec v3=mkvec({{a,2,1,1}});r=run_exec(v3);
 CHECK(r==0&&t_mismatch>=1&&logtext().find("differs_from_ic_plan=1")!=std::string::npos,"결과가 예상과 다르면 0 + 기록(differs_from_ic_plan)");
 next_rank=2;
 // ---- 5) 게임이 거절·아무것도 안 바뀜 → 그 턴만 옛 방식
 commit_apply=false;commit_ret=0;a[0x1c2]=1;a[0x1c3]=0;*(U16*)(a+0x15e)=0;ic0=ic_calls;Vec v4=mkvec({{a,2,1,1}});r=run_exec(v4);
 CHECK(r==1&&ic_calls==ic0+1&&why_count[W_REJECTED]==1,"게임이 거절하고 안 바뀌었으면 IC 원래 처리로 한 번 더");
 commit_apply=true;commit_ret=1;
 // ---- 6) 계획 값 이상(상위+내정>3, 줄어드는 값) → 옛 방식
 ic0=ic_calls;c0=commits;Vec v5=mkvec({{a,2,2,2}});r=run_exec(v5);CHECK(ic_calls==ic0+1&&commits==c0&&why_count[W_TARGET]>=1,"목표 합이 3 넘으면 새 엔진 안 씀");
 a[0x1c2]=2;Vec v6=mkvec({{a,2,1,0}});ic0=ic_calls;r=run_exec(v6);CHECK(ic_calls==ic0+1&&why_count[W_TARGET]>=2,"이미 받은 수보다 줄어드는 목표면 새 엔진 안 씀");
 a[0x1c2]=1;
 // ---- 7) 다른 세력 무장이 섞인 계획 → 옛 방식
 ic0=ic_calls;Vec v7=mkvec({{a,2,1,1},{x2,2,1,0}});r=run_exec(v7);CHECK(ic_calls==ic0+1&&why_count[W_FORCE]==1,"세력 섞인 계획이면 새 엔진 안 씀");
 // ---- 8) 다른 스레드에서 불리면 → 옛 방식
 cur_thread=999;ic0=ic_calls;c0=commits;Vec v8=mkvec({{a,2,1,1}});r=run_exec(v8);CHECK(ic_calls==ic0+1&&commits==c0&&why_count[W_THREAD]==1,"게임 처리 스레드가 아니면 새 엔진 안 씀");
 cur_thread=win_thread_id;
 // ---- 9) 13번 들어오면 → 손 안 댐
 merit13_loaded_flag=true;ic0=ic_calls;c0=commits;r=run_exec(v8);CHECK(ic_calls==ic0+1&&commits==c0&&why_count[W_MERIT13]==1,"13번(논공 가챠) 올라와 있으면 새 엔진 안 씀");
 merit13_loaded_flag=false;
 // ---- 10) 설정 Engine=0 이면 새 엔진 안 씀
 engine_on=0;ic0=ic_calls;c0=commits;r=run_exec(v8);CHECK(ic_calls==ic0+1&&commits==c0&&why_count[W_ENGINE_OFF]==1,"설정에서 Engine=0 이면 IC 원래 처리");
 engine_on=1;
 // ---- 10b) 게임 코드를 한 바이트도 안 바꿈(논공 확정 입구·옛 지켜보기 자리 그대로)
 {static U8 snap_ai[5],snap_pl[5];memcpy(snap_ai,G+0x2ca66,5);memcpy(snap_pl,G+0x6cba7c,5);int ok=1;U8 z[5]={0};ok=!memcmp(snap_ai,z,5)&&!memcmp(snap_pl,z,5);
  CHECK(ok,"옛 지켜보기 자리(0x2ca66·0x6cba7c)를 건드리지 않음");}
 // ---- 11) 불러온 뒤 보상 수가 줄면 기록(새 엔진/옛 방식 구분)
 drop_check();                              // 기준 잡기(200년 5월)
 U8*W2=(U8*)calloc(1,0x100);*(short*)(W2+0x34)=200;W2[0x36]=8;*(U8**)(D+am::DB_WORLD)=W2;   // 다른 월드·더 나중 날짜 = 불러오기
 b[0x1c3]=0;drop_check();
 std::string L=logtext();
 CHECK(L.find("after load officer #23 upper 1->1 lower 2->0 (given by NEW engine)")!=std::string::npos,"불러온 뒤 내정 보상 수가 줄면 'drop' 기록(새 엔진이 준 것)");
 CHECK(L.find("officer #55")==std::string::npos||L.find("#55 upper")==std::string::npos,"안 줄어든 무장은 기록 안 함");
 // ---- 12) 확정 도중 멈춤 대비: Pending=1 이 남아 있으면 다음 시작에서 새 엔진 끔 + 오류창
 ini[L"Pending"]=L"1";disabled=0;int bx0=boxes;
 memcpy(G+am::RVA_COMMIT,am::G_COMMIT,16);memcpy(G+am::RVA_VALID,am::G_VALID,16);SAN14_ModInitV1(&api);
 jmp_to(G+am::RVA_COMMIT,(void*)s_commit);jmp_to(G+am::RVA_VALID,(void*)s_valid);
 CHECK(disabled==1&&ini[L"Disabled"]==L"1"&&ini[L"Pending"]==L"0"&&boxes==bx0+1,"지난번 확정 도중 멈췄으면 새 엔진 끔(Disabled=1)·오류창 띄움(R43 공통 형식)");
 CHECK(logtext().find("R43F\t2\tSAN14_AutoMerit.dll\t")!=std::string::npos&&logtext().find("\tcrash_guard\t")!=std::string::npos&&logtext().find("R43F-2-crash_guard")!=std::string::npos,"R43F 기록 줄(단계 crash_guard·진단 코드)");
 // ---- 13) 판 확인(R42i3): 빌드 시각·크기가 판, 함수 앞부분은 따로
 // 13a 다른 모드가 논공 확정 입구를 jmp 로 우회 → 판 OK, 새 엔진 그대로 씀
 ini.clear();disabled=0;memcpy(G+am::RVA_COMMIT,am::G_COMMIT,16);memcpy(G+am::RVA_VALID,am::G_VALID,16);G[am::RVA_COMMIT]=0xe9;SAN14_ModInitV1(&api);
 CHECK(ver_ok==1&&fn_commit_state==1&&SAN14_AutoMeritState(0)==W_READY,"다른 모드의 jmp 우회 → 판 OK·새 엔진 씀");
 // 13b mov rax,주소; jmp rax 모양 우회도 허용
 memcpy(G+am::RVA_COMMIT,am::G_COMMIT,16);G[am::RVA_COMMIT]=0x48;G[am::RVA_COMMIT+1]=0xb8;G[am::RVA_COMMIT+10]=0xff;G[am::RVA_COMMIT+11]=0xe0;SAN14_ModInitV1(&api);
 CHECK(ver_ok==1&&fn_commit_state==1&&SAN14_AutoMeritState(0)==W_READY,"mov rax/jmp rax 우회도 허용");
 // 13c 알 수 없게 바뀜 → 판은 OK 지만 새 엔진 안 씀(옛 방식). R43: 시작 때 자세한 창 한 번(기대/실제 16바이트), 실행 때는 창 없음
 memcpy(G+am::RVA_COMMIT,am::G_COMMIT,16);G[am::RVA_COMMIT+3]^=0xff;bx0=boxes;SAN14_ModInitV1(&api);
 CHECK(ver_ok==1&&fn_commit_state==2&&SAN14_AutoMeritState(0)==W_FUNC_CHANGED,"함수 앞부분이 알 수 없게 바뀌면 새 엔진 안 씀");
 CHECK(boxes==bx0+1&&logtext().find("\tfuncbytes\t")!=std::string::npos&&logtext().find("41 56 48 83 ec 30 48 83 79 08 00 4c 8b f1 77 09\t4156487c")!=std::string::npos,"R43: 시작 때 오류창 + 기록에 기대/실제 바이트");
 int th1=threads_started;bx0=boxes;ic0=ic_calls;c0=commits;r=run_exec(v8);CHECK(ic_calls==ic0+1&&commits==c0&&threads_started==th1&&boxes==bx0,"그때는 IC 원래 처리, 실행 때 추가 창 없음");
 CHECK(logtext().find("commit=CHANGED(4156487c")!=std::string::npos,"시작 기록에 바뀐 앞 16바이트(hex)");
 // 13d 빌드 시각 다름 = 진짜 다른 게임 판 → 옛 방식. R43: 시작 때 자세한 창(기대/실제 stamp·size), 실행 때는 추가 창 없음
 memcpy(G+am::RVA_COMMIT,am::G_COMMIT,16);*(U32*)(G+0x108)=am::STAMP+1;bx0=boxes;SAN14_ModInitV1(&api);
 CHECK(ver_ok==0&&SAN14_AutoMeritState(0)==W_VERSION,"빌드 시각이 다르면(다른 게임 판) 새 엔진 안 씀");
 CHECK(boxes==bx0+1&&logtext().find("\tversion\t")!=std::string::npos&&logtext().find("stamp=0x6ad06f54 size=0x2a02000")!=std::string::npos&&logtext().find("R43F-2-version")!=std::string::npos,"R43: 시작 때 오류창 + 기록에 실제 stamp/size·진단 코드");
 int th0b=threads_started;bx0=boxes;ic0=ic_calls;r=run_exec(v8);run_exec(v8);
 CHECK(ic_calls==ic0+2&&threads_started==th0b&&boxes==bx0,"판 다를 때 IC 원래 처리 + 실행 때 추가 창 없음");
 *(U32*)(G+0x108)=am::STAMP;
 // ---- 14) 기록 줄
 L=logtext();
 CHECK(L.find("R42D\t2\tSAN14_AutoMerit.dll\tstart\tversion=OK")!=std::string::npos,"시작 기록");
 CHECK(L.find("code_patch=none")!=std::string::npos,"시작 기록에 게임 코드 안 바꿈 표시");
 CHECK(L.find("\texec\tnative commit officers=3 result=1 differs_from_ic_plan=0")!=std::string::npos,"새 엔진 실행 기록");
 CHECK(L.find("\tfallback\treason=12 ic_plan_target_invalid")!=std::string::npos,"옛 방식 이유 기록");
 printf("\n자동논공 R42i4 네이티브 시험: 통과 %d / 실패 %d\n",pass,fail);
 (void)ruler2;(void)x2;return fail?1:0;
}
