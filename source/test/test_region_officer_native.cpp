// 34번 지역담당관 공석 경고 R42f2 — 리눅스 네이티브 시험(에뮬 없이)
//  게임 함수(무장 유효·플레이어 세력·지역 소유·거점 지역·스타일 바꾸기)와 글자 칸 색 읽기(vf16)를 가짜 메모리 jmp 로 이어 RoFrame 을 그대로 부른다.
//  확인: 공석 0.3초 뒤 빨강 / 이미 빨강이면 색을 읽어 빨강일 땐 안 칠함·지워졌으면 다시 / 못 읽으면 5초마다 / 채워지면 원래 색
//        돌려쓴 판에 남은 빨강 지우기(fresh_fix) / 10초 넘게 안 보인 기억 칸 다시 쓰기(stale) / 꽉 차면 그 판만 건너뜀 / 5분 stats 줄
//  빌드·실행: clang++ -std=c++17 -O1 -w -I test/shim -I src test/test_region_officer_native.cpp -o /tmp/ro && /tmp/ro
#include "common.h"
#include <sys/mman.h>
#include <string>
#include <vector>
#include <cassert>
static unsigned long long tick;extern "C" unsigned long long GetTickCount64(){return tick;}
extern "C" DWORD GetCurrentThreadId(){return 1;}
#include "../src/region_officer.cpp"
extern "C"{void RoFrameC(){}void RoFrameF(){}}
static std::string narrow(const wchar_t*w){std::string s;for(;*w;w++)s+=(char)(*w<128?*w:'_');for(auto&c:s)if(c=='\\')c='/';return s;}
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*out,DWORD cap){std::string p="/tmp/ro_root\\SAN14CE_KO.exe";DWORD n=0;for(;n<p.size()&&n+1<cap;n++)out[n]=(wchar_t)p[n];out[n]=0;return n;}
extern "C" BOOL CreateDirectoryW(const wchar_t*p,void*){std::string c="mkdir -p '"+narrow(p)+"'";if(system(c.c_str())){}return 1;}
static std::vector<std::string> lines;
extern "C" HANDLE CreateFileW(const wchar_t*p,DWORD,DWORD,void*,DWORD,DWORD,void*){FILE*f=fopen(narrow(p).c_str(),"ab");return f?(HANDLE)f:INVALID_HANDLE_VALUE;}
extern "C" BOOL WriteFile(HANDLE h,const void*b,DWORD n,DWORD*w,void*){lines.push_back(std::string((const char*)b,n));*w=(DWORD)fwrite(b,1,n,(FILE*)h);return 1;}
extern "C" BOOL ReadFile(HANDLE,void*,DWORD,DWORD*,void*){return 0;}
extern "C" BOOL CloseHandle(HANDLE h){if(h&&h!=INVALID_HANDLE_VALUE)fclose((FILE*)h);return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof*t);t->wYear=2026;t->wMonth=11;t->wDay=3;}
extern "C" BOOL WritePrivateProfileStringW(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){return 1;}
extern "C" unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,int d,const wchar_t*){return (unsigned)d;}
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){return 1;}
extern "C" HMODULE GetModuleHandleW(const wchar_t*){return nullptr;}
extern "C" void*GetProcAddress(HMODULE,const char*){return nullptr;}
extern "C" LONG BCryptOpenAlgorithmProvider(void*,const wchar_t*,void*,unsigned){return -1;}
extern "C" LONG BCryptCreateHash(void*,void*,void*,unsigned,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptHashData(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptFinishHash(void*,void*,unsigned,unsigned){return -1;}
extern "C" LONG BCryptDestroyHash(void*){return 0;}
extern "C" LONG BCryptCloseAlgorithmProvider(void*,unsigned){return 0;}
// ---- 가짜 게임
static U8*D,*FORCE_ME=(U8*)calloc(1,64),*FORCE_OTHER=(U8*)calloc(1,64),*PERSON=(U8*)calloc(1,0x200);
static U8*g_me(U8*){return FORCE_ME;}
static U8*owner_of_area[600];static U8*g_owner(U8*a){U8*base=*(U8**)(D+0x6caf0);return owner_of_area[(a-base)/0x98];}
static int g_valid(U8*){return 1;}
static U8*g_foot_area(U8*d){return *(U8**)(d);}
static int sets,last_style;static int cur_style[2]; // 칸마다 지금 스타일(거점형/도시형 구분 없이 판 하나씩 시험)
static U8*colors;   // 스타일 표 흉내(game+0x1a26e60 에 씀)
static int color_mode; // 0 정상 읽기 1 못 읽음
static void g_set(U8*group,int child,int style){sets++;last_style=style;U8*el=(*(U8***)(group+8))[child];at<int>(el,0x100)=style;}
static U8*g_color(U8*el,int){if(color_mode)return nullptr;return colors+(size_t)at<int>(el,0x100)*0x30;}
static void jmp_to(U8*at_,void*f){U8 c[12]={0x48,0xb8};memcpy(c+2,&f,8);c[10]=0xff;c[11]=0xe0;memcpy(at_,c,12);}
static unsigned long rd_calls;static I32 rdbl(const void*p,UPTR){rd_calls++;return p!=nullptr;}
static S14_CoreAPI api;
struct Plate{U8*layout,*sub,*el;};
static void*vt_el[0x20];
static Plate mk_plate(U8*area){Plate p;p.layout=(U8*)calloc(1,0x300);*(U8**)p.layout=game+VT_CAP;at<U8*>(p.layout,0x1f0)=area;
 p.sub=(U8*)calloc(1,0x200);*(U8**)p.sub=game+VT_CAPTEXT;at<U8*>(p.layout,0x208)=p.sub;
 U8**arr=(U8**)calloc(0x20,8);p.el=(U8*)calloc(1,0x200);*(void***)p.el=vt_el;at<U8>(p.el,0x112)=2;at<int>(p.el,0x100)=0x13;arr[0x16]=p.el;
 at<U8**>(p.sub+0x130,8)=arr;at<int>(p.sub+0x130,0x10)=0x20;return p;}
int main(){
 if(system("mkdir -p /tmp/ro_root")){}
 game=(U8*)mmap(0,0x2a00000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE,-1,0);
 D=(U8*)calloc(1,0x90000);*(U8**)(game+0x2780f60)=D;
 U8*areas=(U8*)calloc(501,0x98);*(U8**)(D+0x6caf0)=areas;for(int i=1;i<=500;i++)*(U8**)(D+0x6caf0+i*8)=areas+i*0x98;
 *(U8**)(D+0x871e8)=(U8*)calloc(1,64);*(U8**)(D+0xdf30)=FORCE_OTHER;*(U8**)(D+0x158+7*8)=PERSON;
 jmp_to(game+0x330b90,(void*)g_me);jmp_to(game+0x22e260,(void*)g_owner);jmp_to(game+0x331780,(void*)g_valid);jmp_to(game+0x22b880,(void*)g_foot_area);jmp_to(game+0x9314a0,(void*)g_set);
 colors=game+0x1a29e60;/* R43 새 판 스타일 표 */for(int s=0;s<0x4e;s++)for(int k=0;k<16;k++)colors[s*0x30+k]=(U8)(s*7+k);
 jmp_to(game+0x500000,(void*)g_color);vt_el[0x80/8]=(void*)(game+0x500000);
 api.Readable=rdbl;core=&api;
 for(int i=1;i<=500;i++)owner_of_area[i]=FORCE_ME;
 U8*A=areas+5*0x98;at<U16>(A,0x36)=0; // 공석
 Plate p=mk_plate(A);tick=100000;
 auto frame=[&](Plate&q,int ms){for(int t=0;t<ms;t+=16){tick+=16;RoFrame(q.layout,0);}};
 frame(p,200);assert(at<int>(p.el,0x100)==0x13);           // 0.3초 전: 아직 흰색
 frame(p,300);assert(at<int>(p.el,0x100)==0x14);           // 0.3초 뒤: 빨강
 int s0=sets;frame(p,5000);assert(sets==s0);              // 빨강 그대로면 다시 안 칠함(예전: 1초마다)
 at<int>(p.el,0x100)=0x13;frame(p,1200);assert(at<int>(p.el,0x100)==0x14&&reapplied>=1); // 게임이 색을 되돌리면 1초 안에 다시 빨강
 color_mode=1;s0=sets;frame(p,4000);int s1=sets;frame(p,2000);color_mode=0;assert(s1-s0<=1&&sets-s1<=1&&color_unk_n>0); // 못 읽으면 5초마다만
 at<U16>(A,0x36)=7;frame(p,400);assert(at<int>(p.el,0x100)==0x13);   // 담당관 채움 → 흰색
 // 돌려쓴 판에 남은 빨강(우리가 안 칠함) → 원래 색으로
 // 시험12: 남은 빨강 확인은 판이 다른 지역으로 바뀐 뒤 한 번만(매 0.15초 전부 확인하던 것이 1~2 FPS 원인)
 {U8*B=areas+9*0x98;at<U16>(B,0x36)=7;at<U8*>(p.layout,0x1f0)=B;at<int>(p.el,0x100)=0x14;frame(p,400);assert(at<int>(p.el,0x100)==0x13&&restored_fresh==1);
  // 같은 판·같은 지역인데 바뀌지 않았으면 더는 메모리 확인 안 함(색 읽기 횟수 0)
  int s2=sets;at<int>(p.el,0x100)=0x14;frame(p,3000);assert(sets==s2&&restored_fresh==1);at<int>(p.el,0x100)=0x13;at<U8*>(p.layout,0x1f0)=A;frame(p,400);}
 // 시험12: 판 343개(실게임 live=343)·담당관 채움·매 프레임 3초 → 메모리 확인(VirtualQuery) 횟수. 이전 판은 0.15초마다 판마다 6번(약 4만 번)
 {std::vector<Plate> v;for(int i=0;i<343;i++){U8*a=areas+(100+i)*0x98;at<U16>(a,0x36)=7;v.push_back(mk_plate(a));}
  for(int t=0;t<20;t+=16){tick+=16;for(auto&q:v)RoFrame(q.layout,0);}
  unsigned long r0=rd_calls;for(int t=0;t<3000;t+=16){tick+=16;for(auto&q:v)RoFrame(q.layout,0);}
  printf("판 343개 첫 3초: 메모리 확인 %lu번(처음 한 번씩)\n",rd_calls-r0);assert(rd_calls-r0<=343*6);
  r0=rd_calls;for(int t=0;t<3000;t+=16){tick+=16;for(auto&q:v)RoFrame(q.layout,0);}printf("판 343개 다음 3초: %lu번\n",rd_calls-r0);assert(rd_calls-r0==0);}
 // 기억 칸: 판 5000개를 한 번씩 → 꽉 찰 수 있음(그 판만 건너뜀), 10초 뒤엔 오래된 칸 다시 씀
 std::vector<Plate> many;for(int i=0;i<5000;i++){U8*a=areas+(1+i%500)*0x98;many.push_back(mk_plate(a));}
 for(auto&q:many){tick+=1;RoFrame(q.layout,0);}
 unsigned full0=cache_full;tick+=11000;for(auto&q:many){tick+=1;RoFrame(q.layout,0);}
 printf("판 5000개: 꽉 참 %u→%u, 오래된 칸 다시 씀 %u\n",full0,cache_full,stale_reuse);assert(stale_reuse>0);
 // 5분 stats 줄
 tick+=400000;RoFrame(p.layout,0);tick+=10;RoFrame(p.layout,0);
 bool st=false;for(auto&l:lines)if(l.find("R42F\t34\tSAN14_RegionOfficer.dll\tstats\tlive=")!=std::string::npos){st=true;printf("기록: %s",l.c_str());}
 assert(st);
 printf("통과: 34번 R42f2 시험 (칠함 %d번, 다시 칠함 %u, 색 못 읽음 %u)\n",paints,reapplied,color_unk_n);
 return 0;
}
