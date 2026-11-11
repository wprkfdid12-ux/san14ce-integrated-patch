#include "bridge_macros.inc"
.text
// 호출 규약: 휘발성 레지스터·플래그 보존, 16바이트 정렬 맞춘 뒤 C 함수 호출 (rbp 는 보존 후 복구)
.macro ALIGNED_CALL fn
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call \fn
 mov %rbp,%rsp
 pop %rbp
.endm
.globl FaceBridge1
FaceBridge1:
 mov %ax,0x10(%rdi)
 lea 0x20(%rsp),%rcx
 SAVE_VOLATILE
 mov %rdi,%rcx
 movzwl 0xa6(%rdi),%edx
 ALIGNED_CALL FaceAssign
 RESTORE_VOLATILE
 jmp *FaceRet1(%rip)
.globl FaceBridge2
FaceBridge2:
 decl 0x8(%r15)
 mov %rdi,%rax
 SAVE_VOLATILE
 ALIGNED_CALL FaceRescan
 RESTORE_VOLATILE
 jmp *FaceRet2(%rip)
.globl FaceBridge3
FaceBridge3:
 mov %al,0x165(%r15)
 SAVE_VOLATILE
 mov %r15,%rcx
 xor %edx,%edx
 ALIGNED_CALL FaceAssign
 RESTORE_VOLATILE
 jmp *FaceRet3(%rip)
.globl FaceBridge4
FaceBridge4:
 xor %eax,%eax
 cmp %eax,0x50(%rdi)
 sete %al
 SAVE_VOLATILE
 test %eax,%eax
 je 1f
 cmpl $1,0x20(%rdi)
 jne 1f
 ALIGNED_CALL FaceRescan
1:
 RESTORE_VOLATILE
 jmp *FaceRet4(%rip)
