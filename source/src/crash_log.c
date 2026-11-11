/* SAN14_CrashLog: 게임이 튕길 때(접근 위반 등 치명 예외) 어느 파일의 어느 위치에서 났는지 게임 폴더 SAN14_CrashLog.log 에 남긴다.
   게임 코드·데이터는 아무것도 바꾸지 않는다(예외 처리기 등록만). 예외는 그대로 원래 처리로 넘긴다.
   R2 (2026-10-25): 사람이 바로 읽을 수 있게 ① 튕김 종류(미할당/해제됨=UAF, 널, 나눗셈 등) ② 호출 흔적에 모드 DLL이
   끼어 있으면 그 이름을 "모드 요약" 한 줄로 덧붙인다. 관찰만 하며 게임 동작은 전과 동일.
   R3 (2026-11-07 시험12): 15:28 턴 튕김이 기록 없이 꺼짐 → 잡는 종류를 넓힘.
   ① 오류 등급 예외 전부(0xC*******) + 중단점(0x80000003: 엉뚱한 주소로 뛰어 int3 채움에 닿은 경우) + C++ 예외(0xE06D7363, 게임이 처리할 수도 있어 따로 16개까지)
   ② 실행 시작 줄 하나, 정상 종료(ExitProcess → DllMain 분리) 때 종료 줄 하나 → '시작 줄 뒤 아무것도 없음' = TerminateProcess/즉시 실패(__fastfail)로 꺼진 것
   계속 읽거나 도는 것 없음(예외가 날 때와 시작·끝에만 씀). */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64; typedef void*HANDLE;
typedef struct {u32 code,flags;void*rec;void*addr;u32 n,pad;u64 info[15];} EXREC;
typedef struct {EXREC*rec;u8*ctx;} EXPTRS;
typedef struct {u16 y,mo,dw,d,h,mi,s,ms;} SYSTIME;
typedef struct {void*base;void*alloc;u32 prot;u32 pad;u64 size;u32 state,protect,type,pad2;} MBI;
typedef struct {u32 size,abi;const void*resolver;void*readable;void*patch;void*relay;void*free_relay;void(*log)(const char*);} Core;
__declspec(dllimport) void* __stdcall AddVectoredExceptionHandler(u32,long(__stdcall*)(EXPTRS*));
__declspec(dllimport) int __stdcall GetModuleHandleExW(u32,const void*,HANDLE*);
__declspec(dllimport) u32 __stdcall GetModuleFileNameW(HANDLE,u16*,u32);
__declspec(dllimport) HANDLE __stdcall CreateFileW(const u16*,u32,u32,void*,u32,u32,HANDLE);
__declspec(dllimport) int __stdcall WriteFile(HANDLE,const void*,u32,u32*,void*);
__declspec(dllimport) u32 __stdcall SetFilePointer(HANDLE,long,long*,u32);
__declspec(dllimport) int __stdcall CloseHandle(HANDLE);
__declspec(dllimport) void __stdcall GetLocalTime(SYSTIME*);
__declspec(dllimport) u32 __stdcall GetCurrentThreadId(void);
__declspec(dllimport) u64 __stdcall VirtualQuery(const void*,MBI*,u64);
static u16 path[1100];static volatile long count;static int ready;static u64 seen[64];
static char buf[8192];static u32 n;
static u16 mods[8][64];static int nmods;     /* R2: 이번 튕김 흔적에서 모은 모드 DLL 이름(중복 제거) */
static void put(const char*s){while(*s&&n<sizeof buf-1)buf[n++]=*s++;}
static void hex(u64 v,int w){char t[17];int i;for(i=15;i>=0;i--){t[i]="0123456789abcdef"[v&15];v>>=4;}t[16]=0;put(t+16-w);}
static void dec(u32 v){char t[11];int i=10;t[i]=0;do{t[--i]=(char)('0'+v%10);v/=10;}while(v);put(t+i);}
static void wput(const u16*w){char c[4];while(*w&&n<sizeof buf-8){u32 ch=*w++;if(ch<0x80){c[0]=(char)ch;c[1]=0;}else if(ch<0x800){c[0]=(char)(0xc0|(ch>>6));c[1]=(char)(0x80|(ch&63));c[2]=0;}else{c[0]=(char)(0xe0|(ch>>12));c[1]=(char)(0x80|((ch>>6)&63));c[2]=(char)(0x80|(ch&63));c[3]=0;}put(c);}}
/* ---- R2: 모드 DLL 판별 (게임/윈도우 DLL 과 구분) ---- */
static u16 lc(u16 c){return (c>='A'&&c<='Z')?(u16)(c+32):c;}
static int starts(const u16*s,const char*p){int i;for(i=0;p[i];i++)if(lc(s[i])!=(u16)p[i])return 0;return 1;}
static int eqi(const u16*s,const char*p){int i;for(i=0;p[i];i++)if(lc(s[i])!=(u16)p[i])return 0;return s[i]==0;}
static int ends_dll(const u16*s){int n2=0;while(s[n2])n2++;if(n2<4)return 0;return lc(s[n2-4])=='.'&&lc(s[n2-3])=='d'&&lc(s[n2-2])=='l'&&lc(s[n2-1])=='l';}
static int is_mod(const u16*leaf){
 if(!ends_dll(leaf))return 0;             /* 게임 본체(.exe)·런처 제외: 모드는 전부 .dll */
 if(starts(leaf,"san14"))return 1;        /* SAN14_*.dll = 우리/추가 모드 */
 return eqi(leaf,"dinput8.dll")||eqi(leaf,"dxgi.dll")||eqi(leaf,"d3d11_chain.dll")||eqi(leaf,"fieldleader3d_poc.dll")||eqi(leaf,"seasonsync.dll")||eqi(leaf,"version.dll");
}
/* 주소 a 가 어느 모듈에 속하는지 찾고, leaf(파일 이름)와 모듈 내 오프셋을 돌려준다. 모드면 1 반환. */
static int resolve(u64 a,u16*leaf,u64*off){
 HANDLE m=0;u16 name[600];u32 k,i,s=0;MBI mb;
 if(!VirtualQuery((void*)a,&mb,sizeof mb)||mb.state!=0x1000)return -1;
 if(!GetModuleHandleExW(6,(void*)a,&m)||!m)return -1;
 k=GetModuleFileNameW(m,name,600);if(!k)return -1;for(i=0;i<k;i++)if(name[i]=='\\'||name[i]=='/')s=i+1;
 for(i=0;name[s+i]&&i<63;i++)leaf[i]=name[s+i];leaf[i]=0;
 *off=a-(u64)m;return is_mod(leaf)?1:0;
}
static void add_mod(const u16*leaf){
 int i,j;for(i=0;i<nmods;i++){for(j=0;leaf[j]&&mods[i][j];j++)if(lc(leaf[j])!=lc(mods[i][j]))break;if(!leaf[j]&&!mods[i][j])return;}
 if(nmods<8){for(j=0;leaf[j]&&j<63;j++)mods[nmods][j]=leaf[j];mods[nmods][j]=0;nmods++;}
}
static int where(u64 a){u16 leaf[64];u64 off;int r=resolve(a,leaf,&off);if(r<0)return 0;
 if(r>0)add_mod(leaf);                                  /* 모드면 요약 목록에 */
 wput(leaf);put("+0x");{u64 v=off;char t[17];int j=16;t[j]=0;if(!v)t[--j]='0';while(v){t[--j]="0123456789abcdef"[v&15];v>>=4;}put(t+j);}
 return 1;}
