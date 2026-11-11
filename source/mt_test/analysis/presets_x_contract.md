# R4 optional SortiePresets RC25 X display bridge

This continues the delivered R3 code. It does not rebuild or install a replacement
SortiePresets module. Actual Windows UI/GPU interaction remains unverified.

## Supplied evidence

`references/compatibility/sortie_rc25/` preserves the user's RC25 developer DLL,
`ui.h`, and `preset_core.h` as analysis/test inputs only. They are not payload files.

- DLL SHA256: `617b4eaf0ce7633ab94c1a3fbeb4eff301a748e2ca3395fdbed7055c44fb75a4`
- Image size: `0x9d000`; `.text` RVA `0x1000`, virtual size `0x15168`
- `.text` SHA256: `8aa48b5ad80a49ca598181db90731c73c846c5a4431c2855f2d000eba2ebb556`
- No absolute code relocations occur in this text range. The hash is suitable for
  ASLR-loaded code before our own call-site bridges are installed.

| RC25 item | RVA / layout |
|---|---|
| `refresh_panel(int full)` | `0x10280`, 3147 bytes |
| `hook_tick` | `0x13e70`, 514 bytes |
| `update_tooltip` | `0xd630`, 882 bytes, unchanged |
| `ui` | `0x34040`, size `0xc00` |
| nine physical labels | `ui + 0x58`, pointer stride 8 |
| real trait tooltip IDs | `ui + 0xd0`, integer stride 4 |
| selected preset / shown | `ui + 0xb64` / `ui + 0xb70` |
| `book` | `0x35360`, size `0x584` |
| slot occupied / officer ID | `book + 4` / `book + 8`, 11 slots, stride `0x80` |
| installed / panel_ready / game | `0x1b028` / `0x33d40` / `0x958e8` |

The struct sizes/offsets agree with the supplied C structs and the DLL accesses.
RC25 labels use the existing native `0x85c270` text and `0x856bc0` color functions.
Native visibility is vtable slot `0xc8`; labels are children of `ui.panel`.

## Exact call-site extension

All thirteen original E8 calls target `0x10280`:

`1101b, 11062, 11176, 119a1, 12a13, 131dd, 13615, 1395a, 139dd, 13a1d, 13a53, 13b50, 14045` (hex RVAs).

Only those calls are redirected to a nearby relay. `presets_x_refresh` invokes
the original function first, then paints X over physically empty labels. The
original function body, tooltip function, input state machine and selection book
remain unchanged. On each native tick, refresh at `0x14045` still precedes tooltip
update at `0x1404a`. No new callback or worker thread owns the panel lifecycle.

The bridge checks panel readiness, shown state, selected record, live officer,
committed X mask, all affected label pointers/vtables, and label parent ownership.
Any native nonzero displayed trait ID wins. The sidecar never writes an X into the
native trait array or `ui.trait_ids`. Therefore X has no effect or fake description.
Native refresh clears previous labels before the X pass, including officer changes.
An external real trait edit clears only the overlapping X through R3 reconciliation.

## Registration and conflict handling

Registration probes the already loaded module at Merit initialization and after a
successful native new game/load, outside the state lock. This handles the Core's
unspecified registration order without per-frame searches or timers.

The bridge validates PE type/size, installed flag, game pointer, complete text
hash and every original call target. `display_patch.inc` owns a bounded batch of
13 DLL-local E8 edits. It validates module range/length/non-overlap and all expected
bytes before changing protections, rechecks before writing, and restores protections
in reverse order. It does not send DLL addresses to the supplied Core's game-only
registry or change that registry's range checks. Only game-image hooks use Core.
Unknown/modified modules are logged once and left untouched. Optional failure does
not disable Merit or its required shared Hover integration.

Relay lookup is capped at 192 VirtualQuery calls and 150 ms. On partial write
failure, only still-owned matching bytes are restored. A failed protection/cache change
retains potentially callable relay memory until process exit, never leaving a
freed-code jump. Other modules are assumed to stay loaded for the game process,
as in the supplied Core loader; runtime unloading is not supported.

## R4 regression evidence

`tests/test_r4.py` executes the production Win64 bridge with Microsoft ABI calls.
It first executes the supplied shared Core's real registration function (RVA
`0x1140`, image/size fields at `0x6f38`/`0x6f34`, owned count at `0x5000`). This
reproduces rejection of DLL-local targets and verifies full Merit+Hover+RC25
initialization without relaxing Core. The earlier R3 mock omitted this image-range
rule; that real initialization defect is now explicitly covered and corrected.
It includes early/uninitialized public API guards, red-trait preservation, physical
X cells 2/9, selected-officer isolation, external edits, unknown versions, late
registration, allocation limits, and partial/failed rollback protection.

The `original_rc25_trait_loop` case executes RC25's unchanged `refresh_panel(0)`
machine code, not a Python reimplementation. It substitutes only external native
entity/widget endpoints and peripheral status/portrait helpers. It verifies the
actual nine-slot ID cache/visibility loop before and after X and real-trait edits.
Other call-path tests substitute the original refresh endpoint to inspect forwarding
of its full-refresh argument. Neither kind is a Windows/live-game test.

## Public API and remaining work

`source/merit_display_api.h` documents `SAN14_MeritSlotDisplayV1` for other custom
panels. R4 returns 0 with cleared outputs before initialization, without a world,
on invalid metadata, or for invalid input; consumers retain their native fallback.
The UI-thread-only API may reconcile stale sidecar bits, but never writes native
officer data. It is not a universal hook into unknown future UI implementations.

Coverage now includes the exact bundled Hover's common screens plus this exact
provided RC25 panel. Other private/custom panels still need their own integration.
The X is still native-rendered text: original X image/resource identification,
font rendering, pointer hover, geometry, modal focus and GPU behavior remain live
verification work. No claim is made that the original game lacks an X icon.
