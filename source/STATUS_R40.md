# 통합패치 R40 작업 상태 — /home/claude/r40 (2026-10-28)
기준: R39 정식판 + R40 AI 호환판(/home/claude/r40ai). 원칙(사용자): 오래 해도 모드가 계속 돌아가야 함 — 실패하면 그 하나만 건너뛰고 나머지 계속 + 사유 기록.
사용자 결정(17:51 "니 추천대로"): 확실한 것만 R40(A1·A4·A6·A11~A21), 화면 쪽(A5·A7·A8·A9·A10·B1~B4·C)은 진단 기록 넣고 실게임 기록 보고 R41.

## 만드는 순서
1. ./build_all.sh (r40/src → build_out; 툴체인 r40/build_tc = p27/build + SetEndOfFile)
2. cd mt && bash build_new.sh (13번, 1.0.1판) / 시험은 mt_test/source(롤백판 시험용 소스, 같은 논리 수정 반영) + mt_test/tests
3. bash /home/claude/port/field_build.sh r40/field_src/native r40/field_out (11번, r40 IC 지문 추가)
4. ./sync_payload.sh → python3 build_pkg_r40.py → pkg/, dist/ (R39 대비 바뀐 ZIP = 1of11 만)

## 새·바뀐 것
- SAN14_ICGuard.dll(base, 새): IC 파일 그대로, 실행 중 덧댐. A 탐색 분리(0x1591d nop)·논공 불일치 무장 6턴 제외(0x56b20/0x4c840/0x2d750 감싸기) / B 창 고착 5곳(0x68430,0x47420,0x47870,0x47f90,0x48480 감싸기)·포로 512(0x2bb93 74 55→74 41) / E 문전박대 본거지(0x399b0) / F 처단불가 시 IC 포로 처우 처단 막기(0x23f04 je→동굴, ExtBridge.ini·NoExecute.ini 2초마다) / C DomesticCE 0x15f6·0x1611 / D AISearch 0x1e46. 시험 test/test_icguard.py 전부 통과, 에뮬 시작 6묶음 OK.
- 13번 MeritTraits(b6be9269): INTENT·손상 저널 → 새로 씀, 쓰기 실패해도 저장 안 막음, 로드 무장별 새 출발(통째 실패=새 출발, 오류창 없음), 장부 무장별 재구성, apply_previews/award_commit 무장별, AI 묶음 부분 진행, PREVIEW_CAP 512. mt_test/tests/test_r40.py + 옛 시험(새 기준으로 고친 단언) 통과, 나머지는 R39 와 같은 기존 실패.
- 25번 Patch27: DeathLogs.bin·OfficerNames.bin 반쪽 레코드 잘라냄, 헤더 손상은 .bad 로 두고 새로, 쓰기 실패 영구 꺼짐 없앰(에뮬 확인).
- 32번 HiredFace: 신분 1~6만 얼굴 사용으로 셈, 2500/2501 고용무장 재배정(test_hired_face 추가 통과).
- 29번 StatusColor: Cmd/Command/Menu/Btn/Button/Order 화면 안 칠함(A6) + 진단 R40D 29 paint_screen/blocked_screen(처음 40).
- 34번 RegionOfficer: 진단 R40D 34 warn_area(처음 30곳: 지역 번호·officer·kind·chars). 동작은 그대로.
- 4번 SettingsHelpScenic·11번 Field3D: r40 IC(ae365eab) 지문 추가(A19 — 없으면 명승 설정·3D 연결이 스스로 꺼짐).
- 19번 FPS120: R39 성능판 2d476b9b 포장(A18).
- AI 설치관리자(A21): 관리.ps1·진단.ps1 스팀 폴더 자동 찾기 → 바탕화면 「ai 수정 통합패치 호환판\SAN14_AI_v13.3_통합패치호환」에 넣음(10/28 18시). pwsh 못 돌려 정적 점검만.

- 설치기 시험(10/28 19:50, 리눅스 하네스 scratchpad/csc/build101.ps1·runexe.ps1, 소스 csc/srcR40): 기본 271 통과(R39 265)·0 실패 / ZIP 따로 풀기 11 통과 / R39 위에 덮어 설치 18 통과. ICGuard.dll·ini 설치 확인. 작업 폴더가 커서 하나씩 돌리고 지울 것(디스크 한도).

