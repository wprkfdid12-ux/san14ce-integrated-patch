#include "patch27.h"
#include "r39_record.h"
const S14_CoreAPI*core27;U8*game27;U32 installed27;volatile LONG flags27=15;
static DWORD scope_tls=TLS_OUT_OF_INDEXES;
struct Scope{bool recruit;U8*death;Scope*previous;bool active;
 Scope(bool r,U8*p):recruit(r),death(p),previous((Scope*)TlsGetValue(scope_tls)),active(false){active=TlsSetValue(scope_tls,this)!=0;}
 ~Scope(){if(active)TlsSetValue(scope_tls,previous);}
};
extern "C" {
 UPTR RemoteYes,RemoteNo,DeathYes,DeathNo,EmployCityFn,EmployContA,EmployContB,TravelDaysCont;
 void RemoteBridge();void DeathBridge();void EmployBridgeA();void EmployBridgeB();
 long long Orig230a40(U8*,U8*,void*,U8*,void*);
 bool Patch27LoadOptions();
 bool DeathLogsPlan(HookPlan*);void DeathLogsAttach();
 bool NamesInstall(const S14_CoreAPI*);
 void Patch27StartUiAttach();bool Patch27FeaturesOn();
 bool CityEventSkipInstall(const S14_CoreAPI*a);
 __declspec(dllexport) U32 Patch27Installed(){return installed27;}
 __declspec(dllexport) U32 Patch27Flags(){return (U32)flags27;}
 // Persistence and UI validation are owned by the settings page adapter.
 __declspec(dllexport) void Patch27SetFlags(U32 value){InterlockedExchange(&flags27,LONG((value&7)|ExtendedNames));}
 // R12: called only when a recruit order's ronin is in a city the ordering force does not own.
 int EmployRemoteAllowed(U8*order,U8*target){
  if(!enabled27(RemoteRecruit)||!target||!core27->Readable(target,0xe7))return 0;
  return at<U8>(target,0xe6)==5; // still a ronin; native presence/absence and persuasion checks follow
 }
 // R13: hex steps between two map cells (220x220, flat-top columns, even columns shifted down:
 // the neighbour rule of the native step routine 0x2a6b80).
 int HexSteps(int a,int b){
  int x1=a%220,y1=a/220,x2=b%220,y2=b/220;
  int q1=x1,r1=y1-((x1+(x1&1))>>1),q2=x2,r2=y2-((x2+(x2&1))>>1);
  int dq=q1-q2,dr=r1-r2,ds=dq+dr;if(dq<0)dq=-dq;if(dr<0)dr=-dr;if(ds<0)ds=-ds;
  return (dq+dr+ds)/2;
 }
 // R13: native travel days = days(path steps). When the native land path search fails the
 // routine counts one step (1 day for any distance: 장안→회계 1일). Remote ronin made that
 // visible, so with 27번 remote recruit on the result is never below days(straight hex steps).
 // A found path has at least that many steps, so any trip the game can path is unchanged.
 __declspec(dllexport) long long TravelDays(U8*from,U8*to,void*route,U8*executor,void*home){
  long long native=Orig230a40(from,to,route,executor,home);
  if(!enabled27(RemoteRecruit)||(U8*)__builtin_return_address(0)==game27+0x330a92)return native; // AI city-distance table stays native
  if(!from||!to||from==to)return native;
  int a=fn<int(*)(void*)>(game27,0x22f830)(from),b=fn<int(*)(void*)>(game27,0x22f830)(to);
  if(a<0||b<0||a>=0xbd10||b>=0xbd10||a==b)return native;
  int steps=HexSteps(a,b);if(steps<2)return native;
  long long straight=fn<long long(*)(int,U8*,void*)>(game27,0x2331f0)(steps,executor,home);
  return straight>native?straight:native;
 }
 int RemoteAllowed(){auto*s=(Scope*)TlsGetValue(scope_tls);return enabled27(RemoteRecruit)&&s&&s->recruit;}
 int DeathScopeActive(){auto*s=(Scope*)TlsGetValue(scope_tls);return enabled27(BattleDeaths)&&s&&s->death!=nullptr;}
 int DeathAllowed(U8*person){auto*s=(Scope*)TlsGetValue(scope_tls);return enabled27(BattleDeaths)&&s&&s->death==person;}
 __declspec(dllexport) int RemoteCandidate(void*a,void*b){Scope s(enabled27(RemoteRecruit),nullptr);return fn<int(*)(void*,void*)>(game27,0x1f0a30)(a,b);}
 // R9: the post-award screen treats every "acting" officer as eligible, not only deployed
 // commanders (user request). The one order kind the R2 design excluded (0x10) stays excluded.
 __declspec(dllexport) int PostBusy(U8*person){
  int original=fn<int(*)(void*)>(game27,0x233be0)(person);
  if(!original||!enabled27(DeployedPosts)||!core27->Readable(person,0xe7))return original;
  U8*order=fn<U8*(*)(void*)>(game27,0x22c4b0)(person);
  if(fn<int(*)(void*)>(game27,0x331780)(order)){
   if(!core27->Readable(order,8)||!core27->Readable(at<U8*>(order,0),0xa0))return original;
   if(reinterpret_cast<int(*)(void*)>(at<void*>(at<U8*>(order,0),0x98))(order)==0x10)return original;
  }
  return 0;
 }
 __declspec(dllexport) int DeathCutin(U8*scene,void*position){
  int native=fn<int(*)(void*,void*)>(game27,0x36a850)(scene,position);if(native||!enabled27(BattleDeaths))return native;
  if(scene&&(!core27->Readable(scene,0x1e4)||at<int>(scene,0x1e0)>=4))return 0;
  void*app=fn<void*(*)()>(game27,0x14dae0)();if(!app)return 0;
  return fn<void*(*)(void*)>(game27,0x14d9d0)(app)!=nullptr;
 }
 // R10: the native fatal routine takes the DYING officer in rcx and the killer in rdx
 // (both call sites 0x258a6b / 0x27ce59; the routine keeps rcx in r13 and tests the
 // "player involved" flag at 0x2950df, where DeathBridge passes r13). R2 scoped the
 // killer, so AI-vs-AI deaths never reached the native notice/report/history path.
 __declspec(dllexport) int FatalBattle(U8*person,U8*killer,int reason,int fourth,int notify,void*sixth,int seventh,int history,int silent){
  Scope s(false,enabled27(BattleDeaths)?person:nullptr);
  return fn<int(*)(U8*,U8*,int,int,int,void*,int,int,int)>(game27,0x2979b0)(person,killer,reason,fourth,notify,sixth,seventh,history,silent);
 }
 // 통합편의 시작 상태: 1 = 끝남(또는 통합편의 안 실림 → 예전처럼 그냥 진행), 0 = 아직(wait() 한 번 = 통합편의 쪽 한도 15초), -1 = 기다림 함수 없음
 static int p27_ic_state(){
   HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");if(!common)return 1;
   auto wait=reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1"));if(!wait)return -1;
   return wait()?1:0;
 }
 static int p27_after_ic(const S14_CoreAPI*a,int ic){
   if(ic<=0){
    bool wait=ic==0;
    if(a->Log)a->Log(Patch27FeaturesOn()?"Patch27: common module startup incomplete; no game hooks changed.":"Patch27 R22: features off (item 25 not installed); common module startup incomplete - settings pages not attached.");
    if(Patch27FeaturesOn())r39::fail(25,"SAN14_Patch27.dll","ic_wait","SAN14_ModInitV1 -> SAN14_WaitConvenienceV1 (SAN14_IntegratedConvenience.dll)",wait?"IC startup finished (wait()!=0)":"export SAN14_WaitConvenienceV1 present",wait?"wait()==0 (IC startup incomplete within 10 minutes)":"export missing",L"25번(원격 등용·출진 중 관직·전사 기록·이름 확장) 전부 안 켜짐. 통합편의 시작이 끝나지 않았거나 통합편의 파일이 다른 판");
    return 0;
   }
   // R3: settings page 6 is linked by a start-up worker once the common module is ready,
   // independent of any settings window callback (the window tick path remains as a fallback).
   Patch27StartUiAttach();
   // R26: 1번 설정 창 '증축 이벤트 생략'(통합편의 증축 스위치) — 25번 설치 여부와 무관
   CityEventSkipInstall(a);
   if(!Patch27FeaturesOn()){if(a->Log)a->Log("Patch27 R22: features off (item 25 not installed); settings pages only, no game hooks.");return 1;}
   scope_tls=TlsAlloc();if(scope_tls==TLS_OUT_OF_INDEXES){r39::fail(25,"SAN14_Patch27.dll","tls","SAN14_ModInitV1 -> TlsAlloc()","TLS index","TLS_OUT_OF_INDEXES",L"25번 기능 전부 안 켜짐");return 0;}
   RemoteYes=(UPTR)(game27+0x1f0ba0);RemoteNo=(UPTR)(game27+0x1f0c4d);DeathYes=(UPTR)(game27+0x297b77);DeathNo=(UPTR)(game27+0x298445);
   EmployCityFn=(UPTR)(game27+0x22efd0);EmployContA=(UPTR)(game27+0x1ea047);EmployContB=(UPTR)(game27+0x1ea410);
   {HookPlan p(a);const U8 e[]={0x3b,0xd8,0x0f,0x85,0xad,0,0,0};p.call(0x6df899,0x1f0a30,(void*)RemoteCandidate);p.jump(0x1f0b98,e,sizeof(e),(void*)RemoteBridge);
    // R12: recruit order daily territory check (instant-return and 10/30-day variants), same bytes at both sites.
    // R43(새 판): 두 자리의 call rel32 가 바뀜(대상 0x22efd0 = 옛 0x22d580 EmployCityFn). 0x1e9fff+5+0x44fcc / 0x1ea3c8+5+0x44c03 = 0x22efd0
    const U8 ea[]={0xe8,0xcc,0x4f,0x04,0x00,0x0f,0xb6,0x4e,0x71,0x38,0x48,0x14,0xeb,0x3a};
    const U8 eb[]={0xe8,0x03,0x4c,0x04,0x00,0x0f,0xb6,0x4e,0x71,0x38,0x48,0x14,0xeb,0x3a};
    p.jump(0x1e9fff,ea,sizeof(ea),(void*)EmployBridgeA);p.jump(0x1ea3c8,eb,sizeof(eb),(void*)EmployBridgeB);
    if(p.install("SAN14CE.Patch27.RemoteRecruit"))installed27|=RemoteRecruit;else r39::fail_plan(25,"SAN14_Patch27.dll",p,"remote recruit (call 0x6df899, jump 0x1f0b98, employ 0x1e9fff/0x1ea3c8)",L"원격 등용(25번)이 안 됨 → 게임 기본. 다른 25번 묶음은 따로 확인");}
   // R13: travel days for remote ronin (separate group: if another module owns 0x230a40, recruit still works).
   if(installed27&RemoteRecruit){HookPlan p(a);TravelDaysCont=(UPTR)(game27+0x232f7f);
    const U8 t[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18};
    p.jump(0x232f70,t,sizeof(t),(void*)TravelDays);
    if(!p.install("SAN14CE.Patch27.RemoteRecruitDays")){if(a->Log)a->Log("Patch27: travel-days hook not installed; remote recruit keeps native day counts.");r39::fail_plan(25,"SAN14_Patch27.dll",p,"remote recruit travel days exe+0x232f70",L"원격 등용 이동 일수 계산이 게임 기본값(원격 등용 자체는 됨)");}}
   {HookPlan p(a);p.call(0x72f805,0x233be0,(void*)PostBusy);p.call(0x72fe5b,0x233be0,(void*)PostBusy);if(p.install("SAN14CE.Patch27.DeployedPosts"))installed27|=DeployedPosts;else r39::fail_plan(25,"SAN14_Patch27.dll",p,"deployed-officer post busy calls exe+0x72f805/0x72fe5b",L"출진 중 무장 관직 임명 제한(25번)이 안 됨 → 게임 기본");}
   {HookPlan p(a);const U8 e[]={0x85,0xdb,0x0f,0x84,0xce,8,0,0};
    p.call(0x25b022,0x36a850,(void*)DeathCutin);p.call(0x27f716,0x36a850,(void*)DeathCutin);
    p.call(0x25b0eb,0x2979b0,(void*)FatalBattle);p.call(0x27f7a9,0x2979b0,(void*)FatalBattle);
    p.jump(0x297b6f,e,sizeof(e),(void*)DeathBridge);
    if(DeathLogsPlan(&p)&&p.install("SAN14CE.Patch27.BattleDeaths"))installed27|=BattleDeaths;
    else r39::fail_plan(25,"SAN14_Patch27.dll",p,"battle deaths (cutin 0x25b022/0x27f716, fatal 0x25b0eb/0x27f7a9, jump 0x297b6f, death logs)",L"전투 전사·전사 기록(25번)이 안 됨 → 게임 기본");
   }
   if(installed27&BattleDeaths)DeathLogsAttach();
   if(NamesInstall(a))installed27|=ExtendedNames;   // 실패 창은 NamesInstall 안에서(자리·바이트 포함)
   if(!installed27){TlsFree(scope_tls);scope_tls=TLS_OUT_OF_INDEXES;}
   if(a->Log)a->Log("Patch27: checked feature groups, native death presentation/history and independent persistent switches.");
   return installed27!=0;
  }
 // 15초 안에 못 끝났을 때만: 뒤 작업에서 최대 10분 더 기다렸다가 끝나는 즉시 25번을 켬(게임 진행은 안 막음)
 static DWORD WINAPI p27_worker(LPVOID){
   uint64_t until=GetTickCount64()+600000;int ic=0;
   while(GetTickCount64()<until){ic=p27_ic_state();if(ic!=0)break;Sleep(1000);}
   if(ic==1&&core27->Log)core27->Log("Patch27: common module startup finished late; continuing install from background worker.");
   p27_after_ic(core27,ic);return 0;
 }
 __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*a){
  if(!r39::check_api(a,25,"SAN14_Patch27.dll"))return 0;core27=a;game27=a->resolved->image;
  if(!Patch27LoadOptions()){r39::fail(25,"SAN14_Patch27.dll","options","SAN14_ModInitV1 -> Patch27LoadOptions() (San14ModSettings.ini / Patch27.ini)","options loaded","load failed",L"25번(원격 등용·출진 중 관직·전사 기록·이름 확장)과 1번 설정 쪽 6쪽이 전부 안 켜짐");return 0;}
  // 정식판 뒤(제보 ic_wait): 통합편의 시작을 여기서 15초만 기다리고 못 끝나면 실패 창 + 25번 전부 안 켜짐(25번 안 깐 사람에게도 창)이었음.
  //  평소(15초 안에 끝남)는 예전과 똑같이 이 자리에서 바로 진행 → 통합편의가 뒤 모듈(20번·14번)보다 먼저 끝나는 순서 그대로.
  //  15초를 넘긴 느린 PC 만: 뒤 작업에서 최대 10분 더 기다려 끝나는 즉시 켬(AutoPersonnelRepair·ExecuteLock·ICGuard 와 같은 방식).
  //  실패 창은 25번을 깔았고 10분 안에도 안 끝났을 때만.
  int ic=p27_ic_state();
  if(ic==0){
   if(HANDLE t=CreateThread(nullptr,0,p27_worker,nullptr,0,nullptr)){CloseHandle(t);if(a->Log)a->Log("Patch27: common module startup slow (>15 s); waiting in background up to 10 minutes.");return 1;}
  }
  return p27_after_ic(a,ic);
 }
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
