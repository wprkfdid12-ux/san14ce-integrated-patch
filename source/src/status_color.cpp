// R39 29번: 항장·부상 색 표시 — 항장(공로 음수) = 이름+능력치 회색(RGB 150,150,146), 부상 = 이름+능력치 빨강, 둘 다면 빨강.
//  게임 글자 색 기호 "^NN" 을 씀(^03 = 순정 빨강 ff6445). 회색 기호는 게임에 없어서 "^0A" 를 새로 만든다(H5).
//  공통 지점(창마다 따로 안 함):
//   H1 0x230978  무장 전체 이름 함수(0x22eec0) 끝      → 항장/부상 무장의 이름을 최근 목록(32칸)에 적어 둠(문자열은 안 바꿈)
//   H2 0x9332c5  화면 배치 글자칸 넣기 0x9332b0 의 프롤로그 뒤(호출 3,000곳, 앞 16바이트는 14번 TraitHover 몫)   → 글자가 최근 목록 이름과 같으면 앞에 색 기호. 능력치 칸(0x85e750 안 호출)이면 능력치 색
//   H3 0x85e750  능력치 칸 함수 입구                  → 능력치 번호·기본값 기억
//   H4 0x231770  능력치 계산 함수 입구                → 마지막 무장·번호·스레드 기억(능력치 칸이 누구 것인지)
//   H5 0x154ec0  글자 색 기호 처리(CApp vfunc25)       → 기호 0x0A = 회색 추가(그 외 0~9 는 순정 그대로)
//  메시지창·기록·이름 복사 등 다른 곳의 이름 문자열은 건드리지 않음. 설정: SAN14Mods\modules\StatusColor.ini [StatusColor] Enabled=1
#include "common.h"
#include "r39_record.h"
#include "ic_after_restore.h"
static const S14_CoreAPI*core;static U8*game;static volatile long enabled=1;static unsigned colored_total;static bool first_logged;
extern "C"{
 void*ScRet6;void ScBridge6();
 void*ScRet2,*ScRet3,*ScRet4,*ScRet5,*ScExit5;
 U8*ScLastP;U32 ScLastIdx,ScLastTid;   // H4 이 채움
 U32 ScStatIdx,ScStatVal,ScStatBase;    // H3 이 채움
 void ScBridge1();void ScBridge2();void ScBridge3();void ScBridge4();void ScBridge5();
}
// 0=없음 1=회색(항장) 2=빨강(부상)
static int state_of(U8*p){
 if(at<U8>(p,0x161))return 2;
 if((int16_t)at<U16>(p,0x1be)<0)return 1;
 return 0;
}
static U8*person_by_id(unsigned id){
 if(id>0x1770)return nullptr;U8*db=*(U8**)(game+0x2780f60);if(!db)return nullptr;
 return *(U8**)(db+0x158+(size_t)id*8);
}
static bool is_person(U8*p){
 if(!p)return false;unsigned id=at<U16>(p,0x10);return id&&person_by_id(id)==p;
}
// 최근 이름 목록 (항장·부상 무장만)
struct NameEnt{U16 id;U16 len;wchar_t name[64];};
static NameEnt ring[128];static volatile long ring_pos,ring_used;
// 같은 스레드에서 방금 이름을 꺼낸 무장(건강해도 기록): 동명이인일 때 이 무장 기준으로 판정 → 건강한 동명이인이 칠해지지 않게
static volatile DWORD last_tid;static U16 last_id,last_len;static wchar_t last_name[64];
extern "C" void ScNameSeen(U8*p,const wchar_t*buf){
 if(!enabled||!p||!buf)return;
 if(!is_person(p))return;
 {unsigned m=0;while(m<63&&buf[m])m++;if(m&&!buf[m]){last_tid=0;for(unsigned k=0;k<m;k++)last_name[k]=buf[k];last_name[m]=0;last_len=(U16)m;last_id=at<U16>(p,0x10);last_tid=GetCurrentThreadId();}}
 if(!state_of(p))return;
 unsigned n=0;while(n<63&&buf[n])n++;if(!n||buf[n])return;
 unsigned id=at<U16>(p,0x10);
 for(int i=0;i<128;i++)if(ring[i].id==id&&ring[i].len==n){bool same=true;for(unsigned k=0;k<n;k++)if(ring[i].name[k]!=buf[k]){same=false;break;}if(same)return;}
 long slot=(InterlockedIncrement(&ring_pos)-1)&127;NameEnt&e=ring[slot];
 e.id=0;e.len=0;for(unsigned k=0;k<n;k++)e.name[k]=buf[k];e.name[n]=0;e.len=(U16)n;e.id=(U16)id;
 if(ring_used<128)InterlockedIncrement(&ring_used);
}
// 색 입힌 글자 만들기: 앞에 기호, 안쪽 색 기호(^00 제외)도 같은 색으로
// 색 입힌 글자 버퍼: 정적 16칸 링(칸당 1100 wchar). 위치는 InterlockedIncrement 로 잡음(스레드 겹쳐도 같은 칸 동시 사용은 16번 뒤에나).
// 수명: 0x9332b0 → 0x9333c0 등이 호출 안에서 곧바로 복사하므로 호출 동안만 살아 있으면 됨. 길이 상한: 원문+기호가 1090자를 넘으면 칠하지 않고 원문.
static wchar_t outbuf[16][1100];static volatile long outpos;
static const wchar_t*paint(const wchar_t*text,int st){
 wchar_t*o=outbuf[(InterlockedIncrement(&outpos)-1)&15];const wchar_t c1=L'0',c2=st==2?L'3':L'A';
 unsigned j=0;o[j++]=L'^';o[j++]=c1;o[j++]=c2;
 auto hex=[](wchar_t ch){return (ch>=L'0'&&ch<=L'9')||(ch>=L'A'&&ch<=L'F');};
 for(unsigned i=0;text[i];i++){
  if(j>=1090)return text; // 너무 길면 원래대로
  if(text[i]==L'^'&&text[i+1]==L'^'){o[j++]=L'^';o[j++]=L'^';i++;continue;}
  if(text[i]==L'^'&&hex(text[i+1])&&hex(text[i+2])&&!(text[i+1]==L'0'&&text[i+2]==L'0')){o[j++]=L'^';o[j++]=c1;o[j++]=c2;i+=2;continue;}
  o[j++]=text[i];
 }
 o[j]=0;return o;
}
static void note(){
 colored_total++;
 if(!first_logged){first_logged=true;r39::counter(29,"SAN14_StatusColor.dll","first_colored",1);}
}
// 편집·입력 화면 막기: 글자칸 묶음(rcx)은 레이아웃 객체 +0x130 에 들어 있음(호출 827곳이 lea/add rcx,+0x130 확인).
// 주인 객체(rcx-0x130, 안 되면 rcx 자체)의 vtable[-1] → RTTI COL(서명 1, pSelf 일치 검사) → 클래스 이름에 "Edit"/"Input" 있으면 칠하지 않음.
// 검증 못 하면 칠하지 않음(보수). vtable 별 결과는 64칸 캐시.
static struct{U8*vt;U8 allow;}vcache[64];
// R40 진단(A5/A6): 칠한 화면·막은 화면의 클래스 이름을 처음 40개만 적용확인.log 에 남김 → 다음 판에서 빠진 화면(무장일람 등) 확정용
static U8*diag_seen[64];static volatile long diag_n;
static void diag_class(U8*vt,const char*name,int allow){
 for(long i=0;i<diag_n&&i<64;i++)if(diag_seen[i]==vt)return;
 long k=InterlockedIncrement(&diag_n)-1;if(k>=40)return;diag_seen[k]=vt;
 char line[256];line[0]=0;r39::scat(line,256,"R40D\t29\tSAN14_StatusColor.dll\t");r39::scat(line,256,allow?"paint_screen\t":"blocked_screen\t");
 char nm[120];int j=0;for(;j<118&&name[j];j++)nm[j]=name[j];nm[j]=0;r39::scat(line,256,nm);r39::append_line(line);
}
static int rtti_allow(U8*vt){ // 1 허용 0 막음 -1 검증 실패
 const size_t lo=0x1343000,hi=0x1a23000;size_t r=(size_t)(vt-game);
 if(vt<game||r<lo||r>=hi)return -1;
 unsigned h=(unsigned)(((UPTR)vt>>3)&63);if(vcache[h].vt==vt)return vcache[h].allow;
 if(!core->Readable(vt-8,8))return -1;U8*col=*(U8**)(vt-8);size_t cr=(size_t)(col-game);
 if(col<game||cr<lo||cr>=hi||!core->Readable(col,0x18))return -1;
 if(at<U32>(col,0)!=1||at<U32>(col,0x14)!=(U32)cr)return -1;
 U8*td=game+at<U32>(col,0xc);if((size_t)(td-game)>=S14_GAME_IMAGE||!core->Readable(td,0x10+128))return -1;
 const char*n=(const char*)td+0x10;int allow=1;
 // R40 A6: 메뉴·명령·버튼 화면의 글자(예: 명령 '진군')가 같은 이름의 무장(진군) 때문에 칠해지던 것 → 그런 화면은 칠하지 않음
 auto has=[&](int i,const char*w){for(int k=0;w[k];k++)if(n[i+k]!=w[k])return false;return true;};
 for(int i=0;i<124&&n[i];i++){
  if(has(i,"Edit")||has(i,"Input")||has(i,"Cmd")||has(i,"Command")||has(i,"Menu")||has(i,"Btn")||has(i,"Button")||has(i,"Order")){allow=0;break;}
 }
 vcache[h].allow=(U8)allow;vcache[h].vt=vt;diag_class(vt,n,allow);return allow;
}
static bool layout_allows(U8*holder){
 if(!holder)return false;
 U8*cand[2]={holder-0x130,holder};
 for(U8*o:cand){if(!core->Readable(o,8))continue;int a=rtti_allow(*(U8**)o);if(a>=0)return a==1;}
 return false;
}
extern "C" const wchar_t*ScLabel(const wchar_t*text,U8*ret,U8*holder){
 if(!enabled||!text||!text[0])return text;
 if(ret==game+0x85e90c){ // 능력치 칸
  if(ScLastTid!=GetCurrentThreadId())return text;
  U8*p=ScLastP;unsigned k=ScStatIdx;if(k>4||ScLastIdx!=k||!is_person(p))return text;
  if(at<U8>(p,0xec+k)!=(U8)ScStatBase)return text;
  int st=state_of(p);if(!st||!layout_allows(holder))return text;note();return paint(text,st);
 }
 if(!ring_used||text[0]==L'^')return text; // 이미 색 기호로 시작(예: 34번 "^03!")하면 손대지 않음
 unsigned n=0;while(n<64&&text[n])n++;if(n>=64)return text;
 if(last_tid==GetCurrentThreadId()&&last_len==n){bool same=true;for(unsigned k=0;k<n;k++)if(last_name[k]!=text[k]){same=false;break;}
  if(same){U8*p=person_by_id(last_id);if(!is_person(p))return text;int st=state_of(p);if(!st||!layout_allows(holder))return text;note();return paint(text,st);}}
 for(int i=0;i<128;i++){
  NameEnt&e=ring[i];if(!e.id||e.len!=n||e.name[0]!=text[0])continue;
  bool same=true;for(unsigned k=0;k<n;k++)if(e.name[k]!=text[k]){same=false;break;}
  if(!same)continue;
  U8*p=person_by_id(e.id);if(!is_person(p))return text;int st=state_of(p);if(!st||!layout_allows(holder))return text;
  note();return paint(text,st);
 }
 return text;
}

