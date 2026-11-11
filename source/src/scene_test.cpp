// R42 C 시험 도구 SAN14_SceneTest.dll — 시험판 전용(본판 설치기·설정 창에 안 들어감)
//  게임 옆 작은 창(맨 위) 버튼 + Ctrl+Alt+숫자 단축키. 버튼은 '요청'만 쌓고, 실제 일은 게임 상태 스레드
//  (플레이어 전략 상태 CUserStrategyState Tick, vtable 칸 0x13d9458 — 다른 모듈 설치 뒤 그 위에 이어 붙임)에서 함.
//  모든 버튼: ① 비어 있는 수동 세이브 칸 찾기(게임 CSaveLoadManager 칸 정보 0x935ac0/0x9352e0 + Steam 원격 파일 확인 보조)
//            ② 게임 자체 저장(CSaveLoadState 새 칸 저장과 같은 길: 요청 0x33aee0 → CSaveState 쌓기 0x466530)
//            ③ 저장 확인(칸 정보에 머리 생김) 뒤 ④ 시험 실행. 빈 칸 없거나 저장 확인 못 하면 시험 안 하고 기록만.
//  결과는 게임 폴더 SAN14_기록\적용확인.log 의 'R42T' 줄(사용자에게 txt 만들게 하지 않음).
//  버튼: 1 재해(B1) 2 등용 결과(B2) 3 보물 발견(B3) 4 논공 저장정보 오류(13번) 5 22번 범위 밖 지역 6 명품 창 7 모드 설정 창
//        8 처단(B4) 상태 기록 9 개성 9칸(A10) 상태 기록 10(Ctrl+Alt+0) 카메라 막기 — 8·9·10 은 데이터를 안 바꾸므로 저장 없이
#include "common.h"
#include "r39_record.h"
using U64=uint64_t;
namespace {
const S14_CoreAPI*core;U8*game;
volatile long req;          // 0 없음, 1..8 버튼
int stage;long cur;int slot=-1;ULONGLONG t0,lastCheck;wchar_t status[256]=L"대기 중";
volatile long testMode;     // 단위시험: 창 안 띄움
HWND hwnd;
// ---- 기록 ----
void log(int test,const char*msg){
 char line[1300];line[0]=0;r39::scat(line,1300,"R42T\t");r39::snum(line,1300,(unsigned)test);r39::scat(line,1300,"\tSAN14_SceneTest.dll\t");r39::stamp(line,1300);r39::scat(line,1300,"\t");r39::scat(line,1300,msg);
 r39::append_line(line);if(core&&core->Log){char m[1360];m[0]=0;r39::scat(m,1360,"SceneTest: ");r39::scat(m,1360,msg);core->Log(m);}
}
struct Buf{char b[1100];Buf(){b[0]=0;} Buf&s(const char*x){r39::scat(b,1100,x);return *this;} Buf&n(long long v){if(v<0){s("-");v=-v;}r39::snum(b,1100,(unsigned)v);return *this;}
 Buf&h(unsigned long long v){char t[20];int k=0;do{int d=(int)(v&15);t[k++]=(char)(d<10?'0'+d:'a'+d-10);v>>=4;}while(v);char o[22];o[0]='0';o[1]='x';int j=2;while(k)o[j++]=t[--k];o[j]=0;return s(o);}};
// 결과 한 줄을 게임 화면 밑 로그창(행동보고 기록)에도: 게임 스레드에서만(이력 추가는 게임 자료를 만짐)
volatile DWORD gameTid;
void hist(const wchar_t*s){
 if(!game||GetCurrentThreadId()!=gameTid)return;
 U8*rt=*(U8**)(game+0x2780f60);if(!rt)return;U8*sc=*(U8**)(rt+0x871e8);int date=sc?*(int*)(sc+0x450):0;
 void*mgr=((void*(*)())(game+0x16ff70))();if(!mgr)return;
 static wchar_t line[300];const wchar_t*pre=L"[통합패치 시험] ";int k=0;for(int i=0;pre[i];i++)line[k++]=pre[i];for(int i=0;s[i]&&k<298;i++)line[k++]=s[i];line[k]=0;
 const wchar_t*empty=*(const wchar_t**)(game+0x1a4ab88);
 ((U8*(*)(void*,int,int,int,int,int,const wchar_t*,const wchar_t*,int,int,int,int,int))(game+0x936980))(mgr,1,0,0,1,date,line,empty?empty:L"",0x1394,0,0,1,0);
}
void set_status(const wchar_t*s){int i=0;for(;s[i]&&i<255;i++)status[i]=s[i];status[i]=0;
 if(!(s[0]==L'빈'&&s[1]==L' '&&s[2]==L'칸')&&!(s[0]==L'요'&&s[1]==L'청')&&!(s[0]==L'앞'&&s[1]==L' '))hist(s);   // '빈 칸에 자동 저장 중…' 같은 진행 문구는 빼고 결과만
 if(hwnd){auto pm=(BOOL(*)(HWND,unsigned,UPTR,UPTR))GetProcAddress(GetModuleHandleW(L"user32.dll"),"PostMessageW");if(pm)pm(hwnd,0x8001,0,0);}}
// ---- 게임 도우미 ----
template<class F> F G(U32 r){return (F)(game+r);}
U8*root(){return *(U8**)(game+0x2780f60);}
bool valid(void*o){return o&&G<int(*)(void*)>(0x331780)(o)!=0;}
U8*scen(){U8*r=root();return r?*(U8**)(r+0x871e8):nullptr;}
int date(){U8*s=scen();return s?*(int*)(s+0x34):0;}
U8*player_force(){U8*s=scen();return s?G<U8*(*)(U8*)>(0x330b90)(s):nullptr;}
bool is_player(U8*force){return valid(force)&&G<int(*)(U8*)>(0x235150)(force)!=0;}
U8*person(U32 id){U8*r=root();if(!r||id>0x1770)return nullptr;return *(U8**)(r+0x158+8*(UPTR)id);}
struct MBI_{void*base;void*abase;DWORD aprot;WORD part;SIZE_T size;DWORD state,prot,type;};
bool readable(const void*p,size_t n){static SIZE_T(WINAPI*vq)(const void*,MBI_*,SIZE_T);if(!vq)vq=(SIZE_T(WINAPI*)(const void*,MBI_*,SIZE_T))GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"VirtualQuery");MBI_ mi;if(!p||!vq||!vq(p,&mi,sizeof mi))return false;if(mi.state!=0x1000||(mi.prot&(0x01|0x100)))return false;return (U8*)p+n<=(U8*)mi.base+mi.size;}
U8*area(U32 id){U8*r=root();if(!r||id>0x1f4)return nullptr;U8*a=*(U8**)(r+0x6caf0+8*(UPTR)id);if(!readable(a,0x98)||*(U8**)a!=game+0x13a99c0)return nullptr;return a;}   // CAreaData(vt 0x13a99c0)만(R42c: 표 뒤쪽 다른 자료를 지역으로 읽어 튕기던 것 막음)
U8*force_of(U8*obj){return G<U8*(*)(U8*)>(0x22c990)(obj);}            // 지역·도시 등 vt+0xb0 세력 번호 → 세력 표
U8*force_of_person(U8*p){return G<U8*(*)(U8*)>(0x22e450)(p);}
bool active(U8*state){return G<int(*)(void*)>(0x572670)(state)!=0;}
U8*mgr_states(){return G<U8*(*)()>(0x11040)();}
bool skip_state(){return G<int(*)()>(0x335110)()!=0;}                  // 게임이 지금 연출 건너뛰는 중
// ---- 세이브 칸 ----
U8*save_mgr(){return *(U8**)(game+0x27dba68);}
bool cc_mode(){void*o=G<void*(*)()>(0x1d37b0)();return o&&G<int(*)()>(0x1d2a00)()!=-1;}
void slot_name(int s,char*out,int cap){const char*p=G<const char*(*)(int,int,int)>(0x32d090)(s,0,0);int i=0;if(p)for(;p[i]&&i<cap-1;i++)out[i]=p[i];out[i]=0;}
int steam_exists(const char*name){ // 게임 0x935b46 과 같은 호출: SteamInternal_ContextInit(&ctx)→[0]=ISteamRemoteStorage, vt+0x68 FileExists
 auto init=*(void*(**)(void*))(game+0x1340b28);if(!init)return -1;void**ctx=(void**)init(game+0x1a280c0);if(!ctx||!ctx[0])return -1;
 void*rs=ctx[0];auto fe=(*(U8(***)(void*,const char*))rs)[0x68/8];return fe(rs,name)?1:0;}
