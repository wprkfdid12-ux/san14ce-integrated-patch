# 삼국지14 CE — 드래그습득 (DragLearn v2.1) 모더 공유 키트

> **English summary.** In the tactic-learning screen, the tactic-book list and the general list become
> the game's own multi-select lists (checkboxes / drag / select-all). [Decide] → [Yes] makes every
> selected general learn every selected book the game allows for them, at the normal price (1 book +
> 1 command order per learn). Limits follow command orders, free slots and books left; replace mode
> stays single-select.
> Code sites are found by byte patterns, verified unique in the 2026-10 Steam build of
> SAN14CE_KO / SAN14CE / SAN14CE_SC / SAN14CE_TC. CRT-free C, kernel32 only.
> Claude/AI: read `00_CLAUDE_먼저읽기.md` first.

만든 사람: ______ (배포 전에 적어 주세요) · 키트 작성일: 2026-10-12 · 기준 게임 빌드: 2026-10 스팀판(10-15 패치 전)

---

## 1. 기능

**1안 = 무장 1명에게 전법서 여러 권, 2안 = 전법서를 무장 여러 명에게. 둘을 함께 쓰면 고른 무장 모두에게 고른 전법서 모두.**

| 상황 | 동작 |
|---|---|
| 습득 → 습득무장 목록 (2안) | 체크박스로 여러 명 선택 가능. 최대 = **명령서 수**(최대 10명). 창에는 첫 무장이 보임 |
| 습득창 → [전법추가] → 전법서 목록 | 체크박스가 생기고 여러 권을 고를 수 있음(드래그, 일괄선택/일괄해제 포함). 게임 자체 다중선택 화면 |
| 전법서 목록의 회색(선택 불가) | 무장 1명: 원래대로. **여러 명: 고른 무장 중 한 명이라도 배울 수 있으면 선택 가능**, 아무도 못 배우면 회색 |
| 고를 수 있는 최대 권수 | **명령서 수 ÷ 고른 무장 수, 빈 전법 칸 수(여러 명이면 가장 빈 칸 많은 무장), 10** 중 가장 작은 값. 최대에 닿으면 나머지 줄은 게임이 회색으로 막음 |
| [결정] → [예] | 무장 순서대로, 각 무장에게 고른 전법서를 순서대로 습득. 습득 1회당 전법서 1·명령서 1 (원래 비용 그대로) |
| 각 무장 (첫 무장 포함) | 그 무장이 이미 배웠거나 대응 진형이 없는 전법(목록에서 회색이 되는 경우와 같은 규칙)은 건너뜀. 칸이 꽉 찬 무장도 건너뜀 |
| 연속입력 | 고른 무장들이 그대로 유지되어, 다음 전법서도 모두에게 적용 |
| 명령서 1개 이하 / 빈 칸 1칸 이하 | 원래처럼 1권 선택 |
| 10칸이 다 차서 지울 전법을 고른 뒤(교체 습득) | 원래처럼 1권 선택·교체. 다른 무장들은 그 전법서를 평소처럼 배움(빈 칸이 있을 때) |
| 이미 배운 전법, 배울 수 없는 전법 | 원래처럼 회색(선택 불가) |

안전장치: 추가 습득 직전마다 허용 여부·전법서 남은 권수·명령서·빈 칸을 다시 확인하고, 모자라면 건너뛰거나 멈춥니다(쓰지 않은 전법서·명령서는 빠지지 않음). 게임의 습득 함수는 명령서나 전법서가 모자라도 습득시키고, 칸이 넘쳐도 비용을 빼기 때문입니다.

알려진 표시 한계: 습득창의 명령서 미리보기(`11 → 10(-1)`), 확인창 문구, 결과 메시지는 첫 무장·첫 권 기준으로만 나옵니다. 실제 차감은 습득 횟수만큼 정확합니다.

실게임 확인(한국어판, 2026-10-12): v2.1 DLL을 SAN14Conv 로더로 휠클릭과 함께 불러와 1안·2안·회색 보정·교체 습득, 지도 휠클릭, 메뉴 위 휠클릭(반응 없음) 모두 정상(자세히는 00_CLAUDE 10절). CE 테이블 test5~8 단계 확인 내용: 2권·3권 일괄 습득, 빈 칸 3에서 5권 선택 시 3권만 습득, 명령서 2개일 때 2권 제한, 빈 칸 2에서 2권 제한, 10칸 찬 뒤 교체 습득 정상.

---

## 2. 넣는 방법

### 방법 1 — 소스로 넣기
1. `src/drag_learn.h`, `src/drag_learn.c`, `src/dl_block.h` 세 파일을 추가하고 `drag_learn.c`만 컴파일합니다. C99, 표준 라이브러리 없이 빌드됩니다(CRT 없는 빌드라면 `memcpy`/`memset`만 있으면 됨). 링크는 `kernel32`.
   - `<windows.h>`를 같이 쓰려면 `-DDL_USE_WINDOWS_H`
2. 아무 스레드에서 한 번 부릅니다. 게임 시작 직후(스팀 DRM이 코드를 풀기 전)에 불러도 스스로 기다립니다.

