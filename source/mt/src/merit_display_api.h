#ifndef SAN14_MERIT_DISPLAY_API_H
#define SAN14_MERIT_DISPLAY_API_H
/* Optional in-process, Win64/UI-thread-only display API.
 * Resolve SAN14_MeritSlotDisplayV1 from the ALREADY LOADED
 * SAN14_MeritTraits.dll using GetModuleHandleW/GetProcAddress. Never load or
 * initialize another mod from DllMain. Missing export / return 0: keep your
 * existing native display. It may become available after a scenario load.
 *
 * officer_id: native database ID; physical_slot: 0..8 (not compacted index).
 * Both outputs must point to writable caller-owned storage.
 * return 1: actual_trait is the current native ID (0 for empty); is_x is 1
 * only for an empty physical slot occupied by a CONFIRMED MISS.
 * return 0: unavailable/invalid, supplied outputs cleared. Caller falls back.
 * Real IDs, captions, colors and descriptions still come from native APIs.
 * Never register X in a trait database, effects getter, save or tooltip ID.
 * The call does not change native officer data; it can clear stale sidecar X
 * bits when an external editor has filled that exact physical slot.
 */
typedef int (*SAN14_MeritSlotDisplayV1_Fn)(unsigned int officer_id,
    unsigned int physical_slot,unsigned short *actual_trait,unsigned int *is_x);
#endif
