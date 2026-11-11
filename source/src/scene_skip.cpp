// R42 B1~B3 (4번) 연출 생략 모듈 SAN14_SceneSkip.dll  — R42c: 카메라 자동 이동 전부 막기 + 턴 끝 결과 창(종류별 묶음)
//  B1 재해(역병·메뚜기)·풍작 연출 생략: 효과(지역 재해 칸·인구 감소·이력 기록)는 게임 그대로, 연출 4가지만 안 함
//    ① 재해 음악(0x406d30 소리 관리자, 종류 7/5) ② 화면 위 알림 띠 펼침(0x4541e0 → 0x842a90, 0xf 메뚜기/0x10 역병/0xe 풍작)
//    ③ 나레이션 창(0x1f1e90, CCmnNarrationState 모달) ④ 알림 띠 접기(0x4421f0 → 0x835230)
//    자리: 발생 0x2989a0(0x29b8ff/0x29b91c/0x29b9de/0x29b9eb), 번짐 0x29a4b0(0x29cb8b/0x29cba7/0x29cdb6/0x29cdc3),
//          풍작 0x2ba220(0x2bc9f2/0x2bca04/0x2bcbda/0x2bcc11). 세 함수 모두 '내 지역이 걸렸을 때'만 이 연출을 함.
//  B1b(R42c) 재해 '진행 중' 알림(월 처리 29단계 0x428010 → 0x297e10): "OO에서 메뚜기떼 피해가 발생 중입니다." 창 + 효과음.
//    0x297e10 은 도시마다 카메라(0x2e00c0→0x453570, 0x2d1d10→0x453570) + 0.5초 기다림(0x306a30) → 문장(0x5e39) → 효과음 0x405b40(0x12/0x13)
//    → 창 CCmnDlgMsgWithRInputDialog 쌓기 0x2454c0 + 지금 상태 멈춤 0x574750 → 끝에 처음 카메라 자리로 되돌림(0x298927).
//    효과음 자리(0x29a24a/0x29a5a4)에서 문장(rdi = 만든 문장 버퍼)을 받아 로그창 한 줄 + 결과 모음, 효과음·창·멈춤은 짝 맞춰 건너뜀.
//  카메라(R42c, 사용자 10/29 "카메라 이동 관련된 건 전부 끄게"): 모드 설정 '카메라 자동 이동'(통합편의 [Camera] AutoFocus)이 꺼져 있으면
//    주 카메라 요청 함수 CMainCameraCtrl::vt[2] 0x14330(모든 카메라 이동·확대가 여기로 옴) 입구에서 부른 자리(돌아갈 주소)로 판정해
//    사건·재해·월 처리·명승 영상·컷신·이벤트 카메라는 안 움직이게 하고, 직접 누른 '위치로 가기'·목록 이동·화면 조작 카메라는 그대로 둠.
//    카메라 뒤 0.5~1초 기다림(0x306a30) 10곳도 같이 건너뜀(제자리에서 기다릴 이유가 없음).
//  턴 끝 결과 창(R42c, 사용자 10/29): 로그창 한 줄은 그대로 두고, 월 처리 마지막 단계(79단계 중 78번 0x42ae20, 단계표 0x1a61040)에서
//    이번에 생략된 재해·등용 결과·보물 발견 + 게임 이력에 들어온 우리 세력 명승·증축 소식을 종류별로 묶어 나레이션 창 1번.
//    창을 띄운 프레임은 그 단계를 0 으로 돌려(잠시 멈춤) 창이 닫힌 뒤 원래 마지막 단계가 그대로 실행됨(게임 단계 함수와 같은 방식).
//  B2 등용 결과 대사 장면 / B3 보물 발견 창: scene_skip_b23.cpp
//  설정: SAN14Mods\modules\SceneSkip.ini [SceneSkip] Disaster=1 Employ=1 Treasure=1 Summary=1 (모드 설정 창 4번 줄 4개)
#include "common.h"
#include "r39_record.h"
using U64=uint64_t;
static const S14_CoreAPI*core;static U8*game;
static volatile long en[4]={1,1,1,1}; // 0 재해 1 등용 2 보물 3 턴 끝 결과 창
static const wchar_t*const kKey[4]={L"Disaster",L"Employ",L"Treasure",L"Summary"};
static volatile long hooked[7];        // 0 재해 1 등용 2 보물 3 결과 창 4 카메라 5 재해 진행 중 6 명승·증축 이력 받기
static volatile long cam_block;        // 1 = 카메라 자동 이동 막음('카메라 자동 이동' 끔)
extern "C" bool SsEmployInstall(const S14_CoreAPI*api,HookPlan&plan,volatile long*enabled);
extern "C" bool SsTreasureInstall(const S14_CoreAPI*api,HookPlan&plan,volatile long*enabled);

static void ini_path(wchar_t*p,size_t cap){p[0]=0;if(r39::game_root(p,cap))r39::wcat(p,cap,L"SAN14Mods\\modules\\SceneSkip.ini");}
static void log_line(const char*a,unsigned v1,const char*b=nullptr,unsigned v2=0,const char*c=nullptr){
 if(!core||!core->Log)return;char m[240];m[0]=0;r39::scat(m,240,a);r39::snum(m,240,v1);if(b){r39::scat(m,240,b);r39::snum(m,240,v2);}if(c)r39::scat(m,240,c);core->Log(m);
}
static void hexs(char*o,size_t cap,unsigned v){char t[12];int k=0;do{int d=v&15;t[k++]=(char)(d<10?'0'+d:'a'+d-10);v>>=4;}while(v);char s[16];s[0]='0';s[1]='x';int j=2;while(k)s[j++]=t[--k];s[j]=0;r39::scat(o,cap,s);}

// ---- 공용: 행동보고(이력) 한 줄 (Patch27 city_event_skip log_narration 과 같은 호출 형태) ----
static volatile long own_hist;   // 우리가 넣는 줄은 명승·증축 받기에서 뺌
extern "C" void SsAddHistory(const wchar_t*line){
 if(!game||!line||!line[0])return;
 U8*root=*(U8**)(game+0x2780f60);if(!root)return;
 U8*sc=*(U8**)(root+0x871e8);int date=sc?*(int*)(sc+0x450):0;
 using Mgr=void*(*)();
 using AddHistory=U8*(*)(void*,int,int,int,int,int,const wchar_t*,const wchar_t*,int,int,int,int,int);
 void*mgr=((Mgr)(game+0x16ff70))();if(!mgr)return;
 const wchar_t*empty=*(const wchar_t**)(game+0x1a4ab88);
 own_hist++;
 ((AddHistory)(game+0x936980))(mgr,1,0,0,1,date,line,empty?empty:L"",0x1394,0,0,1,0);
 own_hist--;
}

