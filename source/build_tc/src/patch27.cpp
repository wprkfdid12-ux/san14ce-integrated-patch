#include "patch27.h"
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
  if(!enabled27(RemoteRecruit)||(U8*)__builtin_return_address(0)==game27+0x32d7d2)return native; // AI city-distance table stays native
  if(!from||!to||from==to)return native;
  int a=fn<int(*)(void*)>(game27,0x22d2c0)(from),b=fn<int(*)(void*)>(game27,0x22d2c0)(to);
  if(a<0||b<0||a>=0xbd10||b>=0xbd10||a==b)return native;
  int steps=HexSteps(a,b);if(steps<2)return native;
  long long straight=fn<long long(*)(int,U8*,void*)>(game27,0x230cc0)(steps,executor,home);
  return straight>native?straight:native;
 }
 int RemoteAllowed(){auto*s=(Scope*)TlsGetValue(scope_tls);return enabled27(RemoteRecruit)&&s&&s->recruit;}
 int DeathScopeActive(){auto*s=(Scope*)TlsGetValue(scope_tls);return enabled27(BattleDeaths)&&s&&s->death!=nullptr;}
 int DeathAllowed(U8*person){auto*s=(Scope*)TlsGetValue(scope_tls);return enabled27(BattleDeaths)&&s&&s->death==person;}
 __declspec(dllexport) int RemoteCandidate(void*a,void*b){Scope s(enabled27(RemoteRecruit),nullptr);return fn<int(*)(void*,void*)>(game27,0x1eedf0)(a,b);}
 // R9: the post-award screen treats every "acting" officer as eligible, not only deployed
 // commanders (user request). The one order kind the R2 design excluded (0x10) stays excluded.
 __declspec(dllexport) int PostBusy(U8*person){
  int original=fn<int(*)(void*)>(game27,0x231770)(person);
  if(!original||!enabled27(DeployedPosts)||!core27->Readable(person,0xe7))return original;
  U8*order=fn<U8*(*)(void*)>(game27,0x229fc0)(person);
  if(fn<int(*)(void*)>(game27,0x32e4c0)(order)){
   if(!core27->Readable(order,8)||!core27->Readable(at<U8*>(order,0),0xa0))return original;
   if(reinterpret_cast<int(*)(void*)>(at<void*>(at<U8*>(order,0),0x98))(order)==0x10)return original;
  }
  return 0;
 }
 __declspec(dllexport) int DeathCutin(U8*scene,void*position){
  int native=fn<int(*)(void*,void*)>(game27,0x367310)(scene,position);if(native||!enabled27(BattleDeaths))return native;
  if(scene&&(!core27->Readable(scene,0x1e4)||at<int>(scene,0x1e0)>=4))return 0;
  void*app=fn<void*(*)()>(game27,0x14bec0)();if(!app)return 0;
  return fn<void*(*)(void*)>(game27,0x14bdb0)(app)!=nullptr;
 }
 // R10: the native fatal routine takes the DYING officer in rcx and the killer in rdx
 // (both call sites 0x258a6b / 0x27ce59; the routine keeps rcx in r13 and tests the
 // "player involved" flag at 0x2950df, where DeathBridge passes r13). R2 scoped the
 // killer, so AI-vs-AI deaths never reached the native notice/report/history path.
 __declspec(dllexport) int FatalBattle(U8*person,U8*killer,int reason,int fourth,int notify,void*sixth,int seventh,int history,int silent){
  Scope s(false,enabled27(BattleDeaths)?person:nullptr);
  return fn<int(*)(U8*,U8*,int,int,int,void*,int,int,int)>(game27,0x294f20)(person,killer,reason,fourth,notify,sixth,seventh,history,silent);
 }
 __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*a){
  if(!valid_api(a))return 0;core27=a;game27=a->resolved->image;
  if(!Patch27LoadOptions())return 0;
  if(HMODULE common=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll")){
   auto wait=reinterpret_cast<U32(*)()>(GetProcAddress(common,"SAN14_WaitConvenienceV1"));
   if(!wait||!wait()){if(a->Log)a->Log("Patch27: common module startup incomplete; no game hooks changed.");return 0;}
  }
  // R3: settings page 6 is linked by a start-up worker once the common module is ready,
  // independent of any settings window callback (the window tick path remains as a fallback).
  Patch27StartUiAttach();
  // R26: 1번 설정 창 '증축 이벤트 생략'(통합편의 증축 스위치) — 25번 설치 여부와 무관
  CityEventSkipInstall(a);
  if(!Patch27FeaturesOn()){if(a->Log)a->Log("Patch27 R22: features off (item 25 not installed); settings pages only, no game hooks.");return 1;}
  scope_tls=TlsAlloc();if(scope_tls==TLS_OUT_OF_INDEXES)return 0;
  RemoteYes=(UPTR)(game27+0x1eef60);RemoteNo=(UPTR)(game27+0x1ef00d);DeathYes=(UPTR)(game27+0x2950e7);DeathNo=(UPTR)(game27+0x2959b5);
  EmployCityFn=(UPTR)(game27+0x22cae0);EmployContA=(UPTR)(game27+0x1e8437);EmployContB=(UPTR)(game27+0x1e8800);
  {HookPlan p(a);const U8 e[]={0x3b,0xd8,0x0f,0x85,0xad,0,0,0};p.call(0x6dd049,0x1eedf0,(void*)RemoteCandidate);p.jump(0x1eef58,e,sizeof(e),(void*)RemoteBridge);
   // R12: recruit order daily territory check (instant-return and 10/30-day variants), same bytes at both sites.
   const U8 ea[]={0xe8,0xec,0x46,0x04,0x00,0x0f,0xb6,0x4e,0x71,0x38,0x48,0x14,0xeb,0x3a};
   const U8 eb[]={0xe8,0x23,0x43,0x04,0x00,0x0f,0xb6,0x4e,0x71,0x38,0x48,0x14,0xeb,0x3a};
   p.jump(0x1e83ef,ea,sizeof(ea),(void*)EmployBridgeA);p.jump(0x1e87b8,eb,sizeof(eb),(void*)EmployBridgeB);
   if(p.install("SAN14CE.Patch27.RemoteRecruit"))installed27|=RemoteRecruit;}
  // R13: travel days for remote ronin (separate group: if another module owns 0x230a40, recruit still works).
  if(installed27&RemoteRecruit){HookPlan p(a);TravelDaysCont=(UPTR)(game27+0x230a4f);
   const U8 t[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18};
   p.jump(0x230a40,t,sizeof(t),(void*)TravelDays);
   if(!p.install("SAN14CE.Patch27.RemoteRecruitDays")&&a->Log)a->Log("Patch27: travel-days hook not installed; remote recruit keeps native day counts.");}
  {HookPlan p(a);p.call(0x72ce95,0x231770,(void*)PostBusy);p.call(0x72d4eb,0x231770,(void*)PostBusy);if(p.install("SAN14CE.Patch27.DeployedPosts"))installed27|=DeployedPosts;}
  {HookPlan p(a);const U8 e[]={0x85,0xdb,0x0f,0x84,0xce,8,0,0};
   p.call(0x2589a2,0x367310,(void*)DeathCutin);p.call(0x27cdc6,0x367310,(void*)DeathCutin);
   p.call(0x258a6b,0x294f20,(void*)FatalBattle);p.call(0x27ce59,0x294f20,(void*)FatalBattle);
   p.jump(0x2950df,e,sizeof(e),(void*)DeathBridge);
   if(DeathLogsPlan(&p)&&p.install("SAN14CE.Patch27.BattleDeaths"))installed27|=BattleDeaths;
  }
  if(installed27&BattleDeaths)DeathLogsAttach();
  if(NamesInstall(a))installed27|=ExtendedNames;
  if(!installed27){TlsFree(scope_tls);scope_tls=TLS_OUT_OF_INDEXES;}
  if(a->Log)a->Log("Patch27: checked feature groups, native death presentation/history and independent persistent switches.");
  return installed27!=0;
 }
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
