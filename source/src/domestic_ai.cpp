// 22번 AI 내정 순환(군주 반영) — R42g (2026-11-02) 새 모듈 SAN14_DomesticAI.dll
//  무엇: AI 세력이 지역 개발 중점(상업·농업·병영)을 고르는 게임 판정 함수를 우리 규칙으로 바꿈.
//        AI 는 그 결과를 자기 결정으로 알고 담당관 임명 명령에 그대로 씀(덮어쓰기 싸움 없음). 플레이어 세력 지역은 원래 판정 그대로.
//  어디: 행동트리 노드 CDomesticPriorityDevelopmentSelectNode(0x7b0b0) 안의 단 하나뿐인 호출 0x7b17b → 0xb2af0(군단, 지역, 결과*, 가중치 3개).
//        게임 원래 판정 = 군단 방침(+0x1a 2/3/4)이면 그 항목, 아니면 (군단장 물자선호+100)×가중치×단계 가 가장 작은 항목(같으면 상업).
//  규칙: domestic_ai_rule.h (정치 70↑ 전부 / 40~69 절반 / 40↓ 3분의 1 순환, 나머지는 군주 물자 몰빵, 달마다 섞임).
//  자료(게임 코드로 확인): db=*(exe+0x2780f60) · world=*(db+0x871e8) · 연=world+0x34(i16) 월=+0x36(u8)
//        지역표 *(db+0x6caf0) 0x98 간격 1..500 · 지역+0x36 담당관 id · +0x3a 중심 칸 · +0x3c/+0x3e/+0x40 상업·농업·병영 · +0x72 중점
//        지역 소유 세력 0x22e260(지역) · 플레이어 세력 0x330b90(world) · 세력+0x10 군주 id · 무장표 db+0x158+id*8(≤0x1770) · 유효 0x331780
//        정치 = 0x231770(무장, 3, 0xfff) (게임 AI 노드가 쓰는 능력치 읽기 그대로) · 물자 금/군량/병사 = 무장+0x170/+0x172/+0x174 (i16, 20=중시)
//  기록: 적용확인.log 시작 줄(R42D 22 … start) + 게임 달이 바뀔 때마다 한 줄(R42D … month) — 프레임 영향 없음(파일 쓰기 월 1회).
//  설정: SAN14Mods\modules\DomesticAI.ini [DomesticAI] AiRotate=1. export SAN14_DomesticAIGet/Set.
//  충돌: 다른 AI 모드(SAN14_AI)는 이 노드를 감싸 둠(vtable 0x134f918 ≠ 0x7b0b0). 그쪽이 원래 노드를 부르면 우리 규칙도 돌고, 안 부르면 안 돎 →
//        시작 줄에 적어 두고 실제로 돌았는지는 월별 줄 calls 로 확인(그쪽이 맞출 일, 우리는 튕기지만 않게).
#include "common.h"
#include "r39_record.h"
#include "domestic_ai_rule.h"
static const S14_CoreAPI*core;static U8*game;static volatile long enabled=1;
using Select=void(*)(U8*,U8*,int*,int,int,int);using FnP_P=U8*(*)(U8*);using FnI_P=int(*)(U8*);using FnI_PII=int(*)(U8*,int,int);
static U8*db(){return *(U8**)(game+0x2780f60);}
static U8*world(){U8*d=db();return d?*(U8**)(d+0x871e8):nullptr;}
static bool obj_ok(U8*p){return p&&fn<FnI_P>(game,0x331780)(p)!=0;}
static unsigned area_index(U8*a){
 U8*d=db();if(!d||!a)return 0;U8*base=*(U8**)(d+0x6caf0);if(!base||a<base)return 0;
 UPTR off=(UPTR)(a-base);if(off%0x98)return 0;UPTR i=off/0x98;return (i>=1&&i<=500)?(unsigned)i:0;
}
static U8*person(unsigned id){U8*d=db();if(!d||!id||id>0x1770)return nullptr;U8*p=*(U8**)(d+0x158+(UPTR)id*8);return obj_ok(p)?p:nullptr;}
// ---- 월별 요약
struct ForceSeen{unsigned calls;int politics;unsigned share,pref;};
static ForceSeen seen[64];static volatile long c_calls,c_player,c_ai,c_rot,c_fix,c_same,c_orig;static long cur_ym=-1;static volatile long log_lock;
static void flush_month(long ym){
 if(!r39::s14_diag()||ym<0)return;char line[1024];line[0]=0;const size_t C=sizeof line;
 r39::scat(line,C,"R42D\t22\tSAN14_DomesticAI.dll\tmonth\t");r39::snum(line,C,(unsigned)(ym/12));r39::scat(line,C,"/");r39::snum(line,C,(unsigned)(ym%12+1));
 r39::scat(line,C,"\tcalls=");r39::snum(line,C,(unsigned)c_calls);r39::scat(line,C,"\tplayer=");r39::snum(line,C,(unsigned)c_player);
 r39::scat(line,C,"\tai=");r39::snum(line,C,(unsigned)c_ai);r39::scat(line,C,"\trot=");r39::snum(line,C,(unsigned)c_rot);
 r39::scat(line,C,"\tfix=");r39::snum(line,C,(unsigned)c_fix);r39::scat(line,C,"\tsame=");r39::snum(line,C,(unsigned)c_same);
 r39::scat(line,C,"\torig=");r39::snum(line,C,(unsigned)c_orig);r39::scat(line,C,"\tforces=");
 bool first=true;
 for(unsigned f=0;f<64;f++){if(!seen[f].calls)continue;if(!first)r39::scat(line,C,",");first=false;
  r39::snum(line,C,f);r39::scat(line,C,":");r39::snum(line,C,(unsigned)(seen[f].politics<0?0:seen[f].politics));r39::scat(line,C,":");r39::snum(line,C,seen[f].share);r39::scat(line,C,":");r39::snum(line,C,seen[f].pref);r39::scat(line,C,":");r39::snum(line,C,seen[f].calls);
  if(r39::slen(line)>900){r39::scat(line,C,",...");break;}}
 r39::scat(line,C,"\t");r39::stamp(line,C);r39::append_line(line);
 for(unsigned f=0;f<64;f++)seen[f]=ForceSeen{};c_calls=c_player=c_ai=c_rot=c_fix=c_same=c_orig=0;
}
// ---- AI 명령서 99장(11차 P212): 브리지는 domestic_ai_bridge.S. 스위치는 옛 DomesticCE 와 같은 칸(SAN14_DomesticCE.ini [Automation] Enabled·AiOrders, 없으면 켬)
static volatile LONG DaiTopups;
extern "C"{volatile LONG DaiOrdOn,DaiOrdHits;U8*DaiGame;void*DaiOrdRet;void DaiOrdBridge();}
// 21차(사용자 11-11): AI 명승 투자는 '투자하고 남는 금 ≥ 비용×AiScenicMult 이고 ≥ AiScenicFloor' 일 때만(플레이어 세력 제외). 브리지 = domestic_ai_bridge.S
extern "C"{volatile LONG DaiScOn,DaiScBlocked,DaiScPassed;long long DaiScMult=2,DaiScFloor=5000;void*DaiScPass;void*DaiScFail;void DaiScenicBridge();}
static bool scenic_ok;
static bool orders_ok;
extern "C" void DaiOrdSwitch(){wchar_t p[1024];if(!r39::game_root(p,1024))return;r39::wcat(p,1024,L"SAN14_DomesticCE.ini");
 {int m=(int)GetPrivateProfileIntW(L"Automation",L"AiScenicMult",2,p),f=(int)GetPrivateProfileIntW(L"Automation",L"AiScenicFloor",5000,p);
  if(m<0)m=0;if(m>20)m=20;if(f<0)f=0;if(f>150000)f=150000;DaiScMult=m;DaiScFloor=f;
  InterlockedExchange(&DaiScOn,(scenic_ok&&GetPrivateProfileIntW(L"Automation",L"AiScenicGold",1,p))?1:0);}
 InterlockedExchange(&DaiOrdOn,(orders_ok&&GetPrivateProfileIntW(L"Automation",L"Enabled",1,p)&&GetPrivateProfileIntW(L"Automation",L"AiOrders",1,p))?1:0);}
