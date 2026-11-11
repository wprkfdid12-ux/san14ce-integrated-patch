#pragma once
// 정식판 뒤(11-08 갤 제보 '32번 켜면 먹통, 빼면 됨'): 통합편의는 시작 때 게임 코드 2,346곳이 원래 모양인지 확인(최대 3분)한 뒤에야
//  1~7번을 켬. 그 확인 자리 안을 우리 모듈이 먼저 고치면(32번 HiredFace 4곳·29번 StatusColor 1곳) 통합편의가 3분 기다리다 1~7번을 전부 안 켬.
//  PC 속도에 따라 누가 먼저냐가 갈려 어떤 PC 에서만 생겼음.
//  → 그런 자리를 고치는 모듈은 통합편의가 그 확인을 끝낸 뒤(SAN14_WaitConvenienceV1 = 1)에 고친다. 뒤 작업에서 기다림(게임 진행 안 막음).
//    통합편의가 안 깔렸거나 꺼져 있으면 바로 고침. 통합편의가 60초 안에 안 올라오거나 10분이 지나도 안 끝나면 그냥 고침(예전 동작).
#include "common.h"
#include "r39_record.h"
namespace icw{
// 통합편의가 깔려 있고 켜져 있나(등록 파일 Enabled=1 + DLL 파일 있음)
inline bool ic_installed(){
 wchar_t r[1024];if(!r39::game_root(r,1024))return false;
 wchar_t d[1100];d[0]=0;r39::wcat(d,1100,r);r39::wcat(d,1100,L"SAN14_IntegratedConvenience.dll");if(GetFileAttributesW(d)==(DWORD)0xffffffffu)return false;
 wchar_t i[1100];i[0]=0;r39::wcat(i,1100,r);r39::wcat(i,1100,L"SAN14Mods\\modules\\IntegratedConvenience_GreenlandCompat.ini");
 if(GetFileAttributesW(i)==(DWORD)0xffffffffu)return false;
 return GetPrivateProfileIntW(L"Mod",L"Enabled",1,i)!=0;
}
// 지금 바로 고쳐도 되나(통합편의 없음 또는 확인 끝남) — 기다리지 않음
inline bool ready_now(){
 HMODULE ic=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");if(!ic)return !ic_installed();
 return false;   // 실려 있으면 뒤 작업에서 wait() 로 확인(바로 부르면 최대 15초 막힘)
}
struct Job{void(*fn)();const S14_CoreAPI*api;const char*who;};
inline DWORD WINAPI worker(LPVOID p){
 Job*j=(Job*)p;unsigned long long t0=GetTickCount64();
 for(;;){   // 통합편의가 올라올 때까지(최대 60초)
  if(GetModuleHandleW(L"SAN14_IntegratedConvenience.dll"))break;
  if(GetTickCount64()-t0>60000)break;Sleep(200);
 }
 HMODULE ic=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");
 const char*how="ic-absent";
 if(ic){auto wait=reinterpret_cast<U32(*)()>(GetProcAddress(ic,"SAN14_WaitConvenienceV1"));how="ic-no-wait-export";
  if(wait){how="ic-timeout";unsigned long long until=GetTickCount64()+600000;while(GetTickCount64()<until){if(wait()){how="ic-ready";break;}Sleep(500);}}}
 if(j->api&&j->api->Log){char m[200];m[0]=0;r39::scat(m,200,j->who);r39::scat(m,200,": installing after IC restore check (");r39::scat(m,200,how);r39::scat(m,200,").");j->api->Log(m);}
 j->fn();return 0;
}
// 통합편의 확인이 끝난 뒤 fn() 실행. 반환: true = 바로 실행함 / false = 뒤 작업으로 넘김(ModInit 은 1 을 돌려주면 됨)
inline bool run_after_restore(void(*fn)(),const S14_CoreAPI*api,const char*who){
 if(ready_now()){fn();return true;}
 static Job jobs[4];static volatile LONG n;LONG k=InterlockedIncrement(&n)-1;if(k>=4){fn();return true;}
 jobs[k]=Job{fn,api,who};
 if(HANDLE t=CreateThread(nullptr,0,worker,&jobs[k],0,nullptr)){CloseHandle(t);
  if(api&&api->Log){char m[200];m[0]=0;r39::scat(m,200,who);r39::scat(m,200,": waiting for IC restore check before patching (its sites overlap IC's check list).");api->Log(m);}
  return false;}
 fn();return true;
}
}
