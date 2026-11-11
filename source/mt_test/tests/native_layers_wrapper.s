.intel_syntax noprefix
.text
.globl native_layers_wrapper
# Enter a preserved internal CE UI loop at its native stack shape.
# RCX=row, RDX=6 integer statuses, R8=loop address, R9=packed selection.
# Native calls use MS ABI; the patched fixture epilogue restores this frame.
native_layers_wrapper:
 push rbx
 push rbp
 push rsi
 push rdi
 push r14
 sub rsp, 128
 mov rbp, rcx
 mov r10, r8
 mov rax, [rdx]
 mov [rsp+0x48], rax
 mov rax, [rdx+8]
 mov [rsp+0x50], rax
 mov rax, [rdx+16]
 mov [rsp+0x58], rax
 mov [rsp+0x40], r9d
 xor esi, esi
 xor ebx, ebx
 jmp r10
