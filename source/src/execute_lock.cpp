// R42 B4 (26번 처단불가): 처단불가가 켜져 있으면 포로 처우 창의 '처단' 버튼을 회색(누를 수 없음)으로.
//  지금까지(R40 A4): 26번 NoExecute 가 처단을 실제로 막음 — 게임 포로 처우 창에서 처단을 누르면 "군사가 말림"(S1 0x66d20e),
//   7번 포로 회유 창은 ICGuard F(0x180023f04)가 처단 대신 유지. 버튼 자체는 눌림 → 누르고 나서야 막힌 걸 앎.
//  창 구조(게임 코드):
//   - 게임 포로 처우 창 = CTreatPrisonerLayout(vt 0x142def0). 버튼 4개 = 창+0x380 + i*8 (0x66ae00 고리: 번호 2·4·6·8, 글자 0x12·0x3c·0x3d·0x3e).
//     글자 0x3c = '처단'(군주 포로 창 CTreatLordPrisonerDlg 0x80cd5b 에서 처단불가 표시 비트9 이면 끄는 바로 그 버튼 글자) → 처단 = 창+0x388.
//   - 버튼 켜고 끄기는 0x672450 한 곳(선택한 포로 중 +0x15e 비트12 없는 포로가 있으면 처단 켬). 0x66b202(만들 때)·0x66ed11(vt+0x238 갱신)·0x66fb8e 에서 부름.
//   - 7번 포로 회유 창 = IC 가 같은 CTreatPrisonerLayout 을 vtable 사본으로 띄움(IC 0x180047b0c~). IC 는 vt+0x238 을 0x1800678f0 으로 바꿔
//     게임 갱신(0x6713d0) 뒤 자기 0x18006acd0 에서 버튼 4개를 다시 켜고 끔(창 = [IC+0x105050], [IC+0x105058]==2 일 때) → 이 뒤에도 한 번 더 끔.
//   - 군주 포로 창(CTreatLordPrisonerDlg)의 처단 버튼은 NoExecute Mode=1 이 이미 끔(S5 0x80cd67) → 손대지 않음.
//  고침: G1 게임 0x67261e(0x672450 끝, 5바이트 mov rbx,[rsp+0x60]) 에서 처단불가 켜짐이면 창+0x388 버튼을 끔(vt+0xd0, 게임과 같은 호출).
//        G2 IC 0x18006acd0 입구(8바이트) 감싸기: 원래 실행 뒤 같은 처리. IC 가 알려진 판(known_common)일 때만.
//  처단불가 켜짐 = SAN14_ExtBridge.dll SAN14_ExtState(L"NoExecute")==1(설정 창에서 바로 반영) 그리고 SAN14_NoExecute.ini Mode=1(2초마다 다시 읽음).
//   ExtBridge 가 없으면 ICGuard F 와 같은 규칙(NoExecute.dll 올라옴 + ExtBridge.ini [Ext] NoExecute≠0 + NoExecute.ini Enabled=1·Mode=1).
//   NoExecute 가 안 깔렸으면 아무것도 안 함. 꺼지면 다음 갱신 때 게임 원래 규칙대로 다시 켜짐(우리는 끄기만 함).
//  설정: SAN14Mods\modules\ExecuteLock.ini [ExecuteLock] Enabled=1
#include "module_hooks.h"
#include "r39_record.h"
using U64=uint64_t;
static const S14_CoreAPI*core;static U8*game;static U8*ic;static volatile long enabled=1;static unsigned greyed,ic_greyed;static bool g_ok,ic_ok;
static const char*MOD="SAN14_ExecuteLock.dll";
static wchar_t root_dir[1024];
extern "C"{void ElBridge();void*ElRet;}
static void logf(const char*s){if(core&&core->Log)core->Log(s);}
static bool readable(const void*p,size_t n){return p&&core&&core->Readable((void*)p,n);}
// ---- 처단불가 켜짐 판단 ----
static volatile long cached_on=-1,force_on=-1;static U64 cached_at;
static int ini_int(const wchar_t*rel,const wchar_t*sec,const wchar_t*key,int def){wchar_t p[1200];p[0]=0;r39::wcat(p,1200,root_dir);r39::wcat(p,1200,rel);return (int)GetPrivateProfileIntW(sec,key,def,p);}
static bool noexec_on_uncached(){
 if(!GetModuleHandleW(L"SAN14_NoExecute.dll"))return false;
 if(ini_int(L"SAN14Mods\\Ext\\SAN14_NoExecute.ini",L"NoExecute",L"Mode",0)!=1)return false;
 HMODULE eb=GetModuleHandleW(L"SAN14_ExtBridge.dll");auto st=eb?reinterpret_cast<int(*)(const wchar_t*)>(GetProcAddress(eb,"SAN14_ExtState")):nullptr;
 if(st){int s=st(L"NoExecute");return s==1;}
 return ini_int(L"SAN14Mods\\modules\\ExtBridge.ini",L"Ext",L"NoExecute",1)!=0&&ini_int(L"SAN14Mods\\Ext\\SAN14_NoExecute.ini",L"NoExecute",L"Enabled",0)==1;
}
static bool noexec_on(){
 if(force_on>=0)return force_on==1; // 시험 장치 전용
 U64 now=GetTickCount64();
 if(cached_on<0||now-cached_at>2000){cached_on=noexec_on_uncached()?1:0;cached_at=now;}
 return cached_on==1;
}
// ---- 버튼 끄기 ----
static const U32 VT_TREAT=0x1430f80; // CTreatPrisonerLayout
static bool layout_ok(U8*L){
 if(!readable(L,0x3a8))return false;U8*vt=at<U8*>(L,0);
 if(vt==game+VT_TREAT)return true;
 if(ic&&vt==ic+0x106130)return true;   // IC 포로 회유 창 vtable 사본(IC 0x180106128 = RTTI 칸, +8 부터 함수)
 return false;
}
static bool grey(U8*L){
 U8*btn=at<U8*>(L,0x388);if(!readable(btn,0x48))return false;U8*bvt=at<U8*>(btn,0);if(!readable(bvt,0xd8))return false;
 at<void(*)(U8*,int)>(bvt,0xd0)(btn,0);return true;
}
extern "C" void ElFix(U8*L){
 if(!enabled||!L||!noexec_on()||!layout_ok(L))return;
 if(grey(L)){greyed++;if(greyed==1||greyed%50==0)r39::counter(26,MOD,"execute_button_greyed",greyed);}
}
// G2: IC 0x18006acd0 감싸기
using Fn4=U64(*)(U64,U64,U64,U64);static Fn4 o_icpost;
static U64 w_icpost(U64 a,U64 b,U64 c,U64 d){
 U64 r=o_icpost(a,b,c,d);
 if(enabled&&ic&&at<volatile U32>(ic,0x105058)==2){U8*L=at<U8*volatile>(ic,0x105050);
  if(L&&noexec_on()&&layout_ok(L)&&grey(L)){ic_greyed++;if(ic_greyed==1||ic_greyed%50==0)r39::counter(26,MOD,"execute_button_greyed_ic",ic_greyed);}}
 return r;
}
static bool install_ic(U8*m){
 ModuleTransaction tx;static const U8 e[]={0x4c,0x8b,0xdc,0x55,0x49,0x8d,0x6b,0xa1};
 static const U8 tail[]={0x48,0x83,0xc4,0x28,0xe9}; // 0x180067903: IC 갱신 덮개 끝(add rsp,0x28; jmp 0x18006acd0) 지문
 static const U8 cmp7[]={0x83,0x3d,0x64,0xa3,0x09,0x00,0x02};
 if(memcmp(m+0x67903,tail,5)){r39::fail_bytes(26,MOD,"IC update-wrapper tail IC+0x67903",0x67903,tail,m+0x67903,5,L"7번 포로 설득 창의 처단 버튼 회색 표시가 동작 안 함(게임 창 쪽은 그대로)");return false;}
 if(memcmp(m+0x6aced,cmp7,7)){r39::fail_bytes(26,MOD,"IC captive-window state check IC+0x6aced",0x6aced,cmp7,m+0x6aced,7,L"7번 포로 설득 창의 처단 버튼 회색 표시가 동작 안 함(게임 창 쪽은 그대로)");return false;}
 o_icpost=(Fn4)tx.detour(m+0x6acd0,e,8,(void*)w_icpost);
 if(!o_icpost){r39::fail_bytes(26,MOD,"IC post-update fn entry IC+0x6acd0 (detour)",0x6acd0,e,m+0x6acd0,8,L"7번 포로 설득 창의 처단 버튼 회색 표시가 동작 안 함(게임 창 쪽은 그대로)");return false;}
 if(!tx.install()){r39::fail(26,MOD,"ic_install","install_ic -> ModuleTransaction::install (VirtualProtect/write)","install=1","install=0",L"7번 포로 설득 창의 처단 버튼 회색 표시가 동작 안 함(게임 창 쪽은 그대로)");return false;}
 return true;
}
static DWORD WINAPI ic_worker(LPVOID){
 U64 until=GetTickCount64()+600000;
 while(GetTickCount64()<until){
  HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
  auto wait=common?reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1")):nullptr;
  if(wait&&wait())break;
  Sleep(1000);
 }
 U8*m=known_common();
 if(!m){logf("ExecuteLock R42: integrated module (7) not found or unknown build; captive-persuasion window left as is.");
  HMODULE icm=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
  if(icm)r39::fail(26,MOD,"ic_build","ic_worker -> known_common() SHA-256 of SAN14_IntegratedConvenience.dll","known IC build (R40 ae365eab… or earlier P1/R11/ce101/R39)","IC loaded but hash unknown",L"7번 포로 설득 창의 처단 버튼 회색 표시가 동작 안 함(게임 창 쪽은 그대로). 통합편의 파일이 통합패치 판이 아님");
  return 0;}
 ic=m;ic_ok=install_ic(m);
 logf(ic_ok?"ExecuteLock R42: 7 captive-persuasion window also greys the Execute button while No-Execute (26) is on.":"ExecuteLock R42: integrated module bytes differ; captive-persuasion window left as is.");
 r39::start(26,MOD,g_ok&&ic_ok?"OK":(g_ok||ic_ok?"PARTIAL":"FAIL"),(g_ok?1:0)+(ic_ok?1:0),2,"-",ic_ok?"-":"IC window hook not installed");
 return 0;
}
// 시험용
extern "C" __declspec(dllexport) U64 SAN14_ExecuteLockTest(int op,U64 a,U64 b){
 switch(op){
  case 1:game=(U8*)a;core=(const S14_CoreAPI*)b;return 1;
  case 2:ic=(U8*)a;return 1;
  case 3:force_on=(long)a;return 1; // 강제 켬/끔(시험)
  case 4:ElFix((U8*)a);return greyed;
  case 5:return install_ic((U8*)a)?1:0;
  case 6:return (U64)(void*)w_icpost;
  case 7:o_icpost=(Fn4)a;return 1;
  case 8:return ic_greyed;
  case 9:return (U64)(void*)&ElBridge;
  case 10:ElRet=(void*)a;return 1;
 }
 return 0;
}
extern "C" __declspec(dllexport) void SAN14_ExecuteLockSetEnabled(int on){InterlockedExchange(&enabled,on?1:0);}
extern "C" __declspec(dllexport) int SAN14_ExecuteLockState(){return enabled;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,26,MOD))return 0;core=api;game=api->resolved->image;r39::game_root(root_dir,1024);
 wchar_t ini[1100];ini[0]=0;r39::wcat(ini,1100,root_dir);r39::wcat(ini,1100,L"SAN14Mods\\modules\\ExecuteLock.ini");enabled=GetPrivateProfileIntW(L"ExecuteLock",L"Enabled",1,ini)?1:0;
 HookPlan plan(api);
 static const U8 e1[]={0x48,0x8b,0x5c,0x24,0x60};ElRet=game+0x672623;plan.jump(0x67261e,e1,5,(void*)&ElBridge);
 g_ok=plan.install("SAN14CE.ExecuteLock");
 logf(g_ok?"ExecuteLock R42: prisoner-treatment window greys the Execute button while No-Execute (26) is on (0x672450 end).":"ExecuteLock R42: hook NOT installed (bytes differ / conflict). Execute button unchanged.");
 HANDLE t=CreateThread(nullptr,0,ic_worker,nullptr,0,nullptr);if(t)CloseHandle(t);
 if(!g_ok){r39::start(26,MOD,"FAIL",0,2,"-","game window hook not installed");r39::fail_plan(26,MOD,plan,"prisoner-treatment window end exe+0x67261e",L"처단 불가일 때 처단 버튼 회색 표시(26번)가 동작 안 함. 처단 금지 자체(AISearch/NoExecute)는 그대로");}
 return g_ok?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
