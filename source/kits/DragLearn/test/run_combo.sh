#!/bin/bash
# WheelRegion + DragLearn together.  ./test/run_combo.sh <WheelRegion_Kit dir> image.bin [...]
set -e
B=$(cd "$(dirname "$0")/.." && pwd); W=$(cd "$1" && pwd); shift; mkdir -p $B/out
clang -O1 -g -Wall -Wno-unused-parameter -DDL_IMPORT= -DDL_WINAPI= -DWR_IMPORT= -DWR_WINAPI= -I$B/src -I$W/src \
  $B/test/combo_test.c $B/src/drag_learn.c $W/src/wheel_region.c -lpthread -o $B/out/combo_test
for img in "$@"; do $B/out/combo_test "$img"; done
