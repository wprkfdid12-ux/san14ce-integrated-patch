// R42 C 시험 도구 바탕화면 실행기 — 시험도구_실행.exe (시험판 전용, 본판에 안 들어감)
//  게임 폴더에 아무것도 안 넣음. 같은 폴더의 SAN14_SceneTest.dll 을 실행 중인 게임(SAN14CE_KO.exe)에 올려 시험 창을 띄움.
//  게임 경로 지정 필요 없음: 실행 중인 게임을 프로세스 이름으로 찾고, 안 켜져 있으면 Steam 으로 켠 뒤 기다림.
//  통합패치(ModCore)가 올라온 뒤에 붙임. 이미 붙어 있으면 그대로 둠.
#include <stdint.h>
typedef unsigned long DWORD;typedef int BOOL;typedef void*HANDLE;typedef void*HMODULE;typedef wchar_t WCHAR;typedef const wchar_t*LPCWSTR;typedef uintptr_t UPTR;typedef unsigned short WORD;typedef unsigned char BYTE;
#define WINAPI __stdcall
extern "C"{
__declspec(dllimport) HANDLE WINAPI CreateToolhelp32Snapshot(DWORD,DWORD);
__declspec(dllimport) BOOL WINAPI Process32FirstW(HANDLE,void*);__declspec(dllimport) BOOL WINAPI Process32NextW(HANDLE,void*);
__declspec(dllimport) BOOL WINAPI Module32FirstW(HANDLE,void*);__declspec(dllimport) BOOL WINAPI Module32NextW(HANDLE,void*);
__declspec(dllimport) HANDLE WINAPI OpenProcess(DWORD,BOOL,DWORD);__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
__declspec(dllimport) void*WINAPI VirtualAllocEx(HANDLE,void*,UPTR,DWORD,DWORD);__declspec(dllimport) BOOL WINAPI VirtualFreeEx(HANDLE,void*,UPTR,DWORD);
__declspec(dllimport) BOOL WINAPI WriteProcessMemory(HANDLE,void*,const void*,UPTR,UPTR*);
__declspec(dllimport) HANDLE WINAPI CreateRemoteThread(HANDLE,void*,UPTR,void*,void*,DWORD,DWORD*);
__declspec(dllimport) DWORD WINAPI WaitForSingleObject(HANDLE,DWORD);__declspec(dllimport) BOOL WINAPI GetExitCodeThread(HANDLE,DWORD*);
__declspec(dllimport) DWORD WINAPI GetModuleFileNameW(HMODULE,WCHAR*,DWORD);__declspec(dllimport) HMODULE WINAPI GetModuleHandleW(LPCWSTR);
__declspec(dllimport) void*WINAPI GetProcAddress(HMODULE,const char*);__declspec(dllimport) HMODULE WINAPI LoadLibraryExW(LPCWSTR,HANDLE,DWORD);
__declspec(dllimport) BOOL WINAPI FreeLibrary(HMODULE);__declspec(dllimport) void WINAPI Sleep(DWORD);
__declspec(dllimport) BOOL WINAPI CreateProcessW(LPCWSTR,WCHAR*,void*,void*,BOOL,DWORD,void*,LPCWSTR,void*,void*);
__declspec(dllimport) HANDLE WINAPI GetStdHandle(DWORD);__declspec(dllimport) BOOL WINAPI WriteConsoleW(HANDLE,const void*,DWORD,DWORD*,void*);
__declspec(dllimport) DWORD WINAPI GetFileAttributesW(LPCWSTR);__declspec(dllimport) void WINAPI ExitProcess(unsigned);
__declspec(dllimport) uint64_t WINAPI GetTickCount64();
}
struct PE32W{DWORD size,usage,pid;UPTR heap;DWORD mod,threads,parent;long pri;DWORD flags;WCHAR exe[260];};
struct ME32W{DWORD size,mid,pid,gusage,pusage;BYTE*base;DWORD bsize;HMODULE h;WCHAR mod[256];WCHAR path[260];};
static unsigned wl(const WCHAR*s){unsigned n=0;while(s[n])n++;return n;}
static bool weq(const WCHAR*a,const WCHAR*b){while(*a&&*b){WCHAR x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;a++;b++;}return *a==*b;}
static void say(const WCHAR*s){DWORD w;HANDLE o=GetStdHandle((DWORD)-11);WriteConsoleW(o,s,wl(s),&w,0);WriteConsoleW(o,L"\r\n",2,&w,0);}
static DWORD find_game(){
 HANDLE h=CreateToolhelp32Snapshot(2,0);if(h==(HANDLE)-1)return 0;PE32W e;e.size=sizeof e;DWORD pid=0;
 if(Process32FirstW(h,&e))do{if(weq(e.exe,L"SAN14CE_KO.exe")){pid=e.pid;break;}}while(Process32NextW(h,&e));
 CloseHandle(h);return pid;
}
static BYTE*module_in(DWORD pid,LPCWSTR name){
 HANDLE h=CreateToolhelp32Snapshot(0x8|0x10,pid);if(h==(HANDLE)-1)return 0;ME32W e;e.size=sizeof e;BYTE*b=0;
 if(Module32FirstW(h,&e))do{if(weq(e.mod,name)){b=e.base;break;}}while(Module32NextW(h,&e));
 CloseHandle(h);return b;
}
static void finish(const WCHAR*msg,unsigned code){say(msg);say(L"(10초 뒤 이 창은 닫힙니다)");Sleep(10000);ExitProcess(code);}
extern "C" void mainCRTStartup(){
 WCHAR dll[600];DWORD n=GetModuleFileNameW(0,dll,560);while(n&&dll[n-1]!='\\')n--;
 const WCHAR*dn=L"SAN14_SceneTest.dll";for(unsigned i=0;dn[i];i++)dll[n++]=dn[i];dll[n]=0;
 if(GetFileAttributesW(dll)==(DWORD)-1)finish(L"[오류] 같은 폴더에 SAN14_SceneTest.dll 이 없습니다.",1);
 say(L"통합패치 R42c 시험 도구 실행기 — 게임 폴더에는 아무것도 넣지 않습니다.");
 DWORD pid=find_game();
 if(!pid){
  say(L"게임이 안 켜져 있어 Steam 으로 켭니다. 게임이 뜨면 자동으로 붙습니다(최대 5분 기다림).");
  WCHAR cmd[]=L"cmd.exe /c start \"\" steam://rungameid/872410";
  struct{DWORD cb;void*r1;void*desk;void*title;DWORD x,y,xs,ys,xc,yc,fill,flags;WORD show,r2;void*r3;HANDLE i,o,e;}si={};si.cb=sizeof si;si.flags=1;si.show=0;
  struct{HANDLE p,t;DWORD pid,tid;}pi={};
  if(CreateProcessW(0,cmd,0,0,0,0x08000000,0,0,&si,&pi)){CloseHandle(pi.p);CloseHandle(pi.t);}
  uint64_t t0=GetTickCount64();while(!(pid=find_game())&&GetTickCount64()-t0<300000)Sleep(1000);
  if(!pid)finish(L"[오류] 5분 안에 게임(SAN14CE_KO.exe)이 안 떴습니다. 게임을 켠 뒤 이 실행기를 다시 누르세요.",2);
 }
 say(L"게임을 찾았습니다. 통합패치가 올라올 때까지 기다립니다…");
 uint64_t t0=GetTickCount64();while(!module_in(pid,L"SAN14_ModCore.dll")&&GetTickCount64()-t0<300000){if(!find_game())finish(L"[오류] 게임이 꺼졌습니다.",3);Sleep(1000);}
 if(!module_in(pid,L"SAN14_ModCore.dll"))finish(L"[오류] 통합패치(ModCore)가 이 게임에서 안 켜졌습니다. 통합패치 설치를 먼저 확인하세요.",4);
 Sleep(3000);
 HANDLE ph=OpenProcess(0x0002|0x0008|0x0010|0x0020|0x0400|0x0008,0,pid); // CREATE_THREAD|VM_OPERATION|VM_READ|VM_WRITE|QUERY_INFORMATION
 if(!ph)finish(L"[오류] 게임 프로세스를 열 수 없습니다(게임을 관리자 권한으로 켰다면 이 실행기도 관리자 권한으로).",5);
 BYTE*base=module_in(pid,L"SAN14_SceneTest.dll");
 if(!base){
  UPTR bytes=(wl(dll)+1)*2;void*rem=VirtualAllocEx(ph,0,bytes,0x3000,0x04);
  if(!rem||!WriteProcessMemory(ph,rem,dll,bytes,0))finish(L"[오류] 게임에 경로를 쓰지 못했습니다.",6);
  void*ll=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
  HANDLE th=CreateRemoteThread(ph,0,0,ll,rem,0,0);if(!th)finish(L"[오류] 게임에 시험 도구를 올리지 못했습니다(백신이 막았을 수 있음).",7);
  WaitForSingleObject(th,30000);CloseHandle(th);VirtualFreeEx(ph,rem,0,0x8000);
  base=module_in(pid,L"SAN14_SceneTest.dll");if(!base)finish(L"[오류] 시험 도구 DLL 이 게임에 안 올라갔습니다.",8);
 }
 HMODULE me=LoadLibraryExW(dll,0,0x1);if(!me)finish(L"[오류] 시험 도구 DLL 을 읽지 못했습니다.",9); // DONT_RESOLVE_DLL_REFERENCES: 입구 위치만 계산
 void*fa=GetProcAddress(me,"SAN14_SceneTestAttach");UPTR rva=fa?(UPTR)fa-(UPTR)me:0;FreeLibrary(me);
 if(!rva)finish(L"[오류] 시험 도구 DLL 판이 다릅니다(SAN14_SceneTestAttach 없음).",10);
 HANDLE th=CreateRemoteThread(ph,0,0,base+rva,0,0,0);if(!th)finish(L"[오류] 시험 도구 시작을 못 했습니다.",11);
 WaitForSingleObject(th,30000);DWORD rc=0;GetExitCodeThread(th,&rc);CloseHandle(th);CloseHandle(ph);
 if(rc==1)finish(L"[완료] 게임 옆에 '통합패치 시험 도구' 창이 떴습니다. 내 턴 화면에서 버튼(또는 Ctrl+Alt+1~9)을 누르세요.",0);
 if(rc==3)finish(L"[완료] 시험 도구가 이미 붙어 있습니다(게임 옆 창 확인).",0);
 if(rc==2)finish(L"[오류] 지원하지 않는 게임 파일입니다(공식 1.0.1 한국어판만).",12);
 if(rc==4)finish(L"[오류] 통합패치(ModCore)가 안 켜진 게임입니다.",13);
 finish(L"[오류] 시험 도구 시작 결과를 알 수 없습니다.",14);
}
