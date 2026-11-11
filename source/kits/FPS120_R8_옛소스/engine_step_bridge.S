
.text
.globl engine_step_bridge
.type engine_step_bridge, @function
engine_step_bridge:
 subq $216, %rsp
 movq %rcx, 32(%rsp)
 movq %rdx, 40(%rsp)
 movq %r8, 48(%rsp)
 movq %r9, 56(%rsp)
 movq %r10, 64(%rsp)
 movq %r11, 72(%rsp)
 movdqu %xmm0, 80(%rsp)
 movdqu %xmm1, 96(%rsp)
 movdqu %xmm2, 112(%rsp)
 movdqu %xmm3, 128(%rsp)
 movdqu %xmm4, 144(%rsp)
 movdqu %xmm5, 160(%rsp)
 movl 52(%rax), %ecx
 call engine_step_bits
 movl %eax, 24(%r15)
 movdqu 80(%rsp), %xmm0
 movdqu 96(%rsp), %xmm1
 movdqu 112(%rsp), %xmm2
 movdqu 128(%rsp), %xmm3
 movdqu 144(%rsp), %xmm4
 movdqu 160(%rsp), %xmm5
 movq 32(%rsp), %rcx
 movq 40(%rsp), %rdx
 movq 48(%rsp), %r8
 movq 56(%rsp), %r9
 movq 64(%rsp), %r10
 movq 72(%rsp), %r11
 addq $216, %rsp
 ret
.size engine_step_bridge, .-engine_step_bridge
