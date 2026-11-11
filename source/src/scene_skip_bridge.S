#include "bridge_macros.inc"
.text
// R42c SceneSkip 브리지.
//  FILTER_CALL: 휘발 레지스터 저장 → 거르개(rcx = 들어올 때 rsp(=[돌아갈 주소]), rdx = 저장 틀) → 결과를 rax 칸(0xb0)에 넣고 복원.
//  저장 틀: +0x80 r11 +0x88 r10 +0x90 r9 +0x98 r8 +0xa0 rdx +0xa8 rcx +0xb0 rax +0xb8 flags, +0xc0 = 들어올 때 rsp.
.macro FILTER_CALL filter
 SAVE_VOLATILE
 lea 0xc0(%rsp),%rcx
 mov %rsp,%rdx
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call \filter
 mov %rbp,%rsp
 pop %rbp
 mov %rax,0xb0(%rsp)
 RESTORE_VOLATILE
.endm

// 카메라 요청 CMainCameraCtrl::vt[2] 0x14330 입구 11바이트(sub rsp,0x38 / cmp dword [rcx+0x188],0) 자리.
//  거르개 1 = 막음: 아무것도 안 하고 돌아감(원래 '카메라 잠김' 길과 같이 rax = 요청 구조체). 0 = 원래 11바이트 실행 후 0x1433b.
.globl SsCamBridge
SsCamBridge:
 FILTER_CALL SsCamFilter
 test %eax,%eax
 jnz 1f
 sub $0x38,%rsp
 cmpl $0,0x188(%rcx)
 jmp *SsCamRet(%rip)
1:
 mov %rdx,%rax
 ret

// 게임 이력 추가 0x936980 입구 5바이트(mov rax,rsp / push rbp / push rdi). 거르개는 보기만 함(줄은 언제나 게임이 그대로 넣음).
.globl SsHistBridge
SsHistBridge:
 FILTER_CALL SsHistFilter
 mov %rsp,%rax
 push %rbp
 push %rdi
 jmp *SsHistRet(%rip)

// 월 처리 마지막 단계 0x42ae20 입구 6바이트(push rbx / sub rsp,0x20). 거르개 1 = 결과 창을 띄웠으니 이번엔 단계 0 으로 돌아감(다음에 다시 불림).
.globl SsTurnBridge
SsTurnBridge:
 FILTER_CALL SsTurnEnd
 test %eax,%eax
 jnz 1f
 push %rbx
 sub $0x20,%rsp
 jmp *SsTurnRet(%rip)
1:
 xor %eax,%eax
 ret

// 재해 진행 중 효과음 0x403810(rcx 관리자, edx 종류, xmm2/xmm3 0) 부르는 자리: 이 때 rdi = 방금 만든 알림 문장 → 넷째 인자(r9)로.
.globl SsDisSoundBridge
SsDisSoundBridge:
 mov %rdi,%r9
 jmp SsDisSound


