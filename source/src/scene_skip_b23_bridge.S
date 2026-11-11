#include "bridge_macros.inc"
.text
// R42 B2/B3: 0x6c6b10(등용 대사 장면) / 0x6c6dd0(탐색 발견 창) 입구 6바이트(push rdi; sub rsp,0x70) 자리.
//  거르개가 1 을 돌려주면 장면을 쌓지 않고 바로 ret(게임 자체 건너뛰기 0x335110 경로와 같은 결과), 0 이면 원래 6바이트 실행 후 +6 으로.
//  SAVE_VOLATILE 뒤 rax 보관 칸 = 0xb0(%rsp) → 거르개 결과를 거기 넣어 RESTORE 뒤 rax 로 받음(전역 변수 안 씀).
.macro B23_BRIDGE name,filter,ret
.globl \name
\name:
 SAVE_VOLATILE
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call \filter
 mov %rbp,%rsp
 pop %rbp
 mov %rax,0xb0(%rsp)
 RESTORE_VOLATILE
 test %eax,%eax
 jnz 1f
 push %rdi
 sub $0x70,%rsp
 jmp *\ret(%rip)
1:
 ret
.endm
B23_BRIDGE SsEmpBridge,SsEmpFilter,SsEmpRet
B23_BRIDGE SsTreBridge,SsTreFilter,SsTreRet
