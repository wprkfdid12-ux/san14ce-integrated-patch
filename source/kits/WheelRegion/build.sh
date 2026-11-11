#!/bin/bash
# CRT-free clang build of SAN14_WheelRegion.dll (Linux or Windows with clang + lld).
#   ./build.sh            -> out/SAN14_WheelRegion.dll
# Regenerate the generated headers first if you changed their sources:
#   python3 tools/gen_block.py                 (src/wr_block.S  -> src/wr_block.h)
#   python3 tools/build_targets.py MapExCe.bin (map             -> src/wr_targets.h)
set -e
B=$(cd "$(dirname "$0")" && pwd)
O=$B/out; L=$O/lib
mkdir -p $O $L
for n in kernel32 user32; do llvm-dlltool -m i386:x86-64 -d $B/lib/$n.def -l $L/lib$n.a; done
CF="--target=x86_64-w64-windows-gnu -O2 -fasynchronous-unwind-tables -ffreestanding -fno-builtin -nostdlibinc -isystem $(clang -print-resource-dir)/include -fno-stack-protector -fno-stack-check -mno-stack-arg-probe -Wall -Wextra -Werror -Wno-unused-parameter"
for f in src/wheel_region.c dll/wr_dll.c dll/crt_min.c; do
  clang $CF -c $B/$f -o $O/$(basename $f .c).o
done
clang --target=x86_64-w64-windows-gnu -shared -fuse-ld=lld -nostdlib \
  -Wl,--entry,DllMain -Wl,--dynamicbase,--nxcompat,--no-insert-timestamp,--high-entropy-va \
  $O/wheel_region.o $O/wr_dll.o $O/crt_min.o -L$L -lkernel32 -luser32 -o $O/SAN14_WheelRegion.dll
sha256sum $O/SAN14_WheelRegion.dll
