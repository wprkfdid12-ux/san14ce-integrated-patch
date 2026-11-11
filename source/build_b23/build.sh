#!/bin/bash
# B2·B3 단독 빌드(시험용): 공용 build_out 안 씀
set -e
B=/home/claude/r41/build_tc; S=/home/claude/r41/src; O=/home/claude/r41/build_b23; T=/home/claude/r41/test
CXXF="--target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -I$B/inc -I$S -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -fno-threadsafe-statics -fno-stack-protector -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-function"
LDF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va -L$B/lib"
clang --target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -c $B/src/crt_min.c -o $O/crt_min.o
clang --target=x86_64-w64-windows-gnu -c $B/src/chkstk.S -o $O/chkstk.o
clang++ $CXXF -c $S/scene_skip_b23.cpp -o $O/scene_skip_b23.o
clang --target=x86_64-w64-windows-gnu -I$S -c $S/scene_skip_b23_bridge.S -o $O/scene_skip_b23_bridge.o
clang++ $CXXF -c $T/b23_testmain.cpp -o $O/b23_testmain.o
clang $LDF $O/scene_skip_b23.o $O/scene_skip_b23_bridge.o $O/b23_testmain.o $O/crt_min.o $O/chkstk.o -lkernel32 -lbcrypt -luser32 -o $O/SAN14_SceneSkipB23Test.dll
sha256sum $O/SAN14_SceneSkipB23Test.dll
