#pragma once
// R43 시험11 표본기 — Windows 부분(diag.cpp 에서만 포함). 설명은 diag_perf.h 머리말.
#include "diag_perf.h"
namespace perfw{
using namespace perf;
typedef intptr_t LPARAM_;typedef unsigned long ULONG;
typedef HANDLE (WINAPI*PSnap)(DWORD,DWORD);
struct ME32{DWORD dwSize,th32ModuleID,th32ProcessID,GlblcntUsage,ProccntUsage;BYTE*modBaseAddr;DWORD modBaseSize;HMODULE hModule;WCHAR szModule[256];WCHAR szExePath[260];};
struct TE32{DWORD dwSize,cntUsage,th32ThreadID,th32OwnerProcessID;LONG tpBasePri,tpDeltaPri;DWORD dwFlags;};
struct MBI{PVOID BaseAddress,AllocationBase;DWORD AllocationProtect;WORD PartitionId;SIZE_T RegionSize;DWORD State,Protect,Type;};
typedef BOOL (WINAPI*PM32)(HANDLE,ME32*);typedef BOOL (WINAPI*PT32)(HANDLE,TE32*);
typedef HANDLE (WINAPI*POpenThread)(DWORD,BOOL,DWORD);typedef DWORD (WINAPI*PSusp)(HANDLE);typedef BOOL (WINAPI*PGetCtx)(HANDLE,void*);
typedef SIZE_T (WINAPI*PVQ)(LPCVOID,MBI*,SIZE_T);typedef BOOL (WINAPI*PThrTimes)(HANDLE,FILETIME*,FILETIME*,FILETIME*,FILETIME*);
typedef LONG (WINAPI*PNtQIT)(HANDLE,int,void*,ULONG_PTR,ULONG*);
typedef BOOL (WINAPI*PEnumWin)(BOOL (WINAPI*)(HWND,LPARAM_),LPARAM_);
static PSnap pSnap;static PM32 pM32F,pM32N;static PT32 pT32F,pT32N;static POpenThread pOpenThread;static PSusp pSuspend,pResume;static PGetCtx pGetCtx;static PVQ pVQ;static PThrTimes pTimes;static PNtQIT pNtQIT;
typedef DWORD (WINAPI*PGWTPI)(HWND,DWORD*);typedef BOOL (WINAPI*PIsVis)(HWND);typedef HWND (WINAPI*PGetWin)(HWND,unsigned);
static PGWTPI pGWTPI;static PIsVis pIsVis;
static wchar_t logpath[1100];
static Mod mods[600];static int nmods;
struct Set{Tab<4096> rip,c1,pair,mod1;DWORD tid;unsigned ns,nsys,ngame,nmod,nfail;HANDLE h;};
static Set sets[4];static int nsets;   // [0]=메인, [1..3]=이번 회차 직전 1초 CPU 가 가장 많은 다른 스레드
static unsigned char stk[32768];
static char out[24576];static size_t olen;
static void o(const char*s){while(*s&&olen+1<sizeof out)out[olen++]=*s++;out[olen]=0;}
static void oh(u64 v){char t[20];int n=0;do{int d=(int)(v&15);t[n++]=(char)(d<10?'0'+d:'a'+d-10);v>>=4;}while(v);o("0x");char r[20];for(int i=0;i<n;i++)r[i]=t[n-1-i];r[n]=0;o(r);}
static void od(u64 v){char t[24];int n=0;do{t[n++]=(char)('0'+v%10);v/=10;}while(v);char r[24];for(int i=0;i<n;i++)r[i]=t[n-1-i];r[n]=0;o(r);}
static void flush(){HANDLE f=CreateFileW(logpath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_ALWAYS,0,0);if(f!=INVALID_HANDLE_VALUE){DWORD w=0;WriteFile(f,out,(DWORD)olen,&w,0);CloseHandle(f);}olen=0;out[0]=0;}
static bool ieq_tail(const WCHAR*path,const wchar_t*part){ // path 안에 part(소문자 비교)
 size_t m=0;while(part[m])m++;for(size_t i=0;path[i];i++){size_t j=0;while(j<m&&path[i+j]){WCHAR a=path[i+j];if(a>='A'&&a<='Z')a+=32;if(a!=part[j])break;j++;}if(j==m)return true;}return false;}
struct Rg{u64 lo,hi;};static Rg rg[8192];static int nrg;
static bool rd_ok(u64 lo,u64 hi){int a=0,b=nrg-1;while(a<=b){int m=(a+b)/2;if(lo<rg[m].lo)b=m-1;else if(lo>=rg[m].hi)a=m+1;else return hi<=rg[m].hi;}return false;}
static void load_ranges(){nrg=0;for(int i=0;i<nmods;i++){if(mods[i].sys)continue;u64 a=mods[i].base;while(a<mods[i].end&&nrg<8192){MBI mb;if(!pVQ((LPCVOID)a,&mb,sizeof mb))break;u64 e=(u64)mb.BaseAddress+mb.RegionSize;if(e<=a)break;
  DWORD pr=mb.Protect&0xff;bool ok=mb.State==MEM_COMMIT&&!(mb.Protect&0x100)&&pr!=0x01&&pr!=0;   // NOACCESS·GUARD 제외
  if(ok){u64 lo=a,hi=e>mods[i].end?mods[i].end:e;if(nrg&&rg[nrg-1].hi==lo)rg[nrg-1].hi=hi;else{rg[nrg].lo=lo;rg[nrg].hi=hi;nrg++;}}a=e;}}}
static void load_mods(){nmods=0;HANDLE s=pSnap(0x18,0);if(s==INVALID_HANDLE_VALUE)return;ME32 e;e.dwSize=sizeof e;
 HMODULE exe=GetModuleHandleW(0);
 for(BOOL ok=pM32F(s,&e);ok&&nmods<600;ok=pM32N(s,&e)){Mod&m=mods[nmods++];m.base=(u64)e.modBaseAddr;m.end=m.base+e.modBaseSize;m.game=(e.hModule==exe);
  m.sys=!m.game&&ieq_tail(e.szExePath,L"\\windows\\");int k=0;for(;e.szModule[k]&&k<63;k++)m.name[k]=(char)(e.szModule[k]<128?e.szModule[k]:'_');m.name[k]=0;e.dwSize=sizeof e;}
 CloseHandle(s);sort_mods(mods,nmods);load_ranges();}
static void oaddr(u64 a){int mi=find_mod(mods,nmods,a);if(mi<0){oh(a);return;}o(mods[mi].name);o("+");oh(a-mods[mi].base);
 if(mods[mi].sys){u64 off=0;const char*nm=near_export((const u8*)mods[mi].base,mods[mi].end-mods[mi].base,a,&off);if(nm){o(" (");char t[64];int k=0;while(nm[k]&&k<63){t[k]=nm[k];k++;}t[k]=0;o(t);o("+");oh(off);o(")");}}}
// 메인 스레드 = 게임 프로세스의 보이는 최상위 창 스레드
static DWORD g_main;static DWORD g_pid;
static BOOL WINAPI enum_cb(HWND h,LPARAM_){DWORD pid=0;DWORD tid=pGWTPI(h,&pid);if(pid==g_pid&&pIsVis(h)){g_main=tid;return FALSE;}return TRUE;}
static u64 ft(const FILETIME&f){return ((u64)f.dwHighDateTime<<32)|f.dwLowDateTime;}
struct Thr{DWORD tid;u64 cpu0,cpu1,k0,k1;u64 start;};
static Thr thr[512];static int nthr;
static void thr_snap(bool first){HANDLE s=pSnap(4,0);if(s==INVALID_HANDLE_VALUE)return;TE32 e;e.dwSize=sizeof e;
 for(BOOL ok=pT32F(s,&e);ok;ok=pT32N(s,&e)){if(e.th32OwnerProcessID==g_pid){HANDLE h=pOpenThread(0x0800|0x0040,FALSE,e.th32ThreadID);
   if(h){FILETIME c,x,k,u;if(pTimes(h,&c,&x,&k,&u)){int i=0;for(;i<nthr;i++)if(thr[i].tid==e.th32ThreadID)break;
     if(i==nthr&&first&&nthr<512){thr[nthr].tid=e.th32ThreadID;thr[nthr].cpu1=0;thr[nthr].k1=0;thr[nthr].start=0;if(pNtQIT){u64 sa=0;pNtQIT(h,9,&sa,8,0);thr[nthr].start=sa;}nthr++;}
     if(i<nthr){if(first){thr[i].cpu0=ft(k)+ft(u);thr[i].k0=ft(k);}else{thr[i].cpu1=ft(k)+ft(u);thr[i].k1=ft(k);}}}
    CloseHandle(h);}}e.dwSize=sizeof e;}
 CloseHandle(s);}
static void sample_one(Set&S){
 alignas(16) static unsigned char ctx[1232];
 memset(ctx,0,sizeof ctx);*(DWORD*)(ctx+0x30)=0x100003;   // CONTROL|INTEGER
 size_t n=0;bool ok=false;
 if(pSuspend(S.h)!=(DWORD)-1){
  if(pGetCtx(S.h,ctx)){u64 rsp=*(u64*)(ctx+0x98);MBI mb;if(pVQ((LPCVOID)rsp,&mb,sizeof mb)&&mb.State==MEM_COMMIT){u64 end=(u64)mb.BaseAddress+mb.RegionSize;n=end>rsp?(size_t)(end-rsp):0;if(n>sizeof stk)n=sizeof stk;memcpy(stk,(const void*)rsp,n);}ok=true;}
  pResume(S.h);}
 if(!ok){S.nfail++;return;}
 S.ns++;u64 rip=*(u64*)(ctx+0xF8),r10=*(u64*)(ctx+0xC8),rdx=*(u64*)(ctx+0x88);
 int mi=find_mod(mods,nmods,rip);if(mi>=0&&mods[mi].sys)S.nsys++;else if(mi>=0&&mods[mi].game)S.ngame++;else S.nmod++;
 S.rip.add(rip,(r10&0xffffffffull)|(rdx<<32));
 u64 cs[4];int gi=-1;int k=callers(stk,n,mods,nmods,cs,4,false,&gi,rd_ok);
 u64 c1=(mi>=0&&!mods[mi].sys)?rip:(k>0?cs[0]:0),c2=(mi>=0&&!mods[mi].sys)?(k>0?cs[0]:0):(k>1?cs[1]:0);
 if(c1){S.c1.add(c1);S.pair.add(c1*1000003ull^c2,c1,c2);}
 u64 m1=0;if(mi>=0&&!mods[mi].sys&&!mods[mi].game)m1=rip;else{u64 cm[1];if(callers(stk,n,mods,nmods,cm,1,true,0,rd_ok)==1)m1=cm[0];}
 if(m1)S.mod1.add(m1);}
static void cpu_table(unsigned secs){
 o("-- 스레드 CPU(");od(secs);o("초 동안, ms: 합계/커널) 상위\r\n");
 unsigned idx[12];int nt=0;for(int i=0;i<nthr;i++){if(!thr[i].cpu1)continue;u64 d=thr[i].cpu1-thr[i].cpu0;int j=nt<12?nt++:12;if(j==12){u64 dl=thr[idx[11]].cpu1-thr[idx[11]].cpu0;if(d<=dl)continue;j=11;}idx[j]=(unsigned)i;
  while(j>0&&(thr[idx[j-1]].cpu1-thr[idx[j-1]].cpu0)<d){unsigned t=idx[j-1];idx[j-1]=idx[j];idx[j]=t;j--;}}
 for(int j=0;j<nt;j++){Thr&t=thr[idx[j]];o("  tid ");od(t.tid);if(t.tid==g_main)o("(메인)");o(" ");od((t.cpu1-t.cpu0)/10000);o("/");od((t.k1-t.k0)/10000);o(" 시작 ");oaddr(t.start);o("\r\n");}}
static void report_set(Set&S){
 o("## 스레드 ");od(S.tid);if(S.tid==g_main)o("(메인)");o(" 표본 ");od(S.ns);o(" 실패 ");od(S.nfail);
 o("  [시스템 ");od(S.ns?S.nsys*100ull/S.ns:0);o("% 게임 ");od(S.ns?S.ngame*100ull/S.ns:0);o("% 모듈/기타 ");od(S.ns?S.nmod*100ull/S.ns:0);o("%]\r\n");
 unsigned top[24];
 o("-- 멈춘 위치(RIP) 상위  [r10=첫 인자 하위32 / rdx 하위32]\r\n");{unsigned n=S.rip.top(top,20);for(unsigned i=0;i<n;i++){o("  ");od(S.rip.c[top[i]]);o("  ");oaddr(S.rip.k[top[i]]);o("  r10=");oh(S.rip.aux[top[i]]&0xffffffffull);o(" rdx=");oh(S.rip.aux[top[i]]>>32);o("\r\n");}}
 o("-- 첫 비시스템 호출자(게임 포함) 상위\r\n");{unsigned n=S.c1.top(top,20);for(unsigned i=0;i<n;i++){o("  ");od(S.c1.c[top[i]]);o("  ");oaddr(S.c1.k[top[i]]);o("\r\n");}}
 o("-- 첫 모듈 호출자(게임 실행 파일 제외) 상위\r\n");{unsigned n=S.mod1.top(top,20);for(unsigned i=0;i<n;i++){o("  ");od(S.mod1.c[top[i]]);o("  ");oaddr(S.mod1.k[top[i]]);o("\r\n");}}
 o("-- (첫, 둘째) 호출자 쌍 상위\r\n");{unsigned n=S.pair.top(top,16);for(unsigned i=0;i<n;i++){o("  ");od(S.pair.c[top[i]]);o("  ");oaddr(S.pair.aux[top[i]]);o("  <-  ");oaddr(S.pair.aux2[top[i]]);o("\r\n");}}
 flush();}
static void round_(int r,unsigned secs){
 load_mods();
 // 1초 동안 CPU 를 재서 메인 말고 가장 바쁜 스레드 3개를 고름
 nthr=0;thr_snap(true);Sleep(1000);thr_snap(false);
 nsets=0;sets[0].tid=g_main;nsets=1;
 for(int pick=0;pick<3;pick++){int best=-1;u64 bd=0;for(int i=0;i<nthr;i++){if(!thr[i].cpu1||thr[i].tid==g_main||thr[i].tid==GetCurrentThreadId())continue;bool used=false;for(int j=0;j<nsets;j++)if(sets[j].tid==thr[i].tid)used=true;if(used)continue;u64 d=thr[i].cpu1-thr[i].cpu0;if(d>bd){bd=d;best=i;}}
  if(best<0||bd<50000)break;sets[nsets++].tid=thr[best].tid;}   // 1초에 5ms 넘게 쓴 스레드만
 for(int i=0;i<nsets;i++){Set&S=sets[i];S.rip.clear();S.c1.clear();S.pair.clear();S.mod1.clear();S.ns=S.nsys=S.ngame=S.nmod=S.nfail=0;S.h=pOpenThread(0x0002|0x0008|0x0040,FALSE,S.tid);}
 nthr=0;thr_snap(true);
 ULONGLONG t0=GetTickCount64();
 while(GetTickCount64()-t0<secs*1000ull){for(int i=0;i<nsets;i++)if(sets[i].h)sample_one(sets[i]);Sleep(1);}
 for(int i=0;i<nsets;i++)if(sets[i].h){CloseHandle(sets[i].h);sets[i].h=0;}
 thr_snap(false);
 SYSTEMTIME st;GetLocalTime(&st);
 o("==== 표본 ");od((u64)r);o("회차 ");od(st.wHour);o(":");od(st.wMinute);o(":");od(st.wSecond);o(" 메인스레드 ");od(g_main);o(" 따로 잰 스레드 ");od((u64)(nsets-1));o("개\r\n");
 cpu_table(secs);flush();
 for(int i=0;i<nsets;i++)report_set(sets[i]);}
static DWORD WINAPI perf_main(void*){
 g_pid=GetCurrentProcessId();
 for(int i=0;i<600&&!g_main;i++){HMODULE u=GetModuleHandleW(L"user32.dll");if(u){PEnumWin pe=(PEnumWin)GetProcAddress(u,"EnumWindows");if(pe)pe(enum_cb,0);}if(!g_main)Sleep(1000);}
 if(!g_main){o("game window not found in 600s\r\n");flush();return 0;}
 o("==== Diag 표본기 시작(SAN14_Minimap.ini Profile=1). 메인 스레드 ");od(g_main);o("\r\n");flush();
 Sleep(5000);round_(1,30);Sleep(60000);round_(2,30);Sleep(60000);round_(3,30);
 o("==== 표본기 끝\r\n");flush();return 0;}
// 켜기: 게임 폴더 SAN14_Minimap.ini 에 Profile=1 이 있으면. 돌아온 값: 켰으면 true
static bool start(const wchar_t*root){
 wchar_t ini[1100];ini[0]=0;r39::wcat(ini,1100,root);r39::wcat(ini,1100,L"SAN14_Minimap.ini");
 HANDLE f=CreateFileW(ini,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,0,0);if(f==INVALID_HANDLE_VALUE)return false;
 char b[512];DWORD n=0;ReadFile(f,b,sizeof b-1,&n,0);CloseHandle(f);b[n]=0;bool on=false;
 for(DWORD i=0;i+9<=n;i++)if(!memcmp(b+i,"Profile=1",9)&&(i==0||b[i-1]=='\n'))on=true;
 if(!on)return false;
 HMODULE k=GetModuleHandleW(L"kernel32.dll"),nt=GetModuleHandleW(L"ntdll.dll"),u=GetModuleHandleW(L"user32.dll");if(!k||!u)return false;
 pSnap=(PSnap)GetProcAddress(k,"CreateToolhelp32Snapshot");pM32F=(PM32)GetProcAddress(k,"Module32FirstW");pM32N=(PM32)GetProcAddress(k,"Module32NextW");
 pT32F=(PT32)GetProcAddress(k,"Thread32First");pT32N=(PT32)GetProcAddress(k,"Thread32Next");pOpenThread=(POpenThread)GetProcAddress(k,"OpenThread");
 pSuspend=(PSusp)GetProcAddress(k,"SuspendThread");pResume=(PSusp)GetProcAddress(k,"ResumeThread");pGetCtx=(PGetCtx)GetProcAddress(k,"GetThreadContext");
 pVQ=(PVQ)GetProcAddress(k,"VirtualQuery");pTimes=(PThrTimes)GetProcAddress(k,"GetThreadTimes");pNtQIT=nt?(PNtQIT)GetProcAddress(nt,"NtQueryInformationThread"):nullptr;
 pGWTPI=(PGWTPI)GetProcAddress(u,"GetWindowThreadProcessId");pIsVis=(PIsVis)GetProcAddress(u,"IsWindowVisible");
 if(!pSnap||!pM32F||!pM32N||!pT32F||!pT32N||!pOpenThread||!pSuspend||!pResume||!pGetCtx||!pVQ||!pTimes||!pGWTPI||!pIsVis)return false;
 logpath[0]=0;r39::wcat(logpath,1100,root);r39::wcat(logpath,1100,L"SAN14_기록\\Diag_Perf.log");
 HANDLE t=CreateThread(0,0,perf_main,0,0,0);if(!t)return false;CloseHandle(t);return true;}
}
