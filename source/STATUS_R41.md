# 통합패치 R41 (10/28 21시대, 사용자 "수정판 완성해봐")
작업 공간 /home/claude/r41 (r40 복사, 스크립트 경로 r41). 만드는 순서: ./build_all.sh → python3 modcore_patch.py → ./sync_payload.sh → python3 build_pkg_r41.py (pkg/data 는 r40/pkg/data 복사본 필요).
결과: dist/SAN14CE_통합패치_R41_1.0.1_1~11of11.zip, manifest 8221df58. R39/R40 대비 바뀐 ZIP = 1of11(2of11 은 안개 3D 변형 1개 빠진 것뿐, 나머지 파일은 R39 2of11 과 같음).

## 바뀐 것
- 12번 안개 삭제(번호 유지, 빈 번호): manifest 12 행·11&12 행 없음. 설치기 Spec.Retired(12), 옛 기록 12번 → 읽을 때 꺼짐(State.RetiredOn) → [선택 설치] 때 안개 파일 지움, 12-27 충돌·안내 삭제, 탭/전체 개수 33, 12번 줄 숨김. Engine.Clean() 이 어떤 경우에도 12 설치 안 함(시험용 KeepRetired). R40 manifest 해시 Known 추가.
- 29번 StatusColor(c645a25b→재빌드): H6 0x930e39(0x930e30 안) 별도 묶음 "SAN14CE.StatusColor.Table". 0x930d20 경로는 건너뜀, 같은 스레드 방금 꺼낸 이름(via=last) 또는 표 컨트롤 코드(0x866000~0x879000)에서 온 호출+최근 이름 목록(via=ring)이면 칠함. 이름 목록 32→128. 진단 R41D 29 direct <rva> via= paint= (처음 30곳).
- 34번 RegionOfficer: 0x2dff10(지역 담당 바뀐 뒤 늘 불리는 함수, 31곳) 입구 별도 묶음 "SAN14CE.RegionOfficer.Live" → 그 지역 이름판 즉시 다시 판정(화면 스레드 아니면 표시 후 다음 새로고침). "!" 폭 2칸. 원인 확정: 정보창 '담당'도 [지역+0x36] 을 읽음 → 판정 칸은 맞고, 이름판 새로고침이 턴 때만 돌아 옛 판정이 남았던 것.
- ModSettings: 27번 설명에서 12번 문구 삭제(12번 줄은 안개 없으면 자동 숨김 그대로).
- README_R41.txt R41 절.

## 시험
- test/test_status_color.py, test_region_officer.py 전부 통과(새 사례: 표 칸 칠함·0x930d20 경로 건너뜀·표 밖 직접은 방금 꺼낸 이름만·브리지6 / 즉시 반영 임명·해임·다른 스레드 미룸·사라진 판 안 건드림·브리지3).
- 설치기: 기본 285·분할 11·R40 위 덮어 설치 25(안개 깔린 R40 → R41: 안개 파일 지움, 11번 일반판) 통과.
- 에뮬 r_g43(R41 전체 설치본): 모듈 다 올라옴, InstallHooks 31건 바이트 불일치 0, 새 묶음 2개 OK.

## 남은 것
- A8 명품 썸네일: 30번 명품 다중선택(목록 방식 2)에서 썸네일이 보유자(군주) 얼굴로 나오는 것으로 좁힘. 선택 목록 대화상자(0x6839e0, 설정 0x476c80: +0x18 방식 +0x1c 최대 +0x20 최소)에서 방식별 썸네일 분기 찾을 것.
- A10 개성 9칸, B1~B3 연출 생략, B4 처단 버튼 회색, C 테스트 도구, 22번 AI 명령서 50/52칸.
- 실게임: R41 깔고 나서 R41D 29 줄(표 경로가 0x86f9f2 였는지)·34번 즉시 반영 확인.

