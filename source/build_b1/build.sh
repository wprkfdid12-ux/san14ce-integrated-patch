#!/bin/bash
set -e
B=/home/claude/r41/build_tc; S=/home/claude/r41/src; O=/home/claude/r41/build_b1
CXXF="--target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -I$B/inc -I$S -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -fno-threadsafe-statics -fno-stack-protector -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-function"
LDF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va -L$B/lib"
clang --target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -c $B/src/crt_min.c -o $O/crt_min.o
clang --target=x86_64-w64-windows-gnu -c $B/src/chkstk.S -o $O/chkstk.o
clang++ $CXXF -c $S/scene_skip.cpp -o $O/ss.o
clang --target=x86_64-w64-windows-gnu -I$S -c $S/scene_skip_bridge.S -o $O/ssb.o
B23=${B23:-$O/b23_stub.cpp}
clang++ $CXXF -c $B23 -o $O/b23.o
EX=""; [ -f "$B23BR" ] && { clang --target=x86_64-w64-windows-gnu -I$S -c $B23BR -o $O/b23b.o; EX=$O/b23b.o; }
clang $LDF $O/ss.o $O/ssb.o $O/b23.o $EX $O/crt_min.o $O/chkstk.o -lkernel32 -lbcrypt -luser32 -o $O/SAN14_SceneSkip.dll
sha256sum $O/SAN14_SceneSkip.dll
