# RTK14 DragLearn (1인 드래그습득) - machine-code block (x64, Windows ABI)
#
# SOURCE of the bytes in dl_block.h (tools/gen_block.py assembles it). Not linked directly.
# drag_learn.c copies the bytes into one page within +-2 GB of the game image, fills the
# data fields (game function / global addresses found by byte patterns), and redirects
# five game sites:
#   S  tactic-book list opens   "mov [rbp-44],rdi / mov ecx,5BC" (9 bytes)
#        -> turns the list into the game's own multi-select (checkbox) mode, with the
#           maximum = min(cap, command orders / cost, free tactic slots).
#           Replace mode (all 10 slots full, a tactic picked to delete) and limit <= 1
#           keep the original single selection.
#   X  right after the list closed with [결정]   "mov r8,[r13+4B8]" (7 bytes)
#        -> remembers every selected book (up to 16) from the game's result chain.
#   Y  [예] in the confirm dialog   "call LearnTactic" (5 bytes)
#        -> runs the original learn, then calls the same learn function for each other
#           selected book, each time checking orders >= cost and free slots < 10 first.
#   P  general list opens (learn window's "습득무장")  "mov [rsp+5C],rsi / mov ecx,32E" (10 bytes)
#        -> multi-select generals, max = min(cap, command orders / cost)
#   Q  right after the general list closed   "mov r14d,14000" (6 bytes)
#        -> remembers every selected general (up to 16); the window shows the first one
#   D  book list grey-out decision  "cmp dword [rip+lock],0" (7 bytes, the point where the
#        game is about to grey out a book for the shown general)
#        -> with several generals: keep the book selectable if ANY selected general may learn it
#   With several generals, Y learns every selected book for every selected general
#   (general by general), skipping pairs the game would not allow (already learned /
#   no matching formation) and books that ran out.
# Nothing else changes. All references inside the block are RIP-relative.
#
# Game structures used (2026-10 Steam build, every language exe):
#   learn window (r13 at S/X, rbx at Y):  +478 force  +488 person  +490 tactic to replace
#                                          +4B8 sub object
#   learn command (rcx of LearnTactic):    +0 person  +8 book(CGemData)  +10 replace  +18 force
#   book CGemData: +0 vtable, +52 tactic id
#   person: tactic slot i via f_slot(person, i) (byte person+130+i)
#   data root [p_db]: +774D0 + id*8 = tactic object (id > 200 -> entry 0); f_valid = object check
#   orders: f_orders(force) -> object, byte +14 = command orders; cost = int [p_cost] (=1)
#   general list (r?bx = window at P/Q): params at rsp+40: +18 mode, +1C max, +20 min;
#                result handle pointer at [rsp+28] after the list closed
#   eligibility (same as the game's grey-out in the book list): tactic t = table[book+52];
#     not learned: f_tid(t) != f_slot(person,i) for all i;
#     formation: some formation f in data root [+77428, +774D0) with valid(f), id=f_fid(f) <= 20,
#     bit id set in dword [t+6C], and (byte [f+7E]==2 ? f_pspec(person)==id : f_pform(person,id)!=0)
#   book count: idx = f_gidx(book), 1..30 -> word [force + 120 + idx*2]
#   list result: handle h = [[rsp+38]] at X; count = [[p_cnt]+h*8]; first node = [[p_head]+h*8];
#                node +0 = CGemData*, node +8 = next node; h < [p_npool]

    .intel_syntax noprefix
    .text
    .globl dl_block_start