void refresh_slots(){U8*m=save_mgr();if(m)G<void(*)(U8*,int)>(0x935ac0)(m,0);}
void*slot_header(int s){U8*m=save_mgr();return m?G<void*(*)(U8*,int)>(0x9352e0)(m,s):nullptr;}
int pick_empty(Buf&why){
 if(!save_mgr()){why.s("no save manager");return -1;}
 int lo=cc_mode()?0x3c:0,hi=cc_mode()?0x6d:0x31; // 칸 정보 다시 읽기(0x935ac0) 안 함: 저장 일꾼과 겹치면 튕김 — 기존 칸 머리 + Steam 파일 존재로만 판정
 for(int s=lo;s<=hi;s++){if(slot_header(s))continue;char nm[64];slot_name(s,nm,64);if(!nm[0])continue;int e=steam_exists(nm);if(e==1)continue;why.s(nm);return s;}
 why.s("all manual slots used");return -1;
}
// MSVC std::string(0x20): 16바이트 이하는 안쪽, 넘으면 게임 할당기(0x3f90e0)
void mkstr(U8*s,const char*t){U64 n=0;while(t[n])n++;memset(s,0,0x20);
 if(n<16){memcpy(s,t,n+1);*(U64*)(s+0x18)=15;}else{char*p=G<char*(*)(U64)>(0x3f90e0)(n+1);memcpy(p,t,n+1);*(char**)s=p;*(U64*)(s+0x18)=n;}
 *(U64*)(s+0x10)=n;}
