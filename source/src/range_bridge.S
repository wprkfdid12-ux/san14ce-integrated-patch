#include "bridge_macros.inc"
.text
// 휘발성 레지스터·플래그 보존, 16바이트 정렬 후 C 호출 (rbp 는 보존 후 복구)
.macro ALIGNED_CALL fn
 push %rbp
 mov %rsp,%rbp
 and $-16,%rsp
 sub $0x20,%rsp
 call \fn
 mov %rbp,%rsp
 pop %rbp
.endm
// SAVE_VOLATILE 뒤 저장 칸: r11 0x80, r10 0x88, r9 0x90, r8 0x98, rdx 0xa0, rcx 0xa8, rax 0xb0, flags 0xb8

// T 0x2c9109: mov eax,ebx ; mov rbx,[rsp+0x40]   (rsi = 진형, ebx = 원거리 0/1)
.globl RangeBridgeT
RangeBridgeT:
 SAVE_VOLATILE
 mov %rsi,%rcx
 mov %ebx,%edx
 ALIGNED_CALL RangeTail
 mov %eax,%ebx
 RESTORE_VOLATILE
 mov %ebx,%eax
 mov 0x40(%rsp),%rbx
 jmp *RangeRetT(%rip)

// C1 0x1f8bc: edi = 사거리 (진형 rdi), ecx = 2
.globl RangeBridgeC1
RangeBridgeC1:
 SAVE_VOLATILE
 mov %rdi,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeFor
 mov %eax,%edi
 RESTORE_VOLATILE
 mov $2,%ecx
 jmp *RangeRetC1(%rip)

// C2 0x16dced: ebx = 사거리 (진형 rbp)
.globl RangeBridgeC2
RangeBridgeC2:
 SAVE_VOLATILE
 mov %rbp,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeFor
 mov %eax,%ebx
 RESTORE_VOLATILE
 jmp *RangeRetC2(%rip)

// C3 0x16dd14: ebx = 사거리 (진형 r14)
.globl RangeBridgeC3
RangeBridgeC3:
 SAVE_VOLATILE
 mov %r14,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeFor
 mov %eax,%ebx
 RESTORE_VOLATILE
 jmp *RangeRetC3(%rip)

// C4 0x170336: ebx = 사거리 (진형 rbx)
.globl RangeBridgeC4
RangeBridgeC4:
 SAVE_VOLATILE
 mov %rbx,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeFor
 mov %eax,%ebx
 RESTORE_VOLATILE
 jmp *RangeRetC4(%rip)

// C5 0x2cb976: 원거리면 edi += 사거리-1 (진형 rsi)
.globl RangeBridgeC5
RangeBridgeC5:
 SAVE_VOLATILE
 mov %rsi,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeC5
 add %eax,%edi
 RESTORE_VOLATILE
 jmp *RangeRetC5(%rip)

// C6 0x2ccad1 (12바이트): ebp = ecx = 사거리 (진형 rsi). 돌아가는 곳 0x2ca72d: cmovne ecx,ebp ; mov ebp,ecx
.globl RangeBridgeC6
RangeBridgeC6:
 SAVE_VOLATILE
 mov %rsi,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeFor
 mov %eax,%ebp
 RESTORE_VOLATILE
 mov %ebp,%ecx
 jmp *RangeRetC6(%rip)

// C7 0x88cff9: ecx = 사거리 (진형 rbx)
.globl RangeBridgeC7
RangeBridgeC7:
 SAVE_VOLATILE
 mov %rbx,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeFor
 mov %eax,%eax
 mov %rax,0xa8(%rsp)
 RESTORE_VOLATILE
 jmp *RangeRetC7(%rip)

// P0 0x2c9424: movzx r14d,[rbx+0x3c] ; mov r8d,eax  → r8 = RangeP0(부대 rbx, eax)
.globl RangeBridgeP0
RangeBridgeP0:
 movzbl 0x3c(%rbx),%r14d
 mov %eax,%r8d
 SAVE_VOLATILE
 mov %rbx,%rcx
 mov %eax,%edx
 ALIGNED_CALL RangeP0
 mov %rax,0x98(%rsp)
 RESTORE_VOLATILE
 jmp *RangeRetP0(%rip)

