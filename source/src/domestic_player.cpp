// 22번 플레이어 내정 자동 — R42h (2026-11-04) 새 모듈 SAN14_DomesticPlayer.dll
//  R42i3(11-05): 데모·타이틀 화면처럼 플레이어가 없을 때(세력 0)·게임을 안 불러왔을 때(연도 100 미만)는 아무것도 안 함.
//      주인 없는 지역(세력 0)·값이 전부 0인 지역(상·농·병·인구)은 건너뜀. (데모 모니터링에서 불러오기 전부터 쓰던 것 발견)
//  왜: 소스 없는 옛 DomesticCE 가 자기 '게임 상태 찾기'([[game+0x27d4140]+0x1e0] 체인·세력 ≤51 전제)에서 매 틱 실패해
//      플레이어 내정 자동이 어느 시점부터 안 돎(맵 확장 큰 세이브에서 깨짐). DomesticCE 를 못 고치니(바이너리) 우리 소스로 같은 일을 직접 함.
//  어떻게: 3초마다 지역표를 직접 읽어, 플레이어 세력 지역의 중점 개발을 '골고루'(가장 낮은 단계부터)로 맞추고, 인구 하한을 지킴.
//          게임 함수는 안 부르고(스레드 안전) 메모리만 읽고 씀. DomesticAI 가 이미 잘 읽는 그 자리들만 씀 → DomesticCE 가 실패하는 체인에 안 의존.
//  멱등: '가장 낮은 단계'는 늘 같은 결과라, DomesticCE 가 나중에 되살아나 같이 돌아도 같은 값 → 충돌 없음. 바뀔 때만 씀.
//  자료(DomesticAI 와 같은, 게임 코드로 확인된 자리):
//     db=*(exe+0x2780f60) · world=*(db+0x871e8) · 연 world+0x34(i16) 월 +0x36(u8) · 플레이어 세력번호 world+0x3a(u8)
//     지역표 db+0x6caf0 = 포인터 표(칸 i = *(표+i*8), 1..500) ← 연속 배열 가정 금지. 맵 확장에서 표가 연속이 아닐 수 있어 반드시 포인터로 읽음(DomesticCE 가 여기서 깨짐)
//     지역+0x36 담당관 id · +0x3c/+0x3e/+0x40 상업·농업·병영(u16, 개발값)
//     지역 소유 세력번호 +0x78(u8) · 중점 +0x72(u8: 1=상 2=농 3=병 — R43 바로잡음) · 인구 +0x52(u16 — 11차 P213 바로잡음: 게임·옛 DomesticCE 모두 16비트로 읽고 씀. +0x54 는 다른 16비트 칸이라 32비트로 쓰면 덮어씀) ← 첫 틱에 기록으로 자리 확인
//  R43 11-08(갤 제보 '상업 지정해도 다음 턴 농업'): ① 중점 번호가 거꾸로였음. 게임 판정 함수 0xb2af0 확인 = 1 상업 · 2 농업 · 3 병영
//      (방침3→+0x3c 상업→1, 방침2→+0x3e 농업→2, 방침4→+0x40 병영→3; DomesticAI 규칙과 같음). 옛 판은 1=농 2=상이라 '상업'으로 쓴 2 가 농업이 됐음.
//    ② 플레이어 선택 존중: 3초마다 덮어쓰지 않음. 지역마다 '처음 볼 때'(중점 없음·만렙 항목일 때만 씀)와 '골고루 답이 바뀔 때'(개발 단계 변화)만 씀.
//      우리가 마지막에 쓴 값과 지금 값이 다르면 플레이어(또는 게임)가 바꾼 것 → 그 지역은 이번 실행 동안 손 안 댐(단 그 항목이 만렙이면 다음 것으로).
//      세이브 불러오기(연월이 뒤로·플레이어 세력 바뀜·한 틱에 4곳 넘게 남이 바꿈) → 기억 지움(불러온 값을 그대로 출발점으로).
//  R43 11-08 추가 제보('턴마다 상·농·군 돌아감, 다 안 키우고 넘김'): 옛 DomesticCE(바이너리)도 사용자 PC 에서 살아 있어(ACTIVE, changes=N)
//      같은 지역 중점을 자기 규칙으로 또 씀 → 두 모듈이 서로 덮어씀. R42h 의 '같은 값이라 충돌 없음' 가정이 틀렸음(번호도 달랐음).
//      → 플레이어 중점·최소 인구는 이 모듈만 맡음: DomesticCE 의 모든 내정(칸1)·최소 인구(칸2)를 시작 때 ini 로 끄고, 틱마다 메모리 스위치가 켜져 있으면 끔.
//         (DomesticCE 가 알려진 판(R43 해시)일 때만. AI 명령서 99장(칸3)·저돌은 그대로 DomesticCE 가 함)
//  설정: SAN14Mods\modules\DomesticPlayer.ini [DomesticPlayer] AllDomestic=1 PopulationFloor=1
//  기록: 시작 줄(R42D 22 DomesticPlayer start) · 첫 틱 점검(R42D 22 audit) · 달 바뀔 때 요약(R42D 22 pmonth)
#include "common.h"
#include "r39_record.h"
#include "domestic_ai_rule.h"   // dai::level
using U64=uint64_t;
static const S14_CoreAPI*core;static U8*game;
static volatile long en_dom=1,en_pop=1;
static volatile long started,audited;
static volatile long t_ticks,t_focus,t_pop,t_areas,t_idle,idle_logged;
static void forget_all();
static long cur_ym=-1;static volatile long log_lock;
static U8*db(){return *(U8**)(game+0x2780f60);}
static U8*world(){U8*d=db();return d?*(U8**)(d+0x871e8):nullptr;}
static bool rd(const void*p,size_t n){return p&&core->Readable((void*)p,n);}
// 상업(+0x3c,코드1)·농업(+0x3e,코드2)·병영(+0x40,코드3) 중 단계 가장 낮은 것(≥1000 제외, 같으면 상>농>병). 없으면 0.
//  번호는 게임 판정 함수 0xb2af0 이 쓰는 그대로(1 상업 · 2 농업 · 3 병영).
enum{F_COM=1,F_AGRI=2,F_BAR=3};
static unsigned focus_code(unsigned com,unsigned agri,unsigned bar){
 struct{unsigned v,code;}s[3]={{com,F_COM},{agri,F_AGRI},{bar,F_BAR}};unsigned best=0,bl=99;
 for(int i=0;i<3;i++){if(s[i].v>=1000u)continue;unsigned l=dai::level(s[i].v);if(l<bl){bl=l;best=s[i].code;}}
 return best;
}
// 지금 중점 항목이 쓸모없나(중점 없음/모르는 값/그 항목 만렙)
static bool focus_dead(unsigned cur,unsigned com,unsigned agri,unsigned bar){
 if(cur==F_COM)return com>=1000u;if(cur==F_AGRI)return agri>=1000u;if(cur==F_BAR)return bar>=1000u;return true;
}
// 지역별 기억(이번 실행 동안). 지역 포인터가 바뀌면 그 칸은 새로 봄.
struct Seen{U8*area;U8 owner;U8 want;U8 wrote;U8 keep;};
static Seen seen[501];static U8*seen_world;static unsigned seen_player;static long seen_ym=-1;
static volatile long t_kept,t_ce_off;
static void forget_all(){memset(seen,0,sizeof seen);}
// 지역 i 의 객체 포인터 (표가 연속이 아닐 수 있어 반드시 포인터로)
static U8*area_at(U8*tab,unsigned i){if(!rd(tab+(size_t)i*8,8))return nullptr;U8*a=*(U8**)(tab+(size_t)i*8);return rd(a,0x98)?a:nullptr;}
static void audit(U8*tab,U8*world_,unsigned player){
 if(!r39::s14_diag())return;
 char m[1000];m[0]=0;const size_t C=sizeof m;
 r39::scat(m,C,"R42D\t22\tSAN14_DomesticPlayer.dll\taudit\tplayer=");r39::snum(m,C,player);r39::scat(m,C,"\tareas=");
 unsigned shown=0;
 for(unsigned i=1;i<=500&&shown<8;i++){
  U8*a=area_at(tab,i);if(!a)continue;
  if(at<U8>(a,0x78)!=player)continue;
  unsigned com=at<U16>(a,0x3c),ag=at<U16>(a,0x3e),bar=at<U16>(a,0x40),foc=at<U8>(a,0x72);int pop=at<U16>(a,0x52);
  r39::scat(m,C,"#");r39::snum(m,C,i);r39::scat(m,C,"(pop=");r39::snum(m,C,(unsigned)(pop<0?0:pop));
  r39::scat(m,C," 상");r39::snum(m,C,com);r39::scat(m,C,"농");r39::snum(m,C,ag);r39::scat(m,C,"병");r39::snum(m,C,bar);
  r39::scat(m,C," 중점");r39::snum(m,C,foc);r39::scat(m,C,"→");r39::snum(m,C,focus_code(com,ag,bar));r39::scat(m,C,") ");shown++;
 }
 if(!shown)r39::scat(m,C,"(플레이어 지역 없음 — 아직 게임 안 불러옴)");
 r39::scat(m,C,"\t");r39::stamp(m,C);r39::append_line(m);
}
// P206(2026-11-09 사용자 승인: 턴 종료 기능이 실제로 도는지 기록으로 확인) — 달이 바뀔 때 한 번, 읽기만.
//  이 한 줄로 판단하는 것: 22번 '최소 인구 1만'(under10k 가 0 인가) · 22번 'AI 명령서 99장'(ai_cmd99 = ai_corps 인가 — 군단 수) ·
//   28번 '인구 증가 보너스'(pop_all 이 1·4·7·10월에 다른 달보다 더 느는가 — 앞뒤 줄의 차이로 봄).
//  자리: 지역 인구 +0x52·소유 세력 +0x78(위와 같음), 군단 객체 = db+0xe0d0+8*번호(CDistrictData, vtable exe+0x13a98f0), 명령서 = 군단+0x14(BYTE, 16차에 바로잡음 —
//   통합편의가 플레이어 명령서를 읽는 자리와 같음: SAN14CE_11Patch.log '명령서 N'). 게임 값은 안 바꾼다.
static void world_line(U8*d,U8*tab,unsigned player,int year,int month){
 if(!r39::s14_diag())return;
 unsigned pop_all=0,pop_me=0,under=0,under_me=0,owned=0;bool has[0x34]={};
 for(unsigned i=1;i<=500;i++){U8*a=area_at(tab,i);if(!a)continue;unsigned o=at<U8>(a,0x78);if(!o||o>0x33)continue;
  unsigned com=at<U16>(a,0x3c),ag=at<U16>(a,0x3e),bar=at<U16>(a,0x40);int pop=at<U16>(a,0x52);
  if(com>1000||ag>1000||bar>1000||pop<=0)continue;   // 게임 상태가 아닌 칸
  owned++;has[o]=true;pop_all+=(unsigned)pop;if(pop<10000)under++;if(o==player){pop_me+=(unsigned)pop;if(pop<10000)under_me++;}}
 unsigned nai=0,n99=0,mn=255,mine=0,mx=0,sum=0;
 // 16차(P220): 명령서는 군단(CDistrictData, db+0xe0d0+8*번호, vtable exe+0x13a98f0)+0x14. 군단+0x10 = 세력 번호. 15차까지는 세력+0x14(다른 값)를 읽었음.
 bool got_mine=false;
 if(rd(d+0xe0d0,8*0x34))for(unsigned k=1;k<=0x33;k++){U8*di=*(U8**)(d+0xe0d0+8*(size_t)k);
  if(!rd(di,0x18)||at<U8*>(di,0)!=game+0x13a98f0)continue;unsigned f=at<U8>(di,0x10);if(!f||f>0x33||!has[f])continue;unsigned c=at<U8>(di,0x14);
  if(f==player){if(!got_mine){mine=c;got_mine=true;}continue;}nai++;if(c>=99)n99++;if(c<mn)mn=c;if(c>mx)mx=c;sum+=c;}
 char m[320];m[0]=0;const size_t C=sizeof m;
 r39::scat(m,C,"R42D\t22\tSAN14_DomesticPlayer.dll\tworld\t");r39::snum(m,C,(unsigned)year);r39::scat(m,C,"/");r39::snum(m,C,(unsigned)month);
 r39::scat(m,C,"\tareas=");r39::snum(m,C,owned);r39::scat(m,C,"\tpop_all=");r39::snum(m,C,pop_all);r39::scat(m,C,"\tpop_player=");r39::snum(m,C,pop_me);
 r39::scat(m,C,"\tunder10k=");r39::snum(m,C,under);r39::scat(m,C,"\tunder10k_player=");r39::snum(m,C,under_me);
 r39::scat(m,C,"\tai_corps=");r39::snum(m,C,nai);r39::scat(m,C,"\tai_cmd99=");r39::snum(m,C,n99);r39::scat(m,C,"\tai_cmd_min=");r39::snum(m,C,nai?mn:0);r39::scat(m,C,"\tai_cmd_max=");r39::snum(m,C,mx);r39::scat(m,C,"\tai_cmd_avg=");r39::snum(m,C,nai?sum/nai:0);r39::scat(m,C,"\tplayer=");r39::snum(m,C,player);r39::scat(m,C,"\tcmd_player=");r39::snum(m,C,mine);
 r39::scat(m,C,"\t");r39::stamp(m,C);r39::append_line(m);
}
// 11차: 인물 집계 한 줄(달마다, 읽기만) — 26번(처단불가·재야 탈출·AI 탐색)의 결과를 숫자로 본다.
//  무장 표 db+0x158+8*번호(≤0x1770), 신분 = 인물+0xe6(1~4 소속 무장, 5 재야, 6 포로, 그 밖 = 미등장·사망), 소속 세력 = 인물+0xe0.
//  보는 법: captive 가 줄 때 gone 이 같이 늘면 처단/사망, officers 가 늘면 등용·석방. ronin 이 꾸준히 줄면 AI 탐색·등용이 도는 것.
static void census_line(U8*d,unsigned player,int year,int month){
 if(!r39::s14_diag())return;
 if(!rd(d+0x158,8*0x1771))return;unsigned off=0,mine=0,ron=0,cap=0,cap_mine=0,gone=0;
 for(unsigned i=1;i<=0x1770;i++){U8*q=*(U8**)(d+0x158+8*(size_t)i);if(!q||!rd(q,0xe8))continue;unsigned st=at<U8>(q,0xe6),f=at<U8>(q,0xe0);
  if(st>=1&&st<=4){off++;if(player&&f==player)mine++;}else if(st==5)ron++;else if(st==6){cap++;if(player&&f==player)cap_mine++;}else gone++;}
 char m[260];m[0]=0;const size_t C=sizeof m;
 r39::scat(m,C,"R42D\t26\tSAN14_DomesticPlayer.dll\tcensus\t");r39::snum(m,C,(unsigned)year);r39::scat(m,C,"/");r39::snum(m,C,(unsigned)month);
 r39::scat(m,C,"\tofficers=");r39::snum(m,C,off);r39::scat(m,C,"\tofficers_player=");r39::snum(m,C,mine);r39::scat(m,C,"\tronin=");r39::snum(m,C,ron);
 r39::scat(m,C,"\tcaptive=");r39::snum(m,C,cap);r39::scat(m,C,"\tcaptive_of_player_force=");r39::snum(m,C,cap_mine);r39::scat(m,C,"\tgone=");r39::snum(m,C,gone);
 r39::scat(m,C,"\t");r39::stamp(m,C);r39::append_line(m);
}
// 달이 바뀐 것을 3초 틱에서 알아채 한 줄. 22번 설정(모든 내정·인구 하한)을 꺼 둬도 기록은 남는다(읽기만이므로).
static long world_ym=-1;
static void world_tick(){
 if(!r39::s14_diag())return;   // 20차: 달 기록(world·census)은 확인판에서만
 U8*d=db();U8*w=world();if(!d||!w||!rd(d,0x6caf0+8)||!rd(w,0x40))return;
 U8*tab=d+0x6caf0;if(!rd(tab,8*501))return;
 unsigned player=at<U8>(w,0x3a);int year=at<short>(w,0x34),month=at<U8>(w,0x36);
 if(month<1||month>12||year<100)return;                  // 타이틀·불러오기 전. 데모(플레이어 0)에서도 남긴다 — AI 쪽 기능 확인용(11차)
 long ym=(long)year*12+(month-1);if(ym==world_ym)return;
 if(InterlockedExchange(&log_lock,1))return;             // 다른 기록 중이면 다음 틱에
 world_ym=ym;world_line(d,tab,player,year,month);census_line(d,player,year,month);InterlockedExchange(&log_lock,0);
}
static void month_line(int year,int month){
 if(!r39::s14_diag())return;
 char m[300];m[0]=0;const size_t C=sizeof m;
 r39::scat(m,C,"R42D\t22\tSAN14_DomesticPlayer.dll\tpmonth\t");r39::snum(m,C,(unsigned)year);r39::scat(m,C,"/");r39::snum(m,C,(unsigned)month);
 r39::scat(m,C,"\tticks=");r39::snum(m,C,(unsigned)t_ticks);r39::scat(m,C,"\tareas=");r39::snum(m,C,(unsigned)t_areas);
 r39::scat(m,C,"\tfocus_set=");r39::snum(m,C,(unsigned)t_focus);r39::scat(m,C,"\tpop_set=");r39::snum(m,C,(unsigned)t_pop);r39::scat(m,C,"\tplayer_kept=");r39::snum(m,C,(unsigned)t_kept);r39::scat(m,C,"\tce_off=");r39::snum(m,C,(unsigned)t_ce_off);
 r39::scat(m,C,"\t");r39::stamp(m,C);r39::append_line(m);
 t_ticks=t_areas=t_focus=t_pop=0;
}
// 3초 틱 — 게임 함수 안 부름, 메모리만
static void tick(){
 if(!en_dom&&!en_pop)return;
 U8*d=db();U8*w=world();if(!d||!w||!rd(d,0x6caf0+8)||!rd(w,0x40))return;
 U8*tab=d+0x6caf0;if(!rd(tab,8*501))return;              // 지역 포인터 표
 unsigned player=at<U8>(w,0x3a);
 int year=at<short>(w,0x34),month=at<U8>(w,0x36);if(month<1||month>12)return;
 if(player==0||year<100){   // 플레이어 없음(데모·타이틀) 또는 아직 게임 안 불러옴 → 손 안 댐
  InterlockedIncrement(&t_idle);
  if(r39::s14_diag()&&!InterlockedExchange(&idle_logged,1)){char m[200];m[0]=0;const size_t C=sizeof m;r39::scat(m,C,"R42D\t22\tSAN14_DomesticPlayer.dll\tidle\tplayer=");r39::snum(m,C,player);
   r39::scat(m,C,"\tyear=");r39::snum(m,C,(unsigned)(year<0?0:year));r39::scat(m,C,"\tno player or game not loaded -> nothing written\t");r39::stamp(m,C);r39::append_line(m);}
  return;
 }
 InterlockedExchange(&idle_logged,0);
 if(!InterlockedExchange(&audited,1))audit(tab,w,player);
 InterlockedIncrement(&t_ticks);
 unsigned areas=0,fset=0,pset=0,kept=0;
 { long ym=(long)year*12+(month-1);   // 세이브 불러오기 흔적 → 기억 지움
   if(w!=seen_world||player!=seen_player||(seen_ym>=0&&ym<seen_ym)){forget_all();seen_world=w;seen_player=player;}
   seen_ym=ym; }
 // 1차: 남이 바꾼 지역 수(우리가 쓴 값과 다름) — 4곳 넘으면 불러오기로 보고 기억 지움
 { unsigned foreign=0;
   for(unsigned i=1;i<=500;i++){Seen&q=seen[i];if(!q.area||!q.wrote||q.keep)continue;U8*a=area_at(tab,i);if(a!=q.area)continue;if(at<U8>(a,0x72)!=q.wrote)foreign++;}
   if(foreign>4)forget_all(); }
 for(unsigned i=1;i<=500;i++){
  U8*a=area_at(tab,i);if(!a){seen[i].area=nullptr;continue;}
  if(at<U8>(a,0x78)!=player){seen[i].area=nullptr;continue;}   // 플레이어 소유 지역만(남의 지역이 되면 기억 지움 → 다시 얻으면 처음부터)
  unsigned com=at<U16>(a,0x3c),ag=at<U16>(a,0x3e),bar=at<U16>(a,0x40);
  if(com>1000||ag>1000||bar>1000)continue;            // 개발값이 이상하면(게임 상태 아님) 그 지역만 건너뜀
  if(!com&&!ag&&!bar&&at<U16>(a,0x52)==0)continue;     // 값이 전부 0 = 빈 칸(실제 지역 아님)
  areas++;
  if(en_dom){
   Seen&q=seen[i];unsigned want=focus_code(com,ag,bar);unsigned cur=at<U8>(a,0x72);bool write=false;
   if(q.area!=a){q=Seen{a,(U8)player,(U8)want,0,0};write=want&&focus_dead(cur,com,ag,bar);}          // 처음 봄: 중점이 쓸모없을 때만
   else{
    if(q.wrote&&cur!=q.wrote)q.keep=1;                 // 우리가 쓴 값이 아님 = 플레이어가 고름 → 손 안 댐
    if(!q.keep&&want!=q.want)write=want!=0;           // 개발 단계가 바뀌어 골고루 답이 바뀜(플레이어가 안 고른 지역만)
    q.want=(U8)want;
   }
   if(!write&&want&&focus_dead(cur,com,ag,bar))write=true;   // 중점 없음·고른 항목 만렙이면 언제든 다음 것으로
   if(!write&&q.keep)kept++;
   if(write&&cur!=want){at<U8>(a,0x72)=(U8)want;fset++;}
   if(write){q.wrote=(U8)want;q.keep=0;}
  }
  if(en_pop){unsigned pop=at<U16>(a,0x52);if(pop>=1&&pop<10000){at<U16>(a,0x52)=10000;pset++;}}   // 옛 DomesticCE 와 같은 규칙: 1~9999 → 10000 (0 은 그대로), 16비트만 씀  // 인구 하한 1만
 }
 t_kept=kept;
 t_areas=areas;for(unsigned q=0;q<fset;q++)InterlockedIncrement(&t_focus);for(unsigned q=0;q<pset;q++)InterlockedIncrement(&t_pop);
 long ym=(long)year*12+(month-1);
 if(ym!=cur_ym){if(!InterlockedExchange(&log_lock,1)){long prev=cur_ym;cur_ym=ym;if(prev>=0)month_line((int)(prev/12),(int)(prev%12+1));InterlockedExchange(&log_lock,0);}}
}
// 옛 DomesticCE 의 플레이어 내정(칸1 모든 내정·칸2 최소 인구) 끄기 — 스위치 자리 CE+0x2d580+4*칸(설정 창이 쓰는 그 자리)
static int ce_known=-1;   // DomesticCE 판 확인 결과(-1 아직, 1 알려진 판, 0 모르는 판 → 손 안 댐)
static void ce_quiet(){
 HMODULE m=GetModuleHandleW(L"SAN14_DomesticCE.dll");if(!m)return;
 int&ok=ce_known;if(ok<0){static const U8 sha[]={0xca,0xab,0x6a,0x7c,0xaf,0xc2,0x05,0x9e,0x72,0xb8,0x9b,0x86,0x7c,0x15,0x77,0xbc,0xeb,0x2f,0xc8,0x29,0xf8,0xb9,0x4d,0xc6,0xaf,0x77,0x40,0x79,0x01,0x06,0x64,0x24};ok=module_sha(m,sha)?1:0;}
 if(!ok)return;
 for(unsigned slot=1;slot<=2;slot++){volatile U32*f=(volatile U32*)((U8*)m+0x2d580+4*slot);if(rd((const void*)f,4)&&*f){*f=0;InterlockedIncrement(&t_ce_off);}}   // 4바이트 맞춤 쓰기(원자적)
}
static void ce_ini_off(){wchar_t p[1024];if(!r39::game_root(p,1024))return;r39::wcat(p,1024,L"SAN14_DomesticCE.ini");if(GetFileAttributesW(p)==(DWORD)0xffffffffu)return;
 WritePrivateProfileStringW(L"Automation",L"AllDomestic",L"0",p);WritePrivateProfileStringW(L"Automation",L"PopulationFloor",L"0",p);}
