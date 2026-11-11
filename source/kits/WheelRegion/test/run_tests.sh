#!/bin/bash
# Linux host test. Optional args: path to an unpacked game image + its RVAs (see host_test.c).
set -e
B=$(cd "$(dirname "$0")/.." && pwd); cd $B
clang -O1 -g -Wall -Wextra -Wno-unused-parameter -DWR_IMPORT= -DWR_WINAPI= -Isrc \
  test/host_test.c src/wheel_region.c -lpthread -o out/host_test
./out/host_test "$@"