dl_block_start:
# ---------------------------------------------------------------- data
d_on:       .long 1          # 1 = feature on
d_max:      .long 10         # book selection cap (2..16)
d_min:      .long 0          # minimum selection count passed to the list
d_limit:    .long 0          # last computed book limit
d_orders:   .long 0          # last seen command orders
d_filled:   .long 0          # last counted used tactic slots
d_seln:     .long 0          # remembered book count (cleared after Y)
d_opens:    .long 0          # diagnostics: book list opened
d_multi:    .long 0          #   book list opened in multi-select mode
d_xhits:    .long 0          #   book selections remembered
d_learned:  .long 0          #   extra learns done
d_stops:    .long 0          #   stopped by a guard (orders / a general's slots full)
d_fails:    .long 0          #   extra learn call returned 0
d_skips:    .long 0          #   pair skipped (not allowed / no book left)
d_pmax:     .long 10         # general selection cap (1 = 2안 off)
d_plimit:   .long 0          # last computed general limit
d_npers:    .long 0          # remembered general count (kept for 연속입력; a cancelled list keeps it)
d_popens:   .long 0          # diagnostics: general list opened
d_pmulti:   .long 0          #   general list opened in multi-select mode
d_qhits:    .long 0          #   general selections remembered
d_nb:       .long 0          # Y: books this time
d_np:       .long 0          # Y: generals this time
d_repl:     .long 0          # Y: first general was in replace mode
d_tmpi:     .long 0          # scratch loop index (S, D)
d_tmpmin:   .long 0          # scratch (S)
d_unhid:    .long 0          # diagnostics: books kept selectable for other generals (D)
d_fskip:    .long 0          # diagnostics: first general could not learn the first book (Y)
d_cmd:      .quad 0
d_first_ret:.quad 0
d_sel_person:.quad 0
f_valid:    .quad 0          # object validity check (obj) -> int
f_orders:   .quad 0          # force -> orders object
f_slot:     .quad 0          # (person, i) -> tactic id
f_learn:    .quad 0          # LearnTactic(cmd*) -> int
f_tid:      .quad 0          # tactic object -> tactic id
f_fid:      .quad 0          # formation object -> formation id
f_pspec:    .quad 0          # person -> special formation id
f_pform:    .quad 0          # (person, formation id) -> has it
f_gidx:     .quad 0          # book -> book index (1..30)
p_db:       .quad 0          # address of the data-root global
p_cost:     .quad 0          # address of int "orders per learn"
p_head:     .quad 0          # address of result chain head table pointer
p_cnt:      .quad 0          # address of result count table pointer
p_npool:    .quad 0          # address of result handle range (int)
r_s:        .quad 0          # S + 9
r_x:        .quad 0          # X + 7
r_y:        .quad 0          # Y + 5
r_p:        .quad 0          # P + 10
r_q:        .quad 0          # Q + 6
r_d:        .quad 0          # D + 7 (the original "je" after the cmp)
r_en:       .quad 0          # D's "selectable" target (mov esi,14000 / next book)
p_lock:     .quad 0          # address of the dword D compares with 0
f_fix:      .quad 0          # DLL: fix the learn window's texts (window) - 0 = off
f_r1:       .quad 0          # R1: the game's "show general" (75B700)
f_r2:       .quad 0          # R2: the game's "window (re)opens" (6BD7B0)
r_r1:       .quad 0          # R1 + 5
r_r2:       .quad 0          # R2 + 5
f_r3:       .quad 0          # R3: the game's "show the picked tactic" (7588B0)
r_r3:       .quad 0          # R3 + 5
    .balign 16
d_gems:     .fill 16, 8, 0   # remembered CGemData* (in selection order)
d_persons:  .fill 16, 8, 0   # remembered CPersonData* (in selection order)
    .balign 64

# ---------------------------------------------------------------- count_filled(person) -> eax
# number of used tactic slots, counted exactly like LearnTactic does. Missing data root -> 10.
count_filled:
    push    rbx
    push    rsi
    push    rdi
    sub     rsp, 0x20                       # 8 (ret) + 24 + 32 -> 16-aligned, home area reserved
    mov     rbx, rcx
    xor     esi, esi
    xor     edi, edi
cf_loop:
    cmp     esi, 10
    jae     cf_end
    mov     rcx, rbx
    mov     edx, esi
    call    qword ptr [rip + f_slot]
    mov     rcx, qword ptr [rip + p_db]
    mov     rcx, qword ptr [rcx]
    test    rcx, rcx
    je      cf_full
    cmp     eax, 0xC8
    ja      cf_zero
    mov     eax, eax
    mov     rcx, qword ptr [rcx + rax*8 + 0x774D0]
    jmp     cf_chk
cf_zero:
    mov     rcx, qword ptr [rcx + 0x774D0]
cf_chk:
    call    qword ptr [rip + f_valid]
    test    eax, eax
    je      cf_next
    inc     edi
cf_next:
    inc     esi
    jmp     cf_loop
cf_full:
    mov     edi, 10
