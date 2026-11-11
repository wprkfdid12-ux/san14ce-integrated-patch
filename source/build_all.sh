#!/bin/bash
# R40 전체 재빌드: r40/src → r40/build_out (최종 패키지 전 반드시 실행)
set -e
B=/home/claude/r41/build_tc; S=/home/claude/r41/src; O=/home/claude/r41/build_out; mkdir -p $O
CXXF="--target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -I$B/inc -I$S -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -fno-threadsafe-statics -fno-stack-protector -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-function"
LDF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va -L$B/lib"
clang --target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -c $B/src/crt_min.c -o $O/crt_min.o
clang --target=x86_64-w64-windows-gnu -c $B/src/chkstk.S -o $O/chkstk.o
build() {
  local name=$1; shift; local defs=""; local objs=()
  if [ -n "$ONLY" ] && [ "$ONLY" != "$name" ]; then return 0; fi   # ONLY=이름.dll 이면 그것만
  if [[ "$1" == -D* ]]; then defs=$1; shift; fi
  for f in "$@"; do
    local o=$O/${name}_$(basename $f).o
    case $f in
      *.S) clang --target=x86_64-w64-windows-gnu -I$S -c $S/$f -o $o ;;
      *) clang++ $CXXF $defs -c $S/$f -o $o ;;
    esac
    objs+=($o)
  done
  clang $LDF "${objs[@]}" $O/crt_min.o $O/chkstk.o -lkernel32 -lbcrypt -luser32 -o $O/$name
  sha256sum $O/$name
}
build SAN14_Patch27.dll patch27.cpp patch27_bridge.S patch27_ui.cpp officer_names.cpp death_log.cpp city_event_skip.cpp
build SAN14_DomesticTalent.dll domestic_talent.cpp
build SAN14_AutoPersonnelRepair.dll auto_personnel_repair.cpp
build SAN14_PrisonerReturn.dll prisoner_return.cpp prisoner_bridge.S
build SAN14_SettingsHelpScenic.dll settings_help.cpp
build SAN14_SettingsHelpScenic_domestic.dll -DSAN14_DOMESTIC_UI_BRIDGE settings_help.cpp
build SAN14_PatchTester.dll patch_tester.cpp
# R39 새 모듈
build SAN14_HiredFace.dll hired_face.cpp hired_face_bridge.S
build SAN14_ModSettings.dll mod_settings.cpp mod_settings_bridge.S
[ -z "$SKIP_B" ] && [ -f $S/status_color.cpp ] && build SAN14_StatusColor.dll status_color.cpp status_color_bridge.S || true
[ -z "$SKIP_B" ] && [ -f $S/region_officer.cpp ] && build SAN14_RegionOfficer.dll region_officer.cpp $( [ -f $S/region_officer_bridge.S ] && echo region_officer_bridge.S ) || true
[ -f $S/range.cpp ] && build SAN14_Range.dll range.cpp $( [ -f $S/range_bridge.S ] && echo range_bridge.S ) || true
build SAN14_ICGuard.dll ic_guard.cpp
build SAN14_Notice.dll notice.cpp
build SAN14_ItemPicture.dll item_picture.cpp item_picture_bridge.S   # R41e A8
build SAN14_SceneSkip.dll scene_skip.cpp scene_skip_bridge.S scene_skip_b23.cpp scene_skip_b23_bridge.S   # R42 B1~B3 4번 연출 생략
build SAN14_ExecuteLock.dll execute_lock.cpp execute_lock_bridge.S   # R42 B4 처단불가 시 처단 버튼 회색
build SAN14_Traits9Check.dll traits9_check.cpp   # R42 A10 개성 9칸 확인(읽기만)
build SAN14_DomesticAI.dll domestic_ai.cpp domestic_ai_bridge.S   # R42g 22번 AI 내정 순환(군주 반영)
build SAN14_DomesticPlayer.dll domestic_player.cpp   # R42h 22번 플레이어 내정 자동(DomesticCE 대체)
build SAN14_AutoMerit.dll auto_merit.cpp   # R42i 2번 자동논공 새 엔진(게임 논공 확정 함수로)
build SAN14_Diag.dll diag.cpp   # R43 종합 진단(시작 30·90초, 읽기만)
build SAN14_Stats200New.dll stats200_new.cpp stats200_new_bridge.S   # R43 17번 능력치 200: 새 판 전용 소스 모듈(옛 이식본 대체)
# R43 시험13: 튕김 기록기 CrashLog R3 (C, 따로 만든 kernel32 가져오기 목록)
if [ -z "$ONLY" ] || [ "$ONLY" = "SAN14_CrashLog.dll" ]; then
 llvm-dlltool -m i386:x86-64 -d $S/crash_log_k32.def -l $O/libk32_crash.a
 clang --target=x86_64-w64-windows-gnu -O2 -ffreestanding -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -c $S/crash_log.c -o $O/crash_log.o
 clang --target=x86_64-w64-windows-gnu -fuse-ld=lld -nostdlib -shared -Wl,-e,DllMain -Wl,--no-insert-timestamp -s -o $O/SAN14_CrashLog.dll $O/crash_log.o $O/chkstk.o $O/libk32_crash.a
 sha256sum $O/SAN14_CrashLog.dll
fi
# 2026-11-10 P201: 19번 본체(바이너리)를 통합편의 확인 뒤에 싣는 시작 지연 부품
build SAN14_FPS120Late.dll fps120_late.cpp