bool push_save(int s,Buf&why){
 char nm[64];slot_name(s,nm,64);if(!nm[0]){why.s("no file name");return false;}
 static U8 rq[0x48];memset(rq,0,sizeof rq);*(int*)rq=s;mkstr(rq+8,nm);mkstr(rq+0x28,"");
 G<void(*)(U8*)>(0x33aee0)(rq);                                   // 저장 요청(칸·파일·설명)
 U8*m=mgr_states();if(!m){why.s("no state manager");return false;}
 G<void(*)(U8*,const char*,void*)>(0x466530)(m,(const char*)(game+0x13b2918),nullptr); // "CSaveState"
 why.s(nm);return true;
}
// ---- 시험 1: 재해(B1) ----
struct Patch{U32 rva;U8 old[6];};
const Patch kMonth[4]={{0x299dcd,{0x0f,0x87,0xa6,0x06,0,0}},{0x299dde,{0x0f,0x83,0x95,0x06,0,0}},{0x29a513,{0x0f,0x87,0x2a,0x0c,0,0}},{0x29a521,{0x0f,0x83,0x1c,0x0c,0,0}}};
bool month_bypass(bool on){ // 분기 달 검사(1·4·7·10월만 발생·끝) 4곳을 시험 동안만 6바이트 nop
 static const U8 nop6[6]={0x66,0x0f,0x1f,0x44,0x00,0x00};
 if(on)for(auto&p:kMonth)if(memcmp(game+p.rva,p.old,6))return false;
 for(auto&p:kMonth){DWORD o;U8*a=game+p.rva;if(!VirtualProtect(a,6,PAGE_EXECUTE_READWRITE,&o))return false;memcpy(a,on?nop6:p.old,6);VirtualProtect(a,6,o,&o);FlushInstructionCache((HANDLE)(intptr_t)-1,a,6);}
 return true;
}
long ss_counter(int i){auto t=(void*(*)(int))GetProcAddress(GetModuleHandleW(L"SAN14_SceneSkip.dll"),"SAN14_SceneSkipTestTable");if(!t)return -1;long*p=(long*)t(i);return p?*p:-1;}
int ss_get(const char*fn,int w){auto f=(int(*)(int))GetProcAddress(GetModuleHandleW(L"SAN14_SceneSkip.dll"),fn);return f?f(w):-1;}
long cam_stat(int i){auto f=(long(*)(int))GetProcAddress(GetModuleHandleW(L"SAN14_SceneSkip.dll"),"SAN14_SceneSkipCamStats");return f?f(i):-1;}
void cam_pos(double*o){U8*c=G<U8*(*)()>(0x110d0)();for(int i=0;i<3;i++)o[i]=c?*(double*)(c+0xa0+8*i):0;}
bool cam_same(const double*a,const double*b){for(int i=0;i<3;i++){double d=a[i]-b[i];if(d<0)d=-d;if(d>0.5)return false;}return true;}
void utf8(Buf&b,const wchar_t*w){char t[1000];int n=WideCharToMultiByte(65001,0,w,-1,t,999,nullptr,nullptr);if(n<=0)return;t[999]=0;for(char*q=t;*q;q++)if(*q=='\n')*q='|';b.s(t);}
void log_summary(int test){
 auto f=(const wchar_t*(*)())GetProcAddress(GetModuleHandleW(L"SAN14_SceneSkip.dll"),"SAN14_SceneSkipSummaryText");int n=ss_get("SAN14_SceneSkipGet",5);
 Buf b;b.s("turn-end result window (shown when this month's processing ends) has ").n(n).s(" item(s) waiting");if(f&&n>0){b.s(": ");utf8(b,f());}log(test,b.b);
}
// R42c: 재해 시험은 게임 함수를 직접 부르지 않음(내 턴 틱에서 월 처리 함수를 억지로 부르면 상태가 어긋날 수 있음).
//  버튼 = '조건만 걸기': 내 지역 한 곳에 역병 대기 표시 + 분기 달 검사 4곳 잠깐 끔 → 사용자가 '진행'으로 턴을 넘기면
//  게임이 원래 순서(월 처리 15단계 발생 → 29단계 진행 중 알림 → 78단계 턴 끝 결과 창)대로 처리. 처리가 지나가면 내 턴 틱에서
//  달 검사를 되돌리고 결과(생략 수·로그창 줄·카메라 시도/막음·결과 창 수)를 기록. 40일 안에 안 일어나면 표시를 지우고 끝냄.
int day_index(int d);
bool t1_armed;U32 t1_area;int t1_date;long t1_c[12];int t1_last_date;
long cs_get(int i){return cam_stat(i);}
void t1_snapshot(long*c){c[0]=ss_counter(9);c[1]=ss_counter(24);c[2]=ss_counter(25);c[3]=ss_counter(11);c[4]=cam_stat(0);c[5]=cam_stat(1);c[6]=cam_stat(3);c[7]=cam_stat(5);c[8]=ss_counter(7);c[9]=ss_counter(8);}
void test_disaster(){
 if(t1_armed){log(1,"SKIP disaster test already armed - advance the turn");set_status(L"재해 시험: 이미 준비됨 - '진행'으로 턴을 넘기세요");return;}
 U8*pf=player_force();if(!valid(pf)){log(1,"FAIL no player force");return;}
 U32 pick=0;for(U32 id=1;id<=0x1f4;id++){U8*a=area(id);if(!a)continue;if(!is_player(force_of(a)))continue;if(a[0x38]&0x03)continue;pick=id;break;}
 if(!pick){log(1,"FAIL no player area without plague");set_status(L"재해 시험: 역병 없는 내 지역이 없음");return;}
 if(!month_bypass(true)){log(1,"FAIL month-check bytes differ (another mod?) - nothing armed");set_status(L"재해 시험: 달 검사 자리가 달라 준비 안 함");return;}
 U8*a=area(pick);a[0x38]|=0x02;t1_area=pick;t1_date=t1_last_date=date();t1_snapshot(t1_c);t1_armed=true;
 Buf b;b.s("armed: area ").n(pick).s(" plague-pending (bits ").h(a[0x38]).s(" pop ").n(a[0x54]).s("), quarter-month check off until this month's processing passes; SceneSkip disaster/camera/summary hooked=").n(ss_get("SAN14_SceneSkipHooked",0)).s("/").n(ss_get("SAN14_SceneSkipHooked",4)).s("/").n(ss_get("SAN14_SceneSkipHooked",3)).s(" camblock=").n(ss_get("SAN14_SceneSkipGet",4));log(1,b.b);
 set_status(L"재해 시험 준비 끝: '진행'으로 턴을 넘기면 이번 달 처리에서 내 지역 역병이 납니다");
}
void t1_tick(){
 if(!t1_armed)return;int d=date();if(d==t1_last_date)return;t1_last_date=d;     // 턴이 넘어가 내 턴으로 돌아올 때만
 U8*a=area(t1_area);bool plague=a&&(a[0x38]&1);long c[12];t1_snapshot(c);long dn=c[0]-t1_c[0],dos=c[1]-t1_c[1],dop=c[2]-t1_c[2],dsum=c[3]-t1_c[3],dreq=c[4]-t1_c[4],dblk=c[5]-t1_c[5],dun=c[6]-t1_c[6],dw=c[7]-t1_c[7],dbgm=c[8]-t1_c[8],dcut=c[9]-t1_c[9];
 bool happened=dn>0||dos>0||plague;bool expired=day_index(d)-day_index(t1_date)>40;
 if(!happened&&!expired)return;
 month_bypass(false);if(a&&!plague)a[0x38]&=~0x02;t1_armed=false;
 Buf b;b.s("result: area ").n(t1_area).s(plague?" PLAGUE":" no plague").s(" | skipped narration(log line) +").n(dn).s(" music +").n(dbgm).s(" banner +").n(dcut).s(" ongoing notice +").n(dos).s(" ongoing window +").n(dop).s(" | camera requests +").n(dreq).s(" blocked +").n(dblk).s(" unlisted +").n(dun).s(" waits skipped +").n(dw).s(" | turn-end window +").n(dsum).s(expired&&!happened?" (40 days passed without the disaster - disarmed)":"");
 bool ok=happened&&dsum>=1&&(dblk>=0);b.s(ok?" OK":" CHECK");log(1,b.b);log_summary(1);
 static wchar_t st[200];st[0]=0;
 if(!happened){r39::wcat(st,200,L"재해 시험: 40일 동안 안 일어나 준비를 풀었음");}
 else{r39::wcat(st,200,L"재해 시험 끝: 창 없이 로그창 한 줄 · 카메라 시도 ");char nb[16];wchar_t w[16];int i;
  nb[0]=0;r39::snum(nb,16,(unsigned)dreq);for(i=0;nb[i];i++)w[i]=nb[i];w[i]=0;r39::wcat(st,200,w);r39::wcat(st,200,L"→막음 ");
  nb[0]=0;r39::snum(nb,16,(unsigned)dblk);for(i=0;nb[i];i++)w[i]=nb[i];w[i]=0;r39::wcat(st,200,w);r39::wcat(st,200,dsum>=1?L" · 턴 끝 결과 창 1번":L" · 결과 창 없음(확인 필요)");}
 set_status(st);
}
// ---- 시험 10: 카메라 막기 — 판정표 + 실제 사건 카메라 함수(명령 실행 사건 0x2e8e30)에 다른 지역을 비추라고 요청 ----
ULONGLONG camCheckAt;double camP0[3];long camB0,camA0;
void test_camera(){
 if(ss_get("SAN14_SceneSkipHooked",4)!=1){log(10,"SKIP SceneSkip camera hook not installed");set_status(L"카메라 시험: 막는 자리가 안 걸려 있음(기록 참조)");return;}
 auto probe=(int(*)(unsigned,unsigned))GetProcAddress(GetModuleHandleW(L"SAN14_SceneSkip.dll"),"SAN14_SceneSkipCamProbe");
 static const unsigned evd[]={0x29892a,0x2a359d,0x2a3a2f,0x2c2367,0x2d5809,0x2e8e4c,0x374fa2,0x376615,0x422a65,0x42da35,0x839825,0x855aa7,0x85fa52},evo[]={0x2d1dd5,0x2da5a6,0x2e019e,0x41c77c,0x420bdc,0x42b50c,0x85fc2b};
 static const unsigned usd[]={0x4f94d1,0x5020b3,0x544204,0x67b0cb,0x71f27e,0x71fe5e,0x729498,0x7e80a5,0x891db2,0x44d9ea},uso[]={0x44d9aa,0x66ec09,0x87fc99,0x88b0cc,0x8919da,0x891abd,0x891b89,0x8e4e00};
 int eb=0,et=0,ua=0,ut=0;if(probe){for(unsigned r:evd){et++;eb+=probe(r,0)==1;}for(unsigned o:evo){et++;eb+=probe(0x45141f,o)==1;}for(unsigned r:usd){ut++;ua+=probe(r,0)==0;}for(unsigned o:uso){ut++;ua+=probe(0x45141f,o)==0;}}
 int on=ss_get("SAN14_SceneSkipGet",4);
 Buf b;b.s("camera table: event places ").n(eb).s("/").n(et).s(" judged block, user places ").n(ua).s("/").n(ut).s(" judged keep; block switch=").n(on).s(" stats req/blocked/user/unlisted/off/waits=");for(int i=0;i<6;i++){b.n(cam_stat(i)).s(i<5?"/":"");}log(10,b.b);
 if(on!=1){set_status(L"카메라 시험: 설정 '카메라 자동 이동'이 켜져 있어 막기 꺼짐 — 끄고 다시");return;}
 U8*rt=root();if(!rt){log(10,"FAIL no game root");return;}
 U8*reg=nullptr;double p[3];cam_pos(p);float best=-1;
 for(int i=0;i<=0x33;i++){U8*r=*(U8**)(rt+0xdd38+8*i);if(!valid(r))continue;int cell=G<int(*)(U8*)>(0x22d740)(r);float o[4]={0,0,0,0};G<void(*)(void*,int,float*)>(0x1886b0)(*(void**)(game+0x1bb6d50),cell,o);
  float dx=(float)(o[0]-p[0]),dz=(float)(o[2]-p[2]);float d=dx*dx+dz*dz;if(d>best){best=d;reg=r;}}
 if(!reg){log(10,"FAIL no region to aim at");return;}
 int cell=G<int(*)(U8*)>(0x22d740)(reg);
 alignas(16) static U8 rq[0x80];memset(rq,0,sizeof rq);              // 0x453570 이 만드는 요청과 같은 모양
 *(int*)rq=500;float o[4]={0,0,0,0};G<void(*)(void*,int,float*)>(0x1886b0)(*(void**)(game+0x1bb6d50),cell,o);if(*(int*)(game+0x1b59290)==0)o[1]=0;memcpy(rq+0x10,o,16);
 *(double*)(rq+0x20)=(double)G<float(*)(int)>(0xea70)(1);*(double*)(rq+0x28)=*(double*)(game+0x1342c08);*(double*)(rq+0x30)=*(double*)(game+0x13ae8d8);*(int*)(rq+0x38)=1;
 cam_pos(camP0);camB0=cam_stat(1);camA0=cam_stat(0);
 G<void(*)(void*)>(0x2e8e30)(rq);                                 // 게임의 사건 카메라 함수(원래: 그 지역으로 0.5초 이동·확대 + 0.5초 기다림)
 long a=cam_stat(0)-camA0,bl=cam_stat(1)-camB0;
 Buf c;c.s("real event camera call 0x2e8e30 -> region cell ").n(cell).s(" (farthest from camera): requests +").n(a).s(" blocked +").n(bl).s("; checking camera position again in 1.5 s");log(10,c.b);
 camCheckAt=GetTickCount64()+1500;
 set_status(bl==a&&a>0?L"카메라 시험: 사건 카메라 요청 1회 → 막음 1회 (1.5초 뒤 위치 다시 확인)":L"카메라 시험: 막지 못함(기록 참조)");
}
void camera_recheck(){
 if(!camCheckAt||GetTickCount64()<camCheckAt)return;camCheckAt=0;
 double p[3];cam_pos(p);bool same=cam_same(camP0,p);long bl=cam_stat(1)-camB0,a=cam_stat(0)-camA0;
 Buf b;b.s("camera position after 1.5 s: ").s(same?"unchanged":"MOVED").s(" (requests +").n(a).s(" blocked +").n(bl).s(")").s(same&&bl>=1?" OK":" CHECK");log(10,b.b);log_summary(10);
 set_status(same&&bl>=1?L"카메라 시험 끝: 사건 카메라 이동 시도 → 막음, 화면 제자리(정상)":L"카메라 시험: 화면이 움직였음(기록 참조)");
}
// ---- 시험 2·3: 등용 결과(B2)·보물 발견(B3) 장면 자리 직접 부르기 ----
U8*ruler(){U8*pf=player_force();if(!valid(pf))return nullptr;U32 id=G<U32(*)(U8*)>(0x230400)(pf);return person(id);}
U8*other_officer(){for(U32 id=1;id<=0x1770;id++){U8*p=person(id);if(!valid(p))continue;U8*f=force_of_person(p);if(valid(f)&&!is_player(f))return p;}return nullptr;}
U8*any_item(){U8*r=root();if(!r)return nullptr;for(U8**q=(U8**)(r+0x77b18);q<(U8**)(r+0x78168);q++)if(*q)return *q;return nullptr;}
bool state_on_stack(U32 name_rva){U8*m=mgr_states();return m&&G<void*(*)(U8*,const char*)>(0x56fc30)(m,(const char*)(game+name_rva))!=nullptr;}
// 판정: 생략이 되면 장면 상태(CStrategyEmployTalkState/CSearchDiscoveryState)가 안 쌓임 + SceneSkip 이 'R42B 4 ... employ_skip_ok/treasure_skip' 줄을 남김
void test_employ(){
 if(ss_get("SAN14_SceneSkipHooked",1)!=1||ss_get("SAN14_SceneSkipGet",1)!=1){log(2,"SKIP SceneSkip employ not hooked or switched off (test calls the scene entry with made-up args, only safe when skipped)");set_status(L"등용 시험: 생략 기능이 꺼져 있어 안 함");return;}
 U8*a=ruler(),*t=other_officer();if(!a||!t){log(2,"FAIL no ruler/other officer");return;}
 int found=0;static U8 arg[0x20];
 for(int succ=1;succ>=0;succ--){memset(arg,0,sizeof arg);*(U8**)arg=a;*(U8**)(arg+8)=t;*(int*)(arg+0x10)=succ;G<void(*)(U8*)>(0x6c6b10)(arg);found+=state_on_stack(0x1441088);}
 Buf b;b.s("employ scene entry 0x6c6b10 called twice (success, fail) actor=").n(*(U16*)(a+0x10)).s(" target=").n(*(U16*)(t+0x10)).s(" talk-scene states pushed=").n(found)
  .s(found?" CHECK (scene appeared)":" OK (no talk scene; SceneSkip lines employ_skip_ok/employ_skip_fail + 2 history lines expected)");
 log(2,b.b);set_status(found?L"등용 시험: 장면이 떴음(기록 확인)":L"등용 시험 끝: 장면 없음·행동보고 2줄이면 정상");
}
void test_treasure(){
 if(ss_get("SAN14_SceneSkipHooked",2)!=1||ss_get("SAN14_SceneSkipGet",2)!=1){log(3,"SKIP SceneSkip treasure not hooked or switched off");set_status(L"보물 시험: 생략 기능이 꺼져 있어 안 함");return;}
 U8*a=ruler(),*it=any_item();if(!a||!it){log(3,"FAIL no ruler/item");return;}
 static U8 arg[0x40];memset(arg,0,sizeof arg);*(int*)arg=0;*(U8**)(arg+8)=a;*(U8**)(arg+0x18)=it;  // 곳 없음 → 제목 문장(누가·무슨 명품)
 G<void(*)(U8*)>(0x6c6dd0)(arg);int found=state_on_stack(0x1447328);
 Buf b;b.s("treasure window entry 0x6c6dd0 kind 0 actor=").n(*(U16*)(a+0x10)).s(" item=").n(*(U16*)(it+0x10)).s(" discovery states pushed=").n(found)
  .s(found?" CHECK (window appeared)":" OK (no window; SceneSkip line treasure_skip + 1 history line expected; item owner not changed by this test)");
 log(3,b.b);set_status(found?L"보물 시험: 창이 떴음(기록 확인)":L"보물 시험 끝: 창 없음·행동보고 1줄이면 정상");
}
// ---- 시험 4: 논공 저장정보 오류(13번) — 오류 표시를 켠 채로 저장(자동 저장이 곧 시험) ----
volatile long mbox_count;void*mbox_real;void**mbox_slot;
int WINAPI MboxSpy(HWND h,LPCWSTR t,LPCWSTR c,unsigned f){InterlockedIncrement(&mbox_count);return ((int(*)(HWND,LPCWSTR,LPCWSTR,unsigned))mbox_real)(h,t,c,f);}
void**iat_slot(HMODULE m,const char*dll,const char*fnn){ // 모듈 가져오기 표에서 dll!fn 칸
 if(!m)return nullptr;U8*b=(U8*)m;U32 e=*(U32*)(b+0x3c);U32 rva=*(U32*)(b+e+0x90);if(!rva)return nullptr;
 for(U8*d=b+rva;*(U32*)(d+12);d+=20){const char*n=(const char*)(b+*(U32*)(d+12));bool eq=true;for(int i=0;dll[i]||n[i];i++){char x=n[i],y=dll[i];if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y){eq=false;break;}}
  if(!eq)continue;U64*ilt=(U64*)(b+(*(U32*)d?*(U32*)d:*(U32*)(d+16)));void**iat=(void**)(b+*(U32*)(d+16));
  for(int i=0;ilt[i];i++){if(ilt[i]>>63)continue;const char*nm=(const char*)(b+(U32)ilt[i]+2);int k=0;while(fnn[k]&&nm[k]==fnn[k])k++;if(!fnn[k]&&!nm[k])return &iat[i];}}
 return nullptr;}
