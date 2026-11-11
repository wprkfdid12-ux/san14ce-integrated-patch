/* Stack probe used by the mingw x64 ABI for frames larger than one page. */
.text
.globl ___chkstk_ms
___chkstk_ms:
 push %rcx
 push %rax
 cmp $0x1000,%rax
 lea 24(%rsp),%rcx
 jb 2f
1:
 sub $0x1000,%rcx
 orq $0,(%rcx)
 sub $0x1000,%rax
 cmp $0x1000,%rax
 ja 1b
2:
 sub %rax,%rcx
 orq $0,(%rcx)
 pop %rax
 pop %rcx
 ret
