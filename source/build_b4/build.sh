#!/bin/bash
# B4/22번 포크 전용 빌드 (공용 build_out 안 씀)
set -e
B=/home/claude/r41/build_tc; S=/home/claude/r41/src; O=/home/claude/r41/build_b4
CXXF="--target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -I$B/inc -I$S -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -fno-threadsafe-statics -fno-stack-protector -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-function"
LDF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va -L$B/lib"
[ -f $O/crt_min.o ] || clang --target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -c $B/src/crt_min.c -o $O/crt_min.o
[ -f $O/chkstk.o ] || clang --target=x86_64-w64-windows-gnu -c $B/src/chkstk.S -o $O/chkstk.o
build() {
  local name=$1; shift; local objs=()
  for f in "$@"; do local o=$O/${name}_$(basename $f).o
    case $f in *.S) clang --target=x86_64-w64-windows-gnu -I$S -c $S/$f -o $o ;; *) clang++ $CXXF -c $S/$f -o $o ;; esac; objs+=($o); done
  clang $LDF "${objs[@]}" $O/crt_min.o $O/chkstk.o -lkernel32 -lbcrypt -luser32 -o $O/$name; sha256sum $O/$name
}
build SAN14_ICGuard.dll ic_guard.cpp
[ -f $S/execute_lock.cpp ] && build SAN14_ExecuteLock.dll execute_lock.cpp $( [ -f $S/execute_lock_bridge.S ] && echo execute_lock_bridge.S ) || true