// ---- 턴 끝 결과 모음 ----
static bool ss_summary_on(){return en[3]!=0;}
#include "scene_summary.h"
// ---- 22차(P219): 넓은 창 = 게임 사건 결과 창(도시 증축 사건 뒤에 뜨는 그 창) ----
//  게임 사건 장면은 결과를 0x832f10(창 관리자 0x11040(), 이름 "CEventResultState", &{사건번호, 0}, &닫힐 때 부를 것)으로 쌓고,
//  그 창이 열릴 때(CEventResultState vt[1] 0x83c680) 레이아웃 "09_01_02_02_event_talk_result" 를 만든 뒤 0x83c793 에서
//  0x842af0(레이아웃, 사건번호) 로 제목(레이아웃+0x138 →+0x18)·본문(→+0x20) 글자를 사건표에서 채움(0x30f250 글자 넣기, wchar_t*, \n 줄바꿈).
//  우리는 사건번호 자리에 표시 번호 kWideId 를 넣어 쌓고, 0x83c793 호출만 우리 함수로 돌려 그 번호일 때 제목·본문을 우리 글자로 채움(다른 번호는 원래 0x842af0).
//  닫힐 때 부를 것 = 나레이션 창(0x1f1e90)이 쓰는 것과 같은 꼴(vtable 0x13a6160, 잡은 값 = 지금 맨 위 상태, 부르면 0x5746f0 로 그 상태 다시 진행) → 쌓은 뒤 0x574750 으로 지금 상태 멈춤.
//  못 쓰면(자리 바이트 다름·다른 모드가 먼저 고침·창이 안 열림) 작은 나레이션 창(제목+3줄, 넘치면 "그 외 N건") + 오류 창 1번.
static void diag_page(const char*how,const wchar_t*title,const wchar_t*body);   // 확인판 전용 기록(아래)
static void auto_attach(U8*lay);   // 메시지 속도 저절로 넘김(아래)
static const int kWideId=0x5343F001;      // 게임 사건 번호(수천 이하)와 안 겹치는 표시 번호
struct WideChk{U32 rva;U32 n;U8 b[18];const char*what;};
static const WideChk kWideChk[]={
 {0x832f10,16,{0x4c,0x89,0x4c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57},"push CEventResultState exe+0x832f10 entry"},
 {0x83c776,6,{0x8b,0x97,0x70,0x04,0x00,0x00},"CEventResultState enter: edx=[state+0x470] (event id) exe+0x83c776"},
 {0x83c783,7,{0x83,0xbf,0x74,0x04,0x00,0x00,0x00},"CEventResultState enter: cmp [state+0x474],0 exe+0x83c783"},
 {0x842b18,7,{0x48,0x8b,0x87,0x38,0x01,0x00,0x00},"result text fill: layout+0x138 exe+0x842b18"},
 {0x842b24,4,{0x48,0x8b,0x58,0x18},"result text fill: title element +0x18 exe+0x842b24"},
 {0x842b62,4,{0x48,0x8b,0x58,0x20},"result text fill: body element +0x20 exe+0x842b62"},
 {0x1467b48,18,{'C','E','v','e','n','t','R','e','s','u','l','t','S','t','a','t','e',0},"state name string exe+0x1467b48"},
};
static volatile long wide_ok;          // 1 = 넓은 창을 씀(자리 확인·호출 자리 걸기 끝)
static volatile long wide_armed,wide_filled,wide_shown,wide_fallback,wide_fill_other,wide_redo;
static ULONGLONG wide_armed_at,wide_filled_at;static volatile long wide_auto_done,auto_ok,auto_closed;static int auto_bad=-1;static U8 auto_bad_ac[16];       // 쌓은 시각. 창이 열리기 전에 단계가 다시 불리면(한두 프레임) 기다리고, 3초 넘게 안 열리면 실패로 봄
static int wide_bad=-1;                // 자리 확인 실패 번호(kWideChk), 100 = 호출 자리 못 걸음, 101 = 창 구조 이상, 200 = 창이 안 열림/글자 못 넣음
static char wide_bad_why[160];
static U8 wide_bad_ac[18];
extern "C" void SsResultFill(U8*layout,int id){
 if(id==kWideId&&wide_armed){
  InterlockedExchange(&wide_armed,0);
  U8*t=layout&&core->Readable(layout+0x138,8)?*(U8**)(layout+0x138):nullptr;
  void*title=t&&core->Readable(t+0x18,0x10)?*(void**)(t+0x18):nullptr;void*body=t?*(void**)(t+0x20):nullptr;
  if(title&&body){
   using SetText=void(*)(void*,const wchar_t*,int,int,int,void*);
   ((SetText)(game+0x30f250))(title,sumtitle,1,1,1,nullptr);
   ((SetText)(game+0x30f250))(body,sumtext,1,1,1,nullptr);
   wide_filled_at=GetTickCount64();InterlockedExchange(&wide_auto_done,0);auto_attach(layout);
   InterlockedIncrement(&wide_filled);diag_page("wide_filled",sumtitle,sumtext);return;}
  wide_bad=101;wide_bad_why[0]=0;r39::scat(wide_bad_why,sizeof wide_bad_why,t?"title/body element pointer is null":"layout+0x138 is null or unreadable");
  InterlockedExchange(&wide_redo,1);return;}                              // 우리 번호인데 구조가 다름 → 사건표를 찾지 않고 빈 창(다음 단계에서 작은 창으로 다시 보여 줌)
 if(id==kWideId){InterlockedIncrement(&wide_fill_other);return;}
 ((void(*)(U8*,int))(game+0x842af0))(layout,id);
}
static bool wide_check(){
 for(int i=0;i<(int)(sizeof kWideChk/sizeof kWideChk[0]);i++){const WideChk&c=kWideChk[i];
  if(!core->Readable(game+c.rva,c.n)){wide_bad=i;memset(wide_bad_ac,0,sizeof wide_bad_ac);return false;}
  if(memcmp(game+c.rva,c.b,c.n)){wide_bad=i;memcpy(wide_bad_ac,game+c.rva,c.n);return false;}}
 // 가상 함수 표: CEventResultState vt[1] = 0x83c680(열기), 닫힐 때 부를 것 vt[0] = 0x203920(복사)·vt[2] = 0x830ec0(부르기)
 struct P{U32 at;U32 want;};static const P ptrs[]={{0x14679c8+8,0x83c680},{0x13a6160,0x203920},{0x13a6160+16,0x830ec0}};
 for(auto&q:ptrs){if(!core->Readable(game+q.at,8)||*(U8**)(game+q.at)!=game+q.want){wide_bad=101;wide_bad_why[0]=0;r39::scat(wide_bad_why,sizeof wide_bad_why,"vtable entry exe+0x");r39::shex(wide_bad_why,sizeof wide_bad_why,q.at);r39::scat(wide_bad_why,sizeof wide_bad_why," != exe+0x");r39::shex(wide_bad_why,sizeof wide_bad_why,q.want);return false;}}
 return true;
}
struct WideSite{U32 rva,len;U8 ex[16],ac[16];const char*why;bool set;};static WideSite wide_site;
static bool install_wide(const S14_CoreAPI*api){
 if(!wide_check())return false;
 HookPlan p(api);p.call(0x83c793,0x842af0,(void*)&SsResultFill);
 if(!p.install("SAN14CE.SceneSkip.TurnSummaryWide")){wide_bad=100;wide_site.rva=p.bad_rva;wide_site.len=p.bad_len;memcpy(wide_site.ex,p.bad_expected,16);memcpy(wide_site.ac,p.bad_actual,16);wide_site.why=p.bad_why;wide_site.set=true;return false;}
 return true;
}
// ---- 25차(사용자 11-11): 환경설정 '메시지 속도'를 이 창에도 — 수동이면 그대로(확인 눌러야 넘어감), 느리게·보통·빠르게면 시간이 지나면 저절로 닫힘 ----
//  게임 나레이션 창은 설정값 = 0x3ec1a0()(설정) +0x20 번호(0 수동·1 느리게·2 보통·3 빠르게) → 표 0x13c6248 {0x7fffffff,60,35,15} 를 기다림 값으로 씀(0x86418e),
//  그 레이아웃의 프레임마다 부르는 vt[1](0x865520)이 시간을 재서 넘김. 사건 결과 창 레이아웃(CEventResultLayout vtable 0x1467820)은 vt[1] 이 빈 함수(0x8ca1c0).
//  26차(11-11 화면: "가만히 두면 안 넘어가고 F12 누르니 넘어감"): 25차는 창의 입력 처리(0x83e280)에서 시간을 쟀는데 그건 입력이 있을 때만 불림 → 틀린 자리.
//  → 우리 창 레이아웃만 가상 함수 표를 복사본으로 바꿔(52칸 + 앞 RTTI 칸) vt[1] 자리에 우리 함수를 넣음. 게임 코드·원래 표는 안 건드림.
//  시간이 다 되면 확인 단추와 같은 닫기 vt[0x198](0x8637f0). 기다림 = 표 값 × 0.1초(느리게 6초·보통 3.5초·빠르게 1.5초).
struct AutoChk{U32 rva;U32 n;U8 b[16];const char*what;};
static const AutoChk kAutoChk[]={
 {0x3ec470,4,{0x48,0x63,0x41,0x20},"message speed index = [config+0x20] exe+0x3ec470"},
 {0x13c6248,16,{0xff,0xff,0xff,0x7f,0x3c,0,0,0,0x23,0,0,0,0x0f,0,0,0},"message speed table {manual,60,35,15} exe+0x13c6248"},
};
static const U32 kLayoutVt=0x1467820,kLayoutSlots=52;
static void*my_vt[kLayoutSlots+1];   // [0] = RTTI 칸, [1..] = 표
static unsigned auto_wait_ms(){
 if(!auto_ok)return 0;U8*cfg=((U8*(*)())(game+0x3ec1a0))();if(!cfg||!core->Readable(cfg+0x20,4))return 0;
 int idx=*(int*)(cfg+0x20);if(idx<1||idx>3)return 0;int w=((const int*)(game+0x13c6248))[idx];if(w<=0||w>600)return 0;return (unsigned)w*100u;
}
// 우리 창 레이아웃의 vt[1](프레임마다): 시간이 다 되면 확인과 같은 닫기
extern "C" void SsResultLayoutUpdate(U8*lay,void*){
 if(wide_auto_done)return;
 unsigned ms=auto_wait_ms();if(!ms)return;                       // 수동(또는 값 이상) → 확인 눌러야 넘어감
 if(GetTickCount64()-wide_filled_at<ms)return;
 InterlockedExchange(&wide_auto_done,1);InterlockedIncrement(&auto_closed);
 ((void(*)(U8*))(game+0x8637f0))(lay);
}
static void auto_attach(U8*lay){   // SsResultFill 에서 우리 창 레이아웃에만
 if(!auto_ok||!lay||*(void**)lay!=(void*)(game+kLayoutVt))return;
 *(void***)lay=&my_vt[1];
}
static bool install_auto(const S14_CoreAPI*){
 for(int i=0;i<(int)(sizeof kAutoChk/sizeof kAutoChk[0]);i++){const AutoChk&c=kAutoChk[i];
  if(!core->Readable(game+c.rva,c.n)){auto_bad=i;memset(auto_bad_ac,0,16);return false;}
  if(memcmp(game+c.rva,c.b,c.n)){auto_bad=i;memcpy(auto_bad_ac,game+c.rva,c.n);return false;}}
 // 표 모양: vt[1] = 빈 함수 0x8ca1c0, vt[51](0x198) = 닫기 0x8637f0, vt[52] 칸부터는 다른 표(CEventResultState 0x14679c8 의 RTTI 칸)
 void**vt=(void**)(game+kLayoutVt);
 if(!core->Readable(vt-1,(kLayoutSlots+1)*8)||vt[1]!=(void*)(game+0x8ca1c0)||vt[0x198/8]!=(void*)(game+0x8637f0)||(U8*)(vt+kLayoutSlots+1)!=game+0x14679c8){auto_bad=101;return false;}
 my_vt[0]=vt[-1];for(U32 i=0;i<kLayoutSlots;i++)my_vt[1+i]=vt[i];my_vt[2]=(void*)&SsResultLayoutUpdate;
 return true;
}
static void auto_report(){   // 넓은 창을 띄울 때 한 번(메시지 속도 저절로 넘김만 안 됨 — 창은 확인 눌러 닫으면 됨)
 static volatile long told;if(auto_ok||InterlockedExchange(&told,1))return;
 const wchar_t*eff=L"턴 끝 결과 창에 환경설정 '메시지 속도'(저절로 넘김)가 적용 안 됨 → 확인을 눌러 닫으면 됨. 결과 창 자체는 그대로";
 char where[160],ex[80],ac[80];where[0]=ex[0]=ac[0]=0;
 if(auto_bad>=0&&auto_bad<(int)(sizeof kAutoChk/sizeof kAutoChk[0])){const AutoChk&c=kAutoChk[auto_bad];r39::scat(where,sizeof where,c.what);r39::sbytes(ex,sizeof ex,c.b,c.n);r39::sbytes(ac,sizeof ac,auto_bad_ac,c.n);
  r43c::Owner o=r39::site_owner(c.rva,c.b,auto_bad_ac,c.n);r39::fail_ex(4,"SAN14_SceneSkip.dll","auto_close_site",where,ex,ac,eff,&o);return;}
 r39::fail(4,"SAN14_SceneSkip.dll","auto_close_shape","CEventResultLayout vtable exe+0x1467820 (vt[1]=0x8ca1c0, vt[0x198]=0x8637f0, 52 slots)","as in game 0x6ad06f53","vtable differs",eff);
}
// 넓은 창을 못 쓸 때 오류 창(한 번). 기능이 꺼져 있으면 부르지 않음(턴 끝 창을 띄울 때만 부름).
static void wide_report(){
 static volatile long told;if(InterlockedExchange(&told,1))return;
 const wchar_t*eff=L"턴 끝 결과를 넓은 사건 결과 창에 못 띄움 → 작은 나레이션 창(제목 + 3줄, 넘치면 '그 외 N건')으로 대신 보여 줌. 결과는 로그창(행동보고)에도 그대로 있음";
 char where[200],ex[80],ac[80];where[0]=ex[0]=ac[0]=0;
 if(wide_bad>=0&&wide_bad<(int)(sizeof kWideChk/sizeof kWideChk[0])){const WideChk&c=kWideChk[wide_bad];
  r39::scat(where,sizeof where,c.what);U32 n=c.n>16?16:c.n;r39::sbytes(ex,sizeof ex,c.b,n);r39::sbytes(ac,sizeof ac,wide_bad_ac,n);
  r43c::Owner o=r39::site_owner(c.rva,c.b,wide_bad_ac,n);r39::fail_ex(4,"SAN14_SceneSkip.dll","wide_window_site",where,ex,ac,eff,&o);return;}
 if(wide_bad==100&&wide_site.set){r39::scat(where,sizeof where,"result text call exe+0x83c793 (");r39::scat(where,sizeof where,wide_site.why);r39::scat(where,sizeof where,")");
  r39::sbytes(ex,sizeof ex,wide_site.ex,wide_site.len);r39::sbytes(ac,sizeof ac,wide_site.ac,wide_site.len);
  r43c::Owner o=r39::site_owner(wide_site.rva,wide_site.ex,wide_site.ac,wide_site.len);r39::fail_ex(4,"SAN14_SceneSkip.dll","wide_window_hook",where,ex,ac,eff,&o);return;}
 if(wide_bad==101){r39::fail(4,"SAN14_SceneSkip.dll","wide_window_shape","CEventResultState / event_talk_result layout","game structure as in 0x6ad06f53",wide_bad_why,eff);return;}
 r39::fail(4,"SAN14_SceneSkip.dll","wide_window_open","exe+0x832f10 push CEventResultState -> exe+0x83c793 text fill","window opened and text filled",wide_bad_why[0]?wide_bad_why:"window did not open (text fill never called)",eff);
}
// 넓은 창 한 장 쌓기. 못 쌓으면 false(이유는 wide_bad_why).
static bool wide_push(){
 using Fn0=void*(*)();
 if(((int(*)())(game+0x335110))())return true;   // 나레이션 창도 이 때는 안 띄움(게임 같은 조건) → 띄운 셈으로 넘김
 U8*em=core->Readable(game+0x27872f0,8)?*(U8**)(game+0x27872f0):nullptr;
 if(em&&core->Readable(em+0x1b0,4)&&*(int*)(em+0x1b0))return true;
 void*mgr=((Fn0)(game+0x11040))();
 void*top=mgr?((void*(*)(void*))(game+0x572480))(mgr):nullptr;
 if(!top){wide_bad=200;wide_bad_why[0]=0;r39::scat(wide_bad_why,sizeof wide_bad_why,mgr?"no current state (0x572480 returned null)":"state manager 0x11040 returned null");return false;}
 // std::function 꼴(MSVC x64): 0x38 바이트 자리 + 끝 8바이트 = 구현 주소. 구현 = {vtable 0x13a6160, 잡은 상태}
 alignas(16) U64 fn[8];memset(fn,0,sizeof fn);fn[0]=(U64)(UPTR)(game+0x13a6160);fn[1]=(U64)(UPTR)top;fn[7]=(U64)(UPTR)&fn[0];
 struct{int id,flag;}arg={kWideId,0};
 wide_armed_at=GetTickCount64();InterlockedExchange(&wide_armed,1);
 ((void(*)(void*,const char*,void*,void*))(game+0x832f10))(mgr,(const char*)(game+0x1467b48),&arg,fn);
 ((void(*)(void*))(game+0x574750))(top);
 InterlockedIncrement(&wide_shown);
 return true;
}
// 확인판 전용(본판 빌드에선 빠짐): 결과 창마다 모양을 기록 — R42D 4 SceneSkip summary_page 방식 제목 본문(줄은 " / ") 줄수 최장줄
static void diag_page(const char*how,const wchar_t*title,const wchar_t*body){
 if(!r39::s14_diag())return;
#if S14_DIAG_RECORDS
 int lines=body&&body[0]?1:0,cur=0,mx=0;for(const wchar_t*q=body;q&&*q;q++){if(*q==L'\n'){lines++;if(cur>mx)mx=cur;cur=0;}else if(is_tag(q))q+=2;else cur++;}if(cur>mx)mx=cur;   // 보이는 글자 수(색 표시 뺌)
 static wchar_t flat[0x900];int k=0;for(const wchar_t*q=body;q&&*q&&k<0x8f0;q++){if(*q==L'\n'){flat[k++]=L' ';flat[k++]=L'/';flat[k++]=L' ';}else flat[k++]=*q;}flat[k]=0;
 static char line[0x2000];line[0]=0;const size_t C=sizeof line;
 r39::scat(line,C,"R42D\t4\tSAN14_SceneSkip.dll\tsummary_page\t");r39::stamp(line,C);r39::scat(line,C,"\t");r39::scat(line,C,how);r39::scat(line,C,"\tlines=");r39::snum(line,C,(unsigned)lines);r39::scat(line,C,"\tmaxlen=");r39::snum(line,C,(unsigned)mx);r39::scat(line,C,"\twidth=");r39::snum(line,C,(unsigned)kLineW);r39::scat(line,C,"\tper_page=");r39::snum(line,C,(unsigned)kPageLines);r39::scat(line,C,"\t");
 auto put=[&](const wchar_t*w){size_t n=r39::slen(line);
#ifdef S14_TEST_SHIM
  for(;*w&&n+4<C;w++){unsigned c=*w;if(c<0x80)line[n++]=(char)c;else if(c<0x800){line[n++]=(char)(0xc0|c>>6);line[n++]=(char)(0x80|(c&63));}else{line[n++]=(char)(0xe0|c>>12);line[n++]=(char)(0x80|((c>>6)&63));line[n++]=(char)(0x80|(c&63));}}line[n]=0;
#else
  int a=WideCharToMultiByte(CP_UTF8,0,w,-1,line+n,(int)(C-n-1),0,0);n+=a>0?a-1:0;line[n]=0;
#endif
 };
 put(title?title:L"");r39::scat(line,C,"\t");put(flat);
 r39::append_line(line);
#else
 (void)how;(void)title;(void)body;
#endif
}
static void show_small(){diag_page("small",L"",sumsmall);((void(*)(const wchar_t*,int))(game+0x1f1e90))(sumsmall,0);InterlockedIncrement(&wide_fallback);ss_summary_finish();}
// 월 처리 마지막 단계(0x42ae20) 입구: 모은 결과가 있으면 창 1번 띄우고 1(이번엔 단계 멈춤), 없으면 0(원래 단계 실행)
extern "C" int SsTurnEnd(U8*,U8*){
 // 앞 장이 넓은 창으로 쌓였는데 글자를 못 넣었으면(창이 안 열렸거나 구조가 다름) → 넓은 창 그만 쓰고 그 결과를 작은 창으로
 if(wide_armed){
  if(GetTickCount64()-wide_armed_at<3000)return 1;     // 아직 창이 열리기 전 — 기다림(새 창 안 띄움)
  InterlockedExchange(&wide_armed,0);wide_bad=200;InterlockedExchange(&wide_redo,1);}
 if(wide_redo){InterlockedExchange(&wide_redo,0);InterlockedExchange(&wide_ok,0);wide_report();show_small();return 1;}
 long total=0;if(!ss_summary_step(&total))return 0;
 if(total){long s=InterlockedIncrement(&summary_shown);
  if(s<=20||s%50==0){log_line("SceneSkip R42c: turn-end result window shown once (",(unsigned)total,wide_ok?" item(s) grouped by kind, wide event-result window":" item(s) grouped by kind, small narration window",0,nullptr);r39::counter(4,"SAN14_SceneSkip.dll","turn_summary_window",(unsigned)s);}}
 if(wide_ok&&wide_push()){auto_report();return 1;}
 if(wide_ok){InterlockedExchange(&wide_ok,0);InterlockedExchange(&wide_armed,0);}
 wide_report();show_small();
 return 1;
}
// 게임 이력 추가 0x936980 입구: 우리 세력 줄 중 '명승'·'증축' 이 들어간 것만 결과 모음에(줄 자체는 게임 그대로)
static bool has(const wchar_t*s,const wchar_t*k){if(!s)return false;for(int i=0;i<600&&s[i];i++){int j=0;while(k[j]&&s[i+j]==k[j])j++;if(!k[j])return true;}return false;}
static int player_force_id(){
 U8*root=*(U8**)(game+0x2780f60);if(!root)return -1;U8*sc=*(U8**)(root+0x871e8);if(!sc)return -1;
 U8*p=((U8*(*)(U8*))(game+0x330b90))(sc);if(!p||!((int(*)(void*))(game+0x331780))(p))return -1;
 return ((int(*)(U8*))((*(void***)p)[0x60/8]))(p);
}
static volatile long hist_seen;
extern "C" int SsHistFilter(U8*sp,U8*frame){
 if(!en[3]||own_hist>0||!core)return 0;
 const wchar_t*title=*(const wchar_t**)(sp+0x38);const wchar_t*body=*(const wchar_t**)(sp+0x40);
 int cat=-1;const wchar_t*use=title;
 if(title&&core->Readable(title,4)){if(has(title,L"\xBA85\xC2B9"))cat=3;else if(has(title,L"\xC99D\xCD95"))cat=4;} // 명승 / 증축
 if(cat<0&&body&&core->Readable(body,4)){if(has(body,L"\xBA85\xC2B9"))cat=3;else if(has(body,L"\xC99D\xCD95"))cat=4;if(cat>=0&&!(title&&title[0]))use=body;}
 if(cat<0)return 0;
 int force=(int)*(I32*)(frame+0x98);   // r8d = 세력
 int pf=player_force_id();if(pf<0||force!=pf)return 0;
 SsCollect(cat,use,0);
 long n=InterlockedIncrement(&hist_seen);
 if(n<=20)log_line(cat==3?"SceneSkip R42c: scenery news collected for the turn-end window #":"SceneSkip R42c: expansion news collected for the turn-end window #",(unsigned)n);
 return 0;
}

