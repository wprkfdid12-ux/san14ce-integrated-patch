// R43 8차(사용자 11-09 "9·13·14번이 서로 기생하지 말고 따로 작동하게" · figekth 'T9 B ... Traits9 dependency absent') — 9번이 스스로 9칸을 켜는지(리눅스 네이티브).
//  빌드/실행: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_traits9_independent_native.cpp -o /tmp/t9i && /tmp/t9i
//  가짜 본체(SAN14_Traits9.dll): SAN14_ApplyTraits9 = InitOnce{5자리 5→9 → 화면 부품 Apply}. 가짜 화면 부품: 5자리가 9가 아니면 SKIPPED(한 번만 시도, 실제 v2.6 과 같음).
//  확인: ① 등록 이름 순서: MeritTraits.ini < Mod09_Traits9Check.ini < ModPackBridge.ini
//        ② 9번만 / 9+14 / 14번 꺼 둠 — 다 같이: 본체 먼저 → 화면 부품 붙음(5/5, 화면 on) + 기록 줄, 창 없음  (14번 설정을 아예 안 읽음)
//        ③ 13번이 먼저 켬: 본체 ALREADY, 화면 부품 그대로(두 번 안 붙음)  ④ 뒤에 통합편의가 또 부름: 아무 일 없음
//        ⑤ 본체 파일/내보내기 없음: 9번 실패 창 1번  ⑥ 설정은 새 이름 우선, 없으면 옛 이름
#include "common.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdio>
#define INVALID_FILE_ATTRIBUTES 0xffffffffu
#define PAGE_EXECUTE_READWRITE 0x40
#define FILE_SHARE_DELETE 4
static std::set<std::string> files;static int traithover=1,bridge_enabled=1;
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
static std::string rel(const wchar_t*p){std::string s=narrow(p);std::string r="/tmp/t9e_root/";return s.rfind(r,0)==0?s.substr(r.size()):s;}
extern "C" DWORD GetFileAttributesW(const wchar_t*p){return files.count(rel(p))?0x20:INVALID_FILE_ATTRIBUTES;}
extern "C" BOOL VirtualProtect(void*,size_t,DWORD,DWORD*){return 0;}
extern "C" BOOL FlushInstructionCache(HANDLE,const void*,size_t){return 1;}
extern "C" HANDLE GetCurrentProcess(){return 0;}
extern "C" void Sleep(DWORD){}
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*)(void*),void*,DWORD,DWORD*){return (HANDLE)1;}
#include "../src/traits9_check.cpp"
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p="/tmp/t9e_root\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string c="mkdir -p '"+narrow(p)+"'";if(system(c.c_str())){}return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE&&h!=(HANDLE)1)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=8;}
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){return 1;}
static std::vector<std::string> ini_reads;
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*sec,const wchar_t*key,int d,const wchar_t*p){std::string f=rel(p),k=narrow(key),s=narrow(sec);ini_reads.push_back(f+":"+s+"."+k);
 if(f.find("ModPackBridge.ini")!=std::string::npos){if(k=="TraitHover")return (unsigned)traithover;if(k=="Enabled")return (unsigned)bridge_enabled;}return (unsigned)d;}
static int boxes;extern "C" int MessageBoxW(void*,const wchar_t*t,const wchar_t*,unsigned){boxes++;(void)t;return 1;}
static bool core_loaded,has_core=true,core_once,ui_once,ui_skipped;static int core_calls,ui_attach;static std::string loaded_path;static U8 core_mod[16];
static std::vector<U8> img(0x500000);
static void ui_apply(){if(ui_once)return;ui_once=true;   // 실제 화면 부품: InitOnce, 5자리가 9 여야 붙음
 if(nine_sites()!=5){ui_skipped=true;return;}U8*g=img.data()+0x494431;g[0]=0xff;g[1]=0x25;memset(g+2,0,4);ui_attach++;}
static void core_apply(){core_calls++;if(core_once)return;core_once=true;
 for(const Site&s:kSites)memcpy(img.data()+s.rva,s.after,s.len);ui_apply();}
