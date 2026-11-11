// 2번 자동인사 수정(AutoPersonnelRepair R4): 설정창을 열지 않아도 시작 후 연결되는지 (R11 통합편의 판 기준)
// + R4 설치 순서 보장: 통합편의 등록이 켜져 있으면 ModInit 안에서 통합편의 시작 작업을 끝까지 기다린다
#include "mock.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
using namespace mock;
extern "C" { int SAN14_ModInitV1(const S14_CoreAPI*); void AutoRepairMasterChanged(U8); void AutoRepairUiTick(); }
static std::atomic<int> ready{0};static std::atomic<int> waitCalls{0};
extern "C" unsigned m_wait(){waitCalls++;return ready.load();}
extern "C" unsigned char m_policy(uint64_t){return 7;}
int main(){
 init();printf("[2번 자동인사 수정 검사]\n");
 U8*ic=alloc_exec(0x200000,(UPTR)game+0x10000000);modules[u"SAN14_IntegratedConvenience.dll"]=(HMODULE)ic;
 digests[u"SAN14_IntegratedConvenience.dll"]=hex("9c50b73bdfe13dbfd9bfea0f3deb560882d6892aea45df78b13c57f4a8d7c282");procs["SAN14_WaitConvenienceV1"]=(void*)m_wait; // R11 통합편의(말풍선 수정판). 27번 검사는 P1 판으로 돌려 둘 다 받는지 확인
 const U8 p0[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x83,0xc4,0x20,0x5b};memcpy(ic+0x65580,p0,sizeof p0);thunk(ic+0x65580+sizeof p0,(void*)m_policy);
 check(SAN14_ModInitV1(&api)==1,"모듈 시작 성공");
 usleep(60000);
 check(ic[0x65580]==0x40,"통합편의 모듈 준비 전에는 연결하지 않음");
 ready=1;for(int i=0;i<500&&ic[0x65580]!=0xe9;i++)usleep(10000);
 check(ic[0x65580]==0xe9&&*(void**)(ic+0x19e6f0)==(void*)&AutoRepairMasterChanged,"설정창을 한 번도 열지 않아도 준비 직후 자동인사 수정 연결");
 AutoRepairUiTick();check(true,"설정창 경로가 뒤늦게 불려도 중복 연결 없음");
 auto hasLog=[](const char*k){for(auto&l:logs)if(l.find(k)!=std::string::npos)return true;return false;};
 check(!hasLog("AutoPersonnel R4"),"통합편의 등록 파일이 없으면 순서 맞춤을 하지 않음(기존 동작)");
 // 통합편의 등록(Enabled=1)과 DLL 이 있을 때: ModInit 이 돌아오기 전에 시작 작업 대기를 한 번 부른다
 profile_ints[u"IntegratedConvenience_GreenlandCompat.ini|Enabled"]=1;existing_files.push_back(u"SAN14_IntegratedConvenience.dll");
 int before=waitCalls.load();check(SAN14_ModInitV1(&api)==1,"다시 시작해도 성공");
 check(waitCalls.load()==before+1&&hasLog("AutoPersonnel R4: convenience start-up finished before later modules"),"등록이 켜져 있으면 ModInit 안에서 통합편의 시작 완료를 기다림");
 profile_ints[u"IntegratedConvenience_GreenlandCompat.ini|Enabled"]=0;before=waitCalls.load();SAN14_ModInitV1(&api);usleep(20000);
 check(waitCalls.load()==before,"등록이 꺼져 있으면(Enabled=0) 기다리지 않음");
 return finish();
}
