#include "r39_record.h"   // R43: common.h(형식·valid_api)+실패 보고
static const S14_CoreAPI *core;
static U8 *game,*module,*tip,*tip_owner;
static bool registered,failed,checked;
static int last=-1,candidate=-1;
static uint64_t hovered_since;
// UI state thread only. Separate records also handle nested report/discovery
// states. If all slots are occupied, leave the next movie entirely native.
struct MovieSkip{U8*state;bool queued;};
static MovieSkip movie_skips[16];
static MovieSkip*movie_record(U8*state){for(auto&r:movie_skips)if(r.state==state)return &r;return 0;}
static void forget_movie(U8*state){if(auto*r=movie_record(state))*r={};}
template<class T>static T fn(uint32_t r){return reinterpret_cast<T>(game+r);}
template<class T>static T vf(void*p,size_t o){return reinterpret_cast<T>(at<void*>(at<void*>(p,0),o));}
using Call4=UPTR(*)(void*,void*,UPTR,UPTR);using One=void(*)(void*);using Two=void(*)(void*,void*);using Flag=void(*)(void*,int);
using ReportInt=void(*)(void*,int,int);using ReportPtr=void(*)(void*,void*,int);using AddNotice=void(*)(void*,void*,UPTR,void*,void*);
static const wchar_t *descriptions[]={
L"사건이나 보고가 발생한 장소로 카메라를 자동 이동합니다.",
L"턴 진행 중 반복되는 보고 화면을 자동으로 넘깁니다. 보고에 따른 결과는 그대로 적용됩니다.",
L"명승을 발견하거나 발전시켰을 때 나오는 연출·개별 발견 보고창을 넘기고, 그때 화면이 어두워지는 효과도 없앱니다. 발견·발전 결과와 보상, 행동보고 이력은 그대로 남습니다.",
L"명승 연출 때 화면이 어두워지는 효과를 없앱니다.",
L"도시 증축 이벤트(그림·대사·결과창·암전)를 띄우지 않습니다. 증축은 그대로 적용되고, 결과는 화면 아래 기록에 중요 표시로 한 줄 남습니다.",
L"붙잡은 포로를 회유하는 추가 기능을 켜거나 끕니다.",
L"재야 무장이 임관을 청하는 등용문 기능을 켜거나 끕니다.",
L"인재 관련 화면에서 추가 설명을 표시합니다.",
L"개성에 마우스를 올리면 그 개성의 효과를 보여 줍니다.",
L"포로 관련 화면에서 포박된 무장 일러스트를 사용합니다.",
L"자동인사 기능을 켜거나 끕니다. 필요한 인사 업무를 자동으로 처리합니다.",
L"여러 무장의 논공을 함께 처리하는 기능을 켜거나 끕니다.",
L"명승에 자동으로 투자하는 기능을 켜거나 끕니다. 투자 비용이 사용됩니다.",
L"논공을 자동으로 처리하는 기능을 켜거나 끕니다.",
L"여러 부대를 함께 움직일 때 사용하는 추가 이동 기능을 켜거나 끕니다.",
L"턴 진행 속도를 더 빠르게 선택할 수 있도록 속도 단계를 늘립니다.",
L"관직이 없는 무장이 지휘할 수 있는 병력 수를 설정합니다.",
L"외교 메뉴의 문전박대 버튼을 보이거나 숨깁니다. 끄면 버튼이 사라지고 사신 자동 거절도 꺼집니다. 거절 방식(OFF·ON·항상)은 외교 메뉴의 문전박대 버튼으로 바꿉니다.",
L"충성도가 설정한 기준보다 낮은 무장에게 자동으로 포상합니다.",
L"추가한 배경음악을 게임 안에서 선택하고 재생합니다.",
L"선택한 아군 부대를 따라 움직이는 수행 기능을 켜거나 끕니다.",
L"타 세력이 지배하는 도시의 재야 무장도 등용 후보에 표시합니다. 신분과 명령 가능 여부는 기존 조건을 따릅니다.",
L"출진 중인 아군 부대의 지휘관에게 관직을 수여할 수 있습니다. 임무 중인 다른 무장의 조건은 그대로입니다.",
L"AI끼리의 전투를 포함한 전사를 순정 일러스트와 메시지로 알리고, 왼쪽 아래 기록창에 남깁니다.",
L"경험치로 능력치가 거의 오르지 않게 하는 데이터를 쓰거나 원래대로 돌립니다. 게임을 다시 시작하면 적용됩니다.",
L"관직·작위·방책 수여로 더해지는 능력치 보너스를 빼거나 원래대로 돌립니다. 게임을 다시 시작하면 적용됩니다.",
L"모든 지역의 내정을 자동으로 맡깁니다.",
L"AI 세력의 명령서를 항상 99장으로 채웁니다.",
L"도시 인구가 1만 아래로 내려가지 않게 합니다.",
L"개성 '저돌'의 효과를 고친 값으로 적용합니다.",
L"내정 자동화가 지역 정보를 처음부터 다시 읽게 합니다. 잠시 뒤 다시 동작합니다.",
L"12번 안개의 미니맵을 보이거나 숨깁니다. 끄면 미니맵과 왼쪽 아래 미니맵 버튼, 미니맵 모서리의 압정 버튼이 모두 사라집니다. 바로 적용됩니다.",
L"12번 안개의 날씨를 켜거나 끕니다. 끄면 날씨 표시창·비·눈 효과·부대창 날씨 표시·날씨 전투 규칙·바람이 빠지고 늘 맑음으로 계산합니다. 계절 패치 날씨와 겹치지 않게 쓸 때 끄세요. 바로 적용됩니다."
};
// R22: 설정 창 표시 위치(쪽*5+줄) → 설명 번호. IC 줄은 ic_layout 변환표와 같다(논공 동시 제거, 명승 두 줄 합침).
static const signed char layout_desc[35]={0,1,2,4,5, 6,7,8,9,10, 12,13,14,15,16, 17,18,19,20,31, 26,27,28,29,30, 21,22,23,24,25, 32,-1,-1,-1,-1};
// 11-10(P203, 교훈 L020): 통합편의 해시를 여기서 따로 들고 있다가 R43 통합편의를 몰라 명승 연출 생략·설정 설명이 조용히 꺼졌다.
//   목록은 common.h known_common() 한 곳만 쓴다(교훈_점검 c02 가 다시 못 따로 들게 막음). 한 번 실패하면 다시 묻지 않는 것은 예전과 같다.
static bool known_module() {
 if(module)return true;
 if(!GetModuleHandleW(L"SAN14_IntegratedConvenience.dll"))return false;   // 아직 안 올라옴 — 다음에 다시
 if(checked)return false; checked=true;
 module=known_common();
 if(!module&&core->Log)core->Log("SettingsHelp: unknown integrated module; settings/scenic extension left inactive.");
 return module!=0;
}
static bool inside(U8*w,const int *pt) {
 if(!w||!core->Readable(w,0x48))return false;
 int width=at<int>(w,0x28),height=at<int>(w,0x2c),x=0,y=0;U8*p=w;
 if(width<=0||height<=0)return false;
 for(unsigned n=0;p&&n<30;++n){if(!core->Readable(p,0x48)||(at<U32>(p,0x40)&3)!=3)return false;x+=at<int>(p,0x20);y+=at<int>(p,0x24);p=at<U8*>(p,8);}
 return !p && pt[0]>=x&&pt[0]<x+width&&pt[1]>=y&&pt[1]<y+height;
}
static void hide(){if(tip)vf<Flag>(tip,0xc8)(tip,0);last=-1;}
static void destroy(void*resources) {
 if(tip){hide();if(registered)vf<One>(tip,0x40)(tip);vf<Two>(tip,0x28)(tip,resources);vf<Flag>(tip,0)(tip,1);}
 tip=tip_owner=0;registered=failed=false;last=candidate=-1;
}
static bool make_tip(U8*owner) {
 if(failed)return false;if(tip)return true;
 tip=fn<U8*(*)(UPTR)>(0x3f90e0)(0x180);if(!tip){failed=true;return false;}
 fn<One>(0x866fa0)(tip);fn<One>(0x92d020)(tip+0x130);at<U8*>(tip,0)=game+0x139f580;at<UPTR>(tip,0x178)=0;tip_owner=owner;
 fn<int(*)(void*,void*)>(0x89e550)(tip,0);vf<Flag>(tip,0xc8)(tip,0);
 if(!at<void*>(tip,0x138)){failed=true;return false;}
 at<int>(tip,0x40)|=8;fn<void(*)(void*,U8)>(0x8a3ac0)(tip,255);vf<Flag>(tip,0x70)(tip,6);return true;
}
extern "C" __declspec(dllexport) UPTR SettingsTick(U8*owner,void*a,UPTR b,UPTR c) {
 UPTR result=fn<Call4>(0x8ca1c0)(owner,a,b,c);
 // Both adapters attach on the native UI thread, before processing this frame.
 if(HMODULE repair=GetModuleHandleW(L"SAN14_AutoPersonnelRepair.dll"))
  if(auto tick=reinterpret_cast<void(*)()>(GetProcAddress(repair,"AutoRepairUiTick")))tick();
 if(HMODULE patch=GetModuleHandleW(L"SAN14_Patch27.dll"))
  if(auto tick=reinterpret_cast<void(*)()>(GetProcAddress(patch,"Patch27UiTick")))tick();
 int idx=-1;
 if(tip_owner && tip_owner!=owner)return result;
 if(known_module() && at<U8*>(module,0x1c10f0)==owner && at<U8>(module,0x1c1100) && at<int>(owner,0x140)==0x52){
  int page=at<int>(module,0x1c1140);U8*input=fn<U8*(*)()>(0x111f0)();
#ifdef SAN14_DOMESTIC_UI_BRIDGE
  // #24 extends the same settings page. Keep its UI bridge in the domestic helper build.
  if(HMODULE domestic=GetModuleHandleW(L"SAN14_DomesticCE.dll"))
   if(auto tick=reinterpret_cast<void(*)(U8*,int,U8*)>(GetProcAddress(domestic,"DomesticUiTick")))tick(owner,page,input);
#endif
  if(page>=0&&page<7&&input)for(int row=0;row<5;++row){int key=layout_desc[page*5+row];if(key<0)continue;
   U8*label=fn<U8*(*)(void*,int)>(0x92f390)(owner+0x130,row*8+0x14);
   if(inside(label,(int*)(input+0x34))){idx=key;break;}
  }
 }
 uint64_t now=GetTickCount64();if(candidate!=idx){candidate=idx;hovered_since=now;hide();}
 if(idx<0){hide();return result;}
 if(now-hovered_since<180||!make_tip(owner))return result;
 // R32: 13번 논공 개성(SAN14_MeritTraits.dll)이 있으면 2번 자동 논공은 IC 가 강제로 끄고 잠근다 → 이유를 안내.
 const wchar_t*text=descriptions[idx];
 if(idx==13&&GetModuleHandleW(L"SAN14_MeritTraits.dll"))text=L"13번 '논공 개성 가챠·군주도 적용'을 사용 중이라 자동 논공을 강제로 껐습니다. 두 기능을 같이 쓰면 논공 기록이 어긋나 저장이 막히는 충돌이 생기기 때문입니다. 논공은 논공 화면에서 직접 해 주세요. 13번을 빼면 다시 켤 수 있습니다.";
 if(last!=idx){fn<void(*)(void*,const wchar_t*)>(0x8a4380)(tip,text);last=idx;}
 if(!registered){vf<One>(tip,0x38)(tip);registered=true;}
 alignas(16) U8 manager[0x40]={};at<void*>(manager,0x10)=tip;at<int>(manager,0x38)=20;at<int>(manager,0x3c)=20;
 fn<One>(0x8a7f60)(manager);vf<Flag>(tip,0xc8)(tip,1);return result;
}
extern "C" __declspec(dllexport) void SettingsHide(U8*owner){if(tip_owner==owner){candidate=-1;hovered_since=0;hide();if(registered){vf<One>(tip,0x40)(tip);registered=false;}}fn<One>(0x8cceb0)(owner);}
extern "C" __declspec(dllexport) void SettingsRelease(U8*owner,void*resources){if(tip_owner==owner)destroy(resources);fn<Two>(0x8ff9c0)(owner,resources);}
extern "C" __declspec(dllexport) int DiscoveryEnter(U8*state,void*a,UPTR b,UPTR c){
 int result=fn<int(*)(void*,void*,UPTR,UPTR)>(0x724e40)(state,a,b,c);
 // Original enum 5 creates CInfoSummarySceneryDiscovery; other discoveries unchanged.
 if(result && known_module() && at<U8>(module,0x19e690+2) && at<U8>(module,0x105002) && at<int>(state,0x4b0)==5){
  // Original phase 1 body: close the two dialogue slots and unregister presentation.
  // Original phase 2 and Exit retain all cleanup and parent result notification.
  fn<void(*)(void*,int)>(0x83e450)(state,3);fn<void(*)(void*,int)>(0x83e450)(state,4);
  vf<One>(state,0x40)(state);at<int>(state,0x50c)=2;
  if(core->Log)core->Log("Scenic discovery presentation skipped; original completion/cleanup retained.");
 }
 return result;
}
extern "C" __declspec(dllexport) int SceneryMovieEnter(U8*state,void*a,UPTR b,UPTR c){
 forget_movie(state);
 int result=fn<int(*)(void*,void*,UPTR,UPTR)>(0x423cd0)(state,a,b,c);
 if(core->Log){
  core->Log(!known_module()?"Scenery R4.3: discovery entered; integrated settings unavailable.":
   !at<U8>(module,0x19e692)?"Scenery R4.3: discovery entered; scenic feature unavailable.":
   !at<U8>(module,0x105002)?"Scenery R4.3: discovery entered; skip setting OFF.":
   "Scenery R4.3: discovery entered; skip setting ON.");
 }
 return result;
}
extern "C" __declspec(dllexport) void SceneryMovieExit(U8*state){
 forget_movie(state);
 // Restores the main UI using the game's own Exit procedure.
 fn<One>(0x423c20)(state);
}
static void SceneryResultSilent(U8*state){
 // Native CSearchSceneryMovieState phase 2 is reproduced up through the history
 // notification. Its final 0x1f0240 call only creates/queues the individual
 // action-report screen, so #5 deliberately omits that one presentation step.
 // This leaves the discovery/reward and Action Report > History entry intact.
 if(at<I32>(state,0x498)!=0&&at<I32>(state,0x49c)>0){
  void*recipient=fn<void*(*)(void*)>(0x1d3bc0)(fn<void*(*)()>(0x3fa4a0)());
  void*report=game+0x1a26440;fn<One>(0x30c380)(report);
  int reward=at<I32>(state,0x498);
  if(reward==0x5c76)fn<ReportInt>(0x30f9a0)(report,at<I32>(state,0x480),0);
  else if(reward==0x5c75){void*row=fn<void*(*)(void*)>(0x22c2c0)(at<void*>(state,0x490));fn<ReportInt>(0x30f950)(report,at<U16>(row,0x4e),0);}
  int scenic=fn<int(*)(void*)>(0x22b9c0)(at<void*>(state,0x490));fn<ReportInt>(0x30fa60)(report,scenic,0);
  fn<ReportPtr>(0x30fcb0)(report,at<U8*>(state,0x490)+0x10,0);
  fn<ReportInt>(0x30fdb0)(report,at<I32>(state,0x49c),0);
  void*built=vf<void*(*)(void*,int)>(report,8)(report,reward);
  void*notifications=fn<void*(*)()>(0x1970e0)();
  fn<AddNotice>(0x197e50)(notifications,recipient,0x1000,game+0x139f770,built);
 }
 ++at<I32>(state,0x470);
}
extern "C" __declspec(dllexport) void SceneryMovieTick(U8*state){
 if(auto*r=movie_record(state)){
  if(r->queued||!fn<int(*)(void*)>(0x572670)(state))return;
  U8*manager=fn<U8*(*)()>(0x11040)();
  // Do not race any unrelated pending state command. #5 no longer queues the
  // scenic discovery report screen, but another system may still own this slot.
  if(!manager||at<UPTR>(manager,0x30))return;
  void*parent=at<void*>(state,0x478);r->queued=true;
  fn<One>(0x124a0)(manager);
  if(parent)fn<One>(0x5746f0)(parent);
  if(core->Log)core->Log("Scenery R4.4: scenic camera and individual discovery report skipped; history/result preserved.");
  return;
 }
 // CSearchSceneryMovieState owns the camera/presentation; the child
 // CSearchDiscoveryState hook alone cannot skip this parent state.
 // Never abandon a camera move already in progress: let native cleanup finish.
 if(!core->Readable(state,0x580)||at<U8*>(state,0)!=game+0x13d5f68||
    at<int>(state,0x470)!=0||!known_module()||
    !at<U8>(module,0x19e692)||!at<U8>(module,0x105002)){
  fn<One>(0x423d00)(state);return;
 }
 if(!fn<int(*)(void*)>(0x572670)(state))return; // Must own the top state.
 // Use the same validity checks as phase 0 before the native reward report.
 if(!fn<int(*)(void*)>(0x331780)(at<void*>(state,0x490))||
    !fn<int(*)(void*)>(0x331780)(at<void*>(state,0x488))){
  fn<One>(0x423d00)(state);return;
 }
 MovieSkip*record=0;for(auto&r:movie_skips)if(!r.state){record=&r;break;}
 if(!record){fn<One>(0x423d00)(state);return;}
 *record={state,false};
 // Start from the native result phase, but reproduce it without the final
 // CReportWaitState submission. The report text/history preparation still runs;
 // only the individual discovery report screen is omitted.
 at<int>(state,0x470)=2;
 SceneryResultSilent(state);
 // No scenic discovery report state is queued. Finish on the next top-state tick.
 // Completion keeps native phase 3's pop/signal order; Exit/destruction stay native.
}
static const char*SH_MOD=
#ifdef SAN14_DOMESTIC_UI_BRIDGE
 "SAN14_SettingsHelpScenic_domestic.dll";
