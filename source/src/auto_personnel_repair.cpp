#include "module_hooks.h"
static const S14_CoreAPI*api;static U8*module;static bool attempted,attached;
using PolicyFn=U8(*)(uint64_t);static PolicyFn original_policy;
static bool save(uint64_t policy,U8 active){return fn<U8(*)(uint64_t,U8)>(module,0x4c440)(policy,active)!=0;}
static void refresh(){fn<void(*)()>(module,0x6b130)();}
extern "C" __declspec(dllexport) void AutoRepairMasterChanged(U8 enabled){
 if(!module)return;enabled=enabled?1:0;
 uint64_t policy=at<uint64_t>(module,0x105010);
 if(!save(policy,enabled)){
  __atomic_exchange_n(module+0x10500a,U8(!enabled),__ATOMIC_SEQ_CST);
  if(api->Log)api->Log("AutoPersonnel: could not save activation; master switch restored.");return;
 }
 __atomic_exchange_n(module+0x19e77c,enabled,__ATOMIC_SEQ_CST);refresh();
}
extern "C" __declspec(dllexport) U8 AutoRepairPolicy(uint64_t wanted){
 if(!module||!original_policy)return 0;
 uint64_t old=at<uint64_t>(module,0x105010);
 bool resume=at<U8>(module,0x10500a)&&at<U8>(module,0x19e77d)&&!at<U8>(module,0x19e77c)&&!(old&1)&&(wanted&1);
 if(!resume)return original_policy(wanted);
 if(U32(wanted)>15||U32(wanted>>32)>100||!save(wanted,1))return 0;
 InterlockedExchange64(reinterpret_cast<volatile LONG64*>(module+0x105010),LONG64(wanted));
 __atomic_exchange_n(module+0x19e77c,U8(1),__ATOMIC_SEQ_CST);refresh();return 1;
}
// R3: attach once the common module reports start-up complete, from a worker
// started at load time. Previously this only ran when the settings window ticked,
// so the repair stayed inactive until that window was opened.
static SRWLOCK attach_lock=SRWLOCK_INIT;
static void attach(){
 if(attached||attempted)return;
 if(!GetModuleHandleW(L"SAN14_IntegratedConvenience.dll"))return;
 attempted=true;module=known_common();if(!module){if(api->Log)api->Log("AutoPersonnel: unknown common module preserved.");
  r39::fail(2,"SAN14_AutoPersonnelRepair.dll","ic_build","attach() -> known_common() SHA-256 of SAN14_IntegratedConvenience.dll","known IC build (R40 ae365eab… / R39 / ce101 / R11 / P1)","IC loaded but hash unknown",L"자동인사 복구(2번: 켠 뒤 저장 상태 복원)가 안 붙음 → 자동인사는 통합편의 원래 동작. 통합편의 파일이 통합패치 판이 아님");return;}
 const U8 expected[]={0x40,0x53,0x48,0x83,0xec,0x20};ModuleTransaction tx;
 original_policy=reinterpret_cast<PolicyFn>(tx.detour(module+0x65580,expected,sizeof(expected),(void*)AutoRepairPolicy));
 tx.pointer(module+0x19e6f0,nullptr,(void*)AutoRepairMasterChanged);
 attached=tx.install();if(!attached){r39::fail_tx(2,"SAN14_AutoPersonnelRepair.dll",tx.badinfo,module,"IC","auto-personnel policy fn IC+0x65580 / master-changed slot IC+0x19e6f0",L"자동인사 복구(2번: 켠 뒤 저장 상태 복원)가 안 붙음 → 자동인사는 통합편의 원래 동작");original_policy=nullptr;module=nullptr;}
 if(api->Log)api->Log(attached?"AutoPersonnel R3: explicit master/reward enable now restores saved scheduler state; startup pause retained.":"AutoPersonnel: foreign hook or write failure; original callbacks preserved.");
}
extern "C" __declspec(dllexport) void AutoRepairUiTick(){
 if(attached||attempted)return;
 AcquireSRWLockExclusive(&attach_lock);attach();ReleaseSRWLockExclusive(&attach_lock);
}
static DWORD WINAPI attach_worker(LPVOID){
 uint64_t until=GetTickCount64()+600000; // up to 10 minutes of real time
 while(!attached&&!attempted&&GetTickCount64()<until){
  HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
  auto wait=common?reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1")):nullptr;
  if(wait&&wait()){AcquireSRWLockExclusive(&attach_lock);attach();ReleaseSRWLockExclusive(&attach_lock);return 0;}
  Sleep(1000);
 }
 return 0;
}
// R4 (R17 패키지): 설치 순서 보장.
//  통합편의(1~12번)는 자기 시작 작업 때 게임 함수 몇 곳(진행 배속 0x44acf0, 논공·투자 쪽)이
//  원본인지 확인하고 나서 고친다. 이 모듈들의 ini 이름 순서상 120프레임(20번)·논공 개성(14번)이
//  같은 함수를 먼저 고치면 통합편의는 "원본 아님"으로 보고 진행 배속(7번)·자동 투자/논공(2번)·
//  논공 동시(3번)를 스스로 끈다. 25번(00_LegionCommands)이 깔려 있으면 그 모듈이 통합편의를 먼저
//  불러 시작 작업이 끝날 때까지 기다려 주어서 문제가 가려졌다(시작 에뮬레이션으로 확인).
//  여기서 25번과 똑같은 순서(등록 Enabled=1 확인 → 같은 DLL 로드 → SAN14_WaitConvenienceV1)를
//  밟아, 25번 설치 여부와 상관없이 통합편의가 다른 모듈보다 먼저 설치를 끝내게 한다.
//  이 파일의 ini(AutoPersonnelRepair.ini)는 20번(FPS120.ini)·14번(MeritTraits.ini)보다 앞에 로드된다.
//  기다림은 통합편의 쪽 한도(10ms×1500=15초)를 그대로 쓰고, 실패해도 원래 동작(비동기 연결)으로 간다.
static void convenience_first(){
 wchar_t dir[1024];DWORD n=GetModuleFileNameW(nullptr,dir,1024);if(!n||n>=900)return;
 while(n&&dir[n-1]!=L'\\'&&dir[n-1]!=L'/')n--;if(!n)return;dir[n]=0;
 wchar_t ini[1024],dll[1024];wcscpy(ini,dir);wcscpy(ini+n,L"SAN14Mods\\modules\\IntegratedConvenience_GreenlandCompat.ini");wcscpy(dll,dir);wcscpy(dll+n,L"SAN14_IntegratedConvenience.dll");
 if(GetPrivateProfileIntW(L"Mod",L"Enabled",0,ini)!=1||GetFileAttributesW(dll)==INVALID_FILE_ATTRIBUTES)return;
 HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");if(!common)common=LoadLibraryExW(dll,nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 auto wait=common?reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1")):nullptr;if(!wait)return;
 U32 done=wait();
 if(api->Log)api->Log(done?"AutoPersonnel R4: convenience start-up finished before later modules (install order guaranteed).":"AutoPersonnel R4: convenience start-up not finished within its own wait; order not changed.");
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*a){if(!r39::check_api(a,2,"SAN14_AutoPersonnelRepair.dll"))return 0;api=a;convenience_first();HANDLE t=CreateThread(nullptr,0,attach_worker,nullptr,0,nullptr);if(t)CloseHandle(t);return 1;}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
