MeritTraits RC4-R4, incremental continuation of the latest delivered RC4-R3 source.
Read ../CONTINUE_PROMPT_KO.txt first. Do not restart/rewrite the existing system.

Build: Linux x86_64, Zig 0.16.0 or clang + lld-link. Tests: Python3, gcc, binutils.
  export MERIT_ZIG=/absolute/path/to/zig
  bash source/build.sh
  python tools/package_r4.py --stage
  python tests/run_r4.py
  python tests/simulate_rc4.py
  python tools/package_r4.py --zip /absolute/new-output-dir --baseline /absolute/R3-source
Run from package root. Export MERIT_ZIG for the package checker's rebuild too.
No toolchain/network download is hidden in these commands.

run_r4 is the current gate (411 executed-DLL +135 static assertions).
run_r2/run_r3 and their package gates are historical, not current R4 manifests.
test_r3.py, test_r2.py and test_r1_skip.py DO remain current behavior regressions.
Tests execute real production Win64 machine code through a Microsoft ABI bridge
with substituted OS/Steam/native UI endpoints. NOT Windows/live game/GPU/FPS proof.
The simulation compiles the same C engine for Linux, 1000 official targets x3 draws.
Builds omit debug paths; Zig links with SOURCE_DATE_EPOCH=0. Never reuse stale maps.

Final ability rule: official TARGET and TEACHERS use original DB ability vectors.
Never write/revert native ability edits/growth. Only custom targets with no DB row
use native base abilities. Similarity uses four proposal axes; display shows five.
Current owned traits, tactics and parents retain their live behavior.

Flow: tile -> preview -> separate accept -> original bottom decision -> atomic
native award/trait+X+passion metadata -> native gratitude/return. Cancellation uses
the exact original cancel-button action, not a fade-only close or invented pop.
Five rerolls are keyed per officer, across their three opportunities; cancel does
not refund spent coupons. MISS preview never creates X or passion by itself.
V3 extends reserved padding while preserving GSlot64/GState232. V2 migration must
not modify pinned source bytes. Keep two-generation journal and ambiguity guards.

hover_x_adapter.inc is a version-pinned DISPLAY-ONLY extension of the bundled
Hover. See ../analysis/hover_x_contract.md for seven sites and limitations.
IMPORTANT R4 fix: the actual supplied Core only accepts GAME-image hooks. R3
passed six DLL-local Hover sites to it, so its real activation could be refused;
the old Patch stub omitted this bound. R4 keeps Core unchanged, registers only
the single game information site there, and uses display_patch.inc for the six
fingerprinted module-local calls. Tests execute the real Core registry to verify
both the old address rejection and complete corrected Merit activation.
DLL-local transactions validate bounds/bytes/overlap, open all pages, recheck,
write, flush, and restore protections in reverse order. Caller-owned rollback
keeps relays alive on unresolved protection or instruction-cache failure.
No fake native trait IDs/effect getter hooks. Real descriptions are authoritative
from the existing Hover renderer. R4 separates all-native-color display lookup
from blue/gold gacha eligibility, preserving real red traits beside an X.
presets_x_adapter.inc adds an OPTIONAL exact supplied RC25 panel bridge without
replacing its DLL or input/tooltip functions. See ../analysis/presets_x_contract.md.
tests/test_r4.py also executes RC25's actual nine-trait refresh loop with external
native endpoints substituted. Original bytes live under references/compatibility,
not payload. Unknown/modified RC25 skips only that optional integration.
Other independent custom panels can use the exported SAN14_MeritSlotDisplayV1;
merit_display_api.h documents lifetime/fallback rules. Universal coverage is not claimed.
Original X icon resource identification and live font/render validation remain open.

Loader/Core/base Resolver/Hover files are preserved byte-for-byte. Other installed
mods, game EXE/data and user saves are not patched by the installer. A runtime
Hover fingerprint mismatch disables activation rather than overwriting unknown code.
No private game executable, complete reconstructed game image or player save ships.
R4 preserves R3 engine/state/UI input source and original official DB hashes.
No probability, personal coupon, native cancel, or v3 storage schema change in R4.
