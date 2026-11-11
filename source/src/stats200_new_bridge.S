.text
// 0x231afb: 원래 `je 0x231b42 ; nop3`. 플래그는 바로 앞 `cmp ecx,0x7b` 의 것(릴레이 jmp [rip] 는 플래그를 안 바꿈).
//  ecx!=0x7b           → 0x231b00 (원래 je 안 탐과 같음)
//  ecx==0x7b, esi<100  → 0x231b42 (원래대로 esi=100)
//  ecx==0x7b, esi>=100 → 0x231b00 (새 동작: 상한을 100 으로 끌어내리지 않음) — 이 경우만 센다. 0x231b00 첫 명령들(mov,mov,cmp)이 플래그를 새로 만들므로 lock inc 의 플래그 변화는 무해.
.globl XpBridge
XpBridge:
 jne 1f
 cmp $0x64,%esi
 jl 2f
 lock incl XpHitsRaw(%rip)
1: jmp *XpCont(%rip)
2: jmp *XpCap(%rip)
// 0x2bc30a(경험치 성장 함수의 레벨업 구간 시작)에서 jmp 로 옴. rsp 는 게임 함수 것 그대로(16 정렬, 아래 0x20 은 그 함수의 호출용 빈칸).
//  rsi=인물, rdi=능력 번호, r13=처음 능력, r15d=0. 구간이 하던 r14 복구(0x2bc313 mov r14,[rsp+0x48])를 여기서 하고,
//  레벨업을 Stats200Grow(인물, 능력 번호)로 한 뒤 0x2bc3c8(능력이 바뀌었는지 보고 알림)로 돌아간다. rsi·rdi·r13·r15 는 호출 규약상 보존.
.globl GrowBridge
GrowBridge:
 mov 0x48(%rsp),%r14
 mov %rsi,%rcx
 mov %edi,%edx
 call Stats200Grow
 jmp *GrowRet(%rip)
.data
.globl XpHitsRaw
XpHitsRaw: .long 0
