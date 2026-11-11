# R42 B4(26번 처단 버튼 회색) · 22번(DomesticCE 지역별 건너뛰기) 메모 — 포크 작업

## B4 SAN14_ExecuteLock.dll (새 모듈, src/execute_lock.cpp + execute_lock_bridge.S, 시험 test/test_execute_lock.py 15 PASS)
- 게임 포로 처우 창 = CTreatPrisonerLayout(vt 0x142def0). 버튼 4개 = 창+0x380+i*8 (만드는 고리 0x66ae00: 번호 2·4·6·8, 글자 0x12·0x3c·0x3d·0x3e).
  글자 0x3c = 처단: 군주 포로 창 CTreatLordPrisonerDlg(vt 0x145d848, 0x80c9f0)에서 처단불가 비트9면 끄는 버튼 글자가 0x3c → 처단 버튼 = 창+0x388.
- 버튼 켜고 끄기 = 0x66fd80 한 곳(선택 포로 중 +0x15e 비트12 없는 포로 있으면 처단 켬). 부르는 곳 0x66b202(만들 때)·0x66ed11(vt+0x238)·0x66fb8e(0x66fa90 갱신).
- 7번 포로 회유 창 = IC 가 같은 CTreatPrisonerLayout 을 vtable 사본(IC 0x180106130)으로 띄움(0x180047b0c~, 0x18003b86b 에서 [창]=사본).
  IC 는 vt+0x238 을 0x1800678f0(게임 0x66ed00 → IC 0x18006acd0)로 바꿈. 0x18006acd0 은 [IC+0x105058]==2 일 때 [IC+0x105050] 창 버튼 4개를 게임 0x141e0(=vt+0xd0)로 다시 켜고 끔 → 처단 다시 켜질 수 있음.
- 군주 포로 창 처단 버튼은 NoExecute Mode=1 S5(0x80cd67 jae→NOP2)가 이미 끔. 손 안 댐.
- 고친 자리: G1 게임 0x66ff4e(0x66fd80 유일한 끝, `48 8b 5c 24 60` 5바이트, r13=창) → 처단불가 켜짐이면 창+0x388 버튼 vt+0xd0(btn,0).
  G2 IC 0x18006acd0 입구 8바이트 `4c 8b dc 55 49 8d 6b a1` 감싸기(ModuleTransaction.detour) — 원래 실행 뒤 같은 처리. known_common(IC 해시 5종 중 하나)일 때만, 지문 0x180067903 `48 83 c4 28 e9`·0x18006aced `83 3d .. 02`.
- 켜짐 판단: SAN14_NoExecute.dll 올라옴 + SAN14_NoExecute.ini Mode=1 + ExtBridge SAN14_ExtState(L"NoExecute")==1 (ExtBridge 없으면 ICGuard F 와 같은 ini 규칙). 2초 캐시.
  끄기만 하므로 처단불가를 끄면 다음 갱신 때 게임 원래 규칙(비트12)대로 다시 켜짐.
- 겹침: AI v13.3 해시 48곳·IC 전역 호환 표(0x1800dd240 174칸, 0x1800de800 표)·우리 R41f 훅 기록(hooks_r41f.pkl) 어느 것도 0x66fd80~0x66ff6d 안 덮음. AI captive_dialog 는 포획 직후 장면(prepare_traits9.py 계산 자리)이라 별개.
- 실게임 확인 줄: 적용확인.log `R39 26 SAN14_ExecuteLock.dll ... OK 2/2`, `R39C 26 SAN14_ExecuteLock.dll execute_button_greyed N`(처음·50번마다), IC 창은 `execute_button_greyed_ic`.
  runtime.log `ExecuteLock R42: prisoner-treatment window greys ...`, `ExecuteLock R42: 7 captive-persuasion window also greys ...`.
- 위험: 처단 버튼 위치를 글자 번호(0x3c)로 추정 — 군주 포로 창과 같은 글자 표(0x30cf80)라 근거 강함. 실게임에서 '처단' 아닌 버튼이 회색이면 이 가정이 틀린 것.