void spy(bool on){
 if(on){mbox_slot=iat_slot(GetModuleHandleW(L"SAN14_MeritTraits.dll"),"USER32.dll","MessageBoxW");if(!mbox_slot)return;mbox_real=*mbox_slot;DWORD o;VirtualProtect(mbox_slot,8,PAGE_READWRITE,&o);*mbox_slot=(void*)&MboxSpy;VirtualProtect(mbox_slot,8,o,&o);}
 else if(mbox_slot){DWORD o;VirtualProtect(mbox_slot,8,PAGE_READWRITE,&o);*mbox_slot=mbox_real;VirtualProtect(mbox_slot,8,o,&o);mbox_slot=nullptr;}
}
volatile int*merit_sym(const char*name){auto d=(const void*const*(*)())GetProcAddress(GetModuleHandleW(L"SAN14_MeritTraits.dll"),"SAN14_MeritDiagnosticsV1");if(!d)return nullptr;
 struct Sym{const char*n;const void*a;};const Sym*s=(const Sym*)d();for(;s&&s->n;s++){int k=0;while(name[k]&&s->n[k]==name[k])k++;if(!name[k]&&!s->n[k])return (volatile int*)s->a;}return nullptr;}
int merit_before=-1;
bool merit_prepare(){volatile int*f=merit_sym("metadata_fault");if(!f){log(4,"SKIP 13 MeritTraits not loaded");return false;}
 mbox_count=0;spy(true);merit_before=*f;*f=1;Buf b;b.s("metadata_fault ").n(merit_before).s("->1 before auto-save, MessageBoxW spy=").s(mbox_slot?"on":"off");log(4,b.b);return true;}
