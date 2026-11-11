#!/bin/bash
# Linux host test. Optional: unpacked game images (made from your own exe, never shipped).
#   ./test/run_tests.sh [image.bin ...]
set -e
B=$(cd "$(dirname "$0")/.." && pwd); cd $B; mkdir -p out
clang -O1 -g -Wall -Wextra -Wno-unused-parameter -DDL_IMPORT= -DDL_WINAPI= -Isrc \
  test/host_test.c src/drag_learn.c -lpthread -o out/host_test
if [ $# -eq 0 ]; then ./out/host_test; exit; fi
for img in "$@"; do
  echo "######## $img"
  ./out/host_test "$img" $(python3 test/find_expected.py "$img")
done
