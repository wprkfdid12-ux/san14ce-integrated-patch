/*
 * RTK14 DragLearn  -  "드래그습득" (1안: one general x many books, 2안: many generals x a book)
 * Romance of the Three Kingdoms XIV (CE, x64).
 *
 * In the tactic-learning screen the book list AND the general list become the game's own
 * multi-select lists (checkboxes, drag, 일괄선택). On [결정] -> [예] every selected general
 * learns every selected book (when the game allows that pair), at the normal price
 * (1 book + 1 command order per learn).
 *   - generals: max selectable = min(cap, command orders)
 *   - books:    max = min(cap, command orders / generals, free slots of the shown general)
 *   - a limit of 1 -> the original single list
 *   - replace mode (10 slots full, a tactic chosen to delete) -> original single list
 *   - before every extra learn: allowed for that general (not learned, formation), a book
 *     left, orders >= cost, a free slot - otherwise skip / stop (nothing spent)
 *
 * Integration (host mod):
 *   1. add src/drag_learn.c to the build (it includes dl_block.h)
 *   2. link kernel32 (no CRT needed; provide memcpy/memset in a freestanding build)
 *   3. call DL_Start(&host, &cfg) once, any time after the game process started
 *      (it waits by itself until Steam has unpacked the game code)
 * Plain C API, no CRT objects cross it. Thread safe.
 */
#ifndef RTK14_DRAG_LEARN_H
#define RTK14_DRAG_LEARN_H

#ifdef __cplusplus
extern "C" {
#endif

#define DL_API_VERSION 2

/* One code patch. Same layout as WR_Patch (WheelRegion) and S14_Hook (SAN14 ModCore). */
typedef struct DL_Patch {
    unsigned char *target;
    unsigned int length;            /* 5..16 */
    unsigned char expected[16];
    unsigned char replacement[16];
} DL_Patch;

typedef struct DL_Host {
    unsigned int size;              /* sizeof(DL_Host) */
    void (*log)(const char *line);  /* optional, ASCII line without newline */
    /* optional: install all patches as one all-or-nothing group (hook registry).
       NULL = DragLearn writes them itself. */
    int (*patch_group)(const char *group, const DL_Patch *patches, unsigned int count);
} DL_Host;

typedef struct DL_Config {
    unsigned int size;              /* sizeof(DL_Config) */
    int max_select;                 /* 0 = 10. upper cap of books per batch (2..16) */
    int max_generals;               /* 0 = 10. upper cap of generals per batch (1 = one general only, ..16) */
    int wait_seconds;               /* 0 = 300. max wait for the game code to be unpacked */
} DL_Config;

#define DL_STATE_STARTED        0x0001u
#define DL_STATE_FOUND          0x0002u  /* all sites and game functions resolved */
#define DL_STATE_HOOKED         0x0004u
#define DL_STATE_STOPPED        0x0020u
#define DL_STATE_FAIL_TIMEOUT   0x0100u
#define DL_STATE_FAIL_PATTERN   0x0200u  /* other game version: feature off, game untouched */
#define DL_STATE_FAIL_MEMORY    0x0400u
#define DL_STATE_FAIL_PATCH     0x0800u  /* a site was already patched by someone else / host refused */

int DL_Start(const DL_Host *host, const DL_Config *cfg);
void DL_Stop(void);                 /* restores the original bytes it wrote itself */
unsigned int DL_GetState(void);
/* 0 book lists opened, 1 book lists in multi mode, 2 book selections remembered, 3 extra learns,
   4 stops by a guard, 5 extra learn calls refused by the game, 6 last book limit, 7 last orders,
   8 last used slots, 9 skipped pairs, 10 general lists opened, 11 general lists in multi mode,
   12 general selections remembered, 13 last general limit, 14 remembered generals,
   15 books kept selectable for other generals, 16 first general skipped */
unsigned int DL_GetCounter(int which);
/* turn the batch on/off at run time (1/0) without unpatching */
void DL_SetEnabled(int on);

#ifdef __cplusplus
}
#endif
#endif