static void orders_line(int year,int month){   // 달마다 한 줄: 채우는 순간에 99 로 넣은 횟수(읽기 전용 기록)
 if(!r39::s14_diag()){InterlockedExchange(&DaiOrdHits,0);InterlockedExchange(&DaiTopups,0);InterlockedExchange(&DaiScBlocked,0);InterlockedExchange(&DaiScPassed,0);return;}
 char m[200];m[0]=0;const size_t C=sizeof m;r39::scat(m,C,"R42D\t22\tSAN14_DomesticAI.dll\torders\t");r39::snum(m,C,(unsigned)year);r39::scat(m,C,"/");r39::snum(m,C,(unsigned)month);
 r39::scat(m,C,"\ton=");r39::snum(m,C,(unsigned)DaiOrdOn);r39::scat(m,C,"\tset99=");r39::snum(m,C,(unsigned)InterlockedExchange(&DaiOrdHits,0));r39::scat(m,C,"\ttopup=");r39::snum(m,C,(unsigned)InterlockedExchange(&DaiTopups,0));r39::scat(m,C,"\tscenic_blocked=");r39::snum(m,C,(unsigned)InterlockedExchange(&DaiScBlocked,0));r39::scat(m,C,"\tscenic_passed=");r39::snum(m,C,(unsigned)InterlockedExchange(&DaiScPassed,0));r39::scat(m,C,"\t");r39::stamp(m,C);r39::append_line(m);}