cf_end:
    mov     eax, edi
    add     rsp, 0x20
    pop     rdi
    pop     rsi
    pop     rbx
    ret
    .balign 64

# ---------------------------------------------------------------- eligible(person, book) -> eax
# the game's own rule for greying a book out in the list (already learned / no formation)
eligible:
    push    rbx
    push    rsi
    push    rdi
    push    r12
    push    r13
    push    r14
    sub     rsp, 0x28                       # 8 + 48 + 40 -> aligned, home area reserved
    mov     rbx, rcx
    mov     r12, rdx
    mov     rcx, r12
    call    qword ptr [rip + f_valid]
    test    eax, eax
    je      el_no
    movzx   eax, byte ptr [r12 + 0x52]
    mov     rcx, qword ptr [rip + p_db]
    mov     rcx, qword ptr [rcx]
    test    rcx, rcx
    je      el_no
    cmp     eax, 0xC8
    ja      el_zero
    mov     r13, qword ptr [rcx + rax*8 + 0x774D0]
    jmp     el_tac
el_zero:
    mov     r13, qword ptr [rcx + 0x774D0]
el_tac:
    mov     rcx, r13
    call    qword ptr [rip + f_valid]
    test    eax, eax
    je      el_no
    mov     rcx, r13
    call    qword ptr [rip + f_tid]
    mov     esi, eax
    xor     edi, edi
el_learned:
    cmp     edi, 10
    jae     el_form
    mov     rcx, rbx
    mov     edx, edi
    call    qword ptr [rip + f_slot]
    cmp     eax, esi
    je      el_no
    inc     edi
    jmp     el_learned
el_form:
    mov     rax, qword ptr [rip + p_db]
    mov     rax, qword ptr [rax]
    lea     r14, [rax + 0x77428]
el_floop:
    mov     rax, qword ptr [rip + p_db]
    mov     rax, qword ptr [rax]
    add     rax, 0x774D0
    cmp     r14, rax
    jae     el_no
    mov     rcx, qword ptr [r14]
    test    rcx, rcx
    je      el_fnext
    call    qword ptr [rip + f_valid]
    test    eax, eax
    je      el_fnext
    mov     rcx, qword ptr [r14]
    call    qword ptr [rip + f_fid]
    mov     edi, eax
    cmp     eax, 0x14
    ja      el_fnext
    mov     ecx, dword ptr [r13 + 0x6C]
    bt      ecx, edi
    jnc     el_fnext
    mov     rax, qword ptr [r14]
    cmp     byte ptr [rax + 0x7E], 2
    jne     el_fother
    mov     rcx, rbx
    call    qword ptr [rip + f_pspec]
    cmp     eax, edi
    je      el_yes
    jmp     el_fnext
el_fother:
    mov     rcx, rbx
    mov     edx, edi
    call    qword ptr [rip + f_pform]
    test    eax, eax
    jne     el_yes
el_fnext:
    add     r14, 8
    jmp     el_floop
el_yes:
    mov     eax, 1
    jmp     el_ret
el_no:
    xor     eax, eax
el_ret:
    add     rsp, 0x28
    pop     r14
    pop     r13
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    ret
    .balign 64

# ---------------------------------------------------------------- read_chain
# in: rax = pointer to result handle, r10 = output buffer (16 qwords). out: ecx = count.
# uses rax rcx rdx r9 r10 r11 only, calls nothing.
read_chain:
    xor     ecx, ecx
    test    rax, rax
    je      rc_end
    mov     edx, dword ptr [rax]
    mov     r9, qword ptr [rip + p_npool]
    cmp     edx, dword ptr [r9]
    jae     rc_end
    mov     r9, qword ptr [rip + p_cnt]
    mov     r9, qword ptr [r9]
    test    r9, r9
    je      rc_end
    mov     r9, qword ptr [r9 + rdx*8]
    mov     r11, qword ptr [rip + p_head]
    mov     r11, qword ptr [r11]
    test    r11, r11
    je      rc_end
    mov     r11, qword ptr [r11 + rdx*8]
rc_loop:
    test    r11, r11
    je      rc_end
    cmp     rcx, r9
    jae     rc_end
    cmp     ecx, 16
    jae     rc_end
    mov     rax, qword ptr [r11]
    mov     qword ptr [r10 + rcx*8], rax
    inc     ecx
    mov     r11, qword ptr [r11 + 8]
    jmp     rc_loop