// ---- B1 재해 ----
static volatile long dis_bgm,dis_cut,dis_narr,dis_reset,dis_lines_total;
static volatile long ong_sound,ong_push,ong_wait;
static int cut_pending,ong_armed,ong_pending;  // 짝 맞추기(띠 펼침↔접기, 진행 중 문장↔창↔멈춤)
static const int kLineCap=400;
static bool dis_on(){return en[0]!=0;}
extern "C" void SsBgm(void*mgr,int kind,float fade){
 if(dis_on()){InterlockedIncrement(&dis_bgm);return;}
 ((void(*)(void*,int,float))(game+0x406d30))(mgr,kind,fade);
}
extern "C" void SsCutin(void*obj,int id){
 if(dis_on()){cut_pending++;InterlockedIncrement(&dis_cut);return;}
 ((void(*)(void*,int))(game+0x4541e0))(obj,id);
}
extern "C" void SsReset(void*obj){
 if(cut_pending>0){cut_pending--;InterlockedIncrement(&dis_reset);return;}
 ((void(*)(void*))(game+0x4421f0))(obj);
}
static void one_line(wchar_t*one,const wchar_t*text){int k=0;for(const wchar_t*q=text;*q&&k<kLineCap-1;q++)one[k++]=(*q==L'\r'||*q==L'\n')?L' ':*q;one[k]=0;}
// 23차(11-11 화면): 풍작도 같은 생략 길(0x2bcbda)이라 【재해】로 묶였음 → 풍작 자리는 SsNarrHarvest 로 따로(결과 창 종류 5 '풍작')
static void narr_common(const wchar_t*text,int mode,int cat);
extern "C" void SsNarr(const wchar_t*text,int mode){narr_common(text,mode,0);}
extern "C" void SsNarrHarvest(const wchar_t*text,int mode){narr_common(text,mode,5);}
static void narr_common(const wchar_t*text,int mode,int cat){
 if(dis_on()){
  // 사용자 요구(10/29): 어떤 결과든 게임 화면 밑 로그창(행동보고 기록)에 한 줄은 무조건 — 재해마다 그 문장을 한 줄로
  if(text&&text[0]){static wchar_t one[kLineCap];one_line(one,text);SsAddHistory(one);SsCollect(cat,one,0);}
  long c=InterlockedIncrement(&dis_narr);InterlockedIncrement(&dis_lines_total);
  if(c<=20||c%50==0){char m[200];m[0]=0;r39::scat(m,200,"SceneSkip R42b: disaster scene skipped (no window), one line to the bottom log; total ");r39::snum(m,200,(unsigned)c);if(core&&core->Log)core->Log(m);r39::counter(4,"SAN14_SceneSkip.dll","disaster_log_line",(unsigned)c);}
  return;}
 ((void(*)(const wchar_t*,int))(game+0x1f1e90))(text,mode);
}
// B1b 진행 중: 효과음 자리에서 문장 받기(브리지가 rdi → 넷째 인자)
extern "C" void SsDisSound(void*mgr,int id,float,const wchar_t*text){
 if(dis_on()){
  if(text&&core&&core->Readable(text,4)&&text[0]){static wchar_t one[kLineCap];one_line(one,text);SsAddHistory(one);SsCollect(0,one,0);}
  ong_armed++;long c=InterlockedIncrement(&ong_sound);
  if(c<=20||c%50==0){log_line("SceneSkip R42c: ongoing-disaster notice skipped (no window, no sound, camera untouched), one line to the bottom log; total ",(unsigned)c);r39::counter(4,"SAN14_SceneSkip.dll","disaster_ongoing_log_line",(unsigned)c);}
  return;}
 ((void(*)(void*,int,float,float))(game+0x405b40))(mgr,id,0.0f,0.0f);
}
extern "C" void SsDisPush(void*a,void*b,void*c,void*d){
 if(ong_armed>0){ong_armed--;ong_pending++;InterlockedIncrement(&ong_push);return;}
 ((void(*)(void*,void*,void*,void*))(game+0x2454c0))(a,b,c,d);
}
extern "C" void SsDisWait(void*state){
 if(ong_pending>0){ong_pending--;InterlockedIncrement(&ong_wait);return;}
 ((void(*)(void*))(game+0x574750))(state);
}
extern "C"{void SsDisSoundBridge();void SsCamBridge();void SsHistBridge();void SsTurnBridge();void*SsCamRet;void*SsHistRet;void*SsTurnRet;}
// R43: 묶음별 실패 자리 보관(오류창에 기대/실제 바이트) — 0 disaster 1 employ 2 treasure 3 summary 4 camera 5 ongoing 6 news
struct BadSite{U32 rva,len;U8 ex[16],ac[16];const char*why;bool set;};static BadSite ss_bad[7];
static void keep_bad(int g,const HookPlan&p){if(g<0||g>6||!p.bad)return;BadSite&b=ss_bad[g];b.rva=p.bad_rva;b.len=p.bad_len;memcpy(b.ex,p.bad_expected,16);memcpy(b.ac,p.bad_actual,16);b.why=p.bad_why;b.set=true;}
static bool fin(HookPlan&p,const char*id,int g){bool ok=p.install(id);if(!ok)keep_bad(g,p);return ok;}
static bool install_disaster(const S14_CoreAPI*api){
 HookPlan p(api);
 // 발생 0x2989a0
 p.call(0x29b8ff,0x406d30,(void*)&SsBgm);p.call(0x29b91c,0x4541e0,(void*)&SsCutin);p.call(0x29b9de,0x1f1e90,(void*)&SsNarr);p.call(0x29b9eb,0x4421f0,(void*)&SsReset);
 // 번짐 0x29a4b0
 p.call(0x29cb8b,0x406d30,(void*)&SsBgm);p.call(0x29cba7,0x4541e0,(void*)&SsCutin);p.call(0x29cdb6,0x1f1e90,(void*)&SsNarr);p.call(0x29cdc3,0x4421f0,(void*)&SsReset);
 // 풍작 0x2ba220
 p.call(0x2bc9f2,0x406d30,(void*)&SsBgm);p.call(0x2bca04,0x4541e0,(void*)&SsCutin);p.call(0x2bcbda,0x1f1e90,(void*)&SsNarrHarvest);p.call(0x2bcc11,0x4421f0,(void*)&SsReset);
 return fin(p,"SAN14CE.SceneSkip.Disaster",0);
}
static bool install_ongoing(const S14_CoreAPI*api){
 HookPlan p(api);
 p.call(0x29a24a,0x405b40,(void*)&SsDisSoundBridge);p.call(0x29a5a4,0x405b40,(void*)&SsDisSoundBridge); // 역병 / 메뚜기 진행 중 효과음(문장 받기)
 p.call(0x29a31b,0x2454c0,(void*)&SsDisPush);p.call(0x29a715,0x2454c0,(void*)&SsDisPush);                 // 알림 창 쌓기
 p.call(0x29a323,0x574750,(void*)&SsDisWait);p.call(0x29a71d,0x574750,(void*)&SsDisWait);                 // 지금 상태 멈춤
 return fin(p,"SAN14CE.SceneSkip.DisasterOngoing",5);
}

