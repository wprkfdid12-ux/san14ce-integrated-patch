#include "bridge_macros.inc"
.text
.macro ALIGNED_CALL fn
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call \fn
 mov %rbp,%rsp
 pop %rbp
.endm
// H1 0x230978: 이름 함수 끝. rdi=무장, rbx=이름 버퍼. 원래 끝(mov rax,rbx / 복구 / ret)을 여기서 수행.
.globl ScBridge1
ScBridge1:
 SAVE_VOLATILE
 mov %rdi,%rcx
 mov %rbx,%rdx
 ALIGNED_CALL ScNameSeen
 RESTORE_VOLATILE
 mov %rbx,%rax
 mov 0x30(%rsp),%rbx
 add $0x20,%rsp
 pop %rdi
 ret
// H2 0x9332b0: 글자칸 넣기 입구. r8=글자, [rsp]=돌아갈 주소. 바뀐 글자를 r8 로.
.globl ScBridge2
// R39b: 0x9332c5 (프롤로그 뒤). 이때 반환 주소 = 진입 rsp 기준 [rsp+0x28] → SAVE_VOLATILE 뒤 0xe8
ScBridge2:
 SAVE_VOLATILE
 mov 0x98(%rsp),%rcx
 mov 0xe8(%rsp),%rdx
 mov 0xa8(%rsp),%r8
 ALIGNED_CALL ScLabel
 mov %rax,0x98(%rsp)
 RESTORE_VOLATILE
 movslq %edx,%rsi
 mov %r8,%rdi
 mov %esi,%edx
 mov %rcx,%r14
 jmp *ScRet2(%rip)
// H3 0x85e750: 능력치 칸 입구. edx=번호 r8d=값 r9d=기본값
.globl ScBridge3
ScBridge3:
 mov %edx,ScStatIdx(%rip)
 mov %r8d,ScStatVal(%rip)
 mov %r9d,ScStatBase(%rip)
 push %rbx
 push %rbp
 push %rsi
 push %rdi
 push %r14
 sub $0x840,%rsp
 jmp *ScRet3(%rip)
// H4 0x231770: 능력치 계산 입구. rcx=무장 edx=번호 (rax 는 입구에서 쓰지 않음)
.globl ScBridge4
ScBridge4:
 mov %rcx,ScLastP(%rip)
 mov %edx,ScLastIdx(%rip)
 mov %gs:0x48,%eax
 mov %eax,ScLastTid(%rip)
 mov %edx,0x10(%rsp)
 push %rbp
 push %rsi
 push %rdi
 jmp *ScRet4(%rip)
// H5 0x154ec0: 색 기호 분기. 0x0A 면 회색 칠하고 함수 끝(0x154138)으로, 아니면 원래 cmp eax,9 → 0x154ecb(ja)
.globl ScBridge5
ScBridge5:
 movzbl 0x8(%r8),%eax
 mov %r8,%rdi
 cmp $0x0a,%eax
 je 1f
 cmp $0x9,%eax
 jmp *ScRet5(%rip)
1:
 SAVE_VOLATILE
 mov %rdi,%rcx
 ALIGNED_CALL ScGray
 RESTORE_VOLATILE
 jmp *ScExit5(%rip)
// R41 H6 0x9333c9 (0x9333c0 안, test/je 뒤): rcx=글자 요소 rdx=글자 [rsp]=돌아갈 주소. 바뀐 글자를 rdx 로, 원래 12바이트 실행 후 0x930e45 로.
.globl ScBridge6
ScBridge6:
 SAVE_VOLATILE
 mov 0xa8(%rsp),%rcx
 mov 0xa0(%rsp),%rdx
 mov 0xc0(%rsp),%r8
 ALIGNED_CALL ScText
 mov %rax,0xa0(%rsp)
 RESTORE_VOLATILE
 mov %rsp,%r11
 push %rbx
 push %rsi
 sub $0x838,%rsp
 jmp *ScRet6(%rip)
