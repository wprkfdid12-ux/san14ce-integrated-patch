# R3→R4 기존 Hover / X 표시 연결 계약

이 문서는 실제 게임 화면 인증이 아니라, 제공된 바이너리와 역어셈블 근거에 대한 구현 계약입니다.
`UI_Traits9_Hover.dll` 파일은 수정하지 않습니다. 실행 시 정확한 기존 엔진에만 표시용 연결을 추가합니다.

## 정확한 버전 제한

- 게임: CEKO timestamp `0x6a6d9f72`, image size `0x29ff000`, 기존 전용 Resolver 검사 유지.
- Hover 파일 SHA256: `2926c3ef05abd004bf0e5d9f7113321568800a6cf4d60b7e7e0bf25367b0f164`.
- Hover `.text`: RVA `0x1000`, 길이 `0x5af8`, SHA256 `2b68ba30397e1e03aa35e31735ffde860221273f365bc01aa217a7e9a0821d94`.
- 원래 Hover의 game pointer `+0x32d68`도 현재 게임 주소여야 합니다.
- 기존 정보창 game RVA `0x4915a1`은 `FF25 [Hover+0x16a0]` 점프여야 합니다.
- 해시/주소 불일치는 가챠 활성화 실패입니다. 알 수 없는 다른 패치 코드를 덮어쓰지 않습니다.

## 별도 7개 표시 연결

| 소유 모듈 / RVA | 기존 대상 | R3 연결 | 범위 |
|---|---|---|---|
| Hover `0x2740` | `0x2080` | `hover_x_collect` | 공통 개성 collector |
| Hover `0x29a3` | `0x2080` | `hover_x_collect` | 공통 개성 collector |
| Hover `0x2e79` | `0x2080` | `hover_x_collect` | 부대 표시용 인덱스 생성 |
| Hover `0x4355` | `0x2080` | `hover_x_collect` | 공통 개성 collector |
| Hover `0x398b` | `0x3610` | `hover_x_columns` | 목록 descriptor 구성 |
| Hover `0x53ec` | `0x3610` | `hover_x_columns` | 목록 descriptor 구성 |
| Game `0x4915a1` | Hover `0x16a0` | `hover_x_info` | 기존 무장 정보창 |

6개 call의 opcode와 기존 상대 대상 주소를 각각 확인합니다. 원래 gameplay34개 hook과
native27개 adapter 명세는 유지합니다. R4에서는 DLL 내부6개 call을 검증된 모듈 전용 transaction으로,
게임 안의 정보창1개만 원래 Core로 등록합니다. Core 파일과 game-image 범위 제한은 그대로 둡니다.
R3는7개를 모두 Core로 넘겼지만 실제 Core는 DLL 주소를 거부하므로 활성화가 실패할 수 있었습니다.
이전 모의 Patch 접점은 그 범위 제한을 누락했습니다. R4는 모의 접점도 고치고 제공된 Core의
실제 등록 함수 기계코드로 이 거절과 수정 후 초기화 성공을 재현했습니다.
부분 실패 시 자기 변경 bytes만 되돌리고 새 gameplay hooks도 되돌립니다.
page 보호 또는 instruction cache 복구가 불확실하면 relay를 프로세스 종료까지 유지합니다.
근처 relay page는 1개, 최대192개 VirtualQuery /150ms 제한입니다. 전역 프레임 할당이 아닙니다.

## 표시/효과 분리

collector는 먼저 원래 함수를 호출합니다. X가 없으면 결과를 그대로 반환합니다.
X가 있는 무장만 물리9칸 인덱스로 표시를 펴며, 기존 실제 개성 object를 그대로 사용합니다.
X/빈칸 임시 descriptor는 UI 이름 필드만 있는 표시 객체입니다. DB에 등록하지 않습니다.
실제 개성 getter, 발동, 효과, 후보 가중치에는 연결하지 않습니다.

