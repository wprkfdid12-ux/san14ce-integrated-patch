#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
S=src; O=obj_new; mkdir -p $O
cp merit_resolver.c $S/merit_resolver.c
# generated headers already present in src
T=--target=x86_64-w64-windows-gnu
flags=(-O2 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-incompatible-pointer-types-discards-qualifiers -ffreestanding -funwind-tables -fasynchronous-unwind-tables -fno-builtin -fno-stack-protector)
for u in merit_core gacha merit_runtime merit_resolver; do clang $T "${flags[@]}" -c $S/$u.c -o $O/$u.obj; done
for u in bridges stack_probe; do clang $T -g0 -c $S/$u.s -o $O/$u.obj; done
L="-fuse-ld=lld -nostdlib -shared -Wl,-e,DllMain -Wl,--no-insert-timestamp -s"
clang $T $L -o $O/SAN14_MeritTraits.dll $O/merit_core.obj $O/gacha.obj $O/merit_runtime.obj $O/bridges.obj $O/stack_probe.obj chkstk.obj libkernel32.a libuser32.a libbcrypt.a
clang $T $L -o $O/SAN14_MeritResolver_CEKO.dll $O/merit_resolver.obj
ls -l $O/*.dll
