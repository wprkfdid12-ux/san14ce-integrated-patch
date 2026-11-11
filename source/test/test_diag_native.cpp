// R43 종합 진단(SAN14_Diag.dll) — 리눅스 네이티브 시험(에뮬 없이). 가짜 게임 폴더(/tmp/dg_root)에 등록 파일·DLL 파일·기록을 만들고
//  모듈 소스를 그대로 돌린다. 윈도 API 는 여기서 파일/디렉터리로 흉내.
//  빌드·실행: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_diag_native.cpp -o /tmp/dg && /tmp/dg
#include "common.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sstream>
#include <glob.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
static int threads_started,boxes;static std::wstring last_box;
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
static std::string lower(std::string s){for(auto&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
static std::set<std::string> loaded_set;   // 프로세스에 로드된 DLL(소문자)
static U8*G_exe;static U8*G_ic;   // 정식판 뒤: 4) 시험에서만 통합편의 흉내 이미지
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*fn)(void*),void*arg,DWORD,DWORD*){threads_started++;fn(arg);return (HANDLE)1;}   // 바로 실행(시험)
extern "C" void Sleep(DWORD){}
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){if(!n)return (HMODULE)G_exe;if(G_ic&&lower(narrow(n))=="san14_integratedconvenience.dll")return (HMODULE)G_ic;return loaded_set.count(lower(narrow(n)))?(HMODULE)0x7777:nullptr;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
#include "../src/diag.cpp"
// ---- 파일 흉내
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*o,DWORD cap){std::string p="/tmp/dg_root\\x.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)o[n]=(wchar_t)p[n];o[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string c="mkdir -p '"+narrow(p)+"'";if(system(c.c_str())){}return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD access,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),(access&GENERIC_READ)?"rb":"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE h,void*b,DWORD n,DWORD*r,void*){*r=(DWORD)fread(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE&&h!=(HANDLE)1)fclose((FILE*)h);return 1;}
extern "C" BOOL GetFileSizeEx(HANDLE h,LARGE_INTEGER*o){long c=ftell((FILE*)h);fseek((FILE*)h,0,SEEK_END);o->QuadPart=ftell((FILE*)h);fseek((FILE*)h,c,SEEK_SET);return 1;}
extern "C" BOOL SetFilePointerEx(HANDLE h,LARGE_INTEGER d,LARGE_INTEGER*,DWORD){fseek((FILE*)h,(long)d.QuadPart,SEEK_SET);return 1;}
extern "C" DWORD GetFileAttributesW(const wchar_t*p){struct stat st;if(stat(narrow(p).c_str(),&st))return 0xffffffffu;return S_ISDIR(st.st_mode)?0x10:0x20;}
static void to_ft(time_t t,FILETIME*f){unsigned long long v=((unsigned long long)t+11644473600ull)*10000000ull;f->dwLowDateTime=(DWORD)v;f->dwHighDateTime=(DWORD)(v>>32);}
extern "C" void GetSystemTimeAsFileTime(FILETIME*f){to_ft(time(nullptr),f);}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=5;t->wHour=12;t->wMinute=0;t->wSecond=30;}
extern "C" DWORD GetEnvironmentVariableW(const wchar_t*n,wchar_t*o,DWORD cap){std::string k=narrow(n);if(k!="LOCALAPPDATA")return 0;std::string v="/tmp/dg_local";DWORD i=0;for(;i<v.size()&&i+1<cap;i++)o[i]=(wchar_t)v[i];o[i]=0;return i;}
struct Find{std::vector<std::string> names;size_t i=0;};
static void fill(Find*f,WIN32_FIND_DATAW*fd){const std::string&p=f->names[f->i];std::string base=p.substr(p.rfind('/')+1);struct stat st;stat(p.c_str(),&st);memset(fd,0,sizeof*fd);fd->dwFileAttributes=S_ISDIR(st.st_mode)?0x10:0x20;to_ft(st.st_mtime,&fd->ftLastWriteTime);for(size_t k=0;k<base.size()&&k<259;k++)fd->cFileName[k]=(wchar_t)base[k];}
extern "C" HANDLE FindFirstFileW(const wchar_t*pat,WIN32_FIND_DATAW*fd){glob_t g;std::string p=narrow(pat);if(glob(p.c_str(),0,nullptr,&g)){return INVALID_HANDLE_VALUE;}Find*f=new Find;for(size_t i=0;i<g.gl_pathc;i++)f->names.push_back(g.gl_pathv[i]);globfree(&g);if(f->names.empty()){delete f;return INVALID_HANDLE_VALUE;}fill(f,fd);return (HANDLE)f;}
extern "C" BOOL FindNextFileW(HANDLE h,WIN32_FIND_DATAW*fd){Find*f=(Find*)h;if(++f->i>=f->names.size())return 0;fill(f,fd);return 1;}
extern "C" BOOL FindClose(HANDLE h){delete (Find*)h;return 1;}
// ini: 파일을 읽어 [section] key=value
static std::string ini_get(const std::string&path,const std::string&sec,const std::string&key){std::ifstream in(path);std::string l,cur;while(std::getline(in,l)){if(!l.empty()&&l.back()=='\r')l.pop_back();if(l.empty()||l[0]==';')continue;if(l[0]=='['){cur=l.substr(1,l.find(']')-1);continue;}size_t e=l.find('=');if(e==std::string::npos)continue;if(cur==sec&&l.substr(0,e)==key)return l.substr(e+1);}return "";}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*s,const wchar_t*k,int d,const wchar_t*p){std::string v=ini_get(narrow(p),narrow(s),narrow(k));return v.empty()?(unsigned)d:(unsigned)atoi(v.c_str());}
extern "C" DWORD GetPrivateProfileStringW(const wchar_t*s,const wchar_t*k,const wchar_t*d,wchar_t*o,DWORD cap,const wchar_t*p){std::string v=ini_get(narrow(p),narrow(s),narrow(k));if(v.empty())v=narrow(d);DWORD i=0;for(;i<v.size()&&i+1<cap;i++)o[i]=(wchar_t)v[i];o[i]=0;return i;}
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){return 1;}
extern "C" int MessageBoxW(void*,const wchar_t*t,const wchar_t*,unsigned){boxes++;last_box=t;return 1;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
// ---- 도우미
static int pass_n,fail_n;
#define CHECK(c,m) do{if(c){pass_n++;printf("통과 %s\n",m);}else{fail_n++;printf("실패 %s\n",m);}}while(0)
static void put(const std::string&rel,const std::string&text,bool append=false){std::string p="/tmp/dg_root/"+rel;std::string d=p.substr(0,p.rfind('/'));std::string c="mkdir -p '"+d+"'";if(system(c.c_str())){}std::ofstream o(p,append?std::ios::app:std::ios::trunc);o<<text;}
static void put_utf16(const std::string&rel,const std::wstring&text){std::string p="/tmp/dg_root/"+rel;std::ofstream o(p,std::ios::binary);for(wchar_t c:text){char b[2]={(char)(c&0xff),(char)(c>>8)};o.write(b,2);}}
static void reg(const std::string&ini,const std::string&file,int enabled=1){put("SAN14Mods/modules/"+ini,"[Mod]\nId=x\nFile="+file+"\nEnabled="+std::to_string(enabled)+"\nCoreABI=65536\n");put(file,"MZ");}
static std::string logtext(){std::string s;FILE*g=popen("cat /tmp/dg_root/SAN14___/*.log 2>/dev/null","r");char b[4096];size_t n;while(g&&(n=fread(b,1,sizeof b,g))>0)s.append(b,n);if(g)pclose(g);return s;}
static std::vector<std::string> r43d(){std::vector<std::string> v;std::istringstream in(logtext());std::string l;while(std::getline(in,l))if(l.rfind("R43D\t",0)==0)v.push_back(l);return v;}
static bool has(const std::wstring&h,const wchar_t*n){return h.find(n)!=std::wstring::npos;}
static S14_Resolved res;static S14_CoreAPI api;
static I32 readable(const void*,UPTR){return 1;}static I32 inst(const char*,const S14_Hook*,U32){return 1;}static U8*relay(const void*,const void*){return nullptr;}static void frel(void*){}
static void reset_state(){nprob=0;n_reg=n_loaded=n_started=0;}
int main(){
 if(system("rm -rf /tmp/dg_root /tmp/dg_local && mkdir -p /tmp/dg_root/SAN14Mods/modules /tmp/dg_local/SAN14CE_ModCore")){}
 static U8 img[0x200];memset(img,0,sizeof img);img[0]='M';img[1]='Z';*(U32*)(img+0x3c)=0x100;*(U32*)(img+0x100)=0x4550;*(U32*)(img+0x108)=S14_GAME_STAMP;*(U32*)(img+0x150)=S14_GAME_IMAGE;G_exe=img;
 res.size=sizeof res;res.abi=S14_ABI;res.image_size=S14_GAME_IMAGE;res.image=img;api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=readable;api.InstallHooks=inst;api.MakeRelay=relay;api.FreeRelay=frel;
 // ---- 1) 전부 정상: 등록 모듈 로드 + 시작 줄 + runtime 문구 → 문제 0, 창 없음
 reg("Diag.ini","SAN14_Diag.dll");reg("HiredFace.ini","SAN14_HiredFace.dll");reg("DomesticPlayer.ini","SAN14_DomesticPlayer.dll");reg("FPS120.ini","SAN14_FPS120.dll");reg("DomesticCE.ini","SAN14_DomesticCE.dll");
 reg("IntegratedConvenience_GreenlandCompat.ini","SAN14_IntegratedConvenience.dll");reg("Patch27.ini","SAN14_Patch27.dll");reg("StatusColor.ini","SAN14_StatusColor.dll",0);   // StatusColor 는 Enabled=0 → 무시
 for(const char*d:{"san14_diag.dll","san14_hiredface.dll","san14_domesticplayer.dll","san14_fps120.dll","san14_domesticce.dll","san14_integratedconvenience.dll","san14_patch27.dll"})loaded_set.insert(d);
 put("SAN14___/____.log","R39\t32\tSAN14_HiredFace.dll\t2026-11-05 11:59:40\tOK\t4/4\tEnabled=1\t-\nR42D\t22\tSAN14_DomesticPlayer.dll\tstart\t2026-11-05 11:59:41\tOK\t1/1\tAllDomestic=1 PopFloor=0\t-\n");
 {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log");o<<"old session\nFPS120 R8 ACTIVE\nSAN14 ModCore 1.0 / standalone\nFPS120 R8 ACTIVE\nDomesticCE loaded: CT-compatible all-region automation\nDomesticCE started: all domestic\nPatch27 R22: features off (item 25 not installed); settings pages only, no game hooks.\n";}
 put_utf16("SAN14CE_11Patch.log",L"\xFEFF통합편의 시작\r\n지원 실행 파일 확인 완료\r\n");
 int b0=boxes;int r=SAN14_ModInitV1(&api);
 CHECK(r==1&&boxes==b0,"전부 정상이면 창 없음(25번 미설치·1번만 → Patch27 features off 도 정상)");
 CHECK(SAN14_DiagState(0)==0&&SAN14_DiagState(1)==7&&SAN14_DiagState(2)==7&&SAN14_DiagState(3)==7,"등록 7(Enabled=0 제외)·로드 7·시작 확인 7·문제 0");
 {auto v=r43d();CHECK(v.size()==1&&v[0].find("\tcheck30\tregistered=7 loaded=7 started=7 problems=0")!=std::string::npos,"R43D check30 줄(문제 0 이면 90초 확인 안 함)");}
 CHECK(logtext().find("R39\t0\tSAN14_Diag.dll\t")!=std::string::npos&&logtext().find("\tOK\t1/1\tFirstCheckSec=30 FinalCheckSec=90")!=std::string::npos,"Diag 자기 시작 줄");
  // 1b) 정식판 뒤(제보 ic_wait): 통합편의가 늦어 25번이 아직 뒤에서 기다리는 중 → 문제 아님(10분 넘으면 25번이 직접 창)
 {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log");o<<"old session\nFPS120 R8 ACTIVE\nSAN14 ModCore 1.0 / standalone\nFPS120 R8 ACTIVE\nDomesticCE loaded: CT-compatible all-region automation\nDomesticCE started: all domestic\nPatch27: common module startup slow (>15 s); waiting in background up to 10 minutes.\n";}
 reset_state();{int b1=boxes;int r1=SAN14_ModInitV1(&api);CHECK(r1==1&&boxes==b1&&SAN14_DiagState(0)==0&&SAN14_DiagState(3)==7,"25번이 통합편의를 뒤에서 기다리는 중(startup slow)이면 창 없음·시작 확인 7");}
 {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log");o<<"old session\nFPS120 R8 ACTIVE\nSAN14 ModCore 1.0 / standalone\nFPS120 R8 ACTIVE\nDomesticCE loaded: CT-compatible all-region automation\nDomesticCE started: all domestic\nPatch27 R22: features off (item 25 not installed); settings pages only, no game hooks.\n";}   // 원래 내용으로 되돌림
 // ---- 2) 문제 여럿: 파일 없음 / 로드 안 됨 / 시작 줄 없음 / 시작 상태 FAIL / DomesticCE READ-ONLY / runtime 문구 없음 / 통합편의 기록 오래됨 / 묶음 DLL 파일만 있음 / R43F 줄
 reg("Range.ini","SAN14_Range.dll");if(system("rm /tmp/dg_root/SAN14_Range.dll")){}                 // 파일 없음
 reg("SceneSkip.ini","SAN14_SceneSkip.dll");                                                            // 파일 있는데 로드 안 됨
 reg("RegionOfficer.ini","SAN14_RegionOfficer.dll");loaded_set.insert("san14_regionofficer.dll");          // 로드됐는데 시작 줄 없음
 reg("DomesticAI.ini","SAN14_DomesticAI.dll");loaded_set.insert("san14_domesticai.dll");                   // 시작 상태 FAIL
 reg("MeritTraits.ini","SAN14_MeritTraits.dll");loaded_set.insert("san14_merittraits.dll");                // runtime 에 실패 줄
 reg("CityCaps.ini","SAN14_CityCaps.dll");loaded_set.insert("san14_citycaps.dll");                         // runtime 문구 없음
 put("SAN14Mods/Minimap/SAN14_FogTrial.dll","MZ");                                                      // 파일만 있고 로드 안 됨(27번)
 put("SAN14___/____.log","R42D\t22\tSAN14_DomesticAI.dll\tstart\t2026-11-05 11:59:50\tFAIL\t0/1\tAiRotate=1\tcall site bytes differ\n"
  "R43F\t22\tSAN14_DomesticAI.dll\t2026-11-05 11:59:50\thook\tAI domestic focus call site exe+0x7b17b (bytes differ) planned=0\te8 c0 18 04 00\te8 11 18 04 00\t_\tR43F-22-hook\tstamp=S14_GAME_STAMP size=S14_GAME_IMAGE (=R43 base)\n"
  "R39\t34\tSAN14_RegionOfficer.dll\t2026-11-05 10:00:00\tOK\t2/2\tWarning=1\t-\n",true);   // RegionOfficer 줄은 옛 실행(10시) → 안 침
 {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log",std::ios::app);o<<"DomesticCE: READ-ONLY mode\nMeritTraits: merit awards not enabled - existing Hover renderer fingerprint MISMATCH at 0x494431\n";}
 {struct utimbuf u;u.actime=u.modtime=time(nullptr)-3600;utime("/tmp/dg_root/SAN14CE_11Patch.log",&u);}  // 통합편의 기록 1시간 전
 reset_state();b0=boxes;r=SAN14_ModInitV1(&api);
 CHECK(r==1&&boxes==b0+1,"문제가 있으면 창 1개");
 std::wstring B=last_box;
 CHECK(SAN14_DiagState(0)==9,"문제 9건(파일없음·로드안됨·시작줄없음·FAIL·READ-ONLY·13번실패줄·23번문구없음·통합편의오래됨·안개미로드)");
 CHECK(has(B,L"33번 안행·누선/투석 사거리: DLL 파일이 게임 폴더에 없음")&&has(B,L"SAN14_Range.dll file missing"),"파일 없음 → 설치기 다시");
 CHECK(has(B,L"4번 연출 생략")&&has(B,L"로드되지 않음")&&has(B,L"SAN14_SceneSkip.dll not in process"),"파일 있는데 로드 안 됨");
 CHECK(has(B,L"34번 지역담당관 공석 경고: 로드는 됐는데 이번 실행 시작 기록이 없음")&&has(B,L"no start line for SAN14_RegionOfficer.dll since 2026-11-05 11:59:00"),"옛 실행 줄은 안 침 → 시작 기록 없음");
 CHECK(has(B,L"22번 AI 내정 순환(군주 반영): 시작은 했지만 상태가 실패/일부")&&has(B,L"start state=FAIL"),"시작 상태 FAIL");
 CHECK(has(B,L"22번 내정 자동화(DomesticCE): 읽기 전용 모드")&&has(B,L"READ-ONLY"),"DomesticCE READ-ONLY");
 CHECK(has(B,L"13번 논공 개성 가챠: 13번 시작 실패")&&has(B,L"merit awards not enabled - existing Hover renderer fingerprint MISMATCH"),"13번 실패 줄 그대로 근거");
 CHECK(has(B,L"23번 도시 자원·병력 상한: 로드는 됐는데 성공 기록이 없음")&&has(B,L"lacks 'CityCaps: city limits extended'"),"runtime 문구 없음");
 CHECK(has(B,L"1번 통합편의(1~7번 공통 부품): 통합편의 기록(SAN14CE_11Patch.log)이 이번 실행에 안 써짐"),"통합편의 기록 오래됨");
 CHECK(has(B,L"27번 독립 미니맵(FogTrial): 파일은 있는데 프로세스에 로드되지 않음"),"묶음 DLL 파일만 있음");
 CHECK(has(B,L"이미 따로 오류창을 띄운 실패(R43F):")&&has(B,L"22번 SAN14_DomesticAI.dll 단계=hook 코드=R43F-22-hook"),"R43F 줄 요약");
 CHECK(has(B,L"문제 9건")&&has(B,L"(= 통합패치 기준판)")&&has(B,L"진단 코드: R43D-9")&&has(B,L"Ctrl+C"),"머리글·판·코드·복사 안내");
 {auto v=r43d();CHECK(v.size()==4&&v[2].find("\tcheck30\t")!=std::string::npos&&v[3].find("\tcheck90\tregistered=13 loaded=11 started=5 problems=9")!=std::string::npos,"R43D check30+check90 줄(문제 목록 포함; 앞 1·1b 실행 줄 2개)");}
 // ---- 3) 끔
 put("SAN14Mods/modules/Diag.ini","[Mod]\nId=x\nFile=SAN14_Diag.dll\nEnabled=1\n[Diag]\nEnabled=0\n");reset_state();b0=boxes;int t0=threads_started;r=SAN14_ModInitV1(&api);
 CHECK(r==1&&boxes==b0&&threads_started==t0&&logtext().find("\tOFF\t0/0\tEnabled=0")!=std::string::npos,"Diag Enabled=0 이면 안 돌고 OFF 줄");
 // ---- 4) 정식판 뒤(11-08 제보): 통합편의가 게임 코드 원본 확인에서 막힘(다른 모드가 먼저 고침) → 원인 한 줄 + 기다리던 26번·1번 보강은 그 줄에 묶음 + 큰 제목 '다른 모드와 충돌'
 //        + 2번 자동인사 '구조/비용 확인 실패' 2번은 문제 아님
 {if(system("rm -rf /tmp/dg_root /tmp/dg_local && mkdir -p /tmp/dg_root/SAN14Mods/modules /tmp/dg_local/SAN14CE_ModCore")){}
  const size_t MB=1u<<20;U8*M=(U8*)mmap(nullptr,96*MB,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);memset(M,0x90,96*MB);
  U8*EXE=M,*IC=M+48*MB,*FOR=M+80*MB;memcpy(EXE,img,0x200);G_exe=EXE;res.image=EXE;G_ic=IC;
  static const U8 wc[12]={0x48,0x83,0xec,0x28,0xc7,0x44,0x24,0x20,0xdc,0x05,0x00,0x00};memcpy(IC+0x1f8120,wc,12);
  uint64_t v=0x1f473;memcpy(IC+0xb7310,&v,8);U32 l3=3,eo=0;memcpy(IC+0xb7318,&l3,4);memcpy(IC+0xb731c,&eo,4);for(unsigned i=1;i<0x92a;i++){uint64_t z=0x100;U32 zl=0;memcpy(IC+0xb7310+16*i,&z,8);memcpy(IC+0xb7318+16*i,&zl,4);}
  U8 e3[3]={0x48,0x8b,0x05};memcpy(IC+0xb5710,e3,3);
  uint64_t f0=0x174cd,f5=5;memcpy(IC+0xdedd0,&f0,8);memcpy(IC+0xdedd0+0x18,&f5,8);U32 flag=0;memcpy(IC+0x1fa000,&flag,4);
  // 게임 원본 확인 자리 0x1f473 을 다른 모드가 jmp 로 바꿈
  U8*site=EXE+0x1f473;site[0]=0xE9;int32_t rel=(int32_t)((FOR+0x4321)-(site+5));memcpy(site+1,&rel,4);
  // 51곳 표는 첫 칸만(원래 모양으로 두기)
  {U8*st=EXE+0x174cd;st[0]=0xE8;int32_t rr=(int32_t)(0-(0x174cd+5));memcpy(st+1,&rr,4);}
  auto&h=r43c::th();h.exe_lo=EXE;h.exe_hi=EXE+S14_GAME_IMAGE;
  h.readable=[](const void*p,size_t n){const U8*a=(const U8*)p;return a>=G_exe&&a+n<=G_exe+96*(1u<<20);};
  h.module_of=[](const void*p,wchar_t*nm,size_t cap,const U8**base){const U8*a=(const U8*)p;const wchar_t*name=nullptr;const U8*b=nullptr;
   if(a>=G_exe&&a<G_exe+S14_GAME_IMAGE){name=L"SAN14CE_KO.exe";b=G_exe;}else if(a>=G_ic&&a<G_ic+(4u<<20)){name=L"SAN14_IntegratedConvenience.dll";b=G_ic;}else if(a>=G_exe+80*(1u<<20)&&a<G_exe+81*(1u<<20)){name=L"AI_Tactics_X.dll";b=G_exe+80*(1u<<20);}
   if(!name)return false;size_t k=0;for(;name[k]&&k+1<cap;k++)nm[k]=name[k];nm[k]=0;if(base)*base=b;return true;};
  static std::wstring sm,sb,st;static int nshow;nshow=0;h.show=[](const wchar_t*t,const wchar_t*m,const wchar_t*c){nshow++;st=t;sm=m;sb=c;};
  loaded_set.clear();for(const char*d:{"san14_diag.dll","san14_integratedconvenience.dll","san14_icguard.dll","san14_executelock.dll"})loaded_set.insert(d);
  reg("Diag.ini","SAN14_Diag.dll");reg("IntegratedConvenience_GreenlandCompat.ini","SAN14_IntegratedConvenience.dll");reg("ICGuard.ini","SAN14_ICGuard.dll");reg("ExecuteLock.ini","SAN14_ExecuteLock.dll");
  put_utf16("SAN14CE_11Patch.log",L"\xFEFF2026-11-05 12:00:01 지원 실행 파일 확인 완료. 게임 코드 복원을 기다립니다.\r\n2026-11-05 12:00:20 자동인사: 포상 0명 / 투자 0곳 / 논공 0명 / 탐색 0명 (시도 0) / 명령서 21 / 상태 구조/비용 확인 실패.\r\n2026-11-05 12:00:40 자동인사: 포상 0명 / 투자 0곳 / 논공 0명 / 탐색 0명 (시도 0) / 명령서 21 / 상태 구조/비용 확인 실패.\r\n");
  put("SAN14CE_ModCore_placeholder","x");
  {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log");o<<"SAN14 ModCore 1.0\n";}
  reset_state();int b0=boxes;int r=SAN14_ModInitV1(&api);
  std::wstring P;for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(r==1&&nprob==1,"문제 1건(통합편의 원본 확인 막힘)만 — 26번·1번 보강은 따로 안 셈");
  CHECK(has(P,L"1번 통합편의(1~7번 공통 부품): 통합편의가 게임 코드 원본 확인을 아직 못 끝냄")&&has(P,L"그래서 아직 시작 못 한 기능:")&&has(P,L"처단 버튼")&&has(P,L"보강"),"원인 한 줄 + 기다리던 기능 이름 묶음");
  CHECK(has(P,L"→ 다른 모드와 충돌: AI_Tactics_X.dll (exe+0x1f473)"),"문제 줄 끝에 주인(다른 모드)·자리");
  CHECK(!has(P,L"2번 턴 종료 자동"),"2번 '구조/비용 확인 실패' 2번은 문제 아님");
  CHECK(nshow==1&&boxes==b0&&sm==L"다른 모드와 충돌 — AI_Tactics_X.dll"&&has(st,L"다른 모드와 충돌"),"큰 제목 창: 다른 모드와 충돌 — 이름(예전 창 아님)");
  CHECK(has(sb,L"■ 원인")&&has(sb,L"통합편의 원본 확인 자리 exe+0x1f473: AI_Tactics_X.dll (다른 모드) 이(가) 먼저 고침")&&has(sb,L"통합패치 고장이 아닙니다")&&has(sb,L"시작 종합 진단 — 문제 1건"),"본문: 원인 단락 + 원래 진단 글자");
  // 원본 확인이 끝난 경우(flag=1)는 예전대로: 기다리던 모듈은 시작 줄이 없으면 각자 문제
  flag=1;memcpy(IC+0x1fa000,&flag,4);reset_state();nshow=0;SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(!has(P,L"원본 확인을 아직 못 끝냄")&&has(P,L"이번 실행 시작 기록이 없음"),"원본 확인 끝났으면 예전 판정 그대로");
  // 2번 줄 3번 + 성공 턴 없음 → 문제
  put_utf16("SAN14CE_11Patch.log",L"\xFEFF2026-11-05 12:00:01 지원 실행 파일 확인 완료.\r\n2026-11-05 12:00:20 자동인사: 포상 0명 / 투자 0곳 / 논공 0명 / 탐색 0명 (시도 0) / 명령서 21 / 상태 구조/비용 확인 실패.\r\n2026-11-05 12:00:30 자동인사: 포상 0명 / 투자 0곳 / 논공 0명 / 탐색 0명 (시도 0) / 명령서 21 / 상태 구조/비용 확인 실패.\r\n2026-11-05 12:00:40 자동인사: 포상 0명 / 투자 0곳 / 논공 0명 / 탐색 0명 (시도 0) / 명령서 21 / 상태 구조/비용 확인 실패.\r\n");
  reset_state();SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"2번 턴 종료 자동 포상·투자·논공·자동인사")&&has(P,L"(같은 줄 3번)"),"2번 줄 3번 이상 + 성공 턴 없음 → 문제");   // 시험기는 근거 한글을 ? 로 적음
  // 5번 수행 실패 줄 + 그 자리(51곳 표 첫 칸)를 다른 모드가 바꿈 / 9번 화면 부품 HOVER_SKIPPED + 개성 칸을 14번(TraitHover)이 차지 → 통합패치끼리
  {U8*st=EXE+0x174cd;st[0]=0xE9;int32_t rr=(int32_t)((FOR+0x55)-(st+5));memcpy(st+1,&rr,4);}
  for(const auto&e:kR43T9Slots){uint64_t fn=(uint64_t)(EXE+e[1]);memcpy(EXE+e[0],&fn,8);}
  {uint64_t th=(uint64_t)(EXE+84*MB+0x2dd0);memcpy(EXE+0x13affa8,&th,8);}
  h.module_of=[](const void*p,wchar_t*nm,size_t cap,const U8**base){const U8*a=(const U8*)p;const wchar_t*name=nullptr;const U8*b=nullptr;const size_t M1=1u<<20;
   if(a>=G_exe&&a<G_exe+S14_GAME_IMAGE){name=L"SAN14CE_KO.exe";b=G_exe;}else if(a>=G_ic&&a<G_ic+4*M1){name=L"SAN14_IntegratedConvenience.dll";b=G_ic;}
   else if(a>=G_exe+80*M1&&a<G_exe+81*M1){name=L"AI_Tactics_X.dll";b=G_exe+80*M1;}else if(a>=G_exe+84*M1&&a<G_exe+85*M1){name=L"SAN14_TraitHover.dll";b=G_exe+84*M1;}
   if(!name)return false;size_t k=0;for(;name[k]&&k+1<cap;k++)nm[k]=name[k];nm[k]=0;if(base)*base=b;return true;};
  put_utf16("SAN14CE_11Patch.log",L"\xFEFF2026-11-05 12:00:01 지원 실행 파일 확인 완료.\r\n2026-11-05 12:00:02 수행 호환성 검사 실패. 수행 버튼과 추적 후크를 적용하지 않았습니다.\r\n");
  put("SAN14_Traits9_UI.log","SAN14 Traits9 UI v2.6: HOVER_SKIPPED - native table/duel handler conflict; nine-trait UI remains active.\r\n");loaded_set.insert("san14_traits9_ui.dll");
  reset_state();nshow=0;SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"→ 다른 모드와 충돌: AI_Tactics_X.dll (exe+0x174cd)"),"5번 수행 실패 줄 끝에 그 자리 주인(다른 모드)");
  CHECK(has(P,L"→ 통합패치 기능끼리 겹침: SAN14_TraitHover.dll (exe+0x13affa8)"),"9번 HOVER_SKIPPED 줄 끝에 개성 칸 주인(14번 = 통합패치끼리)");
  CHECK(nshow==1&&sm==L"다른 모드와 충돌 — AI_Tactics_X.dll"&&has(sb,L"9번 개성 칸 처리 자리 exe+0x13affa8: SAN14_TraitHover.dll (통합패치 파일) 이(가) 먼저 고침"),"큰 제목은 다른 모드, 본문에 둘 다");
  // R43 11-08 제보 '5번 진행 배속: 진행속도 확장 준비 검사 실패' — 번호는 6번(설치기 번호) + 그 자리(함수표 칸 0x1476e58)를 다른 모드가 차지 → 주인
  {uint64_t a=(uint64_t)(FOR+0x99),b2=(uint64_t)(EXE+0x896620),c=(uint64_t)(EXE+0x895910);memcpy(EXE+0x1476e58,&a,8);memcpy(EXE+0x1476e68,&b2,8);memcpy(EXE+0x1476e80,&c,8);}
  {U8*st=EXE+0x174cd;st[0]=0xE8;int32_t rr=(int32_t)(0-(0x174cd+5));memcpy(st+1,&rr,4);}   // 5번 자리는 원래대로
  put_utf16("SAN14CE_11Patch.log",L"\xFEFF2026-11-05 12:00:01 지원 실행 파일 확인 완료.\r\n2026-11-05 12:00:02 진행속도 확장 준비 검사 실패. 원래 세 단계 유지.\r\n2026-11-05 12:00:03 BGM 확장: 호환성 검사 실패. BGM 후크 미적용.\r\n");
  put("SAN14_Traits9_UI.log","ok\r\n");
  reset_state();nshow=0;SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"6번 진행 배속: 통합편의 기록")&&!has(P,L"5번 진행 배속"),"진행속도 실패 줄 = 6번(옛 표는 5번으로 잘못 붙였음)");
  CHECK(has(P,L"8번 BGM 추가·플레이어"),"BGM 실패 줄 = 8번");
  CHECK(has(P,L"→ 다른 모드와 충돌: AI_Tactics_X.dll (exe+0x1476e58)")&&nshow==1&&sm==L"다른 모드와 충돌 — AI_Tactics_X.dll","6번 진행 배속 실패 줄 끝에 그 자리 주인 + 큰 제목");
  {uint64_t a=(uint64_t)(EXE+0x880570);memcpy(EXE+0x1476e58,&a,8);}reset_state();nshow=0;SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"6번 진행 배속")&&!has(P,L"다른 모드와 충돌"),"자리가 원래대로면 주인 없이 실패 줄만");
  // P208(2026-11-10, gggoeik R43D-5): 13번 실패(9칸 화면이 못 붙음)일 때 9번 개성 칸을 가진 다른 모드 DLL 이름을 줄 끝에
  reg("MeritTraits.ini","SAN14_MeritTraits.dll");loaded_set.insert("san14_merittraits.dll");
  {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log",std::ios::app);o<<"SAN14CE.MeritTraits.HoverX RVA=00494431 len=5\nMeritTraits: merit awards not enabled - nine-trait UI prerequisite failed\n";}
  reset_state();nshow=0;SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"13번 시작 실패")&&!has(P,L"13번 시작 실패(runtime.log 'MeritTraits:' 줄 참조) →"),"13번 실패 + 개성 칸이 전부 원래/통합패치 것 → DLL 이름 안 붙임");
  {uint64_t f=(uint64_t)(FOR+0x77);memcpy(EXE+kR43T9Slots[5][0],&f,8);}
  reset_state();nshow=0;SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  {bool same=false;size_t a=0;while(a<P.size()){size_t e=P.find(L'\n',a);if(e==std::wstring::npos)e=P.size();std::wstring ln=P.substr(a,e-a);if(ln.find(L"13번 시작 실패")!=std::wstring::npos&&ln.find(L"→ 다른 모드와 충돌: AI_Tactics_X.dll (exe+0x")!=std::wstring::npos)same=true;a=e+1;}
  CHECK(same,"13번 실패 줄 끝에 9번 개성 칸을 가진 다른 모드 이름(제보만 보고 누구와 충돌인지 알 수 있게)");}
  CHECK(nshow==1&&has(sm,L"다른 모드와 충돌")&&has(sm,L"AI_Tactics_X.dll"),"큰 제목 '다른 모드와 충돌 — 이름'");
  {uint64_t fn=(uint64_t)(EXE+kR43T9Slots[5][1]);memcpy(EXE+kR43T9Slots[5][0],&fn,8);}
  std::remove("/tmp/dg_local/SAN14Mods/modules/MeritTraits.ini");loaded_set.erase("san14_merittraits.dll");{std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log");o<<"SAN14 ModCore 1.0\n";}
  G_ic=nullptr;h=r43c::TestHooks{};}
 // ---- 5) 정식판 뒤(11-08 갤 제보 '9칸 안 깔았는데 9칸 오류'): 남은 옛 파일은 숨기지 않고 원인(옛 파일 남음)과 정리 방법으로 알림
 {if(system("rm -rf /tmp/dg_root /tmp/dg_local && mkdir -p /tmp/dg_root/SAN14Mods/modules /tmp/dg_local/SAN14CE_ModCore /tmp/dg_root/.san14ce_compat_r3")){}
  G_exe=img;res.image=img;loaded_set.clear();for(const char*d:{"san14_diag.dll","san14_citycaps.dll"})loaded_set.insert(d);
  reg("Diag.ini","SAN14_Diag.dll");reg("CityCaps.ini","SAN14_CityCaps.dll");
  static const char*inst="R43";
  auto state=[](std::initializer_list<int> on){std::string x=std::string("\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<SAN14Compat version=\"7\" installer=\"")+inst+"\">\r\n  <Selected>\r\n";
   for(int i=1;i<=34;i++){bool b=false;for(int k:on)if(k==i)b=true;x+="    <Feature id=\""+std::to_string(i)+"\" enabled=\""+(b?"true":"false")+"\" />\r\n";}x+="  </Selected>\r\n</SAN14Compat>\r\n";put(".san14ce_compat_r3/state.xml",x);};
  state({1,2,3,4,5,6,7});   // 9·13·23 안 고름
  put("SAN14_Traits9.log","SAN14 Traits9: SKIPPED - unsupported game version; no code changed.\r\n");
  {std::ofstream o("/tmp/dg_local/SAN14CE_ModCore/runtime.log");o<<"SAN14 ModCore 1.0\nCityCaps: city limits extended\n";}
  reset_state();int b0=boxes;SAN14_ModInitV1(&api);std::wstring P;for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(boxes>b0&&has(P,L"9번 개성 9칸: 설치 안 한 기능의 옛 판 파일이 게임 폴더에 남아 실행됨")&&has(P,L"선택 설치"),"안 고른 9번 옛 파일 기록 → 창은 뜨고 '옛 파일 남음·선택 설치로 정리' 안내");
  CHECK(has(P,L"23번 도시 자원·병력 상한: 설치 안 한 기능의 옛 파일(등록 파일·DLL)이 남아 실행 중"),"안 고른 23번 등록 파일이 남아 실행 중 → 옛 파일 남음");
  state({1,2,3,4,5,6,7,9,23});reset_state();SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(!has(P,L"설치 안 한 기능")&&has(P,L"9번 개성 9칸: 자기 기록(.log)에 실패 줄"),"고른 경우는 예전 판정(9번 실패 줄) 그대로");
  state({1,2,3,13,23});reset_state();SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"13번 개성 9칸(13번 공용 부품)"),"13번만 깐 사람은 '13번 공용 부품'으로 표시");
  // R43.29: R43.28 부터 설치기 판이 'R43.차수' — 같은 번호 체계이므로 믿어야 함(R43.28 에서 진단이 고른 항목을 못 읽던 것)
  inst="R43.28";state({1,2,3,4,5,6,7});reset_state();SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(has(P,L"9번 개성 9칸: 설치 안 한 기능의 옛 판 파일이 게임 폴더에 남아 실행됨"),"설치기 판 R43.28 기록도 믿음(고른 항목으로 판정)");
  CHECK(inst_ver[0]&&!strcmp(inst_ver,"R43.28"),"진단이 설치기 판(R43.28)을 읽어 창에 적음");
  inst="R43";
  put(".san14ce_compat_r3/state.xml","<SAN14Compat version=\"6\" installer=\"R42\"><Selected><Feature id=\"1\" enabled=\"true\" /></Selected></SAN14Compat>");reset_state();SAN14_ModInitV1(&api);P.clear();for(unsigned i=0;i<nprob;i++){P+=probs[i].text;P+=L"\n";}
  CHECK(!has(P,L"설치 안 한 기능")&&has(P,L"9번 개성 9칸: 자기 기록(.log)에 실패 줄"),"옛 설치기 기록(번호 다를 수 있음)은 안 믿고 예전 판정");}
 printf("\n종합 진단 R43 네이티브 시험: 통과 %d / 실패 %d\n",pass_n,fail_n);return fail_n?1:0;
}
