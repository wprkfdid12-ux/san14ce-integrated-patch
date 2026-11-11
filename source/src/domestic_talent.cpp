#include "common.h"
#include "r39_record.h"
// R8 (2026-10-03, Claude): PLACEMENT DISABLED.
//  The game function this module used as "assign officer to area" (game+0x1f22f0) is in
//  fact the 탐색 (search) command — the convenience module's auto-personnel search step
//  calls exactly the same pair (0x1f0ba0 "can search" / 0x1f22f0 "search"). So every
//  "placement" sent an officer to search a city and spent an order slip, even with
//  auto-personnel OFF. Until the real assignment function is identified from the game
//  binary, this module issues NO commands. It keeps only the standby pause for
//  DomesticCE's 3-second job (fog-compatible main-map chaining, reviewed in R3/R4).
// R9 (R17 패키지): 매 프레임 하던 확인(메모리 확인 3~4회 = 시스템콜)을 0.1초마다 한 번으로 줄였다.
//  대기 판정 기준은 5초 이상이라 0.1초 간격으로도 똑같이 동작한다. 하루 진행 간격 측정(day_ms)도 같은 방식.
// R3 (2026-10-03, Claude):
//  * Fog compatibility: the fog engine verifies at startup that the main-map state
//    slot (vtable 0x13d6368) still holds the native 0x44bb40 and aborts entirely if
//    not. When fog is configured we therefore never take the slot first; we wait
//    until fog has installed its own handler there and then chain on top of it.
//  * Frame cost: the 6000-officer scan is spread over frames (200 per frame) and an
//    area is re-evaluated at most every 30 s instead of on every in-game day.
static const S14_CoreAPI*core;static U8*game,*domestic;static bool rejected;static int(*get_enabled)();
// The main-map state handler takes (state, arg) — fog's replacement forwards rdx to the native.
// All four register arguments are passed through unchanged.
using Tick=void(*)(void*,void*,void*,void*);
static const U32 SLOT=0x13d9458,NATIVE=0x44ed30;
static volatile Tick next_tick;static volatile LONG installed;
// R4: work only while game time is flowing. In turn standby (date frozen) this
// module and DomesticCE's 3-second job do nothing, so the frame cost there is the same
// as without domestic automation. Placement rule is unchanged.
static const U32 DATE_OFFSET=0x34;          // world+0x34: year(int16), month, day (rollback EXE)
static const uint64_t STANDBY_MIN=5000;     // no date change for max(5 s, 3 days of real time) = standby
static U32 last_date;static uint64_t date_changed_at,pause_written,day_ms;static bool date_seen;
// DomesticCE is only ever paused after the date format has been confirmed in play
// (several consecutive one-day steps). Until then it runs exactly as before.
static unsigned day_steps;static bool date_trusted;
static bool resolve(){
 if(domestic)return true;if(rejected)return false;HMODULE m=GetModuleHandleW(L"SAN14_DomesticCE.dll");if(!m)return false;
 static const U8 sha[]={0xca,0xab,0x6a,0x7c,0xaf,0xc2,0x05,0x9e,0x72,0xb8,0x9b,0x86,0x7c,0x15,0x77,0xbc,0xeb,0x2f,0xc8,0x29,0xf8,0xb9,0x4d,0xc6,0xaf,0x77,0x40,0x79,0x01,0x06,0x64,0x24} /* R43 DomesticCE(새 판 바이트 교체) */;
 if(!module_sha(m,sha)){rejected=true;return false;}get_enabled=(int(*)())GetProcAddress(m,"DomesticGetEnabled");if(!get_enabled){rejected=true;return false;}domestic=(U8*)m;return true;
}
static long long date_ordinal(U32 d){return (long long)(d&0xffff)*400+((d>>16)&0xff)*32+(d>>24);}
// DomesticCE skips its 3-second job while now < its pause stamp (the stamp its own
// "모드 처리 재정리" button uses). In standby we hold that pause; when time flows again we
// release only our own pause, never a maintenance pause DomesticCE set itself.
static void hold_domestic(uint64_t now,bool standby){
 volatile uint64_t*stamp=(volatile uint64_t*)(domestic+0x2c050);uint64_t cur=__atomic_load_n(stamp,__ATOMIC_ACQUIRE);
 if(standby){
  bool ours_or_free=cur==pause_written||cur<=now;
  if(ours_or_free&&cur<now+50000){uint64_t want=now+60000;if(__atomic_compare_exchange_n(stamp,&cur,want,false,__ATOMIC_SEQ_CST,__ATOMIC_ACQUIRE))pause_written=want;}
 }else if(pause_written){
  uint64_t mine=pause_written;__atomic_compare_exchange_n(stamp,&mine,(uint64_t)0,false,__ATOMIC_SEQ_CST,__ATOMIC_ACQUIRE);pause_written=0;
 }
}
extern "C" __declspec(dllexport) void DomesticTalentTick(U8*state,void*a2,void*a3,void*a4){
 // Native (or the fog engine's chained) state progression always runs first, with every argument.
 Tick n=next_tick;if(!n)n=fn<Tick>(game,NATIVE);n(state,a2,a3,a4);
 if(at<U8*>(state,0)!=game+0x13d9430||at<int>(state,0x470)!=2)return;
 static uint64_t last_work;uint64_t tick=GetTickCount64();if(tick-last_work<100)return;last_work=tick;   // R9: 0.1초에 한 번
 U8*manager=game+0x1b65100;int count=at<int>(manager,0x10);U8**stack=at<U8**>(manager,0x20);
 if(count<1||count>128||at<void*>(manager,0x30)||!core->Readable(stack,size_t(count)*8)||stack[count-1]!=state)return;
 if(!resolve())return;
 U8*db=at<U8*>(game,0x2780f60);if(!db||!core->Readable(db,0x871f0))return;U8*world=at<U8*>(db,0x871e8);if(!world||!core->Readable(world,0x454))return;
 uint64_t now=tick;U32 date=at<U32>(world,DATE_OFFSET);
 if(!date_seen){date_seen=true;last_date=date;date_changed_at=0;}
 else if(date!=last_date){
  long long step=date_ordinal(date)-date_ordinal(last_date);last_date=date;
  if(step>0&&step<=40){
   if(date_changed_at){uint64_t gap=now-date_changed_at;day_ms=day_ms?(day_ms*3+gap)/4:gap;}
   if(step<=3){if(++day_steps>=3)date_trusted=true;}else if(step!=19)day_steps=0; // 19 = year rollover
   date_changed_at=now;
  }else date_changed_at=0;
 }
 uint64_t standby_after=day_ms*3>STANDBY_MIN?day_ms*3:STANDBY_MIN;
 bool standby=!date_changed_at||now-date_changed_at>=standby_after;
 if(date_trusted)hold_domestic(now,standby);
 // No officer placement: see the R8 note at the top of this file.
}
static void say(const char*s){if(core&&core->Log)core->Log(s);}
static bool swap_slot(Tick expected,Tick chained){
 // The fog engine later unlocks/writes/relocks this same page (save loads, its own
 // start-up tail). Re-locking it here could fault a fog write in progress, so the
 // page is intentionally left writable after the atomic exchange.
 U8*slot=game+SLOT;DWORD old=0;if(!VirtualProtect(slot,8,PAGE_READWRITE,&old))return false;
 next_tick=chained;LONG64 seen=InterlockedCompareExchange64((volatile LONG64*)slot,(LONG64)(UPTR)&DomesticTalentTick,(LONG64)(UPTR)expected);
 FlushInstructionCache(GetCurrentProcess(),slot,8);
 if(seen!=(LONG64)(UPTR)expected){next_tick=nullptr;return false;}
 InterlockedExchange(&installed,1);return true;
}
static void game_path(wchar_t*out,const wchar_t*leaf){
 DWORD n=GetModuleFileNameW(nullptr,out,1024);if(!n||n>=900){out[0]=0;return;}
 while(n&&out[n-1]!=L'\\'&&out[n-1]!=L'/')n--;wcscpy(out+n,leaf);
}
static bool exists(const wchar_t*leaf){wchar_t p[1024];game_path(p,leaf);return p[0]&&GetFileAttributesW(p)!=INVALID_FILE_ATTRIBUTES;}
static bool fog_configured(){
 wchar_t ini[1024];game_path(ini,L"SAN14Mods\\modules\\FogFieldPair.ini");
 return ini[0]&&GetPrivateProfileIntW(L"Mod",L"Enabled",0,ini)==1&&exists(L"SAN14_FogTrial.dll")&&exists(L"SAN14Mods\\Fog121\\SAN14_FogEngine121.dll");
}
static UPTR image_size(HMODULE m){U8*b=(U8*)m;U32 pe=at<U32>(b,0x3c);return at<U32>(b,pe+0x50);}
// Known Fog 1.2.1 builds (original and weather-collector fix share one data layout).
static bool fog_known(HMODULE fog){
 static const U8 fog16[]={0xc2,0xa8,0xf6,0x26,0x37,0x95,0x17,0x54,0x4e,0x8b,0x65,0xa6,0x3b,0x8e,0x54,0x03,0xe1,0x91,0x34,0xaf,0xa0,0x3f,0xae,0x2c,0xd5,0x69,0x35,0xe9,0x33,0x3b,0xd3,0x71}; // 전장의 안개 1.6 (CE 1.0.1)
 static const U8 fog16fx[]={0x4a,0xae,0x2e,0xd2,0xfe,0xd6,0x9d,0x86,0xf5,0x92,0x44,0xb8,0x75,0xb4,0xdb,0x9d,0xef,0x6e,0xf6,0x07,0x1e,0x8f,0x6d,0x2a,0xc4,0xdd,0xe5,0xe4,0x9a,0xa7,0x09,0x31}; // 1.6 + R24 추가 기능
 return module_sha(fog,fog16)||module_sha(fog,fog16fx);
}
static DWORD WINAPI wait_for_fog(LPVOID){
 Tick native=fn<Tick>(game,NATIVE);int known=-1;unsigned failed_for=0,owned_for=0,native_after_apply=0;bool told=false;
 for(;;){
  Sleep(500);
  Tick current=__atomic_load_n((Tick*)(game+SLOT),__ATOMIC_ACQUIRE);
  if((UPTR)current==(UPTR)&DomesticTalentTick)return 0;
  HMODULE fog=GetModuleHandleW(L"SAN14_FogEngine121.dll");
  if(!fog){if(!told){told=true;say("DomesticTalent R3: fog configured but its engine is not loaded yet; talent placement waits for fog.");}continue;}
  if(known<0)known=fog_known(fog)?1:0;
  bool applied=known==1&&at<U8>((U8*)fog,0x2da364 /*core_applied(1.6)*/)!=0;
  UPTR lo=(UPTR)fog,hi=lo+image_size(fog);
  if((UPTR)current>=lo&&(UPTR)current<hi){
   // Fog owns the slot. Let fog finish the rest of its start-up (it keeps writing
   // this vtable page for a moment after the swap) before chaining: 5 s stable.
   if(known==1&&!applied){owned_for=0;continue;}
   if(++owned_for<10)continue;
   say(swap_slot(current,current)?"DomesticTalent R3: chained after the fog engine's main-map handler (fog left intact).":"DomesticTalent R3: slot changed during chaining; nothing overwritten.");return 0;
  }
  owned_for=0;
  if(current!=native){say("DomesticTalent R3: main-map slot owned by another module; talent placement left inactive.");
   char ac[48];ac[0]=0;r39::scat(ac,sizeof ac,"slot=0x");r39::shex(ac,sizeof ac,(unsigned long long)(UPTR)current);
   r39::fail(22,"SAN14_DomesticTalent.dll","slot_owner","wait_for_fog: main-map tick vtable slot (fog configured)","slot = native fn or fog engine fn","another module's fn (not fog, not native)",L"내정 인재 배치(22번 DomesticTalent)가 동작 안 함 → 다른 모드가 메인 맵 처리 칸을 차지함");(void)ac;return 0;}
  if(known!=1)continue; // unknown fog build: only chain after it, never pre-empt it
  if(applied){ // fog started but does not use this slot (e.g. that fog feature off)
   if(++native_after_apply>=10){say(swap_slot(native,native)?"DomesticTalent R3: fog started without this slot; installed on the native handler.":"DomesticTalent R3: slot changed; nothing overwritten.");return 0;}
   continue;
  }
  native_after_apply=0;
  // Fog start-up attempted (report begun) but never completed: it failed for another reason.
  if(at<U32>((U8*)fog,0xcf5c80 /*startup_length(1.6)*/)!=0){
   if(++failed_for>=40){say(swap_slot(native,native)?"DomesticTalent R3: fog startup did not complete; installed on the native handler.":"DomesticTalent R3: slot changed; nothing overwritten.");return 0;}
  }else failed_for=0;
 }
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*a){
 if(!r39::check_api(a,22,"SAN14_DomesticTalent.dll"))return 0;core=a;game=a->resolved->image;
 if(fog_configured()){
  // Do NOT touch the slot now: fog checks it later and would abort.
  HANDLE t=CreateThread(nullptr,0,wait_for_fog,nullptr,0,nullptr);if(!t){say("DomesticTalent R3: could not start fog-compatible installer.");r39::fail(22,"SAN14_DomesticTalent.dll","thread","SAN14_ModInitV1 -> CreateThread(wait_for_fog)","thread handle","NULL",L"내정 인재 배치(22번 DomesticTalent)가 이번 실행에서 동작 안 함");return 0;}
  CloseHandle(t);say("DomesticTalent R3: fog configured; waiting to chain after fog startup.");return 1;
 }
 S14_Hook h{};h.target=game+SLOT;h.length=8;
 UPTR old=(UPTR)(game+NATIVE),next=(UPTR)DomesticTalentTick;memcpy(h.expected,&old,8);memcpy(h.replacement,&next,8);
 next_tick=fn<Tick>(game,NATIVE);
 int ok=core->InstallHooks("SAN14CE.DomesticTalent",&h,1);if(ok)InterlockedExchange(&installed,1);
 else{char ex[40],ac[40];ex[0]=ac[0]=0;r39::sbytes(ex,sizeof ex,h.expected,8);if(core->Readable(h.target,8))r39::sbytes(ac,sizeof ac,h.target,8);else r39::scat(ac,sizeof ac,"unreadable");
  char where[80];where[0]=0;r39::scat(where,sizeof where,"main-map tick vtable slot exe+0x");r39::shex(where,sizeof where,SLOT);r39::scat(where,sizeof where," (native exe+0x");r39::shex(where,sizeof where,NATIVE);r39::scat(where,sizeof where,")");
  r39::fail(22,"SAN14_DomesticTalent.dll","vtable",where,ex,ac,L"내정 인재 배치(22번 DomesticTalent)가 동작 안 함 → 다른 모드가 메인 맵 처리 칸을 먼저 바꿈(안개 모드면 그 뒤에 이어 붙어야 함)");}
 return ok;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
