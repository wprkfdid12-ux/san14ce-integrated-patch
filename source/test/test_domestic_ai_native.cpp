// 22번 AI 내정 순환(군주 반영) — 리눅스 네이티브 시험기. 모듈 소스(src/domestic_ai.cpp)를 그대로 컴파일해 가짜 게임 메모리 위에서 돌린다.
//  빌드/실행: clang++ -std=c++17 -O1 -I test/shim -I src test/test_domestic_ai_native.cpp src/domestic_ai.cpp -o /tmp/t_dai && /tmp/t_dai
//  확인: 연결 자리 바이트 검사, 플레이어 지역은 원래 함수로, AI 지역은 규칙 결과, 달마다 섞임, 월별 기록 줄, 스위치 끔, 다른 모드가 노드를 바꾼 경우.
#include "common.h"
#include "domestic_ai_rule.h"
#include <sys/mman.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <unistd.h>
#include <sys/stat.h>
// ---- 윈도 API 흉내
static std::string root_dir;
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p=root_dir+"\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string s=narrow(p);mkdir(s.c_str(),0755);return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=2;t->wHour=21;t->wMinute=0;t->wSecond=0;}
static std::map<std::string,std::string> ini;
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*s,const wchar_t*k,const wchar_t*v,const wchar_t*f){ini[narrow(f)+"|"+narrow(s)+"|"+narrow(k)]=narrow(v);return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*s,const wchar_t*k,int d,const wchar_t*f){auto it=ini.find(narrow(f)+"|"+narrow(s)+"|"+narrow(k));return it==ini.end()?(unsigned)d:(unsigned)atoi(it->second.c_str());}
extern "C"{unsigned run_scenic(U8*corps,int gold,int cost,unsigned long*clobber);extern volatile LONG DaiScOn,DaiScBlocked,DaiScPassed;extern long long DaiScMult,DaiScFloor;extern void*DaiScPass;extern void*DaiScFail;unsigned run_orders(U8*force,unsigned val,unsigned long*clobber);extern volatile LONG DaiOrdOn,DaiOrdHits;extern void*DaiOrdRet;void DaiOrdBridge();void DaiOrdSwitch();}
static int boxes;extern "C" int MessageBoxW(void*,const wchar_t*t,const wchar_t*,unsigned){boxes++;printf("  [오류창] %s\n",narrow(t).c_str());return 1;}
extern "C" HMODULE GetModuleHandleW(const wchar_t*){return nullptr;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
// ---- 모듈 쪽 심볼
extern "C" int SAN14_ModInitV1(const S14_CoreAPI*);
extern "C" void DomesticAiSelect(U8*,U8*,int*,int,int,int);
extern "C" int SAN14_DomesticAIGet(int);extern "C" int SAN14_DomesticAISet(int,int);
// ---- 가짜 게임
static U8*game;static U8*db_;static U8*world_;static U8*areas;static std::set<U8*> live;
static U8*persons[0x1771];static U8*forces[0x34];static U8*tiles[0xbd10];
static void thunk(U32 rva,void*target){U8*p=game+rva;p[0]=0x48;p[1]=0xb8;memcpy(p+2,&target,8);p[10]=0xff;p[11]=0xe0;}
static int g_valid(U8*p){return p&&live.count(p)?1:0;}
static U8*g_area_force(U8*a){U16 hex=at<U16>(a,0x3a);U8*t=tiles[hex];U8 f=at<U8>(t,0x14);return forces[f<=0x33?f:0];}
static U8*g_player_force(U8*w){return forces[at<U8>(w,0x3a)];}
static int g_ability(U8*p,int idx,int flags){assert(flags==0xfff);return at<U8>(p,0xec+idx);}
struct OrigCall{U8*district,*area;int*out;int p1,p2,p3;};static std::vector<OrigCall> orig_calls;
static void g_orig_select(U8*d,U8*a,int*out,int p1,int p2,int p3){orig_calls.push_back({d,a,out,p1,p2,p3});*out=7;}
static std::vector<std::string> core_log;
static I32 api_readable(const void*,UPTR){return 1;}
static I32 api_install(const char*id,const S14_Hook*h,U32 n){printf("  InstallHooks(%s, %u)\n",id,n);for(U32 i=0;i<n;i++){if(memcmp(h[i].target,h[i].expected,h[i].length)){printf("  !! expected bytes differ\n");return 0;}}return 1;}
static U8*relay_next;static U8*api_relay(const void*,const void*dest){U8*r=relay_next;relay_next+=16;thunk((U32)(r-game),(void*)dest);return r;}
static void api_free(void*){}
static void api_log(const char*s){core_log.push_back(s);printf("  [log] %s\n",s);}
static U8*make_person(unsigned id,int politics,int gold,int food,int troops){U8*p=(U8*)calloc(1,0x200);at<U16>(p,0x10)=(U16)id;at<U8>(p,0xef)=(U8)politics;at<short>(p,0x170)=(short)gold;at<short>(p,0x172)=(short)food;at<short>(p,0x174)=(short)troops;persons[id]=p;live.insert(p);return p;}
static U8*make_force(unsigned id,unsigned ruler){U8*f=(U8*)calloc(1,0x200);at<U8*>(f,0)=game+0x13a9880;at<U16>(f,0x10)=(U16)ruler;forces[id]=f;live.insert(f);return f;}
static U8*area(unsigned idx){return areas+idx*0x98;}
static void set_area(unsigned idx,unsigned force,unsigned c,unsigned a,unsigned b,unsigned focus,unsigned officer){
 U8*ar=area(idx);U16 hex=(U16)(1000+idx);at<U16>(ar,0x3a)=hex;U8*t=tiles[hex];if(!t){t=(U8*)calloc(1,0x40);tiles[hex]=t;}at<U8>(t,0x14)=(U8)force;
 at<U16>(ar,0x3c)=(U16)c;at<U16>(ar,0x3e)=(U16)a;at<U16>(ar,0x40)=(U16)b;at<U8>(ar,0x72)=(U8)focus;at<U16>(ar,0x36)=(U16)officer;live.insert(ar);
}
static void set_date(int year,int month){at<short>(world_,0x34)=(short)year;at<U8>(world_,0x36)=(U8)month;}
static std::vector<std::string> log_lines(){std::vector<std::string> v;std::ifstream f(root_dir+"/"+narrow(L"SAN14_\xAE30\xB85D")+"/"+narrow(L"\xC801\xC6A9\xD655\xC778.log"));std::string l;while(std::getline(f,l))v.push_back(l);return v;}
static std::vector<std::string> split(const std::string&s,char d){std::vector<std::string> v;std::stringstream ss(s);std::string x;while(std::getline(ss,x,d))v.push_back(x);return v;}
static int fails;
#define CHECK(c,msg) do{if(c)printf("PASS %s\n",msg);else{printf("FAIL %s\n",msg);fails++;}}while(0)
int main(){
 char tmpl[]="/tmp/dai_XXXXXX";root_dir=mkdtemp(tmpl);
 size_t SZ=S14_GAME_IMAGE+0x10000;game=(U8*)mmap(nullptr,SZ,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(game!=MAP_FAILED);
 relay_next=game+0x29f0000;
 db_=(U8*)calloc(1,0x90000);world_=(U8*)calloc(1,0x1000);areas=(U8*)calloc(1,501*0x98);
 at<U8*>(game,0x2780f60)=db_;at<U8*>(db_,0x871e8)=world_;at<U8*>(db_,0x6caf0)=areas;live.insert(world_);
 for(unsigned i=0;i<=0x1770;i++)at<U8*>(db_,0x158+i*8)=nullptr;
 at<U8*>(game,0x134f918)=game+0x7b0b0;
 static const U8 site[5]={0xe8,0x70,0x79,0x03,0x00};memcpy(game+0x7b17b,site,5);
 static const U8 osite[7]={0x41,0x3b,0xc7,0x41,0x0f,0x4f,0xc7};memcpy(game+0x9fa17,osite,7);   // 11차: 명령서 채우기 자리(cmp eax,r15d / cmovg eax,r15d)   // R43: 0x7b17b+5+rel = 0xb2af0
 static const U8 scsite[5]={0x41,0x3b,0xc6,0x7c,0x06};memcpy(game+0x3e9c6,scsite,5);   // 21차: AI 명승 투자 금 비교(cmp eax,r14d / jl +6)
 thunk(0x331780,(void*)&g_valid);thunk(0x22e260,(void*)&g_area_force);thunk(0x330b90,(void*)&g_player_force);thunk(0x231770,(void*)&g_ability);thunk(0xb2af0,(void*)&g_orig_select);
 // 인물·세력: 1=플레이어 군주(유비 78 병사), 2=조조(94 군량), 3=여포(13 병사), 4=도겸(64 금), 5=유장(38 금)
 make_person(100,78,0,0,20);make_person(200,94,0,20,0);make_person(300,13,0,0,20);make_person(400,64,20,0,0);make_person(500,38,20,0,0);
 for(unsigned i=0;i<=0x1770;i++)at<U8*>(db_,0x158+i*8)=persons[i];
 make_force(1,100);make_force(2,200);make_force(3,300);make_force(4,400);make_force(5,500);
 for(unsigned i=0;i<=0x33;i++)at<U8*>(db_,0xdf30+i*8)=forces[i];
 at<U8>(world_,0x3a)=1;   // 플레이어 세력 1
 // 지역 1..338: 세력을 돌아가며 배정(1~5), 개발치 0, 중점 1, 담당관 없음
 for(unsigned i=1;i<=338;i++)set_area(i,1+(i%5),0,0,0,1,0);
 for(unsigned h=0;h<0xbd10;h++)at<U8*>(db_,0xe270+h*8)=tiles[h];
 set_date(194,1);
 S14_Resolved res{};res.size=sizeof res;res.abi=S14_ABI;res.layout_abi=1;res.image_size=S14_GAME_IMAGE;res.image=game;
 S14_CoreAPI api{};api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=api_readable;api.InstallHooks=api_install;api.MakeRelay=api_relay;api.FreeRelay=api_free;api.Log=api_log;
 int ok=SAN14_ModInitV1(&api);
 CHECK(ok==1,"ModInit: 연결 자리(0x7b17b e8 70 79 03 00) 확인하고 설치");
 {auto L=log_lines();CHECK(L.size()==2&&L[0].rfind("R42D\t22\tSAN14_DomesticAI.dll\torders_start\t",0)==0&&L[0].find("\tOK\t1/1\tAiOrders=1")!=std::string::npos&&L[1].rfind("R42D\t22\tSAN14_DomesticAI.dll\tstart\t",0)==0&&L[1].find("\tOK\t1/1\tAiRotate=1\t-")!=std::string::npos,"시작 줄 R42D 22 start OK 1/1 AiRotate=1 (설치기 R39 판정과 겹치지 않음)");
 {auto L=log_lines();bool r39=false;for(auto&l:L)if(l.rfind("R39\t",0)==0)r39=true;CHECK(!r39,"R39 줄은 쓰지 않음 → 설치기 22번 판정은 예전 방식(DomesticCE ACTIVE) 그대로");}}
 // 1) 플레이어 지역 → 원래 함수 그대로(결과 7, 인자 전달)
 {int out=-1;U8 dist[0x40]={};orig_calls.clear();DomesticAiSelect(dist,area(5),&out,11,22,33);   // 5%5==0 → 세력 1 = 플레이어
  CHECK(orig_calls.size()==1&&out==7&&orig_calls[0].district==dist&&orig_calls[0].area==area(5)&&orig_calls[0].p1==11&&orig_calls[0].p2==22&&orig_calls[0].p3==33,"플레이어 지역: 원래 판정 함수로 인자 6개 그대로 넘김");}
 // 2) 조조(정치 94, 군량) 지역: 12달 내내 전부 순환. 개발 0이면 상업(1) 부터
 {unsigned rot=0,total=0;orig_calls.clear();
  for(int m=1;m<=12;m++){set_date(194,m);for(unsigned i=1;i<=338;i++){if(1+(i%5)!=2)continue;int out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);total++;if(out==1)rot++;}}
  CHECK(orig_calls.empty()&&rot==total&&total>0,"조조(정치 94): 모든 지역·모든 달이 순환(상업부터)");}
 // 3) 여포(정치 13, 병사) 지역: 순환은 3분의 1쯤, 나머지는 병영(3). 달마다 섞이고 같은 달 안에서는 고정
 {std::vector<unsigned> ids;for(unsigned i=1;i<=338;i++)if(1+(i%5)==3)ids.push_back(i);
  std::vector<std::set<unsigned>> rotset(13);unsigned rot=0,fix=0;
  for(int m=1;m<=12;m++){set_date(194,m);for(unsigned i:ids){int out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);int again=-1;DomesticAiSelect(nullptr,area(i),&again,1,1,1);assert(again==out);
    if(out==1){rot++;rotset[m].insert(i);}else if(out==3)fix++;else{printf("  이상한 결과 %d\n",out);fails++;}}}
  double share=100.0*rot/(rot+fix);unsigned changed=0;for(int m=2;m<=12;m++)if(rotset[m]!=rotset[m-1])changed++;
  printf("  여포 순환 비율 %.1f%% (목표 33), 달 바뀔 때 구성 달라진 횟수 %u/11\n",share,changed);
  CHECK(share>25&&share<41,"여포(정치 13): 순환 지역이 3분의 1 안팎");CHECK(changed==11,"여포: 순환 지역 구성이 달마다 바뀜");
  // 1년 전체로 보면 거의 모든 지역이 한 번 이상 순환 지역이 됨
  std::set<unsigned> any;for(int m=1;m<=12;m++)any.insert(rotset[m].begin(),rotset[m].end());
  CHECK(any.size()>=ids.size()*9/10,"여포: 1년 동안 지역 90% 이상이 한 번은 순환");}
 // 4) 도겸(정치 64, 금) → 절반 순환, 나머지 상업(1). 개발치가 상업 100·농업 0 이면 순환 지역은 농업(2)
 {std::vector<unsigned> ids;for(unsigned i=1;i<=338;i++)if(1+(i%5)==4)ids.push_back(i);
  for(unsigned i:ids)set_area(i,4,100,0,0,1,0);
  unsigned rot=0,fix=0;for(int m=1;m<=12;m++){set_date(195,m);for(unsigned i:ids){int out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);if(out==2)rot++;else if(out==1)fix++;else fails++;}}
  double share=100.0*rot/(rot+fix);printf("  도겸 순환 비율 %.1f%% (목표 50)\n",share);
  CHECK(share>42&&share<58,"도겸(정치 64): 절반 순환(농업), 나머지 상업 몰빵");}
 // 5) 유장(정치 38, 금): 상업이 1000이면 몰빵 지역도 순환 규칙으로(농업 0 → 2)
 {unsigned i=0;for(unsigned k=1;k<=338;k++)if(1+(k%5)==5){i=k;break;}
  set_area(i,5,1000,0,0,1,0);int out=-1;set_date(196,3);DomesticAiSelect(nullptr,area(i),&out,1,1,1);CHECK(out==2,"유장(금): 상업 1000이면 순환 규칙(농업)으로");
  set_area(i,5,1000,1000,1000,3,0);out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);CHECK(out==0,"셋 다 1000이면 0(바꿀 것 없음)");}
 // 6) 결과가 지금 중점과 같고 담당관이 있으면 0, 담당관이 없으면 그 값
 {unsigned i=0;for(unsigned k=1;k<=338;k++)if(1+(k%5)==2){i=k;break;}
  set_area(i,2,0,0,0,1,200);int out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);CHECK(out==0,"조조: 중점 상업 그대로 + 담당관 있음 → 0");
  set_area(i,2,0,0,0,1,0);out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);CHECK(out==1,"조조: 담당관 없음 → 1(임명하게 값 돌려줌)");
  set_area(i,2,0,0,0,2,200);out=-1;DomesticAiSelect(nullptr,area(i),&out,1,1,1);CHECK(out==1,"조조: 중점 농업인데 규칙은 상업 → 1");}
 // 7) 월별 기록 줄: 달이 바뀔 때 직전 달 요약 한 줄 (ai = rot+fix)
 {auto before=log_lines().size();set_date(197,1);int out;DomesticAiSelect(nullptr,area(2),&out,1,1,1);set_date(197,2);DomesticAiSelect(nullptr,area(2),&out,1,1,1);
  auto L=log_lines();CHECK(L.size()>before,"달이 바뀌면 기록 줄이 생김");
  std::string last=L.back();auto f=split(last,'\t');bool fmt=f.size()>=12&&f[0]=="R42D"&&f[1]=="22"&&f[3]=="month"&&f[4]=="197/1";
  std::map<std::string,long> kv;for(auto&x:f){auto p=x.find('=');if(p!=std::string::npos)kv[x.substr(0,p)]=atol(x.substr(p+1).c_str());}
  printf("  %s\n",last.c_str());
  CHECK(fmt&&kv["ai"]==kv["rot"]+kv["fix"]&&kv["calls"]==kv["ai"]+kv["player"]+kv["orig"],"월별 줄 형식: R42D 22 month 연/월 calls=ai+player+orig, ai=rot+fix");
  CHECK(last.find("forces=")!=std::string::npos&&last.find("3:13:33:3:")!=std::string::npos,"월별 줄에 세력별 군주 정치·순환비율·물자 (3:13:33:3)");}
 // 8) 스위치 끔 → 원래 함수로, ini 저장
 {CHECK(SAN14_DomesticAISet(0,0)==1&&SAN14_DomesticAIGet(0)==0,"스위치 끔: Set/Get");orig_calls.clear();int out=-1;DomesticAiSelect(nullptr,area(2),&out,1,1,1);
  CHECK(orig_calls.size()==1&&out==7,"스위치 꺼지면 AI 지역도 원래 판정");
  CHECK(ini[root_dir+"/SAN14Mods/modules/DomesticAI.ini|DomesticAI|AiRotate"]=="0","ini AiRotate=0 저장");SAN14_DomesticAISet(0,1);}
 // 9) 다른 AI 모드가 노드를 통째로 바꾼 경우 → 설치는 되지만 시작 줄 IDLE
 {at<U8*>(game,0x134f918)=game+0x123456;auto before=log_lines().size();int ok2=SAN14_ModInitV1(&api);auto L=log_lines();
  CHECK(ok2==1&&L.size()==before+2&&L.back().find("\tOK\t1/1\t")!=std::string::npos&&L.back().find("AI node wrapped")!=std::string::npos,"다른 모드가 노드를 감싸 두면 OK + 알림 문구로 기록");}
 // 10) 연결 자리 바이트가 다르면 설치 실패 + 오류창
 {game[0x7b17c]=0x11;auto before=log_lines().size();int ok3=SAN14_ModInitV1(&api);auto L=log_lines();
  CHECK(ok3==0&&boxes==1&&L.size()==before+3&&L[L.size()-2].find("\tFAIL\t0/1\t")!=std::string::npos,"바이트 다르면 FAIL 기록 + 오류창 1번");
  CHECK(L.back().rfind("R43F\t22\tSAN14_DomesticAI.dll\t",0)==0&&L.back().find("\thook\t")!=std::string::npos&&L.back().find("exe+0x7b17b (bytes differ)")!=std::string::npos&&L.back().find("\te8 ")!=std::string::npos&&L.back().find("\te8 11 ")!=std::string::npos&&L.back().find("R43F-22-hook")!=std::string::npos,"R43F 줄: 자리·기대/실제 바이트·진단 코드");}
 { // 11차 P212: AI 명령서 99장 — 실제 브리지 코드를 게임의 채우기 자리처럼 돌림(rbp=세력, eax=새 값, r15d=200)
   // 16차(P220): 명령서는 군단(CDistrictData) 객체에 있다 — 군단 k(1..5)=세력 k, 군단 6=플레이어 세력(1)의 둘째 군단. 세력 객체+0x14 는 건드리면 안 됨.
   static U8*dists[0x34];for(unsigned k=0;k<=0x33;k++){U8*o=(U8*)calloc(1,0x40);at<U8*>(o,0)=game+0x13a98f0;live.insert(o);dists[k]=o;at<U8*>(db_,0xe0d0+k*8)=o;}
   for(unsigned k=7;k<=0x33;k++)at<U8*>(db_,0xe0d0+k*8)=dists[0];   // 안 쓰는 칸은 게임처럼 0번 객체
   for(unsigned k=1;k<=5;k++)at<U8>(dists[k],0x10)=(U8)k;at<U8>(dists[6],0x10)=1;for(int f=1;f<=5;f++)at<U8>(forces[f],0x14)=60;
   unsigned long cl[2];DaiOrdRet=nullptr;
   at<U8>(world_,0x3a)=1;InterlockedExchange(&DaiOrdOn,1);InterlockedExchange(&DaiOrdHits,0);
   CHECK(run_orders(dists[2],8,cl)==99&&cl[0]==0x1111&&cl[1]==0x2222,"AI 세력(조조): 채우는 순간 8 → 99, rcx·rdx 그대로");
   CHECK(run_orders(dists[1],8,cl)==8,"플레이어 세력: 게임 값 그대로(8)");
   CHECK(run_orders(dists[1],250,cl)==200&&run_orders(dists[3],250,cl)==99,"게임의 200 상한 명령은 그대로 실행(플레이어 250→200, AI 는 99)");
   CHECK(DaiOrdHits==2,"99 로 넣은 횟수 기록 2");
   InterlockedExchange(&DaiOrdOn,0);CHECK(run_orders(dists[2],8,cl)==8&&DaiOrdHits==2,"스위치 끔(AiOrders=0): AI 도 게임 값 그대로");
   InterlockedExchange(&DaiOrdOn,1);at<U8>(world_,0x3a)=0;CHECK(run_orders(dists[1],5,cl)==99&&run_orders(dists[2],5,cl)==99,"데모(플레이어 없음): 모든 세력 99");
   at<U8>(world_,0x3a)=1;U8*keep=at<U8*>(game,0x2780f60);at<U8*>(game,0x2780f60)=nullptr;CHECK(run_orders(dists[2],8,cl)==8,"게임 자료가 아직 없으면 손 안 댐(튕기지 않음)");at<U8*>(game,0x2780f60)=keep;
   // 13차: AI 세력이 내정 판단을 할 때 그 세력 명령서가 99 보다 적으면 99 로. 플레이어 세력·스위치 끔은 그대로
   { at<U8>(world_,0x3a)=1;InterlockedExchange(&DaiOrdOn,1);for(int f=1;f<=6;f++)at<U8>(dists[f],0x14)=5;int o=0;
     unsigned ai_area=0,pl_area=0;for(unsigned i=1;i<=338;i++){if(!ai_area&&1+(i%5)==2)ai_area=i;if(!pl_area&&1+(i%5)==1)pl_area=i;}
     DomesticAiSelect(nullptr,areas+ai_area*0x98,&o,0,0,0);CHECK(at<U8>(dists[2],0x14)==99&&at<U8>(dists[3],0x14)==99&&at<U8>(dists[4],0x14)==99&&at<U8>(dists[5],0x14)==99&&at<U8>(dists[1],0x14)==5,"AI 세력 하나가 판단할 때 AI 세력 전부 5 → 99, 플레이어는 그대로(14차)");
     DomesticAiSelect(nullptr,areas+pl_area*0x98,&o,0,0,0);CHECK(at<U8>(dists[1],0x14)==5,"플레이어 지역 판단: 플레이어 명령서 그대로(5)");
     InterlockedExchange(&DaiOrdOn,0);at<U8>(dists[2],0x14)=7;DomesticAiSelect(nullptr,areas+ai_area*0x98,&o,0,0,0);CHECK(at<U8>(dists[2],0x14)==7,"스위치 끔: AI 도 그대로");InterlockedExchange(&DaiOrdOn,1);
     at<U8>(dists[2],0x14)=150;DomesticAiSelect(nullptr,areas+ai_area*0x98,&o,0,0,0);CHECK(at<U8>(dists[2],0x14)==150,"99 이상이면 안 건드림"); }
     { bool same=true;for(int f=1;f<=5;f++)if(at<U8>(forces[f],0x14)!=60)same=false;CHECK(same&&at<U8>(dists[6],0x14)==5&&at<U8>(dists[0],0x14)==0,"세력 객체+0x14 는 안 건드림 · 플레이어 둘째 군단도 그대로 · 0번 군단 그대로(16차)");
       unsigned long c2[2];CHECK(run_orders(dists[6],8,c2)==8&&run_orders(dists[0],8,c2)==8,"채우기 자리: 플레이어 둘째 군단·세력 0 군단은 게임 값 그대로(16차)"); }
   // 21차: AI 명승 투자 금 조건(브리지 exe+0x3e9c6) — 남는 금 ≥ 비용×2 이고 ≥ 5000 일 때만, 플레이어 세력·세력 0 군단은 게임 규칙(금 ≥ 비용)
 { unsigned long cl[2];
   CHECK(DaiScMult==2&&DaiScFloor==5000,"기본값: 배수 2, 최소 5000");
   InterlockedExchange(&DaiScOn,1);LONG b0=DaiScBlocked;
   CHECK(run_scenic(dists[2],3000,1000,cl)==0,"AI: 금 3000, 비용 1000 → 남는 2000 < 5000 → 막음");
   CHECK(run_scenic(dists[2],6000,1000,cl)==1&&cl[0]==0x1111&&cl[1]==0x2222,"AI: 금 6000, 비용 1000 → 남는 5000 ≥ 5000·2000 → 통과, rcx·rdx 그대로");
   CHECK(run_scenic(dists[2],5999,1000,cl)==0,"AI: 남는 4999 → 막음(경계)");
   CHECK(run_scenic(dists[2],12000,4000,cl)==1&&run_scenic(dists[2],11999,4000,cl)==0,"AI: 비싼 명승(4000)은 남는 금 8000(=비용×2) 필요");
   CHECK(run_scenic(dists[2],500,1000,cl)==0,"AI: 금이 비용보다 적으면 원래대로 실패");
   CHECK(DaiScBlocked-b0==3,"막은 횟수 = 금은 있었는데 조건 때문에 막은 경우만(3)");
   CHECK(run_scenic(dists[1],1200,1000,cl)==1&&run_scenic(dists[1],900,1000,cl)==0,"플레이어 세력 군단: 게임 규칙 그대로(금 ≥ 비용이면 통과)");
   CHECK(run_scenic(dists[6],1200,1000,cl)==1,"플레이어의 둘째 군단(위임 포함)도 게임 규칙 그대로");
   CHECK(run_scenic(dists[0],1200,1000,cl)==1,"세력 번호 0 군단: 게임 규칙 그대로");
   InterlockedExchange(&DaiScOn,0);CHECK(run_scenic(dists[2],1200,1000,cl)==1,"스위치 끔(AiScenicGold=0): AI 도 게임 규칙");
   ini[root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicFloor"]="8000";ini[root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicMult"]="1";DaiOrdSwitch();
   CHECK(DaiScFloor==8000&&DaiScMult==1,"ini 로 최소·배수 바꿈");
   ini[root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicFloor"]="-5";ini[root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicMult"]="99";DaiOrdSwitch();
   CHECK(DaiScFloor==0&&DaiScMult==20,"이상한 값은 범위로 자름(최소 0~150000, 배수 0~20)");
   ini.erase(root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicFloor");ini.erase(root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicMult");DaiOrdSwitch(); }
     // 17차(P223): 14·15차가 세력+0x14(1번 세력과의 우호도)에 써 넣은 99 되돌리기 — 1번 세력 쪽 칸에 남은 원래 값으로
     { auto rel=[&](unsigned a,unsigned b)->U8&{return at<U8>(forces[a],0x13+b);};
       auto reset=[&](){for(unsigned a=1;a<=5;a++)for(unsigned b=1;b<=5;b++)rel(a,b)=(a==b)?0:(U8)(40+a+b);};   // 양쪽이 같은 표
       auto month=[&](int y,int m){set_date(y,m);int o=0;DomesticAiSelect(nullptr,areas+2*0x98,&o,0,0,0);};
       InterlockedExchange(&DaiOrdOn,0);
       reset();for(unsigned a=1;a<=5;a++)rel(a,1)=99;month(300,1);
       bool back=true;for(unsigned a=2;a<=5;a++)if(rel(a,1)!=40+a+1)back=false;
       CHECK(back&&rel(1,1)==0,"흔적(전 세력의 1번 칸 99) → 1번 세력 쪽 값으로 되돌림, 1번 자기 칸도");
       bool rest=true;for(unsigned a=2;a<=5;a++)for(unsigned b=2;b<=5;b++)if(rel(a,b)!=((a==b)?0:(U8)(40+a+b)))rest=false;for(unsigned b=2;b<=5;b++)if(rel(1,b)!=41+b)rest=false;
       CHECK(rest,"다른 칸은 하나도 안 바뀜");
       rel(3,1)=77;month(300,1);CHECK(rel(3,1)==77,"같은 달에는 다시 안 봄");month(300,2);CHECK(rel(3,1)==44,"달이 바뀌면 한 번 더 보고 맞춤");
       reset();rel(2,3)=10;for(unsigned a=1;a<=5;a++)rel(a,1)=99;month(300,3);
       CHECK(rel(4,1)==99&&rel(2,3)==10,"다른 세력 쌍이 원래 안 맞는 세이브면 손 안 댐(기록만)");
       reset();month(300,4);bool clean=true;for(unsigned a=1;a<=5;a++)for(unsigned b=1;b<=5;b++)if(rel(a,b)!=((a==b)?0:(U8)(40+a+b)))clean=false;CHECK(clean,"멀쩡한 세이브는 아무것도 안 바꿈");
       {auto L=log_lines();int n=0,sk=0;for(auto&l:L)if(l.find("\trelfix\t")!=std::string::npos){n++;if(l.find("skipped")!=std::string::npos)sk++;}CHECK(n==3&&sk==1,"되돌릴 때·건너뛸 때만 relfix 한 줄(멀쩡하면 없음)");}
       InterlockedExchange(&DaiOrdOn,1);set_date(194,12); }
   // 고치기 전 방식(주기적으로 99 를 써 넣음)이 왜 안 됐는지: 게임 규칙 min(지금+1, 세력 최대) 가 99 를 세력 최대로 깎음
   unsigned polled=99,maxf=8,next=polled+1<maxf?polled+1:maxf;CHECK(next==8,"옛 방식 재현: 99 를 써 둬도 게임이 채울 때 세력 최대(8)로 깎임");
   // 스위치는 옛 DomesticCE 와 같은 ini 칸
   ini[root_dir+"/SAN14_DomesticCE.ini|Automation|AiOrders"]="0";DaiOrdSwitch();bool off=DaiOrdOn==0;ini.erase(root_dir+"/SAN14_DomesticCE.ini|Automation|AiOrders");DaiOrdSwitch();
   CHECK(off&&DaiOrdOn==1,"SAN14_DomesticCE.ini AiOrders=0 이면 꺼지고, 없으면 켜짐"); }
 // 24차: 모드 설정 창 'AI 명승 투자 금 제한' 스위치(which 5) — SAN14_DomesticCE.ini [Automation] AiScenicGold, 달마다 다시 읽어도 유지
 {CHECK(SAN14_DomesticAIGet(5)==1,"명승 금 제한 Get(5) = 켬(기본)");
  CHECK(SAN14_DomesticAISet(5,0)==1&&SAN14_DomesticAIGet(5)==0&&DaiScOn==0&&ini[root_dir+"/SAN14_DomesticCE.ini|Automation|AiScenicGold"]=="0","명승 금 제한 끔: 바로 꺼지고 ini 에 0");
  DaiOrdSwitch();CHECK(DaiScOn==0,"달 바뀜 다시 읽기 뒤에도 꺼짐 유지");
  CHECK(SAN14_DomesticAISet(5,1)==1&&SAN14_DomesticAIGet(5)==1&&DaiScOn==1,"다시 켬");DaiOrdSwitch();CHECK(DaiScOn==1,"다시 읽기 뒤 켬 유지");}
 printf("\n%s (%d 실패)\n",fails?"실패":"모두 통과",fails);return fails?1:0;
}