## R41b 진단판(10/28 21:4x, manifest ea2ffb62)
- 34번 RegionOfficer(84aa… 재빌드): 진단만 추가 — 이름판 처음 볼 때·4초 뒤 "R41D 34 plate/plate_later"(kind·이름·data·area·표 번호·+0x36·+0x35·지역 이름·vtable·글자 칸 폭/높이·배경 부품 폭), 정보창 '담당' 줄 0x91f050 걸어 "R41D 34 panel"(area·표 번호·+0x36·이름·vtable, 처음 40곳, 별도 묶음 SAN14CE.RegionOfficer.Diag), 담당 바뀜 "R41D 34 live idx thread=ui/other"(처음 20).
- 시도: 도시형(kind 0) 글자 칸 자식 0x16 폭도 높이×글자수로 넓힘(거점형은 원래 0x931310 으로 넓혀 와서 안 잘림 — 화면에서도 맹진항·포판항은 안 잘림, 신정·하남 등 방패형만 잘림).
- 다음: 사용자 PC 기록에서 panel 의 area 와 같은 이름 plate 의 area 비교 → 다른 대상이면 그 차이로 판정 수정. live thread=other 면 매 프레임 지도 처리(0x13d63c8, 22번 뒤에 엮기)로 즉시 반영.

## 2026-10-28 23:45 R41d
- A7(34번) 해결·사용자 확인("빨간색 뜨고 담당자 있으면 사라짐"): 이름 글자 안 바꾸고 글자 스타일만(도시형 칸0x16 0x13→0x14, 거점형 칸0xe 0→0x10, 0x92ef10). 판정은 vf2(0x1a7f50/0x1a7ea0 입구, 매 프레임 화면 스레드) 0.15초, 공석 0.3초 지속, 빨강 1초 재칠. R41d 1of11 전달.
- A8(명품 썸네일) 분석 중 — 사용자 요구: "확인 또 확인", 테스트 파일은 마지막 확인용 1번만(사용자에게 반복 시험 시키지 말 것).
  - 목록 = CDataSelectState<CItemData>(vt 0x1449cc0) / CDataSelectDlgBase<CItemData>(vt 0x13e74c8). 형식별 슬롯: 2 0x54bd00, 5 0x5451d0, 7 0x491170(정보패널 0x5383c0 조건: cfg+0x94 또는 dlg+0x2d8==1&&cfg+0x90==0x17), 63 0x4d8870(설정, 방식2 버튼 분기), 70 0x539830(줄 포커스→0x50e6f0→0x4f41e0), 71 0x5342d0(줄 클릭: 방식2 체크/방식1 결정), 73 0x551410(선택 수).
  - 그림 넣기 = 0x9307b0(칸묶음, 칸, 종류, id,...): 종류 0xa=얼굴(무장+0xa6), 0x11=명품 그림(0x2100e0(명품)). 0x51f540/0x51fac0 = 무장 정보 패널(얼굴 칸 0xb).
  - 아직 못 찾음: 목록 왼쪽 그림 칸을 채우는 곳(방식1 명품 그림 / 방식2 군주 얼굴 갈림).
- A8 원인 확정(코드): 명품 선택 목록(0x6839e0)은 설정+0x114 가 켜져야 그림 칸이 명품 그림(람다 0x561f40), 꺼지면 보유 무장 얼굴(람다 0x65eee0→0x5a2ff0). 게임의 다른 명품 목록(0x556624 등)은 +0x114=1, 명품 창 목록 3곳(0x747ca6·0x7486db·0x750e2e)만 안 넣음. 목록+0x26c=설정+0x114(0x4d8fb0), 그림 칸 만들기 0x55c8f0, 얻기 선택 0x4ce8f1.
- R41e: 새 모듈 SAN14_ItemPicture.dll(30번, f30 + SAN14Mods/modules/ItemPicture.ini). 0x6839e0 입구 9바이트 훅, 부른 쪽이 +0x114/+0x118/+0x11c 아무것도 안 정했으면 +0x114=1. 단위시험 ALL OK, 설치기 287 PASS, 시작 에뮬 30묶음 충돌 0(DragGive 5곳과 별개). R41e 1of11 전달(확인용).
- 2026-10-29 17:1x 사용자 확인: R41e 명품 그림 됨(적용확인 item_picture_set 17:14:25).
- AI 모드 v13.3 통합패치호환(바탕화면 ai 수정 통합패치 호환판) 대조: 편의 DLL 지역담당관 초상화가 vtable 칸 6개(0x139a338/350/328, 0x13999c8/9e0/9b8)와 함수 48곳 FNV-1a 해시(0x180184770 표)를 검사 → R41d/e 의 34번 vf2 입구 훅(0x1a7f50, 0x1a7ea0)이 해시 범위 안 → AI 초상화 안 켜짐. R41f: 34번 훅을 vf2 끝 에필로그(0x1a7fc5, 0x1a7f41, 11바이트)로 옮김. 에뮬 30묶음 설치 후 AI 해시 48곳·vtable 6칸 전부 일치(R41e 는 2곳 다름) 확인. 설치기 287 PASS. R41f 1of11 만들어 둠(아직 안 보냄).
- 사용자 PC 게임 폴더엔 현재 AI 모드 미적용(dinput8.dll·SAN14_CombatAI.dll 없음).

