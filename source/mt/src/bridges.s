.text
.globl merit_steam_bridge
.def merit_steam_bridge; .scl 2; .type 32; .endef
.seh_proc merit_steam_bridge
merit_steam_bridge:
    subq $40, %rsp
    .seh_stackalloc 40
    .seh_endprologue
    movq %rbx, 32(%rsp)
    call merit_write_steam
    addq $40, %rsp
    cmpq $16, 24(%rbx)
    retq
.seh_endproc
.globl merit_file_bridge
.def merit_file_bridge; .scl 2; .type 32; .endef
.seh_proc merit_file_bridge
merit_file_bridge:
    subq $40, %rsp
    .seh_stackalloc 40
    .seh_endprologue
    movq %rbx, %r9
    call merit_write_file
    addq $40, %rsp
    testl %eax, %eax
    retq
.seh_endproc

/* Exact presentation gate at CEKO 6cbabe, after cost/trait commit and native
 * bookkeeping. Original presentation 6cbabe..6cbed0 contains fade/thank-you UI.
 * The common cleanup at 6cbf12 still needs rdi=[rbp-19], normally restored at
 * 6cbad3. Restore it on the shortcut too. No game logic/cleanup is skipped. */
.globl merit_presentation_gate
.def merit_presentation_gate; .scl 2; .type 32; .endef
merit_presentation_gate:
    cmpq $0, merit_presentation_skip(%rip)
    je merit_original_presentation
    movq -25(%rbp), %rdi
    jmpq *merit_presentation_skip(%rip)
merit_original_presentation:
    movq 1224(%rsi), %rax
    jmpq *merit_presentation_resume(%rip)
