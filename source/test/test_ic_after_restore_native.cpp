// 정식판 뒤(11-08 갤 제보 '32번 켜면 먹통'): 29·32번이 통합편의 원본 확인이 끝난 뒤에 고치는지(ic_after_restore.h) — 리눅스 네이티브
//  빌드/실행: clang++ -std=c++17 -O1 -DS14_TEST_SHIM -Wno-everything -I test/shim27 -I src test/test_ic_after_restore_native.cpp -o /tmp/t_icw && /tmp/t_icw
#include "ic_after_restore.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
static std::string nar(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*o,DWORD cap){const char*p="C:\\G\\SAN14CE_KO.exe";DWORD n=0;for(;p[n]&&n+1<cap;n++)o[n]=p[n];o[n]=0;return n;}
static bool ic_file,ic_ini,ic_enabled=true,ic_loaded_now;static long long load_at=-1;static int ready_after=-1,wcalls;
static unsigned long long now_ms;extern "C" ULONGLONG GetTickCount64(void){return now_ms;}extern "C" void Sleep(DWORD ms){now_ms+=ms;}
extern "C" DWORD GetFileAttributesW(const wchar_t*p){std::string s=nar(p);if(s.find("SAN14_IntegratedConvenience.dll")!=std::string::npos)return ic_file?0x20:0xffffffffu;if(s.find("IntegratedConvenience_GreenlandCompat.ini")!=std::string::npos)return ic_ini?0x20:0xffffffffu;return 0xffffffffu;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int d,const wchar_t*){return ic_enabled?1:0;}
static U8 icmod[8];
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){if(n&&nar(n)=="SAN14_IntegratedConvenience.dll"&&(ic_loaded_now||(load_at>=0&&(long long)now_ms>=load_at)))return (HMODULE)icmod;return nullptr;}
static U32 fake_wait(){int k=wcalls++;if(ready_after>=0&&k>=ready_after)return 1;now_ms+=15000;return 0;}
extern "C" void*GetProcAddress(HMODULE,const char*n){return strcmp(n,"SAN14_WaitConvenienceV1")?nullptr:(void*)fake_wait;}
static LPTHREAD_START_ROUTINE pend;static void*parg;extern "C" HANDLE CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE f,LPVOID a,DWORD,LPDWORD){pend=f;parg=a;return (HANDLE)1;}
extern "C" BOOL CloseHandle(HANDLE){return 1;}
static int ran;static void fn(){ran++;}
static std::vector<std::string> logs;static void lg(const char*s){logs.push_back(s);}
static int fails;static void check(bool c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
static bool has(const char*s){for(auto&l:logs)if(l.find(s)!=std::string::npos)return true;return false;}
static S14_CoreAPI api;
static void reset(){ic_file=ic_ini=true;ic_enabled=true;ic_loaded_now=false;load_at=-1;ready_after=-1;wcalls=0;now_ms=1000;pend=nullptr;ran=0;logs.clear();}
int main(){
 api.Log=lg;
 printf("[1] 통합편의 안 깔림 → 바로 고침\n");{reset();ic_file=false;bool r=icw::run_after_restore(fn,&api,"HiredFace");check(r&&ran==1&&!pend,"바로 실행");}
 printf("[2] 통합편의 꺼 둠(등록 Enabled=0) → 바로 고침\n");{reset();ic_enabled=false;bool r=icw::run_after_restore(fn,&api,"HiredFace");check(r&&ran==1,"바로 실행");}
 printf("[3] 제보 상황: 통합편의 깔림·아직 안 올라옴(등록 순서상 32번이 먼저) → 기다렸다가 확인 끝난 뒤 고침\n");
 {reset();load_at=1500;ready_after=2;bool r=icw::run_after_restore(fn,&api,"HiredFace");check(!r&&ran==0&&pend,"ModInit 은 안 고치고 뒤 작업으로");check(has("waiting for IC restore check"),"기다림 기록");
  pend(parg);check(ran==1&&wcalls==3&&has("(ic-ready)"),"통합편의 확인 끝난 뒤(3번째 wait=1) 고침");}
 printf("[4] 통합편의가 이미 실려 있음 → 확인 끝날 때까지 뒤에서 기다림\n");{reset();ic_loaded_now=true;ready_after=0;bool r=icw::run_after_restore(fn,&api,"StatusColor");check(!r&&ran==0,"바로 안 고침");pend(parg);check(ran==1&&has("StatusColor: installing after IC restore check (ic-ready)"),"확인 뒤 고침");}
 printf("[5] 통합편의 깔렸는데 60초 안에 안 올라옴 → 그냥 고침(예전 동작)\n");{reset();bool r=icw::run_after_restore(fn,&api,"HiredFace");pend(parg);check(!r&&ran==1&&now_ms-1000>=60000&&has("(ic-absent)"),"60초 뒤 고침");}
 printf("[6] 통합편의가 10분 안에 확인을 못 끝냄 → 그냥 고침\n");{reset();ic_loaded_now=true;bool r=icw::run_after_restore(fn,&api,"HiredFace");pend(parg);check(!r&&ran==1&&has("(ic-timeout)"),"10분 뒤 고침");}
 printf(fails?"\n실패 %d\n":"\n전부 통과\n",fails);return fails?1:0;
}