정보창은 원래 Hover 정보창 업데이트를 먼저 실행하고 기존9칸 label/cache를 재사용합니다.
각 label의 인덱스를 실제 물리칸으로 맞춰 실제 개성 설명과 어긋나지 않게 합니다.
원작5칸 label은 기존9칸 방식대로 숨깁니다. 별도 X 위젯을 해제할 소유권은 만들지 않습니다.
R4는 표시용 조회를 가챠 후보 조회와 분리했습니다. 붉은 실제 개성도 native 이름/색으로 유지합니다.

목록은 descriptor의 이름 callback만 연결합니다. 실제 개성은 기존 callback을 호출합니다.
무장 `0x3d..0x41`, 기본무장 `0x23..0x27`, 추가칸 `0x100..0x103`,
부대 `0xaa..0xae`, 부대 추가칸 `0x104..0x107`을 사용합니다.
부대는 기존 Hover의 `0x1fd0`으로 주장 ID를 해석합니다.
이미 관찰한 callback과 다른 다른 패치 callback이 있으면 덮어쓰지 않습니다.

## 가챠 결과 Hover

`native_ui.inc:hover_merit_row`가 기존 Hover `+0x59c0`에 실제 결과ID를 전달합니다.
원작 개성 설명·수치 출력은 기존 renderer가 담당합니다. 복제된 별도 설명표는 없습니다.
완료된 비활성 버튼의 hit target이 부모인 경우를 허용하되 별도 overlay를 통과하지 않습니다.
원래 row tick이 생략되는 held-result 상태에서도 layout tick에서 Hover를 갱신합니다.
연출/꽝/모달/확정 처리 중에는 숨깁니다. row 파괴는 기존 `+0x5ce0` release를 호출합니다.

## 화면 종료

게임 `0x732f50+0x3a`의 원래 ESC 코드15바이트:
`e8 a1 38 12 00 48 83 c4 20 5b e9 b7 f9 cb ff`.
같은 원작 취소 버튼 `layout+0x1c8`에 `0x856830(button,4)`를 호출하고 `0x3f2950()` 입력을 소비합니다.
`0x6be850`은 닫기/fade이지 상태 pop 전체가 아닙니다. 이것만 호출하면 화면 상태가 남을 수 있습니다.
새 취소 요청은 위젯 release callback에서 예약하고 안전tick에서 실행합니다.
연결을 확인하지 못하면 preview와 요청을 해제해 다음 원작 idle tick을 막지 않습니다.
모달/결과 뒤에서 받은 원작 하단결정 상태1은 원작 idle0으로 돌려 뒤늦은 자동commit을 막습니다.

## 다른 패치용 읽기 API

`int SAN14_MeritSlotDisplayV1(u32 officer_id, u32 physical_slot, u16* trait, u32* is_x)`.
Win64 ABI. 슬롯0..8, 성공1/실패0. 실제 개성ID와 별도의 X boolean을 돌려줍니다.
실제 개성이 해당칸에 있으면 항상 우선하며 겹치는 X는 영구 정리합니다.
게임/UI 스레드 전용입니다. R4는 초기화 전/미로딩/잘못된 메타데이터에서도 출력0과 실패0을 반환하며
호출자는 기존 native 표시로 fallback합니다. 계약은 source/merit_display_api.h에 있습니다.
제공된 정확한 RC25 전용창은 별도 adapter로 연결했고, 그 외 전용 UI의 호출 연결은 자동이 아닙니다.

## 남은 검증과 범위 제한

원작 X 아이콘의 정확한 리소스와 소유권을 검증하지 못했습니다. 현재 X는 원래 글자 렌더러로 출력합니다.
이는 원작 아이콘 재사용 완료가 아니며 실게임 폰트 렌더 확인도 남았습니다. 원작 아이콘이 없다는 뜻이 아닙니다.
X Hover는 지시서가 허용한 생략 분기입니다. 가짜 개성 설명은 띄우지 않습니다.
위 경로를 거치지 않는 다른 패치의 독자적인 상세 패널까지 자동 지원한다고 주장하면 안 됩니다.
실제 모든 화면의 위치/스크롤/무장전환, 모달 입력 차단, GPU 렌더·자원 수명·기존 패치 동시 사용을 확인해야 합니다.
실제 Hover collector/column builder 기계코드 실행 및 모의 native label/입력 검사 결과는 `tests/test_r3.py`에 있습니다.
