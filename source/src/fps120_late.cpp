// 19번 120프레임 — 통합편의 6번과의 순서 경쟁을 없애는 부품 SAN14_FPS120Late.dll (2026-11-10, P201 · 교훈 L030·L031)
//  제보(삼갤러25, 게임 새로 설치+통합+날씨만): 종합 진단 "6번 진행 배속: 진행속도 확장 준비 검사 실패. 원래 세 단계 유지".
//  원인: 19번 본체 SAN14_FPS120.dll 이 고치는 7곳 중 일기토 갱신 호출 자리 exe+0x44df40(call, 5바이트)이 통합편의
//        (SAN14_IntegratedConvenience.dll) 6번 진행 배속의 준비 검사 구간(통합편의 표 0xa3150 의 0번: exe+0x44dee0 부터 2500바이트,
//        원래 바이트만 허용) 안. 통합편의는 켜지고 1초쯤 뒤 그 구간을 한 번만 확인 → 그때 19번이 고쳐 놓은 상태면 6번을 조용히 안 켬.
//        어느 쪽이 먼저냐는 PC 속도·모듈 수에 달려 일부 PC 에서만 났다.
//  고침(본체는 한 글자도 안 바꿈. 본체는 아직 이 판 소스가 없는 바이너리):
//   1) 등록(FPS120.ini)을 이 부품으로 돌린다. 이 부품이 예전과 같은 때(모듈 시작 때 — 게임 화면이 돌기 전)에 본체를 싣고 본체의 SAN14_ModInitV1 을 부른다.
//      → 본체의 후크 7곳·시작 기록('FPS120 R8 ACTIVE')·시점은 예전 그대로.
//   2) 통합편의가 깔려 있으면, 본체가 끝난 직후 exe+0x44df40 의 call 대상(4바이트)만 게임 원래 값으로 잠깐 되돌린다.
//      → 통합편의가 확인할 때 그 구간은 원래 모양.
//   3) 통합편의 확인이 끝나면(SAN14_WaitConvenienceV1 = 1, ic_after_restore.h) call 대상을 본체 것으로 다시 넣는다.
//      되돌릴 때·다시 넣을 때 바뀌는 건 같은 call 명령의 대상 4바이트뿐이라 한 번의 원자적 쓰기로 끝난다(게임 화면이 도는 중이어도 반쪽 명령이 안 생김).
//      그 1초 남짓 동안 빠지는 건 '일기토 중 60프레임 제한' 하나(그때는 일기토가 없음). 120프레임 자체는 처음부터 켜져 있다.
//   통합편의가 없거나 꺼져 있으면 2)·3)을 안 한다(기대지 않음).
//  본체를 우리 소스로 다시 짜면(옛 R8 소스: 롤백판 patch20_duel60stock/fps120_duel60stock.c — kits/FPS120_R8_옛소스) 이 부품은 본체에 합친다 — 05_소스장부.tsv.
#include "common.h"
#include "r39_record.h"
#include "ic_after_restore.h"
static const S14_CoreAPI*core;static U8*game;static const char*MOD="SAN14_FPS120Late.dll";
static const U32 kDuelSite=0x44df40;
static const U8 kDuelNative[5]={0xe8,0xbb,0xf6,0xf2,0xff};      // call 0x37d600 (일기토 관리자 갱신) — 새 판(S14_GAME_STAMP) 원래 바이트
static LONG native_disp(){LONG v;memcpy(&v,kDuelNative+1,4);return v;}
static LONG fps_disp;                                            // 본체가 넣은 call 대상(본체 쪽 다리)
// call 대상 4바이트를 한 번에 바꿈. 0x44df41~44 는 한 캐시 줄(0x44df40~7f) 안이라 원자적.
static bool swap_disp(LONG v){
 U8*p=game+kDuelSite+1;DWORD old=0,un=0;if(!VirtualProtect(p,4,0x40,&old))return false;
 InterlockedExchange(reinterpret_cast<volatile LONG*>(p),v);
 VirtualProtect(p,4,old,&un);FlushInstructionCache(GetCurrentProcess(),p-1,5);return true;
}
static LONG cur_disp(){LONG v;memcpy(&v,game+kDuelSite+1,4);return v;}
// 통합편의 확인이 끝난 뒤: 일기토 갱신 후크를 다시 넣음
static void reapply(){
 U8*p=game+kDuelSite;
 if(core->Readable(p,5)&&p[0]==0xe8&&cur_disp()==native_disp()){
  if(swap_disp(fps_disp)){if(core->Log)core->Log("FPS120Late R43: duel-update hook re-applied after IC restore check (duel 60 Hz cap active again).");r39::counter(19,MOD,"duel_hook_reapplied",1);return;}
  U8 want[5]={0xe8};memcpy(want+1,&fps_disp,4);
  r39::fail_bytes(19,MOD,"duel-update call exe+0x44df40: write failed (VirtualProtect)",kDuelSite,want,p,5,L"120프레임(19번)은 켜져 있지만 '일기토 중 60프레임 제한'이 빠짐(일기토가 빨라질 수 있음). 게임을 다시 켜 주세요");return;}
 if(core->Readable(p,5)&&p[0]==0xe8&&cur_disp()==fps_disp){if(core->Log)core->Log("FPS120Late R43: duel-update hook already in place.");return;}
 // 그 사이 다른 누군가가 이 자리를 바꿈 → 손대지 않고 알림(누구인지는 r43 충돌 표시가 찾음)
 U8 want[5];memcpy(want,kDuelNative,5);U8 act[5]={};if(core->Readable(p,5))memcpy(act,p,5);
 r39::fail_bytes(19,MOD,"duel-update call exe+0x44df40 changed by another module while waiting for IC",kDuelSite,want,act,5,L"120프레임(19번)은 켜져 있지만 '일기토 중 60프레임 제한'을 다시 넣지 못함(다른 모드가 그 자리를 바꿈)");
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,19,MOD))return 0;core=api;game=api->resolved->image;
 wchar_t r[1024],p[1100];p[0]=0;
 if(!r39::game_root(r,1024)){r39::start(19,MOD,"FAIL",0,1,"-","game folder unknown");
  r39::fail(19,MOD,"root","SAN14_ModInitV1 -> r39::game_root","game folder path","(none)",L"120프레임(19번)이 안 켜짐 → 60프레임 그대로");return 0;}
 r39::wcat(p,1100,r);r39::wcat(p,1100,L"SAN14_FPS120.dll");
 HMODULE h=LoadLibraryExW(p,nullptr,0);
 if(!h){r39::start(19,MOD,"FAIL",0,1,"-","SAN14_FPS120.dll not loaded");
  r39::fail(19,MOD,"load","SAN14_ModInitV1 -> LoadLibraryExW(game folder\\SAN14_FPS120.dll)","module handle","NULL",L"120프레임(19번) 본체 파일(SAN14_FPS120.dll)을 못 실음 → 60프레임 그대로. 설치기에서 19번을 다시 설치해 주세요");return 0;}
 auto init=reinterpret_cast<int(*)(const S14_CoreAPI*)>(GetProcAddress(h,"SAN14_ModInitV1"));
 if(!init){r39::start(19,MOD,"FAIL",0,1,"-","SAN14_ModInitV1 export missing");
  r39::fail(19,MOD,"export","SAN14_ModInitV1 -> GetProcAddress(SAN14_FPS120.dll, SAN14_ModInitV1)","function","NULL",L"120프레임(19번) 본체가 맞는 파일이 아님 → 60프레임 그대로. 설치기에서 19번을 다시 설치해 주세요");return 0;}
 bool native_before=core->Readable(game+kDuelSite,5)&&!memcmp(game+kDuelSite,kDuelNative,5);
 int ok=init(api);
 if(!ok){r39::start(19,MOD,"FAIL",0,1,"-","SAN14_FPS120.dll init returned 0");
  if(core->Log)core->Log("FPS120Late R43: SAN14_FPS120.dll init returned 0 (see its own FPS120 lines above).");
  r39::fail(19,MOD,"init","SAN14_ModInitV1 -> SAN14_FPS120.dll SAN14_ModInitV1(api)","1","0",L"120프레임(19번)이 안 켜짐 → 60프레임 그대로. 이유는 runtime.log 의 'FPS120' 줄(다른 모드가 같은 자리를 고쳤으면 'unsupported code or hook conflict')");return 0;}
 bool hooked=core->Readable(game+kDuelSite,5)&&game[kDuelSite]==0xe8&&cur_disp()!=native_disp();
 const char*mode="ic-absent";
 if(native_before&&hooked&&icw::ic_installed()){
  fps_disp=cur_disp();
  if(swap_disp(native_disp())){mode="duel-hook-after-ic";
   if(core->Log)core->Log("FPS120Late R43: duel-update call left native until IC restore check finishes (IC 6 speed check covers exe+0x44dee0..0x44e8a3).");
   icw::run_after_restore(reapply,api,"FPS120");}
  else{mode="lift-failed";if(core->Log)core->Log("FPS120Late R43: could not lift duel-update hook; left as FPS120 set it.");}
 }else if(!native_before||!hooked)mode="site-unexpected";   // 자리가 예상과 다름(본체가 판을 달리 봄 등) → 건드리지 않음
 r39::start(19,MOD,"OK",1,1,mode,"-");
 return 1;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
