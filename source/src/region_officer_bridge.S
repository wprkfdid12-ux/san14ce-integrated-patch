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
// R41f F1 0x1a92c5: CCapitalInfoLayout vf2(매 프레임 갱신) 끝 에필로그. rbx=판.
//  (R41d 는 함수 입구 0x1a9250 에 붙였는데, AI 모드 편의 DLL 이 0x1a9250 앞 0x5d 바이트를 해시로 검사해 지역담당관 초상화를 끄므로 검사 범위 밖 에필로그로 옮김)
//  원래 11바이트(mov rbx,[rsp+0x30]; add rsp,0x20; pop rdi; ret)를 여기서 실행.
.globl RoFrameC
RoFrameC:
 SAVE_VOLATILE
 mov %rbx,%rcx
 xor %edx,%edx
 ALIGNED_CALL RoFrame
 RESTORE_VOLATILE
 mov 0x30(%rsp),%rbx
 add $0x20,%rsp
 pop %rdi
 ret
// R41f F2 0x1a9241: CFootholdInfoLayout vf2 끝 에필로그(같은 11바이트). AI 해시 범위 0x1a91a0+0x40 밖.
.globl RoFrameF
RoFrameF:
 SAVE_VOLATILE
 mov %rbx,%rcx
 mov $1,%edx
 ALIGNED_CALL RoFrame
 RESTORE_VOLATILE
 mov 0x30(%rsp),%rbx
 add $0x20,%rsp
 pop %rdi
 ret
