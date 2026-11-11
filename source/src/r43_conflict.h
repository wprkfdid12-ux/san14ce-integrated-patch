#pragma once
// 정식판 뒤(사용자 11-08): 오류창에서 '다른 모드와 충돌'이면 크게 알린다.
//  owner_of_site(): 통합패치가 고치려던 게임 자리가 기대와 다를 때, 그 자리가 어디로 넘어가는지(jmp/call/jcc/간접 jmp/mov+jmp)
//    따라가서(중간 다리 코드는 최대 3번까지) 그 코드를 가진 DLL 이름을 찾는다. 실패한 순간 한 번만 → 평소 비용 없음.
//  owner_of_ptr(): 함수표 칸(포인터)의 주인.
//  판정: FOREIGN(통합패치 파일 아님 = 다른 모드) / OURS(통합패치 파일끼리 겹침 = 통합패치 문제) / CHANGED(바뀌었는데 파일을 못 찾음)
//  show(): 큰 제목줄 창(TaskDialog, 공용 컨트롤 6판을 그때만 불러옴) — 못 쓰면 지금 창(MessageBox)에 '■ 제목' 줄을 붙여 띄움.
//  윈도우 함수는 전부 GetProcAddress 로(가져오기 표·빌드 라이브러리 안 바꿈).
#include "common.h"
#include "r43_our_dlls.h"
namespace r43c{
typedef unsigned UINT_;typedef unsigned long ULONG_;typedef intptr_t LPTR_;typedef uintptr_t UPTR_;
enum{K_NONE=0,K_FOREIGN=1,K_OURS=2,K_CHANGED=3};
struct Owner{int kind;wchar_t name[80];unsigned long long off;};   // name: DLL 파일 이름, off: 그 DLL 안 위치
struct MBI{void*base;void*alloc;DWORD aprot;WORD part;size_t size;DWORD state,prot,type;};
inline bool ieq(const wchar_t*a,const wchar_t*b){for(;;a++,b++){wchar_t x=*a,y=*b;if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y)return false;if(!x)return true;}}
inline bool is_ours(const wchar_t*n){for(auto o:kR43OurDlls)if(ieq(n,o))return true;return false;}
#ifndef S14_TEST_SHIM
inline void* k32(const char*fn){static HMODULE k;if(!k)k=GetModuleHandleW(L"kernel32.dll");return k?GetProcAddress(k,fn):nullptr;}
// 읽을 수 있는 메모리인지(커밋됨·접근 가능)
inline bool readable(const void*p,size_t n){
 using VQ=size_t(WINAPI*)(const void*,MBI*,size_t);static VQ vq;if(!vq)vq=(VQ)k32("VirtualQuery");if(!vq||!p)return false;
 const U8*a=(const U8*)p,*e=a+n;while(a<e){MBI m;if(vq(a,&m,sizeof m)!=sizeof m)return false;if(m.state!=0x1000||(m.prot&0x101)||!(m.prot&0xFE))return false;const U8*end=(const U8*)m.base+m.size;if(end<=a)return false;a=end;}
 return true;
}
// 주소를 가진 DLL 이름(없으면 false). base: 그 DLL 시작
inline bool module_of(const void*p,wchar_t*name,size_t cap,const U8**base){
 using GMHE=BOOL(WINAPI*)(DWORD,const void*,HMODULE*);static GMHE g;if(!g)g=(GMHE)k32("GetModuleHandleExW");if(!g)return false;
 HMODULE h=nullptr;if(!g(0x4|0x2,p,&h)||!h)return false;   // FROM_ADDRESS | UNCHANGED_REFCOUNT
 wchar_t full[600];DWORD n=GetModuleFileNameW(h,full,600);if(!n||n>=600)return false;DWORD s=n;while(s&&full[s-1]!=L'\\'&&full[s-1]!=L'/')s--;
 size_t k=0;for(DWORD i=s;i<n&&k+1<cap;i++)name[k++]=full[i];name[k]=0;if(base)*base=(const U8*)h;return true;
}
#else
// 시험기(S14_TEST_SHIM): 시험이 메모리 지도·창 받기를 넣는다(안 넣으면 '읽을 수 없음' → 판정 안 함, 창은 MessageBoxW)
struct TestHooks{bool(*readable)(const void*,size_t);bool(*module_of)(const void*,wchar_t*,size_t,const U8**);void(*show)(const wchar_t*,const wchar_t*,const wchar_t*);const U8*exe_lo;const U8*exe_hi;};
inline TestHooks&th(){static TestHooks h{};return h;}
inline bool readable(const void*p,size_t n){return th().readable?th().readable(p,n):false;}
inline bool module_of(const void*p,wchar_t*name,size_t cap,const U8**base){return th().module_of?th().module_of(p,name,cap,base):false;}
#endif
inline U32 rd32(const U8*p){U32 v;memcpy(&v,p,4);return v;}
inline unsigned long long rd64(const U8*p){unsigned long long v;memcpy(&v,p,8);return v;}
// p(n 바이트, 읽기 가능 확인됨) 안의 갈림 명령 목표 주소들
//  d0..d1: 바뀐 바이트 범위 — 이 범위와 겹치는 명령만 봄(바뀌지 않은 뒤쪽 바이트를 명령으로 잘못 읽어 엉뚱한 DLL 을 짚는 것 방지)
inline int targets(const U8*p,size_t n,const U8**out,int cap,size_t d0=0,size_t d1=(size_t)-1){
 int k=0;size_t cur=0,clen=0;auto add=[&](const U8*t){if(k<cap&&t&&cur<d1&&cur+clen>d0)out[k++]=t;};
 for(size_t i=0;i<n&&k<cap;i++){const U8*q=p+i;size_t left=n-i;
  cur=i;
  if((q[0]==0xE9||q[0]==0xE8)&&left>=5){clen=5;add(q+5+(int32_t)rd32(q+1));continue;}
  if(q[0]==0x0F&&(q[1]&0xF0)==0x80&&left>=6){clen=6;add(q+6+(int32_t)rd32(q+2));continue;}
  if(q[0]==0xFF&&(q[1]==0x25||q[1]==0x15)&&left>=6){clen=6;const U8*slot=q+6+(int32_t)rd32(q+2);if(readable(slot,8))add((const U8*)rd64(slot));continue;}
  if((q[0]==0x48||q[0]==0x49)&&(q[1]>=0xB8&&q[1]<=0xBF)&&left>=10){clen=10;add((const U8*)rd64(q+2));continue;}   // mov r64,imm64 (+ jmp/call r64)
  if(q[0]==0x68&&left>=13&&q[5]==0xC7&&q[6]==0x44&&q[7]==0x24&&q[8]==0x04){clen=14;add((const U8*)(((unsigned long long)rd32(q+9)<<32)|rd32(q+1)));continue;}   // push lo; mov [rsp+4],hi; ret
 }
 return k;
}
// 목표를 따라가 '게임 실행 파일이 아닌' 첫 DLL 을 찾음(다리 코드는 앞 64바이트를 다시 훑어 최대 depth 번)
inline bool follow(const U8*t,const U8*exe_lo,const U8*exe_hi,int depth,Owner*o){
 if(!t)return false;
 wchar_t nm[80];const U8*b=nullptr;
 if(module_of(t,nm,80,&b)){
  if(t>=exe_lo&&t<exe_hi)return false;   // 게임 안으로 돌아감(빌린 명령 뒤 복귀 등) → 다른 갈래를 봄
  size_t i=0;for(;nm[i]&&i<79;i++)o->name[i]=nm[i];o->name[i]=0;o->off=(unsigned long long)(t-b);o->kind=is_ours(nm)?K_OURS:K_FOREIGN;return true;
 }
 if(depth<=0||!readable(t,64))return false;
 const U8*ts[8];int n=targets(t,64,ts,8);
 for(int i=0;i<n;i++)if(follow(ts[i],exe_lo,exe_hi,depth-1,o))return true;
 return false;
}
inline void exe_range(const U8**lo,const U8**hi){
#ifdef S14_TEST_SHIM
 if(th().exe_lo){*lo=th().exe_lo;*hi=th().exe_hi;return;}
#endif
 const U8*img=(const U8*)GetModuleHandleW(nullptr);*lo=img;*hi=img;
 if(img&&img[0]=='M'&&img[1]=='Z'){U32 pe=rd32(img+0x3c);if(pe>=0x40&&pe<0x1000)*hi=img+rd32(img+pe+0x18+56);}
}
// 바뀐 게임 자리(site, n 바이트)의 주인. 읽을 수 없으면 K_NONE.
inline Owner owner_of_site(const U8*site,size_t n,size_t d0=0,size_t d1=(size_t)-1){
 Owner o{};o.kind=K_NONE;if(!site||!n)return o;if(n>32)n=32;
 size_t scan=n+8;if(!readable(site,scan)){scan=n;if(!readable(site,n))return o;}
 const U8*lo,*hi;exe_range(&lo,&hi);
 const U8*ts[16];int k=targets(site,scan,ts,16,d0,d1);
 for(int i=0;i<k;i++)if(follow(ts[i],lo,hi,3,&o))return o;
 o.kind=K_CHANGED;return o;
}
// 함수표 칸 값(함수 주소)의 주인
inline Owner owner_of_ptr(const U8*fn){
 Owner o{};o.kind=K_NONE;if(!fn)return o;const U8*lo,*hi;exe_range(&lo,&hi);
 if(fn>=lo&&fn<hi){o.kind=K_CHANGED;return o;}   // 게임 안 다른 함수로 바뀜
 if(follow(fn,lo,hi,3,&o))return o;o.kind=K_CHANGED;return o;
}
// 사람이 읽는 판정 문장(큰 제목줄). 반환: 쓴 글자 수
inline void headline(const Owner&o,wchar_t*out,size_t cap){
 out[0]=0;auto cat=[&](const wchar_t*s){size_t n=0;while(out[n])n++;while(*s&&n+1<cap)out[n++]=*s++;out[n]=0;};
 if(o.kind==K_FOREIGN){cat(L"다른 모드와 충돌 — ");cat(o.name);}
 else if(o.kind==K_OURS){cat(L"통합패치 기능끼리 겹침 — ");cat(o.name);cat(L" (통합패치 문제, 제보 부탁)");}
 else if(o.kind==K_CHANGED)cat(L"다른 모드와 충돌 가능성 높음 (바꾼 파일은 못 찾음)");
}
inline void explain(const Owner&o,wchar_t*out,size_t cap){
 out[0]=0;auto cat=[&](const wchar_t*s){size_t n=0;while(out[n])n++;while(*s&&n+1<cap)out[n++]=*s++;out[n]=0;};
 if(o.kind==K_FOREIGN){cat(L"■ 원인: 다른 모드와 충돌\r\n  ");cat(o.name);cat(L" 이(가) 통합패치가 고칠 게임 자리를 먼저 고쳤습니다.\r\n  통합패치 고장이 아닙니다. 둘 중 하나를 끄거나, 그 모드 쪽에서 맞춰야 합니다.\r\n\r\n");}
 else if(o.kind==K_OURS){cat(L"■ 원인: 통합패치 기능끼리 겹침\r\n  ");cat(o.name);cat(L" (통합패치 파일) 이(가) 같은 자리를 먼저 고쳤습니다. 통합패치 쪽 문제이니 이 글자를 제보해 주세요.\r\n\r\n");}
 else if(o.kind==K_CHANGED)cat(L"■ 원인: 다른 모드와 충돌 가능성 높음\r\n  게임 자리가 원래 모양과 다른데, 바꾼 파일은 찾지 못했습니다(다른 모드가 직접 고쳤거나 게임 파일이 바뀜).\r\n\r\n");
}
inline void tag(const Owner&o,char*out,size_t cap){   // 기록 줄용 ASCII: owner=foreign:NAME+0xOFF
 out[0]=0;size_t n=0;auto c=[&](const char*s){while(*s&&n+1<cap)out[n++]=*s++;out[n]=0;};
 if(o.kind==K_NONE){c("owner=-");return;}
 c(o.kind==K_FOREIGN?"owner=foreign:":o.kind==K_OURS?"owner=ours:":"owner=changed");
 if(o.kind!=K_CHANGED){for(const wchar_t*p=o.name;*p&&n+1<cap;p++)out[n++]=(char)(*p<128?*p:'_');out[n]=0;c("+0x");char h[20];int k=0;unsigned long long v=o.off;do{h[k++]="0123456789abcdef"[v&15];v>>=4;}while(v);while(k&&n+1<cap)out[n++]=h[--k];out[n]=0;}
}
// ---- 큰 제목줄 창 ----
#ifndef S14_TEST_SHIM
#pragma pack(push,1)
struct TDC{UINT_ cbSize;void*hwndParent;void*hInstance;int dwFlags;int dwCommonButtons;const wchar_t*pszWindowTitle;const wchar_t*pszMainIcon;const wchar_t*pszMainInstruction;const wchar_t*pszContent;
 UINT_ cButtons;const void*pButtons;int nDefaultButton;UINT_ cRadioButtons;const void*pRadioButtons;int nDefaultRadioButton;const wchar_t*pszVerificationText;const wchar_t*pszExpandedInformation;
 const wchar_t*pszExpandedControlText;const wchar_t*pszCollapsedControlText;const wchar_t*pszFooterIcon;const wchar_t*pszFooter;void*pfCallback;LPTR_ lpCallbackData;UINT_ cxWidth;};
