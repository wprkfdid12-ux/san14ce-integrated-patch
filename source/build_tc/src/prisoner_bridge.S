.text
# 0x2e0000 원본 첫 명령(mov [rsp+0x20], r9d; 5바이트)을 다시 실행하고 0x2e0005로 이어 간다.
# CaptureHook 이 일반 호출로 부르므로 [rsp]=복귀 주소, [rsp+0x28]/[rsp+0x30]=5·6번째 인자 — 원본 진입과 같은 모양.
.globl Orig2e0000
Orig2e0000:
 mov %r9d,0x20(%rsp)
 jmp *Cont2e0000(%rip)
