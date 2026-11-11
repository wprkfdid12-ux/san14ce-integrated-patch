#include "bridge_macros.inc"
.text
// R39 1번 새 모드 설정 창 다리. 우리 설정 id = 0x100~0x1ff, 우리 설명 글 id = 0x7ff00000~0x7ff000ff (게임은 2,000,000 이상 글 id 를 안 씀).
// 함수 첫머리에서 가로채므로 rax·플래그는 마음대로 써도 된다(호출 규약상 휘발성).
.macro ALIGNED_CALL fn
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call \fn
 mov %rbp,%rsp
 pop %rbp
.endm

// A: 0x3ec920 get(struct, id, sub) — 원래 첫 9바이트 cmp edx,0x3c / ja rel32
.globl MsGetBridge
MsGetBridge:
 lea -0x100(%rdx),%eax
 cmp $0x100,%eax
 jae 1f
 mov %edx,%ecx
 jmp ModGet
1:
 cmp $0x3c,%edx
 jbe 2f
 jmp *MsGetJa(%rip)
2:
 jmp *MsGetRet(%rip)

// B: 0x3f2200 set(struct, id, sub, val)
.globl MsSetBridge
MsSetBridge:
 lea -0x100(%rdx),%eax
 cmp $0x100,%eax
 jae 1f
 mov %edx,%ecx
 mov %r9d,%edx
 jmp ModSet
1:
 cmp $0x3c,%edx
 jbe 2f
 jmp *MsSetJa(%rip)
2:
 jmp *MsSetRet(%rip)

// D: 0x638b00 [표준]/글상자(layout, ctl) — 원래 첫 12바이트 push rsi; push rdi; push r14; sub rsp,0x90
.globl MsStdBridge
MsStdBridge:
 mov 0x88(%rdx),%eax
 sub $0x100,%eax
 cmp $0x100,%eax
 jae 1f
 cmpl $-1000,0x80(%rdx)          // [표준] 단추만 우리 처리. 숫자 칸(값 != -1000)은 원래 입력창으로(범위는 훅 H)
 jne 1f
 jmp ModStd
1:
 push %rsi
 push %rdi
 push %r14
 sub $0x90,%rsp
 jmp *MsStdRet(%rip)

// E: CBaseMsgMgr vfunc1 (this, id) → wchar*
.globl MsMsgBridge
MsMsgBridge:
 lea -0x7ff00000(%rdx),%eax
 cmp $0x100,%eax
 jae 1f
 mov %edx,%ecx
 jmp ModMsg
1:
 cmpl $0,MsEntering(%rip)        // 우리 창을 여는 동안만: 원래 글을 받아 '숙련 모드' 제목을 '모드'로
 je 2f
 jmp ModMsgFilter
2:
 jmp *MsMsgOrig(%rip)

// IC '모드' 버튼: IC 릴레이 포인터가 여기로 온다. 우리 창을 열면 0 반환, 아니면 레지스터 그대로 IC 원래 처리로.
.globl MsClickBridge
MsClickBridge:
 SAVE_VOLATILE
 ALIGNED_CALL ModClick
 test %eax,%eax
 jnz 1f
 RESTORE_VOLATILE
 jmp *MsClickOrig(%rip)
1:
 RESTORE_VOLATILE
 xor %eax,%eax
 ret

// H: 0x638c48 (13바이트) 입력창 범위: rax = id(부호 확장), rcx = id*0x1c, r8 = 게임 표.
//  원래: [rsp+0x38]=표[+0], eax=표[+4] (그 뒤 원래 코드가 mov [rsp+0x3c],eax). 우리 id 면 RowMin/RowMax.
.globl MsRngBridge
MsRngBridge:
 push %rdx
 push %r9
 lea -0x100(%rax),%edx
 cmp $64,%edx
 jae 1f
 lea RowMin(%rip),%r9
 mov (%r9,%rdx,4),%eax
 mov %eax,0x48(%rsp)
 lea RowMax(%rip),%r9
 mov (%r9,%rdx,4),%eax
 pop %r9
 pop %rdx
 jmp *MsRngRet(%rip)