## R41 로 넘긴 것(진단 기록 보고)
A5 무장일람 색(R40D 29 줄로 빠진 화면 확인), A7 공석 "!" 오판·잘림(R40D 34 warn_area), A8 명품 썸네일, A9 대열 이동(11Patch.log), A10 개성 9칸 통합, B1~B3 연출 생략, B4 처단 버튼 회색, C 테스트 도구, 22번 AI 명령서 50/52칸.
## A22(10/28 19:43 사용자 지시, 다음 수정 때 꼭 반영) — 같은 제보 3명
DC 댓글 3건(10/27~28): "모드 설정 창이 왜 꺼지는지 모르겠다", "정보창에 모드설정이 안 뜸", 설치기 '게임 적용' 칸에 "적용 안 됨 – 시작 기록 없음".
제보 화면(R39 설치기): 8개 항목만 설치(일부 설치). 19·23·25 = 적용 안 됨 – 시작 기록 없음, 24 = 적용됨(파일), 33 = 실행 기록 없음. 맨 위 1~18 중 3개는 화면에 안 보임.
할 일: 일부만 설치했을 때 모드 설정 창(ModSettings)·정보창 버튼이 안 뜨는 조건, 시작 기록(SAN14_기록\적용확인.log·runtime.log)이 안 남는 조건을 코드로 찾아 고침. 일부 설치(1번 미설치 등) 조합 전부 설치기 시험에 넣기.
### A22 반영(10/28 20시, R40 재빌드 manifest ce5cb492, 1of11 만 바뀜)
- 원인: ModCore 가 리졸버(게임 확인)에서 멈추면 모듈 0개 → 모드 설정 창 없음 + 설치기 전부 '시작 기록 없음'. 계기 = exe 교체형 패치(신무장 1000명: SizeOfImage 0x3b57000·.cecode), 공통 파일 덮어씀, 다른 게임 폴더 실행. 사용자 본인 runtime.log 에도 exe 판 바뀔 때 'SKIPPED: unsupported version' 13회.
- SAN14_Notice.dll(새, base, SAN14Mods/Core): 실패 때만 불림 → 안내창(코드 R40-MC1~6-타임스탬프-크기) + 적용확인.log 'R40E\tMC' 줄. src/notice.cpp.
- ModCore 패치(modcore_patch.py → 40c415a0): 실패 길 0x1b90·0x1fc9 → 동굴 0x2140/0x2170(기록 그대로 + Notice 불러옴). 정상 길 바이트 동일. 에뮬: 정상(안내 없음)·SizeOfImage 바꿈(MC2)·리졸버 없음(MC5) 확인.
- 설치기: WholeStop(exe 종류·runtime.log 마지막 실행 SKIPPED·R40E 코드·CrashLog 줄 없으면 다른 폴더), BaseChanged(상태줄 빨강), Detect 는 appmanifest_872410 있는 라이브러리 우선 + 복사본 알림, 바탕 공통 파일(SAN14Mods/Core·리졸버)이 남에게 바뀌어 있으면 되돌림. Tests A22 14개 추가 → 기본 288·분할 11·R39 위 18 전부 통과.
### R40 실게임 확인(10/28 20:37~, 사용자 PC R40 설치)
- 모두 정상 시작(SKIPPED 0, ICGuard 6묶음, 13번·4번·1번 OK, 새 튕김 없음). A9 대열 이동·수행: 사용자 확인 "잘 뜬다" → R41 목록에서 뺌.
- A5 확정: 무장일람(공로 탭) 항장 회색 안 됨 + R40D 29 줄 0개 → 무장일람 표는 0x930d20 공통 경로를 안 지남. 표 칸 글자 넣는 함수 찾아 같은 판정 걸 것.
- A7 오판 확정: R40D 34 warn_area 15곳 전부 officer=0 인데 맹진항 정보창 '담당 조운' → [지역+0x36] 이 담당관 칸이 아님(또는 지역 포인터 틀림). 게임 정보창 '담당' 글자 만드는 코드의 출처로 판정 바꿀 것.
- A7 잘림 확정: '!' 붙은 거점 이름 끝 글자 잘림(신정→신징, 평잉). 우리 폭 넓히기(0x931310/0x1a8560)가 게임 쪽 재계산에 덮이는 듯 → 폭 정하는 함수 자체에 걸 것.

### R41 추가 지시(10/28 20:56): 12번 필드 안개(1.6·미니맵) 통합패치에서 삭제 — 확정(20:57): 안개 삭제, 번호 재정렬 안 함(12번 빈 번호, 1~11·13~34 총 33개). 바뀌는 것(11번 일반판만, 12↔27 알림 삭제, 설정 창 줄 삭제, 기존 설치자 자동 제거)도 승인
- 지울 때: 12번 파일(FogTrial·Fog121·FogFieldPair·FogFieldPair.ini) 제거, 11번 3D 는 안개 연동판(11&12) 없애고 일반판만, 12↔27 충돌 알림 삭제, 설정 창 그래픽 탭 12번 줄(미니맵·날씨) 삭제, 기존 12번 설치자는 갱신 때 자동 제거(설정 파일 SAN14_FogAI.ini 등은 보관).

## 별도: 게임 폴더 찌꺼기 정리(패치에 안 넣음, 보류) — STATUS_R39.md 끝 참고. 게임 폴더의 AI 설치관리자 사본 정리도 같이.