// ---- 카메라 ----
// 돌아갈 주소(0x14330 을 부른 call 다음 주소). 0x45141f = 공용 0x453570(칸으로 카메라) 안 → 그 함수를 부른 곳(+5)으로 다시 판정.
static const U32 kBlockDirect[]={0x29a83a,0x2a578d,0x2a5c1f,0x2c46e7,0x2d7bb9,0x2eb20c,0x377302,0x378975,0x424dd5,0x42fd45,0x83bd05,0x857d97,0x861d82};
//  0x297e10 재해 진행 중 되돌림 / 0x2a2f50·0x2c1850·0x2d40b0 월 처리 사건 / 0x2e8e30 명령 실행 사건 / 0x374eb0·0x376530 사건 연출 /
//  0x422930 명승 영상 뒤 되돌림 / 0x42d900 월 처리 끝 되돌림 / 0x839760 이벤트 SActMoveCamera* / 0x8559b0·0x85f950 컷신
static const U32 kBlockOuter[]={0x2d4185,0x2dc956,0x2e24ce,0x41eaec,0x422f4c,0x42d82c,0x861f5b};
//  0x2d1d10·0x2e00c0 재해 진행 중 도시마다 / 0x2da490 명령·월 처리 사건 / 0x41c620 턴 시작 부대 자동 비춤 / 0x420ab9 전략 첫 카메라 /
//  0x42b417 계절 사건 / 0x85fb80 컷신
static const U32 kAllowDirect[]={0x4fbd31,0x504913,0x546a24,0x67d78b,0x72168e,0x72226e,0x72b918,0x7ea6a5,0x8940a2,0x44fc2a};
//  타이틀 / CMoveCameraState(목록에서 고른 곳으로) / 0x543fb0 목록 이동 / 보고 지도 / 행정구역 편집·신설 / 내정 화면 / 부대 편성 / 위 단추 / 게임 시작·불러오기 첫 위치
static const U32 kAllowOuter[]={0x44fbea,0x6712d9,0x882049,0x88d3cc,0x893cca,0x893dad,0x893e79,0x8e72e0};
//  게임 시작 첫 위치 / 기록 창 '위치로 가기' / 개입 명령 단추 / 개입 대상 고르기 / 개입 화면 / 지도 정보
static volatile long cs[12]; // 0 요청 1 막음 2 허용(직접 조작) 3 허용(모르는 자리) 4 막을 자리지만 스위치 꺼짐 5 기다림 건너뜀 6 마지막 막은 자리 7 그 바깥 8 마지막 모르는 자리 9 그 바깥
static bool in(const U32*t,size_t n,U32 v){for(size_t i=0;i<n;i++)if(t[i]==v)return true;return false;}
#define IN(t,v) in(t,sizeof(t)/sizeof(t[0]),v)
static int cam_class(U32 ret,U32 outer){ // 1 사건(막을 자리) 0 직접 조작 2 모름
 if(ret==0x45362f){if(IN(kBlockOuter,outer))return 1;if(IN(kAllowOuter,outer))return 0;return 2;}
 if(IN(kBlockDirect,ret))return 1;if(IN(kAllowDirect,ret))return 0;return 2;
}
static U32 rva_of(U64 a){U64 g=(U64)(UPTR)game;return (a>=g&&a<g+S14_GAME_IMAGE)?(U32)(a-g):0xffffffffu;}
extern "C" int SsCamFilter(U64*sp,U8*){
 U32 ret=rva_of(sp[0]),outer=ret==0x45362f?rva_of(sp[0x100/8]):0;
 long n=InterlockedIncrement(&cs[0]);(void)n;
 int c=cam_class(ret,outer);
 if(c==1){
  if(!cam_block){InterlockedIncrement(&cs[4]);return 0;}
  long b=InterlockedIncrement(&cs[1]);InterlockedExchange(&cs[6],(long)ret);InterlockedExchange(&cs[7],(long)outer);
  if(b<=30||b%100==0){char m[200];m[0]=0;r39::scat(m,200,"SceneSkip R42c: automatic camera move blocked at ");hexs(m,200,ret);if(outer){r39::scat(m,200," (from ");hexs(m,200,outer);r39::scat(m,200,")");}r39::scat(m,200,"; total blocked ");r39::snum(m,200,(unsigned)b);if(core&&core->Log)core->Log(m);r39::counter(4,"SAN14_SceneSkip.dll","camera_blocked",(unsigned)b);}
  return 1;
 }
 if(c==0){InterlockedIncrement(&cs[2]);return 0;}
 long u=InterlockedIncrement(&cs[3]);InterlockedExchange(&cs[8],(long)ret);InterlockedExchange(&cs[9],(long)outer);
 if(u<=20){char m[200];m[0]=0;r39::scat(m,200,"SceneSkip R42c: camera request from unlisted place ");hexs(m,200,ret);if(outer){r39::scat(m,200," (from ");hexs(m,200,outer);r39::scat(m,200,")");}r39::scat(m,200," left as is");if(core&&core->Log)core->Log(m);}
 return 0;
}
extern "C" void SsCamWait(int ms,int*flag,int x){
 if(cam_block){InterlockedIncrement(&cs[5]);return;}
 ((void(*)(int,int*,int))(game+0x306a30))(ms,flag,x);
}
static const U8 kCamEntry[11]={0x48,0x83,0xec,0x38,0x83,0xb9,0x88,0x01,0x00,0x00,0x00};
static const U32 kCamWaits[]={0x29a854,0x2a57a0,0x2a5c34,0x2c46fd,0x2d4195,0x2d7bd2,0x2dc963,0x2e24e1,0x2eb225,0x379dac};
static bool install_camera(const S14_CoreAPI*api){
 HookPlan p(api);
 SsCamRet=game+0x1433b;p.jump(0x14330,kCamEntry,11,(void*)&SsCamBridge);
 for(U32 w:kCamWaits)p.call(w,0x306a30,(void*)&SsCamWait);
 return fin(p,"SAN14CE.SceneSkip.Camera",4);
}
static const U8 kTurnEntry[6]={0x40,0x53,0x48,0x83,0xec,0x20},kHistEntry[5]={0x48,0x8b,0xc4,0x55,0x57};
static bool install_summary(const S14_CoreAPI*api){HookPlan p(api);SsTurnRet=game+0x42ae26;p.jump(0x42ae20,kTurnEntry,6,(void*)&SsTurnBridge);return fin(p,"SAN14CE.SceneSkip.TurnSummary",3);}
static bool install_hist(const S14_CoreAPI*api){HookPlan p(api);SsHistRet=game+0x936985;p.jump(0x936980,kHistEntry,5,(void*)&SsHistBridge);return fin(p,"SAN14CE.SceneSkip.TurnSummaryNews",6);}
static void read_camera_switch(){ // 모드 설정 '카메라 자동 이동' = 통합편의 설정 [Camera] AutoFocus (1 움직임 / 0 막음)
 wchar_t p[1200];p[0]=0;if(!r39::game_root(p,1200))return;r39::wcat(p,1200,L"San14ModSettings.ini");
 cam_block=GetPrivateProfileIntW(L"Camera",L"AutoFocus",1,p)==0?1:0;
}