# R42 (2026-10-29, 사용자 "이거부터 해": 미룬 5건 → 완성본)
## B1~B3 4번 연출 생략 = 새 모듈 SAN14_SceneSkip.dll (payload/f04, 4번 선택 시 설치, SAN14Mods/modules/SceneSkip.ini [SceneSkip] Disaster/Employ/Treasure)
- B1 재해(scene_skip.cpp): 재해 처리 0x427ff0 = 0x297490(역병0·메뚜기1 단계기: 0x299d70→0x297500→0x2989a0 발생→0x29a4b0 번짐→0x29b180) + 0x2ba170(풍작 0x2ba220).
  연출은 발생·번짐·풍작 3함수에만, '내 지역 걸림'일 때: 소리 0x404a00(소리 관리자 0x27d26c8, 7/5) · 알림 띠 0x451ff0→0x842a90(0xd 그림 0xf/0x10/0xe + in 애니) · 나레이션 0x1f0ad0(CCmnNarrationState 모달) · 띠 접기 0x43ff90→0x835230.
  12곳 call → 우리 함수(켬이면 안 부름, 나레이션 문장은 모음). 효과(지역+0x38 비트, +0x6b/6c=0xc, +0x54 감소)·이력(0x934400 종류1)은 게임 그대로. 요약: 0x427ffe(10바이트) → 모은 줄 있으면 나레이션 1회(최대 6줄 + '(외 N건)').
  카메라: 이 경로엔 카메라 호출(0x451360) 없음. IC 카메라 스위치 자리 9곳(이벤트 SActMoveCamera* 4 + 0x420bcf·0x422a5a·0x85fc18·0x85fa47·0x855a99)과도 별개.