static DWORD WINAPI worker(void*){for(;;){ce_quiet();tick();world_tick();Sleep(3000);}}
// ---- 설정
static bool ini_path(wchar_t*p){if(!r39::game_root(p,1024))return false;r39::wcat(p,1024,L"SAN14Mods\\modules\\DomesticPlayer.ini");return true;}
static void load_ini(){wchar_t p[1024];if(!ini_path(p))return;en_dom=GetPrivateProfileIntW(L"DomesticPlayer",L"AllDomestic",1,p)?1:0;en_pop=GetPrivateProfileIntW(L"DomesticPlayer",L"PopulationFloor",1,p)?1:0;}
static void save_ini(){wchar_t p[1024];if(!ini_path(p))return;WritePrivateProfileStringW(L"DomesticPlayer",L"AllDomestic",en_dom?L"1":L"0",p);WritePrivateProfileStringW(L"DomesticPlayer",L"PopulationFloor",en_pop?L"1":L"0",p);}
// which: 0 모든 내정(AllDomestic), 2 최소 인구(PopulationFloor) — ModSettings S_DOM arg 와 같은 번호
extern "C" __declspec(dllexport) int SAN14_DomesticPlayerGet(int which){
 if(which==0)return en_dom;if(which==2)return en_pop;if(which==10)return (int)t_focus;if(which==12)return (int)t_kept;if(which==11)return (int)t_pop;return -1;
}
extern "C" __declspec(dllexport) int SAN14_DomesticPlayerSet(int which,int v){
 if(which==0)InterlockedExchange(&en_dom,v?1:0);else if(which==2)InterlockedExchange(&en_pop,v?1:0);else return 0;save_ini();return 1;
}
extern "C" __declspec(dllexport) int SAN14_DomesticPlayerState(){return (en_dom?1:0)|(en_pop?2:0)|(started?4:0);}
// 시험용: 순수 규칙(게임 없이)
extern "C" __declspec(dllexport) unsigned SAN14_DomesticPlayerFocus(unsigned com,unsigned agri,unsigned bar){return focus_code(com,agri,bar);}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,22,"SAN14_DomesticPlayer.dll"))return 0;core=api;game=api->resolved->image;
 load_ini();ce_ini_off();ce_quiet();
 HANDLE h=CreateThread(nullptr,0,worker,nullptr,0,nullptr);if(h){CloseHandle(h);started=1;}
 if(core->Log)core->Log(started?"DomesticPlayer R43: player-force domestic auto (balanced focus 1=com 2=agri 3=bar, keeps player's choice + population floor) via direct area table; game functions not called.":"DomesticPlayer R42h: worker thread NOT started.");
 {char line[512];line[0]=0;const size_t C=sizeof line;r39::scat(line,C,"R42D\t22\tSAN14_DomesticPlayer.dll\tstart\t");r39::stamp(line,C);r39::scat(line,C,"\t");
  r39::scat(line,C,started?"OK":"FAIL");r39::scat(line,C,"\t1/1\t");r39::scat(line,C,en_dom?(en_pop?"AllDomestic=1 PopFloor=1":"AllDomestic=1 PopFloor=0"):(en_pop?"AllDomestic=0 PopFloor=1":"off"));
  r39::scat(line,C,"\t");r39::scat(line,C,started?"-":"CreateThread failed");r39::append_line(line);}
 if(!started){char ac[48];ac[0]=0;r39::scat(ac,sizeof ac,"CreateThread=NULL GetLastError=");r39::snum(ac,sizeof ac,(unsigned)GetLastError());
  r39::fail(22,"SAN14_DomesticPlayer.dll","thread","SAN14_ModInitV1 -> CreateThread(worker)","worker thread handle",ac,L"플레이어 내정 자동(22번 플레이어 쪽)이 이번 실행에서 동작 안 함. AI 내정·다른 기능은 그대로");}
 return started?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
