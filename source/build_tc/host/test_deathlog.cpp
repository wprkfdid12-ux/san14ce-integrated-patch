// 27번 전사 기록 × 안개 기록창 필터: 매 프레임 기록 줄마다 드는 부담 검사
#include "mock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace mock;
extern const S14_CoreAPI*core27;extern U8*game27;
extern "C" { bool DeathLogsInit(); void DeathLogsAttach(); U8 DeathHistoryVisible(U8*);
 U8* DeathHistory(void*,int,int,int,int,int,const wchar_t*,const wchar_t*,int,int,int,int,int); int DeathScopeActive(){return 1;} }
static int fog_filter_calls;
extern "C" U8 m_fog_filter(U8*){++fog_filter_calls;return 0;}
static U8*made;
extern "C" U8* m_native_history(void*,int kind,int district,int cell,int,int date,const wchar_t*title,const wchar_t*body,int speaker,int,int,int,int){
 U8*r=(U8*)calloc(1,0x140);*(U8**)r=mock::game+0x13a75b8;*(U16*)(r+0x10)=date;r[0x12]=district;r[0x86]=kind;*(U16*)(r+0x84)=cell;*(U16*)(r+0x88)=speaker;
 for(int i=0;i<54&&title[i];i++)((wchar_t*)(r+0x14))[i]=title[i];for(int i=0;i<84&&body[i];i++)((wchar_t*)(r+0x8a))[i]=body[i];made=r;return r;}
int main(){
 init();core27=&api;game27=mock::game;printf("[전사 기록 × 기록창 필터 검사]\n");
 native(0x9338c0,(void*)m_native_history);
 U8*fog=alloc_exec(0x100000);modules[u"SAN14_FogEngine121.dll"]=(HMODULE)fog;digests[u"SAN14_FogEngine121.dll"]=hex("603735d2afc39e26046e6581c6c9690a9f967fd3fee41055e468fce26488b3b8");
 const U8 f0[]={0x56,0x57,0x53,0x48,0x83,0xec,0x20,0x48,0x83,0xc4,0x20,0x5b,0x5f,0x5e};memcpy(fog+0x32040,f0,sizeof f0);thunk(fog+0x32040+sizeof f0,(void*)m_fog_filter);
 DeathLogsInit();DeathLogsAttach();
 // 평범한 기록 30줄 (글자가 채워진 실제 모양)
 U8*rows[30];for(int i=0;i<30;i++){U8*r=(U8*)calloc(1,0x140);*(U8**)r=mock::game+0x13a75b8;*(U16*)(r+0x10)=500+i;r[0x12]=3;r[0x86]=1;*(U16*)(r+0x84)=100+i;*(U16*)(r+0x88)=7;
  for(int c=0;c<40;c++)((wchar_t*)(r+0x14))[c]=L'가'+c;for(int c=0;c<80;c++)((wchar_t*)(r+0x8a))[c]=L'나'+c;rows[i]=r;}
 auto filter=(U8(*)(U8*))(fog+0x32040);
 readable_calls=0;for(int f=0;f<60;f++)for(int i=0;i<30;i++)filter(rows[i]);
 printf("    기록 30줄 × 60프레임: 메모리 확인 %u회, 안개 원래 필터 %d회\n",readable_calls.load(),fog_filter_calls);
 check(fog_filter_calls==1800,"모든 줄이 안개 원래 필터로 그대로 전달 (안개 판정 유지)");
 check(readable_calls==0,"평범한 기록 줄은 메모리 확인 0회");
 // 전사 기록 하나 생성 → 기록창에서 보이게
 unsigned opens0=createfile_calls.load();
 U8*death=DeathHistory(nullptr,2,5,1234,0,777,L"전사",L"조운이 전사했습니다",9,0,0,0,0);
 check(createfile_calls.load()-opens0==1,"첫 전사에서 기록 파일을 한 번 연다");
 int before=fog_filter_calls;U8 v=filter(death);
 check(death==made&&v==1&&fog_filter_calls==before,"전사 기록은 안개에 가려지지 않고 표시 (기존 기능 유지)");
 printf("[R5: 전사 기록 저장 비용]\n");
 {unsigned opens=createfile_calls.load(),flushes=flush_calls.load(),reads=readable_calls.load();
  for(int i=0;i<50;i++){wchar_t body[40];for(int c=0;c<30;c++)body[c]=L'가'+c;body[30]=0;DeathHistory(nullptr,2,5,1000+i,0,800+i,L"전사",body,10+i,0,0,0,0);}
  printf("    전사 50건: 파일 열기 %u회, 디스크 동기화 %u회, 메모리 확인 %u회\n",createfile_calls.load()-opens,flush_calls.load()-flushes,readable_calls.load()-reads);
  check(createfile_calls.load()-opens==0,"그 뒤 전사 50건 동안 파일 열기 0회 (전사마다 열고 닫지 않음)");
  check(flush_calls.load()-flushes==0,"전사마다 디스크 동기화(FlushFileBuffers) 0회");
  check(readable_calls.load()-reads<=50*4,"전사 1건당 메모리 확인 4회 이하 (글자마다 묻지 않음)");
  before=fog_filter_calls;check(filter(made)==1&&fog_filter_calls==before,"마지막 전사 기록도 표시됨");}
 return finish();
}
