// 내정 인재 배치(DomesticTalent R3) 실행 검사. MODE 1=안개 없음, 2=안개 정상 시작, 3=안개 시작 실패
#include "mock.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include "../src/domestic_talent.cpp"
using namespace mock;
static int native_calls, fog_calls; static std::vector<U8*> assigned; static U8 *place_obj, *district_obj, *city_obj;
static U8 *mem(size_t n){U8*p=(U8*)calloc(1,n);return p;}
extern "C" {
static void*seen_arg;
static void m_native(void*,void*a){++native_calls;seen_arg=a;}
static int m_valid(void*p){return p!=nullptr;}
static std::atomic<int> work_calls{0};
static int m_person_force(U8*p){++work_calls;return *(int*)(p+0x100);}
static U8*m_place(U8*,int){return place_obj;}
static U8*m_city_of(U8*h){return *(U8**)(h+8);}
static U8*m_assigned(U8*){return nullptr;}
static int m_can(void**){return 1;}
static int m_score(U8*p,U8*,int){++work_calls;return *(int*)(p+0x104);}
static int m_cost(){return 1;}
static void m_assign(void**pair,int){assigned.push_back((U8*)pair[1]);*(U16*)((U8*)pair[0]+0x36)=*(U16*)((U8*)pair[1]+0x10);}
static int m_area_force(U8*a){return *(int*)(a+0x100);}
static int m_enabled(){return 1;}
static U8*m_district(U8*){return district_obj;}
}
static void (*fog_handler_target)(void*);
extern "C" void m_fog_handler(void*s,void*a){++fog_calls;((void(*)(void*,void*))(mock::game+0x44bb40))(s,a);}
int main(){
 init();
 for(auto r:{std::pair<U32,void*>{0x44bb40,(void*)m_native},{0x32e4c0,(void*)m_valid},{0x22c380,(void*)m_person_force},{0x32b110,(void*)m_place},{0x229d00,(void*)m_city_of},
   {0x22c490,(void*)m_assigned},{0x1ef7f0,(void*)m_can},{0x3400b0,(void*)m_score},{0x6d30a0,(void*)m_cost},{0x1f22f0,(void*)m_assign},{0x22c160,(void*)m_area_force}})native(r.first,r.second);
 *(UPTR*)(mock::game+0x13d6368)=(UPTR)(mock::game+0x44bb40);
 U8*db=mem(0x88000);*(U8**)(mock::game+0x277df18)=db;U8*world=mem(0x500);*(U8**)(db+0x871e8)=world;world[0x3a]=1;*(U16*)(world+0x34)=200;world[0x36]=3;world[0x37]=1;
 U8*state=mem(0x500);*(U8**)state=mock::game+0x13d6340;*(int*)(state+0x470)=2;
 U8**stack=(U8**)mem(64);stack[0]=state;*(int*)(mock::game+0x1b620b0+0x10)=1;*(U8***)(mock::game+0x1b620b0+0x20)=stack;
 U8*dom=mem(0x30000);modules[u"SAN14_DomesticCE.dll"]=(HMODULE)dom;digests[u"SAN14_DomesticCE.dll"]=hex("468536c0ebb82a8c6d34f0b32f77f65e9b5bd6723d798a2a2f62f958d7589a8f");procs["DomesticGetEnabled"]=(void*)m_enabled;
 city_obj=mem(0x100);*(U8**)(db+0xdd38+5*8)=city_obj;
 U8**vt=(U8**)mem(0x100);vt[0x80/8]=(U8*)m_district;place_obj=mem(0x100);*(U8***)place_obj=vt;*(U8**)(place_obj+8)=city_obj;
 district_obj=mem(0x100);district_obj[0x14]=5;
 U8*area=mem(0x200);*(U8**)area=mock::game+0x13a6930;*(int*)(area+0x100)=1;area[0x72]=1;area[0x35]=5;*(U16*)(area+0x36)=10;*(U8**)(db+0x6caf0+1*8)=area;
 auto person=[&](unsigned id,int force,int score){U8*p=mem(0x200);*(U8**)p=mock::game+0x13a6b38;*(U16*)(p+0x10)=id;p[0xe6]=2;*(U16*)(p+0xe2)=100;*(U16*)(p+0xe4)=100;*(int*)(p+0x100)=force;*(int*)(p+0x104)=score;*(U8**)(db+0x158+id*8)=p;return p;};
 U8*area2=mem(0x200);*(U8**)area2=mock::game+0x13a6930;*(int*)(area2+0x100)=1;area2[0x72]=2;area2[0x35]=5;*(U16*)(area2+0x36)=11;*(U8**)(db+0x6caf0+2*8)=area2;
 person(11,1,40);person(10,1,50);person(20,1,70);U8*best=person(3000,1,90);person(5999,1,80);person(4000,2,999);
 U8*fog=nullptr;
 if(MODE>=2){
  profile_ints[u"FogFieldPair.ini|Enabled"]=1;existing_files={u"SAN14_FogTrial.dll",u"SAN14_FogEngine121.dll"};
 }
 printf("[내정 인재 배치 검사 MODE %d]\n",MODE);
 int ok=SAN14_ModInitV1(&api);check(ok==1,"모듈 시작 성공");
 if(MODE==1){check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)&DomesticTalentTick,"안개가 없으면 시작 즉시 메인 맵 처리 자리에 연결");}
 else{
  usleep(50000);
  check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)(mock::game+0x44bb40),"안개가 설정돼 있으면 시작 시 원래 자리를 건드리지 않음 (안개 시작 검사 통과 조건)");
  fog=alloc_exec(0x940000);fog[0]='M';fog[1]='Z';*(U32*)(fog+0x3c)=0x80;*(U32*)(fog+0x80+0x50)=0x940000;
  digests[u"SAN14_FogEngine121.dll"]=hex("603735d2afc39e26046e6581c6c9690a9f967fd3fee41055e468fce26488b3b8");
  modules[u"SAN14Mods\\Fog121\\SAN14_FogEngine121.dll"]=(HMODULE)fog;modules[u"SAN14_FogEngine121.dll"]=(HMODULE)fog;
  usleep(100000);
  check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)(mock::game+0x44bb40),"안개 파일이 올라왔어도 안개가 자리를 잡기 전에는 그대로 둠");
  if(MODE==2){
   thunk(fog+0x1000,(void*)m_fog_handler);
   // 안개가 자신의 시작 검사(원래 값 확인)를 통과한 뒤 자기 처리기로 교체
   check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)(mock::game+0x44bb40),"안개 시작 검사 시점에 원래 값 유지");
   *(UPTR*)(mock::game+0x13d6368)=(UPTR)(fog+0x1000);
   usleep(30000);check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)(fog+0x1000),"안개 시작 마무리(완료 표시) 전에는 이어 붙이지 않음");
   fog[0x2c9244]=1; // 안개 시작 완료
   for(int i=0;i<200&&*(UPTR*)(mock::game+0x13d6368)!=(UPTR)&DomesticTalentTick;i++)usleep(10000);
   check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)&DomesticTalentTick,"안개가 자리를 잡은 뒤 그 위에 이어 붙음");
  }else if(MODE==4){
   *(U32*)(fog+0x92a9b8)=0x62;fog[0x2c9244]=1; // 안개는 켜졌지만 이 자리를 쓰지 않는 설정
   for(int i=0;i<400&&*(UPTR*)(mock::game+0x13d6368)!=(UPTR)&DomesticTalentTick;i++)usleep(10000);
   check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)&DomesticTalentTick,"안개가 이 자리를 쓰지 않고 시작을 마치면 원래 처리기에 연결");
  }else{
   *(U32*)(fog+0x92a9b8)=0x62;fog[0x2c9244]=0; // 안개 시작 시도 후 실패 상태
   for(int i=0;i<400&&*(UPTR*)(mock::game+0x13d6368)!=(UPTR)&DomesticTalentTick;i++)usleep(10000);
   check(*(UPTR*)(mock::game+0x13d6368)==(UPTR)&DomesticTalentTick,"안개 시작이 실패로 끝나면 20초 뒤 원래 처리기에 연결");
  }
 }
 auto slot=[&]{return (void(*)(void*,void*,void*,void*))*(UPTR*)(mock::game+0x13d6368);};
 volatile uint64_t*stamp=(volatile uint64_t*)(dom+0x2c050);
 auto frame=[&]{tick+=16;slot()(state,(void*)0x5151,nullptr,nullptr);};
 auto next_day=[&]{if(++world[0x37]>30){world[0x37]=1;if(++world[0x36]>12){world[0x36]=1;++*(U16*)(world+0x34);}}};
 // 1) 불러온 직후 턴 대기
 int w0=work_calls;unsigned maxr=0;
 for(int i=0;i<600;i++){readable_calls=0;frame();if(readable_calls>maxr)maxr=readable_calls;}
 check(native_calls==600,"매 프레임 원래 게임 처리가 먼저 정확히 한 번 실행");
 if(MODE==2)check(fog_calls==600,"안개 처리기도 매 프레임 한 번씩 실행 (안개 기능 유지)");
 check(seen_arg==(void*)0x5151,"게임이 넘긴 두 번째 값이 원래 처리까지 그대로 전달");
 check(maxr<=3,"턴 대기 중 프레임당 부담은 메모리 확인 3회 이하");
 check(*stamp==0,"날짜 진행을 직접 확인하기 전에는 원본 내정자동화를 멈추지 않음 (원래대로)");
 if(MODE==5){ // 날짜 위치가 예상과 다른 경우: 한 달에 한 번만 값이 바뀜
  bool ever=false;for(int i=0;i<20000;i++){if(i%900==0){if(++world[0x36]>12)world[0x36]=1;}frame();if(*stamp)ever=true;}
  check(!ever,"날짜 형식이 예상과 다르면 원본 내정자동화를 한 번도 멈추지 않음 (AI 명령서 기능 보호)");
  check(assigned.empty()&&work_calls==w0,"어떤 경우에도 무장 명령(탐색)을 내리지 않음");
  return finish();
 }
 // 2) 턴 진행 (하루마다 날짜 변경)
 bool released=false;
 for(int f=0;f<1500;f++){if(f%30==0)next_day();frame();if(*stamp==0)released=true;}
 check(released,"턴 진행이 시작되면 내정자동화 3초 작업을 다시 풀어 줌 (AI 명령서·인구 보장 유지)");
 check(assigned.empty()&&work_calls==w0,"턴 진행 중에도 무장 명령(탐색·배치)을 전혀 내리지 않음 (R8: 지피티가 쓴 함수는 탐색 명령이었음)");
 // 3) 진행 멈춤 → 다시 대기
 for(int i=0;i<2400;i++)frame();
 check(*stamp>tick.load(),"날짜 진행 확인 후 다시 대기 상태가 되면 내정자동화 3초 작업을 쉼");
 uint64_t maint=tick.load()+10000;*stamp=maint;for(int i=0;i<100;i++)frame();
 check(*stamp==maint,"내정자동화가 직접 건 정지(모드 처리 재정리)는 덮어쓰지 않음");
 check(assigned.empty(),"끝까지 무장 명령 0건");
 return finish();
}