```c
#include "drag_learn.h"

static void my_log(const char *line) { /* 모드 로그에 한 줄 */ }

void my_mod_start(void)
{
    DL_Host   host = { sizeof host, my_log, NULL };
    DL_Config cfg  = { sizeof cfg };    /* 0 = 기본값 (최대 10권, 10명) */
    DL_Start(&host, &cfg);
}
```

**후킹 장부가 있는 모드** — `host.patch_group`을 넘기면 6곳을 한 묶음(`SAN14CE.DragLearn`, 전부 아니면 전무)으로 그 장부에 설치합니다. `DL_Patch`는 SAN14 ModCore `S14_Hook`, 휠클릭 키트의 `WR_Patch`와 메모리 배치가 같습니다.

실행 중 끄고 켜기: `DL_SetEnabled(0/1)` (패치는 그대로 두고 동작만 원래대로).

### 방법 2 — 완성 DLL 쓰기 (`bin/`)
| 불러오는 방법 | 놓을 곳 |
|---|---|
| **이 키트의 로더** (`version.dll`) | 게임 폴더에 `version.dll`, `SAN14Conv\SAN14_DragLearn.dll` + `.ini` |
| SAN14 ModCore (롤백 통합패치) | 게임 폴더에 DLL, `SAN14Mods\modules\DragLearn.ini` (롤백판 패턴 일치는 미확인 — 안 맞으면 스스로 꺼짐) |
| export를 부르는 로더 | `SAN14_DragLearnStart()` |
| CE "Inject DLL" / 단순 LoadLibrary | DllMain이 3초 뒤 스스로 시작 |

export: `SAN14_DragLearnStart / Stop / State / Counter(n) / SetEnabled(on)`, `SAN14_ModInitV1`.

### SAN14Conv 로더 (`loader/`)
- 게임이 `VERSION.dll`을 불러오므로 게임 폴더의 `version.dll`이 먼저 로드됩니다. 진짜 `System32\version.dll` 함수 17개를 그대로 넘기고, **게임 exe(이름이 SAN14CE로 시작)일 때만** `<게임>\SAN14Conv\*.dll`을 전부 불러옵니다.
- 로더 자체는 게임 코드를 고치지 않습니다. 로그: `SAN14Conv\SAN14Conv_loader.log`
- 같은 제작자의 휠클릭(`SAN14_WheelRegion.dll` + ini)도 같은 폴더에 넣으면 함께 동작합니다.

---

## 3. 다른 모드와의 호환성

| 모드 | 결과 |
|---|---|
| 휠클릭 거점 선택 (WheelRegion, 같은 제작자) | 패치 위치 겹치지 않음(17F590/18C4B4/231500 vs 7482EC/748359/740125/74A338/74A37A/74816E). 4개 언어판에서 함께 설치·해제 실행 검사 통과(`test/results/combo_*.txt`). 같은 로더 폴더에서 동작 |
| 전장의 안개 1.6 (`dinput8.dll`) | 로더 이름 다름. 안개 모드는 코드 바이트를 고치지 않고 vtable·Present만 바꿈 → 겹치는 곳 없음(분석 기준) |
| 발탁 1.3 (`xinput1_3.dll`) | 로더 이름 다름. 무장 기록 데이터만 다룸 → 겹치는 곳 없음(분석 기준) |
| 롤백 통합패치 Patch27 (구버전) | `version.dll`을 같이 씀 → **로더 대신 ModCore 모듈 ini 사용**. 롤백판 실행 파일에서의 패턴 일치는 미확인 |

자세한 점검 순서는 `00_CLAUDE_먼저읽기.md` 7절.

---

## 4. 빌드와 검사

```
python3 tools/gen_block.py      # src/dl_block.S 를 고쳤을 때만
./build.sh                      # clang + lld (리눅스/윈도우). out/SAN14_DragLearn.dll, out/version.dll
./test/run_tests.sh [image.bin ...]
./test/run_combo.sh <WheelRegion_Kit 폴더> image.bin
```
- `image.bin`은 각자 가진 게임 exe에서 스팀 DRM을 풀어 만든 분석용 이미지입니다(배포 금지). `test/find_expected.py`가 같은 이미지에서 기대 주소를 따로 계산해 DLL 코어의 결과와 비교합니다.
- 결과: `test/results/` — 4개 언어판 각 95항목, 함께 설치 4항목 모두 통과. 가짜 게임 함수가 자기 그림자 영역을 일부러 덮어써서, 레지스터 보존 실수(v1에 있던 버그)를 잡아냅니다.

## 5. CE 테이블
`ce/RTK14_gem_test8.CT` — 1안까지의 동작(AOB 방식, 2안은 DLL에만 있음). 감시값: `rtkLimit`(이번 목록 최대), `rtkOrders`, `rtkFilled`, `rtkLearned` 등. **DLL과 동시에 켜지 마세요**(같은 자리를 고침).

## 6. 게임 패치 뒤
`패치후_점검.md` 참고. 로그의 `pattern matches` 줄만 봐도 바로 판단할 수 있습니다.
