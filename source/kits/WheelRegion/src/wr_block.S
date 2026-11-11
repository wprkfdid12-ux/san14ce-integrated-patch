# RTK14 WheelRegion - machine-code block (x64, Windows ABI)
#
# This file is the SOURCE of the bytes in wr_block.h. It is not linked directly:
# tools/gen_block.py assembles it and writes the bytes + offsets into wr_block.h,
# so host mods can use the C files with any compiler (clang, MinGW, MSVC).
#
# At run time wheel_region.c copies these bytes into one page allocated within
# +-2 GB of the game image, fills the data fields, and redirects three game sites:
#   A  hover-tile setter      (whole function replaced, original semantics kept)
#   B  info-target-tile store (mov [rbp+2F4B0],r14d, 7 bytes)
#   K  garrison-unit getter   (movsx eax,word ptr [rcx+1C] / ret, 5 bytes)
# Everything is a no-op unless d_flag != 0 (set by the worker for ~0.6 s after a
# wheel click on an empty tile). All references are RIP-relative inside the block.

    .intel_syntax noprefix
    .text
    .globl wr_block_start
wr_block_start:
# ---------------------------------------------------------------- data (0x00..0x7F)
d_flag:     .long 0          # +0x00  1 while a wheel click is being redirected
d_kfilter:  .long 0          # +0x04  1 = K only answers callers inside the game .text
d_ahits:    .long 0          # +0x08  diagnostics
d_bhits:    .long 0          # +0x0C
d_khits:    .long 0          # +0x10
d_kcalls:   .long 0          # +0x14
d_rsv0:     .quad 0          # +0x18
d_table:    .quad 0          # +0x20  u16[48400] tile -> target tile (0xFFFF = none)
d_pmap:     .quad 0          # +0x28  address of the game global that holds the map object
d_pdb:      .quad 0          # +0x30  address of the game global that holds the data root
d_textlo:   .quad 0          # +0x38  game .text start (absolute)
d_texthi:   .quad 0          # +0x40  game .text end (absolute)
d_retb:     .quad 0          # +0x48  B site + 7 (absolute)
    .balign 128

# ---------------------------------------------------------------- A: hover tile setter
# original:  if (r8d == [rdx+4]) return 0;  [rdx] = r9d; [rdx+4] = r8d; return 1;
# while d_flag: r9d / r8d are first mapped through the table (tile -> region target).
    .globl stub_a
stub_a:
    cmp     dword ptr [rip + d_flag], 0
    je      a_orig
    mov     rax, qword ptr [rip + d_table]
    test    rax, rax
    je      a_orig
    inc     dword ptr [rip + d_ahits]
    mov     r11d, r9d
    cmp     r11d, 48400
    jae     a_r8
    movzx   r10d, word ptr [rax + r11*2]
    cmp     r10d, 0xFFFF
    je      a_r8
    mov     r9d, r10d
a_r8:
    mov     r11d, r8d
    cmp     r11d, 48400
    jae     a_orig
    movzx   r10d, word ptr [rax + r11*2]
    cmp     r10d, 0xFFFF
    je      a_orig
    mov     r8d, r10d
a_orig:
    cmp     r8d, dword ptr [rdx + 4]
    jne     a_store
    xor     eax, eax
    ret
a_store:
    mov     dword ptr [rdx], r9d
    mov     eax, 1
    mov     dword ptr [rdx + 4], r8d
    ret
    .balign 128

# ---------------------------------------------------------------- B: info target tile
# replaced instruction:  mov dword ptr [rbp+2F4B0], r14d   (then jump back to B+7)
# while d_flag: store table[r14d] instead (if it has a target).
    .globl stub_b
stub_b:
    cmp     dword ptr [rip + d_flag], 0
    je      b_keep2
    push    rax
    push    rcx
    mov     eax, r14d
    cmp     eax, 48400
    jae     b_keep
    mov     rcx, qword ptr [rip + d_table]
    test    rcx, rcx
    je      b_keep
    movzx   eax, word ptr [rcx + rax*2]
    cmp     eax, 0xFFFF
    jne     b_store
    mov     eax, r14d
b_store:
    inc     dword ptr [rip + d_bhits]
    mov     dword ptr [rbp + 0x2F4B0], eax
    pop     rcx
    pop     rax
    jmp     qword ptr [rip + d_retb]
b_keep:
    pop     rcx
    pop     rax
b_keep2:
    mov     dword ptr [rbp + 0x2F4B0], r14d
    jmp     qword ptr [rip + d_retb]
    .balign 128

# ---------------------------------------------------------------- K: garrison unit getter
# original:  return (int)*(short*)(rcx+1C);     rcx = hex building object
# while d_flag: return 0 (no garrison) for the building of the redirected target tile
#   ( [[pdb] + E270 + [ [pmap] + 2F4B0 ]*8] == rcx ), optionally only for game callers.
    .globl stub_k
stub_k:
    movsx   eax, word ptr [rcx + 0x1C]
    cmp     dword ptr [rip + d_flag], 0
    je      k_done
    inc     dword ptr [rip + d_kcalls]
    cmp     dword ptr [rip + d_kfilter], 0
    je      k_any
    mov     r10, qword ptr [rsp]
    cmp     r10, qword ptr [rip + d_textlo]
    jb      k_done
    cmp     r10, qword ptr [rip + d_texthi]
    jae     k_done
k_any:
    mov     r10, qword ptr [rip + d_pmap]
    test    r10, r10
    je      k_done
    mov     r10, qword ptr [r10]
    test    r10, r10
    je      k_done
    movsxd  r10, dword ptr [r10 + 0x2F4B0]
    cmp     r10d, 0xBD0F
    ja      k_done
    mov     r11, qword ptr [rip + d_pdb]
    test    r11, r11
    je      k_done
    mov     r11, qword ptr [r11]
    test    r11, r11
    je      k_done
    cmp     rcx, qword ptr [r11 + r10*8 + 0xE270]
    jne     k_done
    xor     eax, eax
    inc     dword ptr [rip + d_khits]
k_done:
    ret
    .balign 128
    .globl wr_block_end
wr_block_end:
