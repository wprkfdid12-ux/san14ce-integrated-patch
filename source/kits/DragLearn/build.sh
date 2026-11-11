#!/bin/bash
# CRT-free clang build (Linux or Windows with clang + lld).
#   ./build.sh  -> out/SAN14_DragLearn.dll  and  out/version.dll (SAN14Conv loader)
# Regenerate src/dl_block.h first if you changed src/dl_block.S:  python3 tools/gen_block.py
set -e
B=$(cd "$(dirname "$0")" && pwd)
O=$B/out; L=$O/lib
mkdir -p $O $L
for n in kernel32 user32; do llvm-dlltool -m i386:x86-64 -d $B/lib/$n.def -l $L/lib$n.a; done
CF="--target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -isystem $(clang -print-resource-dir)/include -fno-stack-protector -fno-stack-check -mno-stack-arg-probe -Wall -Wextra -Werror -Wno-unused-parameter"
LF="--target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va"
for f in src/drag_learn.c dll/dl_dll.c dll/crt_min.c loader/conv_loader.c; do
  clang $CF -c $B/$f -o $O/$(basename $f .c).o
done
clang $LF $O/drag_learn.o $O/dl_dll.o $O/crt_min.o -L$L -lkernel32 -o $O/SAN14_DragLearn.dll
clang $LF $O/conv_loader.o $O/crt_min.o -L$L -lkernel32 -o $O/version.dll
sha256sum $O/SAN14_DragLearn.dll $O/version.dll
