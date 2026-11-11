#include "bridge_macros.inc"
.text
// R42 B4 G1: 0x67261e = 0x66fd80(포로 처우 창 버튼 켜고 끄기) 끝. r13 = 창. 원래 5바이트(mov rbx,[rsp+0x60]) 실행 후 0x672623 으로.
.globl ElBridge
ElBridge:
 SAVE_VOLATILE
 mov %r13,%rcx
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call ElFix
 mov %rbp,%rsp
 pop %rbp
 RESTORE_VOLATILE
 mov 0x60(%rsp),%rbx
 jmp *ElRet(%rip)