#else
 "SAN14_SettingsHelpScenic.dll";
#endif
static int install(const char*id,const U32*sites,const U32*originals,void*const*targets,U32 count,const char*what,const wchar_t*effect){
 S14_Hook hooks[4]={};
 for(U32 i=0;i<count;++i){hooks[i].target=game+sites[i];hooks[i].length=8;UPTR old=(UPTR)(game+originals[i]),next=(UPTR)targets[i];memcpy(hooks[i].expected,&old,8);memcpy(hooks[i].replacement,&next,8);}
 int ok=core->InstallHooks(id,hooks,count);
 if(!ok){ // R43: 어느 vtable 칸이 다른지 찾아서 창
  char where[160],ex[64],ac[64];where[0]=ex[0]=ac[0]=0;r39::scat(where,sizeof where,what);bool found=false;
  for(U32 i=0;i<count&&!found;++i){if(!core->Readable(hooks[i].target,8)||memcmp(hooks[i].target,hooks[i].expected,8)){found=true;r39::scat(where,sizeof where," @ vtable slot exe+0x");r39::shex(where,sizeof where,sites[i]);r39::sbytes(ex,sizeof ex,hooks[i].expected,8);if(core->Readable(hooks[i].target,8))r39::sbytes(ac,sizeof ac,hooks[i].target,8);else r39::scat(ac,sizeof ac,"unreadable");}}
  if(!found){r39::scat(where,sizeof where," (slots match; InstallHooks refused)");r39::scat(ex,sizeof ex,"-");r39::scat(ac,sizeof ac,"-");}
  r39::fail(0,SH_MOD,"vtable",where,ex,ac,effect);
 }
 return ok;
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,0,SH_MOD))return 0;
 core=api;game=api->resolved->image;
 // Independent transactions: a foreign tooltip hook cannot disable discovery
 // skip, and a foreign discovery hook is never overwritten.
 const U32 movieSites[]={0x13d5f70,0x13d5f78,0x13d5f90},movieOriginals[]={0x423cd0,0x423c20,0x423d00};
 void*movieTargets[]={(void*)&SceneryMovieEnter,(void*)&SceneryMovieExit,(void*)&SceneryMovieTick};
 int movie=install("SAN14CE.SceneryMovieSkip",movieSites,movieOriginals,movieTargets,3,"scenery discovery movie (CSceneryMovie vtable 0x13d5f70/0x13d5f78/0x13d5f90)",L"명승 발견 영상 생략이 안 됨 → 원래대로 재생");
 const U32 childSites[]={0x144a278},childOriginals[]={0x724e40};void*childTargets[]={(void*)&DiscoveryEnter};
 int child=install("SAN14CE.SceneryDiscoverySkip",childSites,childOriginals,childTargets,1,"scenery discovery dialog (vtable 0x144a278)",L"명승 발견 대화 생략이 안 됨 → 원래대로");
 const U32 uiSites[]={0x14aa1e8,0x14aa1b0,0x14aa1c8},uiOriginals[]={0x8ca1c0,0x8ff9c0,0x8cceb0};
 void*uiTargets[]={(void*)&SettingsTick,(void*)&SettingsRelease,(void*)&SettingsHide};
 int ui=install("SAN14CE.SettingsHelp",uiSites,uiOriginals,uiTargets,3,"settings help tooltip (vtable 0x14aa1e8/0x14aa1b0/0x14aa1c8)",L"설정 창 항목 도움말(말풍선)이 안 뜸");
 if(core->Log){
  core->Log(movie?"Scenery R4.3: discovery movie skip installed.":"Scenery R4.3: discovery movie skip NOT installed (conflict/version). Other patches preserved.");
  core->Log(child?"Scenery R4.3: discovery dialog skip installed.":"Scenery R4.3: discovery dialog hook unavailable.");
  core->Log(ui?"Scenery R4.3: settings help hooks installed independently.":"Scenery R4.3: settings help hook unavailable; scenic skip is independent.");
 }
 // A successful group holds pointers into this DLL: keep it loaded even if a
 // different group was rejected. Return failure only if no group was installed.
 return movie||child||ui;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
