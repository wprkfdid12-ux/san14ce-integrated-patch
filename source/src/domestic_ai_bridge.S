// 22번 AI 명령서 99장 (11차 P212) — 게임의 명령서 채우기 자리 exe+0x9fa17 (7바이트: cmp eax,r15d / cmovg eax,r15d) 뒤 'mov [rbp+0x14],al' 바로 앞.
//  게임 규칙(0x9f9d5~0x9fa1e, 세력마다): 새 값 = min(지금+1, 세력 최대(0x2db910 = 군주 능력 기반), 200).
//  옛 DomesticCE 는 주기적으로 99 를 써 넣었지만 이 자리가 매번 세력 최대(보통 3~15)로 깎아 AI 가 행동할 때는 99 가 아니었다(달 기록 ai_cmd99=0 으로 확인).
//  여기서 플레이어 세력이 아니면 새 값을 99 로 — 채우는 순간에 넣으므로 주기적으로 읽거나 쓰지 않는다.
//  16차(P220): rbp 는 세력이 아니라 군단(CDistrictData) 객체다 — 도시.vf[0x80] = 군단표 db+0xe0d0[도시+0x30]. 군단+0x10 = 세력 번호.
//   플레이어 세력(world+0x3a)의 군단이거나 세력 번호 0 이면 게임 값 그대로. (15차까지는 세력 객체 주소와 비교해 플레이어 군단도 99 가 될 수 있었음)
//  rbp = 군단 객체, eax = 새 값, r15d = 200. rcx·rdx 는 지켜 준다(플래그는 뒤 명령이 안 씀).
.text
.globl DaiOrdBridge
DaiOrdBridge:
 cmp %r15d,%eax
 cmovg %r15d,%eax
 cmpl $0,DaiOrdOn(%rip)
 je 9f
 push %rcx
 push %rdx
 mov DaiGame(%rip),%rcx
 mov 0x2780f60(%rcx),%rcx
 test %rcx,%rcx
 je 8f
 mov 0x871e8(%rcx),%rdx
 test %rdx,%rdx
 je 8f
 movzbl 0x3a(%rdx),%edx
 movzbl 0x10(%rbp),%ecx
 test %ecx,%ecx
 je 8f
 cmp %edx,%ecx
 je 8f
 mov $99,%eax
 lock incl DaiOrdHits(%rip)
8:
 pop %rdx
 pop %rcx
9:
 jmp *DaiOrdRet(%rip)

// 21차(사용자 11-11): AI 명승 투자 금 조건 — 게임 AI 의 명승 투자 실행(행동 노드 CSceneryInvestSetNode 의 실행 함수 exe+0x3e890,
//  vtable 0x134b260 칸 16 에서만 불림 = AI 전용) 안의 금 비교 자리 exe+0x3e9c6 (5바이트: cmp eax,r14d / jl +6).
//  게임 규칙: 도시 금(eax, 도시.vf[0x90]) < 비용(r14d) 이면 실패(0x3e9d1). 여기서 플레이어 세력이 아닌 군단이면
//  '투자하고 남는 금 ≥ 비용×배수 이고 ≥ 최소' 일 때만 통과. rsi = 군단(+0x10 세력 번호), rdi = 도시. 통과 → 0x3e9cb, 실패 → 0x3e9d1.
//  DaiScOn=0 이면 게임 규칙 그대로. rcx·rdx 는 지켜 줌(뒤 명령이 다시 씀). 매 투자 시도 때만 — 주기적으로 읽지 않음.
.globl DaiScenicBridge
DaiScenicBridge:
 cmp %r14d,%eax
 jl 7f
 cmpl $0,DaiScOn(%rip)
 je 6f
 push %rcx
 push %rdx
 mov DaiGame(%rip),%rcx
 mov 0x2780f60(%rcx),%rcx
 test %rcx,%rcx
 je 5f
 mov 0x871e8(%rcx),%rdx
 test %rdx,%rdx
 je 5f
 movzbl 0x3a(%rdx),%edx
 movzbl 0x10(%rsi),%ecx
 test %ecx,%ecx
 je 5f
 cmp %edx,%ecx
 je 5f
 // 남는 금 = eax - r14d (64비트로 계산), 필요 = max(비용×배수, 최소)
 movslq %eax,%rcx
 movslq %r14d,%rdx
 sub %rdx,%rcx
 imul DaiScMult(%rip),%rdx
 cmp DaiScFloor(%rip),%rdx
 jge 4f
 mov DaiScFloor(%rip),%rdx
4:
 cmp %rdx,%rcx
 jl 3f
 lock incl DaiScPassed(%rip)
 jmp 5f
3:
 lock incl DaiScBlocked(%rip)
 pop %rdx
 pop %rcx
7:
 jmp *DaiScFail(%rip)
5:
 pop %rdx
 pop %rcx
6:
 jmp *DaiScPass(%rip)
