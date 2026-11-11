// P207(2026-11-10 제보): 17번만 깔아도 능력 100 을 찍은 뒤 200 까지 경험치 성장이 안 됨.
//  이 시험은 게임 경험치 성장 함수의 '레벨업 구간' 실제 코드(새 판 exe+0x2bc30a~0x2bc3c7, 190바이트 그대로)를 가짜 게임 이미지에 올려 돌리고,
//  우리 Stats200Grow(다리 GrowBridge 포함)와 결과를 비교한다.
//   [1] 상한 100 으로 두면 게임 구간과 결과가 완전히 같다(능력 1~99 × 경험치 여러 값 × 능력 5종) — 100 아래 동작을 안 바꿨다는 증명
//   [2] 게임 구간 그대로는 100 에서 멈춘다(제보 재현) / 우리 것은 100→101→…→200, 필요 경험치 = t100+(L-100)×(t100−t99)
//   [3] 다리: r14 복구, 스택 그대로, 게임으로 돌아가는 자리
//   [4] 안전 조건(v1.9 와 같음): t99=0·간격 과대(18번 억제 표)·32비트 넘침이면 100 위로 안 큼
//   [5] 함수 입구 비교 6바이트(cmp r13b,0xc8; jb): 200 미만이면 계속, 200 이면 멈춤 / 원래 바이트는 딱 100 에서만 멈춤
//  빌드/실행: clang++ -std=c++17 -O1 -w -DS14_TEST_SHIM -I test/shim -I src test/test_stats200_grow_native.cpp src/stats200_new_bridge.S -o /tmp/t && /tmp/t
#include <sys/mman.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#define S14_GAMECALL __attribute__((ms_abi))   // 게임 함수·우리 다리가 쓰는 윈도 x64 호출 규약을 리눅스 시험 빌드에서도 그대로
#include "windows.h"
// 시험 틀에 없는 윈도 함수(이 시험은 안 부름)
static int VirtualProtect(void*,size_t,unsigned,DWORD*){return 1;}
static void*GetCurrentProcess(){return nullptr;}
static int FlushInstructionCache(void*,void*,size_t){return 1;}
#include "../src/stats200_new.cpp"
// 기록 쪽 윈도 함수: 이 시험에선 파일을 안 남김(자가진단 줄 수만 셈)
static int counter_lines;
extern "C" DWORD GetModuleFileNameW(HMODULE,wchar_t*o,DWORD){o[0]=0;return 0;}
extern "C" BOOL CreateDirectoryW(const wchar_t*,void*){return 1;}
extern "C" HANDLE CreateFileW(const wchar_t*,DWORD,DWORD,void*,DWORD,DWORD,void*){counter_lines++;return (HANDLE)-1;}
extern "C" BOOL WriteFile(HANDLE,const void*,DWORD,DWORD*,void*){return 1;}
extern "C" BOOL CloseHandle(HANDLE){return 1;}
extern "C" void GetLocalTime(SYSTEMTIME*t){memset(t,0,sizeof *t);}
extern "C" int MessageBoxW(void*,const wchar_t*,const wchar_t*,unsigned){return 0;}
extern "C" HMODULE GetModuleHandleW(const wchar_t*){return nullptr;}
static const unsigned char kSection[190]={
0x41,0x8d,0x5d,0x01,0xe8,0x2d,0xb9,0x07,0x00,0x4c,0x8b,0x74,0x24,0x48,0x83,0xfb,0x65,0x73,0x07,0x48,0x8b,0x5c,0xd8,0x60,0xeb,0x04,0x48,0x8b,0x58,0x60,
0x48,0x8b,0xcb,0xe8,0x20,0x54,0x07,0x00,0x85,0xc0,0x75,0x1f,0x4c,0x8b,0x7c,0x24,0x40,0x4c,0x8b,0x64,0x24,0x78,0x4c,0x8b,0xac,0x24,0x80,0x00,0x00,0x00,
0x48,0x8b,0x5c,0x24,0x70,0x48,0x83,0xc4,0x50,0x5f,0x5e,0x5d,0xc3,0x8b,0xd7,0x48,0x8b,0xcb,0xe8,0x93,0x7f,0xf8,0xff,0x3b,0x84,0xbe,0x04,0x01,0x00,0x00,
0x77,0x62,0xbd,0x64,0x00,0x00,0x00,0x0f,0x1f,0x44,0x00,0x00,0x0f,0xb6,0x84,0x37,0xec,0x00,0x00,0x00,0xff,0xc0,0x83,0xff,0x04,0x77,0x0e,0x3b,0xc5,0x8b,
0xc8,0x0f,0x47,0xcd,0x88,0x8c,0x37,0xec,0x00,0x00,0x00,0x3b,0xc5,0x74,0x37,0x8d,0x58,0x01,0xe8,0xa7,0xb8,0x07,0x00,0x83,0xfb,0x65,0x73,0x07,0x48,0x8b,
0x5c,0xd8,0x60,0xeb,0x04,0x48,0x8b,0x58,0x60,0x48,0x8b,0xcb,0xe8,0x9f,0x53,0x07,0x00,0x85,0xc0,0x74,0x13,0x8b,0xd7,0x48,0x8b,0xcb,0xe8,0x31,0x7f,0xf8,
0xff,0x3b,0x84,0xbe,0x04,0x01,0x00,0x00,0x76,0xa8};
// ---- 가짜 경험치 표: 0행은 못 쓰는 행(게임은 101 이상이면 0행을 봄), 1~100행은 필요 경험치(능력 번호별)
struct FRow{int level;int valid;};static FRow frow[101];static unsigned char ftab[0x60+8*101];static unsigned fneed[101][5];
__attribute__((ms_abi)) static unsigned char*stub_table(){return ftab;}
__attribute__((ms_abi)) static int stub_valid(void*r){return r&&((FRow*)r)->valid;}
__attribute__((ms_abi)) static unsigned stub_need(void*r,int idx){return fneed[((FRow*)r)->level][idx];}
static void make_table(unsigned scale,bool zero99){
 for(int L=0;L<=100;L++){frow[L].level=L;frow[L].valid=L>0;FRow*p=&frow[L];memcpy(ftab+0x60+8*L,&p,8);
  for(int i=0;i<5;i++)fneed[L][i]=(unsigned)((unsigned long long)(L*L*140+L*(i+1)*10)*scale);}
 if(zero99)for(int i=0;i<5;i++)fneed[99][i]=0;
}
// ---- 가짜 게임 이미지와 실행 틀
static unsigned char*img;
static void thunk(unsigned rva,void*to){unsigned char*p=img+rva;p[0]=0x48;p[1]=0xb8;memcpy(p+2,&to,8);p[10]=0xff;p[11]=0xe0;}   // mov rax,to ; jmp rax
extern "C"{unsigned long long run_frame(unsigned char*person,unsigned long long idx,unsigned long long stat,void*entry);void run_exit();extern unsigned long long run_rsp_delta;}
// run_frame: 게임 함수 안에 있는 것처럼 레지스터·스택을 꾸며 entry 로 뜀. rsi=인물 rdi=능력번호 r13=처음 능력 r15=0, [rsp+0x48]=r14 저장값(0x1414).
//  끝(run_exit)에서 r14 를 돌려주고, 스택이 그대로였는지(run_rsp_delta)를 남긴다.
__asm__(".text\n.globl run_frame\nrun_frame:\n push %rbx\n push %rbp\n push %r12\n push %r13\n push %r14\n push %r15\n sub $0x98,%rsp\n"
 " mov %rsp,run_saved_rsp(%rip)\n mov %rdx,%r13\n mov %rdi,%rax\n mov %rsi,%rdi\n mov %rax,%rsi\n movq $0x1414,0x48(%rsp)\n mov $0x7777,%r14\n xor %r15d,%r15d\n jmp *%rcx\n"
 ".globl run_exit\nrun_exit:\n mov %rsp,%rax\n sub run_saved_rsp(%rip),%rax\n mov %rax,run_rsp_delta(%rip)\n mov run_saved_rsp(%rip),%rsp\n mov %r14,%rax\n"
 " add $0x98,%rsp\n pop %r15\n pop %r14\n pop %r13\n pop %r12\n pop %rbp\n pop %rbx\n ret\n"
 ".data\nrun_saved_rsp: .quad 0\n.globl run_rsp_delta\nrun_rsp_delta: .quad 0\n.text\n");