## 22번 ICGuard 묶음 C 보강 (src/ic_guard.cpp, 시험 test/test_icguard.py 기존 15 + [22] 19 PASS)
- DomesticCE(a1cd31a7, 게임 폴더 사본 = r41 payload f22 같음) 3초 타이머 0x1800013a0:
  P = [[game+0x27d10f0]+0x1e0] 의 첫 qword(0x180002230). 지역 i(0..337) = P-0x51510+i*0x98 (게임 CAreaData, 0x98 간격, 게임 지역 표 db+0x6caf0 포인터 501칸, new[501] at 0x3263d2).
  첫 고리(0x180001438~) 값 검사 → 둘째 고리(0x18000163b~) 중점 개발(+0x72) 쓰기 0x180002790 + 인구 하한(+0x52) 1만 0x1800028a0 → 셋째 고리 AI 명령서(P+0x4858, 0x30 간격 50칸, +0 세력 +4 명령서 99) → 저돌(P+0x24b8d6).
- 지역 칸 정체(게임 코드 근거): +0x3c/+0x3e/+0x40 개발 3종(0x22e2b0/0x22c760/0x231170), +0x52 인구, +0x72 중점 개발 1농·2상·3병(0x0b1aa0),
  **+0x78 BYTE = 세력 번호**(0x2b08d1/0x2b096e: 세력 관계표 +0xea 인덱스·ebp 세력과 비교), +0x79 = 생성자(0x220740/0x2267a0)가 안 채우는 빈칸.
  DomesticCE 는 +0x78 을 WORD 로 읽어 <256 → 실제로는 빈칸 +0x79==0 검사. 빈칸 쓰레기값이면 멀쩡한 지역 하나 때문에 틱 전체가 빠지던 후보 원인.
- R40 상태: 0x1800015f6(jne→nop)·0x180001611(je→nop)만 → 범위 밖 지역도 둘째 고리에서 처리되고, 대상 확인 장치(338곳 전부 범위 안)가 사라진 상태였음.
- R42 고침(같은 묶음, 4줄 모두 맞을 때만, 하나라도 다르면 하나도 안 바꿈):
  (가) 0x180001617(7바이트 mov [rbp+0x74],0) 감싸기: 지역 338곳 주소 = 게임 지역 표 연속 항목인지 확인 → 맞으면 지역별 모드, 아니면 원래 모드(하나라도 범위 밖/읽기 실패면 틱 건너뜀 0x180001bcc).
  (나) 0x18000163b(8바이트) 감싸기: 지역별 모드에서 그 지역 개발 ≤1000·중점 <4 아니면 다음 지역(0x180001627). +0x78/+0x79 는 안 봄(처리에 안 쓰는 칸).
  읽기는 ModCore Readable 확인 후 복사(우리 kernel32 가져오기 표에 ReadProcessMemory 없음).
- AI 명령서 50/52: 세력 표는 52칸(db+0xdf30, 0..0x33; CForceData 0x268 new[] 0x32685f). DomesticCE 0x30 간격 표(P+0x4858)가 무엇인지(군단 CTroopsData 는 0x40 간격이라 아님) 정적으로 확정 못 함 → **50 그대로 둠**.
  처음 지역별 모드 틱에 `R42D 22 SAN14_ICGuard.dll orders 46:세력/명령서 ... 55:.. forces=N maxforce=M j0=K` 한 줄 기록 → 50·51칸에 실제 세력 값이 있으면 52 로 바꿀 근거.
- 실게임 확인 줄: runtime.log `ICGuard R42 C: DomesticCE 3 s tick checks each area separately`, `ICGuard R42 C: DomesticCE target = game area table (338 areas verified)`(또는 `not matched ... original all-or-nothing`),
  범위 밖 지역 `ICGuard R42 C: area #N out of range (dev a/b/c, focus f) -> skipped only this area`(처음 5번), 적용확인.log `R39C 22 SAN14_ICGuard.dll domestic_area_skipped N`, `R42D 22 ... orders`.
- 위험: 지역 표 대조가 실게임에서 안 맞으면(예: DomesticCE 대상이 다른 판 배치) 원래 모드로 돌아감 = R40 보다 보수적(범위 밖 하나면 틱 건너뜀). 로그로 바로 구분됨.
