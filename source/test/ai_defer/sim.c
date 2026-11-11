// AI 호환판 SAN14_CombatAI.dll 의 '출진 길 찾기 21번째 자리 늦게 걸기'(.aicfix)를 리눅스에서 진짜 DLL 코드로 실행해 본다.
//  가짜 게임(0x140000000)에 0x303fc9: E8 → 0x185710 을 두고, 걸기 상태(calls[25]·ready·published)를 채운 뒤 시작기(0x14b000)를 부른다.
//  시나리오: ic(통합패치 있음·신호 줌) / noic(파일 없음) / ictimeout(신호 안 줌) / rollback(이미 되돌림) / changed(자리 바뀜) / nothread(스레드 실패) / raw(블록 걸침 자리)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
typedef uint8_t U8;typedef uint16_t U16;typedef uint32_t U32;typedef uint64_t U64;
#define MS __attribute__((ms_abi))
static U8*G=(U8*)0x140000000ULL;static U8*I=(U8*)0x180000000ULL;
static const char*sc;static int ic_file=1,ic_loaded=1,wait_ret=1,thread_ok=1;static int n_wait=0,n_sleep=0,n_thread=0,published_before_wait=0,n_gfa=0;
static U8*ICB=(U8*)0x170000000ULL;
static int weq(const U16*w,const char*a){while(*a){U16 c=*w;if(c>='A'&&c<='Z')c+=32;char d=*a;if(d>='A'&&d<='Z')d+=32;if(c!=d)return 0;w++;a++;}return *w==0;}
static int wends(const U16*w,const char*a){int n=0;while(w[n])n++;int m=strlen(a);if(n<m)return 0;return weq(w+n-m,a);}
static MS void*s_GMH(const U16*n){if(!n)return I;if(weq(n,"kernel32.dll"))return (void*)0x7ff000000000ULL;if(weq(n,"SAN14_IntegratedConvenience.dll"))return ic_loaded?ICB:0;return 0;}
static MS U32 s_wait(void){n_wait++;if(*(U32*)(I+0x88349-0x88349+0x88349)&&0){} if(I[0x88349])published_before_wait=1;return wait_ret;}
static MS U32 s_GFA(const U16*p){n_gfa++;if(!wends(p,"\\SAN14_IntegratedConvenience.dll")){printf("  경로 이상\n");return 0xffffffff;}return ic_file?0x20:0xffffffff;}
typedef MS U32(*TP)(void*);
static MS void*s_CT(void*a,U64 b,TP f,void*p,U32 fl,U32*id){n_thread++;if(!thread_ok)return 0;f(p);return (void*)0x1234;}
static MS void*s_GPA(void*m,const char*n){if(!strcmp(n,"CreateThread"))return (void*)s_CT;if(!strcmp(n,"GetFileAttributesW"))return (void*)s_GFA;if(!strcmp(n,"SAN14_WaitConvenienceV1"))return (void*)s_wait;return 0;}
static MS void s_Sleep(U32 ms){n_sleep++;}
static MS U32 s_GMFN(void*m,U16*b,U32 n){const char*p="C:\\Games\\SAN14\\SAN14CE.exe";U32 i=0;for(;p[i];i++)b[i]=p[i];b[i]=0;return i;}
static MS U64 s_zero(void){return 0;}
static MS U64 s_one(void){return 1;}
int main(int argc,char**argv){
 sc=argv[2];
 FILE*f=fopen(argv[1],"rb");fseek(f,0,2);long n=ftell(f);fseek(f,0,0);U8*d=malloc(n);fread(d,1,n,f);fclose(f);
 U32 pe=*(U32*)(d+0x3c);U16 ns=*(U16*)(d+pe+6),os=*(U16*)(d+pe+20);U32 soi=*(U32*)(d+pe+24+56);
 if(mmap(I,soi,7,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED){perror("mmap");return 2;}
 memcpy(I,d,0x400);
 for(int k=0;k<ns;k++){U8*s=d+pe+24+os+40*k;U32 vs=*(U32*)(s+8),va=*(U32*)(s+12),rs=*(U32*)(s+16),rp=*(U32*)(s+20);memcpy(I+va,d+rp,rs<vs||!vs?rs:vs);}
 U32 imp=*(U32*)(d+pe+24+112+8);
 for(U8*id=I+imp;*(U32*)(id+12);id+=20){U64*th=(U64*)(I+*(U32*)(id+16));U64*ilt=(U64*)(I+*(U32*)(id));for(;*ilt;th++,ilt++){void*fn=(void*)s_zero;
   if(!(*ilt>>63)){const char*nm=(const char*)(I+(*ilt&0xffffffff)+2);
   if(!strcmp(nm,"GetModuleHandleW"))fn=(void*)s_GMH;else if(!strcmp(nm,"GetProcAddress"))fn=(void*)s_GPA;else if(!strcmp(nm,"Sleep"))fn=(void*)s_Sleep;
   else if(!strcmp(nm,"GetModuleFileNameW"))fn=(void*)s_GMFN;else if(!strcmp(nm,"CloseHandle")||!strcmp(nm,"VirtualProtect")||!strcmp(nm,"FlushInstructionCache")||!strcmp(nm,"IsProcessorFeaturePresent"))fn=(void*)s_one;}
   *th=(U64)fn;}}
 if(mmap(G,0x1500000,3,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED)return 2;
 U32 site=0x303fc9,native=0x185710;if(!strcmp(sc,"raw"))site=0x303fcc;
 G[site]=0xE8;int32_t orig=(int32_t)(native-(site+5));memcpy(G+site+1,&orig,4);
 for(int i=0;i<16;i++)if((0x303fc0+i)<site||(0x303fc0+i)>=site+5)G[0x303fc0+i]=0x90+i;
 *(U64*)(I+0x88f18)=(U64)G;
 U32 CL=0x87bf0+25*0x3c;int32_t redir=0x12345678;
 I[0x87bf0+0x12d8+25]=1;I[0x87d05]=1;
 *(U32*)(I+CL+0x148)=site&~15u;*(U32*)(I+CL+0x170)=site;*(int32_t*)(I+CL+0x174)=orig;*(int32_t*)(I+CL+0x178)=redir;I[CL+0x17c]=!strcmp(sc,"raw");I[CL+0x17d]=0;
 if(!strcmp(sc,"noic"))ic_file=0,ic_loaded=0;
 if(!strcmp(sc,"ictimeout"))wait_ret=0;
 if(!strcmp(sc,"rollback"))I[0x87d05]=0;
 if(!strcmp(sc,"changed"))G[site+2]^=1;
 if(!strcmp(sc,"nothread"))thread_ok=0;
 if(!strcmp(sc,"lateic"))ic_loaded=0;  // 파일은 있는데 끝내 안 실림
 int zf_res[2];
 for(int flag=0;flag<2;flag++){
  if(flag){I[CL+0x17d]=0;memcpy(G+site+1,&orig,4);if(!strcmp(sc,"changed"))G[site+2]^=1;n_wait=n_thread=0;}
  I[0x88f13]=flag;U8 zf;
  __asm__ volatile("sub $32,%%rsp\n\tmovabs $0x18014b000,%%r11\n\tcall *%%r11\n\tsete %0\n\tadd $32,%%rsp":"=q"(zf)::"rax","rcx","rdx","r8","r9","r10","r11","xmm0","memory","cc");
  zf_res[flag]=zf;
 }
 int32_t now;memcpy(&now,G+site+1,4);
 int pub=I[CL+0x17d];
 int ok_bytes=(G[site]==0xE8&&now==redir);
 int others=1;for(int i=0;i<16;i++){U32 a=0x303fc0+i;if(a<site||a>=site+5)if(G[a]!=0x90+i)others=0;}
 printf("%-10s 스레드=%d 기다림호출=%d 걸림=%d 자리=%s 옆바이트=%s 기다리기전걸림=%d 플래그(ZF) cmp0→%d cmp1→%d\n",sc,n_thread,n_wait,pub,ok_bytes?"새 연결":(now==orig?"원래":"기타"),others?"그대로":"바뀜",published_before_wait,zf_res[0],zf_res[1]);
 return 0;
}