rc_end:
    ret
    .balign 64

# ---------------------------------------------------------------- orders_div -> eax
# eax = command orders / cost of the force in rcx (0 when unknown). Called from S and P.
orders_div:
    sub     rsp, 0x28                       # 8 + 40 -> aligned
    call    qword ptr [rip + f_orders]
    test    rax, rax
    je      od_zero
    movzx   eax, byte ptr [rax + 0x14]
    mov     dword ptr [rip + d_orders], eax
    mov     rcx, qword ptr [rip + p_cost]
    mov     ecx, dword ptr [rcx]
    test    ecx, ecx
    jle     od_ret
    xor     edx, edx
    div     ecx
    jmp     od_ret
od_zero:
    xor     eax, eax
od_ret:
    add     rsp, 0x28
    ret
    .balign 64

# ---------------------------------------------------------------- S: book list opens
# replaced: mov qword ptr [rbp-44],rdi (rdi = 1)  /  mov ecx,5BC
# list parameters: [rbp-48] mode (1 single, 2 multi), [rbp-44] max, [rbp-40] min
    .globl stub_s
stub_s:
    mov     qword ptr [rbp - 0x44], rdi
    inc     dword ptr [rip + d_opens]
    mov     dword ptr [rip + d_seln], 0
    cmp     dword ptr [rip + d_on], 0
    je      s_done
    mov     eax, dword ptr [rip + d_max]
    mov     dword ptr [rip + d_limit], eax
    mov     rcx, qword ptr [r13 + 0x490]        # replace mode -> original single select
    call    qword ptr [rip + f_valid]
    test    eax, eax
    jne     s_single
    mov     rcx, qword ptr [r13 + 0x478]
    call    orders_div
    cmp     dword ptr [rip + d_npers], 1        # several generals: orders shared by all of them
    jbe     s_ord
    mov     rcx, qword ptr [r13 + 0x488]
    cmp     rcx, qword ptr [rip + d_persons]
    jne     s_ord
    xor     edx, edx
    div     dword ptr [rip + d_npers]
s_ord:
    cmp     eax, dword ptr [rip + d_limit]
    jae     s_cnt
    mov     dword ptr [rip + d_limit], eax
s_cnt:
    mov     rcx, qword ptr [r13 + 0x488]
    call    count_filled
    mov     dword ptr [rip + d_filled], eax
    cmp     dword ptr [rip + d_npers], 1        # several generals: the one with most free slots
    jbe     s_have
    mov     rcx, qword ptr [r13 + 0x488]
    cmp     rcx, qword ptr [rip + d_persons]
    jne     s_have
    mov     dword ptr [rip + d_tmpmin], eax
    mov     dword ptr [rip + d_tmpi], 1
s_ploop:
    mov     eax, dword ptr [rip + d_tmpi]
    cmp     eax, dword ptr [rip + d_npers]
    jae     s_pdone
    lea     rcx, [rip + d_persons]
    mov     rcx, qword ptr [rcx + rax*8]
    test    rcx, rcx
    je      s_pnext
    call    count_filled
    cmp     eax, dword ptr [rip + d_tmpmin]
    jae     s_pnext
    mov     dword ptr [rip + d_tmpmin], eax
s_pnext:
    inc     dword ptr [rip + d_tmpi]
    jmp     s_ploop
s_pdone:
    mov     eax, dword ptr [rip + d_tmpmin]
    mov     dword ptr [rip + d_filled], eax
s_have:
    mov     ecx, 10
    sub     ecx, eax
    jns     s_free
    xor     ecx, ecx
s_free:
    cmp     ecx, dword ptr [rip + d_limit]
    jae     s_chk
    mov     dword ptr [rip + d_limit], ecx
s_chk:
    cmp     dword ptr [rip + d_limit], 1
    jbe     s_single
    mov     dword ptr [rbp - 0x48], 2
    mov     eax, dword ptr [rip + d_limit]
    mov     dword ptr [rbp - 0x44], eax
    mov     eax, dword ptr [rip + d_min]
    mov     dword ptr [rbp - 0x40], eax
    inc     dword ptr [rip + d_multi]
    jmp     s_done
s_single:
    mov     dword ptr [rip + d_limit], 1
