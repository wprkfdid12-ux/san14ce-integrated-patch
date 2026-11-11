# R43 — 게임 2026-11-05 업데이트판 호환 작업 상태 (2026-11-05)

기준: 새 exe stamp 0x6ad06f53 / image 0x2a02000 / sha 782cf229a56cdde2… (옛 1.0.1: 0x6a983c25 / 0x29ff000 / 87ed4e46…)
분석 보고: 새판_분석보고_20261105.md · 대응표: upd_tools/자리_대응_검증.tsv (find_new 결과 + 확인필요 49건 승격)

## 2-0 오류창 공통 부품 — 완료
- src/r39_record.h: `r39::fail / check_api / fail_plan / fail_bytes`(+module_hooks.h `fail_tx`) → 창(TOPMOST·안 막음·Ctrl+C) + 적용확인.log `R43F` 줄. 진단 코드 R43F-<번호>-<단계>.
- HookPlan·ModuleTransaction 이 처음 실패 자리(RVA·기대/실제 바이트·이유)를 기록 → 창에 그대로.
- 우리 모듈 20개 전부 연결(valid_api → check_api, FAIL/PARTIAL/SKIP 길마다 fail). 새 모듈 SAN14_Diag.dll(payload base, modules/Diag.ini): 시작 30·90초 종합 진단(로드/시작 줄/runtime.log 문구/통합편의 기록), 문제 있을 때만 창 + R43D 줄.
- Notice(판 불일치 창): 기대/실제 판·꺼진 모듈 목록·R43E 줄. 설치기: R43F/R43D/R43E 파싱해 '게임 적용' 칸·상세에 사유 표시(Engine/App/Tests, **PC csc 컴파일은 2-5**).
- 시험: test_diag_native 19/19, 자동논공 38/38, 내정 2종, 9칸, 34번, 32번 서명 동일.

## 2-1 새 판 주소 일괄 적용 — 완료(남은 것은 2-3)
- 소스: upd_tools/apply_src_new.py(정확/정렬 판정·≥0x10000 만) 233곳 + 전체 지도 2차 교체(변수 p/q·문자열·시험기) + 손으로 16곳
  (mod_settings ja 0x3ecbdf/0x3f24fb · status_color 에필로그 0x155128·표 범위 끝 0x87b350 · officer_names vt 0x13a7260 · scene_skip 소리 0x405b40 · domestic_ai 노드 0xb2af0 ·
   settings_help SettingsTick 원본 0x8ca1c0(새 칸 내용, 'ret 0' 디딤) + vtable 칸 +0x3090 · domestic_talent 칸 0x13d9458 · patch27 employ 기대 바이트 rel32).
  version.h: S14_GAME_STAMP/IMAGE/NAME 한 곳. 검증: upd_tools/verify_sites.py → 새 exe 와 97 일치 / 8 다름(25번 이름 5자리 = 2-3, 나머지 고침).
- 바이너리(남의 모듈): upd_tools/apply_new.py --addr → 1948곳(지문 포함). **고친 것: [rip+x] 모듈 안 참조를 게임 주소로 착각하던 것 제외(48곳 망가뜨릴 뻔)**, objdump 두 줄 명령 즉시값 파일에서 직접 읽기.
  확인필요 자료 자리 203건 중 190건 이웃 이동량 일치, 6건 내용으로 확인(IC 0x1a60010/14·0x1a3e290·CityCaps 0x1a4a1ec·vtable 3곳) → 승격. 남은 17건 = 2-3.
- 13번: mt/src 1.0.1 소스가 배포본과 바이트까지 같음 확인 → upd_tools/gen_merit_resolver_r43.py 로 merit_resolver.c 번역(함수 21·훅표 39·프롤로그 재독·hover 해시), presets_x_adapter 0x85ed10/0x859660, 재빌드 → payload f13.
- 해시표: upd_tools/update_hashes.py → common.h known_common r43(IC bf32cb4e…), DomesticCE 3곳(dd17a2eb…), notice kResolver(IP01 21e4ea3d…). SortiePresets .text 해시 변화 없음.
- 결과물: newpkg/payload_new(= 11월2일+R42i4 payload 에 전부 반영), 적용_보고.txt / upd_tools/소스_적용_보고.txt.

## 2-2 데이터 재생성 — 파일 완료(manifest 는 2-6 포장 때)
- 24번: upd_tools/fixdata_nostat.py(Stat.cs 파이썬 이식, 1.0.1 로 해시 일치 검증) → 새 원본 bbcf4f50… → nostat 70342a6e… (299칸 전부 원본값 그대로였음).
- 10번 KIDSSystemResource.rdb: 게임이 1 레코드 바꿈, 우리 2 레코드와 안 겹침 → 새 원본 위에 우리 레코드 적용(upd_tools/rdb_tool.py) ddbe33ad…; 원본 75ca69e8….
- 31번 ScreenLayout.rdb: 게임이 8 레코드 바꿈, 우리 9 와 안 겹침 → 713d929b…; 원본 8c017045…. CharacterEditor/MaterialEditor 는 업데이트가 안 건드림.
- 놓인 곳: newpkg/payload_new/f10,f31,f24 + newpkg/data_new/(data/ 자리용). manifest Values: fix_original/fix_nostat/model_orig_KIDSSystemResource.rdb/screen_orig_ScreenLayout.rdb 를 위 값으로(2-6).
- 설치기: Spec.ExpectedGame 새 sha, OldGames(1.0.1·1.0.0)+라벨, GameLabel, ExeKindFromHeader 새 stamp, 안내문 전부 판 이름 변수로. Version=R43.

## 2-3 로 넘긴 것(손으로 봐야 함)
- 25번 officer_names: get_s 0x20ffa0 / get_g 0x20e990 (게임이 함수 교체: 0x47872c 호출이 새 0x32dc20 을 부름), call 0x20a8d3/0x20a921 → 0x21fad0 자리(앞뒤 바뀜), 0x84ec80 help getter(못찾음).
- IC: 0x2b0290(바뀐 함수; add r10 ×3 + 훅표 0xddf70), 0x7e9890(→0x7ebe90 후보; add rax ×2 + 훅표 0xddae8), 0x773bd0→0x89ab90(EXIT_EVENT 디딤, 리졸버는 반영), 0x839ca0 cmp ×2, IC 묶음 A~H 바이트 확인(ICGuard 는 IC 안 오프셋이라 그대로).
- 27번 FogTrial: 0x7208c0(함수 못 맞춤), 음수 변위 3곳(-0x8a2140/-0x8741d0/-0x874650), [rip] terrain_bytes 는 모듈 안(무시).
- 22번 DomesticAI: 노드 함수 0xb1aa0→0xb2af0 이 바뀐 함수 → 인자·구조 그대로인지. 13번 F_CONFIRM 0x6ce030 내부(바뀐 함수) 두 자리 0x6ce465/0x6ce871 바이트 같음 확인됨, 의미 확인.
- 15번 RelatedOfficersTab HoverPin 0x2dc2a2/0x65e490(못찾음), 17번 Stats200Reader 0x21064e(함수 못 맞춤), 11번 3D.
- 12번 안개 1.6(SAN14_FogEngine121.dll)은 남의 모드(우리 payload 아님): domestic_talent/death_log 의 fog16 해시는 그대로(모르는 판이면 안전하게 비켜감).

## 2-3 진행 (2026-11-05 저녁)
- **IC(통합편의) 자료 표 이식 누락 발견·수정**: apply_new 는 코드 즉시값·훅표만 바꿨고 .rdata/.data 표(0x92a 확인표 2346·전역 호환표 0xdd240 174·포인터표·호출표·기능 서술자 {rva,len,buf1,buf2}·CAP 0xc5dd0)가 옛 값 그대로였음.
  → upd_tools/ic_port_r43.py(1.0.0→1.0.1 때 ic_port2.py 역할) 로 전부 번역. 확인: upd_tools/ic_verify_r43.py(IC 시작 검사 흉내) 통과 3061 / 실패 0. IC 해시 9e061ad6….
  · 바뀐 함수 안 작은 자리는 문맥 창 점수(map_site_fuzzy)로 77곳, 확인표 전용 재표본 96곳, 전역 검사표 재표본 3곳(0x7e3b70·0x29ede0·0x2b0290), 0x3d5cd0 은 게임 상수 0x1c5→0x1c6 만 다름(기대 바이트만 새 값).
  · **손으로 남은 것: IC '7번 포로 처우' 묶음** — 게임이 포로 처리 함수(옛 0x2e0d24 → 새 0x2e3054, 1851→1990바이트)를 고쳐 IC 가 바꾸던 34바이트 자리(0x2e110c: cmp/je/call 0x2337b0/test/je/call 0x3f7fa0)가 새 코드에선 호출이 둘로 늘어남(r15·r14 두 대상). 지금은 안전 실패(서술자 기대 바이트 옛 것 유지 → IC 가 그 묶음만 건너뜀). 설치기 0x18003da10 묶음(검사 0x18006e470·0x18003d100·0x18003e0d0) 전체가 꺼질 수 있음 → 2-4 에서 확인·이식.
- 다른 바이너리 모듈 자료 구역: upd_tools/apply_data_new.py(근거 강한 것만: guard 표·절대 포인터·자리_대응 값) → FogTrial 40, Traits9_UI 215, TraitHover 145, FPS120 8, CityCaps 3.
- traits9_check.cpp kUiSha(Traits9_UI 해시)가 2-1 때 안 바뀌어 9칸 요약 화면 보강이 통째로 꺼질 뻔 → update_hashes.py 에 포함, 거부 길마다 R43F 창 추가.
- 11번 3D: upd_tools/field_port_r43.py(newmap 기반) → field_src_r43, guards 177·vtables 27 전부 새 exe 확인, registry_bridge IC r43 해시·IC+0x4940 바이트 갱신, exe 해시는 **실제 파일(new.exe 782cf229…)** 로(dec.exe 해시 아님). 빌드 field_out_r43/{normal,fog} → payload f11 에 normal 배치(최종판에 12번 안개 1.6 없음).
- newmap.py 보강: 명령 단위 마스크(imask, 역어셈블 pkl), 주소꼴 즉시값 가림, vtable 칸 내용 대응(vt_map/map_slot), 잎 함수 탐색, 점프표 꼬리 허용(equiv_block), 레코드 표 포인터 내용 대응, 덩어리 중간 정렬(_align).

## 2-5 첫 PC 실행(2026-11-05 밤) → R43-MC1 → 바이너리 모듈 자료 구역 3차 점검
- **R43-MC1(리졸버가 게임 함수 지문을 못 찾음)**: ModCore 리졸버(SAN14_Resolver_IP01/CEKO.dll)의 지문 표(0x220 항목 140개: total,deref,reloff,add,core_off,core_len,패턴,마스크)를 새 exe 로 재생성하지 않았었다
  (1.0.0→1.0.1 때 port_resolver.py 가 하던 일). → upd_tools/resolver_port_r43.py: 옛 exe 유일 일치 → newmap → 새 바이트로 패턴 교체 → 새 exe 에서 core 유일 확인. 140 중 93 재생성, 실패 0.
  · 같은 종류 표가 **두 개 더**: 출진 프리셋 리졸버(f16 SAN14_Resolver_PresetGroups_CEKO.dll, 0x240 항목 7개: u32 total,core_off,core_len,flag / 패턴 @0x10 / 마스크 @0x128) 7 중 5 재생성,
    관련 무장 탭 리졸버(f15 SAN14_RelatedOfficersResolver.dll, 0x190 항목 14개: u32 total,core_off,core_len / 패턴 @0xc / 마스크 @0xce) 14 중 10 재생성. 둘 다 실패 0.
  · f15 리졸버는 표 말고도 .rodata 에 44바이트 코드 조각(call rel32 포함)을 두고 게임 .text 에서 '정확히 한 번' 찾는다(0x1120) → 새 바이트로 교체(0x2dc352→0x511302).
- **느슨한 지문·기록 칸 점검(upd_tools/loose_snippets_r43.py, record_fields_r43.py)**: apply_data_new 의 '강한 근거' 조건이 너무 좁아 남아 있던 것들.
  · loose: 자료 구역에서 옛 exe .text 바이트 열(≥6)을 찾아 유일 자리에 맞춰 앞뒤로 넓히고(진짜 지문 범위), 새 자리로 바이트 교체 + 그 지문을 가리키는 u32 RVA 칸을 함께 번역(이미 바뀐 칸은 newmap 과 대조).
  · record: ① u64 절대 포인터(0x140000000+RVA) ② 코드 RVA 뒤 64바이트 안에 그 자리 옛 바이트(≥5)가 있는 기록 ③ 근거 칸 ±32 안의 함수 시작 RVA·4배수 자료 RVA(map_data) ④ kind2(vtable 칸 + 기대 포인터) 일관성 확인.
    cc 동굴 기대 바이트는 새 자리도 전부 cc 일 때만. 리졸버 지문 표 구역은 둘 다 건너뜀.
  · **apply_data_new 버그**: {u32 rva, u32 1} 짝을 '절대 주소'로 보고 u64(0x140000000+n)로 써서 Traits9_UI 35·TraitHover 35칸이 0x405b6e10 같은 값이 됐었다 → 되돌리고(u32 만) 소스 고침.
  · newmap MANUAL 추가: 0x401c0→0x58d90(pdata 없는 작은 함수, 호출자 20/20·14바이트 유일), 0x415aa2→0x415bc2(함수 끝 뒤 14바이트 cc 동굴), 0x91c140→0x91e670(vtable 18칸이 가리키는 thunk; 게임이 함수 뒤로 옮김).
  · 17번 Stats200(능력치 200) 10개 패치 기록 중 5개가 게임이 다시 짠 함수 안 → extra_bin_fixes.tsv 로 손으로(호출자·유일 14바이트로 확인). **0x230098→0x231afb 는 새 exe 에서 je 뒤 nop 이 3바이트라 8바이트 패치가 안 들어감 → 미적용(게임 로그 XP_GROWTH_NOT_APPLIED). R43 알려진 제한.**
  · 결과(모듈별 바뀐 칸)는 newpkg/payload_new/느슨한지문_적용_보고.txt·기록칸_적용_보고.txt.
- 교훈(회귀 목록에 추가): 판 이식 때 **바이너리 모듈은 코드 즉시값만이 아니라 (1) 리졸버 지문 표(형식 3가지) (2) 자료 구역의 기대 바이트·RVA 기록 (3) 절대 포인터 (4) 'x?' 마스크 패턴**까지 전부 새 exe 로 검증해야 한다. 첫 실행 전에 ic_verify 처럼 모듈별 '시작 검사 흉내'를 돌릴 것.

## 2-5b 둘째 PC 실행(2026-11-06 21:37) → 시험3 판 (IC 9fa16f76… / manifest d4511586…)
- 튕김 2(21:38:48, 미니맵 경로 → IC+0x3afb1 → 게임 0x1873118 실행 오류): IC 0x26b92 `lea rdx,[rsi+0x14769e8]` — rsi 는 **인자로 받은 게임 base**(rcx→rsi 복사). imm_port 는 base 출처를 `mov R,[rip+slot]` 로만 봐서 놓침.
  → **upd_tools/base_flow_r43.py**: 함수(pdata) 단위 레지스터 흐름 추적. def-site 마다 그 레지스터로 만든 주소의 '확실한 옛 주소' 점수(함수 시작·자리표(모듈 이미지 밖)·옛 코드가 참조하는 자료·기대 바이트(0/0xff 제외))로 base 판정, 복사(mov R2,R1)·slot 전파. 모듈 자기 base(`lea R,[rip] #0x180000000`)·스택(`lea R,[rsp±]`)·IAT/자기 rip 참조 변위를 쓰는 레지스터는 제외.
  IC 40칸(+복원 6), FogTrial 27칸(카메라 검사 0x66f94c~ 포함), CityCaps 3칸.
- **장부 오염**: 자리_대응(SITE)에 IC 내부 주소 0x105040(게임 base 칸!)·0x1a3ed0·0x19e6a0 가 섞여 apply_new 가 IC 코드 변위 6곳을 '번역'해 놓았었다(p1fix 의 `mov rsi,[rcx+0x105040]` → 0x106020 — 모드 창 안 뜸 의심). base_flow 가 복원. 교훈: 모듈 이미지 범위 안 값은 자리표라도 강한 근거로 안 씀.
- 9번 Traits9_UI 'code signature mismatch': 기대 바이트가 **Traits9 패치 뒤 모습(09)** 인 기록 5개(0x4a6cf3 등)는 옛 exe 와 안 맞아 record_fields 가 못 봄 + 자료표 0x13fd6a0 → **upd_tools/patched_records_r43.py**(≤2바이트 다른 기록 번역). traits9_check.cpp kSites 도 같은 5곳 옛 값 → 고침.
- 13번 MeritTraits 소리 없이 꺼짐: 리졸버 verify_row_identity 의 `base+0x89abe0u/0x8a49a0u`(표 밖 상수) 옛 값 → resolve() 0. 추가로 hover_x_adapter.inc XC(0x92cee0/0x85c9e0/0x857370), native_rules.inc 코드 해시 8개(새 자리 바이트로 재계산; 명령열 동일 확인, edit_exclusion 함수는 지역변수 초기화 4줄만 추가), hover_text_hash(최종 Traits9_UI .text 270f117d…) → mt/build_new.sh 재빌드. gen_merit_resolver_r43.py 에 `base+0x…u` 규칙 추가.
- 14번 TraitHover: 설치기는 9/13 과 함께면 **`SAN14_TraitHover.dll.v2`** 를 씀 — apply_new 가 `.dll` 만 처리해 .v2 는 옛 판 그대로였음(PE 시간표 검사로 'unsupported game build'). 임시 폴더에서 전체 파이프라인 돌려 교체. 두 변종 모두 기록 칸 0x807de6→0x80a4f6, 0x806d70→0x809480 손으로.
- 19번 FPS120 'atomic patch transaction rejected': 호출 자리 기대 바이트가 **movabs imm64 안에 e8+rel32 로 포장**(0x956f3a·0x44df40·0x153247·0x154e2a) + 중계 거리 음수 즉시값(-(site+5), 0x80000000-(site+5)) 5곳 → 손으로(extra_bin_fixes.tsv 메모).
- 23번 CityCaps: 새 판이 도시 상한 표(0x1a4d250, 0x40바이트) 값 5칸을 바꿈(1000→30,100→47,1500→100,2→15,2→20); 상한 3칸은 그대로 → 기대 표를 새 exe 바이트로.
- 소스 점검(newmap: 옛 주소 같고 새 주소 같지 않은 상수): scene_skip 카메라 목록 39곳+ret 0x45141f→0x45362f, execute_lock VT_TREAT 0x1430f80, region_officer VT 4개+0x1a29e60, city_event_skip 0x1d3110, traits9_check kSites. 장부(연결자리_대장)에 없던 상수들.
- Diag 규칙: DomesticTalent 성공 문구는 안개 없을 때 훅 줄('SAN14CE.DomesticTalent RVA=')뿐 → 규칙 교체; ExtBridge 'dispatch finished'.
- 11번 3D RUNTIME_GUARD: 코드 177·vtable 26 통과, IC 해시만 불일치(IC 가 바뀔 때마다 registry_bridge.h 갱신 필요) → 최종 IC 로 재빌드. **순서: IC 확정 → update_hashes → build_all → field_build → assemble → overlay.**
- 검증 도구: upd_tools/verify_expect_r43.py(모듈 자료 조각·RVA칸·cmp 를 새 exe 와 대조, numpy 8바이트 색인). 남은 불일치는 전부 설명됨(짧은 우연 일치·IC 7번 포로 묶음·AISearch 29바이트 패턴은 런타임 정상).
- 기록 위치: 게임이 쓰는 로그는 게임 폴더 루트·%LOCALAPPDATA%\SAN14CE_ModCore\runtime.log; SAN14_기록\ 의 사본은 **설치기가 설치 때 복사한 스냅샷**(튕김_<설치시각>.log 포함) — 분석은 루트/LocalAppData 를 읽을 것.