// R41 A5: 표(무장일람·무장선택 등)는 글자칸 넣기(0x9332b0)를 거치지 않고 글자 요소에 바로 넣음(0x9333c0 직접 호출, 표 칸 0x86f920 등).
//  H6 0x9333c9(0x9333c0 프롤로그 안): 0x9332b0 에서 온 호출은 H2 가 이미 판정했으므로 건너뜀. 그 밖의 호출은
//   ① 같은 스레드에서 방금 이 무장 이름을 꺼낸 경우(정확히 그 무장) ② 표 컨트롤 코드(0x868350~0x87b350)에서 온 경우 최근 이름 목록으로 판정.
//   메뉴·명령 글자(A6)는 ①②에 해당하지 않으므로 칠하지 않음. 진단: 처음 30곳의 호출 자리를 적용확인.log R41D 줄로.
static size_t ddiag_seen[64];static volatile long ddiag_n;
static void diag_direct(size_t r,int via,int painted){
 for(long i=0;i<ddiag_n&&i<64;i++)if(ddiag_seen[i]==r)return;
 long k=InterlockedIncrement(&ddiag_n)-1;if(k>=30)return;ddiag_seen[k]=r;
 char line[200];line[0]=0;r39::scat(line,200,"R41D\t29\tSAN14_StatusColor.dll\tdirect\t");
 static const char*hx="0123456789abcdef";char h[9];for(int i=0;i<8;i++)h[i]=hx[(r>>(28-4*i))&15];h[8]=0;r39::scat(line,200,h);
 r39::scat(line,200,via==1?"\tvia=last":"\tvia=ring");r39::scat(line,200,painted?"\tpaint=1":"\tpaint=0");r39::append_line(line);
}
extern "C" const wchar_t*ScText(U8*elem,const wchar_t*text,U8*ret){
 if(!enabled||!text||!text[0]||text[0]==L'^'||!game)return text;
 size_t r=(size_t)(ret-game);
 if(ret>=game&&r>=0x9332b0&&r<0x9333c0)return text; // 0x9332b0 경로(H2 판정 끝)
 unsigned n=0;while(n<64&&text[n])n++;if(n>=64)return text;
 U8*p=nullptr;int via=0;
 if(last_tid==GetCurrentThreadId()&&last_len==n){bool same=true;for(unsigned k=0;k<n;k++)if(last_name[k]!=text[k]){same=false;break;}if(same){p=person_by_id(last_id);via=1;}}
 if(!p&&ring_used){
  for(int i=0;i<128;i++){NameEnt&e=ring[i];if(!e.id||e.len!=n||e.name[0]!=text[0])continue;bool same=true;for(unsigned k=0;k<n;k++)if(e.name[k]!=text[k]){same=false;break;}if(same){p=person_by_id(e.id);via=2;break;}}
 }
 if(!p||!is_person(p))return text;
 int st=state_of(p);if(!st)return text;
 bool table=ret>=game&&r>=0x868350&&r<0x87b350;
 bool ok=via==1||table;
 diag_direct(ret>=game?r:0,via,ok);
 if(!ok)return text;
 note();return paint(text,st);
}
// 회색 기호 0x0A: 순정 기호 4번과 같은 방식(색값 → 감마표 0x32ec70 → 글자 객체 vfunc +0x48)
extern "C" void ScGray(U8*ctx){
 U8*obj=*(U8**)ctx;if(!obj)return;void**vt=*(void***)obj;
 typedef U32(*Gamma)(U32);typedef void(*SetColor)(U8*,U32);
 U32 c=((Gamma)(game+0x32ec70))(0xff929696u); // ABGR: R=150 G=150 B=146
 ((SetColor)vt[0x48/8])(obj,c);
}
extern "C" __declspec(dllexport) void*SAN14_StatusColorTestTable(int i){
 void*t[]={(void*)&ScBridge1,(void*)&ScBridge2,(void*)&ScBridge3,(void*)&ScBridge4,(void*)&ScBridge5,(void*)&ScRet2,(void*)&ScRet3,(void*)&ScRet4,(void*)&ScRet5,(void*)&ScExit5,(void*)&ScLabel,(void*)&ScNameSeen,(void*)&ScLastP,(void*)&ScStatIdx,(void*)&ScBridge6,(void*)&ScText,(void*)&ScRet6};
 return i>=0&&i<17?t[i]:nullptr;
}
extern "C" __declspec(dllexport) void SAN14_StatusColorSetEnabled(int on){InterlockedExchange(&enabled,on?1:0);}
extern "C" __declspec(dllexport) int SAN14_StatusColorState(){return enabled;}
static bool sc_ok;
static void sc_install(){
 const S14_CoreAPI*api=core;
 HookPlan plan(api);
 static const U8 e1[]={0x48,0x8b,0xc3,0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f,0xc3};plan.jump(0x230978,e1,14,(void*)&ScBridge1);
 static const U8 e2[]={0x48,0x63,0xf2,0x49,0x8b,0xf8,0x8b,0xd6,0x4c,0x8b,0xf1};ScRet2=game+0x9332c5+11;plan.jump(0x9332c5,e2,11,(void*)&ScBridge2);   // R39b: 함수 앞 16바이트는 14번 TraitHover 가 확인·사용 → 프롤로그 뒤에 걸어 공존
 static const U8 e3[]={0x40,0x53,0x55,0x56,0x57,0x41,0x56,0x48,0x81,0xec,0x40,0x08,0x00,0x00};ScRet3=game+0x85e750 +14;plan.jump(0x85e750,e3,14,(void*)&ScBridge3);
 static const U8 e4[]={0x89,0x54,0x24,0x10,0x55,0x56,0x57};ScRet4=game+0x231770 +7;plan.jump(0x231770,e4,7,(void*)&ScBridge4);
 static const U8 e5[]={0x41,0x0f,0xb6,0x40,0x08,0x49,0x8b,0xf8,0x83,0xf8,0x09};ScRet5=game+0x154ec0 +11;ScExit5=game+0x155128;plan.jump(0x154ec0,e5,11,(void*)&ScBridge5);
 // 되돌아갈 자리 바이트도 확인(0x154ecb ja, 0x155128 에필로그)
 static const U8 j5[]={0x0f,0x87,0x57,0x02,0x00,0x00};static const U8 x5[]={0x48,0x8b,0x4d,0x3f,0x48,0x33,0xcc};
 bool ret_ok=core->Readable(game+0x154ecb,6)&&!memcmp(game+0x154ecb,j5,6)&&core->Readable(game+0x155128,7)&&!memcmp(game+0x155128,x5,7);
 bool ok=ret_ok&&plan.install("SAN14CE.StatusColor");
 if(!ret_ok){U8 cur[7]={};bool r1=core->Readable(game+0x154ecb,6),r2=core->Readable(game+0x155128,7);if(r1&&memcmp(game+0x154ecb,j5,6))r39::fail_bytes(29,"SAN14_StatusColor.dll","return site ja exe+0x154ecb",0x154ecb,j5,game+0x154ecb,6,L"항장·부상 색 표시(29번)가 적용 안 됨 → 게임 기본 색. 다른 기능은 그대로");else r39::fail_bytes(29,"SAN14_StatusColor.dll","return site epilogue exe+0x155128",0x155128,x5,r2?game+0x155128:cur,7,L"항장·부상 색 표시(29번)가 적용 안 됨 → 게임 기본 색. 다른 기능은 그대로");}
 // R41 A5: 표 경로는 따로 묶음(실패해도 나머지 29번은 그대로)
 bool ok6=false;
 if(ok){HookPlan p6(api);static const U8 e6[]={0x4c,0x8b,0xdc,0x53,0x56,0x48,0x81,0xec,0x38,0x08,0x00,0x00};ScRet6=game+0x9333c9+12;p6.jump(0x9333c9,e6,12,(void*)&ScBridge6);ok6=p6.install("SAN14CE.StatusColor.Table");
  if(core->Log)core->Log(ok6?"StatusColor R41: table/list text path hooked (0x9333c0).":"StatusColor R41: table/list text path NOT hooked (bytes differ / conflict); info windows still colored.");}
 if(core->Log)core->Log(ok?"StatusColor R39: 5 sites hooked (exile gray / injury red on names+stats).":"StatusColor R39: hook group NOT installed (bytes differ / conflict). Nothing changed.");
 r39::start(29,"SAN14_StatusColor.dll",ok?(enabled?(ok6?"OK":"PART"):"OFF"):"FAIL",ok?(ok6?6:5):0,6,enabled?"Enabled=1":"Enabled=0",ok?"-":"hook bytes differ or another mod patched the site");
 if(!ok&&ret_ok)r39::fail_plan(29,"SAN14_StatusColor.dll",plan,"status color sites(5) first failing",L"항장·부상 색 표시(29번)가 적용 안 됨 → 게임 기본 색. 다른 기능은 그대로");
 if(ok&&!ok6)r39::fail(29,"SAN14_StatusColor.dll","hook_table","table/list text path exe+0x9333c9 (group 2)","4c 8b dc 53 56 48 81 ec 38 08 00 00","bytes differ or taken by another mod",L"일람표(목록) 글자 색만 안 칠해짐(정보창 색은 됨). 다른 기능은 그대로");
 sc_ok=ok;
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,29,"SAN14_StatusColor.dll"))return 0;core=api;game=api->resolved->image;
 wchar_t ini[1024];if(r39::game_root(ini,1024)){r39::wcat(ini,1024,L"SAN14Mods\\modules\\StatusColor.ini");enabled=GetPrivateProfileIntW(L"StatusColor",L"Enabled",1,ini)?1:0;}
 // 정식판 뒤(11-08): 0x231770 이 통합편의 원본 확인 자리 안 → 통합편의 확인이 끝난 뒤 고침(ic_after_restore.h)
 return icw::run_after_restore(sc_install,api,"StatusColor")?(sc_ok?1:0):1;
}

BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