s_done:
    mov     ecx, 0x5BC
    jmp     qword ptr [rip + r_s]
    .balign 64

# ---------------------------------------------------------------- X: book list closed with [결정]
# replaced: mov r8,[r13+4B8]   (rax rcx rdx r9 r10 r11 are dead here)
    .globl stub_x
stub_x:
    mov     rax, qword ptr [rsp + 0x38]
    lea     r10, [rip + d_gems]
    call    read_chain                          # pushes a return address below the game's rsp only
    mov     dword ptr [rip + d_seln], ecx
    mov     rax, qword ptr [r13 + 0x488]
    mov     qword ptr [rip + d_sel_person], rax
    inc     dword ptr [rip + d_xhits]
    mov     r8, qword ptr [r13 + 0x4B8]
    jmp     qword ptr [rip + r_x]
    .balign 64

# ---------------------------------------------------------------- P: general list opens
# replaced: mov qword ptr [rsp+5C],rsi (rsi = 1: max 1, min 0)  /  mov ecx,32E    rbx = window
# list parameters at rsp+40: [rsp+58] mode, [rsp+5C] max, [rsp+60] min
    .globl stub_p
stub_p:
    mov     qword ptr [rsp + 0x5C], rsi
    inc     dword ptr [rip + d_popens]
    mov     dword ptr [rip + d_plimit], 1
    cmp     dword ptr [rip + d_on], 0
    je      p_done
    cmp     dword ptr [rip + d_pmax], 1
    jbe     p_done
    mov     rcx, qword ptr [rbx + 0x478]
    call    orders_div
    cmp     eax, dword ptr [rip + d_pmax]
    jbe     p_lim
    mov     eax, dword ptr [rip + d_pmax]
p_lim:
    mov     dword ptr [rip + d_plimit], eax
    cmp     eax, 1
    jbe     p_done
    mov     dword ptr [rsp + 0x58], 2
    mov     dword ptr [rsp + 0x5C], eax
    mov     dword ptr [rsp + 0x60], 0
    inc     dword ptr [rip + d_pmulti]
p_done:
    mov     ecx, 0x32E
    jmp     qword ptr [rip + r_p]
    .balign 64

# ---------------------------------------------------------------- Q: general list closed
# replaced: mov r14d,14000   (rax rcx rdx r9 r10 r11 are dead here; [rsp+28] = result handle ptr)
    .globl stub_q
stub_q:
    mov     rax, qword ptr [rsp + 0x28]
    sub     rsp, 0x80                           # keep the old list while reading the new one
    lea     r10, [rsp]
    call    read_chain
    test    ecx, ecx
    je      q_keep                              # cancelled: keep the previous selection
    mov     dword ptr [rip + d_npers], ecx
    lea     r9, [rip + d_persons]
q_copy:
    dec     ecx
    mov     rax, qword ptr [rsp + rcx*8]
    mov     qword ptr [r9 + rcx*8], rax
    test    ecx, ecx
    jne     q_copy
    inc     dword ptr [rip + d_qhits]
q_keep:
    add     rsp, 0x80
    mov     r14d, 0x14000
    jmp     qword ptr [rip + r_q]
    .balign 64

# ---------------------------------------------------------------- D: book grey-out decision
# replaced: cmp dword ptr [rip+lock],0  (7 bytes; then the game's "je" at D+7)
# r12 = book, r13 = window. rax rcx rdx r8-r11 are free here.
    .globl stub_d
stub_d:
    cmp     dword ptr [rip + d_on], 0
    je      d_orig
    cmp     dword ptr [rip + d_npers], 1
    jbe     d_orig
    mov     rax, qword ptr [r13 + 0x488]
    cmp     rax, qword ptr [rip + d_persons]
    jne     d_orig
    mov     dword ptr [rip + d_tmpi], 1
d_loop:
    mov     eax, dword ptr [rip + d_tmpi]
    cmp     eax, dword ptr [rip + d_npers]
    jae     d_orig
    lea     rcx, [rip + d_persons]
    mov     rcx, qword ptr [rcx + rax*8]
    test    rcx, rcx
    je      d_next
    mov     rdx, r12
    call    eligible
    test    eax, eax
    jne     d_keep
d_next:
    inc     dword ptr [rip + d_tmpi]
    jmp     d_loop