void merit_after(bool saved){volatile int*f=merit_sym("metadata_fault");spy(false);if(!saved&&f&&*f==1&&merit_before>=0)*f=merit_before; // 저장 못 했으면 원래 값으로
 Buf b;b.s("after save: saved=").n(saved).s(" metadata_fault=").n(f?*f:-1).s(" MeritTraits message boxes=").n(mbox_count);
 bool ok=saved&&f&&*f==0&&mbox_count==0;b.s(ok?" OK (save not blocked, fault cleared, no error box)":" CHECK");log(4,b.b);
 set_status(ok?L"논공 시험 끝: 오류창 없이 저장됨":L"논공 시험: 기록 확인 필요");}
// ---- 시험 5: 22번 범위 밖 지역 1곳(개발 1001) — 며칠 진행하면 그 지역만 건너뛰는지, 5일 뒤 원래 값 복원 ----
U32 t22_area;U16 t22_old;int t22_date;bool t22_on;
int day_index(int d){return (int)(d&0xffff)*372+((d>>16)&0xff)*31+((d>>24)&0xff);}
void test_22(){
 if(!GetModuleHandleW(L"SAN14_DomesticCE.dll")){log(5,"SKIP 22 DomesticCE not loaded");return;}
 if(t22_on){log(5,"SKIP previous 22 test still running");return;}
 for(U32 id=1;id<=0x1f4;id++){U8*a=area(id);if(!a||!is_player(force_of(a)))continue;t22_area=id;t22_old=*(U16*)(a+0x3c);*(U16*)(a+0x3c)=1001;t22_date=date();t22_on=true;
  Buf b;b.s("area ").n(id).s(" dev(+0x3c) ").n(t22_old).s("->1001 (out of range for DomesticCE); run the game a few days, value restored after 5 days");log(5,b.b);
  set_status(L"22번 시험: 지금부터 며칠 진행하세요(5일 뒤 자동 복원)");return;}
 log(5,"FAIL no player area");}
