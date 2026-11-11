# R42 C 시험 도구 SAN14_SceneTest.dll (시험판 전용) — 2026-10-29 포크

파일: src/scene_test.cpp, build_c/build.sh(본판 build_all.sh 에 안 넣음), test/test_scene_test.py(ALL OK), test_tool/(DLL 988a62ea…, SAN14Mods/modules/SceneTest.ini, 사용법 5줄).

## 구조
- 버튼 창: 자기 스레드 Win32 창(맨 위·도구창, 맑은 고딕) + RegisterHotKey Ctrl+Alt+1~9. user32/gdi32 는 GetProcAddress 로(툴체인 user32.def 에 MessageBoxW 뿐). 버튼은 req 만 세팅(진행 중이면 거절).
- 실행: CUserStrategyState Tick 칸 0x13d63c8(원본 0x44caf0)에 PatchTester R2 방식으로 이어 붙임(5초 안정 뒤, 앞 처리 먼저 실행). AI 모드는 이 칸 안 씀(AI 는 0x13D63A8/B0/D0). 
  원본 처리 뒤 `0x56fe10(state)`(활성) && `[state+0x470]` 1·2(게임 메뉴 명령 처리 0x44f310 가 도는 단계)일 때만.
  근거: 게임 자동저장 0x321af0 와 메뉴 명령 0x44f310 둘 다 이 Tick 안(0x44d05d·0x44d08a)에서 불림 → 같은 스레드·같은 단계.
- 1~7: ① 빈 칸 ② 자동 저장 ③ 저장 확인(1초마다 칸 정보 다시 읽기, 60초 한도) ④ 시험. 8·9 는 데이터 안 바꾸는 상태 기록이라 저장 없이 바로.

## 자동 저장 근거
- 칸→파일: 0x32d090(칸) — 0..0x31 svdexceKO%02d.s14(수동), 0x32..0x3b autosdexceKO(자동), 0x3c..0x6d svdexceccKO(도전 모드 수동), 0x6e.. autosdexcecc. 도전 모드 = 0x1d37b0()!=0 && 0x1d2a00()!=-1(자동저장 0x321c38 과 같은 판정).
- 빈 칸: CSaveLoadManager [game+0x27dba68]. 0x935ac0(mgr,0)=칸 머리 120개 다시 읽기(시작에 0x9359c0 로 비움, 저장 창 열 때·자동저장 때 게임이 부르는 것), 0x9352e0(mgr,칸)=머리(없으면 0).
  보조: Steam 원격 FileExists(게임 0x935b46 과 같은 호출: IAT 0x1340b28 SteamInternal_ContextInit(game+0x1a280c0) → [0] ISteamRemoteStorage, vt+0x68). 머리 없음 + 파일 없음인 첫 칸(사용자 PC 면 17 → .mapx 등 부속 파일만 남은 칸은 머리 기준이라 영향 없음, 본 파일이 남아 있으면 건너뜀).
- 저장: CSaveLoadState vf5(0x50b220)의 '새 칸' 길과 같음 — 요청 {int 칸, std::string 파일, std::string 설명("")}(0x48) → 0x33aee0(전역 0x27d2a90/98/b8 에 넣고 입력 해제) → 0x466530(상태관리 0x110e0(), "CSaveState"@0x13b2918, 0)(CSaveState 0x4f0 만들고 쌓기).
  CSaveState 일꾼 0x56f470 이 0x329a70(root,0,파일,설명,0)(MeritTraits F_SAVE_PREP 자리 그대로 거침)로 씀. std::string 16바이트 넘으면 게임 할당기 0x3f6bc0(해제 0x3f6c50 와 짝).
- 확인: 칸 머리 생기면 성공. 60초 안에 안 생기면 시험 안 함(4번은 오류 표시 원복).

## 버튼별
1 재해(B1): 내 지역(0x22c990→0x2337b0) 중 역병 없는 곳을 부모(+0x35) 다르게 최대 4곳 골라 역병 '대기' 비트(+0x38 bit1, 표 0x13a7cc0) 켬 → 분기 달 검사 4곳(0x299dcd·0x299dde·0x29a513·0x29a521, 6바이트 jcc)을 이 호출 동안만 nop(원래 바이트 확인 뒤, 끝나면 복원) → 0x427ff0 직접 호출(재해 0x297490: 0단계 분기 처리 → 3단계 대기→발생 0x29a4b0(연출 자리 0x29ac7b·97·0x29aea6·b3) → 4단계 → 풍작 0x2ba170 → 요약 자리 0x427ffe).
  기록: 고른 지역 비트·남은 달(+0x6b)·인구(+0x54) 전/후, SceneSkip 카운터(TestTable 7~11 음악/띠/창/접기/요약) 전/후. 실제 발생(+0x38 bit0) 여부.
  주의: 0단계 분기 처리라 진행 중이던 역병·메뚜기가 끝날 수 있음(시험 저장본으로 되돌리기). 필터(0x2c8b80 등)로 안 걸리면 "press again".
