#include "bridge_macros.inc"
.text
.globl RemoteBridge
RemoteBridge:
 cmp %eax,%ebx
 je .Lremote_yes
 SAVE_VOLATILE
 call RemoteAllowed
 test %eax,%eax
 je .Lremote_no_restore
 RESTORE_VOLATILE
.Lremote_yes:
 jmp *RemoteYes(%rip)
.Lremote_no_restore:
 RESTORE_VOLATILE
 jmp *RemoteNo(%rip)
.globl DeathBridge
DeathBridge:
 test %ebx,%ebx
 jne .Ldeath_yes
 SAVE_VOLATILE
 mov %r13,%rcx
 call DeathAllowed
 test %eax,%eax
 je .Ldeath_no_restore
 RESTORE_VOLATILE
.Ldeath_yes:
 jmp *DeathYes(%rip)
.Ldeath_no_restore:
 RESTORE_VOLATILE
 jmp *DeathNo(%rip)
.globl NamesVersionBridge
NamesVersionBridge:
 SAVE_VOLATILE
 call NamesVersion
 RESTORE_VOLATILE
 jmp *NamesVersionReturn(%rip)
.globl NamesInputBridge
NamesInputBridge:
 SAVE_VOLATILE
 mov %rsi,%rcx
 mov %r8d,%edx
 call NamesInput
 test %rax,%rax
 jz .Lname_input_native
 mov 8(%rax),%r14d
 mov (%rax),%r15
 jmp .Lname_input_restore
.Lname_input_native:
 mov $3,%r14d
 lea 0x1a0(%rsi),%r15
 cmpl $0,0x98(%rsp)
 jz .Lname_input_restore
 lea 0x1a6(%rsi),%r15
.Lname_input_restore:
 RESTORE_VOLATILE
 jmp *NamesInputReturn(%rip)
// R12: the native recruit order re-checks every day that the ronin's city is owned by
// the ordering force (0x1e83ef / 0x1e87b8) and cancels on day 1 otherwise. With 27번's
// remote-ronin list on, that ownership test is waived; every other native check stays.
.macro EMPLOY_BRIDGE name, cont
.globl \name
\name:
 call *EmployCityFn(%rip)
 movzbl 0x71(%rsi),%ecx
 cmp %cl,0x14(%rax)
 je 1f
 mov %rsi,%rcx
 mov %rdi,%rdx
 sub $0x20,%rsp
 call EmployRemoteAllowed
 add $0x20,%rsp
 test %eax,%eax
 jz 2f
1:
 xor %eax,%eax
 jmp *\cont(%rip)
2:
 or $1,%eax
 jmp *\cont(%rip)
.endm
EMPLOY_BRIDGE EmployBridgeA, EmployContA
EMPLOY_BRIDGE EmployBridgeB, EmployContB
// R13: original entry of the native travel-days routine (0x230a40); its first 15 bytes
// (three register saves into the caller's shadow space) are replaced by the hook.
.globl Orig230a40
Orig230a40:
 mov %rbx,0x8(%rsp)
 mov %rbp,0x10(%rsp)
 mov %rsi,0x18(%rsp)
 jmp *TravelDaysCont(%rip)