void t22_tick(){if(!t22_on)return;if(day_index(date())-day_index(t22_date)<5)return;U8*a=area(t22_area);if(a&&*(U16*)(a+0x3c)==1001)*(U16*)(a+0x3c)=t22_old;t22_on=false;
 Buf b;b.s("area ").n(t22_area).s(" dev restored to ").n(t22_old).s(" (see ICGuard lines: 'area #N out of range -> skipped only this area')");log(5,b.b);set_status(L"22번 시험 끝: 값 복원함");}
// ---- 시험 6·7·8: 화면 바로 열기 ----
void test_item_window(U8*state){G<void(*)(U8*,int)>(0x44f310)(state,0x15);log(6,"opened item window (menu command 0x15 CStrategyItemState); choose grant/confiscate -> list pictures should be item pictures (30)");set_status(L"명품 창 열림: 수여 목록 그림 확인");}
void test_settings(){auto o=(int(*)())GetProcAddress(GetModuleHandleW(L"SAN14_ModSettings.dll"),"SAN14_ModSettingsOpen");int r=o?o():-1;Buf b;b.s("ModSettings open -> ").n(r);log(7,b.b);set_status(r==1?L"모드 설정 창 열림":L"모드 설정 창 못 엶(기록 참조)");}
void test_execute(){HMODULE m=GetModuleHandleW(L"SAN14_ExecuteLock.dll");auto st=(int(*)())(m?GetProcAddress(m,"SAN14_ExecuteLockState"):nullptr);
 auto ex=(int(*)(const wchar_t*))GetProcAddress(GetModuleHandleW(L"SAN14_ExtBridge.dll"),"SAN14_ExtState");
 auto et=(U64(*)(int,U64,U64))(m?GetProcAddress(m,"SAN14_ExecuteLockTest"):nullptr);
 Buf b;b.s("ExecuteLock loaded=").n(m?1:0).s(" enabled=").n(st?st():-1).s(" ic_greyed=").n(et?(long long)et(8,0,0):-1).s(" NoExecute=").n(ex?ex(L"NoExecute"):-1).s(" -> open 7 captive-persuasion (or win a battle with captives) once; ExecuteLock writes execute_button_greyed lines");
 log(8,b.b);set_status(L"처단 시험: 포로 회유 창을 한 번 여세요(처단 회색 확인)");}
void test_traits9(){HMODULE m=GetModuleHandleW(L"SAN14_Traits9Check.dll");auto tt=(void*(*)(int))(m?GetProcAddress(m,"SAN14_Traits9CheckTestTable"):nullptr);
 if(!tt){log(9,"SKIP Traits9Check not loaded (9/13 not installed?)");set_status(L"개성 9칸: 확인 모듈 없음");return;}
 unsigned n=((unsigned(*)())tt(0))();bool ui=((bool(*)())tt(1))();
 Buf b;b.s("traits9 slots patched=").n(n).s("/5 ui hooked=").n(ui).s(" faults a/b/c=").n(*(long*)tt(5)).s("/").n(*(long*)tt(6)).s("/").n(*(long*)tt(7)).s(" notices shown=").n(*(long*)tt(4))
  .s(n==5&&ui?" OK -> open any officer info, trait tab: 9 cells (also new-officer edit after decide)":" CHECK (Traits9Check notice text tells why)");
 log(9,b.b);set_status(n==5&&ui?L"개성 9칸 정상: 무장 정보 개성 탭에서 9칸 확인":L"개성 9칸 문제: 기록 확인");}
