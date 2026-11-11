// AI 호환판 v15.14 SAN14_CombatAI.dll 의 '늦게 걸기'(.aicfix: 출진 길 찾기 26번째 0x303fc9, 전투 경로 고리 넷째 0x44e50d)를
// 리눅스에서 진짜 DLL 코드로 실행해 본다(call_publish·route_hook::restore 도 DLL 원래 코드 그대로).
//  가짜 게임(0x140000000)에 두 자리 E8 을 두고, 게시 상태(published·ready·CallLink)를 채운 뒤 시작기(0x180214000)를 부른다.
//  시나리오: ic / noic / ictimeout / lateic / rollback / changed / routechanged / nothread / raw / noroute
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
typedef uint8_t U8;typedef uint16_t U16;typedef uint32_t U32;typedef uint64_t U64;
#define MS __attribute__((ms_abi))
static U8*G=(U8*)0x140000000ULL;static U8*I=(U8*)0x180000000ULL;
static const char*sc;static int ic_file=1,ic_loaded=1,wait_ret=1,thread_ok=1;static int n_wait=0,n_thread=0,n_gfa=0;
static U8*ICB=(U8*)0x170000000ULL;
static int weq(const U16*w,const char*a){while(*a){U16 c=*w;if(c>='A'&&c<='Z')c+=32;char d=*a;if(d>='A'&&d<='Z')d+=32;if(c!=d)return 0;w++;a++;}return *w==0;}
static int wends(const U16*w,const char*a){int n=0;while(w[n])n++;int m=strlen(a);if(n<m)return 0;return weq(w+n-m,a);}
static MS void*s_GMH(const U16*n){if(!n)return I;if(weq(n,"kernel32.dll"))return (void*)0x7ff000000000ULL;if(weq(n,"SAN14_IntegratedConvenience.dll"))return ic_loaded?ICB:0;return 0;}
static int pub_seen_at_wait=-1;
static MS U32 s_wait(void){n_wait++;pub_seen_at_wait=I[0xae5e5];return wait_ret;}
static MS U32 s_GFA(const U16*p){n_gfa++;if(!wends(p,"\\SAN14_IntegratedConvenience.dll")){printf("  경로 이상\n");return 0xffffffff;}return ic_file?0x20:0xffffffff;}
typedef MS U32(*TP)(void*);
static MS void*s_CT(void*a,U64 b,TP f,void*p,U32 fl,U32*id){n_thread++;if(!thread_ok)return 0;f(p);return (void*)0x1234;}
static MS void*s_GPA(void*m,const char*n){if(!strcmp(n,"CreateThread"))return (void*)s_CT;if(!strcmp(n,"GetFileAttributesW"))return (void*)s_GFA;if(!strcmp(n,"SAN14_WaitConvenienceV1"))return (void*)s_wait;return 0;}
static MS void s_Sleep(U32 ms){}
static MS U32 s_GMFN(void*m,U16*b,U32 n){const char*p="C:\\Games\\SAN14\\SAN14CE.exe";U32 i=0;for(;p[i];i++)b[i]=p[i];b[i]=0;return i;}
static MS U64 s_zero(void){return 0;}
static MS U64 s_one(void){return 1;}
static MS void*s_proc(void){return (void*)-1;}
static void setlink(U32 cl,U32 site,int32_t orig,int32_t redir,int raw){*(U32*)(I+cl+4)=site&~15u;*(U32*)(I+cl+0x2c)=site;*(int32_t*)(I+cl+0x30)=orig;*(int32_t*)(I+cl+0x34)=redir;I[cl+0x38]=raw;I[cl+0x39]=0;}
int main(int argc,char**argv){
 sc=argv[2];
 FILE*f=fopen(argv[1],"rb");fseek(f,0,2);long n=ftell(f);fseek(f,0,0);U8*d=malloc(n);if(fread(d,1,n,f)!=(size_t)n)return 2;fclose(f);
 U32 pe=*(U32*)(d+0x3c);U16 ns=*(U16*)(d+pe+6),os=*(U16*)(d+pe+20);U32 soi=*(U32*)(d+pe+24+56);
 if(mmap(I,soi,7,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED){perror("mmap");return 2;}
 memcpy(I,d,0x400);
 for(int k=0;k<ns;k++){U8*s=d+pe+24+os+40*k;U32 vs=*(U32*)(s+8),va=*(U32*)(s+12),rs=*(U32*)(s+16),rp=*(U32*)(s+20);memcpy(I+va,d+rp,rs<vs||!vs?rs:vs);}
 U32 imp=*(U32*)(d+pe+24+112+8);
 for(U8*id=I+imp;*(U32*)(id+12);id+=20){U64*th=(U64*)(I+*(U32*)(id+16));U64*ilt=(U64*)(I+*(U32*)(id));for(;*ilt;th++,ilt++){void*fn=(void*)s_zero;
   if(!(*ilt>>63)){const char*nm=(const char*)(I+(*ilt&0xffffffff)+2);
   if(!strcmp(nm,"GetModuleHandleW"))fn=(void*)s_GMH;else if(!strcmp(nm,"GetProcAddress"))fn=(void*)s_GPA;else if(!strcmp(nm,"Sleep"))fn=(void*)s_Sleep;
   else if(!strcmp(nm,"GetModuleFileNameW"))fn=(void*)s_GMFN;else if(!strcmp(nm,"GetCurrentProcess"))fn=(void*)s_proc;
   else if(!strcmp(nm,"CloseHandle")||!strcmp(nm,"VirtualProtect")||!strcmp(nm,"FlushInstructionCache")||!strcmp(nm,"IsProcessorFeaturePresent"))fn=(void*)s_one;}
   *th=(U64)fn;}}
 if(mmap(G,0x1500000,3,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED)return 2;
 *(U64*)(I+0xaf7f8)=(U64)G;
 // 출진 k=25: 0x303fc9 → 0x185710 / 경로 고리 넷째: 0x44e50d → 0x2e6e00
 U32 s1=0x303fc9,t1=0x185710,s2=0x44e50d,t2=0x2e6e00;if(!strcmp(sc,"raw"))s1=0x303fcc;
 int32_t o1=(int32_t)(t1-(s1+5)),o2=(int32_t)(t2-(s2+5)),r1=0x11111111,r2=0x22222222;
 for(int i=0;i<16;i++){G[(s1&~15u)+i]=0x90+i;G[(s2&~15u)+i]=0xa0+i;}
 G[s1]=0xE8;memcpy(G+s1+1,&o1,4);G[s2]=0xE8;memcpy(G+s2+1,&o2,4);
 U32 SC=0xae614+25*0x3c,RH=0x1a92e0+3*0x3c;
 setlink(SC,s1,o1,r1,!strcmp(sc,"raw"));setlink(RH,s2,o2,r2,(s2&15)>11);
 I[0xaf7a8+25]=1;I[0xae5e5]=0;                 // 시작기가 published=1 을 써야 함
 I[0xaf823]=!strcmp(sc,"noroute")?0:1;*(U64*)(I+0xaf828)=0x180001234ULL;
 I[0x9805e]=1;I[0x98071]=1;I[0x98073]=1;
 if(!strcmp(sc,"noic"))ic_file=0,ic_loaded=0;
 if(!strcmp(sc,"ictimeout"))wait_ret=0;
 if(!strcmp(sc,"lateic"))ic_loaded=0;
 if(!strcmp(sc,"changed"))G[s1+2]^=1;
 if(!strcmp(sc,"routechanged"))G[s2+2]^=1;
 if(!strcmp(sc,"nothread"))thread_ok=0;
 int rollback=!strcmp(sc,"rollback");
 U64 rax_after;
 if(rollback){
  // 시작기가 published 를 1 로 쓴 뒤 되돌림이 일어난 경우: 기다리는 동안 published=0 → 걸지 않아야 함 → 스레드 함수를 시작기 뒤에 따로 부름
  thread_ok=0;I[0xae5e5]=0;
  // published 를 0 으로 둔 채 publish_late 만 직접(시작기는 published=1 로 만들므로 우회)
  __asm__ volatile("sub $32,%%rsp\n\tmovabs $0x18021416e,%%r11\n\tcall *%%r11\n\tadd $32,%%rsp":::"rax","rcx","rdx","r8","r9","r10","r11","xmm0","xmm1","memory","cc");
  rax_after=1;
 }else{
  __asm__ volatile("sub $32,%%rsp\n\tmov $1,%%eax\n\tmovabs $0x180214000,%%r11\n\tcall *%%r11\n\tadd $32,%%rsp\n\tmov %%rax,%0":"=r"(rax_after)::"rax","rcx","rdx","r8","r9","r10","r11","xmm0","xmm1","memory","cc");
 }
 int32_t now1,now2;memcpy(&now1,G+s1+1,4);memcpy(&now2,G+s2+1,4);
 int pub1=I[SC+0x39],pub2=I[RH+0x39];
 printf("%-12s rax=%llu PUB=%d 기다림%d 스레드%d 확인시PUB=%d | 출진26 %s(%d) | 경로4 %s(%d) | 사기%d 돌파%d 중계%d owns=%s\n",sc,(unsigned long long)rax_after,I[0xae5e5],n_wait,n_thread,pub_seen_at_wait,
  now1==r1?"걸림":now1==o1?"원래":(now1==(o1^0x100)?"바뀐채":"?"),pub1,now2==r2?"걸림":now2==o2?"원래":(now2==(o2^0x100)?"바뀐채":"?"),pub2,I[0x9805e],I[0x98071],I[0x98073],*(U64*)(I+0xaf828)?"있음":"없음");
 // 기대값
 int ok=1;
 int want1=!(rollback||!strcmp(sc,"changed")),want2=!(rollback||!strcmp(sc,"routechanged")||!strcmp(sc,"noroute"));
 if(!rollback&&rax_after!=1)ok=0;
 if(want1!=(now1==r1&&pub1))ok=0;
 if(want2!=(now2==r2&&pub2))ok=0;
 if(!strcmp(sc,"routechanged")&&(I[0x9805e]||I[0x98071]||I[0x98073]||*(U64*)(I+0xaf828)))ok=0;
 if(strcmp(sc,"routechanged")&&(!I[0x9805e]||!I[0x98071]||!I[0x98073]))ok=0;
 if(!strcmp(sc,"ic")&&(n_wait!=1||pub_seen_at_wait!=1))ok=0;
 if(!strcmp(sc,"ictimeout")&&n_wait!=40)ok=0;
 if((!strcmp(sc,"noic")||!strcmp(sc,"lateic"))&&n_wait!=0)ok=0;
 if(!strcmp(sc,"nothread")&&n_thread!=1)ok=0;
 // 이웃 바이트 그대로
 for(int i=0;i<16;i++){U32 a=(s1&~15u)+i;if((a<s1||a>=s1+5)&&G[a]!=0x90+i)ok=0;a=(s2&~15u)+i;if((a<s2||a>=s2+5)&&G[a]!=0xa0+i)ok=0;}
 printf("  → %s\n",ok?"OK":"FAIL");
 return ok?0:1;
}
