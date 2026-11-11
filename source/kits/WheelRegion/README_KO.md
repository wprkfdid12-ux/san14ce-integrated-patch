# 삼국지14 CE — 지역 휠클릭 거점 선택 (WheelRegion) 통합용 키트

> **English summary.** Wheel-click (middle button) on an empty hex of a region = left click on that
> region's village / port / city / gate, even when an army stands on it (the vanilla game has no way to
> open that settlement then). Normal clicks are untouched. Drop `src/` into your mod and call
> `WR_Start()`, or load the ready `SAN14_WheelRegion.dll` (also a SAN14 ModCore module). Code sites are
> found by byte patterns (no fixed addresses), verified unique in the 2026-10 Steam build of
> SAN14CE_KO / SAN14CE / SAN14CE_SC / SAN14CE_TC. CRT-free C, kernel32 + user32 only.

만든 사람: ______ (배포 전에 적어 주세요) · 키트 작성일: 2026-10-12

---

## 1. 기능

| 상황 | 동작 |
|---|---|
| 지역 안 **빈 칸**을 휠클릭 | 그 지역 거점(마을·항구, 없으면 도시·관문)을 좌클릭한 것과 같음. 내정 메뉴와 지역 범위 표시가 나옴 |
| 거점 칸 위에 **부대가 서 있을 때** | 부대가 아니라 거점이 선택됨 (원래 게임에는 이때 거점 메뉴를 여는 방법이 없음 — 이 기능을 만든 이유) |
| 평소 좌클릭·우클릭, 부대 선택, 이동 지정 | 원래대로 |
| 부대·도시 칸 위에서 휠클릭 | 아무 일 없음 |
| 목표 선택(이동 범위 표시) 중 휠클릭 | 그 거점이 목적지로 지정됨 (기본값. `normal_mode_only=1`이면 이때는 무시) |
| 모든 세력의 거점 | 적용 |

알려진 부작용(표시만): 거점 위에 부대가 있을 때, 휠클릭 직후 약 0.6초 동안 지도 축소 화면의 군세 표시가 작아졌다 커질 수 있음. 내정 실행에는 영향 없음.

---

## 2. 넣는 방법

### 방법 1 — 소스로 넣기 (권장)

1. `src/` 폴더의 네 파일을 프로젝트에 추가합니다: `wheel_region.h`, `wheel_region.c`, `wr_block.h`, `wr_targets.h`
2. `wheel_region.c` 하나만 컴파일 목록에 넣습니다. C99, 표준 라이브러리 없이 빌드됩니다(컴파일러가 `memcpy`/`memset`을 부를 수 있으니 CRT 없는 빌드라면 그 둘만 있으면 됨). 링크는 `kernel32`, `user32`.
   - `<windows.h>`를 같이 쓰고 싶으면 `-DWR_USE_WINDOWS_H`
3. 아무 스레드에서 한 번 부릅니다. 게임 시작 직후(스팀 DRM이 코드를 풀기 전)에 불러도 됩니다. 스스로 기다립니다.

```c
#include "wheel_region.h"

static void my_log(const char *line) { /* 모드 로그에 한 줄 */ }

void my_mod_start(void)
{
    WR_Host  host = { sizeof host, my_log, NULL };
    WR_Config cfg = { sizeof cfg };     /* 0 = 기본값 */
    cfg.hold_ms = 600;                  /* 설정 메뉴와 연결해도 됨 */
    WR_Start(&host, &cfg);
}
```

**모드에 자체 후킹 장부가 있을 때** — `patch_group`을 넘기면 세 곳 패치를 그 장부로 한 번에(전부 아니면 전무) 설치합니다. `WR_Patch`는 SAN14 ModCore의 `S14_Hook`과 메모리 배치가 같아서 그대로 넘길 수 있습니다.

```c
static int my_patch_group(const char *group, const WR_Patch *p, unsigned n)
{
    return api->InstallHooks(group, (const S14_Hook *)p, n) != 0;   /* ModCore 예 */
}
host.patch_group = my_patch_group;
```

**입력을 직접 처리하고 싶을 때** — `cfg.trigger_vk = -1`로 두면 버튼을 읽지 않습니다. 모드의 입력 처리에서 원하는 순간에 `WR_Trigger()`를 부르면 됩니다(어느 스레드든 가능, 실제 작업은 WheelRegion 스레드가 함).