d_keep:
    inc     dword ptr [rip + d_unhid]
    jmp     qword ptr [rip + r_en]
d_orig:
    mov     rax, qword ptr [rip + p_lock]
    cmp     dword ptr [rax], 0
    jmp     qword ptr [rip + r_d]
    .balign 64

# ---------------------------------------------------------------- Y: [예] -> learn
# replaced: call LearnTactic   (rcx = learn command on the caller's stack)
# Learns (general x book) for every remembered general and book after the game's own learn.
    .globl stub_y
stub_y:
    push    rbx
    push    rsi
    push    rdi
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, 0x28                           # 56 + 40 -> aligned; [rsp+0..20) home, [rsp+20] local
    mov     qword ptr [rip + d_cmd], rcx
    mov     rsi, rcx
    mov     rbx, qword ptr [rsi]                # first general (the one shown in the window)
    mov     r12, qword ptr [rsi + 8]            # first book (the one picked in the list)
    mov     r15, qword ptr [rsi + 0x10]         # replace target
    mov     dword ptr [rip + d_nb], 1
    mov     dword ptr [rip + d_np], 1
    cmp     dword ptr [rip + d_on], 0
    je      y_first
    test    rbx, rbx
    je      y_first
    test    r12, r12
    je      y_first
    cmp     dword ptr [rip + d_seln], 1
    jbe     y_np
    cmp     r12, qword ptr [rip + d_gems]       # books: must be this list's selection
    jne     y_np
    cmp     rbx, qword ptr [rip + d_sel_person]
    jne     y_np
    mov     eax, dword ptr [rip + d_seln]
    mov     dword ptr [rip + d_nb], eax
y_np:
    cmp     dword ptr [rip + d_npers], 1
    jbe     y_first
    cmp     rbx, qword ptr [rip + d_persons]    # generals: the window shows the first selected
    jne     y_first
    mov     eax, dword ptr [rip + d_npers]
    mov     dword ptr [rip + d_np], eax
    mov     rcx, rbx                            # the list may now offer books only the others can learn
    mov     rdx, r12
    call    eligible
    test    eax, eax
    jne     y_first
    inc     dword ptr [rip + d_fskip]           # first general cannot learn it: skip only him
    mov     qword ptr [rip + d_first_ret], 1
    jmp     y_after
y_first:
    mov     rcx, rsi
    call    qword ptr [rip + f_learn]           # the game's own learn
    mov     qword ptr [rip + d_first_ret], rax
    test    eax, eax
    je      y_end
y_after:
    cmp     dword ptr [rip + d_nb], 1
    jne     y_go
    cmp     dword ptr [rip + d_np], 1
    je      y_end
y_go:
    mov     rcx, r15
    call    qword ptr [rip + f_valid]
    mov     dword ptr [rip + d_repl], eax
    xor     r14d, r14d                          # general index
gp_loop:
    cmp     r14d, dword ptr [rip + d_np]
    jae     y_end
    test    r14d, r14d
    jne     gp_other
    mov     qword ptr [rsp + 0x20], rbx
    cmp     dword ptr [rip + d_repl], 0         # replace mode: no extra book for the first general
    jne     gp_next
    jmp     gp_books
gp_other:
    lea     rax, [rip + d_persons]
    mov     rax, qword ptr [rax + r14*8]
    test    rax, rax
    je      gp_next
    cmp     rax, rbx
    je      gp_next
    mov     qword ptr [rsp + 0x20], rax
gp_books:
    xor     edi, edi                            # book index
bk_loop:
    cmp     edi, dword ptr [rip + d_nb]
    jae     gp_next
    test    r14d, r14d
    jne     bk_pick
    test    edi, edi
    je      bk_next                             # (first general, first book) = the game's own learn
bk_pick:
    mov     r13, r12
    test    edi, edi
    je      bk_have
    lea     rax, [rip + d_gems]
    mov     r13, qword ptr [rax + rdi*8]
    test    r13, r13
    je      bk_next
    cmp     r13, r12
    je      bk_next
    mov     rax, qword ptr [r13]                # same class as the first book (vtable)
    cmp     rax, qword ptr [r12]
    jne     bk_next