// 17차(P223): 14·15차가 AI 세력 객체+0x14 에 99 를 써 넣은 것 되돌리기.
//  세력+0x13 부터는 세력끼리의 우호도 표(BYTE 52칸, 칸 = 상대 세력 번호)이고 +0x14 는 '1번 세력과의 우호도' 였다.
//  게임은 우호도를 늘 양쪽에 같은 만큼 더한다(0x25a080: A[+0x13+B번호]·B[+0x13+A번호]) → 1번 세력 쪽 칸(1번[+0x13+f])에 원래 값이 그대로 남아 있음.
//  되돌리는 조건: 1번 세력과의 칸만 양쪽이 다르고, 다른 세력 쌍은 전부 양쪽이 같을 때만(= 우리 흔적일 때만). 한 번 고치면 양쪽이 같아져 다시 안 건드림.
//  달이 바뀔 때 AI 판단 자리에서 한 번(52×52 바이트 비교). 매 프레임·주기 읽기 아님.
static volatile long rel_logged;
static bool force_live(U8*f){return f&&at<U8*>(f,0)==game+0x13a9880&&obj_ok(f)&&at<U16>(f,0x10)!=0&&at<U16>(f,0x10)<=0x1770;}
static void rel_repair(int year,int month){
 U8*d=db();if(!d)return;U8*F[0x34];bool L[0x34];for(unsigned f=0;f<=0x33;f++){F[f]=at<U8*>(d,0xdf30+8*(size_t)f);L[f]=f&&F[f]!=F[0]&&force_live(F[f]);}
 if(!L[1])return;
 unsigned other=0,col=0,fixed=0;
 for(unsigned a=2;a<=0x33;a++){if(!L[a])continue;for(unsigned b=a+1;b<=0x33;b++)if(L[b]&&at<U8>(F[a],0x13+b)!=at<U8>(F[b],0x13+a))other++;
  if(at<U8>(F[a],0x14)!=at<U8>(F[1],0x13+a))col++;}
 if(!col)return;
 if(!other){for(unsigned a=2;a<=0x33;a++)if(L[a]&&at<U8>(F[a],0x14)!=at<U8>(F[1],0x13+a)){at<U8>(F[a],0x14)=at<U8>(F[1],0x13+a);fixed++;}
  // 1번 세력의 자기 칸(1번[+0x14])도 99 로 덮였을 수 있음 → 다른 세력들의 자기 칸이 전부 같은 값이면 그 값으로
  int self=-1;bool same=true;for(unsigned a=2;a<=0x33;a++)if(L[a]){int v=at<U8>(F[a],0x13+a);if(self<0)self=v;else if(v!=self)same=false;}
  if(self>=0&&same&&at<U8>(F[1],0x14)!=(U8)self){at<U8>(F[1],0x14)=(U8)self;fixed++;}}
 if(InterlockedIncrement(&rel_logged)<=6){char m[220];m[0]=0;const size_t C=sizeof m;r39::scat(m,C,"R42D\t22\tSAN14_DomesticAI.dll\trelfix\t");r39::snum(m,C,(unsigned)year);r39::scat(m,C,"/");r39::snum(m,C,(unsigned)month);
  r39::scat(m,C,"\tforce1_col_diff=");r39::snum(m,C,col);r39::scat(m,C,"\tother_pairs_diff=");r39::snum(m,C,other);r39::scat(m,C,"\trestored=");r39::snum(m,C,fixed);r39::scat(m,C,other?"\tskipped: relations are not symmetric in this save\t":"\t");r39::stamp(m,C);r39::append_line(m);}
}
static void note_month(int year,int month){
 long ym=(long)year*12+(month-1);if(ym==cur_ym)return;
 if(__atomic_exchange_n(&log_lock,1L,__ATOMIC_SEQ_CST)!=0)return;
 if(ym!=cur_ym){long prev=cur_ym;cur_ym=ym;if(orders_ok){if(prev>=0)orders_line((int)(prev/12),(int)(prev%12+1));DaiOrdSwitch();}flush_month(prev);}
 InterlockedExchange(&log_lock,0);
}
// ---- 판정 자리 (게임 AI 스레드에서 불림)
extern "C" void DomesticAiSelect(U8*district,U8*area,int*out,int p1,int p2,int p3){
 Select orig=fn<Select>(game,0xb2af0);
 // 13차(P212): AI 세력이 내정 판단을 하는 바로 그 순간, 그 세력의 명령서가 99 보다 적으면 99 로(그 세력 한 칸만, 주기적으로 읽지 않음).
 //  12차의 채우기 자리(0x9fa17)는 명령서가 돌아올 때만 불려 한 달에 0~4번뿐이었음(데모 기록 set99) → AI 가 일을 시작할 때 채워 줌.
 //  14차: 13차 데모 기록(44달)에서 내정 판단을 안 하는 세력은 몇 달씩 0장으로 남음 → 판단이 불릴 때마다 AI 세력 전부(최대 51칸)를 본다.
 // 16차(P220·P223): 명령서는 세력(CForceData)이 아니라 군단(CDistrictData, db+0xe0d0+8*번호, vtable exe+0x13a98f0)의 +0x14 다.
 //  군단+0x10 = 세력 번호(BYTE). 플레이어 세력의 군단(위임 군단 포함)은 안 건드린다. 세력 객체에는 더 이상 아무것도 안 쓴다.
 if(DaiOrdOn&&area){U8*w0=world();U8*d0=at<U8*>(game,0x2780f60);if(w0&&d0){unsigned pl=at<U8>(w0,0x3a);U8*z=at<U8*>(d0,0xe0d0);
  for(unsigned k=1;k<=0x33;k++){U8*di=at<U8*>(d0,0xe0d0+8*(size_t)k);if(!di||di==z||at<U8*>(di,0)!=game+0x13a98f0||!obj_ok(di))continue;
   unsigned f=at<U8>(di,0x10);if(!f||f>0x33||f==pl)continue;if(at<U8>(di,0x14)<99){at<U8>(di,0x14)=99;InterlockedIncrement(&DaiTopups);}}}}
 // 17차(P223): 달이 바뀐 뒤 첫 판단 때 한 번 — 22번 설정(순환·명령서)을 꺼 둬도 되돌리기는 돈다.
 {static volatile long rel_ym=-1;U8*w1=world();if(w1&&area){int y1=at<short>(w1,0x34),m1=at<U8>(w1,0x36);if(y1>=100&&m1>=1&&m1<=12){long ym1=(long)y1*12+m1;if(InterlockedExchange(&rel_ym,ym1)!=ym1)rel_repair(y1,m1);}}}
 if(!enabled||!out){orig(district,area,out,p1,p2,p3);return;}
 InterlockedIncrement(&c_calls);
 unsigned idx=area_index(area);U8*w=world();
 if(!idx||!w){InterlockedIncrement(&c_orig);orig(district,area,out,p1,p2,p3);return;}
 U8*force=fn<FnP_P>(game,0x22e260)(area);U8*player=fn<FnP_P>(game,0x330b90)(w);
 if(!force||force==player||!obj_ok(force)){InterlockedIncrement(&c_player);orig(district,area,out,p1,p2,p3);return;}
 U8*ruler=person(at<U16>(force,0x10));
 if(!ruler){InterlockedIncrement(&c_orig);orig(district,area,out,p1,p2,p3);return;}
 int year=at<short>(w,0x34);int month=at<U8>(w,0x36);if(month<1||month>12){InterlockedIncrement(&c_orig);orig(district,area,out,p1,p2,p3);return;}
 // 세력 번호: 중심 칸 소유(0x22e260 과 같은 자리)
 unsigned fid=0;{unsigned hex=at<U16>(area,0x3a);U8*d=db();if(hex<=0xbd0f){U8*tile=*(U8**)(d+0xe270+(UPTR)hex*8);if(tile)fid=at<U8>(tile,0x14);}}
 dai::Ruler r;r.politics=fn<FnI_PII>(game,0x231770)(ruler,3,0xfff);
 r.gold=at<short>(ruler,0x170);r.food=at<short>(ruler,0x172);r.troops=at<short>(ruler,0x174);
 dai::Area a;a.commerce=at<U16>(area,0x3c);a.agri=at<U16>(area,0x3e);a.barracks=at<U16>(area,0x40);a.focus=at<U8>(area,0x72);
 a.officer_valid=person(at<U16>(area,0x36))!=nullptr;
 dai::Result res=dai::decide(a,r,idx,fid,year,month);
 *out=(int)res.focus;
 InterlockedIncrement(&c_ai);if(res.rotated)InterlockedIncrement(&c_rot);else InterlockedIncrement(&c_fix);if(!res.focus)InterlockedIncrement(&c_same);
 if(fid<64){ForceSeen&s=seen[fid];s.calls++;s.politics=r.politics;s.share=dai::share_for(r.politics);s.pref=dai::preferred(r);}
 note_month(year,month);
}
// ---- 설정
static bool ini_path(wchar_t*p){if(!r39::game_root(p,1024))return false;r39::wcat(p,1024,L"SAN14Mods\\modules\\DomesticAI.ini");return true;}
static void save_ini(){wchar_t p[1024];if(!ini_path(p))return;WritePrivateProfileStringW(L"DomesticAI",L"AiRotate",enabled?L"1":L"0",p);}
static void load_ini(){wchar_t p[1024];if(!ini_path(p))return;enabled=GetPrivateProfileIntW(L"DomesticAI",L"AiRotate",1,p)?1:0;}
// 24차(사용자 11-11 "명승투자 제한 온오프 넣어놔"): which 5 = AI 명승 투자 금 제한(AiScenicGold, SAN14_DomesticCE.ini [Automation] — 달마다 다시 읽는 칸과 같음). 자리를 못 걸었으면 -1(설정 창에서 회색)
extern "C" __declspec(dllexport) int SAN14_DomesticAIGet(int which){
 if(which==0)return enabled;if(which==1)return (int)c_calls;if(which==2)return (int)c_ai;if(which==3)return (int)c_rot;if(which==4)return (int)c_fix;
 if(which==5)return scenic_ok?(int)DaiScOn:-1;return -1;
}
extern "C" __declspec(dllexport) int SAN14_DomesticAISet(int which,int v){
 if(which==5){if(!scenic_ok)return 0;wchar_t p[1024];if(!r39::game_root(p,1024))return 0;r39::wcat(p,1024,L"SAN14_DomesticCE.ini");
  if(!WritePrivateProfileStringW(L"Automation",L"AiScenicGold",v?L"1":L"0",p))return 0;InterlockedExchange(&DaiScOn,v?1:0);return 1;}
 if(which!=0)return 0;InterlockedExchange(&enabled,v?1:0);save_ini();return 1;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,22,"SAN14_DomesticAI.dll"))return 0;core=api;game=api->resolved->image;
 load_ini();
 bool node_ours=at<U8*>(game,0x134f918)==game+0x7b0b0;   // 다른 AI 모드가 노드를 통째로 바꿨으면 우리 자리는 안 불림
 HookPlan plan(api);plan.call(0x7b17b,0xb2af0,(void*)&DomesticAiSelect);
 bool ok=plan.install("SAN14CE.DomesticAI");
 if(core->Log)core->Log(ok?(node_ours?"DomesticAI R42g: AI development focus follows the ruler (politics share + preferred resource); player areas untouched.":"DomesticAI R42g: hooked; another mod wraps the AI node, so the rule runs only when that mod calls the original (month lines show real calls)."):"DomesticAI R42g: hook NOT installed (bytes differ / conflict). Nothing changed.");
 { // AI 명령서 99장: 따로 묶음(실패해도 AI 내정 순환은 그대로)
  HookPlan p2(api);static const U8 eo[]={0x41,0x3b,0xc7,0x41,0x0f,0x4f,0xc7};DaiGame=game;DaiOrdRet=game+0x9fa1e;p2.jump(0x9fa17,eo,7,(void*)&DaiOrdBridge);
  orders_ok=p2.install("SAN14CE.DomesticAI.Orders");
  HookPlan p3(api);static const U8 es[]={0x41,0x3b,0xc6,0x7c,0x06};DaiScPass=game+0x3e9cb;DaiScFail=game+0x3e9d1;p3.jump(0x3e9c6,es,5,(void*)&DaiScenicBridge);
  scenic_ok=p3.install("SAN14CE.DomesticAI.Scenic");
  if(!scenic_ok)r39::fail_plan(22,"SAN14_DomesticAI.dll",p3,"AI scenic invest gold check exe+0x3e9c6 (AI node CSceneryInvestSetNode)",L"AI 명승 투자 금 조건(남는 금 5천 이상)이 동작 안 함 → AI 명승 투자는 게임 기본대로. 다른 기능은 그대로");
  DaiOrdSwitch();
  if(core->Log)core->Log(orders_ok?"DomesticAI R43: AI orders 99 installed at the game's own order refill (exe+0x9fa17); player force untouched.":"DomesticAI R43: AI orders 99 NOT installed (refill site bytes differ).");
  char line[300];line[0]=0;const size_t C=sizeof line;r39::scat(line,C,"R42D\t22\tSAN14_DomesticAI.dll\torders_start\t");r39::stamp(line,C);r39::scat(line,C,orders_ok?(DaiOrdOn?"\tOK\t1/1\tAiOrders=1":"\tOFF\t1/1\tAiOrders=0"):"\tFAIL\t0/1\trefill site bytes differ");r39::scat(line,C,scenic_ok?(DaiScOn?"\tAiScenicGold=1 floor=":"\tAiScenicGold=0 floor="):"\tAiScenicGold=FAIL floor=");r39::snum(line,C,(unsigned)DaiScFloor);r39::scat(line,C," mult=");r39::snum(line,C,(unsigned)DaiScMult);r39::append_line(line);
  if(!orders_ok)r39::fail_plan(22,"SAN14_DomesticAI.dll",p2,"AI orders refill site exe+0x9fa17",L"AI 명령서 99장 유지가 동작 안 함 → AI 명령서는 게임 기본대로. 다른 기능은 그대로"); }
 // 시작 줄은 R39 형식이 아닌 R42D 로 남김: 설치기는 22번 상태를 DomesticCE 기록(runtime.log "DomesticCE: ACTIVE")으로 판정하는데,
 // R39 22 줄이 생기면 그 판정을 건너뛰어 DomesticCE 쪽 고장이 가려짐 → 설치기 판정은 예전 그대로 두고 우리 줄은 따로.
 {char line[512];line[0]=0;const size_t C=sizeof line;r39::scat(line,C,"R42D\t22\tSAN14_DomesticAI.dll\tstart\t");r39::stamp(line,C);r39::scat(line,C,"\t");
  r39::scat(line,C,ok?(enabled?"OK":"OFF"):"FAIL");r39::scat(line,C,ok?"\t1/1\t":"\t0/1\t");r39::scat(line,C,enabled?"AiRotate=1":"AiRotate=0");
  r39::scat(line,C,"\t");r39::scat(line,C,ok?(node_ours?"-":"AI node wrapped by another mod (rule runs only if it calls the original; see month lines)"):"call site bytes differ");r39::append_line(line);}
 if(!ok)r39::fail_plan(22,"SAN14_DomesticAI.dll",plan,"AI domestic focus call site exe+0x7b17b -> node fn exe+0xb2af0",L"AI 내정 순환(군주 반영)이 동작 안 함 → AI 내정은 게임 기본대로. 다른 기능은 그대로");
 return ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