| 함수 | 설명 |
|---|---|
| `WR_Start(host, cfg)` | 시작. host/cfg는 복사됨, NULL이면 기본값 |
| `WR_Stop()` | 멈추고, 스스로 쓴 패치는 원래 바이트로 되돌림(호스트가 쓴 패치는 호스트 몫) |
| `WR_GetState()` | `WR_STATE_*` 비트 (찾음/후킹됨/실패 이유) |
| `WR_GetCounter(n)` | 진단 카운터 (0 동작 횟수, 1 A, 2 B, 3 K 호출, 4 K 적중) |
| `WR_SetTargets(t, 48400)` | 지도가 바뀐 유저패치용 표 교체 (`WR_Start` 전에) |
| `WR_Trigger()` | `trigger_vk=-1`일 때 한 번 실행 |

| `WR_Config` | 기본 | 설명 |
|---|---|---|
| `trigger_vk` | 4 (휠클릭) | 5·6 = 마우스 옆 버튼. -1 = 직접 `WR_Trigger()` |
| `hold_ms` | 600 | 클릭 뒤 바꿔치기를 유지하는 시간. 30ms에서도 대부분 되지만 600 권장 |
| `normal_mode_only` | 0 | 1 = 아무것도 선택 안 된 상태에서만 (`[맵+2F6A0]`이 `CHexFocusModeHex`) |
| `game_callers_only` | 0 | 1 = 주둔 부대 숨기기를 게임 코드에만 적용(다른 모드가 같은 함수를 불러도 원래 값) |
| `wait_seconds` | 300 | 게임 코드가 풀리기를 기다리는 최대 시간 |
| `poll_ms` | 15 | 버튼 확인 간격 |

### 방법 2 — DLL 그대로, 통합패치(ModCore) 모듈로