static volatile long cxx_count;
static void stamp_line(const char*what){SYSTIME t;HANDLE f;u32 w;n=0;GetLocalTime(&t);
 put("\r\n=== ");dec(t.y);put("-");dec(t.mo);put("-");dec(t.d);put(" ");dec(t.h);put(":");dec(t.mi);put(":");dec(t.s);put(" ");put(what);put("\r\n");
 f=CreateFileW(path,4,3,0,4,0x80,0);if(f!=(HANDLE)-1){SetFilePointer(f,0,0,2);WriteFile(f,buf,n,&w,0);CloseHandle(f);}}
static long __stdcall handler(EXPTRS*e){
 u32 code;u64 rip,rsp,*sp;int i,shown=0;SYSTIME t;HANDLE f;u32 w;
 if(!ready||!e||!e->rec)return 0;code=e->rec->code;
 if(code==0xE06D7363){if(__atomic_add_fetch(&cxx_count,1,__ATOMIC_SEQ_CST)>16)return 0;}
 else if((code&0xF0000000)!=0xC0000000&&code!=0x80000003)return 0;
 rip=(u64)e->rec->addr;for(i=0;i<64;i++)if(seen[i]==rip)return 0; /* 같은 자리 반복(게임이 스스로 처리하는 예외)은 한 번만 */
 {long k=__atomic_add_fetch(&count,1,__ATOMIC_SEQ_CST);if(k>64)return 0;seen[k-1]=rip;}
 n=0;nmods=0;GetLocalTime(&t);
 put("\r\n=== ");dec(t.y);put("-");dec(t.mo);put("-");dec(t.d);put(" ");dec(t.h);put(":");dec(t.mi);put(":");dec(t.s);put(" thread ");dec(GetCurrentThreadId());put(" code 0x");hex(code,8);
 /* 사람이 읽는 튕김 종류 */
 put(" (");put(code==0xC0000005?"접근 위반":code==0xC0000094?"0 나누기(정수)":code==0xC0000096?"잘못된 명령":code==0xC000001D?"잘못된 명령":code==0xC00000FD?"스택 넘침":code==0xC0000374?"힙 손상":code==0x80000003?"중단점(int3) — 엉뚱한 주소로 뜀 의심":code==0xE06D7363?"C++ 예외(게임이 처리하면 계속 진행)":code==0xC0000409?"보안 검사 실패":code==0xC0000420?"검증 실패":"치명 예외");put(")");
 rip=(u64)e->rec->addr;put("\r\n위치: ");if(!where(rip)){put("0x");hex(rip,16);}
 if(code==0xC0000005&&e->rec->n>=2){u64 bad=e->rec->info[1];MBI mb;
  put("\r\n접근: ");put(e->rec->info[0]==0?"읽기 ":e->rec->info[0]==1?"쓰기 ":"실행 ");put("0x");hex(bad,16);put(" (");
  if(bad<0x10000)put("널 포인터 계열");
  else if(where(bad)){/* 모듈 안을 가리킴 — where 가 이미 이름을 찍음 */}
  else if(VirtualQuery((void*)bad,&mb,sizeof mb)){
   if(mb.state==0x10000)put("미할당 메모리");
   else if(mb.state==0x2000)put("예약만 — 해제됨");
   else if(mb.state==0x1000)put("커밋된 힙 — 해제 후 재사용 의심(UAF)");
   else put("모듈 밖");
  }else put("미할당 메모리");
  put(")");}
 if(e->ctx){rsp=*(u64*)(e->ctx+0x98);put("\r\n레지스터 rax=");hex(*(u64*)(e->ctx+0x78),16);put(" rcx=");hex(*(u64*)(e->ctx+0x80),16);put(" rdx=");hex(*(u64*)(e->ctx+0x88),16);put(" rbx=");hex(*(u64*)(e->ctx+0x90),16);put(" rsp=");hex(rsp,16);
  put(" rbp=");hex(*(u64*)(e->ctx+0xa0),16);put(" rsi=");hex(*(u64*)(e->ctx+0xa8),16);put(" rdi=");hex(*(u64*)(e->ctx+0xb0),16);
  put("\r\n          r8=");hex(*(u64*)(e->ctx+0xb8),16);put(" r9=");hex(*(u64*)(e->ctx+0xc0),16);put(" r12=");hex(*(u64*)(e->ctx+0xd8),16);put(" r13=");hex(*(u64*)(e->ctx+0xe0),16);put(" r14=");hex(*(u64*)(e->ctx+0xe8),16);put(" r15=");hex(*(u64*)(e->ctx+0xf0),16);
  if(rsp&7)put("\r\n스택 위치가 8바이트에 안 맞음(어긋남) — 앞선 후크/연결 코드가 스택을 틀어지게 했을 가능성");
  /* R3: 스택이 어긋나 있어도 흔적을 찾게 4바이트 간격으로 훑음(정렬 안 맞는 8바이트 읽기) */
  put("\r\n호출 흔적:");sp=(u64*)(rsp&~(u64)3);
  {u64 lastpage=0;for(i=0;i<1024&&shown<24;i++){u8*q=(u8*)sp+i*4;u64 v;u64 pg=((u64)q+7)&~(u64)0xfff;
   if(pg!=lastpage){MBI mb;if(!VirtualQuery((void*)((u64)q+7),&mb,sizeof mb)||mb.state!=0x1000)break;lastpage=pg;}
   __builtin_memcpy(&v,q,8);
   if(v>0x10000){u32 save=n;put("\r\n  ");if(where(v))shown++;else n=save;}}}}
 /* R2: 모드 요약 — 흔적에 모드가 끼었나, 끼었으면 누구인가 */
 put("\r\n모드 요약: ");
 if(nmods){put("흔적에 모드 있음 → ");for(i=0;i<nmods;i++){if(i)put(", ");wput(mods[i]);}}
 else put("스택 전부 게임/윈도우 코드 — 모드 후크 없음(데이터 손상 의심)");
 put("\r\n");
 f=CreateFileW(path,4,3,0,4,0x80,0);if(f!=(HANDLE)-1){SetFilePointer(f,0,0,2);WriteFile(f,buf,n,&w,0);CloseHandle(f);}
 return 0;}
__declspec(dllexport) int SAN14_ModInitV1(const Core*c){
 u32 k,i,s=0;const u16*leaf=u"SAN14_CrashLog.log";
 if(ready)return 1;
 k=GetModuleFileNameW(0,path,1000);if(!k||k>=1000)return 0;for(i=0;i<k;i++)if(path[i]=='\\'||path[i]=='/')s=i+1;
 for(i=0;leaf[i];i++)path[s+i]=leaf[i];path[s+i]=0;
 if(!AddVectoredExceptionHandler(0,handler))return 0;
 ready=1;stamp_line("실행 시작 (CrashLog R3: 이 줄 뒤에 기록 없이 꺼지면 TerminateProcess/즉시 실패)");
 if(c&&c->log)c->log("CrashLog: crash recorder armed (R3: all error-class exceptions + int3 + C++ throws, start/exit lines); no game code changed.");
 return 1;}
int DllMain(void*m,u32 r,void*v){(void)m;if(r==0&&v&&ready)stamp_line("정상 종료 경로(ExitProcess)");return 1;}
