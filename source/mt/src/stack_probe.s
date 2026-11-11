/* Local Win64 stack-probe helper. Contract: RAX is the requested allocation;
 * only R10/R11 and flags may change. Do not adjust RSP or argument registers.
 * Probe every 4096-byte page before the caller subtracts the allocation.
 * See Microsoft x64 prolog/epilog documentation (source/TECHNICAL_SOURCES.txt).
 */
.text
.globl __chkstk
.def __chkstk; .scl 2; .type 32; .endef
.seh_proc __chkstk
__chkstk:
.seh_endprologue
    lea 8(%rsp), %r11
    mov %rax, %r10
.Lprobe_page:
    cmp $4096, %r10
    jb .Lprobe_tail
    sub $4096, %r11
    testb $0, (%r11)
    sub $4096, %r10
    jmp .Lprobe_page
.Lprobe_tail:
    sub %r10, %r11
    testb $0, (%r11)
    ret
.seh_endproc
