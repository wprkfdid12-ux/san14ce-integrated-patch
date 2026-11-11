#include "bridge_macros.inc"
.text
.globl MeritReadBridge
MeritReadBridge:
 movw %r15w,0x20(%rsp)
 SAVE_VOLATILE
 lea 0x1c2(%r12),%rcx
 lea 0x120(%rsp),%rdx
 call MeritDecode
 RESTORE_VOLATILE
 jmp *MeritReadReturn(%rip)
.globl MeritWriteBridge
MeritWriteBridge:
 SAVE_VOLATILE
 lea 0x120(%rsp),%rcx
 lea 0x1c2(%r12),%rdx
 call MeritEncode
 RESTORE_VOLATILE
 jmp *MeritWriteReturn(%rip)
