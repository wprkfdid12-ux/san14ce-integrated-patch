/*
 * RTK14 WheelRegion  -  "지역 휠클릭 → 그 지역 거점 선택" 편의 기능
 * Romance of the Three Kingdoms XIV (CE, x64): wheel-click (middle button) on an empty tile
 * of a region acts as a left click on that region's village / port / city / gate, even when
 * an army stands on it. Normal clicks are untouched.
 *
 * Integration (host mod):
 *   1. add src/wheel_region.c to the build (it includes wr_block.h and wr_targets.h)
 *   2. link kernel32 + user32 (no CRT needed; the host must provide memcpy/memset like any
 *      freestanding build)
 *   3. call WR_Start(&host, &cfg) once, from any thread, any time after the game process
 *      started (it waits by itself until the game code is unpacked by Steam)
 *
 * The C API is plain C (no C++/CRT objects cross it). All functions are thread safe to call.
 */
#ifndef RTK14_WHEEL_REGION_H
#define RTK14_WHEEL_REGION_H

#ifdef __cplusplus
extern "C" {
#endif

#define WR_API_VERSION 1

/* One code patch. Same memory layout as S14_Hook of the SAN14 ModCore loader, so a ModCore
   module can pass an array of these straight to api->InstallHooks. */
typedef struct WR_Patch {
    unsigned char *target;          /* address inside the game .text */
    unsigned int length;            /* 5..16 */
    unsigned char expected[16];     /* bytes that must be there now */
    unsigned char replacement[16];  /* bytes to write (E9 rel32 + NOP padding) */
} WR_Patch;

typedef struct WR_Host {
    unsigned int size;              /* sizeof(WR_Host) */
    /* optional: one log line (ASCII, no newline). NULL = no log. */
    void (*log)(const char *line);
    /* optional: let the host install the three patches as one all-or-nothing group
       (for hook registries such as ModCore). Return nonzero when installed.
       NULL = WheelRegion writes them itself (VirtualProtect + verify + write). */
    int (*patch_group)(const char *group, const WR_Patch *patches, unsigned int count);
} WR_Host;

typedef struct WR_Config {
    unsigned int size;              /* sizeof(WR_Config) */
    int trigger_vk;                 /* 0 = VK_MBUTTON (0x04). -1 = no polling: host calls WR_Trigger() */
    int hold_ms;                    /* 0 = 600. how long the redirect stays on after the click */
    int normal_mode_only;           /* 1 = only when nothing is selected (CHexFocusModeHex) */
    int game_callers_only;          /* 1 = K hides the garrison only from game code, not other mods */
    int wait_seconds;               /* 0 = 300. max wait for the game code to be unpacked */
    int poll_ms;                    /* 0 = 15. button polling interval */
} WR_Config;

/* State bits returned by WR_GetState() */
#define WR_STATE_STARTED        0x0001u  /* worker thread running */
#define WR_STATE_FOUND          0x0002u  /* all code sites resolved */
#define WR_STATE_HOOKED         0x0004u  /* patches active */
#define WR_STATE_BUSY           0x0008u  /* a redirect is in progress right now */
#define WR_STATE_FOCUS_CHECK    0x0010u  /* normal_mode_only is active (vtable found) */
#define WR_STATE_STOPPED        0x0020u
#define WR_STATE_FAIL_TIMEOUT   0x0100u  /* game code never appeared (not unpacked / other exe) */
#define WR_STATE_FAIL_PATTERN   0x0200u  /* a site was missing or not unique (other game version) */
#define WR_STATE_FAIL_MEMORY    0x0400u  /* no page near the game image */
#define WR_STATE_FAIL_PATCH     0x0800u  /* bytes differ (another mod patched a site) or host refused */
#define WR_STATE_FAIL_TABLE     0x1000u

/* Start the feature. host/cfg may be NULL (defaults). host and cfg are copied.
   Returns 1 if started (or already running), 0 on bad arguments. */
int WR_Start(const WR_Host *host, const WR_Config *cfg);

/* Stop polling and restore the original bytes (only for patches WheelRegion wrote itself). */
void WR_Stop(void);

/* WR_STATE_* bits */
unsigned int WR_GetState(void);

/* Diagnostics: 0 redirects started, 1 A hits, 2 B hits, 3 K calls, 4 K hits */
unsigned int WR_GetCounter(int which);

/* Replace the built-in table (call before WR_Start). tiles = 48400 entries,
   tile -> target tile or 0xFFFF. The data is copied. Returns 1 on success. */
int WR_SetTargets(const unsigned short *tiles, unsigned int count);

/* With trigger_vk = -1: ask for one redirect at the current mouse position, as if the wheel
   was clicked now. Safe from any thread (the work runs on the WheelRegion worker). */
void WR_Trigger(void);
/* 0 = act like the original game (the trigger button does nothing), 1 = on again */
void WR_SetEnabled(int on);

#ifdef __cplusplus
}
#endif
#endif
