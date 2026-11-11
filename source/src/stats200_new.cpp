// R43 17번 능력치 상한 200 — 새 게임 판(2026-11-05, S14_GAME_STAMP) 전용으로 소스에서 새로 짠 모듈.
//  (옛 이식본 SAN14_Stats200.dll/SAN14_Stats200Reader.dll 을 대신한다. 바이너리 이식이 아니라 자리마다 기대 바이트를 적어 두고 대조한다.)
//  자리(15곳) — 옛 1.0.0 R6 명세(build_tc/src/stats200_direct.cpp)의 14+1 자리를 새 exe 에서 앞뒤 문맥으로 다시 찾음(upd_tools 기록: STATUS_R43.md 2-7):
//   묶음 A "SAN14CE.Stats200New.Caps" 14곳: 편집기·실행 중 상한 100→200 상수(바이트 교체, 점프 없음)
//     0x226e2a 0x226e7f 0x22ae26 0x22ae99 0x23c458 0x23c4b6 : movzx edx,cl → mov dl,0xc8 (인물 편집 입력 상한)
//     0x20b7c5 0x20bfda 0x211f0d                           : mov edx,0x64 → 0xc8 (능력 읽기 상한; 0x211f0d 는 편집 인물 파일 읽기)
//     0x5dfd18 0x5dfb83                                     : mov [rbp+..],0x64 → 0xc8 (화면 표시 상한)
//     0x142fc70 (.data)                                     : 0x64 ×4 → 0xc8 ×4 (상한 표)
//     0x321f3e                                              : cmp ecx,0x64; cmova ecx,r14d → cmp cl,0xc8; jbe +2; mov cl,0xc8
//     0x2bc14a                                              : cmp r13d,0x64; jne → cmp r13b,0xc8; jb (경험치 성장: 능력 200 미만이면 계속. 원래는 딱 100 일 때만 멈춤)
//       (11-10 까지는 0x2bc14e jne→jb 2바이트였는데 그건 '100 이상이면 멈춤'이 돼 100 넘는 성장을 오히려 막았다 — P207)
//   묶음 B "SAN14CE.Stats200New.XP" 1곳: 경험치 성장 분기 0x231afb
//     원래: cmp ecx,0x7b ; je 0x231b42(mov esi,0x64) ; nop3 ; 0x231b00 계속
//     옛 판은 je 뒤 nop 이 6바이트라 8바이트 상수 패치가 들어갔지만 새 판은 nop 3바이트(총 5)뿐 → 5바이트 점프로 우리 릴레이(XpBridge)로 보내고
//     릴레이에서 같은 판정(ecx==0x7b 이고 esi<100 일 때만 esi=100 경로, 아니면 그대로 계속)을 한 뒤 0x231b00/0x231b42 로 돌아간다. 뒤 명령은 안 건드림.
//   묶음 C "SAN14CE.Stats200New.Grow" 1곳(2026-11-10, P207·교훈 L018): 경험치 성장 함수(0x2bc0d0)의 레벨업 구간 0x2bc30a~0x2bc3c7 을 우리 함수로.
//     게임 구간: 다음 레벨 L=능력+1 의 필요 경험치(경험치 표 0x337c40 의 L행, 0x2442f0)가 쌓인 경험치 이하면 능력+1 을 되풀이, 상한 100(mov ebp,0x64).
//       표는 100행뿐이라 L>=101 이면 0행을 봄(cmp ebx,0x65; jae) → 100 넘어서는 못 큼.
//     우리 함수(Stats200Grow): L<=100 은 게임과 똑같이, L=101~200 은 옛 모음팩 v1.9 SAN14_Stats200.dll 의 식 —
//       필요 경험치 = t100 + (L-100)×(t100−t99)  (t99·t100 = 표 99행·100행의 그 능력 필요 경험치. v1.9 intercepted_game_xp 디스어셈블로 확인)
//       v1.9 의 안전 조건도 그대로: t99=0 / t100<=t99 / t100>=0xf0000000 / 간격>10,000,000 / 32비트 넘침이면 안 큼.
//       (18번 '억제' 표는 값이 1000배라 간격이 10,000,000 을 넘어 100 위로 안 큼 — v1.9 와 같은 동작)
//     끝나면 게임 0x2bc3c8(능력이 바뀌었으면 알림 0x25a360)로 돌아감. 구간이 쓰던 r14 복구(0x2bc313)는 다리에서 함.
//     옛 v1.9 는 함수 전체를 가로채 한 번에 1씩 올렸지만, 여기선 게임 구간과 같은 되풀이(경험치가 충분하면 여러 번)로 맞춤.
//  실패 처리: 자리 하나라도 기대 바이트와 다르면 그 묶음은 손대지 않고(전부 아니면 전무) 창 + R43F 줄(어느 자리·기대·실제). 다른 묶음은 계속.
//  ModCore 가 등록 용량으로 거절하면(A 묶음은 상수뿐이라) 직접 쓴다(옛 R6 와 같은 규칙).
#include "common.h"
#include "r39_record.h"
static const S14_CoreAPI*core;static U8*game;
struct Site{U32 rva;U32 len;U8 expected[16];U8 replacement[16];};
static const Site kCaps[14]={
 {0x226e2a,3,{0x0f,0xb6,0xd1},{0xb2,0xc8,0x90}},
 {0x226e7f,3,{0x0f,0xb6,0xd1},{0xb2,0xc8,0x90}},
 {0x22ae26,3,{0x0f,0xb6,0xd1},{0xb2,0xc8,0x90}},
 {0x22ae99,3,{0x0f,0xb6,0xd1},{0xb2,0xc8,0x90}},
 {0x23c458,3,{0x0f,0xb6,0xd1},{0xb2,0xc8,0x90}},
 {0x23c4b6,3,{0x0f,0xb6,0xd0},{0xb2,0xc8,0x90}},
 {0x20b7c5,5,{0xba,0x64,0x00,0x00,0x00},{0xba,0xc8,0x00,0x00,0x00}},
 {0x20bfda,5,{0xba,0x64,0x00,0x00,0x00},{0xba,0xc8,0x00,0x00,0x00}},
 {0x211f0d,5,{0xba,0x64,0x00,0x00,0x00},{0xba,0xc8,0x00,0x00,0x00}},
 {0x5dfd18,7,{0xc7,0x45,0xe0,0x64,0x00,0x00,0x00},{0xc7,0x45,0xe0,0xc8,0x00,0x00,0x00}},
 {0x5dfb83,7,{0xc7,0x45,0x18,0x64,0x00,0x00,0x00},{0xc7,0x45,0x18,0xc8,0x00,0x00,0x00}},
 {0x142fc70,16,{0x64,0,0,0,0x64,0,0,0,0x64,0,0,0,0x64,0,0,0},{0xc8,0,0,0,0xc8,0,0,0,0xc8,0,0,0,0xc8,0,0,0}},
 {0x321f3e,7,{0x83,0xf9,0x64,0x41,0x0f,0x47,0xce},{0x80,0xf9,0xc8,0x76,0x02,0xb1,0xc8}},
 {0x2bc14a,6,{0x41,0x83,0xfd,0x64,0x75,0x07},{0x41,0x80,0xfd,0xc8,0x72,0x07}},
};
static const U32 kXpSite=0x231afb,kXpCont=0x231b00,kXpCap=0x231b42;
static const U8 kXpExpected[5]={0x74,0x45,0x0f,0x1f,0x00};   // je 0x231b42 ; nop DWORD [rax]
extern "C"{void*XpCont,*XpCap;void XpBridge();extern volatile long XpHitsRaw;}
// ---- 묶음 C: 100 넘는 경험치 성장
#ifndef S14_GAMECALL
#define S14_GAMECALL          // 게임 함수 호출 규약(윈도 x64 기본). 리눅스 시험 빌드에서는 __attribute__((ms_abi)) 로 정의
#endif
static const U32 kGrowSite=0x2bc30a,kGrowRet=0x2bc3c8,kXpTable=0x337c40,kRowValid=0x331750,kRowNeed=0x2442f0;
static const U8 kGrowExpected[9]={0x41,0x8d,0x5d,0x01,0xe8,0x2d,0xb9,0x07,0x00};   // lea ebx,[r13+1] ; call 0x337c40
extern "C"{void*GrowRet;void GrowBridge();}
static unsigned grow_cap=200;                       // 시험에서 100 으로 두고 게임 구간과 결과 비교
static volatile LONG grow_over100;                  // 자가진단: 100 넘어 오른 횟수
static bool need_for(unsigned L,unsigned idx,U32*need){
 using Tab=U8*(S14_GAMECALL*)();using Valid=int(S14_GAMECALL*)(void*);using Need=U32(S14_GAMECALL*)(void*,int);
 U8*tab=reinterpret_cast<Tab>(game+kXpTable)();if(!tab)return false;
 Valid valid=reinterpret_cast<Valid>(game+kRowValid);Need req=reinterpret_cast<Need>(game+kRowNeed);
 if(L<=100){void*row=at<void*>(tab,0x60+8*L);if(!valid(row))return false;*need=req(row,(int)idx);return true;}
 void*r99=at<void*>(tab,0x60+8*99),*r100=at<void*>(tab,0x60+8*100);
 if(!r99||!r100||!valid(r99)||!valid(r100))return false;
 U32 t99=req(r99,(int)idx),t100=req(r100,(int)idx);
 if(!t99||t100<=t99||t100>=0xf0000000u)return false;
 U32 step=t100-t99;if(step>10000000u)return false;
 uint64_t n=(uint64_t)t100+(uint64_t)(L-100)*step;if(n>>32)return false;
 *need=(U32)n;return true;
}
extern "C" S14_GAMECALL void Stats200Grow(U8*person,unsigned idx){
 if(idx>4)return;
 U32 xp=at<U32>(person,0x104+4*idx);unsigned stat=at<U8>(person,0xec+idx),before=stat;
 while(stat<grow_cap){U32 need=0;if(!need_for(stat+1,idx,&need))break;if(need>xp)break;stat++;at<U8>(person,0xec+idx)=(U8)stat;}
 if(stat>100&&stat!=before){long k=0;for(unsigned i=(before>100?before:100);i<stat;i++)k=InterlockedIncrement(&grow_over100);   // 100 넘어 오른 횟수
  static volatile LONG logged;if(!InterlockedExchange(&logged,1)||k%200==0)r39::counter(17,"SAN14_Stats200New.dll","grow_over100",(unsigned)k);}
}
// ---- 묶음 A: 상수 14곳
static int caps_apply(U32*applied,U32*already,U32*bad_i){
 S14_Hook hooks[14];U32 n=0;*applied=*already=0;*bad_i=99;
 for(U32 i=0;i<14;i++){
  const Site&s=kCaps[i];U8*t=game+s.rva;
  if(!core->Readable(t,s.len)){*bad_i=i;return -1;}
  if(!memcmp(t,s.replacement,s.len)){(*already)++;continue;}
  if(memcmp(t,s.expected,s.len)){*bad_i=i;return -2;}
  S14_Hook&h=hooks[n++];h.target=t;h.length=s.len;memset(h.expected,0,16);memset(h.replacement,0,16);memcpy(h.expected,s.expected,s.len);memcpy(h.replacement,s.replacement,s.len);
 }
 if(!n)return 1;
 if(core->InstallHooks("SAN14CE.Stats200New.Caps",hooks,n)){*applied=n;return 1;}
 // ModCore 거절(등록 용량 등): 방금 전부 원본으로 확인했고 상수뿐이라 직접 쓴다
 for(U32 i=0;i<n;i++){
  DWORD old=0,un=0;if(!VirtualProtect(hooks[i].target,hooks[i].length,0x40,&old)){*bad_i=i;return -3;}
  memcpy(hooks[i].target,hooks[i].replacement,hooks[i].length);FlushInstructionCache(GetCurrentProcess(),hooks[i].target,hooks[i].length);VirtualProtect(hooks[i].target,hooks[i].length,old,&un);
 }
 *applied=n;return 2;
}
extern "C" __declspec(dllexport) long SAN14_Stats200NewXpHits(){return XpHitsRaw;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,17,"SAN14_Stats200New.dll"))return 0;core=api;game=api->resolved->image;
 U32 applied=0,already=0,bad=99;int a=caps_apply(&applied,&already,&bad);
 char msg[200];msg[0]=0;
 if(a>0){r39::scat(msg,sizeof msg,"Stats200New R43: caps 14 sites OK (applied ");r39::snum(msg,sizeof msg,applied);r39::scat(msg,sizeof msg,", already ");r39::snum(msg,sizeof msg,already);r39::scat(msg,sizeof msg,a==2?", written directly: ModCore registry refused).":").");}
 else{r39::scat(msg,sizeof msg,"Stats200New R43: caps group NOT applied (site #");r39::snum(msg,sizeof msg,bad);r39::scat(msg,sizeof msg,a==-1?" unreadable":a==-2?" bytes differ":" write failed");r39::scat(msg,sizeof msg,"). All 14 left native.");}
 if(core->Log)core->Log(msg);
 if(a<=0&&bad<14){const Site&s=kCaps[bad];U8 act[16]={};if(core->Readable(game+s.rva,s.len))memcpy(act,game+s.rva,s.len);
  r39::fail_bytes(17,"SAN14_Stats200New.dll",a==-1?"caps site unreadable":a==-2?"caps site bytes differ":"caps site write failed",s.rva,s.expected,act,s.len,L"능력치 상한 200(17번) 상수 14곳이 전부 적용 안 됨 → 상한 100 그대로. 다른 기능은 그대로");}
 // ---- 묶음 B: 경험치 성장 분기(릴레이)
 XpCont=game+kXpCont;XpCap=game+kXpCap;
 HookPlan plan(api);
 bool xp_already=core->Readable(game+kXpSite,5)&&game[kXpSite]==0xe9;   // 이미 어떤 점프가 있으면(두 번 초기화) 건드리지 않음
 bool b=false;
 if(xp_already){b=true;if(core->Log)core->Log("Stats200New R43: xp-growth site already hooked; left as is.");}
 else{
  plan.jump(kXpSite,kXpExpected,5,(void*)&XpBridge);
  b=plan.install("SAN14CE.Stats200New.XP");
  if(core->Log)core->Log(b?"Stats200New R43: xp-growth relay installed at exe+0x231afb (cap raise keeps esi>=100).":"Stats200New R43: xp-growth relay NOT installed; growth cap stays native 100.");
  if(!b)r39::fail_plan(17,"SAN14_Stats200New.dll",plan,"xp-growth branch",L"능력치 상한 200(17번) 중 경험치 성장 분기 1곳이 적용 안 됨 → 성장으로는 100 넘게 안 오름. 상수 14곳은 별도");
 }
 // ---- 묶음 C: 100 넘는 경험치 성장(레벨업 구간)
 GrowRet=game+kGrowRet;bool c=false;
 if(core->Readable(game+kGrowSite,9)&&game[kGrowSite]==0xe9){c=true;if(core->Log)core->Log("Stats200New R43: growth section already hooked; left as is.");}
 else{
  HookPlan gp(api);gp.jump(kGrowSite,kGrowExpected,9,(void*)&GrowBridge);
  c=gp.install("SAN14CE.Stats200New.Grow");
  if(core->Log)core->Log(c?"Stats200New R43: growth beyond 100 installed at exe+0x2bc30a (levels 101-200 need t100+(L-100)*(t100-t99)).":"Stats200New R43: growth beyond 100 NOT installed; experience growth stops at 100.");
  if(!c)r39::fail_plan(17,"SAN14_Stats200New.dll",gp,"growth section exe+0x2bc30a",L"능력치 상한 200(17번) 중 '100 넘어서도 경험치로 성장'이 적용 안 됨 → 경험치로는 100 까지만 오름. 편집·표시 상한 200 은 별도");
 }
 unsigned ok=(a>0?14:0)+(b?1:0)+(c?1:0);
 r39::start(17,"SAN14_Stats200New.dll",ok==16?"OK":ok?"PART":"FAIL",ok,16,a==2?"caps=direct":"caps=modcore",ok==16?"-":(a<=0?"caps bytes differ":!b?"xp relay failed":"growth section failed"));
 return ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
