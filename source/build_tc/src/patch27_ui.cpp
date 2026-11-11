#include "patch27.h"
#include "module_hooks.h"
static U8*common27;static bool attached,failed,configured;static bool domestic_state_ok();static wchar_t options[1024];static DWORD root_len;
// R21: 설정 창 6/6 의 4·5번째 줄 — 18번(경험치)·24번(수여 능력치) 데이터 파일 전환. SAN14Mods\DataToggle 사본을 게임 데이터 파일에 복사하며 재시작 뒤 적용된다.
struct DataToggle{const wchar_t*label;const wchar_t*target;const wchar_t*on;const wchar_t*off;int state;bool pending;}; // state: -1 미설치, 0 OFF, 1 ON, 2 확인 필요
static DataToggle toggles[2]={
 {L"경험치 성장 억제",L"0000\\experience_data_exce.bin",L"SAN14Mods\\DataToggle\\experience_modified.bin",L"SAN14Mods\\DataToggle\\experience_original.bin",-1,false},
 {L"수여 능력치 미적용",L"0010_KO\\fixdataexce.s14",L"SAN14Mods\\DataToggle\\fixdata_nostat.s14",L"SAN14Mods\\DataToggle\\fixdata_original.s14",-1,false}};
static wchar_t*game_path(wchar_t*buf,const wchar_t*rel){memcpy(buf,options,root_len*sizeof(wchar_t));wcscpy(buf+root_len,rel);return buf;}
static U8*read_all(const wchar_t*rel,DWORD*len){
 wchar_t path[1200];HANDLE f=CreateFileW(game_path(path,rel),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return nullptr;
 LARGE_INTEGER size;U8*buf=nullptr;DWORD n=0;
 if(GetFileSizeEx(f,&size)&&size.QuadPart>0&&size.QuadPart<4000000){buf=(U8*)VirtualAlloc(nullptr,(SIZE_T)size.QuadPart,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(buf&&(!ReadFile(f,buf,(DWORD)size.QuadPart,&n,nullptr)||n!=(DWORD)size.QuadPart)){VirtualFree(buf,0,MEM_RELEASE);buf=nullptr;}}
 CloseHandle(f);if(buf)*len=n;return buf;
}
static bool same_file(const wchar_t*a,const wchar_t*b){
 DWORD la=0,lb=0;U8*pa=read_all(a,&la);U8*pb=pa?read_all(b,&lb):nullptr;bool same=pa&&pb&&la==lb&&!memcmp(pa,pb,la);
 if(pa)VirtualFree(pa,0,MEM_RELEASE);if(pb)VirtualFree(pb,0,MEM_RELEASE);return same;
}
static void refresh_toggles(){
 for(unsigned i=0;i<2;i++){
  DataToggle&t=toggles[i];wchar_t path[1200];
  if(GetFileAttributesW(game_path(path,t.on))==INVALID_FILE_ATTRIBUTES||GetFileAttributesW(game_path(path,t.off))==INVALID_FILE_ATTRIBUTES){t.state=-1;continue;}
  t.state=same_file(t.target,t.on)?1:same_file(t.target,t.off)?0:2;
 }
}
static void flip_toggle(unsigned i){
 DataToggle&t=toggles[i];if(t.state!=0&&t.state!=1)return;wchar_t src[1200],dst[1200];
 game_path(src,t.state?t.off:t.on);game_path(dst,t.target);
 if(CopyFileW(src,dst,FALSE)){t.pending=true;if(core27->Log)core27->Log(i?"Patch27 R21: fixdataexce.s14 switched; takes effect after the game restarts.":"Patch27 R21: experience_data_exce.bin switched; takes effect after the game restarts.");}
 else if(core27->Log)core27->Log("Patch27 R21: data file copy failed (file in use or write denied); nothing changed.");
 refresh_toggles();
}
using Render=void(*)(U8*);using Click=UPTR(*)(U8*,U8*);static Render old_render;static Click old_click;
static const wchar_t*keys[]={L"RemoteRecruitment",L"DeployedPosts",L"BattleDeathNotices"};
static const wchar_t*labels[]={L"타 세력 도시 재야 등용",L"행동 중 무장 관직 수여",L"전사 일러스트·메시지·기록"};
extern "C" bool Patch27LoadOptions();
// R22: 25번을 끈 채 1번(설정 창)만 켠 경우 설치기가 Patch27Features=0 을 써서 게임 기능 훅 없이 설정 창 5·6쪽만 붙인다.
extern "C" bool Patch27FeaturesOn(){return !configured||GetPrivateProfileIntW(L"Features",L"Patch27Features",1,options)!=0;}
extern "C" bool Patch27LoadOptions(){
 if(configured)return true;DWORD n=GetModuleFileNameW(nullptr,options,1024);if(!n||n>=960)return false;
 while(n&&options[n-1]!=L'\\'&&options[n-1]!=L'/')n--;if(!n)return false;root_len=n;
 wcscpy(options+n,L"SAN14Mods");CreateDirectoryW(options,nullptr);
 wcscpy(options+n,L"SAN14Mods\\Patch27");CreateDirectoryW(options,nullptr);
 wcscpy(options+n,L"SAN14Mods\\Patch27\\Options.ini");
 // Main-menu name registration is unconditional; ignore the obsolete switch.
 U32 flags=ExtendedNames;for(unsigned i=0;i<3;i++)if(GetPrivateProfileIntW(L"Features",keys[i],1,options))flags|=1u<<i;
 InterlockedExchange(&flags27,(LONG)flags);configured=true;refresh_toggles();return true;
}
static U8*widget(U8*owner,int id){return fn<U8*(*)(void*,int)>(game27,0x92c390)(owner+0x130,id);}
static void label(U8*w,const wchar_t*s){if(w)fn<void(*)(void*,const wchar_t*)>(game27,0x85c250)(w,s);}
static void visible(U8*w,bool yes){if(w)fn<void(*)(void*,int)>(game27,0x873ab0)(w,yes);}
static void enable(U8*w,bool yes){if(w)fn<void(*)(void*,int)>(game27,0x14020)(w,yes);}
static bool owner_ok(U8*owner){return common27&&owner&&core27->Readable(owner,0x148)&&at<U8*>(common27,0x1c10f0)==owner&&at<U8>(common27,0x1c1100)&&at<int>(owner,0x140)==0x52;}
// R22: 설정 창 줄 배치(통합편의 ic_layout: 논공 동시 줄 제거·명승 두 줄 합침 → IC 1~4쪽, 5쪽=22번 내정, 6쪽=25번·18·24번)
//  - 4쪽 첫 줄 '문전박대' = 외교 메뉴 버튼 보이기/숨기기(San14ModSettings.ini [Convenience] AudienceButton, IC 상태 바이트).
//    숨길 때 기능이 켜져 있으면 통합편의 원래 동작으로 기능도 끈다. 기능 단계(OFF/ON/항상)는 외교 메뉴 버튼이 맡는다.
//  - 1쪽 '명승 연출생략' = 명승 연출 생략 + 명승 암전 제거를 함께 바꾼다.
//  - 5쪽 = 22번 내정 스위치 4개(모든 내정 / AI 명령서 무제한 / 최소 인구 1만 / 저돌) + 모드 처리 재정리.
static const U32 IC_HSTATE=0x1ee3a0,IC_INI=0x1c1150,IC_STATE=0x105000,IC_AVAIL=0x19e690,IC_CALLBACK=0x19e6a0,IC_AUD_AVAIL=0x19e999,IC_AUD_GATE=0x19e9a8;
static const U32 DC_FLAGS=0x2d580,DC_RESET_PENDING=0x2c058;
static const wchar_t*ic_ini(){return (const wchar_t*)(common27+IC_INI);}
static bool aud_visible(){
 U8 s=at<U8>(common27,IC_HSTATE);
 if(!s){s=GetPrivateProfileIntW(L"Convenience",L"AudienceButton",1,ic_ini())?2:1;at<U8>(common27,IC_HSTATE)=s;}
 return s==2;
}
// R24: 4쪽 다섯째 줄 = 12번 안개 미니맵 보이기/숨기기 (SAN14_FogMinimapUI.ini [Minimap] Hidden). 안개 엔진이 0.5초 안에 읽어 바로 반영.
static bool fog_loaded(){return GetModuleHandleW(L"SAN14_FogEngine121.dll")!=nullptr;}
static bool weather_on(){wchar_t p[1200];return GetPrivateProfileIntW(L"Weather",L"Enabled",1,game_path(p,L"SAN14_FogMinimapUI.ini"))!=0;}
static bool minimap_shown(){wchar_t p[1200];return GetPrivateProfileIntW(L"Minimap",L"Hidden",0,game_path(p,L"SAN14_FogMinimapUI.ini"))==0;}
static void scenic_sync(){
 if(!at<U8>(common27,IC_AVAIL+3))return;
 U8 want=at<U8>(common27,IC_STATE+2);if(at<U8>(common27,IC_STATE+3)==want)return;
 at<U8>(common27,IC_STATE+3)=want;
 void(*cb)(U32)=at<void(*)(U32)>(common27,IC_CALLBACK+3*8);if(cb)cb(want);
 if(ic_ini()[0])WritePrivateProfileStringW(L"Convenience",L"ScenicNoFade",want?L"1":L"0",ic_ini());
}
static U8*domestic27(){HMODULE m=GetModuleHandleW(L"SAN14_DomesticCE.dll");return domestic_state_ok()&&m?(U8*)m:nullptr;}
static const wchar_t*dom_keys[]={L"AllDomestic",L"AiOrders",L"PopulationFloor"};
static const unsigned dom_slot[]={1,3,2};
static const wchar_t*dom_labels[]={L"모든 내정 자동",L"AI 명령서 무제한",L"최소 인구 1만 유지",L"저돌 수정",L"모드 처리 재정리"};
static int dom_state(U8*dm,unsigned row){
 if(row<3)return at<volatile LONG>(dm,DC_FLAGS+4*dom_slot[row])!=0;
 if(row==3){auto get=(int(*)())GetProcAddress((HMODULE)dm,"RecklessGetEnabled");return get?get()!=0:-1;}
 return at<U8>(dm,DC_RESET_PENDING)!=0;
}
static void dom_click(U8*dm,unsigned row){
 if(row<3){
  LONG want=dom_state(dm,row)?0:1;wchar_t path[1200];game_path(path,L"SAN14_DomesticCE.ini");
  if(WritePrivateProfileStringW(L"Automation",dom_keys[row],want?L"1":L"0",path))InterlockedExchange(&at<volatile LONG>(dm,DC_FLAGS+4*dom_slot[row]),want);
  else if(core27->Log)core27->Log("Patch27 R22: SAN14_DomesticCE.ini write failed; domestic switch unchanged.");
  return;
 }
 if(row==3){auto get=(int(*)())GetProcAddress((HMODULE)dm,"RecklessGetEnabled");auto set=(int(*)(int))GetProcAddress((HMODULE)dm,"RecklessSetEnabled");if(get&&set)set(get()?0:1);return;}
 auto reset=(int(*)())GetProcAddress((HMODULE)dm,"DomesticResetRuntime");if(reset)reset();
}
extern "C" __declspec(dllexport) void Patch27Render(U8*owner){
 old_render(owner);if(!owner_ok(owner))return;int page=at<int>(common27,0x1c1140);if(page<0||page>6)return;
 static const wchar_t*pages[]={L"다음 (1/7)",L"다음 (2/7)",L"다음 (3/7)",L"다음 (4/7)",L"다음 (5/7)",L"다음 (6/7)",L"처음 (7/7)"};
 label(widget(owner,0x3c),pages[page]);
 if(page==3&&at<U8>(common27,IC_AUD_AVAIL)){U8*button=widget(owner,0x18);label(button,aud_visible()?L"ON":L"OFF");enable(button,true);}
 if(page==3){U8*name=widget(owner,0x14+8*4),*button=widget(owner,0x18+8*4);bool show=fog_loaded();visible(name,show);visible(button,show);enable(name,false);enable(button,show);
  if(show){label(name,L"미니맵 표시");label(button,minimap_shown()?L"ON":L"OFF");}}
 if(page==4){
  U8*dm=domestic27();
  for(unsigned row=0;row<5;row++){
   U8*name=widget(owner,0x14+8*row),*button=widget(owner,0x18+8*row);int st=dm?dom_state(dm,row):-1;bool show=st>=0;
   visible(name,show);visible(button,show);enable(name,false);enable(button,show);
   if(show){label(name,dom_labels[row]);label(button,row==4?(st?L"요청됨":L"실행"):(st?L"ON":L"OFF"));}
  }
  return;
 }
 if(page==6){ // R25: 7쪽 = 12번 안개 날씨 ON/OFF (바로 적용)
  for(unsigned row=0;row<5;row++){
   U8*name=widget(owner,0x14+8*row),*button=widget(owner,0x18+8*row);bool show=row==0&&fog_loaded();
   visible(name,show);visible(button,show);enable(name,false);enable(button,show);
   if(show){label(name,L"날씨 (12번 안개)");label(button,weather_on()?L"ON":L"OFF");}
  }
  return;
 }
 if(page!=5)return;
 for(unsigned row=0;row<5;row++){
  U8*name=widget(owner,0x14+8*row),*button=widget(owner,0x18+8*row);
  if(row>=3){
   const DataToggle&t=toggles[row-3];bool show=t.state>=0;visible(name,show);visible(button,show);enable(name,false);enable(button,show&&t.state!=2);
   if(show){label(name,t.label);label(button,t.state==2?L"확인 필요":t.pending?(t.state?L"ON (재시작)":L"OFF (재시작)"):t.state?L"ON":L"OFF");}
   continue;
  }
  bool show=true;
  visible(name,show);visible(button,show);enable(name,false);enable(button,show&&(installed27&(1u<<row)));
  if(show){label(name,labels[row]);label(button,(installed27&(1u<<row))?(enabled27(1u<<row)?L"ON":L"OFF"):L"사용 불가");}
 }
}
extern "C" __declspec(dllexport) UPTR Patch27Click(U8*owner,U8*control){
 if(!owner_ok(owner)||!control||!core27->Readable(control,0x88))return old_click(owner,control);
 int page=at<int>(common27,0x1c1140);
 // Native control identity, dialog visibility, enabled state and activation test.
 int id=at<int>(control,0x80);
 if(id==0x40)return old_click(owner,control); // Native close and cleanup.
 if(page==0&&id==0x18+2*8){UPTR r=old_click(owner,control);scenic_sync();Patch27Render(owner);return r;}
 if(page<3||(page==3&&id!=0x18&&id!=0x18+8*4))return old_click(owner,control);
 if(widget(owner,id)!=control||(at<U8>(control,0x40)&7)!=7||!fn<U8(*)(void*)>(common27,0x493d0)(owner)||!fn<U8(*)(void*,int)>(common27,0x280c0)(control,id))return 0;
 uint64_t now=GetTickCount64(),last=at<uint64_t>(common27,0x1c1148);if(last&&now-last<350)return 0;
 if(page==3&&id==0x18+8*4){ // 미니맵 줄
  if(!fog_loaded())return 0;
  wchar_t p[1200];bool hide=minimap_shown();
  if(!WritePrivateProfileStringW(L"Minimap",L"Hidden",hide?L"1":L"0",game_path(p,L"SAN14_FogMinimapUI.ini"))){if(core27->Log)core27->Log("Patch27 R24: SAN14_FogMinimapUI.ini write failed; minimap unchanged.");return 0;}
  at<uint64_t>(common27,0x1c1148)=now;
  if(core27->Log)core27->Log(hide?"Patch27 R24: fog minimap hidden.":"Patch27 R24: fog minimap shown.");
  Patch27Render(owner);return 0;
 }
 if(page==3){ // 문전박대 줄 = 외교 메뉴 버튼 보이기/숨기기
  if(!at<U8>(common27,IC_AUD_AVAIL))return 0;
  bool show=!aud_visible();
  if(ic_ini()[0]&&!WritePrivateProfileStringW(L"Convenience",L"AudienceButton",show?L"1":L"0",ic_ini())){if(core27->Log)core27->Log("Patch27 R22: AudienceButton write failed; diplomacy button unchanged.");return 0;}
  at<U8>(common27,IC_HSTATE)=show?2:1;
  if(!show&&at<int>(common27,IC_AUD_GATE)!=0){old_click(owner,control);} // 숨기면 기능도 끔(통합편의 원래 처리, 저장 포함)
  else at<uint64_t>(common27,0x1c1148)=now;
  if(core27->Log)core27->Log(show?"Patch27 R22: diplomacy audience button shown.":"Patch27 R22: diplomacy audience button hidden; audience gate off.");
  Patch27Render(owner);return 0;
 }
 if(id==0x3c){at<uint64_t>(common27,0x1c1148)=now;at<int>(common27,0x1c1140)=(page+1)%7;Patch27Render(owner);return 0;}
 int row=(id-0x18)/8;if(id<0x18||row<0||row>=5||id!=0x18+row*8)return 0;
 if(page==6){ // 날씨 줄
  if(row!=0||!fog_loaded())return 0;
  wchar_t p[1200];bool off=weather_on();
  if(!WritePrivateProfileStringW(L"Weather",L"Enabled",off?L"0":L"1",game_path(p,L"SAN14_FogMinimapUI.ini"))){if(core27->Log)core27->Log("Patch27 R25: SAN14_FogMinimapUI.ini write failed; weather unchanged.");return 0;}
  at<uint64_t>(common27,0x1c1148)=now;
  if(core27->Log)core27->Log(off?"Patch27 R25: fog weather off.":"Patch27 R25: fog weather on.");
  Patch27Render(owner);return 0;
 }
 if(page==4){U8*dm=domestic27();if(!dm)return 0;at<uint64_t>(common27,0x1c1148)=now;dom_click(dm,(unsigned)row);Patch27Render(owner);return 0;}
 if(row>=3){at<uint64_t>(common27,0x1c1148)=now;flip_toggle(row-3);Patch27Render(owner);return 0;}
 if(!(installed27&(1u<<row)))return 0;
 at<uint64_t>(common27,0x1c1148)=now;U32 bit=1u<<row,wanted=U32(flags27)^bit;
 if(Patch27LoadOptions()&&WritePrivateProfileStringW(L"Features",keys[row],(wanted&bit)?L"1":L"0",options))InterlockedExchange(&flags27,LONG(wanted));
 else if(core27->Log)core27->Log("Patch27: settings write failed; switch state retained.");
 Patch27Render(owner);return 0;
}
static bool old_jump(U8*site,U8*target){
 if(site[0]!=0xe9)return false;I32 d;memcpy(&d,site+1,4);U8*relay=site+5+d;
 if(relay==target)return true;
 // The audited Domestic installer uses E9 -> near FF25 relay -> handler.
 // Accept only this exact relay, with the expected audited handler address.
 const U8 indirect[]={0xff,0x25,0,0,0,0};U8*destination=nullptr;
 if(!core27->Readable(relay,14)||memcmp(relay,indirect,6))return false;
 memcpy(&destination,relay+6,8);return destination==target;
}
extern "C" void DeathLogsAttach();
// R3: the settings page is attached by a startup worker as soon as the common
// module and the Domestic menu are ready, so it no longer depends on any settings
// window callback being reached. The UI tick path is kept as a second route.
static SRWLOCK attach_lock=SRWLOCK_INIT;static int domestic_state=-1;static uint64_t domestic_wait_since;
static bool domestic_state_ok(){return domestic_state==1;}
static void say(const char*s){if(core27&&core27->Log)core27->Log(s);}
static bool dialog_closed(){return !at<U8>(common27,0x1c1100);}
static void try_attach(bool from_worker){
 if(attached||failed||!core27)return;
 if(!GetModuleHandleW(L"SAN14_IntegratedConvenience.dll"))return;
 if(!common27){common27=known_common();if(!common27){failed=true;say("Patch27 R3: SAN14_IntegratedConvenience.dll is not the supported P1 build; settings page 6 not attached.");return;}}
 if(from_worker&&!dialog_closed())return; // never rewrite the renderer while the window is open
 U8 render_expected[5]={0x40,0x53,0x55,0x56,0x57},click_expected[10]={0x40,0x53,0x56,0x48,0x81,0xec,0x88,0x01,0,0};
 HMODULE dm=GetModuleHandleW(L"SAN14_DomesticCE.dll");
 if(dm){
  if(domestic_state<0){
   static const U8 sha[]={0x46,0x85,0x36,0xc0,0xeb,0xb8,0x2a,0x8c,0x6d,0x34,0xf0,0xb3,0x2f,0x77,0xf6,0x5e,0x9b,0x5b,0xd6,0x72,0x3d,0x79,0x8a,0x2a,0x2f,0x62,0xf9,0x58,0xd7,0x58,0x9a,0x8f};
   domestic_state=module_sha(dm,sha)?1:0;domestic_wait_since=GetTickCount64();
  }
  if(!domestic_state){failed=true;say("Patch27 R3: unknown SAN14_DomesticCE.dll; settings page 6 not attached.");return;}
  bool chained=old_jump(common27+0x59100,(U8*)dm+0x23c0)&&old_jump(common27+0x297f0,(U8*)dm+0x25f0);
  if(chained){memcpy(render_expected,common27+0x59100,5);memcpy(click_expected,common27+0x297f0,10);}
  else{
   // Domestic normally links its menu during its own start-up. If it never did
   // (its own signature check failed), attach directly to the untouched renderer
   // after 30 s so page 6 still appears; Domestic rows then stay native.
   if(GetTickCount64()-domestic_wait_since<30000)return;
   if(memcmp(common27+0x59100,render_expected,5)||memcmp(common27+0x297f0,click_expected,10)){failed=true;say("Patch27 R3: settings renderer changed by another module; page 6 not attached.");return;}
   say("Patch27 R3: Domestic menu link absent; attaching page 6 to the original renderer.");
  }
 }
 if(!Patch27LoadOptions()){failed=true;return;}
 ModuleTransaction tx;Render r=(Render)tx.detour(common27+0x59100,render_expected,5,(void*)Patch27Render);Click c=(Click)tx.detour(common27+0x297f0,click_expected,10,(void*)Patch27Click);
 old_render=r;old_click=c;
 if(!tx.install()){old_render=nullptr;old_click=nullptr;failed=true;say("Patch27 R3: settings page 6 write rejected; original menu kept.");return;}
 attached=true;scenic_sync();
 U8*owner=at<U8*>(common27,0x1c10f0);if(!from_worker&&owner_ok(owner))Patch27Render(owner);
 say(from_worker?"Patch27 R3: six settings pages attached at start-up; individual persistent switches.":"Patch27 R3: six settings pages attached from the settings window.");
}
extern "C" __declspec(dllexport) void Patch27UiTick(){
 if(core27&&(installed27&BattleDeaths))DeathLogsAttach();
 if(attached||failed||!core27)return;
 AcquireSRWLockExclusive(&attach_lock);try_attach(false);ReleaseSRWLockExclusive(&attach_lock);
}
static DWORD WINAPI attach_worker(LPVOID){
 for(unsigned i=0;i<2400&&!attached&&!failed;i++){ // up to 10 minutes, every 250 ms
  AcquireSRWLockExclusive(&attach_lock);try_attach(true);ReleaseSRWLockExclusive(&attach_lock);
  if(attached||failed)break;Sleep(250);
 }
 if(!attached&&!failed)say("Patch27 R3: settings page 6 still waiting; will retry when the settings window opens.");
 return 0;
}
extern "C" void Patch27StartUiAttach(){HANDLE t=CreateThread(nullptr,0,attach_worker,nullptr,0,nullptr);if(t)CloseHandle(t);}