## 2-5c 셋째 PC 실행(2026-11-06 23:40/23:43, 시험3 판) → 시험4 판
- 로그 사실: IC 자체 기능 8개 실패(BGM 플레이어·진행속도 확장·BGM 확장·모드 설정 v2·정보 메뉴 모드 설정·도시 증축 암전·명승·논공 동시(줄 자체가 없음)), 27번 미니맵 "camera move code differs", 논공 버튼 튕김 = exe+0x418e31(시험3 에서 이미 고친 Traits9_UI 코드굴, 미전달).
- IC 디스패처(0x18004b5a0~) 구조: 공용 표 0x92a개(sil) → 기능마다 '준비 함수(0x180050e80 등)' + '설치 함수(0x18003c000 등)'. 설치 함수 대부분은 준비 함수가 채운 기록(원본 바이트는 **게임에서 읽어 둠**)만 쓰므로 정적으로 안 깨짐. 깨지는 건 **공용 검사 함수(0x18006d9b0 BGM, 0x18006ec40 진행속도/0x1800580e0 짧은 비교, 0x18006e700…)** 와 '스택에 즉시값으로 만든 기록'.
- 고친 것(전부 IC 76be24ab…):
  · 0x9b2d4 5바이트 `e9c48dffff→e9148effff`: 0x18006ec40 이 game+0x89abf7 원본 jmp 5바이트를 rip 자료와 바이트 루프로 비교(8바이트 앵커인 verify_expect 가 못 봄) → 진행속도 확장(및 0x18003ed90) 실패 원인.
  · BGM vtable 칸 4개: 0x51c17/0x51c29/0x51c3b/0x51c4d `mov DWORD PTR [rbp±],imm`(스택 즉시값, imm_port 대상 아님) 0x140d1f8/0x140d000/0x140d008/0x140d020 → 0x1410288/0x1410090/0x1410098/0x14100b0. 0x6dad5 lea 는 자리_대응이 **0x1410000(+0x3000 page fix)** 으로 잘못 줌 → 새 exe 에서 0x140608a50 을 담은 칸은 0x1410090 유일.
    **주의**: 0x140d000 은 '옛 0x1409f70 의 새 자리'이기도 함(.rdata 슬롯표 0xa2320/0xdd690 두 벌) → extra_bin_fixes 전역 치환으로 6칸이 깨졌다가 복원. 같은 숫자가 옛/새 양쪽에 있는 값은 tsv 전역 치환 금지(주석 처리).
  · 0x13d0e80→0x13d3f10 3곳(0x511d4/0x576d5/0x6dc4d): BGM 곡 ID 표(0x75개, 0x1a63020 표와 대조). base 가 `lea rbx,[rax+r14*4]` 계산값이라 base_flow 밖.
  · 논공 동시 v2 기록(0x3c7f4/0x3c804/0x3c874 `mov QWORD PTR [rbp-..],imm`): 0x6c41ff→0x6c695f, 0x709733→0x70ba43, 0x1447e90→0x144af18.
  · 27번 FogTrial cm_install: 컴파일러가 32바이트 비교를 `or r8,0x451361/2/3` 로 풀어 둠(base 0x453570 은 번역, or 상수는 그대로) → 0x453571/2/3.
- 정적으로 전부 통과 확인: 기록표 423·슬롯표 467·{rva,u32,u8} 표·인라인 0x2d 표·짧은 비교·vtable 칸 검사(upd_tools/ic_tables_r43.py, slot_cmp_r43.py, memcmp_sites_r43.py, small_blob_r43.py, imm_audit_r43.py). 모드 설정 v2 사슬(0x3ce20→0x3f1b0→0x3e490→0x3fe70→0x3f5b0→0x3da10)은 표가 다 맞음 — 다음 실행 로그로 확인할 것. 중계 메모리 할당(0x180023780)·코드 재배치(0x1800320f0) 쪽은 정적 확인 불가.
- 11번 3D: field_src_r43 game_guards.h(177+27)·registry_bridge.h 상수는 새 판 값으로 확인(0x13a3620/0x13a3648/0x1c68a8/0x1cc89b/0x379062/0x1a41270 전부 새 exe 와 일치). 남은 변수는 IC 해시(이번 판 76be24ab… 로 재빌드)와 IC 고유무장 등록 성공 여부(flag IC+0x1c1f8d) → 다음 FieldLeader3D_Loader.log 확인.
- 새 교훈(회귀 목록): (i) 스택/레지스터에 즉시값으로 만든 기록(`mov [rbp-x],imm`, `mov r32,imm` 뒤 `add base`) (ii) `or reg,addr|k` 풀린 루프 (iii) 8바이트 미만 기대 조각 (iv) 계산된 base(`lea R,[rax+r*4]`) 뒤 변위 (v) 같은 숫자가 옛/새 양쪽 주소인 경우 전역 치환 금지.

## 2-6 넷째 PC 실행(2026-11-07 04:14, 시험4 판) → 시험5 판
- 실제 실행 로그로 확정: 시험4 에서 BGM 플레이어·진행속도·BGM 확장·논공 동시 **해결됨**(로그에서 그 실패 줄 사라짐). 남은 실패: 모드 설정 v2 사슬 4개(모드 설정 v2/정보 메뉴 모드 설정/도시 증축 암전/명승) + 27번 "camera move code differs" + 3D RUNTIME_GUARD + TraitHover.
- 모드 설정 v2 원인 찾음 = **IC 0x18003fe70 의 {slot,target} 검사표(0xe2770~0xe2b10) 34칸 미번역**. 패턴: .rdata 슬롯이 .bss(0x1a3fxx) 를 가리키고 그 .bss 가 vtable(0x13bxxxx)을 가리키는 2단 포인터. apply_new/base_flow 가 양쪽 다 옛값으로 남겨 둠. 번역: rdata→bss 는 자리_대응, bss→vtable 은 '이미 번역된 같은 표의 쌍'으로 역산 → 34칸 모두 새 exe 와 일치 확인(ptr_pairs_r43.py). upd_tools/ic_e2770_fix_r43.json + cell_fixes_r43.py.
- 3D installed() 원인 = **IC 고유무장 등록 함수 0x18003dee0 가 못 돌아 flag(IC+0x1c1f8d) 안 섬**. 그 함수의 재지정 변위 상수 0x18003e00f `add eax,-(0x1a3e290+7)` 가 음수로 인코딩돼 도구가 못 봄 → -(0x1a41270+7). 고치니 등록 성사 예상. (가드 표·레지스트리 상수는 이미 전부 새값 확인.)
- 27번 camera = **컴파일러가 바이트 비교 루프를 `or r8,base|1..3` 로 풀어 둔 것**. 새 base 0x453570 은 4번 비트가 켜져 있어 `rcx|0x453571` ≠ `rcx+0x453571`(0x10~0x1c 칸 오독). or→add 로 바꿈(0x331a2/b8/ce). feed90 설치(0x1e300~) xchg rel32 5곳·카메라 vtable 끝검사(0xe564)도 음수 상수라 같이 번역.
- TraitHover: (i) 반환주소 재기록 4곳 `lea ecx,[rax-0x8070b6]` 음수 상수, (ii) {kind,slot,target} 콜백표의 **target 열(0x92ad0)만 옛값으로 남은 39칸** → 0x8ca1c0, (iii) 기대 코드 기록 rva 0x807db6. 콜백 slot 열은 이미 OK 였는데 target 열은 별 경로라 누락됐던 것.
- 새 공용 도구: upd_tools/neg_imm_r43.py(음수 인코딩 주소 상수), ptr_pairs_r43.py(포인터 쌍 64/32비트 전수, 실제 역참조 검증), slot_pairs_r43.py(보조), ic_tables_r43.py/slot_cmp_r43.py/memcmp_sites_r43.py/small_blob_r43.py/imm_audit_r43.py/same_value·patched_records·verify_expect. **ptr_pairs 전 모듈 불일치 0** 확인.
- 회귀 교훈 추가: (vi) 음수(2의 보수)로 인코딩된 주소 상수 — `add/lea/mov r,-K`; (vii) 2단 포인터 검사표(rdata→bss→vtable) 양쪽 다 번역 필요; (viii) {kind,slot,target} 표의 target 열은 slot 열과 별도 경로 — 둘 다 확인; (ix) `or r,base|k` 는 base 의 하위 비트가 1이면 += 로 못 바꿔 쓰니 add 로.
- **여전히 손 못 댄 1곳**: 17번 능력치200 XP 성장 분기(새 exe 0x231afb). 옛 자리는 je+nop6=8바이트라 8바이트 패치가 들어갔는데, 새 exe 는 je 뒤 nop 이 3바이트뿐(총 5바이트)이고 그 다음이 살아 있는 명령(0x231b00 mov rdx,[rdx+8]). 8바이트를 쓰면 그 명령을 깨뜨림. 바이너리 이식본이라 5바이트 점프+릴레이로 바꾸려면 모듈의 패치 엔진 자체를 손봐야 해서(라이브 인물편집 순회 경로) 블라인드로 바꾸면 세이브/편집 손상 위험 → 이번에도 안전 실패 유지(그 1개 항목만 네이티브, 나머지 Stats200 전부 적용). 엔진까지 손보는 건 사용자 확인 후.
- 판: IC eb8d71631828b628, overlay manifest 33bf4910d0c5e359, 62파일 R43_시험판에 기록. 소스 ZIP 작업재료에 R43시험5 로 보관.

## 2-7 시험6: 17번 능력치 200 을 소스 모듈로 새로 짬 (사용자 결정: 이식 대신 새로)
- src/stats200_new.cpp + stats200_new_bridge.S → SAN14_Stats200New.dll (f17). 옛 이식본 SAN14Mods/ModPack/SAN14_Stats200.dll·SAN14_Stats200Reader.dll·Stats200Reader.ini 는 f17 에서 뺌(build_overlay_r43 REMOVED 목록 → manifest 제거 + Known, 설치기가 state.xml 대로 지움). 설치기: ModPackBridge ini Stats200=0, Judge 17 새 문구, Tests 갱신. Diag/feature_table: Stats200New.ini.
- 자리 15곳: 1.0.0 R6 명세(build_tc/src/stats200_direct.cpp) 의 문맥 48바이트로 새 exe 에서 다시 찾음(fuzzy 앵커+점수). 이식본은 10곳만 했는데 R6 의 14+1 전부 새 exe 에 있음(0x226e2a 0x226e7f 0x22ae26 0x22ae99 0x23c458 0x23c4b6 / 0x20b7c5 0x20bfda 0x211f0d / 0x5dfd18 0x5dfb83 / .data 0x142fc70 / 0x321f3e / 0x2bc14e / XP 0x231afb).
- XP 성장 분기: 새 exe 는 je(2)+nop(3) 뿐 → 5바이트 jmp → XpBridge(jne→계속 / cmp esi,100; jl→esi=100 경로 / 아니면 계속+카운터). 뒤 명령(0x231b00) 안 건드림. 플래그는 cmp ecx,0x7b 것 그대로(ModCore 릴레이 jmp [rip] 무해).
- 묶음 2개(Caps 14 전부-아니면-전무, XP 1) 따로 실패 보고(r39::fail_bytes/fail_plan, R43F). ModCore 거절 시 상수는 직접 쓰기(R6 규칙).
- 검사: build_tc/host/test_stats_new.cpp — 실제 새 exe 평탄 이미지(/tmp/new_img.bin, newmap 으로 생성)에 모듈을 돌려 17/17 통과(15자리 변경·5바이트만 씀·재초기화 무해·릴레이 3경로·불일치 보류·거절 시 직접 쓰기). winstubs.h 에 exeHandle 추가.
- 판: manifest ce77e84c93c22899, 62파일. PC 브리지가 끊겨 아직 R43_시험판에 못 씀 → 연결되면 커밋.

## 2-8 시험7: 개성 탭 튕김 + 같은 부류 전수 (2026-11-07 13:11 실행 로그 기준)
- 튕김: SAN14CE_KO.exe+0x231d24 ← Traits9_UI+0x2fd5. Traits9_UI 0x180002fa0 이 `cmp r8d,1; sbb; and r8d,D; lea rax,[rax+r8+A]` 로 두 게임 함수(A / A+D) 중 고름. A 는 번역(0x210a60)됐는데 **차이 D 0x212a0 옛값** → 0x231d00 엉뚱한 곳 호출. D=0x215d0(새 0x232030-0x210a60). 회귀 (ix) 두 주소 고르기 차이 상수.
- 같은 부류 전수: upd_tools/delta_sel_r43.py(lea 형, 1건=위), split_cmp_r43.py(새 회귀 (x) switch 이분탐색 경계 = 다음 주소-1 / 구간 끝값):
  · TraitHover(.dll·.v2) 0x3b55/0x4cdc `cmp ...,0x1460017; jg` → 0x14630a7 (콜백 칸 0x1463090 이 엉뚱한 갈래).
  · IC 0x3f435 `cmp rcx,0x4562a0` → 0x458490 (0x45847c 의 0x14바이트 창을 덮는 기록 찾기 끝값 = 시작+0x14).
  · FogTrial c48_code_matches 0x3504f/0x35089/0x350c3 `cmp r14,0x29fefff`(옛 이미지 크기-1) → 0x2a01fff.
  · stale_records_r43.py: TraitHover 기대 코드 기록 rva 6개(0x807db6,0x8080a0,0x8080d0,0x806d60,0x8070c0,0x8070d0) 새 자리로.
- 13번: Diag 규칙 rt_bad "MeritTraits: " 가 정보 줄에도 걸려 거짓 R43D-1 → "merit awards not enabled". 리졸버 hover_text_hash 를 새 Traits9_UI .text(ce94ab86…)로 — update_hashes.py 5단계로 자동화(Traits9_UI 고칠 때마다 같이 바뀜).
- TraitHover 콜백 표 0x98칸 정적 검사: 새 exe 칸 내용 = 기록 target 전부 일치(불일치 0). 13:11 로그의 콜백 38줄은 이전 판 것으로 판단 — 다음 로그로 재확인.
- IC 바뀜 → a89249b7…: common.h r43·registry_bridge.h(3D) 갱신 → build_all → field_build → mt → assemble → overlay. 3D Runtime 에 새 IC 해시 들어간 것 확인.
- 판: manifest 8e64a2e1c5953549, 62파일 R43_시험판에 기록.

## 2-9 시험8: 종합 진단이 '자기 기록에만 실패를 적는 모듈'도 창으로 (사용자: 모드 버튼 안 뜰 때 오류창 없이 조용했음)
- 원인: 통합편의는 "모드 설정 v2 설치 실패" 등을 SAN14CE_11Patch.log(UTF-16)에만 적음. Diag 는 통합편의를 '시작 줄'만 봤음. TraitHover·Traits9·미니맵·DragGive/DragLearn/WheelRegion 도 자기 .log 에만 적음.
- src/diag_ownlogs.h(새, Windows 없이 시험 가능) + diag.cpp ⑤ check_ownlogs: 30/90초 확인 때 한 번씩만 읽음(계속 읽지 않음).
  · 통합편의: 마지막 '지원 실행 파일 확인 완료'(≥ 이번 실행) 뒤 줄 중 실패|미적용|못했|못 찾 → 기능 번호(모드 설정·정보 메뉴=1, 자동인사·논공=2, 문전박대=3, 증축·명승·보고·포커스=4, 진행속도·수행·대열=5, 인재·포로·등용문=6, BGM=7, 개성 9개=9, 일기토·고유무장=10). 제외: '무장 원본 직접 읽기'(매번 오류 1), 9번 미설치 때 'IC↔9 연결 실패'.
  · 실행마다 새로 쓰는 기록(수정 시각 ≥ 이번 실행): SKIPPED/FAILED/failed/WARNING/differs/not found/unsupported/... 줄. 'panel off' 같은 정상 줄은 안 걸림.
  · TraitHover(이어 씀): Diag 시작 때 크기 뒤로 붙은 줄만, 이유별로 묶어 개수(duel skipped 제외). 시작 전에 이미 써졌으면 SAN14_기록\Diag_state.ini 의 지난 실행 끝 크기.
- 검사 build_tc/host/test_diag_ownlogs.cpp: 실제 PC 기록으로 15/15 — 13:06 실행 0건, 04:14 실행 4건(1번 모드 설정 v2·1번 정보 메뉴·4번 증축·4번 명승), 11-06 21:37 실행 BGM(7)·진행속도(5) 포함 8건, TraitHover 콜백 38줄=1건(x38), 정상 기록 6개 거짓 경보 0.
- Diag 가져오기 함수 추가 없음(FindFirstFileW·MultiByteToWideChar·WritePrivateProfileStringW 기존 것). 판: manifest 0cf2b773b09f9d15, 바뀐 파일 4개(Diag.dll·manifest·ManifestHash.cs·SHA256SUMS).

