# A10 개성 9칸 통합(9·13·14번) + GsjiN 제보 — 분석·설계·구현 (R42 포크, 2026-10-29)

## 1. 지금 구조 (R41f manifest 기준)
| 조합 | 9칸 데이터(5자리 5→9) | 9칸 화면(3x3 편집·정보창 9칸) 파일/이름 | 누가 화면을 붙이나 | 14번 TraitHover |
|---|---|---|---|---|
| 9 단독 | SAN14_Traits9.dll(게임 폴더, IC 가 시작 때 불러 적용) | SAN14_Traits9_UI.dll(게임 폴더) | Traits9.dll | v2(coexist) — **주인 이름 'UI_Traits9_Hover.dll' 만 앎 → 'startup tick owned externally' 로 전부 포기 = 전법 설명 안 나옴(실제 고장)** |
| 13 단독 / 9+13 | MeritTraits ModInit 의 ensure_nine (SAN14CE.MeritTraits.Traits9) | SAN14Mods/MeritTraits/UI_Traits9_Hover.dll (같은 바이너리 49fb15b4) | MeritTraits support_ui | v2 — 이 이름은 알아서 위에 얹힘(35 슬롯) |
| 14 단독 | — | — | — | v1 원본 |
- 9칸 화면 DLL 두 개는 바이트까지 같은 파일(49fb15b4)인데 이름·자리·주인만 조합별로 바뀜.
- 9+13 에서 생기는 문제(에뮬 v0 로 확인):
  · IC 기록 "개성 9개 UI·설명 애드온 연결 실패." — IC(0x18003afc0)는 이름 SAN14_Traits9_UI.dll 로만 찾음 → 13 조합에선 14번 '개성·전법 마우스 설명' 스위치(IC 스위치 8)가 9칸 화면 설명(UI+0xc000)에 안 닿음.
  · 9칸 자체가 MeritTraits 시작 성공에 매달림: native_guard_check / rules_prepare_native / prepare_hooks 중 하나라도 실패하면(AI 저장 겹침, 다른 모드가 지키는 21함수 수정, %LOCALAPPDATA% 쓰기 실패 등) ensure_nine·support_ui 까지 안 감 → 9를 같이 골랐어도 Traits9.dll 은 '9&!13' 이라 안 깔려 있음 → 9칸 통째로 사라짐.
- 게임 쪽 사실(정적 분석): 무장 개성은 원래 u16×9 칸(+0x118), 편집 자료도 9칸(+0x84, 0x21bb90 은 칸 0~8 허용), 신무장 저장(0x398ed9)도 9칸 통째 복사. 5칸 제한은 선택창 최대 수(0x4a6cf3·0x4a7313)·판정 3곳(0x22558f·0x23064a·0x232d97)뿐 → **결정 뒤 데이터가 5칸으로 잘리는 경로는 없음. '5개만 보임'은 화면 부품(3x3)이 안 붙은 표시 문제.**

## 2. GsjiN 원인 (코드 근거로 좁힌 결론)
- 9개를 고를 수 있었다 = 5자리 패치(ensure_nine)는 됐다. 결정 뒤 5개 = 편집 3x3 칸이 없다 = 화면 부품이 안 붙었거나(SKIPPED/LoadLibrary 실패) 그 화면에서 DISPLAY_SKIPPED.
- 9+13 에서 화면 부품은 MeritTraits support_ui 가 13번 전용 사본을 LoadLibraryExW 로 올림 → 이 단계 실패는 MeritTraits 가 아무 기록 없이 실패("prerequisite/hook verification failed")하는 경로(R33 사례: 실게임에서 사본 DLL 이 안 올라옴).
- 또 하나 확정된 원인(A3): AI 모드 설치기가 traits9 스위치와 무관하게 ScreenLayout 개성 화면 4항목을 9칸판으로 교체 → 화면 부품이 낯선 배치라 ALLSCREEN_SKIPPED(사용자 실제 기록 SAN14_Traits9_UI.log 와 일치). 통합패치 호환판 AI 설치기(P1/P2)는 이미 traits9 꺼짐 → 4항목 연결 안 함.
- 제보자 기록이 없어 둘 중 어느 쪽인지는 확정 못 함 → 이번 판부터 게임 안 오류창(아래 Traits9Check)이 실패 글자를 그대로 보여 줌.

## 3. 통합 설계 (구현함)
- **공용 부품 = 게임 폴더 SAN14_Traits9.dll + SAN14_Traits9_UI.dll** — 9 또는 13 을 깔면 언제나 깔림(when=9|13). IC 가 시작 때(ModCore 2번째) 불러 5자리+화면을 붙임 → 어떤 조합이든 같은 파일·같은 이름·같은 순서. 9칸이 MeritTraits 성공 여부와 무관해짐.
- 9번 = 콜라보 해금(SAN14_Collab.dll)만, 13번 = 논공 가챠(SAN14_MeritTraits.dll+리졸버)만.
  MeritTraits 는 이미 적용된 5자리를 건드리지 않고("existing verified Traits9 UI module preserved"), 공용 화면 위에 HoverX(가챠 개성 색)만 얹음.