// ---- 설정 창 연결 ----  which: 0 재해 1 등용 2 보물 3 턴 끝 결과 창 4 카메라 막기(설정 '카메라 자동 이동' 끔과 같이 움직임, 파일은 통합편의 쪽이 씀)
extern "C" __declspec(dllexport) int SAN14_SceneSkipGet(int which){if(which>=0&&which<4)return (int)en[which];if(which==4)return (int)cam_block;if(which==5)return (int)pending_total();return 0;}
extern "C" __declspec(dllexport) int SAN14_SceneSkipHooked(int which){return which>=0&&which<7?(int)hooked[which]:0;}
extern "C" __declspec(dllexport) int SAN14_SceneSkipSet(int which,int on){
 if(which==4){InterlockedExchange(&cam_block,on?1:0);log_line("SceneSkip R42c: camera auto-move block switched to ",(unsigned)(on?1:0));return 1;}
 if(which<0||which>=4)return 0;InterlockedExchange(&en[which],on?1:0);
 wchar_t p[1200];ini_path(p,1200);if(p[0])WritePrivateProfileStringW(L"SceneSkip",kKey[which],on?L"1":L"0",p);
 return 1;
}
extern "C" __declspec(dllexport) long SAN14_SceneSkipCamStats(int i){return i>=0&&i<12?cs[i]:-1;}
extern "C" __declspec(dllexport) int SAN14_SceneSkipCamProbe(unsigned ret,unsigned outer){return cam_class(ret,outer);}
extern "C" __declspec(dllexport) const wchar_t*SAN14_SceneSkipSummaryText(){if(page_cur<page_cnt||wide_armed)return sumtext;build_all();build_page(0);page_cnt=0;return sumtext;}   // 미리 보기(첫 창 본문). 모은 것은 안 비움
extern "C" __declspec(dllexport) long SAN14_SceneSkipWideStats(int i){const long v[]={wide_ok,wide_shown,wide_filled,wide_fallback,wide_fill_other,(long)wide_bad,auto_ok,auto_closed,(long)auto_bad};return i>=0&&i<9?v[i]:-1;}
extern "C" __declspec(dllexport) void*SAN14_SceneSkipTestTable(int i){
 void*t[]={(void*)&SsBgm,(void*)&SsCutin,(void*)&SsNarr,(void*)&SsReset,(void*)&SsTurnEnd,(void*)&SsTurnBridge,(void*)sumtext,(void*)&dis_bgm,(void*)&dis_cut,(void*)&dis_narr,(void*)&dis_reset,(void*)&summary_shown,(void*)en,(void*)&SsAddHistory,
  (void*)&SsCollect,(void*)&SsCamFilter,(void*)cs,(void*)&SsCamWait,(void*)&SsDisSound,(void*)&SsDisPush,(void*)&SsDisWait,(void*)&SsHistFilter,(void*)&cam_block,(void*)cats,(void*)&ong_sound,(void*)&ong_push,(void*)&ong_wait,(void*)&SsDisSoundBridge,(void*)&SsCamBridge,(void*)&SsHistBridge,(void*)&SsCamRet,(void*)&SsHistRet,(void*)&SsTurnRet};
 return i>=0&&i<(int)(sizeof t/sizeof t[0])?t[i]:nullptr;
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,4,"SAN14_SceneSkip.dll"))return 0;core=api;game=api->resolved->image;
 wchar_t p[1200];ini_path(p,1200);
 if(p[0])for(int i=0;i<4;i++)en[i]=GetPrivateProfileIntW(L"SceneSkip",kKey[i],1,p)?1:0;
 read_camera_switch();
 hooked[0]=install_disaster(api);
 hooked[5]=install_ongoing(api);
 // B2·B3 는 묶음을 따로(한쪽 자리가 달라도 다른 쪽·재해는 그대로 동작)
 {HookPlan q(api);hooked[1]=SsEmployInstall(api,q,&en[1])&&q.install("SAN14CE.SceneSkip.Employ");if(!hooked[1])keep_bad(1,q);}
 {HookPlan q(api);hooked[2]=SsTreasureInstall(api,q,&en[2])&&q.install("SAN14CE.SceneSkip.Treasure");if(!hooked[2])keep_bad(2,q);}
 hooked[3]=install_summary(api);
 if(hooked[3])wide_ok=install_wide(api)?1:0;
 if(wide_ok)auto_ok=install_auto(api)?1:0;
 if(p[0])ss_summary_shape(GetPrivateProfileIntW(L"SceneSkip",L"SummaryWidth",36,p),GetPrivateProfileIntW(L"SceneSkip",L"SummaryLines",9,p));
 hooked[6]=hooked[3]&&install_hist(api);
 hooked[4]=install_camera(api);
 static const char*const nm[7]={" disaster="," employ="," treasure="," summary="," camera="," ongoing="," news="};
 char m[300];m[0]=0;r39::scat(m,300,"SceneSkip R42:");
 for(int i=0;i<7;i++){r39::scat(m,300,nm[i]);r39::scat(m,300,hooked[i]?"hooked":"NOT hooked");}
 r39::scat(m,300," (switches ");for(int i=0;i<4;i++){r39::snum(m,300,(unsigned)en[i]);}r39::scat(m,300," camera_block=");r39::snum(m,300,(unsigned)cam_block);r39::scat(m,300,") wide_result_window=");r39::scat(m,300,wide_ok?"on":"off(small narration window)");r39::scat(m,300," auto_close=");r39::scat(m,300,auto_ok?"on":"off");r39::scat(m,300," R42c");
 if(core->Log)core->Log(m);
 unsigned ok=0;for(int i=0;i<7;i++)ok+=hooked[i]?1:0;
 char st[120];st[0]=0;r39::scat(st,120,"Disaster=");r39::snum(st,120,(unsigned)en[0]);r39::scat(st,120," Employ=");r39::snum(st,120,(unsigned)en[1]);r39::scat(st,120," Treasure=");r39::snum(st,120,(unsigned)en[2]);
 r39::scat(st,120," Summary=");r39::snum(st,120,(unsigned)en[3]);r39::scat(st,120," CameraBlock=");r39::snum(st,120,(unsigned)cam_block);
 r39::start(4,"SAN14_SceneSkip.dll",ok==7?"OK":ok?"PARTIAL":"FAIL",ok,7,st,ok==7?"-":"site bytes differ or taken by another mod (that part keeps the original scene)");
 if(ok!=7){ // R43: 안 걸린 묶음마다 자리·기대/실제 바이트를 창으로
  static const char*const gname[7]={"disaster(0x2989a0/0x29a4b0/0x2ba220 call sites)","employ talk entry exe+0x6c6b10","treasure/discovery entry exe+0x6c6dd0","turn summary entry exe+0x42ae20","camera entry exe+0x14330 + wait calls","disaster-ongoing call sites exe+0x29a24a..0x29a71d","turn summary news entry exe+0x936980"};
  static const wchar_t*const geff[7]={L"재해 발생 연출 생략(4번)이 안 됨 → 그 연출은 원래대로",L"등용 대화 연출 생략이 안 됨 → 원래대로",L"보물 발견 창 생략이 안 됨 → 원래대로",L"턴 요약 생략이 안 됨 → 원래대로",L"카메라 이동 생략이 안 됨 → 원래대로",L"재해 진행 중 알림 생략이 안 됨 → 원래대로",L"턴 요약 뉴스 생략이 안 됨 → 원래대로"};
  static const char*const gstage[7]={"hook_disaster","hook_employ","hook_treasure","hook_summary","hook_camera","hook_ongoing","hook_news"};
  for(int i=0;i<7;i++){if(hooked[i])continue;if(i==6&&!hooked[3])continue; // news 는 summary 가 안 되면 당연히 안 함
   BadSite&b=ss_bad[i];
   if(b.set){char where[160],ex[80],ac[80];where[0]=ex[0]=ac[0]=0;r39::scat(where,sizeof where,gname[i]);r39::scat(where,sizeof where," @ exe+0x");r39::shex(where,sizeof where,b.rva);r39::scat(where,sizeof where," (");r39::scat(where,sizeof where,b.why);r39::scat(where,sizeof where,")");
    r39::sbytes(ex,sizeof ex,b.ex,b.len);r39::sbytes(ac,sizeof ac,b.ac,b.len);r39::fail(4,"SAN14_SceneSkip.dll",gstage[i],where,ex,ac,geff[i]);}
   else r39::fail(4,"SAN14_SceneSkip.dll",gstage[i],gname[i],"group installed","install returned 0 (no site recorded)",geff[i]);}
 }
 return ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