void run_test(long k,U8*state,bool saved){
 switch(k){case 1:test_disaster();break;case 2:test_employ();break;case 3:test_treasure();break;case 4:merit_after(saved);break;case 5:test_22();break;case 6:test_item_window(state);break;case 7:test_settings();break;case 8:test_execute();break;case 9:test_traits9();break;case 10:test_camera();break;}
}
// ---- 게임 상태 스레드에서 한 단계씩 ----
void pump(U8*state){
 gameTid=GetCurrentThreadId();
 t22_tick();camera_recheck();t1_tick();
 if(stage==0){long k=req;if(!k)return;cur=k;Buf why;
  if(k>=8){InterlockedExchange(&req,0);run_test(k,state,false);return;}   // 상태 기록만(게임 데이터 안 바꿈) → 저장 칸 아끼려고 저장 없이
  if(skip_state()){log((int)k,"WAIT game is skipping scenes now");return;}
  int s=pick_empty(why);
  if(s<0){Buf b;b.s("SKIP no empty manual save slot (").s(why.b).s(") - test not run");log((int)k,b.b);set_status(L"빈 세이브 칸이 없어 시험 안 함");InterlockedExchange(&req,0);return;}
  if(k==4&&!merit_prepare()){InterlockedExchange(&req,0);return;}
  Buf w;if(!push_save(s,w)){Buf b;b.s("FAIL auto-save not started: ").s(w.b);log((int)k,b.b);if(k==4)spy(false);InterlockedExchange(&req,0);return;}
  slot=s;stage=1;t0=lastCheck=GetTickCount64();Buf b;b.s("auto-save to empty slot ").n(s).s(" ").s(w.b).s(" started");log((int)k,b.b);set_status(L"빈 칸에 자동 저장 중…");return;}
 if(stage==1){ULONGLONG now=GetTickCount64();if(now-lastCheck<1000)return;lastCheck=now;
  // 저장 확인: 칸 정보 다시 읽기(0x935ac0)는 안 씀 — 그 함수는 칸 머리를 전부 지우고(0x9359c0) 120칸을 다시 읽는데,
  //  저장 일꾼 스레드가 같은 칸 머리를 만지는 중이면 해제된 메모리를 건드려 게임이 튕김(R42 시험 '재해 두 번째 누름' 튕김 원인).
  //  → CSaveState 가 상태 더미에서 빠지고(저장 끝) Steam 원격 저장에 파일이 생겼을 때 확인.
  if(state_on_stack(0x13b2918))return;            // "CSaveState" 아직 진행 중
  char nm[64];slot_name(slot,nm,64);bool ok=nm[0]&&steam_exists(nm)==1;
  if(!ok&&now-t0<60000)return;
  Buf b;b.s(ok?"auto-save confirmed slot ":"FAIL auto-save not confirmed in 60 s, slot ").n(slot);log((int)cur,b.b);
  stage=0;long k=cur;InterlockedExchange(&req,0);
  if(ok||k==4)run_test(k,state,ok);else set_status(L"자동 저장 확인 실패 - 시험 안 함");}
}
}
// ---- 플레이어 턴 칸 이어 붙이기 (PatchTester R2 방식) ----
using Tick4=void(*)(void*,void*,void*,void*);
static volatile Tick4 nextTick;static volatile long turnHooked;
extern "C" __declspec(dllexport) void SceneTestTick(U8*state,void*a,void*b,void*c){
 Tick4 n=nextTick;if(!n)n=(Tick4)(game+0x44ed30);n(state,a,b,c);
 if(!active(state))return;int ph=*(int*)(state+0x470);
 if(ph!=1&&ph!=2){ // 플레이어가 명령하는 단계(게임 명령 처리 0x44f310 이 도는 단계)에서만. 요청이 오래 못 돌면 단계 값을 한 번 기록(진단)
  static ULONGLONG since;static int told;if(req&&!told){ULONGLONG n=GetTickCount64();if(!since)since=n;else if(n-since>8000){Buf b;b.s("request waiting: player-turn phase value ").n(ph).s(" (expected 1 or 2)");log((int)req,b.b);told=1;}}
  return;}
 pump(state);
}
static DWORD WINAPI tickInstaller(LPVOID){
 U8*slotp=game+0x13d9458;void*last=nullptr;unsigned stable=0;
 for(unsigned i=0;i<1200;i++){Sleep(500);void*cur=__atomic_load_n((void**)slotp,__ATOMIC_ACQUIRE);if(cur==(void*)&SceneTestTick)return 0;
  if(cur==last)stable++;else{stable=0;last=cur;}if(stable<10)continue;     // 5초 안 바뀌면 다른 모듈 설치 끝
  DWORD old=0;if(!VirtualProtect(slotp,8,PAGE_READWRITE,&old)){log(0,"FAIL tick slot not writable");return 0;}
  nextTick=(Tick4)cur;LONG64 seen=InterlockedCompareExchange64((volatile LONG64*)slotp,(LONG64)(UPTR)&SceneTestTick,(LONG64)(UPTR)cur);
  if(seen!=(LONG64)(UPTR)cur){nextTick=nullptr;stable=0;last=nullptr;continue;}
  InterlockedExchange(&turnHooked,1);Buf b;b.s("player-turn tick chained on ").s(cur==(void*)(game+0x44ed30)?"native":"another module").s(" (").h((UPTR)cur-(UPTR)game).s(")");log(0,b.b);return 0;}
 log(0,"FAIL player-turn tick never stable");return 0;
}
// ---- 시험 창 (자기 스레드) ----
namespace{
struct WNDCLASSW_{unsigned style;void*proc;int ce,we;HINSTANCE inst;void*icon,*cursor,*bg;LPCWSTR menu,cls;};
struct MSG_{HWND h;unsigned m;UPTR w;UPTR l;DWORD t;LONG x,y;DWORD p;};
const wchar_t*kBtn[10]={L"1 재해: 조건 걸기 → 진행 누르면 실제 발생",L"2 등용 결과 장면(B2)",L"3 보물 발견 창(B3)",L"4 논공 저장정보 오류(13번)",L"5 22번 범위 밖 지역(누르고 며칠 진행)",L"6 명품 창 열기(30번 그림)",L"7 모드 설정 창 열기",L"8 처단 버튼(B4) 상태 기록",L"9 개성 9칸(A10) 상태 기록",L"0 카메라 막기: 사건 카메라 실제 요청"};
HMODULE u32;HWND hStatus;
template<class F> F U(const char*n){return (F)GetProcAddress(u32,n);}
void press(int k){long z=0;if(!__atomic_compare_exchange_n(&req,&z,(long)k,0,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)){set_status(L"앞 시험 진행 중 - 끝난 뒤 다시");return;}set_status(L"요청 받음 - 게임 화면(내 턴)에서 곧 시작");}
UPTR WINAPI WndProc(HWND h,unsigned m,UPTR w,UPTR l){
 if(m==0x0111){int id=(int)(w&0xffff);if(id>=101&&id<=110)press(id-100);return 0;}       // WM_COMMAND
 if(m==0x0312){int id=(int)w;if(id>=1&&id<=10)press(id);return 0;}                         // WM_HOTKEY
 if(m==0x8001){if(hStatus)U<BOOL(*)(HWND,LPCWSTR)>("SetWindowTextW")(hStatus,status);return 0;}
 if(m==0x0010)return 0;                                                                    // WM_CLOSE: 닫지 않음(게임 끝날 때 같이)
 return U<UPTR(*)(HWND,unsigned,UPTR,UPTR)>("DefWindowProcW")(h,m,w,l);
}
DWORD WINAPI uiThread(LPVOID){
 u32=GetModuleHandleW(L"user32.dll");if(!u32)u32=LoadLibraryExW(L"user32.dll",0,0);if(!u32)return 0;
 HINSTANCE inst=GetModuleHandleW(L"SAN14_SceneTest.dll");
 WNDCLASSW_ wc{};wc.proc=(void*)&WndProc;wc.inst=inst;wc.cls=L"SAN14SceneTest";wc.bg=(void*)(UPTR)(15+1);
 wc.cursor=U<void*(*)(HINSTANCE,LPCWSTR)>("LoadCursorW")(0,(LPCWSTR)(UPTR)32512);
 U<unsigned short(*)(const WNDCLASSW_*)>("RegisterClassW")(&wc);
 auto cw=U<HWND(*)(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,void*,HINSTANCE,void*)>("CreateWindowExW");
 hwnd=cw(0x8|0x80,L"SAN14SceneTest",L"통합패치 시험 도구 (시험판 전용)",0x00C00000|0x00080000|0x10000000,20,80,330,470,0,0,inst,0); // TOPMOST|TOOLWINDOW, CAPTION|SYSMENU|VISIBLE
 if(!hwnd){log(0,"FAIL test window not created");return 0;}
 HMODULE g32=GetModuleHandleW(L"gdi32.dll");void*font=nullptr;
 if(g32){auto cf=(void*(*)(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR))GetProcAddress(g32,"CreateFontW");if(cf)font=cf(-13,0,0,0,400,0,0,0,129,0,0,0,0,L"Malgun Gothic");}
 auto sm=U<UPTR(*)(HWND,unsigned,UPTR,UPTR)>("SendMessageW");
 for(int i=0;i<10;i++){HWND b=cw(0,L"BUTTON",kBtn[i],0x40000000|0x10000000,8,8+i*38,300,34,hwnd,(void*)(UPTR)(101+i),inst,0);if(font)sm(b,0x30,(UPTR)font,1);}
 hStatus=cw(0,L"STATIC",status,0x40000000|0x10000000,8,8+10*38,300,40,hwnd,0,inst,0);if(font)sm(hStatus,0x30,(UPTR)font,1);
 auto rh=U<BOOL(*)(HWND,int,unsigned,unsigned)>("RegisterHotKey");if(rh){for(int i=0;i<9;i++)rh(hwnd,1+i,0x1|0x2|0x4000,'1'+i);rh(hwnd,10,0x1|0x2|0x4000,'0');} // Ctrl+Alt+숫자
 MSG_ msg;auto gm=U<BOOL(*)(MSG_*,HWND,unsigned,unsigned)>("GetMessageW");auto tm=U<BOOL(*)(const MSG_*)>("TranslateMessage");auto dm=U<UPTR(*)(const MSG_*)>("DispatchMessageW");
 while(gm(&msg,0,0,0)>0){tm(&msg);dm(&msg);}
 return 0;
}
}
// ---- 단위시험·시작 ----
extern "C" __declspec(dllexport) void*SAN14_SceneTestTable(int i){
 void*t[]={(void*)&SceneTestTick,(void*)&nextTick,(void*)&req,(void*)&stage,(void*)&slot,(void*)&testMode,(void*)status,(void*)&mbox_count,(void*)&t22_on};
 return i>=0&&i<(int)(sizeof t/sizeof t[0])?t[i]:nullptr;}