- B2·B3(scene_skip_b23.cpp, 포크): 등용 대사 0x6c43b0 / 탐색 명품 발견 0x6c4670(종류 0만) 입구 6바이트. 이력 한 줄 SsAddHistory(종류1, Patch27 관례) — 게임 자체 이력(종류2)과 '전체'에선 두 줄. notes_b23.md.
- 묶음 3개 따로(Disaster/Employ/Treasure). 시험 test_scene_skip_b1.py(SS_FULL=1)·test_scene_skip_b23.py ALL OK. AI 해시 겹침 없음(tools/aihash.py).
- 설정 창 4번 줄 3개(S_SCENE), Spec 4번 '버튼 7개', Engine Judge 4 + RuntimeIni SceneSkip.ini.
## B4·22·A10·C 반영 + R42 완성본 (2026-10-29 19시)
- B4 SAN14_ExecuteLock.dll(f26, ca0325f6) 게임 0x66ff4e + IC 0x18006acd0 / 22번 ICGuard 묶음 C 보강(20cb262a) — notes_b4_22.md.
- A10 공용 부품: Traits9+Traits9_UI when 9|13, MeritTraits 폴더 사본 삭제, MeritTraits de2c78e1, 14번 v3(cacc22f8), Traits9Check(06b30a6e) — notes_a10.md. (IC 는 Traits9Module ini 안 봄 — 파일 있으면 불러옴)
- C 시험 도구 SAN14_SceneTest.dll(test_tool/, 본판 제외) — notes_c.md. 에뮬에선 창 못 만듦(가짜 user32, 정상).
- SceneSkip 0ae0e272(SsB23Table export). 설치기 Known: prev_manifests/state_r41e_user_installed.xml·manifest_r41f_built.xml.
- 검증: 단위시험 전부 OK, 전체 설치본 에뮬 34묶음 불일치 0·AI 해시 0, 설치기 296 PASS, ZIP 해시=manifest. 바뀐 ZIP = 1of11 만(2~11 은 R41 과 같음). manifest ae8f21d1.
- (19:0x 사용자 "게임 로컬 폴더에 안 넣어도 쓰게, 경로 자동") 시험 도구를 바탕화면 실행기 방식으로: src/scene_test_launcher.cpp → 시험도구_실행.exe(10fb32cc). 실행 중 SAN14CE_KO.exe 를 이름으로 찾음(없으면 steam://rungameid/872410), ModCore 올라온 뒤 SAN14_SceneTest.dll(2e967eb0)을 LoadLibraryW 원격 스레드로 올리고 SAN14_SceneTestAttach(이미지 크기 0x29ff000·ModCore 확인, 한 번만) 호출. 기록은 게임 폴더 SAN14_기록 그대로. test_scene_test_attach.py OK.
  PC 에 넣음: 바탕화면\통합패치\테스트\시험도구\(시험도구_실행.exe, SAN14_SceneTest.dll, 사용법.txt). 게임 폴더엔 아무것도 안 넣음. 위험: 백신이 원격 스레드 올리기를 막을 수 있음(그러면 실행기가 오류 문구 띄움).
- (19:1x 사용자: 재해 두 번째 누르니 튕김 + "어떤 결과든 화면 밑 로그창에 한 줄 무조건")
  튕김: CrashLog 기록 없음(VEH 못 잡는 종료 = 힙 손상/fastfail 추정). 기록상 두 번째 자동 저장 파일은 다 써졌고 'confirmed' 전 종료.
  원인 추정: 시험 도구가 저장 확인하려고 1초마다 0x935ac0(칸 머리 전부 해제 0x9359c0 + 120칸 다시 읽기)을 불러, 저장 일꾼 스레드가 만지는 칸 머리와 겹침(해제된 메모리).
  고침(SAN14_SceneTest 861bd706): 0x935ac0 안 부름. 확인 = CSaveState 가 상태 더미에서 빠짐 + Steam 원격 파일 있음. 빈 칸 고르기도 다시 읽기 없이(칸 머리+Steam 파일).
  로그창: 화면 밑 로그(CMainLogLayout 0x142d190)는 이력 줄(0x934400, +0x86 종류 필터 0x668d10, 체크 없으면 전부)을 보여 줌.
   SceneSkip(6242e6bf): 재해 건너뛸 때마다 그 문장을 SsAddHistory(종류1, a5=1)로 한 줄. 시험 도구: 결과 문구를 '[통합패치 시험] …' 한 줄로(게임 스레드에서만, 진행 문구 제외).
  R42 1of11 다시 묶음(manifest 바뀜), 설치기 296 PASS. PC 시험도구 DLL 덮어씀.
## 다음 수정안(사용자 10/29 19:33 확정, "만들어" 전까지 만들지 않음)
- B1 재해: 요약 창(0x427ffe 나레이션) 없앰 → 창 0, 로그창(이력) 한 줄만. 등용 결과·보물 발견도 창 없이 로그창 한 줄만(지금도 그렇게 동작 — 유지 확인).
  고칠 곳: scene_skip.cpp SsSummary 에서 0x1f0ad0 호출 빼기(요약 줄 모으기·0x427ffe 훅도 필요 없으면 제거), 설정 창 4번 설명·Spec 4번·README 문구에서 '요약 창' 삭제, test_scene_skip_b1 단언 수정.
## R42b (10/29 19:5x, 사용자 "다 수정하고 그 수정본으로 테스트 폴더에, 시험도구도")
- SceneSkip 704c868b: 재해 요약 창 없앰(0x427ffe 안 검, 12곳만). 재해마다 로그창 한 줄(SsAddHistory). 문구: 설정 창·Spec·README 에서 '요약 창'·'중요 탭' → '화면 밑 로그창 한 줄'.
- 시험 도구 SAN14_SceneTest 0b90aea7: 재해 판정 = 창 0·음악/띠 건너뜀·나레이션 건너뜀 ≥1 → OK. 결과 '[통합패치 시험] …' 로그창 한 줄.
- 튕김(시험 1 뒤 아무 버튼 → 자동 저장 직후 종료, 3번 재현) 원인 추정: 시험 도구가 내 턴 틱 안에서 0x427ff0 을 불러 요약 나레이션(0x1f0ad0)이 CUserStrategyState 틱 도중 상태를 멈춤 → 다음 저장 상태가 빠지며 돌아갈 때 꼬임. R42b 는 창 자체가 없어 해당 없음. (저장 확인 0x935ac0 제거는 원인 아니었음 — 그 판에서도 재현)
- 설치기 296 PASS, manifest R42b. PC 테스트 폴더에 바뀐 파일만 덮어씀(ModSettings·SceneSkip·manifest·설치기 소스·README·SHA256SUMS) + 시험도구 DLL. 2~11 은 그대로.
- 19:41 사용자 [선택 설치]가 시작 두 줄 뒤 멈춤(state.xml 19:02 그대로) — 게임 켜진 채 눌러 '게임 종료' 오류로 추정(오류는 CollectRecords 뒤 WriteLog 라 설치기.log 에 안 남음 — 다음 판에서 순서 바꿀 것).

## R42c (2026-10-29 밤) — 4번 카메라 전부 막기 + 턴 끝 결과 창
- 카메라: 주 카메라 요청 CMainCameraCtrl::vt[2] 0x143d0(vt 0x13426f8, 0x11170()+0x50) 입구 11바이트 훅. 돌아갈 주소로 판정,
  0x45141f(공용 0x451360 안)이면 진입 rsp+0x100 = 0x451360 을 부른 곳. 막음 표(직접 13 / 0x451360 바깥 7), 허용 표(직접 10 / 바깥 8), 모르는 자리는 그대로+기록.
  카메라 뒤 기다림 0x304640 10곳 건너뜀. 스위치 = San14ModSettings.ini [Camera] AutoFocus=0 (설정 '카메라 자동 이동' 끔 → SceneSkipSet(4)).
- 재해 진행 중(29단계 0x428010→0x297e10): 효과음 0x29833a/0x298694(rdi=문장) → 로그 한 줄, 창 0x29840b/0x298805, 멈춤 0x298413/0x29880d 짝 건너뜀.
- 턴 끝 결과 창: 월 처리 단계 실행기 0x42dab0(CTurnState#5, 79단계 표 0x1a61040/조건 0x1a612c0), 78번 0x428b00 입구 훅 → 모은 것 있으면 0x1f0ad0 창 1번 + 0 반환(멈춤) → 창 닫힌 뒤 원래 끝 단계.
  명승·증축은 게임 이력 0x934400 입구에서 우리 세력(r8d == 0x32e910()->vt[0x60]) 줄의 제목/본문에 '명승'/'증축' 이 있으면 모음(글자 기준, 실제 문구 미확인).
- 시험: test_scene_skip_b1(SS_FULL) / test_scene_skip_r42c / test_scene_skip_e2e(게임 코드 그대로: 0x2e8e30 막힘, 0x451360 바깥 판정, 단계 실행기) / b23 ALL OK. AI 해시·쓰기·참조 겹침 0.
- 시험 도구: 1번 재해에 진행 중 단계(0x297e10)까지 + 카메라 수 판정, 10번(Ctrl+Alt+0) 카메라 막기: 판정표 + 실제 사건 카메라 0x2e8e30 요청 → 1.5초 뒤 위치 비교.
- R42c 추가(21:4x): 게임 튕김 0x9bbbc(10/28 22:14 첫 발생, 오늘 19:07·19:30·19:33·21:26·21:30)는 AI 부대 길찾기 스레드가 길 목록(0x1427d10f8 노드 표)의 지워진 칸을 읽는 것.
  계절 맵 v26k38b(19:02 설치)의 RouteSearch·RouteCost(길찾기 교체) 시기와 맞음, 두 번은 달 바뀌고 20초 뒤 → 계절 맵 쪽 문제로 넘김(통합패치 안 건드림).
- 9칸 오류 C(ALLSCREEN_SKIPPED geometry): Traits9_UI(49fb15b4, 소스 없음) +0x2280 이 요약 칸(CInfoSummaryPersonality 등, 0x8a4450 목록 칸 포함)에서 실패.
  Traits9Check 가 시작 뒤(게임 자료 올라온 뒤, 13번 지문 확인 뒤) +0x2280 입구 14바이트를 ff25 → T9AllCreate 로 돌림: 원래 조건 맞으면 원래와 똑같이(시험으로 9칸 위치·크기·부모·기억칸·말풍선·기록 동일 확인),
  어긋나면 why(P/R/E/G/N) 기록(R42T9 D 줄, 오류창 글에도 붙음) + 너그럽게(높이 ±8, 줄 간격 >= 높이/2, 같은 줄 ±4, 폭 >= 20) 다시. 부모 다르면 안 만듦.
  AI 안내는 input_state.json "installed": true 또는 SAN14_CombatAI.dll 있을 때만(사용자 PC 는 해제 상태였는데 안내가 붙었음).
- 시험 도구: 1번 재해 = 조건만 걸고(대기 표시+달 검사 끔) 사용자가 턴을 넘기면 실제 처리 → 내 턴에서 되돌리고 결과. 5번·지역 읽기: CAreaData(vt 0x13a6980)만(표 뒤쪽 다른 자료를 읽어 20:28 튕김).
- R42c 정식판 ZIP(22:42) 11개, manifest a6f1e429 → prev_manifests/manifest_r42c_delivered.xml

## R42d (2026-10-29) 설치기 판정만
- 13번 '게임 적용' 칸: 9칸 화면 부품 기록이 APPLIED 가 아닌 성공(ALLSCREEN_GRID_CREATED·TABLE9_CREATED 등)이면 노란 '실패'로 오판하던 것 수정.
- Engine.T9Screen: 마지막 실행(R39 9 줄 −60초 이후)의 적용확인.log R42T9 C/D 줄 + 화면 부품 기록 마지막 줄 → FAIL/OK/NONE. 9번은 OK 일 때 화면 실패면 PART.
- 설치기 시험 306 PASS(새 13번 판정 11개 포함). 실제 PC 기록으로 전 항목 판정 확인(13번 초록).
- 정식판 폴더: 바탕화면\통합패치\통합패치 10월 29일 아마 최종판 (사용자가 지정) — R42d 6파일 덮어씀. 테스트 폴더도 같음.
- R42d 추가: 9번+13번 "같이 켜도 됩니다" 참고 창 삭제(Spec 9-13 규칙 제거). 정식판 폴더에 설치기 파일 6개 덮어씀.
- R42d 추가2: 10번 신무장 탭 0명 수정(ICGuard 묶음 G, ic_guard.cpp): IC CreateFileW 가져오기 칸(IC+0x9a060)만 바꿔 게임폴더\SteamData\editdataexceKO.s14 없으면 2D v25 순서(ini→원래→같은폴더 다른이름→스팀 라이브러리/HKCU SteamPath/HKLM InstallPath의 userdata\*\872410\remote, 로그인 계정 우선·최신)로 실제 파일 열기
  + IC 해석 0x18002e410 의 [+2]==4 검사 nop(0x2e4f1), 번호 불일치는 그 칸만 건너뛰기(0x2e4d9 jne→0x2e9f6) — 2D tools/patch2d_chain.py 와 같은 수정. 사용자 파일: 실제 [+2]=0x4003 이라 원래는 전체 실패.
  시험: test/test_icguard_newoff.py ALL OK(9경우), IC 해석을 실제 사용자 파일로 에뮬 → 1명(3001) 읽힘(고치기 전 0), test_icguard 22 ALL OK.
  22번 "AI 명령서 무제한" → "AI 명령서 99장 유지"(ModSettings·Patch27 옛 창·설치기 Spec). 정식판 폴더에 9파일 덮어씀(ICGuard b6460e15, ModSettings 61aac094).

## R42g (2026-11-02) 22번 AI 내정 순환(군주 반영) — 새 모듈 SAN14_DomesticAI.dll (f22)
- 제보(카페 어지랭이): AI 는 상업만 몰빵. 확인: 22번 DomesticCE 순환은 플레이어 세력 지역만(소유 확인 0x180002230/0x180032340). AI 는 원작 판정.
- 게임 AI 판정 자리: 행동트리 노드 CDomesticPriorityDevelopmentSelectNode (vtable 칸 0x134c8f8 → 0x7a110), 그 안 유일한 호출 0x7a1db → 0xb1aa0(군단, 지역, int*결과, 가중치 3개).
  원작 0xb1aa0: 군단 방침(+0x1a=2/3/4)이면 그 항목(1000 이면 아래로), 아니면 군단장 물자선호(+0x170/172/174 +100)×가중치×단계(0x1413a6d9c 표 100/300/600/1000) 최소(같으면 상업). 결과가 지금 중점(+0x72)과 같고 담당관(+0x36) 유효면 0.
  결과는 CDomesticOfficerSetNode(0x36320) → 임명 명령 0x1ed290 이 씀. 다른 쓰는 곳: 0x76a3c0(화면 쪽). AI 가 매일 다시 쓰지 않음.
- 지역: 표 *(db+0x6caf0) 0x98 간격, +0x36 담당관 id, +0x3a 중심 칸, +0x3c/+0x3e/+0x40 상업·농업·병영, +0x72 중점(1상업 2농업 3병영). 소유 세력 0x22c810, 플레이어 0x32e910(world).
- 무장: 표 db+0x158+id*8, 능력치 0x22fce0(무장, 0통솔..3정치..4매력, 0xfff), 기본 능력치 바이트 +0xec.., 물자 금/군량/병사 +0x170/+0x172/+0x174 (i16, 20=중시), 건설 +0x180, 호전 +0x16a, 야망 +0x17c (파일 레코드 +0x4e 차이).
- 규칙 domestic_ai_rule.h: 정치 70↑ 전부 / 40~69 절반 / 40↓ 3분의 1 순환(지역·세력·연월 섞기, 난수 안 씀), 순환 = 단계 가장 낮은 것(같으면 상업→농업→병영), 나머지 = 군주 물자 항목(1000 이면 순환으로).
- 기록: 적용확인.log 에 R42D 22 start/month 줄(R39 형식 안 씀 — 설치기 22번 판정은 runtime.log "DomesticCE: ACTIVE" 그대로 두려고).
- AI 모드(SAN14_AI, 사용자 PC 설치됨): 이 노드를 감싸 둠. domestic=0 이면 원래 0x7a110 을 그대로 부름(디스어셈블 확인, conv 0x180007e30) → 우리 규칙 돎. domestic=1 이면 그쪽 처리.
- 설정 창: 편의 탭 22번 다섯째 줄 "AI 내정 순환(군주 반영)" (S_DOM_AI, SAN14_DomesticAIGet/Set, SAN14Mods\modules\DomesticAI.ini AiRotate).
- 시험: test/test_domestic_ai_native.cpp (리눅스 네이티브, test/shim 윈도 흉내) 22항목 통과.
- 이번 소스에 들어간 R42e 수정: 2번 설정 이름(mod_settings.cpp, 전달 해시 80ac65d7 과 같게 재빌드 확인). 
  들어가지 않은 R42e 수정(작업 환경 초기화로 소스 없음, 전달 바이너리가 기준): ICGuard 묶음 H 길찾기 보호(a810d27d), 32번 얼굴 최적화(1f4e1b11), 34번 R42f2(1397f285), IP01 리졸버, 9칸(Traits9Check 356fecc0).
  → 다음에 그 모듈을 고칠 땐 메모(/areas/san14-patch-regression.md)·전달본 디스어셈블로 다시 얹을 것. 패키지는 전달본 위 덮어쓰기(build_overlay_r42g.py)로 만듦.
- 게임 exe 복호: port/stubdec.py (SteamStub v3.1) → .text 복호 후 objdump.

## 2026-11-03 업데이트 대비 선행작업
- 4_대장(PC 작업재료\업데이트대비\4_대장): 연결자리_대장(1326곳)·구조값_대장(34개)·지문_지도 + find_new.py(새 판 자리 찾기·구조값 검사)·apply_new.py(소스 없는 DLL 지문·주소·IC 훅표 한 번에 고치기, 원본 안 건드림). 일본어판·PK 로 검증.
- R42e·R42f 소스 재구성: R42e_소스재구성.md 참고. Notice 는 바이트까지 같음, ICGuard H·HiredFace·RegionOfficer R42f2·Traits9Check 는 동작 같음(네이티브 시험 4개 추가).
- installer101/ 를 배포 R42g(+R42g2 ManifestHash) 글자로 맞춤. 기준_R42g2/ 에 manifest·ModCore·IP01 리졸버 보관.
- build_all.sh: ONLY=이름.dll 이면 그 모듈만 빌드.
- 찾은 문제(미수정): Notice kResolver 틀림 → 정상 exe 에서 늘 R40-MC6 안내 / 설치기 Spec.Version "R42e" 그대로.
