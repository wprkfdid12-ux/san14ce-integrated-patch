#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
python3 generate_collab_db.py
python3 generate_trait_rules.py
mkdir -p build
# R17 changes the main module only. Keep the exact distributed private resolver
# when --main-only is requested (its source, ABI and hook table are unchanged).
main_only=0
if [[ "${1:-}" == "--main-only" || "${MERIT_MAIN_ONLY:-0}" == 1 ]]; then
  main_only=1
  test -f build/SAN14_MeritResolver_CEKO.dll
elif [[ $# != 0 ]]; then
  echo 'Usage: build.sh [--main-only]' >&2; exit 2
fi
# Retains the original Zig path; permits an offline LLVM build without downloads.
if [[ -n "${MERIT_ZIG:-}" ]]; then
  cc=("$MERIT_ZIG" cc); target=(-target x86_64-windows-msvc)
  for dll in kernel32 user32 bcrypt; do
    "$MERIT_ZIG" lib /def:"$dll.def" /machine:x64 /out:"build/$dll.lib"
  done
else
  cc=("${MERIT_CLANG:-clang}"); target=(--target=x86_64-pc-windows-msvc)
  : "${MERIT_LINK:=lld-link}"
  for dll in kernel32 user32 bcrypt; do
    "$MERIT_LINK" /lib /def:"$dll.def" /machine:x64 /out:"build/$dll.lib"
  done
fi
flags=(-O2 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-incompatible-pointer-types-discards-qualifiers -ffreestanding -funwind-tables -fasynchronous-unwind-tables -fno-builtin -fno-stack-protector)
units=(merit_core gacha merit_runtime)
if [[ $main_only == 0 ]]; then units+=(merit_resolver); fi
for unit in "${units[@]}"; do
  "${cc[@]}" "${target[@]}" "${flags[@]}" -c "$unit.c" -o "build/$unit.obj"
done
for unit in bridges stack_probe; do
  "${cc[@]}" "${target[@]}" -g0 -c "$unit.s" -o "build/$unit.obj"
done
objects=(build/merit_core.obj build/gacha.obj build/merit_runtime.obj build/bridges.obj build/stack_probe.obj build/kernel32.lib build/user32.lib build/bcrypt.lib)
if [[ -n "${MERIT_ZIG:-}" ]]; then
  # Zig's COFF frontend does not emit the MSVC /map format. Do not leave maps
  # from a different compiler/build where a verifier could mistake their RVAs.
  rm -f build/merit.map build/resolver.map build/SAN14_MeritTraits.pdb build/SAN14_MeritResolver_CEKO.pdb
  SOURCE_DATE_EPOCH=0 "${cc[@]}" "${target[@]}" -g0 -s -shared -nostdlib -Wl,-e,DllMain -o build/SAN14_MeritTraits.dll "${objects[@]}"
  if [[ $main_only == 0 ]]; then
    SOURCE_DATE_EPOCH=0 "${cc[@]}" "${target[@]}" -g0 -s -shared -nostdlib -Wl,-e,DllMain -o build/SAN14_MeritResolver_CEKO.dll build/merit_resolver.obj
  fi
else
  "$MERIT_LINK" /dll /nodefaultlib /entry:DllMain /machine:x64 /dynamicbase /nxcompat /timestamp:0 /map:build/merit.map /out:build/SAN14_MeritTraits.dll "${objects[@]}"
  if [[ $main_only == 0 ]]; then
    "$MERIT_LINK" /dll /nodefaultlib /entry:DllMain /machine:x64 /dynamicbase /nxcompat /timestamp:0 /map:build/resolver.map /out:build/SAN14_MeritResolver_CEKO.dll build/merit_resolver.obj
  fi
fi