// P1 0x2c94aa: (원거리, 행동 4) 반경 3 목록 → 거리 ≤ 1+사거리. 중심 edx, 부대 칸 ebx
.globl RangeBridgeP1
RangeBridgeP1:
 SAVE_VOLATILE
 mov %r8,%rcx
 mov %ebx,%r8d
 mov $2,%r9d
 ALIGNED_CALL RangeP12
 cmp $1,%eax
 je 1f
 cmp $2,%eax
 je 2f
 RESTORE_VOLATILE
 mov $3,%r8d
 lea 0x40(%rsp),%rcx
 jmp *RangeRetP1(%rip)
1:
 RESTORE_VOLATILE
 jmp *RangeInP1(%rip)
2:
 RESTORE_VOLATILE
 jmp *RangeOut(%rip)

// P2 0x2c95e1: (원거리, 행동 5~7) 반경 2 목록 → 거리 ≤ 사거리. 중심 edx, 부대 칸 ebx
.globl RangeBridgeP2
RangeBridgeP2:
 SAVE_VOLATILE
 mov %r8,%rcx
 mov %ebx,%r8d
 mov $1,%r9d
 ALIGNED_CALL RangeP12
 cmp $1,%eax
 je 1f
 cmp $2,%eax
 je 2f
 RESTORE_VOLATILE
 mov $2,%r8d
 lea 0x40(%rsp),%rcx
 jmp *RangeRetP2(%rip)
1:
 RESTORE_VOLATILE
 jmp *RangeInP1(%rip)
2:
 RESTORE_VOLATILE
 jmp *RangeOut(%rip)

// P3 0x2c96dc: (원거리, 대상 도시) 반경 3 → 거리 ≤ 1+사거리. 부대 rbx, 중심 edx, 부대 칸 edi, 대상종류 esi
.globl RangeBridgeP3
RangeBridgeP3:
 SAVE_VOLATILE
 mov %rbx,%rcx
 mov %edi,%r8d
 mov %esi,%r9d
 shl $8,%r9d
 or $2,%r9d
 ALIGNED_CALL RangeP34
 cmp $1,%eax
 je 1f
 cmp $2,%eax
 je 2f
 RESTORE_VOLATILE
 lea -0x2(%rsi),%r8d
 lea 0x40(%rsp),%rcx
 jmp *RangeRetP3(%rip)
1:
 RESTORE_VOLATILE
 jmp *RangeInP3(%rip)
2:
 RESTORE_VOLATILE
 jmp *RangeOut(%rip)

// P4 0x2c974f: (원거리·대상 도시 아님) 반경 2 → 거리 ≤ 사거리. 대상이 도시(5)면 원거리 아닌 길이라 원래 코드
.globl RangeBridgeP4
RangeBridgeP4:
 SAVE_VOLATILE
 mov %rbx,%rcx
 mov %edi,%r8d
 mov %esi,%r9d
 shl $8,%r9d
 or $1,%r9d
 ALIGNED_CALL RangeP34
 cmp $1,%eax
 je 1f
 cmp $2,%eax
 je 2f
 RESTORE_VOLATILE
 lea 0x40(%rsp),%rcx
 mov $2,%r8d
 jmp *RangeRetP4(%rip)
1:
 RESTORE_VOLATILE
 jmp *RangeInP3(%rip)
2:
 RESTORE_VOLATILE
 jmp *RangeOut(%rip)

// P5 0x2b76e2: mov r9d,0xbd10 ; mov r8d,ebx  → r8d = RangeP5(부대 r14, ebx)
.globl RangeBridgeP5
RangeBridgeP5:
 mov $0xbd10,%r9d
 mov %ebx,%r8d
 SAVE_VOLATILE
 mov %r14,%rcx
 mov %ebx,%edx
 ALIGNED_CALL RangeP5
 mov %eax,%eax
 mov %rax,0x98(%rsp)
 RESTORE_VOLATILE
 jmp *RangeRetP5(%rip)