extern "C" HMODULE GetModuleHandleW(const wchar_t*n){return nullptr;}
extern "C" HMODULE LoadLibraryExW(const wchar_t*p,HANDLE,DWORD){loaded_path=narrow(p);if(!files.count(rel(p)))return nullptr;core_loaded=true;return (HMODULE)core_mod;}
extern "C" void*GetProcAddress(HMODULE,const char*n){return (has_core&&!strcmp(n,"SAN14_ApplyTraits9"))?(void*)core_apply:nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
// ---- 가짜 게임
static std::vector<std::string> logs;static void lg(const char*s){logs.push_back(s);}
static I32 rd_ok(const void*,UPTR){return 1;}
static int fails=0;static void check(bool c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
static std::string up(std::string s){for(auto&c:s)c=(char)toupper((unsigned char)c);return s;}
static void reset(){files={"SAN14_Traits9.dll","SAN14_Traits9_UI.dll","SAN14Mods/ModPack/SAN14_TraitHover.dll"};traithover=1;bridge_enabled=1;core_loaded=false;has_core=true;core_once=ui_once=ui_skipped=false;core_calls=ui_attach=0;
 loaded_path.clear();logs.clear();boxes=0;ini_reads.clear();std::fill(img.begin(),img.end(),0);
 for(const Site&s:kSites){U8*p=img.data()+s.rva;memcpy(p,s.after,s.len);for(int k=0;k<s.len;k++)if(p[k]==9)p[k]=5;}}   // 원래 5칸(9 → 5)
static bool logged(){for(auto&l:logs)if(l.find("started by module 9 itself (slots=5/5 ui=on)")!=std::string::npos)return true;return false;}
static bool read_14(){for(auto&r:ini_reads)if(r.find("ModPackBridge")!=std::string::npos)return true;return false;}
int main(){
 printf("[1] 등록 이름 순서\n");
 {std::vector<std::string> v={"ModPackBridge.ini","Mod09_Traits9Check.ini","MeritTraits.ini","ModSettings.ini","IntegratedConvenience_GreenlandCompat.ini","Traits9Check.ini"};
  std::sort(v.begin(),v.end(),[](const std::string&a,const std::string&b){return up(a)<up(b);});
  auto at=[&](const char*n){return std::find(v.begin(),v.end(),n)-v.begin();};
  check(at("MeritTraits.ini")<at("Mod09_Traits9Check.ini")&&at("Mod09_Traits9Check.ini")<at("ModPackBridge.ini"),"MeritTraits < Mod09_Traits9Check < ModPackBridge");}
 r39::game_root(root,1024);
 static S14_Resolved res;static S14_CoreAPI api;res.size=sizeof res;res.abi=S14_ABI;res.image_size=S14_GAME_IMAGE;res.image=img.data();
 api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Log=lg;api.Readable=rd_ok;core=&api;game=img.data();
 printf("[2] 9번 혼자 켬(14번 있음/없음/꺼 둠 모두 같은 결과)\n");
 {reset();check(nine_sites()==0&&!ui_hooked(),"시작 전: 5칸, 화면 부품 없음");}
 {reset();early_nine();check(core_calls==1&&loaded_path.find("SAN14_Traits9.dll")!=std::string::npos,"본체 SAN14_Traits9.dll 의 SAN14_ApplyTraits9 1번");
  check(nine_sites()==5&&ui_hooked()&&ui_attach==1&&!ui_skipped,"본체 5자리 먼저 → 화면 부품 붙음(figekth SKIPPED 재발 없음)");check(logged()&&boxes==0,"기록 줄, 창 없음");check(!read_14(),"14번 설정 안 읽음(14번에 안 기댐)");}
 {reset();files.erase("SAN14Mods/ModPack/SAN14_TraitHover.dll");early_nine();check(nine_sites()==5&&ui_hooked()&&boxes==0,"14번 없음: 똑같이 켜짐");}
 {reset();traithover=0;bridge_enabled=0;early_nine();check(nine_sites()==5&&ui_hooked()&&boxes==0,"14번 꺼 둠: 똑같이 켜짐");}
 printf("[3] 13번이 먼저 켬\n");
 {reset();core_apply();int a=ui_attach;early_nine();check(nine_sites()==5&&ui_hooked()&&ui_attach==a&&boxes==0,"본체 ALREADY · 화면 부품 두 번 안 붙음");}
 printf("[4] 뒤에 통합편의가 또 부름\n");
 {reset();early_nine();core_apply();check(ui_attach==1&&nine_sites()==5&&boxes==0,"InitOnce — 아무 일 없음");}
 printf("[5] 본체 없음\n");
 {reset();files.erase("SAN14_Traits9.dll");early_nine();check(boxes==1&&core_calls==0,"파일 없음 → 9번 실패 창 1번");}
 {reset();has_core=false;early_nine();check(boxes<=1&&core_calls==0&&!ui_hooked(),"내보내기 없음 → 안 부름(같은 자리 실패 창은 한 번만 — 앞에서 떴음)");}
 printf("[6] 설정 파일 이름\n");
 {wchar_t p[1200];reset();files.insert("SAN14Mods/modules/Mod09_Traits9Check.ini");ini_path(p);check(narrow(p).find("Mod09_Traits9Check.ini")!=std::string::npos,"새 이름 우선");
  files.erase("SAN14Mods/modules/Mod09_Traits9Check.ini");ini_path(p);check(narrow(p).find("modules/Traits9Check.ini")!=std::string::npos,"없으면 옛 이름");}
 printf(fails?"\n실패 %d\n":"\n전부 통과\n",fails);return fails?1:0;
}