## 2-10 시험9: 턴 진행 튕김 + 34번 공석 표시 + 13번 꺼짐 + 증축 연출 (2026-11-07 14:13 실행 로그)
- 턴 진행 튕김 exe+0x44c57a(CUserStrategyState::enter 0x44c340 안): 통합편의 0x44e2c1(45바이트) 후크의 trampoline 이 '되돌아가는 jmp' 대상을 `sub rdx,r8` 로 바꾼 base 에서 `lea rax,[rdx+0x44c569]` 로 만듦 → 옛값 그대로라 새 판 0x44c569(엉뚱한 함수 한가운데)로 뜀. 0x44e7a9(옛 0x44c081→0x44c569 와 같은 자리, 바이트 대조 OK)로. 범위 검사 `mov eax,0x8044b5b9` → 0x8044d7f9. 회귀 (xi): base 를 바꾼 뒤 쓰는 상대 주소 상수 — upd_tools/rel_base_r43.py(새).
- 34번 지역담당관 공석(빨간 이름) 안 됨: region_officer.cpp VT_CAPTEXT/VT_FOOTSIMPLE 가 엉뚱한 칸(0x139fef8/0x139f188) → RTTI 로 0x139ceb8(CCapitalTextLayout)/0x139c148(CFootholdInfoLayoutSimple). upd_tools/rtti_r43.py(새: RTTI 이름↔vtable 표).
- 13번 꺼짐("loaded Hover engine is not a supported binary"): 시험7 에 고친 hover_text_hash 를 mt/src/merit_resolver.c 에 넣었는데 mt/build_new.sh 가 mt/merit_resolver.c 를 src/ 로 복사해 덮어씀 → 옛 해시로 빌드됨. 원본 mt/merit_resolver.c 고침, update_hashes.py 5단계 대상도 그 파일로. 빌드 결과에 새 해시 확인 + Traits9_UI .text(0x1000,0x5af8) 해시 일치 오프라인 확인.
- 25번 Patch27 증축 연출 생략: city_event_skip.cpp kSkipVt 38개·kNarrationVt 가 1.0.1 값 그대로 → RTTI 이름으로 새 판 vtable(38개 전부 하나씩 대응).
- 우리 소스 전체 RTTI 대조(옛 소스 상수의 클래스+칸 = 새 상수의 클래스+칸): 위 둘 외 불일치 없음(시험용 scene_test·patch_tester 는 배포 안 함).
- IC 604e2c0f… → common.h r43·registry_bridge(3D) 갱신, 전체 재빌드. 검사: split/stale/delta/rel_base 0건, Diag 기록 15/15, 17번 17/17.
- 판: manifest 784c4ba44a95a7d5, 바뀐 파일 12개 R43_시험판에 기록.

## 2-11 시험10: 1 FPS + 옛 판 주소 전수 조사
- 1 FPS(전략 화면 한 프레임 ~700ms, MinimapPerf state=CUserStrategyState): 시험9 에 통합편의 0x44e2c1 후크의 되돌아가기를 옛 판과 같은 상대 자리 0x44e7a9 로 맞췄는데, 그 자리는 `cmp [rsi+0x484],0x20 / jl 0x44e1f8` 되돌이 고리 → 통합편의가 0 을 돌려줄 때마다 고리를 다시 돎. 원래 게임의 건너뛰기 자리 0x44e7ba(블록 안 jne 대상)로 바꿈(범위 검사 0x8044d80a). 원래 게임보다 더 도는 일 없음.
- 전수 조사(upd_tools/census_idx_r43.py·census_r43.py·census_filter_r43.py·census_delta_r43.py + 썽크 검사):
  · 대상 28개 이식 모듈의 '바뀌지 않은' 모든 즉시값·변위(실행 구역 전부: .text·.p1fix·.compat·.s14sw)와 자료 구역 전부(.rdata·.data·.rodata·.s14sync 등)의 정렬 4/8바이트.
  · 꼴: 그대로 / +1(경계) / 0x80000000+ / 음수·음수-k / 0x140000000+ ; 증거: 옛 exe 함수 시작·분기 대상·vtable 칸·rip 참조 자료·jmp 썽크(18350개).
  · 1차 826 → unwind 281 제외 → 코드 틀(새 exe 와 바이트 일치) 71 제외 → 나머지 줄마다 판정. 모듈 자기 것(임포트 이름표·EH/unwind·.s14sync=통합편의 자기 pdata 사본·RTTI), 글자(한글·ASCII·서명 문자열), 구조 오프셋, 표 크기(0xbd10×32), 비트 플래그, FNV 상수(0x1000193·0x811c9dc5), 시간 제한(0x1d4bf) 등 = 손댈 것 아님.
  · 진짜 옛값 8곳(전부 27번 FogTrial, '레지스터에 rva 를 넣고 game 을 더해 call/jmp'): name_restore 0xac9aa0→0xacc350, mini36_snapshot 0x2318b0→0x233310, terrain_build 0x14cb00→0x14dae0, 옛 썽크 0x450f80(3곳)→0x876580, 0x9246b0→0x87a690, 0x924fd0→0x87abc0. 머리 16바이트 대조 OK. cell_fixes_r43.py 에 기록.
  · 고친 뒤 다시 돌려 FogTrial 코드 후보는 전부 위 '손댈 것 아님' 꼴만 남음.
- 우리 소스(upd_tools/src_census_r43.py: 옛 소스와 숫자 단위 정렬, 안 바뀐 숫자의 옛 주소 증거 + 바뀐 숫자의 대응/RTTI 대조): src(배포분) 4건 모두 통합편의 모듈 오프셋·db 구조 오프셋(정상). 13번 리졸버(merit_resolver_101.c ↔ merit_resolver.c) 1건 구조 오프셋. mt/src 의 XC 주소는 새 exe 함수/명령 자리 확인. 3D(field_src ↔ field_src_r43) 2건 자기 DLL export rva. 시험용(scene_test·patch_tester)은 배포 안 함.
- 판: manifest a8c36c2620bae484. 1 FPS 수정판(IC e1215cc0…)은 먼저 R43_시험판에 기록, 이어서 FogTrial 8곳.

## 2-12 시험11: 프레임 1~2 진단 표본기(Diag)
- FogTrial 표본: 메인 스레드 game 0.5%·ntdll.dll+0x160000 쪽 99% → 누가 부르는지 모름.
- src/diag_perf.h(순수 로직, 호스트 시험 /tmp 통과) + diag_perf_win.h: SAN14_Minimap.ini 에 Profile=1 일 때만. 게임 창 스레드를 ~1ms 마다 Suspend→GetThreadContext·VirtualQuery·스택 32KB 복사→Resume(멈춘 동안 잠금 잡는 함수 안 부름). 복사본에서 call 뒤 주소(모듈 읽기 가능 구간만)로 첫 비시스템 호출자·첫 모듈 호출자·(첫,둘째) 쌍, RIP 정확값(+ntdll export 이름, r10/rdx), 스레드별 CPU(합계/커널)·시작 주소. 창 뜬 뒤 5·95·185초에 30초씩 3회 → SAN14_기록\Diag_Perf.log, 끝나면 멈춤.
- 판: manifest 849214f8654f5dc3, 바뀐 파일 4개(Diag.dll 38f93f02…·manifest·ManifestHash.cs·SHA256SUMS).
- 시험11 1차 결과(15:12 실행, 로딩·전략 화면): 메인 스레드는 CPU 1.4s/30s, 대부분 Sleep(ZwDelayExecution) ← exe+0xc00231 ← exe+0xb3e492(일 끝나길 기다림). 진짜 범인 후보 = 게임 작업 스레드 tid 76204(시작 exe+0x1033ca8) CPU 29.7s/30s 중 커널 29.5s. DomesticPlayer 스레드 커널 0.8s/30s(작음).
- 시험11b: 표본기를 '메인 + 직전 1초 CPU 상위 3개 스레드'로(Set 별 표). Diag 257370c7…, manifest bed3d49f1401bb66.

## 2-13 시험12: 1~2 FPS 원인 = 34번 지역담당관(RegionOfficer) — 고침
- 시험11b 표본(15:18): 게임 작업 스레드(이름판 갱신, 메인이 Sleep(1)로 기다림) 표본 70%가 ZwQueryVirtualMemory ← ModCore Readable(+0x1042) ← RegionOfficer child_ok(+0x296e).
- 원인: RoFrame 의 '돌려쓴 판에 남은 빨강 지우기' 검사(child_ok+color_is = Readable 6번)가 빨강 아닌 모든 판에서 0.15초마다 → 판 343개 x 6 / 0.15초 ≈ 초당 1.4만 번 VirtualQuery. 시험9 에 VT_CAPTEXT/VT_FOOTSIMPLE 를 바로잡으면서 sub_ok 가 참이 돼 이 길이 처음 살아남(그 전엔 판정 전에 빠져나감) → 시험9부터 1 FPS. (시험10 FogTrial 8곳은 무관: 대상 함수 본문 일치 확인, FogTrial 자기 일 0ms)
- 고침: Ent.fresh — 판이 바뀐(changed) 다음 판정 한 번만 검사. 빨강 판의 1초 색 확인·칠하기는 그대로.
- 시험: test_region_officer_native(새 판 스타일 표 0x1a29e60 으로 시험 고침 + 판 343개 정지 상태 3초 Readable 0번, 돌려쓴 판 남은 빨강 1번 지움) 통과. test_diag_native 13번 기대 문구 시험7 규칙에 맞춤 19/19. domestic_ai·domestic_player·auto_merit 38·traits9 10·ICGuard 동굴 통과. 바이너리 모듈은 시험10 이후 안 바뀜(패키지 비교: 바뀐 파일 RegionOfficer.dll·manifest·ManifestHash·SHA256SUMS 4개).
- 판: manifest 7752f8e1555888cd, RegionOfficer 40de2de5…

