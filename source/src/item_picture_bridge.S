#include "bridge_macros.inc"
.text
// R41e 0x686100 입구(명품 선택 목록 열기): rcx=목록 설정. 그림 방식 정한 뒤 원래 9바이트(push rdi; sub rsp,0x480) 실행, +9 로.
.globl ItBridge
ItBridge:
 SAVE_VOLATILE
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call ItFix
 mov %rbp,%rsp
 pop %rbp
 RESTORE_VOLATILE
 push %rdi
 sub $0x480,%rsp
 jmp *ItRet(%rip)