- MeritTraits support_ui: 화면이 아직 없으면 게임 폴더 SAN14_Traits9_UI.dll → (없을 때만) 옛 SAN14Mods/MeritTraits/UI_Traits9_Hover.dll 순서로 올림(공용 파일 빠진 비상 경로, 에뮬 c_13_fb 로 확인).
- 14번 coexist 판 v3: 주인 이름을 공용 이름 SAN14_Traits9_UI.dll 로(th_coexist_a10.py, 같은 글자 수 20 → 16바이트만 다름). 9 단독+14 에서도 전법 설명이 붙음.
- 새 모듈 SAN14_Traits9Check.dll(9|13, 우리 소스, 게임 코드 훅 0): 시작 20초 뒤 5자리·0x491f01 확인, 이후 3초마다 SAN14_Traits9_UI.log 의 SKIPPED 줄 확인 → 게임 안 오류창(Ctrl+C 복사, TOPMOST 0x50030) + 적용확인.log R42T9 줄. 같은 글은 두 실행까지만 창, 한 실행 최대 3창, AI 흔적 있으면 'AI 관리에서 적용 다시' 안내 한 줄. ini [Traits9Check] Notice=0 이면 기록만.

## 4. 바꾼/새 파일
- /home/claude/r41/mt/src/merit_runtime.c (support_ui 경로) → mt/obj_new/SAN14_MeritTraits.dll **de2c78e1…** (옛 b6be9269 → /home/claude/r41/a10_bak/). 리졸버 45994929 그대로(재현 확인).
- /home/claude/r41/a10/th_coexist_a10.py, SAN14_TraitHover.dll.v3 **cacc22f8…** (원본 v1 8cfad1a6 에서 생성, 옛 v2 694ede16 은 같은 스크립트 기본 이름으로 바이트 재현 확인).
- /home/claude/r41/src/traits9_check.cpp → SAN14_Traits9Check.dll (본체 build_all 에 줄 추가 필요).
- 시험: test/test_traits9_check.py(ALL OK·WATCH ALL OK), /home/claude/r41/a10/th_test_a10.py(v3 유니콘 ALL OK).

## 5. 위험
- IC 가 9+13 에서도 화면 부품을 찾게 됨(지금까지는 9 단독에서만) → IC 스위치 8 이 UI 설명 켬/끔을 맡음. 9 단독에서 이미 쓰던 경로라 새 경로는 아님.
- 옛 설치에 남는 SAN14Mods/MeritTraits/UI_Traits9_Hover.dll: 공용 파일이 먼저 올라가면 안 쓰임(무해). 설치기가 manifest 에 없는 파일로 지우면 더 깔끔.
- AI 모드 원본 설치기(호환판 아님)를 쓰면 개성 화면 4항목 교체는 여전히 일어남 → 이번엔 오류창으로 바로 보임.

## 6. 시험 결과 (시작 에뮬 hooks_emu, new.dec.exe, scratchpad/a10/h_*.pkl)
| 조합 | 훅 대조 불일치 | 9칸 5자리 | 화면 부품 | IC 연결 | TraitHover | AI 해시 48곳 |
|---|---|---|---|---|---|---|
| v0 = 지금(R41f) 9+13+14 | 0 | MeritTraits | UI_Traits9_Hover | **연결 실패** | v2 35 | 0 |
| c_all 9+13+14 (새) | 0 | Traits9.dll | 공용 SAN14_Traits9_UI | 조기 연결 완료 | v3 35 | 0 |
| c_9_14 (새) | 0 | Traits9.dll | 공용 | 완료 | v3 35 (지금 판 v2 는 0 = 전법 설명 고장, v1 실험으로 확인) | 0 |
| c_13 (새) | 0 | Traits9.dll | 공용 | 완료 | — | 0 |
| c_13_14 (새) | 0 | Traits9.dll | 공용 | 완료 | v3 35 | 0 |
| c_14 단독 | 0 | — | — | (없음) | v1 74 | 0 |
| c_13_fb 공용 Traits9.dll 빠짐(비상) | 0 | MeritTraits | 게임 폴더 UI 를 MeritTraits 가 올림 | 실패(늦게 올라옴) | — | 0 |
| c_chk = c_all + Traits9Check | 0 | 〃 | 〃 | 완료 | v3 35 | 0 |
- 화면 부품 1087곳·TraitHover 35곳 쓰는 자리는 v0 와 c_all/c_9_14 가 완전히 같음(주인 이름만 다름). 다른 모듈 쓰기도 같은 자리(모듈 주소만 차이).
- 단위: test/test_traits9_check.py ALL OK + WATCH ALL OK(정상→나중 화면 실패 C, 9칸 안 켜짐 A, 화면 부품 시작 실패 B+C, AI 안내, Notice=0, 같은 글 2회, 실행당 3창, 게임 코드 쓰기 0), test/test_traithover_v3.py(v3 유니콘) OK.