## 2-14 시험13: 턴 튕김·진행 배속·CrashLog R3 (+ 편집 무장 개성/전법 조사 중)
- 시험12 확인(15:25 실행): MinimapPerf 한 프레임 2.7ms(전 690ms) → 1~2 FPS 해결 확인.
- 턴 튕김(15:28, 게임 폴더 SAN14_CrashLog.log): exe+0x44e7cb 널 쓰기. 시험10 에 통합편의 0x44e2c1 후크 되돌아가기를 0x44e7ba 로 바꾼 게 원인 — 0x44e7b0 `mov rdi,[rbp-0x11]` 을 건너뜀. 옛 0x44c569 와 바이트 같은 0x44e7a9 로 되돌림(범위 검사 0x8044d7f9). 1 FPS 는 RegionOfficer 였으므로 0x44e7a9 가 맞음. cell_fixes: 옛값 목록 허용(이전 시험판 값 되돌리기).
- 5번 진행 배속(통합편의 기록 매번 "원래 화살표 속도"): 통합편의가 읽는 '지금 진행 속도' 칸 0x1a49ca4 가 틀림. 구조체(게터 0x3ec1a0→0x1a49c90, 옛 0x3e9c90→0x1a46ca0)는 +0x2ff0, 필드 사용(0xe58·0x4c·0x10) 같음 → 0x1a49cb4. 4곳(0x32d7f·0x463d3·0x4a5f9·0x4a759). 새 검사 upd_tools/datashift_r43.py(자료 주소 주변 참조 무늬 정렬): 진짜는 이것뿐(나머지 db 루트 등은 경계 거짓 경보 확인).
- CrashLog R3(src/crash_log.c, src/crash_log_k32.def): 오류 등급 예외 전부 + int3 + C++ 예외(16개), 실행 시작 줄·정상 종료 줄. 이전엔 SAN14_기록 복사본만 봐서 15:28 기록을 늦게 찾음 — 게임 폴더 SAN14_CrashLog.log 를 먼저 볼 것.
- IC 해시 60bd7109… → common.h·registry_bridge.h, build_all·field_build·assemble·overlay. 3D 에 새 해시 확인. 네이티브 시험 전부 통과.
- 시험13b: Diag — 10번 미설치(게임 폴더 San14ModelRegistry.tsv 없음)면 통합편의 '일기토 연출 시험 호환성 검사 실패' 줄 무시(사용자 15:54 창). CrashLog R3 — 스택이 8바이트 어긋나도 4바이트 간격으로 흔적 훑기, rbp·rsi·rdi·r8·r9·r12~r15 기록, 어긋남 표시.
- 사실무장 편집 튕김(15:52): exe+0x92f39c(자식 칸 꺼내기) ← 0x494782 = CSaveLoadDialog vf7(0x4944d0, 슬롯 6개 만들기). rbx=0xa(호출 뒤 보존돼야 할 값이 깨짐)·rsp 가 8바이트에 안 맞음 → 정상 컴파일 코드로는 안 생김. 흔적이 짧아 원인 모듈 미확정 → R3 로 다시 잡기.
- 발견: 10번 제거(15:49) 뒤 게임 폴더 0002/KIDSSystemResource.rdb = 344f74…(1.0.1 원본). 새 원본 75ca69… 과 레코드 1개(0x2886f6d2: bin17@0x6c87 → 새 판 bin18@0x6c85, 0x3b794 바이트)만 다름. 설치기 ModelPatch/Unpatch 는 우리 것 아닌 레코드를 그대로 두므로 옛 값이 남음. 저장·불러오기 창과 관련 있는지 미확정. (C# 컴파일러가 작업 공간에 없어 설치기 수정은 보류)
- 백그라운드 조사(포크)는 15:52 작업 공간 재시작 때 죽었음 — 이어받아 직접 봄. 옛 공용 리졸버(Resolver_CEKO, 10-22)는 우리 모듈이 안 씀(MeritTraits 는 자기 리졸버, ModCore 는 IP01) 확인.

## 2-15 시험14: 편집 무장 개성·전법 사라짐 = 9번 콜라보 해금(Collab) db 오프셋 오역 — 고침
- 증거(세이브 직접 풀어 대조, /tmp/claude-0/pt/sv/dec.py): 1.0.1 때 세이브(10-30 KO09·11-04 KO02)는 관은병·조운·제갈량 개성 9개 있음. 새 판 세이브(오늘 KO03·KO05)는 개성 0 / 콜라보 전법을 가진 무장은 전법 01×10(분전). 콜라보 개성·전법이 없는 미축은 그대로. 능력치(편집값)는 그대로.
- 원인: ModPack SAN14_Collab.dll 이 db 구조 안 표 오프셋(전법 db+0x774d0, 개성 db+0x7dd18)을 쓰는데 apply_new 가 코드 rva 로 보고 +0xfa0(0x78470/0x7ecb8) 로 바꿈. 새 exe 도 db+0x774d0 260곳·db+0x7dd18 249곳 그대로(옛 판과 같은 수). → 해금 실패 → 게임이 불러올 때 콜라보 개성·전법을 무장에게서 지움.
- 게임 편집 적용 함수(0x211ba0, 옛 0x2103a0)는 부모·배우자 번호 검사(고대무장)만 바뀌고 개성 9칸(+0x84, 1~0x163)·전법 10칸(+0x9c, 1~0xc8)은 같음 확인.
- 같은 부류 전수(작은 값 0x10000~0x100000 이 바뀐 명령·자료): Collab 4곳만 진짜. 나머지는 코드 rva(함수 본문 일치 확인) 또는 통합편의 자기 표.
- 오늘(새 판) 만든 세이브는 이미 지워진 채 저장됨 — 그 세이브에서는 안 돌아옴. 11-05 이전 세이브·새로 시작(편집 데이터 적용)은 정상이어야 함.
- 판: manifest 9cd882a059073b1c, Collab.dll 4칸.
- 데모 점검(15:52~16:12 실행): 튕김 없음(턴 튕김 해결), 진행 배속 2배 켜짐 확인("선택한 원본 화살표의 2배"), 등록 33·로드 33·시작 33. 포로 복귀·처단불가·재야·AI탐색·AI내정(42개월)·연출생략·34번·29번·32번 동작 기록 있음. 플레이어 조작이 필요한 기능(드래그·빠른메뉴·자동논공 등)은 데모로 확인 불가.
- 시험14b(16:22 사용자: 안 고쳐짐): 불러온 세이브 svdexceKO34(10-31, 개성 있음)인데도 지워짐 → Collab 설치 검사 자체가 실패하고 있었음. vtable 칸 기대값 4개(lea 0xdb90/0xdb30/0xdb60/0xdb00)가 옛 값 그대로 — 새 판 칸 값은 0xdaf0/0xda90/0xdac0/0xda60(함수 본문 같음). 고침(cell_fixes, 옛값 목록 지원). 낮은 주소(0x1000~0x10000) 안 바뀐 값 전수(lowrva.py: lea/add/mov 꼴): 나머지는 크기·플래그(0x3000 등)뿐.
- Diag: 모드 설정 창에서 끈 Ext·ModPack 기능(ExtBridge.ini [Ext] Minimap=0 등)은 '로드 안 됨'으로 안 셈.
- 판: manifest 0d2227d3db20f5dd.
- 사실무장 편집 튕김(16:22, 장수 클릭): 같은 자리(0x92f39c ← 0x494782, 공용 탭 만들기 함수). rbx=0xa·rsp 4바이트 어긋남 그대로, R3 흔적은 쓰레기값 위주라 원인 모듈 미확정.
- 시험14c(사용자 16:29 "꺼 둔 모드는 오류로 띄우지 말 것"): Diag 가 모든 끄기 설정을 따름 — ① modules\*.ini Enabled=0(전부터) ② ExtBridge.ini [Ext] 각 값·[Mod] Enabled ③ ModPackBridge.ini [ModPack] Collab/TraitHover/Field3D·[Mod] Enabled ④ San14ModSettings.ini 의 통합편의 기능(Traits9Module·ModelRegistryModule·BgmExpansionModule·BgmPlayer·ProgressSpeedModule·FormationMoveModule·AutoPersonnel(Module)·DualCredit·Camera AutoFocus·SkipReports·ScenicNoFade·CityNoFade·RecruitmentGate·PrisonerPersuasion·AudienceButton·UnitFollow) 0 이면 그 낱말이 든 통합편의 실패 줄 무시 ⑤ 10번 미설치면 일기토 줄 무시. 시험 test_diag_ownlogs 19/19(쌓이는 기록 대비 자르기 고침), test_diag_native 19/19. 판 manifest 17d66d2f77d22968.

## 2-16 시험15: 문전박대(3번) — 군주가 관문에 있으면 사신 그대로 접견(사용자 16:35·16:39)
- 사실: 3번은 데모에서 확인한 적 없음(데모엔 플레이어 없음). 오늘 거절 기록은 16:34:31 한 번뿐, 그 뒤 사신 2명은 기록 없이 접견.
- ICGuard E(군주 도시 판정 실패 시 본거지 대체) 기록은 runtime.log 전체에서 한 번도 없음 → 관문일 때 판정이 '실패'하지 않거나, 그 전 단계에서 걸러짐.
- 정적 확인: IC 판정(IC+0x31520) 상수·게임 거절 함수(0x81fa70/0x81f290)·접견 함수(0x807910)·사신 표(0x21462d0/e8/0x2146300) 대응 정확. 게임 도시 얻기 0x32e3b0 은 옛 판과 동일 동작(없는 번호면 첫 도시를 돌려줌 → 실패 아님 가능).
- 원인 확정 못 함 → ICGuard 에 기록 장치 추가(동작 그대로): IC 판정 결과·사신 종류·대상·군주 위치·도시 판정 2회 결과/대체 단계, 게임 접견 함수를 누가 불렀는지. 사신 올 때만, 최대 80줄(runtime.log "ICGuard diag E").
- test_icguard.py 상수 새 판으로(0x32e3b0·0x13a96d0·0x13a9880·0x331780), import os 누락 고침(unicorn 없어 실행은 못 함). 네이티브 시험 8종 통과. 판 manifest 273f8120a0c63e34(바뀐 것: ICGuard.dll·manifest·ManifestHash·SHA256SUMS).
- 시험16(16:5x): 코드 흐름 대조로 찾은 진짜 이식 오류 — 통합편의가 db 포인터를 자기 전역(0x1801a4ed8)에서 읽어 와 쓰는 자리 10칸이 '코드 주소'로 잘못 옮겨짐. 포로 거점 찾기(0x180031c90: 지역 0x6caf0·도시 0xdd38·관문/항구 0xded8·군단 0xe0d0), 포로 처우 진입 검사(0x1800643c0: world 0x871e8), db+0x78740, 탐색 예산 숫자 70000(0x11170). 새 exe 도 옛 값을 같은 횟수로 씀. cell_fixes 에 추가, IC 해시 9e76d6b05667bd53(common.h·registry_bridge.h). 전 모듈 같은 꼴 검사(dbscan.py: 바뀐 작은 값 중 새 exe 가 옛 값을 구조 변위로 계속 쓰는 것) — 나머지 0건.
- 문전박대 판정 경로(IC+0x31520·0x399b0·0x8ec0·0x24ba0, 게임 0x1db900·0x8080b0·0x807910·0x81fa70·0x81f290)는 옛 판과 한 줄씩 같음(주소·클래스 이름 RTTI·사신 필드 +0x70/76/78/7c 게터 확인). 남은 건 관문일 때 실행 중 값 → 시험15 기록(ICGuard diag E)이 같이 들어 있음. 판 manifest f7c14ab4f5883280.
- 시험17(17:01 기록으로 원인 확정): 관문에 있을 때 군주 도시 판정이 CGateData(게임+0x13a9c30)를 돌려주고 실패 → ICGuard E 대체가 세력+0x160 을 썼는데 값이 0(두 세력 모두) → 대체도 실패(fb=5). 고침: 돌려받은 게 CGateData 면 게임 함수 0x22c1f0(거점→지역 +0x12→지역 +0x35 도시, 통합편의 포로 거점 찾기도 관문에 같은 함수)로 관문이 속한 도시를 씀. +0x160 은 0 이 아닐 때만 둘째 대체. 판 manifest a21787a5e4d2a3fa.
- 시험18(사용자 17:05 "옛 문전박대엔 사신 기록 없었다"): 시험15 사신 기록 장치(IC+0x31520·exe+0x807910 감싸기) 전부 뺌. 문전박대 보강은 옛 R40 E 와 같은 모양(판정 실패 때만 대체, 처음 3번만 한 줄)에 관문→소속 도시만 더함. 판 manifest ad06b9fa1c614d29.
- 시험19(사용자 17:07 "옛 판과 대조해서 알아서 찾아라" → 전 모듈 대조): ① 9번 Traits9_UI 편집기 rel32 상수 2개(게임 0x4e958b call·0x542e53 jmp → 동굴 0x494431) 옛 값 그대로 → call 이 CSaveLoadDialog::slot38(0x4944d0) 한가운데 0x494741 로 → 사실무장 편집 튕김(exe+0x92f39c, rbx=0xa, 스택 4바이트 어긋남)의 원인. 0xfffaaea1/0xfff515d9 로 고침. ② 22번 DomesticCE 수학 라이브러리 __rem_pio2 상수 0x921fb 되돌림. kUiSha·hover_text_hash·DomesticCE 해시 갱신, mt/build_new.sh. 대조 도구: /tmp/claude-0/pt/dbscan.py(구조 변위)·numscan2.py(숫자 상수)·relscan.py(두 자리 사이 상대값) — 나머지 걸린 것은 모두 정상(새 게임 값 반영 또는 DLL 안 점프). 판 manifest 89e45f3cc9164c13.
- 시험20(사용자 17:21·17:23): ① 27번 미니맵 안 그려짐 = 셰이더 진입점 글자열 "vs"(0x00737600)가 주소로 오인돼 0x00739990 으로 바뀜 → D3DCompile 실패(hr=80004005, draws=0). 되돌림(cell_fixes). 전 모듈 글자열 깨짐 검사(textscan.py) — 이것 하나뿐. FEED90(안개판 기능, 지금 꺼짐) 쪽 페이지 표 0x66b000/0x66e6d0/0x66f000 은 새 판 4페이지와 안 맞음 — 꺼져 있어 손 안 댐. 게임 폴더 SAN14_Minimap.ini 의 Profile=1(시험11 진단) 지움.
  ② 전법 마우스 설명: 14번 SAN14_전법설명.txt 에 콜라보 전법이 원래 없었음(11-02 최종판도 같음) + '충명강의' 오타. 게임 전법 표(fixdataexce.s14, 폭 0x5E)에서 계통(+0x30)·대거점(+0x33 bit0)·효과1/2(+0x50/+0x51)·의존능력(+0x58) 규칙을 찾아 기존 125개와 글자까지 일치 확인 후 48개 추가, 총명강의 고침(tactic_desc_gen.py). 효과 07=아군 공성상승은 대칭 추정(귀족영광·궁기만 씀). 판 manifest 3ca90d81e8112bc8.

## 시험22 → 정식판 R43 (2026-11-07, 11월 7일 최종판)
- 설치기(Engine.cs): IndexSpec 에 PrevPayload/PrevKey(옛 1.0.1 원본 색인 data/prev101/KIDSSystemResource.rdb·ScreenLayout.rdb, manifest Value prev_*).
  ModelPatch·ModelUnpatch 첫머리 Refresh: 1.0.1 원본과 바이트까지 같은 레코드(새 원본과 다른 것: KIDS 1개, Screen 8개)만 새 원본으로. 설치 기록 없는 색인도 RefreshOnly(실패하면 그대로).
  MissingPayloads 에 prev 자료 추가. 실제 파일 흉내(python): 옛 원본 통째 → 새 원본과 같음, 그 위 패치 → 합본과 같음(KIDS·Screen 둘 다). PC csc 컴파일은 설치.cmd 실행 때(여기선 컴파일러 없음).
- 27번 FogTrial FEED90: 호출하는 곳 없음(디스어셈블 전수) + feed90_install 첫 바이트 ret, feed90_validate = xor eax,eax;ret (cell_fixes). FogTrial 05aaec0e.
- assemble.sh: f24 fixdata_original.s14 를 게임 폴더 사본(24번이 깔려 nostat 로 바뀜) 대신 newpkg/data_new/fixdata_original_101.s14 에서.
- README 정식판 문구. build_final_r43.py: 11월5일 최종판 + R43 → final_r43/통합패치 11월 7일 최종판 + 분할 ZIP 11개(494MB), 전부 풀어 SHA256SUMS 165·manifest 138·Values 9 대조 통과.
- manifest aa2042a9011fda4f.
- 업데이트 대비(2026-11-07): 작업재료\업데이트대비\3_기준자료\기준자료_20261105_1of2(원본 exe 782cf229)·2of2(해제본 5f3ae40d + bt_exce·fixdata·experience·0002 색인 5개 원본), 1_소스\R43정식, 읽어주세요_업데이트날.txt R43 기준으로 새로.
- 정식판 뒤 진단 수정(19:12): Diag 의 22번 DomesticCE 성공 기준 'DomesticCE: ACTIVE'(세이브를 불러와야 남음 → 타이틀 90초 넘으면 잘못된 창) → 'DomesticCE loaded:'(시작 때, runtime.log 최근 24회 전부 있음). READ-ONLY 는 그대로 실패. Diag 93c4cc7f, manifest 7f3f9081. 덮어쓰기 ZIP(진단수정) 4파일.

## 보류(11-07 19:45 사용자 "킵"): AI 모드 v15.6 통합패치 호환판 계획
- 원칙(사용자): 통합패치는 손대지 않음. 겹치는 기능은 AI 쪽에서 끄고 항목마다 "통합패치 N번과 겹침" 표시. 개인 사용.
- AI v15.6 은 새 게임판 지원(스팀 빌드 1a4a447d), dinput8 체인(원래 dinput8 → SAN14_AI_PreviousInput.dll), 파일 충돌 없음.
- 끌 항목 17: extensions.traits9(9) stats200·stats200_growth(17) tooltip_collab(14·9) battle_deaths(25) scenery_skip(1) domestic.domestic·domestic_direct·domestic_free(22) domestic.population(28) personnel.merit·merit_troops_level·merit_friendship(2·13) officer_ui.drag_learn·performance.fast_menu·personnel.awards(30) domestic.region_officers·region_officer_traits(34).
- 방법: ① AI모드_설치관리자.exe #US 문자열 같은 길이로 설명/이름에 표시(설명 없는 것은 이름) ② FeatureSelect.ps1 Overlap 속성 → Get-FeatureValues 0 강제·ini 주석·메뉴 표시 ③ 바탕화면 'ai 수정 통합패치 호환판\SAN14_AI_v15.6_통합패치호환' 직접. 자료: /tmp/claude-0/ai (i=통합 ZIP, s=소스), us.pkl=exe 문자열 표.
- 참고: AI 제작자 주장 0x45628C(옛 판) 튕김 = IC 1번 행동보고·수지보고 생략 20바이트 창(새 판 0x45847C)을 IC 가 실행 중에 다시 씀. 통합패치는 안 고침.
- 19:5x 만듦: 바탕화면\ai 수정 통합패치 호환판\SAN14_AI_v15.6_통합패치호환 (39파일, DLL 원본 그대로). FeatureSelect.ps1 Overlap 강제 0(값 항목 2개는 표시만), 설치관리자.exe #US 21곳 같은 길이 표시(0651d42b), 통합패치호환_읽어주세요.txt. 작업 사본 /tmp/claude-0/aiout/pkg. 실게임 미확인(pwsh 없음 → 괄호 대조만).
- 20:1x AI 호환판 2차: 19:57 실행에서 5번 수행·대열 실패 = AI 가 옛 0x183CC9(새 0x184FF9, 이동 속도 0x2B0290 호출) 를 같이 걸음(combat.sortie_formation 49곳 + performance.path_fast). 우리 대장(1.0.1) × AI DLL 안 옛 판 자리 목록(2059곳) 대조로 충돌 추가: advisor_briefing(4번 재해·등용·보물 0x6C43B0/0x6C4670/0x29840B…, 22번 DomesticTalent 표칸 0x13D63C8), battle_rules(1번 모드 설정 표칸 0x141BCC8). AI 논공·창고 저장 연결(0x3F7E32·0x13A6BB0)은 AI 설계상 늘 켜짐 → 못 끔(지금은 13·21번이 먼저 잡아 정상).
  설치관리자.exe: 메타데이터를 .rsrc 뒤로 옮겨 #US 늘리고 이름 23개 ldstr 토큰만 새 문자열로(upd_tools/relabel_exe.py, relabel.tsv) — 이름 앞 【통합패치 N번과 겹침/충돌 · 꺼짐】. 다른 스트림·코드 동일, ldstr 1204곳 전부 유효. FeatureSelect 21개 강제 0. 사본 r41/ai_compat.
- 23:0x 제보 수정(덮어쓰기 진단·설치기수정): ① Diag: 1번만·25번 미설치면 Patch27 이 'Patch27 R22: features off' 를 남기는데 성공 문구만 찾아 25번 오경보 → features off 도 정상(시험 시나리오 1 을 그 경우로, 수정 없으면 실패 확인). Diag 35377a06. ② 설치기 RuntimeIni 에 DomesticPlayer.ini·DomesticAI.ini·AutoPersonnelRepairMerit.ini(게임 안 설정 창이 값을 써 넣음) — 빠져 있어 "다른 프로그램이 바꾼 파일"로 설치 멈춤. manifest 2acbb0e3. ③ 모의 일기토 "선택한 무장 원본이 아직 준비되지 않았습니다" = IC 가 두 무장의 번호(0x16e580/584)·이름 문자열·0x1F0 자료 읽기 확인 실패 → 무장을 바꾸는 발탁무장 모드 유력(파일 없어 미확정).
- 23:3x 제보 수정(덮어쓰기 25번시작수정, 누적: 진단·설치기수정 포함): 제보 R43F-25-ic_wait(삼갤러4 ×2) + 같은 사람 종합 진단 'lacks Patch27: checked feature groups'(20모듈).
  원인: Patch27 ModInit 이 SAN14_WaitConvenienceV1(통합편의 쪽 한도 15초) 한 번만 부르고 0 이면 창+return 0 → 느린 PC 에서 25번 전부 안 켜짐, 25번 미설치(1번만)에도 창.
  고침: 평소(15초 안)는 예전과 똑같이 ModInit 안에서 진행(통합편의가 뒤 모듈보다 먼저 끝나는 순서 유지). 15초 넘기면 'startup slow' 기록 + 뒤 작업이 1초 간격 최대 10분 더 기다리고 끝나는 즉시 진행.
  실패 창은 25번 설치 + 10분 넘김(또는 export 없음)일 때만. Diag: 'Patch27: common module startup slow' 도 정상(기다리는 중).
  시험: test/test_patch27_icwait_native.cpp(새, shim27) 8경우 통과 — 옛 소스로는 제보 경우(3·4) 실패 재현. test_diag 20/20(1b 추가). 전 네이티브 통과. Patch27 7f165021, Diag 9320b078, manifest e8f08f5b.
- 같은 때 제보(설치기 'SAN14_CrashLog.dll 액세스 거부'): 설치기는 꾸러미 파일을 읽기만 함(File.ReadAllBytes) → 앞 9개는 읽힘, CrashLog 만 거부 = 제보자 백신이 막음/격리. 안내만(격리함 복원·예외·다시 풀기). 설치기 안내 문구 보강은 제안만(미승인).
- 11-08 덮어쓰기 11월8일(누적: 진단·설치기·25번 시작 수정 포함) — 갤 제보 3건 + 사용자 요청
  ① 개성 설명 안 나옴(9+14, 13 없음): Traits9_UI 와 TraitHover 가 같은 게임 함수표 칸 146곳(Traits9_UI 표 +0x910c = TraitHover 안 같은 표)을 씀. 13번은 MeritTraits.ini(ModPackBridge.ini 앞)에서
     화면 부품을 먼저 켜서 정상, 13번 없으면 화면 부품은 통합편의 원본 확인 뒤 Traits9.dll 이 켬(늦음) → 14번이 먼저 차지 → HOVER_SKIPPED. 고침: Traits9Check 등록 이름 Mod09_Traits9Check.ini
     (MeritTraits < Mod09 < ModPackBridge) + ModInit 에서 화면 부품 LoadLibrary·SAN14_ApplyTraits9UI(InitOnce 라 중복 안전). 14번 없음/꺼짐/이미 켜짐이면 안 함. 설치기: 대상 이름만 바꿈(자료 그대로),
     RuntimeIni 에 새 이름, 옛 이름 사용자 설정 이어받음. 진단·feature_table 새 이름.
  ② 오류창 '다른 모드와 충돌' 크게(사용자 11-08): src/r43_conflict.h — 실패 자리 갈림(jmp/call/jcc/FF25/mov imm64/push-ret) 따라가 DLL 이름(다리 코드 3번까지, 바뀐 바이트 범위만),
     FOREIGN/OURS/CHANGED, TaskDialog 큰 제목(shell32 #124 매니페스트로 공용 컨트롤 6판, 실패 시 MessageBox+■ 제목). r39::fail_plan/fail_bytes → fail_ex(owner), R43F 줄 끝 owner= 칸.
     진단: 통합편의 원본 확인 막힘(IC+0x1fa000==0 또는 '3분' 줄) → 원인 한 줄 + 2,346곳 표(IC+0xb7310/0xb5710) 주인 + 기다리던 ICGuard·ExecuteLock·AutoPersonnel·Patch27 묶음,
     5번 수행·대열 실패 → 51곳 표(IC+0xdedd0) 주인, 9번 HOVER_SKIPPED → 146칸 주인. 9칸 C(unknown layout) 창 큰 제목 '가능성'. 모든 우리 DLL 재빌드(가져오기 +GetProcAddress/LoadLibraryExW 만, 내보내기 그대로).
  ③ 2번 '상태 구조/비용 확인 실패' 는 그 턴 건너뛰는 안전장치(본인 PC 270번 중 12번) → 진단은 3번 이상·성공 턴 없음일 때만.
  시험: test_r43_conflict_native(새 17), test_traits9_early_native(새 10), test_diag_native 31(4번 시나리오 추가), 기존 전부 통과. 에뮬 시험(unicorn)은 이 환경에 없어 못 돌림.
- 11-08 2차(덮어쓰기 11월8일_2차, 누적) — 갤 제보 '9칸 안 깔았는데 9칸 오류(Traits9 unsupported game version)': 통합편의는 게임 폴더에 SAN14_Traits9.dll 이 있기만 하면 부름(설정 무관, IC+0x4bcb9)
  → 기록 없이 남은 1.0.1 때 9번 파일. 사용자 방향: 창을 숨기지 말고 옛 파일을 없앨 것.
  설치기 Plan: 안 고른 항목 대상 + RetiredTargets(Stats200 3개·옛 Traits9Check.ini·ExtBridgeTest.ini) + 안 쓰는 ModPackBridge.ini 중 설치 기록에 없는 것 정리
   — 등록 파일은 우리 Id 목록(OurModuleIds; 남의 모드도 SAN14CE. 씀: 기색 SAN14CE.Condition)일 때, 그 밖은 Known(옛 판) 해시일 때만. 지금 판과 같은 파일·모르는 파일·남의 키트(26·27·28·30)·공용 런타임·게임 자료는 그대로. 사본 .san14ce_compat_r3/leftover/.
  진단: 설치기 기록(installer="R43")으로 고른 항목 확인 → 안 고른 항목의 기록·등록 모듈이 보이면 '설치 안 한 기능의 옛 파일 남음 → 선택 설치로 정리' 문제 + 큰 제목(숨기지 않음). 13번만이면 '13번 공용 부품' 표시.
  시험: test_installer_leftover.py(모형 — C# 컴파일러 없음, 실제 manifest·11월2일 옛 9번 파일·사용자 Condition.ini 로 판정) 8, test_diag_native 36, 전부 통과.
- 11-08 3차(덮어쓰기 11월8일_3차, 누적) — 갤 제보 '32번 켜면 먹통, 빼면 됨' + 새 진단 창이 범인 표시(통합편의 원본 확인 자리 exe+0x231770 StatusColor·0x331416 HiredFace).
  원인: 통합편의는 시작 때 게임 코드 2,346곳(IC+0xb7310 표) 원래 모양 확인(최대 3분) 뒤에야 1~7번을 켬. 32번 4곳(0x331416·0x33149a·0x336a0e·0x339729)·29번 1곳(0x231770)이 그 표 안
  → 우리 모듈이 먼저 고치면 통합편의가 3분 기다리다 1~7번 전부 안 켬(ICGuard·ExecuteLock·25번도 줄줄이). PC 속도에 따라 순서가 갈려 일부 PC 만. 본인 PC 마지막 세션 247곳 중 겹침은 이 5곳뿐.
  고침: src/ic_after_restore.h — 그런 모듈은 통합편의 확인이 끝난 뒤(SAN14_WaitConvenienceV1=1) 뒤 작업에서 고침(통합편의 없음/꺼짐 → 바로, 60초 안 안 올라옴·10분 넘김 → 그냥).
  진단: 29·32번도 '통합편의 기다리는 모듈'로 묶음. 시험: test_ic_restore_overlap.py(새: 우리 모듈 고칠 자리가 통합편의 확인 표 안이면 확인 뒤 고치는지 강제), test_ic_after_restore_native(새 9), 전부 통과.
## 다음 수정판에 넣을 것(사용자 11-08 17:18 승인) → 4차에 넣음
- 1번(모드 설정 창) 잠금: 20·31 말고 아무 번호나 고르면 1번 자동 체크 + 회색으로 못 끔(설명 '다른 번호의 게임 안 스위치용이라 같이 깔립니다').
  20·31(게임 안 스위치 없음, 파일만 바꿈)만 고르면 1번 안 넣음. 18·24 는 설정 창 밸런스 탭에 켬/끔이 있으므로 잠금 대상(17:31 바로잡음 — 처음엔 18·24 도 예외로 잘못 적음). 1번 빼고 깐 사람은 다음 '설치 갱신' 때 자동 포함. 1번은 파일 없이 IC 설정 ModSettingsUI=1 + Patch27(when 1|25, 설정 5·6쪽) 뿐.
  손댈 곳: installer101 App.cs(칸 잠금)·Engine.cs(Desired 에 on[0] 강제)·Tests.cs, 진단 selected() 에 1번 늘 선택, test_installer_leftover·test_diag 기대값.
- 옛 설정 창(통합편의 원래 창) 아예 안 뜨게(사용자 11-08 17:34 "무조건 지금 설정창"): ① ModSettings.ini UseNewWindow=0·San14ModSettings.ini NewSettingsWindow=0 무시(늘 새 창, 설치기도 0 안 남김)
  ② 새 창 훅이 다른 모드 충돌로 못 붙거나(plan.bad/vtable)·IC 해시 모름·릴레이가 딴 데를 가리켜도 IC '모드' 버튼 릴레이(IC+0x1c1358 page+0x66)는 우리 것으로 바꿔
     옛 창 대신 '다른 모드(이름)와 충돌해 설정 창을 열 수 없음' 창(r43c 큰 제목) — 이때 기능은 설치 때 값으로 계속 돎. 실패 창 글자 '옛 창으로 열림' 고침(mod_settings.cpp fail_box·fail_plan).
  사용자에게 알린 단점: 충돌 때는 게임 안에서 켜고 끌 길이 없어짐(옛 창이면 됐음).
  (17:36 사용자: 다른 모드 충돌은 어쩔 수 없음, 통합패치만으로는 새 창 실패가 절대 없게) 점검 결과:
   - 새 창 12자리(get/set/std/msg/bind/enter/exit/update/msg2/scrollbar/range/clamp)를 건드리는 우리 다른 모듈 없음(소스 전수 + 본인 PC runtime.log 48회 실행 기록 겹침 0). IC R43 해시 9e76d6b0… known_common 에 있음. 본인 PC 에서 설치·버튼 교체 성공 기록.
   - 약한 곳 1: relay_worker 가 IC 설정 페이지를 120초만 기다림 → 오늘 같은 IC 늦은 시작이면 새 창 실패 → 옛 창. 고칠 것: IC 확인 끝(SAN14_WaitConvenienceV1) 기준으로 최대 10분 기다림(ic_after_restore 방식).
   - 넣을 시험: 꾸러미 IC 해시 ∈ known_common, 새 창 12자리와 우리 모듈 고칠 자리 겹침 금지(test_ic_restore_overlap 넓히기), 늦은 IC 에서도 버튼 교체.
- (11-08 17:53 갤 제보 '상업으로 지정해도 다음 턴 농업으로') 22번 ① 플레이어 내정 자동(SAN14_DomesticPlayer.dll) 버그 확정:
  게임 판정 함수 0xb2af0 디스어셈블 — 방침3→+0x3c 상업=결과1, 방침2→+0x3e 농업=결과2, 방침4→+0x40 병영=결과3 ⇒ 중점 +0x72: 1=상업 2=농업 3=병영(DomesticAI 규칙과 같음).
  DomesticPlayer focus_code 는 1=농 2=상 3=병으로 거꾸로 → '상업'으로 쓴 2 가 게임에선 농업. 단계 같으면 상업 우선이라 거의 늘 2(농업). 본인 PC audit 줄 '중점1→2' 다수가 그 증거.
  또 설계상 3초마다 플레이어 지역 중점을 덮어써서 직접 고른 값이 유지 안 됨. 제안(미승인): 번호 바로잡기 + 플레이어가 바꾼 지역(우리가 마지막에 쓴 값과 다르면)은 손대지 않기.
- 11-08 4차(덮어쓰기 11월8일_4차, 누적, 28개 파일) — 위 '다음 수정판에 넣을 것' 전부 + 22번 내정 자동 반영(사용자 17:56 "고치자"):
  ① DomesticPlayer 11f51868: 중점 번호 1=상업 2=농업 3=병영(게임 0xb2af0 기준)으로 바로잡음 + 플레이어 선택 유지(처음 볼 때·고른 항목 만렙·중점 없음일 때만 씀,
     안 건드린 지역은 개발 단계가 바뀌어 골고루 답이 바뀔 때만 씀, 우리가 쓴 값과 다르면 그 지역은 손 안 댐, 불러오기 흔적이면 기억 지움). 월별 줄 player_kept=.
     설명 글(설치기 22번 ①, 설정 창 '모든 내정 자동') 고침.
  ② ModSettings 5b4218e2: UseNewWindow 안 읽음(늘 새 창). 훅 실패(다른 모드)면 blocked → 버튼 릴레이는 그대로 바꿔 '모드' 버튼에서 충돌 안내 창(r43c 큰 제목), 1 돌려줌(DLL 남김).
     relay_worker: IC SAN14_WaitConvenienceV1 이 0 이면 최대 10분까지 기다림. 실패 창 글자 '옛 창으로 열림' 없앰.
  ③ 설치기: Spec.UsesSettingsWindow/WithSettingsWindow — 20·31(·1·12) 말고 아무 번호나 깔리면 1번 같이(Engine.Clean, App.Run 기록 한 줄). NewSettingsWindow 늘 1. 1번 설명 글 고침.
     (C# 컴파일러 없음 → 괄호 균형만 확인. 사용자 PC 설치.cmd 가 csc /warnaserror+ 로 빌드)
  ④ Diag 746e93bf: selected(1) — 20·31 말고 고른 번호가 있으면 1번 선택으로 봄.
  시험: test/run_offline_all.sh(새: 오프라인 전체 한 번에 14개) 전부 통과. 새 test_settings_window_r43.py(IC 해시 known·새 창 12자리 겹침 0·소스 규칙). domestic_player 시험 새 규칙으로 고침.
  make_cum_overlay.py(새): 최종판 SUMS 대비 누적 덮어쓰기 ZIP + 해시 재검사. 3차와 비교: 바뀐 것 Diag·ModSettings·DomesticPlayer·App.cs·Spec.cs·Engine.cs·manifest·ManifestHash 만.
  사용자 PC(3차 적용 뒤 17:11·17:22·18:00 실행): Diag problems=0, HiredFace·StatusColor OK, ModSettings 12/12 relay_swapped.
  튕김 기록: 새 1건 2026-11-08 17:05:11 exe+0x93c3db(게임 파일 읽기 작업 스레드, [rdi+0x10]=0xffff000000 깨진 포인터) 스택 전부 게임 코드·모드 없음, 3차 적용 전 실행, 처음 보는 자리 → 지켜봄.
## 덮어쓰기 판 낼 때 순서(사용자 11-08 18:15·18:17 "담부터 그렇게 해")
- 덮어쓰기 ZIP 을 만들면 ① PC 정식 폴더 'C:\Users\user\Desktop\통합패치\통합패치 11월 8일 새버전' 에 그 파일들을 덮어씀(device_commit_files, 끝나면 SHA256SUMS 대조)
  ② 통합패치 전체 압축본(비밀번호 123)을 만들어 대화 파일로 줌 — 파일명에 꼭 '비번123'. 도구: make_full_pw_zip.py <차수> <출력폴더> (최종판 11조각+바뀐 파일, 전부 해시 대조, zip -P 123, 한글 이름 UTF-8 표시, 조각 재검사).
  4차는 사용자가 직접 덮어쓰고 '통합패치 11월 8일비번123.zip'(516MB, 정식 폴더 안)을 만들어 둠(PC SHA256SUMS = 4차 확인). 다시 묶을 때 폴더 안 *.zip·설치기가 만든 SAN14CE_Patch101.exe 는 빼야 함.
## 5차 준비(소스만, 사용자 '만들어' 기다림) — 11-08 18:17~18:18 제보 2건
- 22번 추가 제보 '턴마다 상·농·군 돌아감, 다 안 키우고 넘김': 사용자 PC SAN14_DomesticCE.log 에 'DomesticCE: ACTIVE; validated 338 regions; player=2; changes=N. All domestic…' → 옛 DomesticCE(바이너리)도
  플레이어 지역 중점을 씀. 옛 DomesticPlayer(번호 거꾸로)와 서로 덮어써서 돌아감. 4차는 번호만 고쳐서 CE 가 여전히 플레이어 선택을 덮어쓸 수 있음.
  고침(소스): DomesticPlayer 50dbf5c9 — 시작 때 SAN14_DomesticCE.ini [Automation] AllDomestic=0·PopulationFloor=0, 틱마다 CE(알려진 해시 caab6a7c…)의 칸1·2 스위치(CE+0x2d584/0x2d588)가 켜져 있으면 0.
  AI 명령서(칸3)·저돌은 CE 그대로. ModSettings 0015ecef — '모든 내정'·'최소 인구' 스위치를 눌러도 CE 쪽은 늘 0(DomesticPlayer 만 켜고 끔). 월별 줄 ce_off=.
- 진행 배속 제보('5번 진행 배속: 진행속도 확장 준비 검사 실패. 원래 세 단계 유지'): 통합편의 IC+0x54050 이 게임 자리 5곳(함수표 칸 0x1476e58/68/80 + 코드 0x89abf7(5)·0x44e2c1(0x2d))을 확인·고침.
  우리 모듈 소스·본인 PC 훅 기록 어느 것도 그 자리와 안 겹침. 본인 PC 는 11-06 시험판 때만 실패(시험4 뒤 정상) → 제보자 쪽 다른 모드가 그 자리를 먼저 고쳤을 가능성.
  Diag 88f2221a: ① 통합편의 실패 줄 번호 표가 하나씩 밀려 있었음(진행속도 5→6, 수행·대열 5, 인재·포로 6→7, BGM 7→8) — 바로잡음. ② 6번 실패면 그 5곳 주인 찾아 '다른 모드와 충돌 — 이름' 표시.
  시험: test_diag 40(진행속도·BGM 번호, 6번 주인), domestic_player(CE 끄기 칸1·2·ini, 칸3 그대로), 전체 run_offline_all 통과.
- 11-08 5차(덮어쓰기 11월8일_5차, 누적 28개) = 위 '5차 준비' 그대로(DomesticPlayer 50dbf5c9 · ModSettings 0015ecef · Diag 88f2221a, manifest fca66dde). 4차와 비교해 바뀐 건 이 3개 + manifest·ManifestHash·SUMS 만.
  처음으로 새 순서대로: PC 정식 폴더('통합패치 11월 8일 새버전')에 28개 덮어씀 → 다시 받아 해시 대조 6개 ok.
  전체 압축본 '통합패치 11월 8일 5차 비번123_1of6~6of6.zip'(zip -P 123, 한글 이름 UTF-8 표시, 1번 조각에 설치.cmd·README·설치기·manifest) 대화로 보냄. 165개 파일 해시 대조 0 불일치.
  make_full_pw_zip.py: README_덮어쓰기_* 는 넣지 않음(정식 폴더에도 없음). 조각 크기 100MB 로 씀(6조각, 합 516MB).
- (11-08 18:38 사용자) 전체 압축본은 용량이 커서 그만 — 다음부터는 덮어쓰기 ZIP 만 보냄. 정식 폴더 덮어쓰기는 계속(가벼움). make_full_pw_zip.py 는 요청 있을 때만.
- 11-08 6차(덮어쓰기 11월8일_6차, 누적 30개) — 포로 회유 '등용 눌러도 메시지 없이 모두 거절' 확정·수정.
  재현(본인 PC 20:18, 6차 전): 통합편의 기록 "포로 처우 v3: 2명 표시" → "최근 등용: 처리 0명, 거절 0명, 제외 2명" (처우 action 줄·대사 줄 없음). 11-07 16:53 '유지 → 제외 5명'도 같은 원인.
  원인: IC 0x23d1d cmp r12,[r14+rbx*8+0x1a51d0] — 포로별 억류 거점 표는 IC+0x1a3ed0(목록 만들 때 0x57e13 이 씀). 장부 오염(자리_대응에 섞인 IC 내부 주소)으로 apply_new 가 바꿨고 base_flow 복원에서 빠짐.
   같은 꼴 하나 더: 0x29c73 옛 설정 창 스위치 콜백 표 0x19e6a0→0x19f9a0(빈 BSS). 1.0.1 IC 대조 결과 IC 기준 레지스터 접근 중 바뀐 건 이 2곳, rip 상대 변경 0, 다른 모듈에 같은 변환 0.
  고침: upd_tools/cell_fixes_r43.py 에 2칸(되돌림). IC dfa4ca9f… → common.h r43(새)+r43a(옛 9e76d6b0 유지), registry_bridge.h module_sha256_r43b → build_all·field_build·assemble·overlay.
   새 IC 해시를 품은 모듈만 바뀜: ICGuard·ModSettings·AutoPersonnelRepair·Patch27·ExecuteLock·3D Runtime(+IC). ic_verify_r43: 통과 3063/실패 0(5차와 같음). 4번 등용 장면 생략은 무관(0x6c6b10 호출 4곳 전부 등용 명령 쪽).
  시험: test/test_ic_self_refs.py(새, 5차 IC 로는 2곳 실패 재현) + run_offline_all 15개 전부 통과. 튕김 기록: 오늘 밤 새 튕김 없음(게임이 처리한 C++ 예외만).
  정식 폴더 30개 덮어씀, 다시 받아 10개 해시 대조 ok.
- 11-08 7차(덮어쓰기 11월8일_7차, 누적 31개) — 갤 제보 2건 '폴더·exe 지정해도 설치기가 인식 못 함(먼저 게임 폴더를 확인해 주세요)', 둘 다 발탁·육아 먼저 깔았음.
  원인 경로(설치기 LoadState 가 예외 → loaded=null): ① Files.Child 연결 폴더(재분석 지점) 거부 ② 끊긴 설치 복구(pending.xml) 실패 ③ state.xml 형식/파싱 실패·같은 파일 두 번·잘못된 경로 ④ 백업 blob 없어짐(백신 격리)→Tx.Get 실패 ⑤ 상태 확인 중 파일 하나 해시 실패.
  고침(설치기만): Core.cs 연결 폴더 따라감, Recover 실패 → pending.xml.broken_시각 보관 후 진행(RecoverNote 기록칸), Engine.Load 깨진 기록 보관 후 새로·이상한 줄 건너뜀·없어진 백업은 LostBackup(제거 때 그 파일 남김),
   Status 파일별 예외 → 그 번호만 '설치 확인 필요', App 상태줄·설치 버튼 창에 진짜 이유. 제보자 정확한 원인은 미확인(기록칸 글자 없음) — 코드상 이 증상으로 가는 길 전부 막음.
  C# 컴파일러 없음 → test/test_installer_syntax.py(새: 괄호 짝 + 7차 규칙) — 작업 중 줄 가운데 주석이 코드 삼킨 실수 잡음. run_offline_all 16개 통과. 정식 폴더 4개 덮어씀·대조 ok.
## 8차(11-09 13:12 "만들어" + 13:13 "9·13·14번이 서로 기생하지 말고 독립적으로" → 만듦, 덮어쓰기 11월9일_8차 33개)
- 제보(11-09 11:49, 15모듈, 2D·계절·재해 같이 씀, 기색 모드 없음): '14번: callback owner/address/body does not match supported native or VNext 1.5.6 slot (first site 0x01463070) ×38'.
  (처음엔 기색 v1.2 PhaseTip(0x8ca1c0) 탓으로 잘못 답함 — 제보자는 기색 없음. 바로잡음.)
  분석: TraitHover .dll(14만, when 14&!9&!13)은 칸 값 == 게임 원래 함수만 허용(0x4c80). .v2(14&9|14&13)는 시작 때(0x5070) 칸 값이 SAN14_Traits9_UI.dll 안이면 종류별로 기억해 허용(0x5150).
   깨끗한 새 exe: TraitHover 표 152칸 전부 원래 값과 일치. 사용자 PC(2D·계절·재해 포함) runtime 기록·SeasonRules 바이너리에 그 칸 건드림 없음 → 칸을 바꾸는 건 우리 9번 화면 부품뿐.
  원인 길: ① 9번 안 고른 PC 에 9번 파일이 남음 → 통합편의가 SAN14_Traits9.dll 있으면 무조건 실음 → Traits9_UI 가 칸 차지 → .dll 거부.
     설치기 옛 파일 정리가 '지금 판과 같은 파일은 둠'(Engine.cs Plan 정리 블록) → 설치 기록 없어진 뒤(7차 state.xml 보관 등) 9번 빼고 다시 깔면 남음.
   ② Traits9_UI 가 TraitHover 시작(0x5070)보다 늦게 칸을 차지하면 .v2 도 거부.
  고칠 것: (1) TraitHover .dll·.v2 둘 다 — 칸 검사(0x4c80 쪽) 때 칸 값이 SAN14_Traits9_UI.dll 안이면 그 자리에서 받아 이어 씀(시작 때 기억 표 의존 X). 남의 모드 칸은 그대로 거부.
   (2) 설치기 — 고르지 않은 기능의 대상 파일이 지금 판 해시와 같아도(기록 없음) 정리(leftover 사본 보관). Mutable·data·모델 색인 제외 규칙은 그대로.
   (3) 시험: TraitHover 두 판 칸 검사 흉내(원래/Traits9_UI/남의 DLL) + 설치기 정리 모형.
- 제보(11-09 figekth, 9+14 켜고 13 없음): '[R42 T9 B] ui=file logui=SAN14 Traits9 UI: SKIPPED - code signature mismatch or Traits9 dependency absent' → 편집 화면 개성 5칸만.
  원인: 우리 회귀(2차 early_ui). Traits9Check ModInit 의 early_ui 가 9칸 화면 부품(Traits9_UI)을 9칸 본체(SAN14_Traits9.dll, 5자리 5→9)보다 먼저 켬.
   Traits9_UI 는 그 5자리(0x4a90b3·0x4a96d3·0x226fbf·0x2320aa·0x234737)가 9로 바뀌어 있어야 붙음(UI dll 0x9290 표) → SKIPPED, InitOnce 라 뒤에 통합편의가 Traits9.dll 을 실어도 다시 안 붙음.
   13번은 ensure_nine 로 5자리 먼저 → support_ui 순서라 정상. 5자리·UI 가드 0x494431 은 통합편의 원본 확인 표(2,346곳)와 안 겹침(확인함).
  고칠 것: (4) early_ui — Traits9_UI 대신 SAN14_Traits9.dll 을 먼저 실음(DllMain 에서 5자리 고친 뒤 화면 부품을 스스로 켬 = 통합편의가 하는 것과 같은 순서, 시점만 앞당김; 뒤의 통합편의 로드는 이미 올라온 것이라 다시 안 돎).
    실은 뒤 nine_sites()==5 와 화면 부품 붙음 확인, 아니면 원인 줄 남김.
   (5) 시험: early_ui 순서 규칙(Traits9.dll 먼저) + 9칸 5자리·UI 가드가 통합편의 확인 표와 안 겹침.

### 8차 실제로 한 것(위 계획을 '독립 작동'으로 바꿈)
- 진짜 DLL 코드 흉내 test/th_sim/th_sim.c(PE 를 기본 주소에 올려 TraitHover 시작 0x5070·칸 검사 0x4c80 실행)로 확인한 옛 판 문제:
  .dll+화면 부품 먼저 → 146칸 거부(11-09 ×38 첫 칸 0x1463070 과 일치) / .v2+화면 부품 → 2칸(0x13affc8·0x13f2dd8) 거부 / .v2+화면 부품 없음 → 개성·전법 설명 안 함(0x21b4 가 늘 화면 부품에 맡김, 11-08 '13번 없이 설명 안 나옴' 원인).
- 14번: upd_tools/traithover_unify_r43.py — .v2 바탕 한 파일(.dll=.v2 같은 해시 16ec95c0). 새 구역 .th8(0xaa000).
  칸 검사 0x5150: 원래 함수 늘 받음 + 종류별 기대값. 시작 마스크 0x2ed4 → stubX(화면 부품 없음 0x131 / 있음 0). 설명 고르기 0x21b4 → stubY(없음: .dll 게임 설명 표 / 있음: .v2). 기록 숫자 0x23064a→0x2320aa.
  '화면 부품 있음' = 종류0 기대값(0xa7250) ≠ 게임+0x8ca1c0. cell_fixes_r43.py 끝에서 실행(두 번 돌려도 같음).
- 9번: traits9_check.cpp early_ui → early_nine: 시작 때 SAN14_Traits9.dll 의 SAN14_ApplyTraits9(본체 5자리 → 화면 부품) 직접, 14번 유무 안 봄. 13번이 먼저면 ALREADY/InitOnce.
- 설치기: 안 고른 항목 정리에서 '지금 판과 같은 파일'도 정리(사본 leftover). Engine.cs.
- 시험: test_traithover_unified.py(새), test_traits9_independent_native.cpp(early 시험 대체), test_installer_leftover.py 보강. 오프라인 17개 전부 통과.
- PC(11-09 17:08): 정식 폴더에 7차→8차 바뀐 7개 덮어씀, 바뀐 32개 해시 전부 일치 + SHA256SUMS 같음. 소스 ZIP 8차 두 폴더에 넣음. 튕김 기록 새 것 없음(마지막 11-08 23:56 정상 종료).

## 9차(11-09 17:51 "만들어": 13번 제보 수정 + 통합편의 빠른 쪽) — 덮어쓰기 11월9일_9차 34개
- 제보(13번 'native target function was modified', 통합+2D+계절, 새로 설치): 우리 꾸러미·IC 표·본인 PC 훅 기록 어디도 그 21개 함수 앞을 안 고침 → 밖의 모듈.
  고침: mt/src/merit_runtime.c function_guards_ok — F_ELIGIBLE(13번이 덮어씀)만 엄격, 나머지는 다른 모듈의 점프 연결(E9/FF25, 뒤 바이트 원래, 대상이 게임 밖)이면 통과+기록.
  실패 때 기록에 함수 이름·주소·첫 바이트·점프 대상. 시험 test/merit_guard/run.sh(실제 소스 조각 컴파일) 6경우.
- 통합편의(1~7): 시작 대기 고리가 '신무장·스탯 확장'(IC가 적용 안 하는 기능) 자리 2,346곳 전부 원본을 3분 기다리고 하나라도 다르면 1~7 전부 끔 →
  0x4b5c1 xor ebx,ebx → jmp 0x4b603 (해독 표시만 확인). 그 표를 쓰는 모델 등록 묶음은 재확인 0x4b93f(sil)가 따로 막음. 1~7 각 기능은 자기 자리 확인(0x6e700 등).
  IC 새 해시 47a62d84… → common.h r43c, registry_bridge r43c, 모듈·3D 다시 빌드. 시험 test/test_ic_wait_r43.py.
- 제보(CrashLog DLL 없음): 설치기는 안 지움 → 백신 격리 유력, 수정 안 함(사용자 결정).
- 오프라인 19개 전부 통과. 미룬 것: 22번 DomesticCE 남은 부분, 설치기 빠진 기본 파일 자동 복구, AI 모드 v15.9 호환 분석(시작만, out/ 없음).

## AI 모드 v15.9 호환 분석(11-09 18:3x, 사용자: 겹침 표시 / 충돌=사용 금지 표시 / 나머지는 통합패치·계절·2D·기색과 같이 돌게, 소스 공개라 깊게 고쳐도 됨)
- 자료: /tmp/claude-0/ai159/{rel,src} (v15.9 배포·소스), out/ai_features.json(86개), out/collisions.json(자리 대조 358건), out/ga.pkl(주소표).
- 방법: AI 주소표(GA, 새 판 1a4a447d 값) 중 코드 자리·자료 칸·함수 시작을 우리 자리(ModCore 기록 286·IC 5번 51·IC 6번 5·14번 152칸·9번 5자리+146칸)와 대조, 파일→기능.
- 충돌(같은 자리): 툴팁·콜라보↔14번/9번 화면(298칸) · 개성9칸↔9번 · 능력치200·성장↔17번 · 경로탐색 가속·군세 출진 편성↔IC 5번 · 진행 속도 버튼↔IC 6번(0x89abf7) ·
  전법서 다중↔30번 DragLearn · 전사 연출↔25번 · 명승지 생략↔1번 Scenery · 군사의 열흘 보고↔4번 SceneSkip·22번 DomesticTalent · 숙련 공방+인접↔1번 설정창 ·
  파견 남은 일수↔Patch27 원격등용일수 · 능력치 경험치 표시↔29번 · 논공 연결(늘 켜짐)↔13번·21번 저장칸 · 기색: 숙련 공방·레이아웃 UI·지역담당관 개성 표시.
- 겹침(기능만 같음, 자리 대조 미확인): 자동 내정 3종↔22번 · 인구 배율↔28번 · 명품창고·수여↔30번 · 지역담당관 초상화·개성↔34번 · 논공 친애↔2·13번 · 지도 메뉴 대기↔30번 FastMenu.
- 확인 못 함: IC 1~4·7번 자리, ExtBridge 확장(26·27·28·30), 3D, 계절(자리 기록 없음 — 오늘 R0 칸 비용 실패는 AI 쪽), 2D(3DMigoto·Present 연결 — AI Layout 이 Present vt[8]/[22]·WndProc 소유).
- 빌드: AI 소스는 MSVC+Windows SDK 필요(여기 없음) → 호환은 기능설정.ini 강제 끔 + 필요한 곳 AI DLL 바이너리 수정(소스로 위치 확인), 또는 PC에 VS 있으면 거기서 빌드.
- 사용자 결정(11-09 18:32): 고치는 건 전부 AI 모드 쪽. ① 통합과 기능 겹침 = 항목에 '통합패치 N번과 겹침' 눈에 잘 띄게 표시 ② 통합·계절(특히 AI 이동/길찾기)·2D·기색과
  충돌하는 건 AI 모드 쪽에서 피하게 고침(소스 공개 — 깊게 고쳐도 됨). 통합·계절·2D·기색은 손대지 않음.
- AI v15.9 호환판 1차(11-09 18:35): FeatureSelect 강제 끔 24개+값 2개 표시, 설치 관리자 exe 글자 26곳 바꿈(relabel_exe.py). 소스·표 r41/ai_compat/v159. 남은 것: 전투 AI 경로 연결↔계절 칸 비용, 논공·저장 연결(늘 켜짐)↔13·21번, Present/WndProc↔2D·계절 — AI DLL 수정 필요(빌드 도구 없음 → 바이너리 수정 또는 PC VS).
- AI v15.9 호환판 2차(11-09 19:0x): 전투 AI 이동 49칸 연결 뺌(IC 5번·계절), 논공·명품 저장 연결 스스로 꺼지게(13·21번), 군세 출진 편성 다시 허용. 내역 ai_compat/v159/dll_patches.txt. 남은 것: Present/WndProc ↔ 2D·계절(실행 기록으로 확인).

## AI v15.9 호환판 3판 (11-09)
- 통합패치 겹침·통합/계절/기색 충돌 23개 기능을 AI DLL 안에서 강제로 끔(설정값 늘 0). 설치관리자·ini로 못 켬.
- 실게임 확인 전.
- 3판 보강: 설치관리자 exe 에서 막은 항목 체크칸 잠금 + 회색 배경(주황 [바뀜] 표시 안 함). lock_exe.py.
- AI 호환판 4판(11-09): 데모 진단 R43D-2(5번 수행·대열 실패) 원인 = 출진 길 찾기 GA_301BD9(0x303fc9)가 IC 전역표 107번 안. 기능 유지한 채 그 자리만 IC 시작 확인 뒤 걸게 함(defer_sortie.py). 이동 속도 맞추기 49곳은 계속 뺌(사용자 결정).
- 다른 모더용 호환 자료 정리판(11-09): kit_tools/make_kit.py + 읽어주세요_호환안내.md → SAN14CE_통합패치_R43_호환자료_1of1.zip. 우리 소스 25개 DLL 바이트 일치 재빌드 확인(묶음 안에서), 호환표 01~06.

## 다음 수정본(11-10)에 넣을 것 — 사용자 "체크해놔"
- [제보 11-09 삼갤러25] 6번 진행 배속 실패 "진행속도 확장 준비 검사 실패"(게임 새로 설치 + 통합 + 날씨만, registered=28).
  원인: 19번 FPS120 이 0x44df40(5) 를 고침 → IC 6번 준비 검사표(IC 0xa3150 표 0번: 0x44dee0+2500, 원래 바이트만 허용) 안. 누가 먼저냐(순서 경쟁). 날씨(SeasonRules 0x44b5e0·0x2af6a0·0x267630) 무관.
- 같은 위험(IC 전역표, 5번 수행·대열 검사): 15번 RelatedOfficersTab HoverPin 0x2dc2a2(6)·0x2dc352(5) ⊂ 전역 33번 0x2db9a0+2992 / 30번 WheelRegion 0x232f60(5) = 전역 115번.
  (29번 StatusColor 0x9332c5·0x9333c9 도 전역 70·134번 안이지만 이미 IC 확인 뒤 설치 — 그대로)
- 고칠 방법: 19·15·30번을 IC 시작 확인(SAN14_WaitConvenienceV1) 뒤에 설치. IC 없으면 바로. 셋 다 이 판 소스 없음 → 우리 소스 '시작 지연 부품'을 앞에 둠(ic_after_restore.h 방식). 실제 모듈 코드 실행 시험 + 오프라인 전체 시험.
- 점검 도구로 남길 것: 우리 자리(표 01·05·06) × IC 표(전역 02·후크 03·포인터 04·6번 준비표 0xa3150/0xa34d0) 겹침 자동 검사 → test 에 넣기.
- [사용자 11-09 22:40 실게임] 4번 '명승 연출 생략' 안 됨. 원인: src/settings_help.cpp 의 known_module() 이 통합편의 해시를 자기 목록(…r40 까지)으로만 확인 → R43 IC(9e76d6b0·dfa4ca9f·지금 47a62d84) 모름 → "SettingsHelp: unknown integrated module; settings/scenic extension left inactive." → 명승 발견 연출 생략·설정 창 설명 둘 다 꺼짐.
  runtime.log: 세션 710~972 'skip setting ON' → 1160 이후 'unknown integrated module'. 다른 모듈(execute_lock·ic_guard·auto_personnel_repair·mod_settings)은 common.h known_common() 을 써서 괜찮음.
  IC 자료 자리(0x19e692·0x105002·0x1c10f0·0x1c1100·0x1c1140)는 지금 IC 에서도 같은 용도로 쓰임 확인.
  고칠 방법: settings_help 도 known_common() 으로 통일(목록 한 곳). 시험: IC 해시 목록을 따로 가진 모듈이 없는지 자동 검사 + 지금 IC 해시가 known_common 에 있는지 검사.
- [사용자 11-09 23:01 실게임 화면] 모드 설정 창 탭 배열 엉뚱함: '사운드' 탭에 투석 사거리(밸런스)·항장 색·지역담당관 경고(정보)·독립 미니맵(그래픽)·고용무장 얼굴(무장)·BGM 이 같이 나오고, 그 아래 이름 없는 슬라이더 줄(100/표준)이 십여 개.
  소스 줄 표(mod_settings.cpp rows[])는 사운드=BGM 하나로 맞음 → 창에 탭별 줄을 넘기는 방식(탭 시작 위치·줄 수 0x17c+4*tab, 61칸 표 0x8d0)이 게임 쪽 기대와 어긋나 앞 탭 끝부분이 뒤 탭에 보이고 빈 칸이 슬라이더로 보이는 것으로 보임 — 내일 게임 코드(탭별 목록 만드는 곳)부터 확인.
  할 일: ① 탭마다 그 탭 항목만, 빈 줄 없이 ② 항목 분류 전체 다시 점검(편의/밸런스·규칙/정보·UI/그래픽·화면/무장·얼굴/사운드) ③ 아래 안내 글 '통합패치 R39 모드 설정' → R43 ④ 화면 그리기 가로채 그림으로 6탭 전부 확인.
- [사용자 11-09 23:18 실게임] 2번 '턴 종료 명승 자동 투자' 안 됨. 통합편의 기록: 자동인사는 매 턴 돌지만 새 판(11-07~) 88번 전부 '투자 0곳'(1.0.1 때 10-01~10-29 는 투자 1곳이 21번). 설정 AutoScenery=1 켜짐.
  → 통합편의(소스 없음)의 새 판 이식 때 명승 투자 경로(투자 가능 판정·투자 함수·명승 자료 자리) 중 하나가 안 옮겨졌을 가능성 큼. 내일: IC 자동인사 함수에서 투자 부분 따라가며 게임 주소·구조 위치를 1.0.1 IC 와 대조(자동 검사 도구로), 정당한 0(금 부족·전부 최대 발전)인지도 구분.
- [사용자 11-09 23:22 "내일 수정본에 넣을 거 체크"] 턴 종료 기능 작동 확인용 읽기 전용 기록 추가(Diag, 게임 코드 안 건드림): 매달 끝 — AI 세력 명령서 99장 여부(22번), 인구 1만 아래 도시 수(22번), 1·4·7·10월 인구 증가량(28번 보너스). 지금은 이 셋이 '켜짐' 기록만 있어 판단 불가.
  (작동 확인됨: 2번 포상·탐색, 4번 턴 끝 결과 창, 22번 내 세력 내정·AI 내정 순환, 기색 월 처리 / 안 됨: 2번 명승 투자 / 정상 꺼짐: 2번 자동 논공(13번 때문))

## 내일 수정본 묶음(한 번에 넣기)
1. 19·15·30번 통합편의 확인 뒤 설치(6번 배속 제보·순서 경쟁) 2. 우리 자리 × 통합편의 검사 구간 겹침 자동 검사 3. 명승 연출 생략(settings_help → known_common)
4. 모드 설정 창 탭 재배열·빈 줄 제거·R39→R43 5. 2번 명승 자동 투자 6. Diag 턴 종료 기능 확인 기록
- [사용자 11-10 07:04] 17번 '100 넘어서도 경험치로 성장' 안 되는 것 같다. 기록상 17번 15/15 적용 OK(caps=modcore). 확인할 것:
  ① 18번 '경험치로 능력치 성장=억제' 이면 100 위·아래 모두 성장 거의 없음(사용자 본인 게임은 능력치 고정 성향 — 기억 참고) → 설정값 확인
  ② 기색 v1.2 노쇠 나이 이후 경험치 성장 멈춤(xpstop) — 대상 장수 나이 확인
  ③ 우리 쪽: 0x231afb '경험치 분기'는 다시 보니 특정 개성(0x7b) 장수 능력을 100 으로 고정하는 분기로 보임(경험치 성장 상한 자리라고 단정 못 함). 새 판에서 실제 경험치 성장 상한 자리(능력 +1 시 100 비교)를 다시 찾아 확인 + XpHits·성장 횟수 자가진단 기록 추가.
- [제보 11-10 사용자 전달] 17번만 깐 사람도 100 찍은 뒤 200까지 경험치 성장 안 됨(18번 무관). 원인 확정(코드):
  · 경험치 성장 함수(게임 0x2bc0f8~): 0x2bc141 능력=100 이면 중단 → 우리 패치 0x2bc14e jne→jb 는 '100 이상이면 중단'이 돼 오히려 막음.
  · 같은 함수 레벨업 반복 0x2bc366 mov ebp,0x64 → 능력 +1 을 100 으로 자름(안 고침).
  · 경험치 표(experience_data_exce.bin)는 100행뿐 → 0x2bc318·0x2bc399 'cmp ebx,0x65; jae → 0행 사용' — 101 이상 필요 경험치가 없음.
  · 옛 모음팩 v1.9 SAN14_Stats200.dll(사용자 본인 것)은 게임 경험치 함수를 가로채 100 넘으면 필요 경험치 = t100 + (능력-100)×(t100−t99) 로 늘려 가며 200까지 키움(문자열 XP_PROGRESS t99 t100 step). R43 Stats200New 는 옛 R6 상수 15곳만 옮겨서 이 기능이 빠짐(회귀).
  고칠 방법: Stats200New 에 성장 부분 추가 — ① 0x2bc14a 6바이트 'cmp r13d,0x64; jne' → 'cmp r13b,0xc8; jb'(200 미만이면 계속) ② 0x2bc366 0x64→0xc8 ③ 두 표 조회 자리에서 101~200 필요 경험치를 v1.9 식으로 계산하는 릴레이 ④ 실제 함수 코드 실행 시험(100→101→…→200, 18번 억제표에서도 같은 식) ⑤ 100 넘어 오른 횟수 자가진단 기록.
- [제보 11-09 23:43 gggoeik R43D-5] 27개 중 25개 시작: 13번 실패(prerequisite… merit awards not enabled), 9번 상태 FAIL, 9번 화면 'code signature mismatch or Traits9 dependency absent', 5번 수행·대열 실패.
  판단: 9번 화면 자리를 다른 모드가 먼저 차지 → 9번 화면 실패 → 13번은 9칸 확인(ensure_nine/support_ui)에서 멈춤. 9번 화면 자리 146칸을 차지하는 알려진 모드 = AI 모드 원판(9칸 개성·개성 설명 기능) — 5번 실패도 AI 모드 원판 이동 연결과 같은 모양. (8차 이전 판이면 figekth 와 같은 9번 순서 문제였을 수도)
  할 일: 9번 화면·13번 실패 창에 '그 자리를 가진 DLL 이름'을 적게(r43_conflict.h 방식) → 제보만 보고 누구와 충돌인지 바로 알 수 있게. 통합패치 단독이면 실패 안 하는지 오프라인 시험으로 다시 확인.

## 2026-11-10 교훈·점검 폴더 + 10차 준비(관문 통과 전 — PC 정식 폴더에 안 넣음)
- 사용자 11-10: 교훈 폴더는 기록장이 아니라 '필터'. → r41/교훈_점검(관문.py, 점검/c01~c10, 장부 02~05, 점검표 6종). 포장 도구 make_cum_overlay.py 는 관문 도장이 있어야 묶음. PC 두 소스 폴더에 사본.
- 내 실수 발견·정정: (1) 11-09 호환자료 표 01 에 옛 판(1.0.1) 주소 209줄 섞임 — 게임 기록 머리줄 판 글자(timestamp 6a983c25)가 새 판에서도 옛 값. 자리 뽑기는 교훈_점검/도구/자리표_만들기.py 만(새 판 표식 SAN14CE.Stats200New 이후 기록). (2) 그 표 때문에 '15번이 통합편의 전역 33번과 겹침'이라 한 것은 틀림(새 판 15번 6자리는 안 겹침). 겹치는 건 19번 0x44df40(6번 준비 0번)·30번 0x232f60(전역 115번)뿐. AI 호환판 충돌 대조는 영향 없음(다시 대조).
- 소스가 없다던 모듈의 소스 발견: 19번 FPS120 R8(롤백판 꾸러미 patch20_duel60stock, 1.0.0 주소 — kits/FPS120_R8_옛소스), 30번 WheelRegion·DragLearn 키트(src42d/hold/편의성_추출 → kits/). 키트 빌드 = 꾸러미 파일 바이트 일치(WheelRegion 2b16c325, DragLearn 7cc6e07b). DragGive·FastMenu 키트는 해시 다름(판 확인 필요). 바이너리 26 → 24.
- 10차에 넣은 것(전부 오프라인 시험 통과, 관문 c01~c10 중 c07 만 빨강):
  · P203 4번 명승 연출: settings_help.cpp known_module → known_common(). SettingsHelpScenic e9d166d2 / .v2 d711e80d
  · P204 모드 설정 창: 게임 목록 만들기 0x658df0 = 탭 t 줄 = 표[앞 탭 줄 수 합…+줄 수[t]], 줄 수 layout+0x17c(생성자 0x59e150 기본 9,8,7,8,10,19 = 61). 사용자 화면(사운드 탭 6줄+빈 13줄, 앞 탭들 뒤섞임)은 '우리 표 + 게임 기본 줄 수'와 숫자까지 일치.
    줄 수를 되돌리는 게임 코드는 못 찾음(쓰는 곳은 생성자뿐, 우리 채우기 컴파일 결과도 정상) → keep_layout(): 매 Update 확인·다시 채움 + 'tab layout restored' 기록(다음 기록으로 원인 확정). 재분류 8줄, 아래 글 R39→S14_PATCH_VERW. ModSettings 7347a166. 시험 test_settings_tabs.py
  · P207 17번: 경험치 함수 0x2bc0d0. 입구 0x2bc14a 'cmp r13b,0xc8; jb'(옛 0x2bc14e jne→jb 는 100 이상을 막았음), 레벨업 구간 0x2bc30a(9) → GrowBridge → Stats200Grow → 0x2bc3c8. 식 t100+(L-100)(t100−t99), v1.9 안전 조건. 기색 AgeXp(0x2bc0d0 입구 후크)와 안 겹침. Stats200New 4499cba3, 16/16. 시험 test_stats200_grow_native.cpp(실제 게임 구간 190바이트 실행)
  · P201 19번: SAN14_FPS120Late.dll(src/fps120_late.cpp, 1a71e528) + FPS120.ini File= 교체. 본체는 예전과 같은 때 시작, 통합편의 깔려 있으면 0x44df40 call 대상만 원래 값(e8 bb f6 f2 ff → 0x37d600)으로 뒀다가 확인 뒤 원자적으로 다시. 통째로 미루지 않은 이유: 매 프레임 불리는 7곳을 늦게 고치면 반쪽 명령 위험(L115). 시험 test_fps120_late_native.cpp
  · P202 30번 휠클릭: kits/WheelRegion/dll/wr_ic_wait.h(core_patch 가 부름). 0b0694d2. 시험 test_wheel_ic_wait_native.c
  · P208 Diag: scan_t9_foreign → 13번(rt_bad)·9번(Traits9Check FAIL) 줄 끝에 DLL 이름. SettingsHelp unknown 줄도 문제로. Diag ca8f8cc7. 시험 test_diag 43
  · 설치기 Engine.cs case 17(growth beyond 100 installed). build_overlay OTHER_LOADER 에 san14_fps120.dll, assemble.sh 19번 등록·30번 휠클릭.
- 남은 것(제보 장부 대기 → 관문 c07 이 막음): P205 2번 명승 자동 투자(통합편의 안, 아직 원인 미확정), P206 Diag 턴 종료 작동 기록(AI 명령서·인구 자료 자리 필요), P209 호환자료 다시 만들기(10차 낼 때 make_kit.py).
- 9번 본체(Traits9/Traits9_UI) 소스화는 이번에 안 함(자립 3판). 사용자에게 1판에 9번도 넣겠다고 한 말은 지키지 못함 — P208 은 Diag(소스)에서 처리.

## 2026-11-10 10차 냄 (8건 한 번에 — 사용자 "8건 다 하고 한번에")
- 관문 통과(점검 9개 + 오프라인 전체 시험 25개) → 도장 → PC 정식 폴더에 14개 파일 덮어씀 → `관문.py --넘김 11월10일_10차`(직전 판 = 10차).
- 바뀐 꾸러미 파일(9차 대비 13개 + SHA256SUMS.txt): Engine.cs·ManifestHash.cs·manifest101.xml, base/Diag·ICGuard·ModSettings·SettingsHelpScenic(.dll/.dll.v2),
  f17/Stats200New, f19/FPS120.ini + FPS120Late.dll(새 파일), f22/DomesticPlayer, f30/WheelRegion.
- P205 명승 자동 투자(추정 원인, 실게임 확인 전): 통합편의 투자 함수 IC+0x40480 이 개발 불가 명승 1곳에서 -1 → 턴 종료 반복(IC+0x6840)이 전부 포기.
  ICGuard I: IC+0x4067c `mov eax,-1` → `xor eax,eax`(메모리에서만) + 투자 함수 감싸 기록(invest_done, invest_skip idx/level/cost/days/why 최대 8줄). 시험 ic_invest_skip.
  다음 기록에서 볼 것: 적용확인.log 의 invest_skip why= (not-developable-now / gold-short-or-changed), invest_done 수.
- P206 턴 종료 작동 기록: DomesticPlayer world_tick() — 달마다 `R42D 22 SAN14_DomesticPlayer.dll world y/m areas pop_all pop_player under10k under10k_player ai_forces ai_cmd99 ai_cmd_min`.
  읽기만, 22번 설정을 꺼도 남음. 자리: 세력 표 db+0xdf30+8*번호(IC+0x318e6), 명령서 = 세력+0x14(IC+0x40618 이 투자 뒤 1 줄임), 세력 vtable exe+0x13a9880.
  보는 법: under10k=0 → 최소 인구 1만 동작 / ai_cmd99 = ai_forces → AI 명령서 99장 동작 / 1·4·7·10월 줄의 pop_all 증가폭이 다른 달보다 큼 → 28번 분기 보너스.
- P209 호환 자료 다시 만듦: SAN14CE_통합패치_R43_호환자료_11월10일_10차_1of1.zip (표 01 = 새 판 209곳, 표 07 = 6번 진행 배속 확인 구간 새로, 30번 키트 소스 추가,
  묶음 안 소스 재빌드 28개 바이트 일치 확인). make_kit.py: Caps 줄 설명 고침, 소스 주석의 작업 경로 지움(hired_face.cpp, r39_record.h — 빌드 결과 같음).
- L118 + 자료/우리자리_추가.tsv: 이번 판 새 자리(게임 기록에 아직 없음)를 손으로 적는 표. c03·make_kit 이 같이 읽음. 10차 = 0x2bc14a(6)·0x2bc30a(9).
  → 10차로 게임을 돌린 runtime.log 가 들어오면 자리표를 다시 만들고 추가표의 두 줄을 지울 것.
- 확정 못 한 것: P204 탭별 줄 수가 게임 기본값으로 돌아가는 길(복구 + `tab layout restored` 기록만). P205 원인(위 추정).
- 안 한 것(자립 2판 이후): 19번 본체 소스화(kits/FPS120_R8_옛소스 → 새 판), 9번, 15번, DragGive/FastMenu, ModCore, IC, DomesticCE, FogTrial.
- 실게임 확인 전: 10차 전부(오프라인 시험만). 다음에 SAN14_기록(적용확인.log, 튕김_*.log)·runtime.log 를 받으면 위 항목부터 볼 것.

## 2026-11-10 교훈 폴더에 소스 자립 순서·규칙 넣음 (사용자 지적 P211)
- 내 실수: 자립 순서와 규칙 3개를 추천만 하고 폴더에 안 넣음("만들어" 규칙을 장부에 적용). → L122: 정한 것은 "만들어"와 상관없이 바로 적는다(01_작업규칙에도).
- 점검표 3 에 순서 표(2판 19·15·30번 → 3판 23·26·27·28·22CE → 4판~ 9·13·14·16 → 통합편의 기능별로 떼기 → ModCore·Resolver·Bridge).
- L119 한 판 4개까지 / L120 자리 대조 / L121 고칠 때 소스화. c04 가 자료/직전판_소스장부.tsv 와 비교해 강제(자립기록.tsv 줄·시험 등록·4개 초과). 관문 --넘김 이 직전판 소스 장부도 저장.
- 꾸러미는 안 바뀜(10차 그대로).

## 2026-11-10 11차 냄 (사용자 "만들어. 교훈폴더에 추가도")
- 켜 둔 게임·데모 기록으로 확정한 것: P204 탭 배열 = 기색 모드(Condition.SetupRow, 0x658df0 들머리 후크)가 탭별 줄 수를 게임 기본값으로 되돌림. 10차의 "매 Update 복구"는 효과 없었음.
  고침: ModSettings 13번째 자리 0x658dfa(MsCntBridge, 9바이트 `movq $-2,0x20(%rsp)`) — 우리 창(rcx==MsOurLayout)이면 MsCnt[6] 을 layout+0x17c 에 다시 씀. 시험 settings_counts(실제 브리지 코드).
- P213 최소 인구 1만: 지역 인구(+0x52)는 16비트(게임 16비트 읽기 193·쓰기 36, 옛 DomesticCE 도 2바이트). DomesticPlayer 가 32비트로 읽어 한 번도 발동 안 함 → 16비트로. 규칙은 옛 DomesticCE(1~9999 → 10000).
- 달 기록(world): 데모에서도 남김, player·cmd_player 추가, 인구 16비트.
- P212 AI 명령서 99장 = 보류. DomesticCE 는 db+0x4858+0x30*i 의 +4(=세력+0x14, 16비트)에 99 를 씀(자리는 맞음). 게임은 쓸 때마다 clamp(값-비용,0,200). 턴마다 채우는 함수는 못 찾음 → 22번 DomesticCE 다시 짜기(자립 1순위)에서.
- P205 명승 투자: 게임 함수 4개 옛·새 판 대조 = 같은 함수(주소 실수 아님). 11월 실패 16번 중 7번은 데모. 실제 조건은 invest_skip 기록 대기.
- P214 제보(기색 꺼짐, 다른 사람): 통합패치 무관, 남의 AI 모드 원판이 기색 저장 자리 0x571c60 과 겹침 추정. 기색 쪽 할 일 3개(오류창 자리·상대 이름, 창 제목 "통합패치 R43" 고치기, 줄 수 되돌림 조건) — 기색 작업 때.
- 관문 c11 신설(우리 다른 모드 자리 × 통합패치). 처음 돌자마자 기색 DetailPlace 0x9332d0 ⊂ 통합편의 확인 70번을 찾음(P215 지켜봄).
- 소스 없는 모듈 대책(사용자와 정함): ① 결과 기록 ② 지도(고치는 자리·부르는 함수·칸 크기) ③ 문제가 드러난 것부터 다시 짜기(22번 DomesticCE → 통합편의 2번 명승 투자 → 정한 순서).
  11차에 들어간 것은 ①의 일부(달 기록 확장)뿐. 23·26·27번 결과 기록과 ② 지도는 다음 판(꾸러미 변경 없이 가능한 것부터).
- 튕김: 새 튕김 없음(시작 때 d3d11 쪽 처리된 예외 1번뿐, 정상 종료).

## 2026-11-10 12차 냄 (사용자 "다 끝내고 만들어")
- P212 AI 명령서 99장 확정·고침: 게임 채우기 규칙 = min(지금+1, 세력 최대 0x2db910(세력), 200) @0x9f9d5~0x9fa1e. 주기적 99 쓰기는 여기서 깎임.
  DomesticAI: 0x9fa17(7바이트 `41 3b c7 41 0f 4f c7`) → DaiOrdBridge(src/domestic_ai_bridge.S) — rbp=세력, 플레이어 세력(db+0xdf30+8*world[0x3a])이 아니면 eax=99. 복귀 0x9fa1e.
  스위치 SAN14_DomesticCE.ini [Automation] Enabled·AiOrders(달 바뀔 때 다시 읽음). 기록: runtime.log "AI orders 99 installed…", 적용확인.log `R42D 22 SAN14_DomesticAI.dll orders y/m on= set99=`.
- 인물 집계 census 줄(DomesticPlayer world_tick, 달마다, 읽기만): officers/officers_player/ronin/captive/gone — 26번 결과 확인용.
- P205: 실패 자리 0x4067c = 그 명승 값 < 1(지금 개발 불가). ICGuard I 가 맞게 다룸. invest_done 기록으로 마무리 확인.
- 다음: 22번 DomesticCE 남은 기능을 소스로 옮기고 CE 빼기 → 통합편의 2번 → 자립 표 순서. 지도(자리·함수·칸 크기) 만들기. 기색 쪽 3건은 기색 작업 때.
- 12차 뒤 볼 기록: orders set99>0, world ai_cmd99=ai_forces, counts_rewritten(모드 창 닫을 때), pop_set>0(인구 1만 미만 지역이 있을 때), census.

## 2026-11-10 13차 (12차 데모 기록 확인 뒤)
- 12차 데모(227/2~229/1): 실패 0, 탭 13/13 설치, AI orders 설치 OK. 그러나 set99 달 0~4번, ai_cmd99=0 → 0x9fa17 은 "명령서 하나 돌아올 때" 자리. 부족.
- 13차: DomesticAiSelect 들머리에서 그 AI 세력 명령서 <99 면 99(스위치 DaiOrdOn). 기록 orders topup=, world ai_cmd_max/avg.
- 12차 데모에서 확인된 것: 최소 인구 기록 정상(인구 합 696만 = 16비트 합, under10k 17~24 는 AI 지역 — 플레이어 지역만 올리는 기능이라 정상), census 정상.
- 튕김 P216: 0 나누기 exe+0x551bc 오늘 2번(17:44, 20:44 — 12차 이전). 게임 코드, 나누는 수 = 0x22c410(대상) 개수. 원인 미확정 → 다음에 함수·호출처 읽기.
- 본판 완성 때 확인용 기록 전부 빼기(L126).

## 2026-11-10 14차
- 13차 확인: 탭 배열 화면으로 고쳐짐 확인(사운드 탭 = BGM 플레이어만). AI 명령서 44달 기록 — 평균 최대 99, 그러나 내정 판단 안 하는 세력은 0장으로 남음.
- 14차: DomesticAiSelect 가 불릴 때마다 AI 세력 전부(1~0x33) 99 미만이면 99. 설치기 [설치된 항목만 갱신] 버튼(App.cs Start(...,forced)), 설치기 중복 실행 안내·창 제목(BuildAndRun.ps1). c08 이 ps1·App.cs 도 봄.
- 열려 있는 것: P216 0 나누기 튕김(게임 exe+0x551bc) 원인, P205 invest_done 기록 확인, P214·P215 기색 쪽, 22번 DomesticCE 나머지 자립, 지도.

## 2026-11-10 15차 — P205 명승 자동 투자 원인 확정
- 실게임 2턴(구당협 Lv1 투자가능, 설정 켬): 투자 0곳, invest 기록 0줄 = 투자 함수가 안 불림. 턴 끝 고리 IC+0x6840 들머리: `test byte [ic+0x105010],2`(0x686d), 켬 칸 ic+0x10500c(0x687a).
- 저장값 AutoPersonnelState=196617(0x30009) → 비트 2 꺼짐, AutoScenery=1. 우리 설정 창(S_IC 줄 12)이 켬 칸만 씀.
- 15차: ModSettings 줄 12·13 ↔ 정책 비트 2·4 같이(IC 0x65580(pol|th<<32), 낮은 값 ≤0xf·th≤100), 화면 값 = 둘 다. ICGuard J: 시작 때 맞춤.
- 14차 데모: AI 명령서 동작 확인(13달은 전부 99 이상). 탭 배열 화면 확인.
- 다음 확인: 15차 설치 뒤 그 세이브에서 턴 1번 → 11Patch.log "투자 N곳", 적용확인 invest_done / auto_policy_bits_fixed.

## 2026-11-10 16차 — 명령서는 군단 객체(P220·P222·P223)
- 확정: 명령서 = 군단(CDistrictData, db+0xe0d0+8*번호, vtable 0x13a98f0)+0x14, 군단+0x10 = 세력 번호, +0x12 = 군단장. 세력(CForceData, db+0xdf30)+0x14 는 다른 값(새 게임 전 세력 60, 뜻 모름).
  근거: 0x9f9cf 의 객체 = 도시.vf[0x80](0x22d470) = 군단표[도시+0x30]; 0x2db910 이 객체+0x12 를 무장 번호로 읽음(최대 = min(2*능력, 상한)); 옛 DomesticCE 쓰기 자리(0x30 간격 +0·+4).
- 16차: DomesticAI 채우기 = 군단표 1~0x33 중 세력 번호가 플레이어(world+0x3a)·0 이 아닌 군단만 99. 브리지(0x9fa17) = 군단+0x10 비교. 세력 객체엔 안 씀. DomesticPlayer 달 기록 = 군단 기준(ai_corps, cmd_player = 플레이어 첫 군단).
- 남은 것: 14·15차로 진행한 세이브의 AI 세력+0x14 = 99(되돌릴 값 모름). P219 넓은 결과 창(보류, 다음 판 조사), P221 금(지켜봄), P216 0 나누기.

## 2026-11-10 17차 — 결과 창 전부 표시(P219)·우호도 되돌리기(P223)·금(P221)
- P223 확정: 세력(CForceData)+0x13.. = 세력끼리 우호도 표(BYTE 52칸, 칸 = 상대 세력 번호), 게임은 양쪽에 같은 만큼 더함(0x25a080, 0~200). 14·15차는 +0x14(= 1번 세력과의 우호도)에 99 를 썼음.
  17차 DomesticAI rel_repair: 달 바뀐 뒤 첫 DomesticAiSelect 에서 한 번. 1번 칸만 비대칭·다른 쌍 전부 대칭일 때 F[a][0x14]=F[1][0x13+a]. 기록 `R42D 22 SAN14_DomesticAI.dll relfix y/m force1_col_diff= other_pairs_diff= restored=`.
- P219: "외 N건" 은 우리 4번 SceneSkip 턴 끝 결과 창. src/scene_summary.h(순수 계산) — 항목 64개까지 전부, 한 줄 46자·창당 5줄·여러 창. 시험 scene_summary(네이티브) 묶음에 추가. 옛 unicorn 시험(test_scene_skip_r42c.py)은 이 환경에 unicorn 이 없어 못 돎(묶음에 없음).
- P221: 23번 CityCaps 가 도시 금 상한 50,000 → 150,000(exe+0x1a4d268). 의도한 기능 → 안고침.
- 사용자 지시: P216(0 나누기 튕김)은 하지 않음.
- 화면 확인 남음: 결과 창 한 줄 46자·5줄이 창에 맞는지, relfix 줄, 플레이어 명령서가 99 로 안 튀는지(world 줄 cmd_player).

## 2026-11-10 18차 — 설치기 "게임 켜져 있음" 창 문구(P224)
- Engine.GameClosed: 켜져 있는 프로세스 이름을 글자에 넣음(GameRunningMark). App: 그 경우 해결 줄 = 창 닫기 → 작업 관리자에서 끝내기.

## 2026-11-10 19차 — [포로 회유]·[등용문] 열기 실패(P225, 맵 확장)
- 원인 확정(맵 확장 쪽 답): 통합편의는 두 버튼마다 대조 표(IC+0xc3560·0xc58a0·0xc5c60 / 0xc6f50·0xc5c60, 0x18 간격 rva·len·기대 포인터, memcmp 0x180094d70)를 봄. 맵 확장 136 이 그 안 21곳을 바꿈.
- 19차 ICGuard K(src/ic_mapsites.h 순수 계산 + ic_guard.cpp): B 의 captive_list(IC+0x47870)·recruit_gate(IC+0x47f90) 감싸기 입구에서 SAN14Mods\sites\*.txt 새로 읽음 → 목록 자리만 기대 바이트 ← 게임 바이트(VirtualProtect). 남은 차이 = fail_ex(7, verify_captive/verify_recruit) 창 1번.
- 파일 형식(맵 확장 137 부터): stamp=0x6ad06f53 / mod=이름 / pid=번호 / "rva(16진) 길이(10진)". 원본과 우연히 같은 바이트는 빠져 줄이 쪼개질 수 있음(시험함).
- 새 도시 포로: IC 0x180036ff0 가 1~61 + 객체+0x4e==번호 확인 → 새 도시는 null(건너뜀). 엉뚱한 거점 처리 없음.
- 맵 확장 136 사용자는 137 전까지 그대로 안 열림(창에 안내).
- (23:56) PC 정식 폴더 = `C:\Users\user\Desktop\통합패치\통합패치 11월 10일 새버전` (사용자가 이름 바꿈). 16~19차가 옛 이름으로 써져 "11월 8일 새버전"(support·SHA256SUMS 만 든 반쪽 폴더)이 생김 → 사용자가 지우면 됨. 10일 폴더는 19차로 맞추고 해시 확인(L131). 루트 SHA256SUMS.txt 도 이제 같이 넣음.

## 2026-11-11 20차 — 자동논공 재발(P227)·25번 스위치(P228)·본판 확인용 기록 제거(P226/L132)
- P227: P027 새 엔진(AutoMerit)은 이 PC(13번 설치)에서 늘 쉼 → 실제 게임 검증 없었음. 13번이 있어도 IC 자동논공이 IC 창·세이브 정책 비트 4 로 켜지면 옛 방식(불러오면 내정 보상 사라짐).
  20차 ICGuard w_exec: MeritTraits 있으면 건너뜀 + 0x10500d=0 + 정책 비트 4 끔(0x65580). J 도 13번이면 비트 4 안 켬. AutoMerit fallback → r39::fail(2,"fallback") 창(이유 번호).
- P228: Patch27 Options.ini 쓰기 실패 → p27_write_fail 창(오류 번호·읽기 전용·권한). ModSet 실패 → fail(1,"set_failed").
- P226/L132: r39_record.h S14_DIAG_RECORDS(기본 0) — counter 몸통·달 기록 함수 s14_diag() 로 막음. 시험(nat)은 -DS14_DIAG_RECORDS=1. 확인판 표시 "S14_DIAG_RECORDS_ON"(켤 때만) → 관문 c12 가 본판 DLL·빌드 대본에서 확인.
- PC 미연결 상태에서 만듦 → PC 반영은 연결되면.

## 2026-11-11 21차 — AI 명승 투자 금 조건(P229)
- 게임 AI 명승 투자 = 행동 노드 CSceneryInvestSetNode(vtable 0x134b260) 칸16 → exe+0x3e890 만. rsi=군단(+0x14 명령서, +0x10 세력), rdi=도시, 금=도시.vf[0x90], 비용=0x22c5f0(명승), 기간=0x22c810, 금 빼기 0x27660(도시,-비용), 명승 +0x64=2 +0x66 기간 +0x68 끝날짜.
- 21차 DaiScenicBridge(exe+0x3e9c6, 5바이트 cmp eax,r14d/jl): AI 세력 군단이면 (금-비용) ≥ max(비용×Mult, Floor) 일 때만 0x3e9cb, 아니면 0x3e9d1. 기본 Mult 2·Floor 5000(사용자). ini [Automation] AiScenicGold/AiScenicFloor/AiScenicMult, 달 바뀔 때 다시 읽음.

## 2026-11-11 22차 — 턴 끝 결과를 사건 결과 넓은 창에(P219)·30번 시작 줄(P230/L133)
- P219: 사용자가 말한 큰 창 = 게임 사건 결과 창(CEventResultState vtable 0x14679c8, 레이아웃 "09_01_02_02_event_talk_result", 도시 증축 사건 뒤 "도시증축1 영안" 창).
  사건 장면(0x83d24d)은 0x832f10(관리자 0x11040(), 이름 0x1467b48 "CEventResultState", &{사건번호,flag}, &닫힐 때 부를 것)으로 쌓음. 상태+0x470 = 사건번호, +0x474 = flag.
  열기 vt[1] 0x83c680: 레이아웃 만들고 0x83c793 call 0x842af0(레이아웃, 번호) → 제목 [레이아웃+0x138]+0x18, 본문 +0x20 에 0x30f250(글자칸, wchar_t*, 1,1,1,0) (\n 줄바꿈).
  닫힐 때 부를 것 = MSVC std::function(0x38 자리 + [+0x38] 구현 주소). 나레이션 창(0x1f1e90)과 같은 꼴 vtable 0x13a6160(복사 0x203920, 부르기 0x830ec0 → 0x5746f0(잡은 상태)), 잡은 값 = 0x572480(관리자) 맨 위 상태, 쌓은 뒤 0x574750(그 상태) 멈춤.
  22차: 표시 번호 0x5343F001 로 쌓고 0x83c793 만 SsResultFill 로(그 번호면 우리 제목·본문, 아니면 원래 0x842af0). 한 줄 40자·6줄(SceneSkip.ini SummaryWidth 20~60·SummaryLines 3~10), 넘치면 다음 장(제목 n/전체).
  못 쓰면: 자리 7곳·표 3곳 확인 실패 / 호출 자리 못 걸음 / 창 구조 이상 / 3초 넘게 안 열림 → 작은 나레이션 창(제목 + 종류마다 한 줄, 3줄까지, 넘치면 "그 외 N건") + 오류 창 1번(기능 켠 때만). 시험 scene_wide(가짜 게임)·scene_wide_sites(실제 실행 파일 대조).
  남은 것: 실제 화면에서 40자·6줄이 창에 맞는지(데모 때).
- P230: 30번 ItemPicture 시작 줄 r39::start(30,…) 를 셈 줄과 따로. 관문 c13(Diag 가 보는 모듈마다 본판 시작 줄).
- 21차 뒤 고친 DomesticAI(명승 통과 셈 DaiScPassed, 기록은 확인판만)도 이번 판에 들어감.

## 2026-11-11 23차 — 풍작 따로(P231)·넓은 창 36자·9줄 / AI 명승 확인판
- 22차 화면(사용자 14:10): 사건 결과 창에 제대로 뜸. 창 안쪽 폭 약 36자·높이 약 9줄 → 기본값 SummaryWidth 36·SummaryLines 9.
- 풍작 연출 생략 자리 0x2bcbda → SsNarrHarvest(결과 종류 5 '풍작', 재해 다음 차례). 풍작만 있어도 창.
- AI 명승 막음/통과 셈 확인: 확인판 SAN14_DomesticAI.dll(-DS14_DIAG_RECORDS=1, 달마다 orders 줄 scenic_blocked/scenic_passed)을 게임 폴더에만 잠깐 — 정식 폴더 X. 확인 뒤 본판 DLL(c9d10c67…)로 되돌림.

## 2026-11-11 24차 — 결과 창 글자 수에서 색 표시 빼기(P233/L136)
- 게임 재해·풍작 문장 = "올해는 ^05회계^00, ^05강주^00에서 …" (도시 이름 초록). 23차까지는 ^05/^00 까지 글자로 세서 줄이고 줄바꿈. 24차: vlen(보이는 글자)·vcut(표시 안 깨뜨리고 자름, 색 안이면 ^00 닫음).
- 확인판 SceneSkip(9added9e…)은 창마다 R42D 4 summary_page(방식·줄 수·최장 줄(보이는 글자)·제목·본문) 기록 — 정식 폴더 X, 게임 폴더에만, 확인 뒤 본판으로 되돌림.

## 2026-11-11 25차 — 결과 창 메시지 속도(P234)·AI 명승 금 제한 켬/끔(P235)
- 메시지 속도: 설정 = 0x3ec1a0()(전역 0x1a49c90) +0x20 번호 0 수동·1 느리게·2 보통·3 빠르게, 표 0x13c6248 {0x7fffffff,60,35,15}(나레이션 창 0x86418e 가 레이아웃+0x1a0 에 씀).
  CEventResultState 입력 vt[5] 0x83e280(프레임마다, 0x572670 활성 확인 → 확인 키면 [state+0x478].vt[0x198]) 입구 6바이트 후크: 우리 표시 번호 창이면 채운 시각부터 표 값×100ms 뒤 vt[0x198] 로 닫음(한 장에 한 번).
- AI 명승 금 제한: DomesticAI Get/Set(5) = DaiScOn ↔ SAN14_DomesticCE.ini [Automation] AiScenicGold. 모드 설정 창 밸런스·규칙 탭 줄 추가. 설치기 Spec.cs 22번 '스위치 6개'.
- 24차 확인판 데모(14:38~14:43): 명승 막음 월 110~146회·통과 2~16회. 결과 창 풍작 따로 OK, 말 줄임은 색 표시 셈 탓(24차에서 고침).

## 2026-11-11 26차 — 저절로 넘김 자리 바로잡기(P234/L137)·풍작 도시 수(P236)
- 25차는 CEventResultState vt[5] 0x83e280(입력 처리 — 입력이 있을 때만 불림)에서 시간을 잼 → 가만히 두면 안 넘어감. 26차: 우리 창 레이아웃만 vptr 를 표 복사본(52칸+RTTI)으로, vt[1](원래 빈 함수 0x8ca1c0, 나레이션 레이아웃은 0x865520 에서 +0x198 기다림을 잼)에 SsResultLayoutUpdate → 시간 되면 0x8637f0(vt[0x198] 닫기). 0x83e280 후크 뺌.
- 풍작: 한 문장에 도시 여럿 → Cat.places(^05 개수)로 【풍작 N곳】.

## 2026-11-11 16시 — 튕김 판정 도구·데모 기록(L138)
- 도구/튕김_정리.py: CrashLog → 자료/튕김자리.tsv(자리별 횟수·스택 모듈·그 함수를 고치는 모드, 판정 칸 유지). 11-05 전 기록은 옛 판 주소라 대조 안 함.
- 다른 모드(맵 확장)가 고치는 함수의 튕김은 표에서 뺌(사용자: 맵 확장에서 할 일). 남은 판정: 0x3f9722(P238) 모름/지켜봄(PrisonerReturn 프레임, 게임이 처리).
- 자료/데모기록.tsv 오늘 데모 8건. 자료/맵확장자리_137.txt = 맵 확장 sites 파일 사본.

## 2026-11-11 27차 — 모드 설정 창 vtable 충돌 창에 상대 DLL(P239/L139)
- 디시: R43F-1-vtable exe+0x141f250(숙련 설정 Enter 0x609100 칸)을 다른 모드(제보자: 행동교정 모드)가 먼저 바꿈. 주인 판정은 했는데 fail_box 가 r39::fail 이라 창에 이름 없음 → fail_ex(own).

## R43.28 설치기 업데이트 기능 (2026-11-11 밤)
- 설치기: Spec.Version R43.28(깃허브 태그와 같음). 새 파일 Update.cs(깃허브 배포 읽기·새 판 폴더 만들기·되돌리기 창), App.cs 화면 개편(버튼 줄·상태 줄·경로 줄·탭 숫자·업데이트/작업 기록 탭·[새로]/[바뀜]·1번 잠김 【】), BuildAndRun.ps1(Update.cs·System.Web.Extensions, --game/--after-update 넘김), Engine State.Installer.
- 깃허브 wprkfdid12-ux/san14ce-integrated-patch. 이 세션에서 배포 만들기는 HTTP 403(코드 올리기만 됨) → 배포 파일은 사용자가 올림. make_release_r43.py <판> [--base] → release/<판>/ (판 ZIP 1.6MB + 기본판 조각 2개 238/255MB, 설치기 Build 흉내 검사 2종 통과).
- 여기엔 C# 컴파일러가 없음 → PC 시험 폴더 `바탕화면\통합패치\시험_R43.28_설치기`(설치기 파일만)에서 설치.cmd 한 번 켜 보기 전에는 배포 공개하지 않음(L144). 정식 폴더는 아직 그대로.
- 11-11 21:20 PC 에서 시험 폴더 설치.cmd 컴파일·창 뜸 확인(사용자) → updates/index.json·README.md 올림(업데이트 켜짐). 관문 --넘김 R43.28.
- 11-11 21:25 정식 폴더(통합패치 11월 10일 새버전)에 R43.28 36개 덮어씀, 다시 읽어 해시 대조 0 틀림.
- 11-11 21:30 저장소에 처음_설치.cmd·updates/first_setup.ps1·README 정리(처음 받는 사람은 Download ZIP → 처음_설치.cmd 한 번). PowerShell 이 여기 없어 실제 실행은 못 해 봄(실패 시 자세한 창).
- 11-11 21:5x R43.29(진단 판 표시·R43.28 기록 읽기·설치기 오른쪽 칸) 저장소에 올림. 첫 실제 [업데이트] 시험 대상. 제보 P243(2D DINPUT8 남의 모드)·P244(옛 차수 추정)·P245.
- 11-11 22:05 R43.30(업데이트 진행 창·끝나면 바로 새 설치기) PC 컴파일 확인 → 판 목록 올림.