#pragma pack(pop)
static_assert(sizeof(TDC)==160,"TASKDIALOGCONFIG x64");
struct ACTX{ULONG_ cbSize;DWORD dwFlags;const wchar_t*lpSource;WORD arch;WORD lang;const wchar_t*dir;const wchar_t*res;const wchar_t*app;HMODULE mod;};
inline LONG WINAPI td_cb(void*hwnd,UINT_ msg,UPTR_,LPTR_,LPTR_){
 if(msg==0){using SWP=BOOL(WINAPI*)(void*,void*,int,int,int,int,UINT_);static SWP swp;if(!swp){HMODULE u=GetModuleHandleW(L"user32.dll");if(u)swp=(SWP)GetProcAddress(u,"SetWindowPos");}
  if(swp)swp(hwnd,(void*)(LPTR_)-1,0,0,0,0,0x3|0x10);}   // TDN_CREATED → 맨 앞(TOPMOST), 크기·자리·활성 그대로
 return 0;
}
// TaskDialog 로 띄움(막힘: 누를 때까지). 못 띄우면 false
inline bool task_dialog(const wchar_t*title,const wchar_t*main,const wchar_t*content,bool warn){
 using CAC=HANDLE(WINAPI*)(const ACTX*);using AAC=BOOL(WINAPI*)(HANDLE,UPTR_*);using DAC=BOOL(WINAPI*)(DWORD,UPTR_);using RAC=void(WINAPI*)(HANDLE);using GSD=UINT_(WINAPI*)(wchar_t*,UINT_);
 CAC cac=(CAC)k32("CreateActCtxW");AAC aac=(AAC)k32("ActivateActCtx");DAC dac=(DAC)k32("DeactivateActCtx");RAC rac=(RAC)k32("ReleaseActCtx");GSD gsd=(GSD)k32("GetSystemDirectoryW");
 if(!cac||!aac||!dac||!rac||!gsd)return false;
 wchar_t sh[300];UINT_ n=gsd(sh,260);if(!n||n>=260)return false;const wchar_t*tail=L"\\shell32.dll";for(UINT_ i=0;tail[i]&&n+1<300;i++)sh[n++]=tail[i];sh[n]=0;
 ACTX ax{};ax.cbSize=sizeof ax;ax.dwFlags=0x8;ax.lpSource=sh;ax.res=(const wchar_t*)(UPTR_)124;   // shell32 안의 공용 컨트롤 6판 매니페스트
 HANDLE ctx=cac(&ax);if(ctx==INVALID_HANDLE_VALUE||!ctx)return false;
 UPTR_ cookie=0;bool ok=false;
 if(aac(ctx,&cookie)){
  HMODULE cc=LoadLibraryExW(L"comctl32.dll",nullptr,0);
  using TDI=LONG(WINAPI*)(const TDC*,int*,int*,BOOL*);TDI tdi=cc?(TDI)GetProcAddress(cc,"TaskDialogIndirect"):nullptr;
  if(tdi){TDC c{};c.cbSize=sizeof c;c.dwFlags=0x0008|0x01000000;c.dwCommonButtons=1;c.pszWindowTitle=title;c.pszMainIcon=(const wchar_t*)(UPTR_)(warn?0xFFFF:0xFFFE);
   c.pszMainInstruction=main;c.pszContent=content;c.pfCallback=(void*)td_cb;int btn=0;ok=tdi(&c,&btn,nullptr,nullptr)>=0;}
  dac(0,cookie);
 }
 rac(ctx);return ok;
}
struct ShowArg{const wchar_t*title;const wchar_t*main;const wchar_t*content;UINT_ mbflags;};
inline void show_now(const ShowArg*a){
 if(a->main&&a->main[0]&&task_dialog(a->title,a->main,a->content,true))return;
 // 못 씀 → 지금 창 + 첫 줄에 제목
 static wchar_t fb[4][4200];static volatile LONG nx;LONG s=(InterlockedIncrement(&nx)-1)&3;wchar_t*m=fb[s];m[0]=0;size_t W=4200,k=0;
 auto cat=[&](const wchar_t*x){while(x&&*x&&k+1<W)m[k++]=*x++;m[k]=0;};
 if(a->main&&a->main[0]){cat(L"■■ ");cat(a->main);cat(L" ■■\r\n\r\n");}cat(a->content);
 MessageBoxW(nullptr,m,a->title,a->mbflags);
}
inline DWORD WINAPI show_thread(void*p){show_now((const ShowArg*)p);return 0;}
// 막지 않고 띄움(글자 버퍼는 부른 쪽이 정적으로 들고 있어야 함)
inline void show(const wchar_t*title,const wchar_t*main,const wchar_t*content,UINT_ mbflags){
 static ShowArg args[8];static volatile LONG nx;LONG s=(InterlockedIncrement(&nx)-1)&7;args[s]=ShowArg{title,main,content,mbflags};
 HANDLE t=CreateThread(nullptr,0,show_thread,&args[s],0,nullptr);if(t)CloseHandle(t);else show_now(&args[s]);
}
#else
inline void show(const wchar_t*title,const wchar_t*main,const wchar_t*content,UINT_ mbflags){if(th().show)th().show(title,main,content);else MessageBoxW(nullptr,content,title,mbflags);}
#endif
}
