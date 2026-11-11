MeritTraits RC4-R5-SafeExit, continuation of our latest delivered R4 source.
Read ../CONTINUE_PROMPT_KO.txt first. Do not restart/rewrite the existing system.

Build: Linux x86_64, Zig 0.16.0 or clang + lld-link. Tests: Python3, gcc, binutils.
  export MERIT_ZIG=/absolute/path/to/zig
  bash source/build.sh
  python tools/package_r5.py --stage
  python tests/run_r5.py --with-package
  python tests/simulate_rc4.py
  python tools/package_r5.py --zip /absolute/new-output-dir --baseline /absolute/R4-source
Run from package root. Export MERIT_ZIG for the package checker's rebuild too.
No toolchain/network download is hidden in these commands.

run_r5 --with-package is the current gate; audit/r5_* contains current counts.
Older run_r2/run_r3/run_r4 package gates use historical manifests, not current ones.
test_r3.py, test_r2.py and test_r1_skip.py DO remain current behavior regressions.
Tests execute real production Win64 machine code through a Microsoft ABI bridge
with substituted OS/Steam/native UI endpoints. NOT Windows/live game/GPU/FPS proof.
The simulation compiles the same C engine for Linux, 1000 official targets x3 draws.
Builds omit debug paths; Zig links with SOURCE_DATE_EPOCH=0. Never reuse stale maps.

Final ability rule: official TARGET and TEACHERS use original DB ability vectors.
Never write/revert native ability edits/growth. Only custom targets with no DB row
use native base abilities. Similarity uses four proposal axes; display shows five.
Current owned traits, tactics and parents retain their live behavior.

Flow: tile -> pending -> optional reroll -> bottom decision OR Back/ESC -> original
decision chain -> native award/trait+X+passion metadata -> native gratitude/return.
No upper accept/exit objects exist. Back opens native Yes/No at the safe tick;
only explicit Yes queues acceptance. No preserves result and spent tickets.
Opening held keys cannot consent; Escape inside the warning means No after release.
Each warning has a callback generation. Native queued Escape/decision/select input
is consumed/discarded. Partial allocation failure cannot imply consent. Native
font metrics and skins are reused; the panel retains its native contrast palette.
only a no-draw inheritance choice uses native cancellation. A successful reroll
reserves that officer's coupon immediately, reconciled exactly once at commit.
Other officer pending results remain independent. MISS notice OK only dismisses.
Forced teardown never commits inside destruction; spent coupons remain reserved.
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
R5 preserves R4 engine, slot codec, resolver, display adapters and original DB bytes.
Latest user instruction intentionally changes Back/ESC from discard to accept.
R5 tests model distinct outer layout/inner list ownership, callbacks during
unregister, delayed destruction, per-officer coupon reservations and failed commits.
Legacy accept_preview helpers test internal engine transactions, not a live button.
See ../analysis/r5_exit_and_row_lifetime.md and ../analysis/user_video_log_review.md.
The supplied video/logs are from ANOTHER CHAT'S build, not this one. They define
reference failure scenarios only, not evidence this DLL reproduced that live crash.
All delivery ZIPs contain source/tests/handoff but no user video, save or game EXE.
