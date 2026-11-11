#!/bin/bash
# CRT-free clang build of the Patch27-family DLLs (no MinGW runtime needed).
set -e
B=/home/claude/p27/build
S=${SRC:-$B/src}
O=${OUT:-$B/out}
mkdir -p $O
CXXF="--target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -I$B/inc -I$S -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -fno-threadsafe-statics -fno-stack-protector -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-function"
LDF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va -L$B/lib"
clang --target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -c $B/src/crt_min.c -o $O/crt_min.o
clang --target=x86_64-w64-windows-gnu -c $B/src/chkstk.S -o $O/chkstk.o
build() {
  local name=$1; shift
  local objs=()
  for f in "$@"; do
    local o=$O/${name}_$(basename $f).o
    case $f in
      *.S) clang --target=x86_64-w64-windows-gnu -I$S -c $S/$f -o $o ;;
      *) clang++ $CXXF -c $S/$f -o $o ;;
    esac
    objs+=($o)
  done
  clang $LDF "${objs[@]}" $O/crt_min.o $O/chkstk.o -lkernel32 -lbcrypt -luser32 -o $O/$name
  sha256sum $O/$name
}
build SAN14_Patch27.dll patch27.cpp patch27_bridge.S patch27_ui.cpp officer_names.cpp death_log.cpp
build SAN14_DomesticTalent.dll domestic_talent.cpp
build SAN14_AutoPersonnelRepair.dll auto_personnel_repair.cpp
build SAN14_Stats200_CEKO.dll stats200_direct.cpp
build SAN14_PatchTester.dll patch_tester.cpp
build SAN14_PrisonerReturn.dll prisoner_return.cpp prisoner_bridge.S