bk_have:
    mov     rcx, qword ptr [rsp + 0x20]         # allowed for this general?
    mov     rdx, r13
    call    eligible
    test    eax, eax
    je      bk_skip
    mov     rcx, r13                            # a book of this kind left?
    call    qword ptr [rip + f_gidx]
    lea     edx, [rax - 1]
    cmp     edx, 0x1D
    ja      bk_skip
    movsxd  rax, eax
    mov     rcx, qword ptr [rsi + 0x18]
    movzx   eax, word ptr [rcx + rax*2 + 0x120]
    test    eax, eax
    je      bk_skip
    mov     rcx, qword ptr [rsi + 0x18]         # orders >= cost, else stop everything
    call    qword ptr [rip + f_orders]
    test    rax, rax
    je      y_stop
    movzx   eax, byte ptr [rax + 0x14]
    mov     dword ptr [rip + d_orders], eax
    mov     rcx, qword ptr [rip + p_cost]
    cmp     eax, dword ptr [rcx]
    jl      y_stop
    mov     rcx, qword ptr [rsp + 0x20]         # a free slot, else next general
    call    count_filled
    mov     dword ptr [rip + d_filled], eax
    cmp     eax, 10
    jae     gp_full
    mov     rax, qword ptr [rsp + 0x20]
    mov     qword ptr [rsi], rax
    mov     qword ptr [rsi + 8], r13
    mov     qword ptr [rsi + 0x10], 0
    mov     rcx, rsi
    call    qword ptr [rip + f_learn]
    mov     qword ptr [rsi], rbx                # restore the command after every call
    mov     qword ptr [rsi + 8], r12
    mov     qword ptr [rsi + 0x10], r15
    test    eax, eax
    je      bk_fail
    inc     dword ptr [rip + d_learned]
    jmp     bk_next
bk_fail:
    inc     dword ptr [rip + d_fails]
    jmp     bk_next
bk_skip:
    inc     dword ptr [rip + d_skips]
bk_next:
    inc     edi
    jmp     bk_loop
gp_full:
    inc     dword ptr [rip + d_stops]
gp_next:
    inc     r14d
    jmp     gp_loop
y_stop:
    inc     dword ptr [rip + d_stops]
y_end:
    mov     qword ptr [rsi], rbx
    mov     qword ptr [rsi + 8], r12
    mov     qword ptr [rsi + 0x10], r15
    add     rsp, 0x28
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    mov     dword ptr [rip + d_seln], 0
    mov     rax, qword ptr [rip + d_first_ret]
    jmp     qword ptr [rip + r_y]
    .balign 64
# ---------------------------------------------------------------- R1 / R2: after the game redraws
# R1 replaced: call ShowGeneral(sub, general)   rbx = learn window (after the general list)
# R2 replaced: call Reopen(sub)                 rdi = learn window (연속입력 reopens the window)
# R3 replaced: call ShowTactic(sub, tactic, n, 1) rbx = learn window (after the book list)
# The game's own call runs first, then the DLL rewrites the general name ("우금 외 2명") and the
# orders preview. rsp is 16-aligned at both sites; the callee's home area is the caller's, as before.
    .globl stub_r1
stub_r1:
    call    qword ptr [rip + f_r1]
    cmp     dword ptr [rip + d_on], 0
    je      r1_done
    mov     rax, qword ptr [rip + f_fix]
    test    rax, rax
    je      r1_done
    mov     rcx, rbx
    call    rax
r1_done:
    jmp     qword ptr [rip + r_r1]
    .balign 64
    .globl stub_r2
stub_r2:
    call    qword ptr [rip + f_r2]
    cmp     dword ptr [rip + d_on], 0
    je      r2_done
    mov     rax, qword ptr [rip + f_fix]
    test    rax, rax
    je      r2_done
    mov     rcx, rdi
    call    rax
r2_done:
    jmp     qword ptr [rip + r_r2]
    .balign 64
    .globl stub_r3
stub_r3:
    call    qword ptr [rip + f_r3]
    cmp     dword ptr [rip + d_on], 0
    je      r3_done
    mov     rax, qword ptr [rip + f_fix]
    test    rax, rax
    je      r3_done
    mov     rcx, rbx
    call    rax
r3_done:
    jmp     qword ptr [rip + r_r3]
    .balign 64
    .globl dl_block_end
dl_block_end:
