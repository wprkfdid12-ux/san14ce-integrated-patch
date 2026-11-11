#!/bin/bash
# 시험 전용 SAN14_SceneTest.dll (본판 build_all.sh 에 넣지 않음)
set -e
B=/home/claude/r41/build_tc; S=/home/claude/r41/src; O=/home/claude/r41/build_c
CXXF="--target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -I$B/inc -I$S -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -fno-threadsafe-statics -fno-stack-protector -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-function"
LDF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va -L$B/lib"
clang --target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -c $B/src/crt_min.c -o $O/crt_min.o
clang --target=x86_64-w64-windows-gnu -c $B/src/chkstk.S -o $O/chkstk.o
clang++ $CXXF -c $S/scene_test.cpp -o $O/st.o
clang $LDF $O/st.o $O/crt_min.o $O/chkstk.o -lkernel32 -lbcrypt -luser32 -o $O/SAN14_SceneTest.dll
sha256sum $O/SAN14_SceneTest.dll
# 바탕화면 실행기(게임 폴더에 안 넣고 실행 중인 게임에 붙임)
clang++ --target=x86_64-w64-windows-gnu -O2 -std=c++17 -ffreestanding -nostdlibinc -fno-exceptions -fno-rtti -fno-stack-protector -Wall -Wextra -Werror -Wno-missing-field-initializers -c $S/scene_test_launcher.cpp -o $O/stl.o
clang --target=x86_64-w64-windows-gnu -fuse-ld=lld -nostdlib -Wl,--entry,mainCRTStartup -Wl,--subsystem,console -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va $O/stl.o $O/crt_min.o $O/chkstk.o -L$O -lk32x -o "$O/시험도구_실행.exe"
sha256sum "$O/시험도구_실행.exe"