2 등용 결과(B2): 장면 입구 0x6c43b0({군주, 남의 세력 무장, 성공1}, {…, 실패0}) 두 번. SceneSkip 등용 생략 켬·훅 됨일 때만(만든 인자라 생략 안 되면 위험) → CStrategyEmployTalkState(0x1441088) 가 상태 더미에 있나(0x56fc30) 확인. 실제 소속 변경은 안 일어남(부른 쪽 함수가 하는 일).
3 보물 발견(B3): 0x6c4670({종류0, 군주, 곳 없음, 명품 표 첫 명품}) 한 번 → CSearchDiscoveryState(0x1447328) 확인. 곳 없음 → SceneSkip 제목 문장(0xc39d). 명품 소유 안 바뀜.
4 논공 오류(13번): MeritTraits SAN14_MeritDiagnosticsV1 의 metadata_fault 를 1로 → 자동 저장(이게 시험) → 저장됨·fault 0·MeritTraits 메시지창 0번이면 OK. 메시지창은 MeritTraits 가져오기 표 USER32!MessageBoxW 를 시험 동안만 세는 함수로(창은 그대로 뜸).
  못 한 것: '불러오기 때' 저장정보 불일치(hook_load 무장별 새 출발) 재현 — 게임 불러오기를 부르거나 보조 기록을 망가뜨려야 해서 안 넣음. 필요하면 시험 저장 칸을 사용자가 한 번 불러오고 MeritTraits 기록의 'R40: some officers…' 줄로 확인.
5 22번: 내 지역 하나 개발(+0x3c) 1001(DomesticCE 범위 밖) → 사용자가 며칠 진행 → ICGuard 'area #N out of range -> skipped only this area' 줄. 날짜 5일 지나면 이 도구가 원래 값 복원(플레이어 턴 칸에서). 플레이어 턴(날짜 멈춤)엔 DomesticCE 가 쉬므로 진행 필요.
6 명품 창: 메뉴 명령 0x15(0x44f310 표 0x44ffc0: 0x15=CStrategyItemState) — 게임이 메뉴 누를 때와 같은 호출. 수여 목록 그림(30번 A8) 눈으로.
7 모드 설정 창: SAN14_ModSettingsOpen(상태 쌓기 0x465a60 — 게임 상태 스레드에서 불러도 게임 메뉴 명령과 같은 스레드).
8 처단(B4): ExecuteLock 로드·켬·ic_greyed(op8)·NoExecute 상태 기록만. 창 자동으로 못 엶(포로 처우 창은 전투 뒤 0x2f95bf 길·IC 포로 회유는 IC 안 함수라 인자 모름) → 사용자가 포로 회유 창 한 번 열면 ExecuteLock 의 execute_button_greyed(_ic) 줄로 확인. 포로가 없으면 확인 불가.
9 개성 9칸(A10): SAN14_Traits9CheckTestTable 0 nine_sites()(5/5)·1 ui_hooked()·5~7 fault a/b/c·4 shown 기록 → 정상이면 무장 정보 개성 탭에서 9칸 눈으로.

## 기록 줄 (SAN14_기록\적용확인.log, runtime.log 에도 'SceneTest: ')
- `R42T 0 … player-turn tick chained on native|another module (0x…)` — 연결됨.
- `R42T n … auto-save to empty slot S svdexceKOSS.s14 started` → `auto-save confirmed slot S` → 시험 줄(OK / CHECK / SKIP / FAIL).

## 겹침
- 영구 코드 패치 없음. 실행 중: vtable 칸 0x13d63c8(이어 붙이기), 재해 시험 동안만 jcc 4곳(AI 해시 48곳 안 겹침, aihash.py 확인; r41f 훅 기록에 0x296000~0x29c500 은 Patch27 0x296172/0x2963d3 뿐), 논공 시험 동안만 MeritTraits IAT 한 칸.

## 시험 (test/test_scene_test.py ALL OK)
빈 칸(머리 0~16·파일 17 → 18), 요청 큐 거절, 명령 단계 아님(3) → 원래 처리만, 저장 요청 구조(칸·SSO 15자·설명 0)·CSaveState 쌓기, 확인 전 시험 안 함, 확인 뒤 6번 메뉴 0x15, 빈 칸 없음·60초 실패 → 시험 안 함, 달 검사 nop/복원·바이트 다르면 안 바꿈, 도전 모드 17자 → 할당기, 재해 지역 고르기(부모별·이미 역병·남의 땅 제외)·호출 동안만 nop, 8·9 저장 없음, 2·3 SceneSkip 없으면 장면 자리 안 부름.

## 본체에 부탁
- SceneSkip B2/B3 카운터(emp_skips/tre_skips)는 SsB23Table 이 export 가 아니라 이 도구가 못 읽음 → 장면 상태 유무 + SceneSkip 자체 R42B 줄로 판정. export 해 주면 더 확실.
