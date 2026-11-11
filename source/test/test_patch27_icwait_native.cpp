// 25번(Patch27) 시작 기다림 — 리눅스 네이티브 시험기. 모듈 소스(src/patch27.cpp)를 그대로 컴파일해 통합편의 시작 상태별로 돌린다.
//  빌드/실행: clang++ -std=c++17 -O1 -DS14_TEST_SHIM -I test/shim27 -I src test/test_patch27_icwait_native.cpp src/patch27.cpp -o /tmp/t_p27 && /tmp/t_p27
//  확인(정식판 뒤 제보 ic_wait):
//   - 평소(통합편의 15초 안에 끝남): 예전과 똑같이 ModInit 안에서 바로 진행(뒤 작업 없음) → 통합편의가 뒤 모듈보다 먼저 끝나는 순서 그대로
//   - 15초 넘김: 창 없이 ModInit 바로 끝 → 뒤 작업이 기다리다 끝나는 즉시 25번 진행(창 없음)
//   - 10분 넘어도 안 끝남: 25번 깔림 → ic_wait 창 1번 / 25번 안 깔림 → 창 없음(기록만)
//   (창은 같은 번호·단계면 한 번만 뜨므로 창 확인 순서: 기다림 함수 없음 → 10분)
//   - 통합편의 안 실림: 예전처럼 바로 진행 / 기다림 함수 없음: 25번 깔림만 창
#include "common.h"
#include <sys/mman.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
static std::string root_dir="/tmp/t_p27_root";
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p=root_dir+"\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){mkdir(narrow(p).c_str(),0755);return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=(HANDLE)1)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=7;}
static std::vector<std::string> boxes;extern "C" int MessageBoxW(void*,const wchar_t*t,const wchar_t*,unsigned){boxes.push_back(narrow(t));return 1;}
// 시간·뒤 작업
static unsigned long long now_ms;extern "C" ULONGLONG GetTickCount64(void){return now_ms;}extern "C" void Sleep(DWORD ms){now_ms+=ms;}
static LPTHREAD_START_ROUTINE pending;static int threads;
extern "C" HANDLE CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE f,LPVOID,DWORD,LPDWORD){pending=f;threads++;return (HANDLE)1;}
extern "C" DWORD TlsAlloc(void){return 3;}extern "C" BOOL TlsFree(DWORD){return 1;}extern "C" LPVOID TlsGetValue(DWORD){return nullptr;}extern "C" BOOL TlsSetValue(DWORD,LPVOID){return 1;}
// 통합편의
static bool ic_loaded,ic_export;static int ic_ready_after;static int wait_calls;   // ic_ready_after: 몇 번째 wait() 부터 1 (0=첫 번째, <0=안 끝남)
static U32 fake_wait(){int k=wait_calls++;bool ok=ic_ready_after>=0&&k>=ic_ready_after;if(!ok)now_ms+=15000;return ok?1:0;}  // 실패 한 번 = 통합편의 쪽 한도 15초
static U8 icmod[16];
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){if(!n)return nullptr;std::string s=narrow(n);return (s=="SAN14_IntegratedConvenience.dll"&&ic_loaded)?(HMODULE)icmod:nullptr;}
extern "C" void*GetProcAddress(HMODULE,const char*n){return (ic_export&&!strcmp(n,"SAN14_WaitConvenienceV1"))?(void*)fake_wait:nullptr;}
// 25번 쪽 다른 부품(따로 시험됨)
static bool features_on;static int ui_attach,city_skip,names;
extern "C" {
 bool Patch27LoadOptions(){return true;} bool Patch27FeaturesOn(){return features_on;}
 void Patch27StartUiAttach(){ui_attach++;} bool CityEventSkipInstall(const S14_CoreAPI*){city_skip++;return true;}
 bool NamesInstall(const S14_CoreAPI*){names++;return false;}
 bool DeathLogsPlan(HookPlan*){return true;} void DeathLogsAttach(){}
 void RemoteBridge(){} void DeathBridge(){} void EmployBridgeA(){} void EmployBridgeB(){}
 long long Orig230a40(U8*,U8*,void*,U8*,void*){return 0;}
 int SAN14_ModInitV1(const S14_CoreAPI*);
}
static std::vector<std::string> logs;static void lg(const char*s){logs.push_back(s);}
static I32 readable(const void*,UPTR){return 1;}
static I32 install(const char*,const S14_Hook*,U32){return 1;}
static U8*relay(const void*,const void*){return nullptr;}static void freerelay(void*){}
static S14_Resolved res;static S14_CoreAPI api;
static int fails=0;static void check(bool c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
static bool has_log(const char*s){for(auto&l:logs)if(l.find(s)!=std::string::npos)return true;return false;}
static std::string logfile(){return root_dir+"/SAN14___/____.log";}   // 흉내 경로: 한글은 '_'
static int fail_lines(const char*want){FILE*f=fopen(logfile().c_str(),"rb");if(!f)return 0;char b[4096];int n=0;while(fgets(b,sizeof b,f))if(strstr(b,"F\t25\t")||strstr(b,"F\t25\t")){if(strstr(b,"ic_wait")&&strstr(b,want))n++;}fclose(f);return n;}
static int ic_boxes(){int n=0;for(auto&b:boxes)if(b.find("ic_wait")!=std::string::npos)n++;return n;}
static int run(bool loaded,bool exp,int ready_after,bool on){
 ic_loaded=loaded;ic_export=exp;ic_ready_after=ready_after;features_on=on;wait_calls=0;now_ms=1000;remove(logfile().c_str());pending=nullptr;threads=0;ui_attach=city_skip=names=0;boxes.clear();logs.clear();
 return SAN14_ModInitV1(&api);
}
static void worker(){if(pending){auto f=pending;pending=nullptr;f(nullptr);}}
int main(){
 mkdir(root_dir.c_str(),0755);
 U8*img=(U8*)mmap(nullptr,S14_GAME_IMAGE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 res.size=sizeof res;res.abi=S14_ABI;res.image_size=S14_GAME_IMAGE;res.image=img;
 api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=readable;api.InstallHooks=install;api.MakeRelay=relay;api.FreeRelay=freerelay;api.Log=lg;
 printf("[1] 평소: 통합편의 바로 끝남, 25번 안 깔림\n");
 {int r=run(true,true,0,false);check(r==1&&threads==0,"ModInit 안에서 바로 진행(뒤 작업 안 만듦) = 예전 순서");check(ui_attach==1&&city_skip==1,"설정 6쪽·증축 생략 연결");check(has_log("settings pages only")&&boxes.empty(),"features off 기록, 창 없음");}
 printf("[2] 평소: 통합편의 바로 끝남, 25번 깔림\n");
 {run(true,true,0,true);check(threads==0&&ui_attach==1&&names==1&&has_log("checked feature groups"),"ModInit 안에서 25번 묶음까지 진행");check(ic_boxes()==0,"ic_wait 창 없음");}
 printf("[3] 제보 상황: 통합편의가 15초 넘게 걸림(40초쯤 끝남), 25번 깔림\n");
 {int r=run(true,true,2,true);check(r==1&&threads==1&&ui_attach==0&&fail_lines("")==0,"ModInit 은 창 없이 바로 끝(게임 진행 안 막음), 뒤 작업 시작");check(boxes.empty()&&has_log("startup slow"),"그 시점 창 없음, 기록만");
  worker();check(ui_attach==1&&city_skip==1&&names==1&&has_log("checked feature groups"),"통합편의 끝나는 즉시 25번 묶음 진행");check(ic_boxes()==0&&fail_lines("")==0&&has_log("finished late"),"ic_wait 창·실패 줄 없음");}
 printf("[4] 제보 상황: 통합편의가 15초 넘게 걸림, 25번 안 깔림\n");
 {run(true,true,1,false);check(threads==1&&boxes.empty(),"ModInit 창 없음");worker();check(ui_attach==1&&has_log("settings pages only")&&boxes.empty(),"늦게라도 설정 6쪽 연결, 창 없음");}
 printf("[5] 통합편의에 기다림 함수 없음(다른 판)\n");
 {run(true,false,0,false);check(threads==0&&boxes.empty(),"25번 안 깔림: 창 없음");
  run(true,false,0,true);check(threads==0&&ic_boxes()==1,"25번 깔림: ic_wait 창(export missing)");bool em=false;for(auto&b:boxes)if(b.find("export missing")!=std::string::npos)em=true;check(em&&fail_lines("export missing")==1,"창 글자·실패 줄: export missing");}
 printf("[6] 통합편의가 10분 넘게 안 끝남, 25번 깔림\n");
 {run(true,true,-1,true);check(boxes.empty(),"ModInit 시점 창 없음");unsigned long long t0=now_ms;worker();
  check(now_ms-t0>=600000&&now_ms-t0<600000+20000,"뒤 작업이 10분 기다림");check(fail_lines("within 10 minutes")==1,"그때서야 ic_wait 실패 줄(창과 같은 글자) 1번: 10분 안에 안 끝남");check(ui_attach==0&&names==0,"25번 훅 안 건드림");}
 printf("[7] 통합편의가 10분 넘게 안 끝남, 25번 안 깔림\n");
 {run(true,true,-1,false);worker();check(boxes.empty()&&fail_lines("")==0,"창·실패 줄 없음(25번 안 깜)");check(has_log("features off")&&has_log("startup incomplete"),"기록만 남김");}
 printf("[8] 통합편의 안 실림\n");
 {int r=run(false,false,0,false);check(r==1&&threads==0&&ui_attach==1&&boxes.empty(),"예전처럼 바로 진행, 창 없음");}
 printf(fails?"\n실패 %d\n":"\n전부 통과\n",fails);return fails?1:0;
}
