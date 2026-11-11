// R39 32번: 고용무장 원본 얼굴 — 고용무장이 범용 얼굴 대신 게임 내장 '오리지널 얼굴' 167장(남 101·여 66)을 쓴다.
//  출처: 신무장 1000명 r7 패키지 .cecode 층 13 (분석 메모: 얼굴_분석메모.md). 1000명 확장 없이 우리 식으로 다시 씀.
//  공식 1.0.1 (S14_GAME_STAMP) 전용. 게임 코드 4곳(jmp 릴레이, ModCore 묶음 "SAN14CE.HiredFace", 전부 아니면 전무):
//   H1 0x331416 (고용무장 객체 생성, id 저장 직후)        → 원래 얼굴을 보고 배정
//   H2 0x33149a (같은 함수 끝, 풀 카운터 감소 뒤)         → 전체 훑기
//   H3 0x336a0e (무장 생성 큰 함수, +0x165 저장 직후)      → 배정(원래 얼굴 0)
//   H4 0x339729 (CHirePersonManager 직렬화 끝: 불러오기 성공이면) → 전체 훑기
//  배정 규칙: 성별 표에서 (무장id×31)%n 부터 돌며, 살아 있는 다른 무장이 쓰는 얼굴은 건너뜀. 다 쓰이면 2500/2501.
//  설정: SAN14Mods\modules\HiredFace.ini [HiredFace] Enabled=1. export SAN14_HiredFaceSetEnabled(int) 즉시 반영.
#include "common.h"
#include "r39_record.h"
#include "ic_after_restore.h"
static const S14_CoreAPI*core;static U8*game;static volatile long enabled=1;static unsigned assigned_total,rescans;
static const U16 kMale[101]={2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015,2016,2017,2018,2019,2020,2021,2022,2023,2024,2025,2026,2027,2028,2029,2030,2031,2032,2033,2034,2035,2036,2037,2038,2039,2040,2041,2042,2043,2044,2045,2046,2047,2048,2049,2080,2081,2082,2083,2084,2085,2086,2087,2088,2089,2100,2101,2102,2103,2104,2105,2106,2107,2108,2109,2110,2118,2119,2120,2121,2122,2123,2124,2125,2126,2127,2128,2136,2137,2138,2139,2140,2141,2142,2143,2152,2153,2154,2155,2157,2158,2159,2160,2161,2162,2163};
static const U16 kFemale[66]={2050,2051,2052,2053,2054,2055,2056,2057,2058,2059,2060,2061,2062,2063,2064,2065,2066,2067,2068,2069,2070,2071,2072,2073,2074,2075,2076,2077,2078,2079,2090,2091,2092,2093,2094,2095,2096,2097,2098,2099,2111,2112,2113,2114,2115,2116,2117,2129,2130,2131,2132,2133,2134,2135,2144,2145,2146,2147,2148,2149,2150,2151,2156,2164,2165,2166};
static inline bool ptr_ok(const U8*o){UPTR v=(UPTR)o;return v>0x10000&&v<0x7fffffff0000ull&&!(v&7);}
static U8*person(unsigned id){
 U8*db=*(U8**)(game+0x2780f60);if(!db)return nullptr;
 U8*o=*(U8**)(db+0x158+(size_t)id*8);
 if(!ptr_ok(o))return nullptr;return o; // 게임 무장 표 칸(없으면 0). VirtualQuery 안 씀(훑기마다 6001번 → 달 바뀔 때 크게 느려지던 원인)
}
// R42e 최적화(11-01 사용자 요청: 데모에서 매달 후보 생성마다 6001명 훑기·기록 줄 과다 → 얼굴 결과는 예전과 똑같이, 훑기·기록만 줄임)
//  사용 중 얼굴 표 used_cnt 는 원본 얼굴 167장(2000~2166)만 셈(U16, 상한 없음). 무장마다 표에 넣은 얼굴을 contrib 에 기억(0xffff = 없음).
//  같은 스레드에서 3ms 안에 이어 부르면(게임이 한 번에 후보 여러 명 만들 때) 표를 다시 안 만들고, 그사이 바뀐 무장(dirty 목록)만 다시 맞춤.
//  dirty 목록이 넘치거나(512) 전체 훑기(FaceRescan)가 얼굴을 바꾸면 표를 버리고 다음에 처음부터 다시 만듦.
static U16 used_cnt[167];static U16 contrib[0x1771];static U16 dirty[0x200];static U32 ndirty;static U8 isdirty[0x1771];
static bool cache_ok;static U32 cache_tid;static long long cache_q,qfreq;static unsigned builds,reuses;
static const unsigned NONE=0xffff;
static inline bool face_used(unsigned f){unsigned i=f-2000u;return i<=166u&&used_cnt[i]!=0;}
static inline void uc_inc(unsigned f){unsigned i=f-2000u;if(f!=NONE&&i<=166u)used_cnt[i]++;}
static inline void uc_dec(unsigned f){unsigned i=f-2000u;if(f!=NONE&&i<=166u&&used_cnt[i])used_cnt[i]--;}
static inline bool on_map(U8*o){unsigned st=at<U8>(o,0xe6);return st>=1&&st<=6;} // R40 A16: 1~4 무장, 5 재야, 6 포로만 얼굴을 차지(죽음·미등장은 비움)
static inline unsigned holds(U8*o){return ptr_ok(o)&&at<U16>(o,0x10)!=0&&on_map(o)?at<U16>(o,0xa6):NONE;}
static void mark_dirty(unsigned id){if(id>0x1770||isdirty[id])return;if(ndirty>=0x200){cache_ok=false;return;}dirty[ndirty++]=(U16)id;isdirty[id]=1;}
static void sync(unsigned id,unsigned f){unsigned c=contrib[id];if(f==c)return;uc_dec(c);uc_inc(f);contrib[id]=(U16)f;}
// 무장 표(db+0x158) 를 돌려줌(없으면 0). 사용 중 얼굴 표를 지금 상태로 맞춰 둠.
static U8**table_ready(){
 U8*db=*(U8**)(game+0x2780f60);if(!db){cache_ok=false;return nullptr;}
 U8**tab=(U8**)(db+0x158);
 LARGE_INTEGER q;QueryPerformanceCounter(&q);long long now=q.QuadPart;U32 tid=GetCurrentThreadId();
 if(!qfreq){LARGE_INTEGER f;QueryPerformanceFrequency(&f);qfreq=f.QuadPart>0?f.QuadPart:10000000;}
 long long dt=now-cache_q;
 if(cache_ok&&tid==cache_tid&&dt>=0&&dt<=qfreq*3/1000){
  for(U32 i=0;i<ndirty;i++){unsigned id=dirty[i];sync(id,holds(tab[id]));}
  reuses++;return tab;
 }
 memset(used_cnt,0,sizeof used_cnt);
 for(U32 i=0;i<ndirty;i++)isdirty[dirty[i]]=0;ndirty=0;
 for(unsigned id=0;id<=0x1770;id++){unsigned f=holds(tab[id]);contrib[id]=(U16)f;uc_inc(f);}
 builds++;cache_ok=true;cache_tid=tid;return tab;
}
static unsigned pick(U8*p,unsigned orig){ // used_cnt 를 맞춘 뒤 부를 것(p 자신은 빼고)
 unsigned gender=at<U8>(p,0xa9);if(gender>1)return at<U16>(p,0xa6);
 const U16*tab=gender?kFemale:kMale;unsigned n=gender?66:101;
 unsigned want=orig;
 if(!gender){unsigned i=orig-7800u;if(i<100u)want=kMale[i];}
 else{unsigned i=orig-7900u;if(i<66u)want=kFemale[i];}
 unsigned start=(at<U16>(p,0x10)*31u)%n;
 for(unsigned k=0;k<n;k++)if(tab[k]==want){start=k;break;}
 for(unsigned k=0;k<n;k++){unsigned cand=tab[(start+k)%n];if(!face_used(cand))return cand;}
 return 0x9c4+gender;
}
// 표를 맞춘 상태에서 한 명 바꾸기(자기 얼굴은 잠시 빼고 고른 뒤 새 얼굴을 표에 더함)
static unsigned repick(U8*o,unsigned face,bool counted){
 if(counted)uc_dec(face);
 unsigned f=pick(o,face);
 if(counted||f!=face)uc_inc(f);
 return f;
}
extern "C" void FaceAssign(U8*p,unsigned orig){
 if(!enabled||!ptr_ok(p))return;
 U8**tab=table_ready();if(!tab)return;
 unsigned id=at<U16>(p,0x10);bool self=false;
 if(id<=0x1770&&tab[id]==p){ // 자기 자신이 표에 있으면 표에서 잠시 뺌
  mark_dirty(id);sync(id,id&&on_map(p)?at<U16>(p,0xa6):NONE);self=true;uc_dec(contrib[id]);
 }
 unsigned f=pick(p,orig);
 if(f!=at<U16>(p,0xa6)){at<U16>(p,0xa6)=(U16)f;++assigned_total;if(assigned_total<4||!(assigned_total&0xfff))r39::counter(32,"SAN14_HiredFace.dll","assigned",assigned_total);} // 기록은 처음 3번·4096번마다
 if(self){uc_inc(contrib[id]);sync(id,holds(tab[id]));}
 else mark_dirty(id);
 LARGE_INTEGER q;QueryPerformanceCounter(&q);cache_q=q.QuadPart;
}
extern "C" void FaceRescan(){
 if(!enabled)return;unsigned changed=0;
 if(U8**tab=table_ready()){
  for(unsigned id=0;id<=0x1770;id++){
   U8*o=tab[id];if(!ptr_ok(o))continue;
   unsigned face=at<U16>(o,0xa6);
   bool fallback=(face==0x9c4||face==0x9c5)&&id-4201u<=199u; // R40 A16: hired officers stuck on the shared fallback face get a real one when a face frees up
   if(face-4201u<=199u||fallback){if(!on_map(o))continue;bool counted=at<U16>(o,0x10)!=0;unsigned f=repick(o,face,counted);if(f!=face){at<U16>(o,0xa6)=(U16)f;changed++;}}
  }
 }
 U8*db=*(U8**)(game+0x2780f60);
 if(db){for(unsigned i=0;i<0x208;i++){U8*o=*(U8**)(db+0x861a0+(size_t)i*8);if(!ptr_ok(o))continue;
  if((at<U16>(o,0x15e)&0x4000)&&at<U8>(o,0x1c5)){unsigned face=at<U16>(o,0xa6);unsigned pid=at<U16>(o,0x10);bool counted=pid&&pid<=0x1770&&person(pid)==o;unsigned f=repick(o,face,counted);if(f!=face){at<U16>(o,0xa6)=(U16)f;changed++;}}}}
 rescans++;assigned_total+=changed;cache_ok=false; // 얼굴을 바꿨으니 다음 배정 때 표를 처음부터
 {static unsigned logged;if((changed&&logged<3)||rescans<=2||rescans%500==0){if(changed)logged++;r39::counter(32,"SAN14_HiredFace.dll","rescan_changed",changed);}} // 바뀐 게 없으면 기록 파일 안 씀(고용할 때마다 쓰지 않게)
}
// 시험·점검용: 0 표 새로 만든 수, 1 다시 쓴 수, 2 배정 수, 3 dirty 수
extern "C" __declspec(dllexport) unsigned SAN14_HiredFaceStats(int k){return k==0?builds:k==1?reuses:k==2?assigned_total:k==3?ndirty:0;}
extern "C"{void*FaceRet1,*FaceRet2,*FaceRet3,*FaceRet4;void FaceBridge1();void FaceBridge2();void FaceBridge3();void FaceBridge4();}
extern "C" __declspec(dllexport) void*SAN14_HiredFaceTestTable(int i){
 void*t[]={(void*)&FaceBridge1,(void*)&FaceBridge2,(void*)&FaceBridge3,(void*)&FaceBridge4,(void*)&FaceRet1,(void*)&FaceRet2,(void*)&FaceRet3,(void*)&FaceRet4,(void*)&FaceAssign,(void*)&FaceRescan};
 return i>=0&&i<10?t[i]:nullptr;
}
extern "C" __declspec(dllexport) void SAN14_HiredFaceSetEnabled(int on){InterlockedExchange(&enabled,on?1:0);}
extern "C" __declspec(dllexport) int SAN14_HiredFaceState(){return enabled;}
static bool hf_ok;
static void hf_install(){
 const S14_CoreAPI*api=core;
 HookPlan plan(api);
 static const U8 e1[]={0x66,0x89,0x47,0x10,0x48,0x8d,0x4c,0x24,0x20};FaceRet1=game+0x331416 +9;plan.jump(0x331416,e1,9,(void*)&FaceBridge1);
 static const U8 e2[]={0x41,0xff,0x4f,0x08,0x48,0x8b,0xc7};FaceRet2=game+0x33149a +7;plan.jump(0x33149a,e2,7,(void*)&FaceBridge2);
 static const U8 e3[]={0x41,0x88,0x87,0x65,0x01,0x00,0x00};FaceRet3=game+0x336a0e +7;plan.jump(0x336a0e,e3,7,(void*)&FaceBridge3);
 static const U8 e4[]={0x33,0xc0,0x39,0x47,0x50,0x0f,0x94,0xc0};FaceRet4=game+0x339729 +8;plan.jump(0x339729,e4,8,(void*)&FaceBridge4);
 bool ok=plan.install("SAN14CE.HiredFace");
 if(core->Log)core->Log(ok?"HiredFace R39: 4 sites hooked (original faces for hired officers).":"HiredFace R39: hook group NOT installed (bytes differ / conflict). Nothing changed.");
 r39::start(32,"SAN14_HiredFace.dll",ok?(enabled?"OK":"OFF"):"FAIL",ok?4:0,4,enabled?"Enabled=1":"Enabled=0",ok?"-":"hook bytes differ or another mod patched the site");
 if(!ok)r39::fail_plan(32,"SAN14_HiredFace.dll",plan,"hired-officer face sites exe+0x331416/0x33149a/0x336a0e/0x339729",L"고용 무장 원본 얼굴(32번)이 적용 안 됨 → 게임 기본(고용 얼굴). 다른 기능은 그대로");
 hf_ok=ok;
}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!r39::check_api(api,32,"SAN14_HiredFace.dll"))return 0;core=api;game=api->resolved->image;
 wchar_t ini[1024];if(r39::game_root(ini,1024)){r39::wcat(ini,1024,L"SAN14Mods\\modules\\HiredFace.ini");enabled=GetPrivateProfileIntW(L"HiredFace",L"Enabled",1,ini)?1:0;}
 // 정식판 뒤(11-08): 4곳 모두 통합편의 원본 확인 자리 안 → 통합편의 확인이 끝난 뒤 고침(ic_after_restore.h)
 return icw::run_after_restore(hf_install,api,"HiredFace")?(hf_ok?1:0):1;
}

BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
