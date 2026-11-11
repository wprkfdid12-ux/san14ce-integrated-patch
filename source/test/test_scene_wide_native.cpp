// 22차(P219) 턴 끝 결과 넓은 창(게임 사건 결과 창) — 리눅스 네이티브 시험기. src/scene_skip.cpp 를 그대로 컴파일해 가짜 게임 메모리 위에서 돌린다.
//  확인: ① 자리·표 확인 뒤 넓은 창 켬 ② 창 쌓기 인자(관리자·이름·{번호,0}·닫힐 때 부를 것 꼴)·지금 상태 멈춤 ③ 창이 열리면 제목·본문을 우리 글자로,
//        다른 사건 번호는 원래 0x842af0 ④ 열리기 전 단계가 다시 불리면 기다림 ⑤ 여러 장 차례로 ⑥ 3초 넘게 안 열리면 작은 창 + 오류 창 1번, 다음부터 작은 창
//        ⑦ (인자 site) 자리 바이트가 다르면 처음부터 작은 창 + 오류 창 ⑧ (인자 off) 기능 끔이면 창·오류 창 없음
//  빌드: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_scene_wide_native.cpp src/scene_skip.cpp -o /tmp/t && /tmp/t && /tmp/t site && /tmp/t off
#include "common.h"
using U64=uint64_t;
#include <sys/mman.h>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <unistd.h>
#include <sys/stat.h>
static std::string root_dir;
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
static std::wstring W(const wchar_t*w){return w?std::wstring(w):std::wstring();}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p=root_dir+"\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){mkdir(narrow(p).c_str(),0755);return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){if(h)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=11;}
static std::map<std::string,std::string> ini;
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*s,const wchar_t*k,const wchar_t*v,const wchar_t*f){ini[narrow(s)+"|"+narrow(k)]=narrow(v);return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*s,const wchar_t*k,int d,const wchar_t*){auto it=ini.find(narrow(s)+"|"+narrow(k));return it==ini.end()?(unsigned)d:(unsigned)atoi(it->second.c_str());}
static int boxes,wide_boxes;static std::string last_box;   // 가짜 게임엔 다른 묶음 자리가 없어 그 오류 창도 뜸 → 넓은 창 것만 셈
extern "C" int MessageBoxW(void*,const wchar_t*t,const wchar_t*,unsigned){std::string m=narrow(t);if(m.find("wide_window")==std::string::npos&&m.find("auto_close")==std::string::npos)return 1;boxes++;wide_boxes++;last_box=m;return 1;}
extern "C" HANDLE CreateThread(void*,size_t,DWORD(*f)(void*),void*a,DWORD,DWORD*){f(a);return nullptr;}
extern "C" HMODULE GetModuleHandleW(const wchar_t*){return nullptr;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
extern "C" HMODULE LoadLibraryExW(const wchar_t*,HANDLE,DWORD){return nullptr;}
static unsigned long long now_ms=1000;extern "C" unsigned long long GetTickCount64(void){return now_ms;}
// scene_skip_b23.cpp·bridge(.S) 대신
extern "C" bool SsEmployInstall(const S14_CoreAPI*,HookPlan&,volatile long*){return false;}
extern "C" bool SsTreasureInstall(const S14_CoreAPI*,HookPlan&,volatile long*){return false;}
extern "C" void SsDisSoundBridge(){} extern "C" void SsCamBridge(){} extern "C" void SsHistBridge(){} extern "C" void SsTurnBridge(){}
// 모듈 쪽
extern "C" int SAN14_ModInitV1(const S14_CoreAPI*);
extern "C" int SsTurnEnd(U8*,U8*);
extern "C" void SsResultFill(U8*,int);
extern "C" void SsCollect(int,const wchar_t*,int);
extern "C" long SAN14_SceneSkipWideStats(int);
// ---- 가짜 게임
static U8*game;
static void thunk(U32 rva,void*target){U8*p=game+rva;p[0]=0x48;p[1]=0xb8;memcpy(p+2,&target,8);p[10]=0xff;p[11]=0xe0;}
static U8 mgr_obj[0x100],top_obj[0x100];
struct Push{void*mgr;std::string name;int id,flag;U64 vt,cap;bool impl_local;};static std::vector<Push> pushes;
static std::vector<void*> paused;static std::vector<std::wstring> narr;static std::vector<std::pair<int,std::wstring>> texts;static std::vector<int> orig_fill;
static int busy;
static void*g_mgr(){return mgr_obj;}
static void*g_top(void*m){assert(m==mgr_obj);return top_obj;}
static void g_push(void*m,const char*name,void*arg,U64*fn){Push p;p.mgr=m;p.name=name;p.id=((int*)arg)[0];p.flag=((int*)arg)[1];p.vt=fn[0];p.cap=fn[1];p.impl_local=fn[7]==(U64)(UPTR)fn;pushes.push_back(p);}
static void g_pause(void*s){paused.push_back(s);}
static int g_busy(){return busy;}
static void g_narr(const wchar_t*t,int){narr.push_back(t);}
static U8 title_obj[8],body_obj[8];
static void g_settext(void*e,const wchar_t*t,int a,int b,int c,void*d){assert(a==1&&b==1&&c==1&&!d);texts.push_back({e==title_obj?0:e==body_obj?1:-1,t});}
static void g_fill(U8*,int id){orig_fill.push_back(id);}
static int active=1;static int g_active(U8*){return active;}
static U8 cfg_obj[0x40];static U8*g_cfg(){return cfg_obj;}
static int closes;static void g_close(U8*){closes++;}
static I32 api_readable(const void*,UPTR){return 1;}
static std::vector<std::string> groups;
static I32 api_install(const char*id,const S14_Hook*h,U32 n){for(U32 i=0;i<n;i++)if(memcmp(h[i].target,h[i].expected,h[i].length))return 0;groups.push_back(id);return 1;}
static U8*relay_next;static U8*api_relay(const void*,const void*dest){U8*r=relay_next;relay_next+=16;thunk((U32)(r-game),(void*)dest);return r;}
static void api_free(void*){}
static std::vector<std::string> core_log;static void api_log(const char*s){core_log.push_back(s);}
static int fails;
#define CHECK(c,msg) do{if(c)printf("ok   %s\n",msg);else{printf("FAIL %s\n",msg);fails++;}}while(0)
static const int kId=0x5343F001;
static std::wstring all_body(){std::wstring s;for(auto&t:texts)if(t.first==1)s+=t.second+L"\n";return s;}
int main(int argc,char**argv){
 std::string mode=argc>1?argv[1]:"";
 char tmpl[]="/tmp/ssw_XXXXXX";root_dir=mkdtemp(tmpl);
 size_t SZ=S14_GAME_IMAGE+0x10000;game=(U8*)mmap(nullptr,SZ,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(game!=MAP_FAILED);
 relay_next=game+0x29f0000;
 if(mode=="off")ini["SceneSkip|Summary"]="0";
 // 걸 자리: 턴 끝 단계·이력 입구, 결과 글자 호출 자리 0x83c793(call 0x842af0)
 static const U8 te[6]={0x40,0x53,0x48,0x83,0xec,0x20},he[5]={0x48,0x8b,0xc4,0x55,0x57},cs[5]={0xe8,0x58,0x63,0x00,0x00};
 memcpy(game+0x42ae20,te,6);memcpy(game+0x936980,he,5);memcpy(game+0x83c793,cs,5);
 // 확인 자리(실제 게임 바이트와 같게 — test_scene_wide_sites.py 가 실제 파일과 대조)
 static const U8 b1[]={0x4c,0x89,0x4c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};memcpy(game+0x832f10,b1,16);
 static const U8 b2[]={0x8b,0x97,0x70,0x04,0x00,0x00};memcpy(game+0x83c776,b2,6);
 static const U8 b3[]={0x83,0xbf,0x74,0x04,0x00,0x00,0x00};memcpy(game+0x83c783,b3,7);
 static const U8 b4[]={0x48,0x8b,0x87,0x38,0x01,0x00,0x00};memcpy(game+0x842b18,b4,7);
 static const U8 b5[]={0x48,0x8b,0x58,0x18};memcpy(game+0x842b24,b5,4);static const U8 b6[]={0x48,0x8b,0x58,0x20};memcpy(game+0x842b62,b6,4);
 memcpy(game+0x1467b48,"CEventResultState",18);
 at<U8*>(game,0x14679c8+8)=game+0x83c680;at<U8*>(game,0x13a6160)=game+0x203920;at<U8*>(game,0x13a6160+16)=game+0x830ec0;
 // 24차 메시지 속도 저절로 넘김 자리
 static const U8 a3[]={0x48,0x63,0x41,0x20},a4[]={0xff,0xff,0xff,0x7f,0x3c,0,0,0,0x23,0,0,0,0x0f,0,0,0};
 memcpy(game+0x3ec470,a3,4);memcpy(game+0x13c6248,a4,16);
 // 사건 결과 레이아웃 표(0x1467820, 52칸): vt[1] 빈 함수 0x8ca1c0, vt[51] 닫기 0x8637f0, 앞 칸 RTTI
 at<U8*>(game,0x1467820-8)=game+0x186bf48;for(int i=0;i<52;i++)at<U8*>(game,0x1467820+8*i)=game+0x900000+0x10*i;at<U8*>(game,0x1467820+8)=game+0x8ca1c0;at<U8*>(game,0x1467820+0x198)=game+0x8637f0;
 if(mode=="site")game[0x842b62+3]=0x28;
 if(mode=="autosite")game[0x13c6248+4]=0x3d;
 if(mode=="autoshape")at<U8*>(game,0x1467820+8)=game+0x123450;   // 레이아웃 표가 다른 판   // 메시지 속도 표가 다른 판   // 본문 칸 변위가 다른 판/다른 모드
 S14_Resolved res{};res.size=sizeof res;res.abi=S14_ABI;res.layout_abi=1;res.image_size=S14_GAME_IMAGE;res.image=game;
 S14_CoreAPI api{};api.size=sizeof api;api.abi=S14_ABI;api.resolved=&res;api.Readable=api_readable;api.InstallHooks=api_install;api.MakeRelay=api_relay;api.FreeRelay=api_free;api.Log=api_log;
 int r=SAN14_ModInitV1(&api);int boxes_init=boxes;boxes=0;
 // 이제 게임 함수 자리에 가짜(확인은 끝났음)
 thunk(0x11040,(void*)&g_mgr);thunk(0x572480,(void*)&g_top);thunk(0x832f10,(void*)&g_push);thunk(0x574750,(void*)&g_pause);thunk(0x335110,(void*)&g_busy);
 thunk(0x1f1e90,(void*)&g_narr);thunk(0x572670,(void*)&g_active);thunk(0x3ec1a0,(void*)&g_cfg);thunk(0x8637f0,(void*)&g_close);thunk(0x30f250,(void*)&g_settext);thunk(0x842af0,(void*)&g_fill);
 at<U8*>(game,0x27872f0)=nullptr;
 static U8 layout[0x200],tbl[0x40];at<U8*>(layout,0)=game+0x1467820;at<U8*>(layout,0x138)=tbl;at<U8*>(tbl,0x18)=title_obj;at<U8*>(tbl,0x20)=body_obj;
 auto collect8=[]{SsCollect(0,L"진양에서 역병이 맹위를 떨치고 있습니다.",0);SsCollect(0,L"장안에서 메뚜기떼 피해가 발생 중입니다.",0);SsCollect(1,L"조운이 마초 등용에 성공했습니다",1);SsCollect(1,L"조운이 황충 등용에 실패했습니다",0);SsCollect(2,L"제갈량이 양양에서 맹덕신서를 발견",0);};
 if(mode=="site"){
  CHECK(r==1&&SAN14_SceneSkipWideStats(0)==0,"자리 바이트가 다르면 넓은 창 끔(모듈은 켜짐)");
  CHECK(boxes_init==0,"시작할 때는 넓은 창 오류 창 안 띄움(기능을 쓸 때 띄움)");
  collect8();int t=SsTurnEnd(nullptr,nullptr);
  CHECK(t==1&&pushes.empty()&&narr.size()==1,"넓은 창 대신 작은 나레이션 창 1번");
  {auto L=narr[0];CHECK(L.rfind(L"이번 턴 결과\n【재해 2건】 진양",0)==0&&L.find(L"【등용 결과 2건】")!=std::wstring::npos&&L.find(L"【보물 발견 1건】")!=std::wstring::npos,"작은 창 글자: 제목 + 종류마다 한 줄");}
  CHECK(boxes==1&&last_box.find("wide_window_site")!=std::string::npos&&last_box.find("0x842b62")!=std::string::npos,"오류 창: 어느 자리·기대/실제 바이트(wide_window_site)");
  CHECK(SsTurnEnd(nullptr,nullptr)==0,"다음은 원래 단계");
  collect8();SsTurnEnd(nullptr,nullptr);CHECK(boxes==1&&narr.size()==2,"다음 턴: 작은 창, 오류 창은 다시 안 띄움");
 }else if(mode=="autosite"){
  CHECK(r==1&&SAN14_SceneSkipWideStats(0)==1&&SAN14_SceneSkipWideStats(6)==0,"메시지 속도 표가 다르면 넓은 창은 쓰고 저절로 넘김만 끔");
  collect8();CHECK(SsTurnEnd(nullptr,nullptr)==1&&pushes.size()==1&&boxes==1&&last_box.find("auto_close_site")!=std::string::npos&&last_box.find("0x13c6248")!=std::string::npos,"넓은 창 띄울 때 오류 창 1번(auto_close_site, 자리·바이트)");
  SsResultFill(layout,kId);SsTurnEnd(nullptr,nullptr);collect8();SsTurnEnd(nullptr,nullptr);CHECK(boxes==1,"다시는 안 띄움");
 }else if(mode=="autoshape"){
  collect8();CHECK(SAN14_SceneSkipWideStats(0)==1&&SAN14_SceneSkipWideStats(6)==0&&SsTurnEnd(nullptr,nullptr)==1&&boxes==1&&last_box.find("auto_close_shape")!=std::string::npos,"레이아웃 표가 다르면 저절로 넘김만 끄고 오류 창(auto_close_shape)");
  SsResultFill(layout,kId);CHECK(*(void**)layout==(void*)(game+0x1467820),"그때는 레이아웃 표 안 바꿈");
 }else if(mode=="off"){
  CHECK(r==1,"기능 끔: 모듈은 켜짐");collect8();
  CHECK(SsTurnEnd(nullptr,nullptr)==0&&pushes.empty()&&narr.empty()&&boxes==0&&boxes_init==0,"기능 끔: 창·오류 창 없음");
 }else{
  bool g=false;for(auto&x:groups)if(x=="SAN14CE.SceneSkip.TurnSummaryWide")g=true;
  CHECK(r==1&&g&&SAN14_SceneSkipWideStats(0)==1,"자리 확인 → 결과 글자 호출 자리(0x83c793) 걸고 넓은 창 켬");
  bool logged=false;for(auto&l:core_log)if(l.find("wide_result_window=on")!=std::string::npos)logged=true;CHECK(logged,"시작 기록에 wide_result_window=on");
  collect8();
  int t=SsTurnEnd(nullptr,nullptr);
  CHECK(t==1&&pushes.size()==1&&narr.empty(),"턴 끝: 넓은 창 1장 쌓고 단계 멈춤(작은 창 안 씀)");
  {Push&p=pushes[0];CHECK(p.mgr==mgr_obj&&p.name=="CEventResultState"&&p.id==kId&&p.flag==0,"쌓기 인자: 관리자 0x11040(), 이름 CEventResultState, {표시 번호, 0}");
   CHECK(p.vt==(U64)(UPTR)(game+0x13a6160)&&p.cap==(U64)(UPTR)top_obj&&p.impl_local,"닫힐 때 부를 것: 나레이션 창과 같은 꼴(0x13a6160, 지금 맨 위 상태)");
   CHECK(paused.size()==1&&paused[0]==top_obj,"쌓은 뒤 지금 상태 멈춤(0x574750)");}
  now_ms+=50;CHECK(SsTurnEnd(nullptr,nullptr)==1&&pushes.size()==1&&narr.empty(),"창이 열리기 전에 다시 불리면 기다림(새 창 없음)");
  SsResultFill(layout,kId);
  CHECK(texts.size()==2&&texts[0].first==0&&texts[0].second==L"이번 턴 결과"&&texts[1].first==1,"창이 열릴 때 제목·본문을 우리 글자로");
  {std::wstring b=texts[1].second;bool every=true;for(auto k:{L"진양에서 역병이 맹위를 떨치고 있습니다.",L"장안에서 메뚜기떼 피해가 발생 중입니다.",L"조운이 마초 등용에 성공했습니다",L"조운이 황충 등용에 실패했습니다",L"제갈량이 양양에서 맹덕신서를 발견"})if(b.find(k)==std::wstring::npos)every=false;
   CHECK(every&&b.find(L"【재해 2건】")==0,"본문: 모은 결과 전부, 종류별 머리말");}
  SsResultFill(layout,123);CHECK(orig_fill.size()==1&&orig_fill[0]==123&&texts.size()==2,"다른 사건 번호는 원래 0x842af0 그대로");
  // 26차: 메시지 속도 — 우리 창 레이아웃만 표 복사본(vt[1] = 프레임마다 부르는 함수)으로 바꿔 시간을 잼
  {auto tick=[&]{((void(*)(U8*,void*))((*(void***)layout)[1]))(layout,nullptr);};
   CHECK(SAN14_SceneSkipWideStats(6)==1,"메시지 속도 저절로 넘김 준비(표 모양 확인)");
   CHECK(*(void**)layout!=(void*)(game+0x1467820)&&(*(void***)layout)[0x198/8]==(void*)(game+0x8637f0)&&(*(void***)layout)[-1]==(void*)(game+0x186bf48)&&(*(void***)layout)[5]==(void*)(game+0x900050),"우리 창 레이아웃만 표 복사본(다른 칸·RTTI 칸은 그대로)");
   CHECK(at<void*>(game,0x1467820+8)==(void*)(game+0x8ca1c0),"원래 표는 안 건드림");
   auto open=[&]{SsCollect(0,L"장안 역병",0);CHECK(SsTurnEnd(nullptr,nullptr)==1,"(우리 창 하나 쌓음)");SsResultFill(layout,kId);};
   at<int>(cfg_obj,0x20)=0;open();now_ms+=100000;tick();CHECK(closes==0,"수동: 시간 지나도 안 닫음(확인 눌러야)");CHECK(SsTurnEnd(nullptr,nullptr)==0,"(원래 단계)");
   at<int>(cfg_obj,0x20)=3;open();
   now_ms+=1400;tick();CHECK(closes==0,"빠르게: 1.4초엔 아직(입력 없이 프레임만)");
   now_ms+=200;tick();CHECK(closes==1,"빠르게: 1.5초 지나면 입력 없이도 확인과 같은 닫기");
   now_ms+=5000;tick();CHECK(closes==1,"한 장에 한 번만");
   CHECK(SsTurnEnd(nullptr,nullptr)==0,"(닫힌 뒤 원래 단계)");
   at<int>(cfg_obj,0x20)=1;open();now_ms+=5900;tick();CHECK(closes==1,"느리게: 5.9초엔 아직");now_ms+=200;tick();CHECK(closes==2,"느리게: 6초");
   CHECK(SsTurnEnd(nullptr,nullptr)==0,"(닫힌 뒤 원래 단계)");
   texts.clear();}
  CHECK(SsTurnEnd(nullptr,nullptr)==0,"창이 닫힌 뒤: 원래 단계");
  // 여러 장
  texts.clear();for(int i=0;i<30;i++){wchar_t b[64];swprintf(b,64,L"무장%02d가 인재%02d 등용에 성공했습니다",i,i);SsCollect(1,b,1);}
  int pages=0;while(SsTurnEnd(nullptr,nullptr)==1){pages++;SsResultFill(layout,kId);if(pages>20)break;}
  {std::wstring b=all_body();bool every=true;for(int i=0;i<30;i++){wchar_t k[64];swprintf(k,64,L"무장%02d가 인재%02d 등용에 성공했습니다",i,i);if(b.find(k)==std::wstring::npos)every=false;}
   wchar_t h[40];swprintf(h,40,L"이번 턴 결과 (%d/%d)",pages,pages);
   CHECK(pages>=3&&every&&texts[texts.size()-2].second==h&&narr.empty(),"30건: 넓은 창 여러 장 차례로, 하나도 안 빠짐, 제목에 (n/전체)");}
  // 창이 안 열림(3초 넘게) → 작은 창 + 오류 창 1번
  collect8();size_t np=pushes.size();CHECK(SsTurnEnd(nullptr,nullptr)==1&&pushes.size()==np+1,"다시 쌓음");
  now_ms+=3500;CHECK(SsTurnEnd(nullptr,nullptr)==1&&narr.size()==1,"3초 넘게 안 열리면 같은 결과를 작은 창으로");
  CHECK(boxes==1&&last_box.find("wide_window_open")!=std::string::npos,"오류 창 1번(wide_window_open)");
  CHECK(SsTurnEnd(nullptr,nullptr)==0&&SAN14_SceneSkipWideStats(0)==0,"그 뒤 넓은 창 끔, 원래 단계");
  collect8();CHECK(SsTurnEnd(nullptr,nullptr)==1&&narr.size()==2&&pushes.size()==np+1&&boxes==1,"다음 턴부터 작은 창, 오류 창 다시 안 띄움");
  SsTurnEnd(nullptr,nullptr);
  {size_t nt=texts.size();SsResultFill(layout,kId);CHECK(texts.size()==nt&&SAN14_SceneSkipWideStats(4)==1&&orig_fill.size()==1,"늦게 열린 우리 번호 창은 사건표를 안 찾고 그냥 둠");}
 }
 printf(fails?"FAILED %d\n":"scene_wide: all ok\n",fails);return fails?1:0;
}