1:
 mov (%rcx,%r8),%eax
 mov %eax,0x48(%rsp)
 mov 4(%rcx,%r8),%eax
 pop %r9
 pop %rdx
 jmp *MsRngRet(%rip)
// W: 0x14140 (CCmnButton/RadioButton vfunc 0xd0, 15바이트 전체) — 끄기(edx=0)면 ModEnWatch(ctl, 돌아갈 자리) 가 1 을 주면 막음
.globl MsEnBridge
MsEnBridge:
 test %edx,%edx
 jnz 2f
 SAVE_VOLATILE
 mov 0xc0(%rsp),%rdx
 ALIGNED_CALL ModEnWatch
 mov %rax,0xb0(%rsp)
 RESTORE_VOLATILE
 test %eax,%eax
 jz 3f
 ret
3:
 xor %edx,%edx
2:
 andl $-5,0x40(%rcx)
 neg %edx
 sbb %eax,%eax
 and $4,%eax
 or %eax,0x40(%rcx)
 ret
// S: 0x658e6d (14바이트) 탭 바꿀 때 목록 스크롤바(list+0x188) 보이기 — 원래는 탭 4·5만. 우리 창이면 줄 수(r15d) >= 칸 수(list+0x178) 일 때 보임
.globl MsSbBridge
MsSbBridge:
 cmp MsOurLayout(%rip),%rbp
 jne 1f
 mov 0x8c0(%rbp),%r8
 xor %edx,%edx
 cmp 0x178(%r8),%r15d
 jl 2f
 mov $1,%edx
 jmp 2f
1:
 cmp $4,%edx
 setge %dl
 movzbl %dl,%edx
2:
 jmp *MsSbRet(%rip)
// C: 0x658dfa (9바이트, 목록 만들기 0x658df0 의 둘째 명령 'movq $-2,0x20(%rsp)') — 우리 창이면 탭별 줄 수(layout+0x17c, 6칸)를 여기서 다시 씀.
//    기색 모드(Condition.SetupRow)가 이 함수 들머리(0x658df0, 10바이트)를 후크해 줄 수를 게임 기본값(9,8,7,8,10,19)으로 되돌리기 때문(P204 확정 11-10).
//    그 후크가 돈 바로 뒤, 함수 몸통이 줄 수를 읽기(0x658e54) 전이라 순서와 상관없이 우리 값이 쓰인다. rcx = layout(아직 안 바뀜).
.globl MsCntBridge
MsCntBridge:
 cmp MsOurLayout(%rip),%rcx
 jne 1f
 push %rax
 mov MsCnt(%rip),%rax
 mov %rax,0x17c(%rcx)
 mov MsCnt+8(%rip),%rax
 mov %rax,0x184(%rcx)
 mov MsCnt+16(%rip),%rax
 mov %rax,0x18c(%rcx)
 lock incl MsCntHits(%rip)
 pop %rax
1:
 movq $-2,0x20(%rsp)
 jmp *MsCntRet(%rip)
// K: 0x629c40 (9바이트) 계산기 확인·[표준] 공용 적용 0x627310 의 범위 읽기(전역 표 r12 + id*0x1c): rax=id, rcx=id*0x1c → ebx=최소, eax=최대. 우리 id 면 RowMin/RowMax
.globl MsClampBridge
MsClampBridge:
 push %rdx
 lea -0x100(%rax),%edx
 cmp $64,%edx
 jae 1f
 push %r9
 lea RowMin(%rip),%r9
 mov (%r9,%rdx,4),%ebx
 lea RowMax(%rip),%r9
 mov (%r9,%rdx,4),%eax
 pop %r9
 pop %rdx
 jmp *MsClampRet(%rip)
1:
 pop %rdx
 mov (%rcx,%r12),%ebx
 mov 4(%rcx,%r12),%eax
 jmp *MsClampRet(%rip)
