// 14번 TraitHover 칸 검사 흉내(리눅스에서 진짜 DLL 코드를 그대로 실행): PE 를 기본 주소에 올리고, 가짜 게임·가짜 9칸 화면 부품(UI)·가짜 남의 DLL 을 만든 뒤
//  시작 함수(.v2 0x5070)와 칸 검사 함수(0x4c80)를 152칸 전부에 돌려 받음/거부를 센다.
//  사용: th_sim <TraitHover.dll> <Traits9_UI.dll> <시나리오: native|ui|ui_late|foreign> [v2] [uni]  (uni: 합친 판의 시작 마스크 stubX 도 실행)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
typedef uint8_t U8;typedef uint16_t U16;typedef uint32_t U32;typedef uint64_t U64;
#define MS __attribute__((ms_abi))
static U8*gameb=(U8*)0x140000000ULL;static U8*uib=(U8*)0x110000000ULL;static U8*forb=(U8*)0x120000000ULL;static U8*feb=(U8*)0x130000000ULL;
static int ui_loaded=0,fe_loaded=0;
static int weq(const U16*w,const char*a){while(*a){U16 c=*w;if(c>='A'&&c<='Z')c+=32;char d=*a;if(d>='A'&&d<='Z')d+=32;if(c!=d)return 0;w++;a++;}return *w==0;}
static MS void*s_GetModuleHandleW(const U16*n){if(!n)return (void*)0x180000000ULL;if(weq(n,"SAN14_Traits9_UI.dll"))return ui_loaded?uib:0;if(weq(n,"SAN14_FormationExtension.dll"))return fe_loaded?feb:0;return 0;}
static void*owner(U64 a){if(ui_loaded&&a>=(U64)uib&&a<(U64)uib+0x40000)return uib;if(a>=(U64)forb&&a<(U64)forb+0x40000)return forb;if(fe_loaded&&a>=(U64)feb&&a<(U64)feb+0x40000)return feb;if(a>=0x180000000ULL&&a<0x1800b0000ULL)return (void*)0x180000000ULL;return 0;}
static MS int s_GetModuleHandleExW(U32 f,U64 a,void**out){void*m=owner(a);if(!m){*out=0;return 0;}*out=m;return 1;}
static MS U64 s_zero(void){return 0;}
static MS U64 s_one(void){return 1;}
static void fake_pe(U8*b){memset(b,0,0x40000);b[0]='M';b[1]='Z';*(U32*)(b+0x3c)=0x80;*(U32*)(b+0x80)=0x4550;*(U16*)(b+0x84)=0x8664;*(U16*)(b+0x98)=0x20b;*(U32*)(b+0x80+0x50)=0x40000;}
int main(int argc,char**argv){
 if(argc<4){fprintf(stderr,"usage\n");return 2;}
 FILE*f=fopen(argv[1],"rb");fseek(f,0,2);long n=ftell(f);fseek(f,0,0);U8*d=malloc(n);fread(d,1,n,f);fclose(f);
 U32 pe=*(U32*)(d+0x3c);U16 ns=*(U16*)(d+pe+6),os=*(U16*)(d+pe+20);U32 soi=*(U32*)(d+pe+24+56);
 U8*img=mmap((void*)0x180000000ULL,soi,7,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);if(img==MAP_FAILED){perror("mmap");return 2;}
 memcpy(img,d,0x400);
 for(int k=0;k<ns;k++){U8*s=d+pe+24+os+40*k;U32 vs=*(U32*)(s+8),va=*(U32*)(s+12),rs=*(U32*)(s+16),rp=*(U32*)(s+20);memcpy(img+va,d+rp,rs<vs||!vs?rs:vs);}
 U32 imp=*(U32*)(d+pe+24+112+8);
 for(U8*id=img+imp;*(U32*)(id+12);id+=20){U64*thunk=(U64*)(img+*(U32*)(id+16));for(;*thunk;thunk++){const char*nm=(const char*)(img+(*thunk&0xffffffff)+2);void*fn=(void*)s_zero;
   if(!strcmp(nm,"GetModuleHandleW"))fn=(void*)s_GetModuleHandleW;else if(!strcmp(nm,"GetModuleHandleExW"))fn=(void*)s_GetModuleHandleExW;else if(!strcmp(nm,"VirtualProtect")||!strcmp(nm,"FlushInstructionCache"))fn=(void*)s_one;*thunk=(U64)fn;}}
 if(mmap(gameb,0x1500000,3,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED)return 2;
 mmap(uib,0x40000,3,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);fake_pe(uib);mmap(forb,0x40000,3,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);fake_pe(forb);mmap(feb,0x40000,3,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);fake_pe(feb);
 // 9칸 자리(0x2320aa: cmp ebx,9)
 gameb[0x2320aa]=0x83;gameb[0x2320ab]=0xfb;gameb[0x2320ac]=9;
 // UI 표(UI dll 0x9100: {칸,원래,종류} 146개)
 FILE*g=fopen(argv[2],"rb");fseek(g,0,2);long un=ftell(g);fseek(g,0,0);U8*u=malloc(un);fread(u,1,un,g);fclose(g);
 U32 upe=*(U32*)(u+0x3c);U16 uns=*(U16*)(u+upe+6),uos=*(U16*)(u+upe+20);long uo=-1;
 for(int k=0;k<uns;k++){U8*s=u+upe+24+uos+40*k;U32 va=*(U32*)(s+12),rs=*(U32*)(s+16),rp=*(U32*)(s+20);if(va<=0x9100&&0x9100<va+rs)uo=0x9100-va+rp;}
 U32 uis[146][3];memcpy(uis,u+uo,sizeof uis);
 U32*rec=(U32*)(img+0x62a0);int nrec=0x98;
 const char*sc=argv[3];int v2=argc>4;U32 mask_got=0;
 for(int i=0;i<nrec;i++)*(U64*)(gameb+rec[i*3])=(U64)gameb+rec[i*3+1];
 if(!strcmp(sc,"ui")||!strcmp(sc,"ui_late")){ui_loaded=1;}
 if(!strcmp(sc,"ui"))for(int j=0;j<146;j++)*(U64*)(gameb+uis[j][0])=(U64)uib+0x1000+uis[j][2]*0x40;
 *(U64*)(img+0x9000)=(U64)gameb;
 if(v2){U64 r;__asm__ volatile("sub $32,%%rsp\n\tmov %1,%%rax\n\tmovabs $0x180005070,%%r11\n\tcall *%%r11\n\tadd $32,%%rsp":"=a"(r):"r"((U64)gameb):"rcx","rdx","r8","r9","r10","r11","memory");}
 if(v2&&argc>5){U32 m;__asm__ volatile("sub $32,%%rsp\n\tmovabs $0x1800aa000,%%r11\n\tcall *%%r11\n\tadd $32,%%rsp":"=S"(m)::"r11","memory","cc");printf("MASK 0x%x\n",m);mask_got=m;}
 if(!strcmp(sc,"ui_late")){   // 시작(미리 거는 칸) 뒤에 UI 가 칸을 차지하려 함: TH 시작 마스크 종류 → 0x36d0, 종류 3 → 0x3680 을 먼저 걸고, UI 는 146칸 전부 원래 값일 때만 씀(UI 0x4918 규칙)
  U32 mask=v2?(argc>5?mask_got:0):0x131;
  for(int i=0;i<nrec;i++){U32 k=rec[i*3+2];if(k<=8&&(mask>>k&1))*(U64*)(gameb+rec[i*3])=0x1800036d0ULL;else if(k==3)*(U64*)(gameb+rec[i*3])=0x180003680ULL;}
  int clean=1;for(int j=0;j<146;j++)if(*(U64*)(gameb+uis[j][0])!=(U64)gameb+uis[j][1])clean=0;
  printf("  UI late: %s\n",clean?"HOVER_APPLIED(칸 씀)":"HOVER_SKIPPED(TH 가 먼저 건 칸 있음)");
  if(clean)for(int j=0;j<146;j++)*(U64*)(gameb+uis[j][0])=(U64)uib+0x1000+uis[j][2]*0x40;}
 if(!strcmp(sc,"foreign")){*(U64*)(gameb+rec[0])=(U64)forb+0x1234;}
 typedef MS int(*CHK)(U32,U64);CHK chk=(CHK)0x180004c80ULL;
 int rej=0;
 for(int i=0;i<nrec;i++){U64 v=*(U64*)(gameb+rec[i*3]);int ok=chk(i,v);if(!ok){rej++;if(rej<=6)printf("  reject #%d slot=0x%x kind=%u\n",i,rec[i*3],rec[i*3+2]);}}
 printf("RESULT %s %s reject=%d/%d\n",v2?"v2":"dll",sc,rej,nrec);
 return 0;
}
