#!/bin/bash
# 오프라인 전체 시험 한 번에(리눅스 clang + python). 실행: bash test/run_offline_all.sh   (R43 11-08)
cd "$(dirname "$0")/.." || exit 1
T=$(mktemp -d);fail=0
run(){ local name=$1;shift; if "$@" >"$T/$name.log" 2>&1;then echo "  ok   $name";else echo "  FAIL $name (기록 $T/$name.log)";tail -5 "$T/$name.log";fail=$((fail+1));fi; }
nat(){ local name=$1 out=$T/$1;shift; run "$name" bash -c "clang++ -std=c++17 -O1 -DS14_DIAG_RECORDS=1 $* -o $out && $out"; }   # 시험은 확인용 기록을 켠 채(본판 빌드는 꺼짐, 관문 c12)
nat auto_merit      -w -I test/shim -I src test/test_auto_merit_native.cpp
nat diag            -w -I test/shim -I src test/test_diag_native.cpp
nat domestic_ai     -I test/shim -I src test/test_domestic_ai_native.cpp src/domestic_ai.cpp src/domestic_ai_bridge.S
nat scene_summary   -w test/test_scene_summary_native.cpp
run scene_wide bash -c 'clang++ -std=c++17 -O1 -w -DS14_DIAG_RECORDS=1 -I test/shim -I src test/test_scene_wide_native.cpp src/scene_skip.cpp -o "$0" && "$0" && "$0" site && "$0" autosite && "$0" autoshape && "$0" off' "$T/scene_wide"
nat ic_mapsites     -w test/test_ic_mapsites_native.cpp
nat domestic_player -w -I test/shim -I src test/test_domestic_player_native.cpp
nat ic_after_restore -DS14_TEST_SHIM -Wno-everything -I test/shim27 -I src test/test_ic_after_restore_native.cpp
nat fps120_late     -DS14_TEST_SHIM -Wno-everything -I test/shim27 -I src test/test_fps120_late_native.cpp
nat patch27_icwait  -DS14_TEST_SHIM -I test/shim27 -I src test/test_patch27_icwait_native.cpp src/patch27.cpp
nat r43_conflict    -DS14_TEST_SHIM -Wno-everything -I test/shim -I src test/test_r43_conflict_native.cpp
nat region_officer  -w -I test/shim -I src test/test_region_officer_native.cpp
nat stats200_grow   -w -DS14_TEST_SHIM -I test/shim -I src test/test_stats200_grow_native.cpp src/stats200_new_bridge.S
nat traits9_all     -w -I test/shim -I src test/test_traits9_all_native.cpp
nat traits9_independent -w -I test/shim -I src test/test_traits9_independent_native.cpp
run scene_wide_sites python3 test/test_scene_wide_sites.py
run modsettings_vtable_owner python3 test/test_modsettings_vtable_owner.py
run icguard_cave    python3 test/native/test_icguard_h_cave.py
run ic_restore_overlap python3 test/test_ic_restore_overlap.py
run settings_window python3 test/test_settings_window_r43.py
run ic_self_refs python3 test/test_ic_self_refs.py
run installer_leftover python3 test/test_installer_leftover.py
run installer_syntax python3 test/test_installer_syntax.py
run installer_update python3 test/test_installer_update.py
run traithover_unified python3 test/test_traithover_unified.py
run merit_guard bash test/merit_guard/run.sh
run ic_wait python3 test/test_ic_wait_r43.py
run settings_tabs python3 test/test_settings_tabs.py
run ic_invest_skip python3 test/test_ic_invest_skip.py
run settings_counts bash -c 'clang -O1 -w test/test_settings_counts_native.c src/mod_settings_bridge.S -o "$0" && "$0"' "$T/settings_counts"
run wheel_ic_wait bash -c 'clang -O1 -w -I kits/WheelRegion/dll test/test_wheel_ic_wait_native.c -o "$0" && "$0"' "$T/wheel_ic_wait"
run ai1514 bash test/ai_defer/run1514.sh
echo; [ $fail = 0 ] && echo "오프라인 전체 시험 통과" || echo "실패 $fail"
exit $fail
