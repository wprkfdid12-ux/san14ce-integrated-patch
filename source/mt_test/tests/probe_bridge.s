.text
.globl probe_test
.type probe_test,@function
# SysV inputs: rdi=actual Win64 __chkstk address, rsi=size, rdx=test stack top.
# The helper must preserve RAX, RSP, and all argument registers.
probe_test:
 push %r12
 push %r13
 push %r14
 push %r15
 mov %rsp,%r12
 mov %rdi,%r13
 mov %rsi,%r14
 mov %rdx,%r15
 mov %r15,%rsp
 mov %r14,%rax
 mov $1,%rcx
 mov $2,%rdx
 mov $3,%r8
 mov $4,%r9
 call *%r13
 cmp %r14,%rax
 jne .Lbad
 cmp %r15,%rsp
 jne .Lbad
 cmp $1,%rcx
 jne .Lbad
 cmp $2,%rdx
 jne .Lbad
 cmp $3,%r8
 jne .Lbad
 cmp $4,%r9
 jne .Lbad
 xor %eax,%eax
 jmp .Lout
.Lbad:
 mov $1,%eax
.Lout:
 mov %r12,%rsp
 pop %r15
 pop %r14
 pop %r13
 pop %r12
 ret
.size probe_test,.-probe_test
.section .note.GNU-stack,"",@progbits
