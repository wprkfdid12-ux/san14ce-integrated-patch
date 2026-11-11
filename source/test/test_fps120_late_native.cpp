// P201(2026-11-09 제보): 19번 120프레임이 통합편의 6번 진행 배속 준비 검사 구간(exe+0x44dee0~) 안의 exe+0x44df40 을 먼저 고쳐 6번이 꺼짐.
//  src/fps120_late.cpp(실제 소스)를 가짜 게임 이미지·가짜 본체·가짜 통합편의로 돌려, 통합편의가 확인하는 동안 그 자리가 원래 모양인지 본다.
//  빌드/실행: clang++ -std=c++17 -O1 -DS14_TEST_SHIM -Wno-everything -I test/shim27 -I src test/test_fps120_late_native.cpp -o /tmp/t && /tmp/t
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include "windows.h"
extern "C"{HMODULE LoadLibraryExW(const wchar_t*,HANDLE,DWORD);BOOL VirtualProtect(void*,SIZE_T,DWORD,DWORD*);BOOL FlushInstructionCache(HANDLE,const void*,SIZE_T);HANDLE GetCurrentProcess();}
#include "../src/fps120_late.cpp"
static std::string nar(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
static bool ic_file,ic_enabled,ic_loaded_now,dll_ok,init_ok,init_patches;static int ready_after,wcalls,init_calls,vp_calls;static unsigned long long now_ms;
static U8 img[0x450000];static const LONG FPS_DISP=0x00123456;
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*o,DWORD cap){const char*p="C:\\G\\SAN14CE_KO.exe";DWORD n=0;for(;p[n]&&n+1<cap;n++)o[n]=p[n];o[n]=0;return n;}
extern "C" ULONGLONG GetTickCount64(void){return now_ms;}extern "C" void Sleep(DWORD ms){now_ms+=ms;}
extern "C" DWORD GetFileAttributesW(const wchar_t*p){std::string s=nar(p);if(s.find("SAN14_IntegratedConvenience.dll")!=std::string::npos||s.find("IntegratedConvenience_GreenlandCompat.ini")!=std::string::npos)return ic_file?0x20:0xffffffffu;return 0x20;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int,const wchar_t*){return ic_enabled?1:0;}
static U8 icmod[8],fpsmod[8];
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){if(n&&nar(n)=="SAN14_IntegratedConvenience.dll"&&ic_loaded_now)return (HMODULE)icmod;return nullptr;}
// 통합편의의 확인: 이 시험에서는 wait 를 부를 때마다 '그 순간 그 자리가 원래 모양인가'를 기록(통합편의가 실제로 확인하는 때를 흉내)
static int ic_saw_native=-1;
static U32 fake_wait(){int k=wcalls++;if(ready_after>=0&&k>=ready_after){if(ic_saw_native<0)ic_saw_native=!memcmp(img+0x44df40,kDuelNative,5);return 1;}now_ms+=15000;return 0;}
static int fake_init(const S14_CoreAPI*){init_calls++;if(!init_ok)return 0;if(init_patches){img[0x44df40]=0xe8;memcpy(img+0x44df41,&FPS_DISP,4);}return 1;}
extern "C" void*GetProcAddress(HMODULE m,const char*n){if(m==(HMODULE)icmod)return strcmp(n,"SAN14_WaitConvenienceV1")?nullptr:(void*)fake_wait;if(m==(HMODULE)fpsmod)return strcmp(n,"SAN14_ModInitV1")?nullptr:(void*)fake_init;return nullptr;}
extern "C" HMODULE LoadLibraryExW(const wchar_t*p,HANDLE,DWORD){return dll_ok&&nar(p).find("SAN14_FPS120.dll")!=std::string::npos?(HMODULE)fpsmod:nullptr;}
extern "C" BOOL VirtualProtect(void*,SIZE_T,DWORD,DWORD*o){vp_calls++;if(o)*o=0x20;return 1;}
extern "C" BOOL FlushInstructionCache(HANDLE,const void*,SIZE_T){return 1;}
extern "C" HANDLE GetCurrentProcess(){return nullptr;}
static LPTHREAD_START_ROUTINE pend;static void*parg;extern "C" HANDLE CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE f,LPVOID a,DWORD,LPDWORD){pend=f;parg=a;return (HANDLE)1;}
extern "C" BOOL CloseHandle(HANDLE){return 1;}
static std::vector<std::string> logs;static void lg(const char*s){logs.push_back(s);}
static bool has(const char*s){for(auto&l:logs)if(l.find(s)!=std::string::npos)return true;return false;}
static int fails;static void check(bool c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
static int rd(const void*,uintptr_t){return 1;}
static S14_Resolved res;static S14_CoreAPI api;
static bool native(){return !memcmp(img+0x44df40,kDuelNative,5);}
static bool fps_hook(){LONG v;memcpy(&v,img+0x44df41,4);return img[0x44df40]==0xe8&&v==FPS_DISP;}
static void reset(){ic_file=true;ic_enabled=true;ic_loaded_now=false;dll_ok=init_ok=init_patches=true;ready_after=-1;wcalls=init_calls=vp_calls=0;now_ms=1000;pend=nullptr;logs.clear();ic_saw_native=-1;
 memcpy(img+0x44df40,kDuelNative,5);fps_disp=0;}
static std::vector<std::string> written;
extern "C" BOOL CreateDirectoryW(const wchar_t*,void*){return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*,DWORD,DWORD,void*,DWORD,DWORD,void*){return (HANDLE)2;}
extern "C" BOOL WriteFile(HANDLE,const void*b,DWORD n,DWORD*w,void*){written.push_back(std::string((const char*)b,n));if(w)*w=n;return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof *t);}
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){return 0;}
static bool failed(){for(auto&l:written)if(l.find("R43F")!=std::string::npos)return true;return false;}
static bool started(const char*st){for(auto&l:written)if(l.find("SAN14_FPS120Late.dll")!=std::string::npos&&l.find(st)!=std::string::npos)return true;return false;}
static I32 ih(const char*,const S14_Hook*,U32){return 1;}static U8*mr(const void*,const void*){return img;}static void fr(void*){}
int main(){
 res.size=sizeof res;res.abi=S14_ABI;res.image_size=S14_GAME_IMAGE;res.image=img;
 api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=rd;api.InstallHooks=ih;api.MakeRelay=mr;api.FreeRelay=fr;api.Log=lg;
 printf("[1] 통합편의 안 깔림 → 본체가 넣은 그대로(되돌리지 않음, 기대지 않음)\n");{reset();written.clear();ic_file=false;int r=SAN14_ModInitV1(&api);
  check(r==1&&init_calls==1&&fps_hook()&&!pend&&vp_calls==0,"본체 시작, 일기토 후크 그대로, 기다리는 작업 없음");check(started("\tOK\t")&&!failed(),"시작 기록 OK, 오류 없음");}
 printf("[2] 제보 상황: 통합편의 깔림 → 통합편의가 확인하는 동안 그 자리는 원래 모양, 끝난 뒤 다시 넣음\n");{reset();written.clear();int r=SAN14_ModInitV1(&api);
  check(r==1&&init_calls==1,"본체는 예전과 같은 때(모듈 시작 때) 바로 시작");
  check(native()&&pend,"시작 직후: exe+0x44df40 은 게임 원래 바이트(통합편의 6번 검사 통과 조건), 뒤 작업 대기");
  ic_loaded_now=true;ready_after=2;pend(parg);
  check(ic_saw_native==1,"통합편의가 확인을 끝낸 순간까지 그 자리는 원래 바이트였음");
  check(fps_hook()&&has("re-applied after IC restore check"),"통합편의 확인이 끝난 뒤 일기토 후크를 다시 넣음");check(!failed(),"오류 없음");}
 printf("[3] 통합편의가 이미 확인을 끝낸 뒤에 우리가 시작 → 바로 다시 넣음\n");{reset();written.clear();ic_loaded_now=true;ready_after=0;SAN14_ModInitV1(&api);if(pend)pend(parg);
  check(fps_hook()&&!failed(),"일기토 후크 들어가 있음");}
 printf("[4] 본체 시작 실패(다른 모드가 19번 자리를 고침 등) → 오류 창·기록, 자리 안 건드림\n");{reset();written.clear();init_ok=false;int r=SAN14_ModInitV1(&api);
  check(r==0&&native()&&vp_calls==0&&!pend,"0 돌려줌, 자리 그대로");check(failed()&&started("\tFAIL\t"),"R43F 줄과 FAIL 시작 기록");}
 printf("[5] 기다리는 사이 다른 모드가 그 자리를 바꿈 → 덮어쓰지 않고 알림\n");{reset();written.clear();SAN14_ModInitV1(&api);img[0x44df40]=0xe9;ic_loaded_now=true;ready_after=0;written.clear();pend(parg);
  check(img[0x44df40]==0xe9&&failed(),"남의 바이트를 덮어쓰지 않음 + R43F 줄");}
 printf("[6] 시작할 때 그 자리가 이미 원래 모양이 아님 → 되돌리기 안 함\n");{reset();written.clear();img[0x44df41]^=0x55;U8 keep[5];init_patches=false;memcpy(keep,img+0x44df40,5);int r=SAN14_ModInitV1(&api);
  check(r==1&&!memcmp(keep,img+0x44df40,5)&&vp_calls==0&&!pend,"건드리지 않음");}
 printf("[7] 본체 파일이 없음 → 오류 창·기록\n");{reset();written.clear();dll_ok=false;int r=SAN14_ModInitV1(&api);check(r==0&&failed()&&init_calls==0,"R43F 줄, 본체 안 부름");}
 printf("[8] 통합편의가 60초 안에 안 올라옴 → 그래도 다시 넣음(일기토 제한이 영영 빠지지 않게). 10분 넘김은 ic_after_restore 시험 [6]\n");{reset();written.clear();SAN14_ModInitV1(&api);
  check(pend!=nullptr,"뒤 작업 대기");if(pend)pend(parg);check(fps_hook()&&now_ms>60000,"안 올라와도 60초 뒤 다시 넣음");}
 printf(fails?"\n실패 %d\n":"\n전부 통과\n",fails);return fails?1:0;
}