extern "C" __declspec(dllexport) int SAN14_SceneTestPress(int k){if(k<1||k>10)return 0;press(k);return (int)req;}
extern "C" __declspec(dllexport) int SAN14_SceneTestPickEmpty(){Buf w;return pick_empty(w);}
extern "C" __declspec(dllexport) int SAN14_SceneTestMonth(int on){return month_bypass(on!=0)?1:0;}
static volatile long attached; // ModCore 시작·바탕화면 실행기 중 먼저 온 쪽 한 번만
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,0,"SAN14_SceneTest.dll"))return 0;if(attached)return 1;attached=1;core=api;game=api->resolved->image;
 wchar_t ini[1024];int show=1;if(r39::game_root(ini,1024)){r39::wcat(ini,1024,L"SAN14Mods\\modules\\SceneTest.ini");show=GetPrivateProfileIntW(L"SceneTest",L"Window",1,ini);}
 testMode=!show;
 log(0,"SceneTest R42c loaded (test build only). Buttons queue requests; work runs on the player-turn tick after an auto-save to an empty manual slot.");
 r39::start(0,"SAN14_SceneTest.dll","OK",1,1,"test-only","-");
 HANDLE t=CreateThread(nullptr,0,tickInstaller,nullptr,0,nullptr);if(t)CloseHandle(t);
 if(show){HANDLE u=CreateThread(nullptr,0,uiThread,nullptr,0,nullptr);if(u)CloseHandle(u);}
 return 1;
}
// 바탕화면 실행기(시험도구_실행.exe)가 실행 중인 게임에 이 DLL 을 올린 뒤 부르는 입구 — 게임 폴더에 안 넣고 씀.
//  ModCore 없이 시작(시험 도구는 ModCore 훅을 안 씀: 틱 칸 이어 붙이기·잠깐 바꾸는 자리 모두 자체 처리). 기록은 게임 폴더 SAN14_기록 그대로.
//  돌려줌: 1 시작함, 3 이미 시작됨, 2 지원 안 하는 게임 파일, 4 통합패치(ModCore)가 안 켜진 게임
extern "C" __declspec(dllexport) DWORD WINAPI SAN14_SceneTestAttach(LPVOID){
 if(core||InterlockedExchange(&attached,1))return 3;
 U8*img=(U8*)GetModuleHandleW(nullptr);if(!img)return 2;
 U32 pe=*(U32*)(img+0x3c);U32 size=*(U32*)(img+pe+0x50);
 if(size!=S14_GAME_IMAGE){attached=0;return 2;}
 if(!GetModuleHandleW(L"SAN14_ModCore.dll")){attached=0;return 4;}
 game=img;testMode=0;
 log(0,"SceneTest R42c attached from desktop launcher (test build only, not in game folder). Buttons queue requests; work runs on the player-turn tick after an auto-save to an empty manual slot.");
 r39::start(0,"SAN14_SceneTest.dll","OK",1,1,"test-only desktop","-");
 HANDLE t=CreateThread(nullptr,0,tickInstaller,nullptr,0,nullptr);if(t)CloseHandle(t);
 HANDLE u=CreateThread(nullptr,0,uiThread,nullptr,0,nullptr);if(u)CloseHandle(u);
 return 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