`bin/SAN14_WheelRegion.dll` → 게임 폴더, `bin/SAN14Mods/modules/WheelRegion.ini` → 게임 폴더의 `SAN14Mods\modules\`.
ModCore가 `SAN14_ModInitV1`을 부르고, 패치는 ModCore 훅 장부(`SAN14CE.WheelRegion` 묶음)로 들어갑니다.

### 방법 3 — DLL 그대로, 다른 로더로

- `SAN14_WheelRegionStart()`를 부르는 로더, 또는 `LoadLibrary`만 하는 로더(이때는 3초 뒤 스스로 시작).
- 시험할 때는 Cheat Engine의 *메모리 보기 → 도구 → DLL 주입*으로 넣으면 됩니다.
- 이 키트는 일부러 **프록시 DLL을 제공하지 않습니다.** `dinput8`(안개·통합패치), `xinput1_3`(발탁), `dxgi`·`version`(통합패치) 이름은 이미 다른 모드가 쓰고 있습니다.

DLL 옆 파일(모두 선택): `SAN14_WheelRegion.ini`(설정), `SAN14_WheelRegion_targets.bin`(바뀐 지도용 표), `SAN14_WheelRegion.log`(DLL이 씀).

같은 기능이 두 번 들어가도(소스로 넣은 모드 + DLL) 안전합니다. 두 번째는 이미 바뀐 바이트 때문에 패턴을 못 찾고 스스로 꺼집니다.

---

## 3. 동작 원리

### 순서
1. 작업 스레드가 게임 `.text`에서 패턴 A가 보일 때까지(=스팀 DRM이 코드를 푼 뒤) 0.5초마다 확인
2. 패턴 5개로 코드 3곳과 전역 2개를 찾음. 각각 **정확히 1개**여야 하고, K는 두 호출부가 가리키는 함수가 같아야 함. 하나라도 어긋나면 게임을 건드리지 않고 꺼짐
3. 게임 이미지 ±2GB 안에 4KB 블록 하나를 잡아 기계어 512바이트(`wr_block.S`)를 넣고, 세 곳을 그 블록으로 점프하게 바꿈
4. 버튼을 15ms마다 확인. 눌린 순간 아래 조건이면 실행:
   - 게임 창이 앞에 있음
   - 마우스 밑 종류 `[맵+2F480]=0`, `[맵+2F484]=-1` (빈 칸)
   - 정보 칸 `[맵+2F4B0]`의 표 값이 있고, 그 칸 자체가 아님
5. 실행 순서

| 시간 | 동작 |
|---|---|
| 0 | 플래그 켬, 마우스 +1px |
| 30ms | 마우스 -1px (게임이 호버를 다시 계산) |
| 90ms | 왼쪽 버튼 누름 |
| 130ms | 왼쪽 버튼 뗌 |
| +hold_ms | 플래그 끔, 마우스 +1px → 30ms 뒤 -1px |

합성 입력에는 `dwExtraInfo = 0x57524731`("WRG1")이 붙어 있어 다른 모드가 구분할 수 있습니다. 왼손잡이 설정(버튼 바꿈)도 반영합니다.

### 코드 3곳 (플래그가 꺼져 있으면 원래와 똑같이 동작)

| | 찾는 패턴 | 덮는 바이트 | 플래그 켜졌을 때 |
|---|---|---|---|
| **A** 호버 칸 기록 함수 | `44 3B 42 04 75 03 33 C0 C3 44 89 0A B8 01 00 00 00 44 89 42 04 C3` | 시작 6바이트 (`jmp`+`nop`), 함수 전체를 블록이 대신함 | 기록할 칸을 그 지역 거점 칸으로 바꿈 |
| **B** 정보 칸 저장 `mov [rbp+2F4B0],r14d` | `44 8B F0 48 8B CD 44 89 B5 B0 F4 02 00 E8` 의 +6 | 7바이트 (`jmp`+`nop nop`), 끝나면 +7로 복귀 | 저장할 칸을 거점 칸으로 바꿈 |
| **K** 주둔 부대 번호 `movsx eax,word [rcx+1C] / ret` | 호출부 2곳으로 찾음 (아래) | 5바이트 (`jmp`) — 뒤의 int3 11바이트는 건드리지 않음 | 넘어온 건물이 목표 거점 건물이면 0(부대 없음) |

K 호출부 패턴: `0F B6 43 10 83 E8 0F 83 F8 03 76 ?? 48 8B CB E8 ?? ?? ?? ?? FF C8 3D F3 01 00 00` (+15의 call),
`48 8B 8C C8 70 E2 00 00 EB 07 48 8B 88 70 E2 00 00 E8 ?? ?? ?? ?? 85 C0 74 ?? 3D F4 01 00 00` (+17의 call)

전역 2개: `48 8B 05 ?? ?? ?? ?? 48 89 5C 24 20 48 63 88 B0 F4 02 00 81 F9 0F BD 00 00 77 ?? 48 8B 05 ?? ?? ?? ?? 48 8B 9C C8 70 E2 00 00`
→ 첫 `mov rax,[rip+]` = 맵 객체 전역, 두 번째 = 데이터 루트 전역. **언어판마다 주소가 다르므로 고정 주소를 쓰지 마세요.**

| 판 (2026-10 스팀) | A | B | K | 맵 전역 | 데이터 전역 |
|---|---|---|---|---|---|
| SAN14CE_KO.exe | +17F590 | +18C4B4 | +231500 | +1BB3D00 | +277DF18 |
| SAN14CE.exe | +17F040 | +18BF54 | +2314D0 | +1BB7A70 | +2781C68 |
| SAN14CE_SC.exe | +17F2B0 | +18C654 | +2318C0 | +1BB5F20 | +2780118 |
| SAN14CE_TC.exe | +17F620 | +18C674 | +231660 | +1BB4EC0 | +277F0B8 |

### 맵 객체 필드 (구버전 롤백판도 같음)

| 위치 | 뜻 |
|---|---|
| `+2F480` / `+2F484` | 마우스 밑 대상 종류. 빈 칸 0/-1, 마을 0/4, 도시 0/5, 부대 8/10 |
| `+2F4B0` | 정보 대상 칸 (좌측 정보창·클릭 기준) |
| `+2F6A0` | 포커스 모드 객체. `CHexFocusModeHex` = 평소, `CHexFocusModeMoveRange` = 이동 범위 선택 중 |
| 데이터루트 `+E270 + 칸*8` | 칸의 `CHexData`(건물) 객체. 건물 `+10`(byte) 종류: 15 마을, 16 도시, 17 관문, 18 항구. `+1C`(word) 주둔 부대 번호 |

### 지역 표
`wr_targets.h` = 칸(220×220=48400) → 그 지역 거점 칸 (없으면 0xFFFF). 지역 343개 = 마을 242 + 항구 40 + 도시 51(7칸 중 가운데) + 관문 10(가운데 쪽 칸). 원본 지도 파일 `MapExCe.bin`에서 `tools/build_targets.py`로 만듭니다. **지도를 바꾼 유저패치**는 이 도구를 다시 돌리거나, 만든 `.bin`을 DLL 옆에 두거나, `WR_SetTargets()`로 넘기면 됩니다.

---

## 4. 다른 모드와의 호환성 (2026-10-12 분석)

| 모드 | 결과 |
|---|---|
| 전장의 안개 1.6 (`SAN14_FogTrial.dll` + `dinput8.dll`) | 코드 바이트를 고치지 않음(자체 기록 `code_byte_patches: 0`, vtable 교체만) → 바이트 충돌 없음. **단**, 안개 모드는 안개에 가려진 거점이 정보 칸(`+2F4B0`)에 오면 잠시 -1로 막고 그 거점 클릭도 막습니다. 같은 값을 다루므로 **안개 속 적 거점 지역의 휠클릭은 실게임 확인이 필요**합니다(막히는 쪽이 안개 규칙에 맞음). 휠클릭 버튼은 쓰지 않음 |
| 발탁 1.3 (`SAN14_Batteki.dll` + `xinput1_3.dll`) | 충돌 없음. 코드 바이트·맵 값 안 건드림. 메시지 후킹은 자기 타이머만 처리 |
| 롤백 통합패치 Patch27 R18 (ModCore) | 구버전 실행 파일용. 공개된 후킹 397곳 중 겹치는 곳 없음. K 바로 다음 함수(이동 일수, `RemoteRecruitDays`)를 15바이트 고치지만 우리는 그 앞 5바이트만 씀(사이 11바이트). 통합편의 모듈이 K 함수를 직접 부름 → 필요하면 `game_callers_only=1`. **구버전 코드 패턴은 아직 확인 못 함** → `ce/WheelRegion_pattern_check.CT`로 확인 부탁. 안 맞으면 스스로 꺼짐 |

분석한 모드 중 휠클릭(가운데 버튼)을 쓰는 곳은 없었습니다.

---

## 5. 검증

- **실게임**: 같은 동작을 Cheat Engine 테이블(`ce/RTK14_reference_test18.CT`)로 플레이하며 확인 — 마을·항구·도시·관문, 부대가 선 거점, 지역 범위 표시, 평소 클릭 무변경.
- **이 코드 자체**: `test/run_tests.sh` — 리눅스에서 실제 게임 이미지(한국어판·다른 언어판)를 올려 놓고 패턴 찾기 → 블록 설치 → 휠클릭 → 합성 입력 → 원복까지 실제로 실행, 기계어 스텁을 윈도우 호출 규약으로 직접 호출해 확인. 결과: 두 판 모두 64/64 통과 (`test/results/`). 게임 이미지 없이 돌리면 기계어 스텁 검사 21개만 실행됩니다.
  - 실행: `./test/run_tests.sh <게임이미지.bin> <A> <B> <K> <맵전역> <데이터전역> <CHexFocusModeHex vtable>` (RVA, 16진수)
  - 검사에 쓰는 게임 이미지는 각자 자기 게임에서 만든 것이어야 하고, 키트에는 들어 있지 않습니다.
- **완성 DLL 실게임 확인 (2026-10-12)**: 최신 스팀판 SAN14CE_KO.exe에 CE DLL 주입으로 실행 — 로그 패턴 각 1개·`ready`, 인게임 휠클릭 동작 이상 없음.
- **아직 안 한 것**: 롤백판, 안개 모드와 함께 사용.
- 진단: `WR_GetState()` / `WR_GetCounter()`, DLL은 `SAN14_WheelRegion.log`.

---

## 6. 파일

| 경로 | 내용 |
|---|---|
| `src/wheel_region.h` · `.c` | 핵심 소스 (모드에 넣는 부분) |
| `src/wr_block.S` → `src/wr_block.h` | 기계어 블록 원본과 생성된 바이트 (`tools/gen_block.py`) |
| `src/wr_targets.h` | 지역 표 (`tools/build_targets.py`) |
| `dll/` · `build.sh` · `lib/` | 완성 DLL 빌드 (clang + lld, CRT 없음) |
| `bin/` | 완성 DLL, 설정 ini, ModCore 등록 ini |
| `ce/WheelRegion_pattern_check.CT` | 실행 중인 게임에서 패턴 점검 (읽기만) |
| `ce/RTK14_reference_test18.CT` | 실게임으로 확인한 원래 CE 구현 (참고용) |
| `test/` | 리눅스 실행 검사와 결과 |
| `tools/` | 표·블록 생성 도구 |