static unsigned char person[0x200];
static void set(unsigned idx,unsigned stat,unsigned xp){memset(person,0,sizeof person);person[0xec+idx]=(unsigned char)stat;memcpy(person+0x104+4*idx,&xp,4);}
static unsigned get(unsigned idx){return person[0xec+idx];}
static unsigned long long game_section(unsigned idx,unsigned stat,unsigned xp){set(idx,stat,xp);return run_frame(person,idx,stat,img+0x2bc30a);}
static unsigned long long ours(unsigned idx,unsigned stat,unsigned xp){set(idx,stat,xp);return run_frame(person,idx,stat,(void*)&GrowBridge);}
static int fails;static void check(bool c,const char*m){printf("%s %s\n",c?"  ok  ":"  FAIL",m);if(!c)fails++;}
// 입구 비교 6바이트를 실제로 돌려 보기: r13=v → 계속(1)/멈춤(0)
static int entry_cmp(const unsigned char*six,unsigned v){
 unsigned char*p=img+0x1000;unsigned char pre[]={0x41,0x55,0x41,0xbd,0,0,0,0};memcpy(pre+4,&v,4);memcpy(p,pre,8);            // push r13 ; mov r13d,v
 memcpy(p+8,six,6);unsigned char stop[]={0x31,0xc0,0x41,0x5d,0xc3,0x90,0x90};memcpy(p+14,stop,7);                           // (jb +7 안 탐) xor eax,eax ; pop r13 ; ret ; nop2
 unsigned char go[]={0xb8,1,0,0,0,0x41,0x5d,0xc3};memcpy(p+21,go,8);                                                         // mov eax,1 ; pop r13 ; ret
 return ((int(*)())p)();
}
int main(){
 img=(unsigned char*)mmap(nullptr,0x400000,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);if(img==MAP_FAILED){puts("mmap 실패");return 2;}
 memcpy(img+0x2bc30a,kSection,sizeof kSection);
 thunk(0x337c40,(void*)stub_table);thunk(0x331750,(void*)stub_valid);thunk(0x2442f0,(void*)stub_need);
 thunk(0x2bc334,(void*)&run_exit);thunk(0x2bc3c8,(void*)&run_exit);      // 게임 구간의 두 출구(행 무효 / 정상 끝)
 game=img;GrowRet=(void*)&run_exit;
 check(!memcmp(kSection,kGrowExpected,9),"후크 자리 기대 바이트 = 게임 구간 첫 9바이트(lea ebx,[r13+1]; call 0x337c40)");
 printf("[1] 상한 100 이면 게임 구간과 결과가 같다\n");{
  make_table(1,false);grow_cap=100;int n=0,diff=0;srand(17);
  for(unsigned idx=0;idx<5;idx++)for(unsigned st=1;st<100;st++)for(int k=0;k<12;k++){
   unsigned xp=k==0?0:k==1?fneed[st+1>100?100:st+1][idx]:k==2?fneed[st+1>100?100:st+1][idx]-1:k==3?0xffffffffu:(unsigned)(rand()%1600000);
   unsigned long long r1=game_section(idx,st,xp);unsigned a=get(idx);unsigned long long r2=ours(idx,st,xp);unsigned b=get(idx);n++;
   if(a!=b||r1!=0x1414||r2!=0x1414||run_rsp_delta!=0){if(diff<3)printf("    다름 idx=%u 능력=%u xp=%u 게임=%u 우리=%u r14=%llx/%llx\n",idx,st,xp,a,b,r1,r2);diff++;}}
  char m[120];snprintf(m,sizeof m,"%d가지 전부 같은 능력·r14 복구·스택 그대로 (다름 %d)",n,diff);check(diff==0,m);}
 printf("[2] 100 넘는 성장\n");{
  make_table(1,false);unsigned t99=fneed[99][2],t100=fneed[100][2],step=t100-t99;
  game_section(2,99,0xffffffffu);check(get(2)==100,"게임 구간 그대로: 경험치가 아무리 많아도 100 에서 멈춤(제보 그대로)");
  grow_cap=200;
  ours(2,100,t100+step-1);check(get(2)==100,"우리: 101 에 필요한 경험치(t100+간격)보다 1 모자라면 100 그대로");
  ours(2,100,t100+step);check(get(2)==101,"우리: 딱 채우면 100→101");
  ours(2,100,t100+37*step);check(get(2)==137,"우리: 한 번에 여러 단계(100→137)");
  ours(2,137,t100+37*step+step-1);check(get(2)==137,"우리: 137 에서 다음 단계 모자라면 그대로");
  ours(2,99,t100+100*step);check(get(2)==200,"우리: 99 에서도 표 구간(→100)과 식 구간(→200)을 이어서");
  ours(2,150,0xffffffffu);check(get(2)==200,"우리: 상한은 200");
  ours(2,200,0xffffffffu);check(get(2)==200,"우리: 200 이면 그대로");
  bool all=true;for(unsigned L=101;L<=200;L++){ours(2,L-1,t100+(L-100)*step);if(get(2)!=L)all=false;ours(2,L-1,t100+(L-100)*step-1);if(get(2)!=L-1)all=false;}
  check(all,"우리: 101~200 단계마다 필요 경험치 = t100+(L-100)×(t100−t99) (경계 양쪽 200번 확인)");
  ours(7,100,0xffffffffu);check(true,"능력 번호가 범위 밖(5 이상)이어도 아무것도 안 건드림");}
 printf("[3] 다리(GrowBridge)\n");{
  make_table(1,false);grow_cap=200;unsigned long long r=ours(0,100,0xffffffffu);
  check(r==0x1414,"r14 를 게임 함수가 저장해 둔 값으로 복구(원래 0x2bc313 이 하던 일)");check(run_rsp_delta==0,"스택 위치 그대로 게임 0x2bc3c8 로 돌아감");}
 printf("[4] 안전 조건(v1.9 와 같음)\n");{
  grow_cap=200;make_table(1,true);ours(1,100,0xffffffffu);check(get(1)==100,"99행 필요 경험치가 0 이면 100 위로 안 큼");
  make_table(1000,false);ours(1,100,0xffffffffu);check(get(1)==100,"간격이 10,000,000 을 넘으면(18번 억제 표) 100 위로 안 큼");
  ours(1,50,fneed[60][1]);check(get(1)==60,"억제 표에서도 100 아래는 표대로");
  make_table(1,false);for(int i=0;i<5;i++){fneed[99][i]=0xe0000000u;fneed[100][i]=0xe0800000u;}ours(1,100,0xffffffffu);check(get(1)==162,"필요 경험치가 32비트를 넘는 단계에서 멈춤(100→162, 163 은 넘침)");
  for(int i=0;i<5;i++){fneed[99][i]=0xf0000000u;fneed[100][i]=0xf0000100u;}ours(1,100,0xffffffffu);check(get(1)==100,"t100 이 0xf0000000 이상이면 안 큼");
  frow[100].valid=0;make_table(1,false);frow[100].valid=0;ours(1,100,0xffffffffu);check(get(1)==100,"100행이 무효면 안 큼");}
 printf("[5] 함수 입구 비교 6바이트\n");{
  bool ok=true;for(unsigned v=0;v<=255;v++){int want=v<200;if(entry_cmp(kCaps[13].replacement,v)!=want)ok=false;}
  check(kCaps[13].rva==0x2bc14a&&kCaps[13].len==6&&ok,"우리 바이트(cmp r13b,0xc8; jb): 0~199 계속, 200 이상 멈춤");
  bool orig=true;for(unsigned v=0;v<=255;v++){int want=v!=100;if(entry_cmp(kCaps[13].expected,v)!=want)orig=false;}
  check(orig,"원래 바이트(cmp r13d,0x64; jne): 딱 100 에서만 멈춤");
  const unsigned char old_patch[6]={0x41,0x83,0xfd,0x64,0x72,0x07};bool blocked=true;for(unsigned v=100;v<200;v++)if(entry_cmp(old_patch,v)!=0)blocked=false;
  check(blocked,"11-10 전 패치(jne→jb 만)는 100~199 를 전부 멈추게 했음(제보 원인 재현)");}
 printf(fails?"실패 %d\n":"전부 통과\n",fails);return fails?1:0;
}
