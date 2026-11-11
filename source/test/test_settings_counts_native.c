// P204(11-10 확정): 기색 모드가 게임 목록 만들기(0x658df0) 들머리를 후크해 탭별 줄 수를 게임 기본값(9,8,7,8,10,19)으로 되돌림 → 통합패치 모드 창이 깨짐.
// 고침: 그 후크 바로 뒤(0x658dfa)의 브리지 C(MsCntBridge)가 우리 창이면 줄 수를 다시 씀.
// 이 시험: 실제 브리지 코드(src/mod_settings_bridge.S)를 그대로 돌려
//   ① 기색 흉내(들머리에서 기본값으로 되돌림) 뒤에 브리지를 타면 우리 줄 수가 됨  ② 우리 창이 아니면(다른 layout·창 닫힘) 한 바이트도 안 바꿈
//   ③ 원래 명령(movq $-2,0x20(%rsp))을 그대로 실행하고 레지스터·스택을 안 깨뜨림  ④ 브리지가 없으면(고치기 전) 깨진 채인 것 확인
//  실행: clang -O1 -w test/test_settings_counts_native.c src/mod_settings_bridge.S -o /tmp/t && /tmp/t
#include <stdio.h>
#include <string.h>
#include <stdint.h>
void*ModClick,*ModEnWatch,*ModGet,*ModMsg,*ModMsgFilter,*ModSet,*ModStd,*MsClampRet,*MsClickOrig,*MsGetJa,*MsGetRet,*MsMsgOrig,*MsRngRet,*MsSbRet,*MsSetJa,*MsSetRet,*MsStdRet;
int MsEntering,RowMax[64],RowMin[64];
volatile int MsCnt[6];volatile long MsCntHits;void*MsCntRet;unsigned char*volatile MsOurLayout;
void MsCntBridge(void);
// 게임 함수 흉내: sub rsp,0x40 뒤 둘째 명령 자리에서 브리지로 jmp, 돌아오면 [rsp+0x20] 과 레지스터를 돌려줌
uint64_t run_builder(unsigned char*layout,uint64_t*regs_out);
__asm__(".text\n.globl run_builder\nrun_builder:\n"
" push %rbx\n push %rbp\n push %r12\n push %r13\n push %r14\n push %r15\n"
" mov %rsi,%r12\n mov %rdi,%rcx\n"                       // rcx = layout (윈도 호출 규약의 첫 인자)
" mov $0x1111,%rax\n mov $0x2222,%rdx\n mov $0x8888,%r8\n mov $0x9999,%r9\n mov $0xaaaa,%r10\n mov $0xbbbb,%r11\n mov $0x5555,%rbx\n"
" sub $0x48,%rsp\n movq $0x77,0x20(%rsp)\n"
" lea 1f(%rip),%r13\n mov %r13,MsCntRet(%rip)\n mov %rsp,%r14\n"
" jmp MsCntBridge\n"
"1: cmp %rsp,%r14\n jne 9f\n"
" mov %rax,0(%r12)\n mov %rdx,8(%r12)\n mov %r8,16(%r12)\n mov %r9,24(%r12)\n mov %r10,32(%r12)\n mov %r11,40(%r12)\n mov %rbx,48(%r12)\n mov %rcx,56(%r12)\n"
" mov 0x20(%rsp),%rax\n add $0x48,%rsp\n pop %r15\n pop %r14\n pop %r13\n pop %r12\n pop %rbp\n pop %rbx\n ret\n"
"9: ud2\n");
static int fails;
#define CK(c,m) do{printf("  %s %s\n",(c)?"ok  ":"FAIL",m);if(!(c))fails++;}while(0)
static const int NATIVE[6]={9,8,7,8,10,19},OURS[6]={24,16,4,1,2,1};
static void condition_restore(unsigned char*l){memcpy(l+0x17c,NATIVE,24);}   // 기색 CondSetupRows: 합이 61 이 아니면 기억해 둔 기본값으로
int main(void){
 static unsigned char lay[0x2000],other[0x2000],snap[0x2000];uint64_t r[8];
 for(int i=0;i<6;i++)MsCnt[i]=OURS[i];
 // ④ 고치기 전: 기색이 되돌린 그대로 → 깨짐
 memcpy(lay+0x17c,OURS,24);condition_restore(lay);CK(memcmp(lay+0x17c,NATIVE,24)==0,"브리지 없이는 기색이 되돌린 기본값 그대로(버그 재현)");
 // ① 우리 창: 기색 후크 → 브리지 → 우리 줄 수
 memset(lay,0xcc,sizeof lay);memcpy(lay+0x17c,OURS,24);MsOurLayout=lay;condition_restore(lay);memcpy(snap,lay,sizeof lay);
 uint64_t v=run_builder(lay,r);
 CK(memcmp(lay+0x17c,OURS,24)==0,"우리 창: 기색이 되돌린 뒤에도 줄 수 = 우리 것(24,16,4,1,2,1)");
 memcpy(snap+0x17c,OURS,24);CK(memcmp(lay,snap,sizeof lay)==0,"줄 수 6칸(24바이트) 말고는 layout 을 안 건드림(+0x194 깃발 포함)");
 CK(v==(uint64_t)-2,"원래 명령 movq $-2,0x20(%rsp) 실행됨");
 CK(r[0]==0x1111&&r[1]==0x2222&&r[2]==0x8888&&r[3]==0x9999&&r[4]==0xaaaa&&r[5]==0xbbbb&&r[6]==0x5555&&r[7]==(uint64_t)lay,"레지스터(rax·rdx·r8~r11·rbx·rcx) 그대로");
 CK(MsCntHits==1,"다시 쓴 횟수 기록 1");
 // ② 게임 본래 설정 화면(다른 layout): 손 안 댐 — 기색의 나이 줄·게임 기본 줄 수 그대로
 memset(other,0x11,sizeof other);memcpy(other+0x17c,NATIVE,24);memcpy(snap,other,sizeof other);
 v=run_builder(other,r);CK(memcmp(other,snap,sizeof other)==0&&v==(uint64_t)-2&&MsCntHits==1,"다른 layout(게임 본래 설정 화면): 한 바이트도 안 바꿈");
 // 창 닫힘(MsOurLayout=0): 같은 layout 이어도 손 안 댐
 MsOurLayout=0;memcpy(lay+0x17c,NATIVE,24);memcpy(snap,lay,sizeof lay);v=run_builder(lay,r);
 CK(memcmp(lay,snap,sizeof lay)==0&&MsCntHits==1,"우리 창이 닫힌 뒤: 안 바꿈");
 // 탭을 누를 때마다 게임이 이 함수를 다시 부름 → 매번 기색이 되돌리고 매번 다시 씀
 MsOurLayout=lay;int ok=1;for(int k=0;k<50;k++){condition_restore(lay);run_builder(lay,r);ok&=memcmp(lay+0x17c,OURS,24)==0;}
 CK(ok&&MsCntHits==51,"탭 50번 바꿔도 매번 우리 줄 수");
 puts(fails?"실패":"전부 통과");return fails?1:0;
}
