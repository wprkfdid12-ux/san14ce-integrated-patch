> R9 correction: the R5 inner-list ownership claim below was WRONG. Main+0x1d8 is a transition controller. Read r9_user_evidence_review.md; this file is historical evidence of the faulty assumption.

# R4 기반 현재 수정본의 종료/행 상태 점검

이 문서는 우리 수정본의 코드 검사 근거다. 다른 채팅방의 영상 충돌을
우리 버전의 실게임 충돌로 취급하지 않는다. Windows에서의 재현/해소 확인은 미실시.

## 독립적으로 확인한 네이티브 구조

- range_8a2c50의 0x8a2f5b는 row+8 부모를 가져오고,
  0x8a2f6e에서 그 객체를 F_STATUS(0x6d90f0)에 전달한다.
- fn_6cb690의 0x6cbac7에서는 바깥 state layout+0x1d8의 안쪽 목록을 이용한다.
- 우리 R4 hook_confirm은 F_STATUS에 바깥 layout을 넘겼다.
  R5는 Preview.native_owner에 저장한 원래 row 부모로 조회한다.
  R5 fixture는 두 객체를 분리하고 잘못된 인수를 거부한다.
- analysis/r3_native_state_tick.asm 및 native_fragments_rc2.json:
  layout+0x180의 0은 idle, 1은 원작 결정, 2는 원작 다른 작업.
  결정 성공 후 원작 tick이 상태 반환을 한 번 수행한다.
- fn_6c4980의 vslot0x40은 원작 위젯 unregister다. 메모리 소멸과 같지 않다.
- analysis/r2_row_cleanup.asm의 row vslot0x28은 RDX resource_context를
  원본 0x877bc0에 전달한다. 인수를 빠뜨리면 안 된다.
- 제공 RC25 source/ui.h destroy_ui도 소유 위젯을 unregister,
  vslot0x28(resources), scalar destructor 순으로 정리한다.
  단순히 parent가 포인터를 가진다는 이유로 자동 메모리 소유를 가정하지 않는다.

## 현재 처리

뒤로 callback에서는 요청만 기록한다. 다음 안전 tick에서 네이티브 예/아니오 경고를 띄운다.
예를 명시적으로 선택해야 마지막 결과 복사본들을 검증하고 원작 결정 경로를 한 번 탄다.
아니오/경고 안의 새 ESC는 결과/개인권/멈춘 연출 시간을 보존한다. Enter만으로 동의하지 않는다.
이후 별도 native cancel/pop을 더하지 않는다.
추첨 전 승계 선택만 남으면 해당 미추첨 preview를 버리고 원작 취소를 사용한다.
비용/선택 검증 실패 시 결과와 개인권을 보존하고 요청1을 지워 무한 재시도를 막는다.

각 ID의 pending 결과와 stage별 교체 대상을 독립 보관한다.
다른 무장으로 옮겨도 결과는 유지하며 그 무장의 타일을 다시 눌러 재추첨을 이어간다.
성공한 재추첨권은 즉시 개인 상태에 예약한다. 원작 결정 성공 시 최종 slot 권수로
대체하므로 두 번 차감되지 않는다. 강제 해제에서도 사용한 권수는 복구하지 않는다.

callback 세대는 네이티브 unregister 호출 전에 무효화한다.
해제 동안 row refresh/재등록/새 입력을 막고, FX 포인터는 원본 row 자원 변경 전에 복원한다.
row 포인터는 원본 소멸 전에 visible 배열에서 제거한다. toolbar 삭제는 소유 포인터를
먼저 비워 정확히 한 번 수행하며 원작 resource_context를 넘긴다.

## 실제 검증과 한계

tests/test_r5.py는 production Win64 DLL과 캡처 원작 state tick을 실행한다.
새 검사에는 중첩 부모, 두 장수, 양쪽 종료 입력, 연출/재추첨/꽝 안내, 저장로드,
같은 장수의 역순 기회, 행 재활용, unregister 재진입, 지연 소멸 등이 포함된다.
test_r5_warning.py는 경고의 예/아니오, native 글꼴·스킨·팔레트, held 입력,
버퍼에 남은 ESC 소비, 경고 뒤의 결정/선택 요청 제거, 세대별 늦은 callback,
첫 응답 우선/중복 종료 방지, 부분 할당 실패와 재시도, 경고 중 강제 해제를 검사한다.
경고 생성 실패는 동의가 아니며 원작 결정을 대신 호출하지 않는다.
위젯 구현/OS/실제 대사·비용 엔드포인트는 대체되어 있다.
실제 Windows의 할당기/DirectX/input/destructor 전체 검증이나 무충돌 증명은 아니다.

예상하지 못한 world/layout 해제에서 소멸 중 강제 commit하지 않는다.
그 예외 경로는 사용한 권수만 남기고 미반영 결과를 버린다.
스타9칸에서 교체 미선택이면 기존 포기 규칙으로 기존 개성을 보존한다.
안전한 원작 실패는 거짓 성공/임의 비용/임의 개성 삭제로 바꾸지 않는다.
