// R43 9차: 13번 함수 앞부분 검사(function_guards_ok) 흉내 — 실제 소스 조각(merit_runtime.c 에서 잘라 냄)을 그대로 컴파일
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef uint64_t u64;typedef uintptr_t usize;typedef void*HANDLE;
#define D(p,o) (*(u32*)((u8*)(p)+(o)))
#define S(p,o) (*(uint16_t*)((u8*)(p)+(o)))
enum {F_COMMIT,F_BONUS,F_ROW_REFRESH,F_STATUS,F_CONFIRM,F_STREAM_OPEN,F_LOAD,F_NEWGAME,F_SAVE_PREP,F_SAVE_END,F_VALID,F_LABEL_COLOR,F_CAPTION_COLOR,F_UI_EXIT,F_ELIGIBLE,F_STATE_TICK,F_CLOSE,F_PRESENT_RESUME,F_PRESENT_FINISH,F_BUTTON_EVENT,F_ROW_DESTROY,F_COUNT};
static struct{u32 funcs[F_COUNT];u8 function_guards[F_COUNT][16];}R,*res=&R;
static u8*game;static u8*fakemod;static char last[400];static int nrep;
static int rd(const void*p,usize n){(void)n;return p!=NULL&&(usize)p>4096;}
static int mc_equal(const void*a,const void*b,usize n){return memcmp(a,b,n)==0;}
static void report(const char*s){snprintf(last,sizeof last,"%s",s);nrep++;}
static HANDLE GetModuleHandleW(const u16*n){(void)n;return game;}
static int address_in_module(HANDLE m,usize a){return a>=(usize)m&&a<(usize)m+0x100000;}
#include "guard_part.inc"
static int fails;static void check(int c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
static void reset(void){for(int i=0;i<F_COUNT;i++){R.funcs[i]=0x1000+i*0x100;for(int k=0;k<16;k++)R.function_guards[i][k]=(u8)(0x40+i+k);memcpy(game+R.funcs[i],R.function_guards[i],16);}nrep=0;last[0]=0;}
int main(void){game=calloc(1,0x100000);fakemod=calloc(1,0x1000);
 reset();check(function_guards_ok()==1&&nrep==0,"모두 원래 모양 → 통과, 기록 없음");
 reset();{u8*p=game+R.funcs[F_LOAD];p[0]=0xe9;*(int32_t*)(p+1)=(int32_t)((usize)fakemod-(usize)(p+5));} // 다른 모듈로 점프(E9) — 범위 넘으면 아래 FF25 로
 { long long d=(long long)((usize)fakemod-(usize)(game+R.funcs[F_LOAD]+5)); if(d>0x7fffffffLL||d<-0x80000000LL){u8*p=game+R.funcs[F_LOAD];p[0]=0xff;p[1]=0x25;*(u32*)(p+2)=0;*(u64*)(p+6)=(u64)(usize)fakemod;} }
 check(function_guards_ok()==1&&strstr(last,"F_LOAD")&&strstr(last,"used as is"),"불러 쓰기만 하는 F_LOAD 앞에 다른 모듈 점프 → 통과 + 기록");
 reset();{u8*p=game+R.funcs[F_ELIGIBLE];p[0]=0xff;p[1]=0x25;*(u32*)(p+2)=0;*(u64*)(p+6)=(u64)(usize)fakemod;}
 check(function_guards_ok()==0&&strstr(last,"F_ELIGIBLE")&&strstr(last,"must be original"),"13번이 덮어쓰는 F_ELIGIBLE 이 바뀜 → 끔 + 어느 함수인지 기록");
 reset();game[R.funcs[F_STATUS]+3]^=0xff;
 check(function_guards_ok()==0&&strstr(last,"F_STATUS")&&strstr(last,"not a recognizable"),"점프가 아닌 다른 바이트 변경 → 끔 + 함수 이름·첫 바이트 기록");
 reset();{u8*p=game+R.funcs[F_SAVE_END];p[0]=0xe9;*(int32_t*)(p+1)=0x10;}
 check(function_guards_ok()==0,"점프 대상이 게임 안(다른 모듈 아님) → 끔");
 reset();{u8*p=game+R.funcs[F_BONUS];p[0]=0xff;p[1]=0x25;*(u32*)(p+2)=0;*(u64*)(p+6)=(u64)(usize)fakemod;p[14]^=1;}
 check(function_guards_ok()==0,"절대 점프 뒤 남은 바이트까지 다름 → 끔");
 printf(fails?"실패 %d\n":"전부 통과\n",fails);return fails!=0;}
