// 시험 전용(본판 아님): scene_skip_b23.cpp 를 감싸는 시험 DLL. SsAddHistory 는 기록만, 게임 메시지 함수는 가짜(stub)로.
#include "common.h"
#include "r39_record.h"
extern "C" bool SsEmployInstall(const S14_CoreAPI*api,HookPlan&plan,volatile long*enabled);
extern "C" bool SsTreasureInstall(const S14_CoreAPI*api,HookPlan&plan,volatile long*enabled);
extern "C" void*SsB23Table(int i);
static volatile long en_emp[1]={1},en_tre[1]={1};
static wchar_t hist[8][512];static volatile long hist_n;
static void wcpy(wchar_t*o,const wchar_t*s,int cap){int i=0;for(;s[i]&&i<cap-1;i++)o[i]=s[i];o[i]=0;}
extern "C" void SsAddHistory(const wchar_t*line){long n=hist_n++;wcpy(hist[n&7],line,512);}
extern "C" void SsCollect(int,const wchar_t*,int){}
extern "C" __declspec(dllexport) void*T_Hist(int i){return i<0?(void*)&hist_n:(void*)hist[i&7];}
extern "C" __declspec(dllexport) void T_SetEn(int which,int v){(which?en_tre:en_emp)[0]=v;}
extern "C" __declspec(dllexport) void*T_Table(int i){return SsB23Table(i);}
// ── 가짜 게임 메시지 함수 ──
static wchar_t args[256];
static void addc(wchar_t c){int n=0;while(args[n])n++;if(n<254){args[n]=c;args[n+1]=0;}}
static void addn(unsigned v){wchar_t b[12];int k=0;do{b[k++]=(wchar_t)(L'0'+v%10);v/=10;}while(v);while(k)addc(b[--k]);}
static void rec(wchar_t tag,unsigned v,unsigned slot){addc(tag);addn(v);addc(L'@');addn(slot);addc(L',');}
extern "C" __declspec(dllexport) void S_Reset(void*){args[0]=0;}
extern "C" __declspec(dllexport) void S_Person(void*,U32 id,U32 slot){rec(L'P',id,slot);}
extern "C" __declspec(dllexport) void S_Item(void*,U8*p,U32 slot){rec(L'I',*(U16*)p,slot);}
extern "C" __declspec(dllexport) void S_Area(void*,U32 id,U32 slot){rec(L'A',id,slot);}
extern "C" __declspec(dllexport) void S_City(void*,U32 id,U32 slot){rec(L'C',id,slot);}
static wchar_t tmpl[64];static volatile long tmpl_null;
extern "C" __declspec(dllexport) void T_TmplNull(int v){tmpl_null=v;}
extern "C" __declspec(dllexport) const wchar_t*S_Tmpl(void*,U32 id){if(tmpl_null)return nullptr;tmpl[0]=L'T';int k=1;wchar_t b[12];int n=0;do{b[n++]=(wchar_t)(L'0'+id%16+(id%16>9?7:0));id/=16;}while(id);while(n)tmpl[k++]=b[--n];tmpl[k++]=L'\n';tmpl[k]=0;return tmpl;}
static int fmtobj;
extern "C" __declspec(dllexport) void*S_FmtObj(){return &fmtobj;}
extern "C" __declspec(dllexport) void S_Fmt(void*f,wchar_t*out,U32 cap,const wchar_t*pct,const wchar_t*t){
 out[0]=0;if(f!=&fmtobj||!pct||pct[0]!=L'%'||pct[1]!=L's')return;int k=0;
 for(int i=0;t[i]&&k<(int)cap-2;i++)out[k++]=t[i];out[k++]=L'{';for(int i=0;args[i]&&k<(int)cap-2;i++)out[k++]=args[i];out[k++]=L'}';out[k]=0;}
static U8 force_obj[0x20],city_obj[0x60];
extern "C" __declspec(dllexport) U8*S_Force(U8*){return force_obj;}
extern "C" __declspec(dllexport) U32 S_Lord(U8*f){return f==force_obj?77:0;}
extern "C" __declspec(dllexport) U32 S_AreaId(U8*){return 55;}
extern "C" __declspec(dllexport) U8*S_CityOf(U8*){*(U16*)(city_obj+0x4e)=33;return city_obj;}
extern "C" __declspec(dllexport) int SAN14_ModInitV1(const S14_CoreAPI*api){
 if(!valid_api(api))return 0;HookPlan plan(api);
 bool a=SsEmployInstall(api,plan,en_emp),b=SsTreasureInstall(api,plan,en_tre);
 return a&&b&&plan.install("SAN14CE.SceneSkip.Test")?1:0;
}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