__asm__(".text\n.globl run_orders\nrun_orders:\n"
" push %rbp\n push %r15\n push %r12\n mov %rdx,%r12\n mov %rdi,%rbp\n mov %esi,%eax\n mov $200,%r15d\n mov $0x1111,%rcx\n mov $0x2222,%rdx\n"
" lea 1f(%rip),%r8\n mov %r8,DaiOrdRet(%rip)\n jmp DaiOrdBridge\n"
"1: mov %rcx,0(%r12)\n mov %rdx,8(%r12)\n pop %r12\n pop %r15\n pop %rbp\n ret\n");

__asm__(".text\n.globl run_scenic\nrun_scenic:\n"
" push %rsi\n push %r14\n push %r12\n push %rbx\n mov %rcx,%r12\n mov %esi,%r9d\n mov %rdi,%rsi\n mov %edx,%r14d\n"
" lea 1f(%rip),%r8\n mov %r8,DaiScPass(%rip)\n lea 2f(%rip),%r8\n mov %r8,DaiScFail(%rip)\n mov $0x1111,%rcx\n mov $0x2222,%rdx\n"
" mov %r9d,%eax\n jmp DaiScenicBridge\n"
"1: mov $1,%r8d\n jmp 3f\n2: xor %r8d,%r8d\n3: mov %rcx,0(%r12)\n mov %rdx,8(%r12)\n mov %r8d,%eax\n pop %rbx\n pop %r12\n pop %r14\n pop %rsi\n ret\n");
