/*
 * SAN14_WheelRegion.dll - ready-made wrapper around src/wheel_region.c
 *
 * One DLL, three ways to load it:
 *   1. SAN14 ModCore (rollback integrated patch): put the DLL in the game folder and
 *      SAN14Mods\modules\WheelRegion.ini next to the other modules. ModCore calls
 *      SAN14_ModInitV1 and the three patches go through its hook registry.
 *   2. Any loader that calls an export: SAN14_WheelRegionStart().
 *   3. Any loader that only does LoadLibrary (or Cheat Engine "Inject DLL"): DllMain starts
 *      a thread that starts the feature after 3 s unless 1 or 2 already did.
 *
 * Files next to the DLL (all optional):
 *   SAN14_WheelRegion.ini          settings (see the file)
 *   SAN14_WheelRegion_targets.bin  48400 x u16 table for a modified map (tools/build_targets.py)
 *   SAN14_WheelRegion.log          written by the DLL
 */
#include <stddef.h>
#include <stdint.h>
#include "../src/wheel_region.h"

#define IMP __declspec(dllimport)
#define API __stdcall
IMP unsigned long API GetModuleFileNameW(void *, wchar_t *, unsigned long);
IMP void *API CreateFileW(const wchar_t *, unsigned long, unsigned long, void *, unsigned long, unsigned long, void *);
IMP int API WriteFile(void *, const void *, unsigned long, unsigned long *, void *);
IMP int API ReadFile(void *, void *, unsigned long, unsigned long *, void *);
IMP unsigned long API GetFileSize(void *, unsigned long *);
IMP int API CloseHandle(void *);
IMP unsigned int API GetPrivateProfileIntW(const wchar_t *, const wchar_t *, int, const wchar_t *);
IMP void *API CreateThread(void *, size_t, unsigned long(API *)(void *), void *, unsigned long, unsigned long *);
IMP void API Sleep(unsigned long);
IMP int API DisableThreadLibraryCalls(void *);
IMP void *API VirtualAlloc(void *, size_t, unsigned long, unsigned long);
IMP int API VirtualFree(void *, size_t, unsigned long);
IMP unsigned long API GetFileAttributesW(const wchar_t *);
IMP void *API GetModuleHandleW(const wchar_t *);
IMP void *API GetProcAddress(void *, const char *);
IMP unsigned long long API GetTickCount64(void);

#define INVALID ((void *)(intptr_t)-1)

static void *g_self;
static wchar_t g_dir[520];
static unsigned g_dirlen;
static void *g_logfile = INVALID;
static volatile long g_begun;

/* ---------------------------------------------------------------- files next to the DLL */
static void path_of(wchar_t *out, const wchar_t *name) {
    unsigned i = 0, j = 0;
    for (; i < g_dirlen && i < 500; i++) out[i] = g_dir[i];
    while (name[j] && i < 518) out[i++] = name[j++];
    out[i] = 0;
}
static void file_log(const char *line) {
    unsigned long n = 0, w;
    if (g_logfile == INVALID) {
        wchar_t p[520];
        path_of(p, L"SAN14_WheelRegion.log");
        g_logfile = CreateFileW(p, 0x40000000 /*GENERIC_WRITE*/, 1 /*SHARE_READ*/, 0, 2 /*CREATE_ALWAYS*/, 0x80, 0);
        if (g_logfile == INVALID) return;
    }
    while (line[n]) n++;
    WriteFile(g_logfile, line, n, &w, 0);
    WriteFile(g_logfile, "\r\n", 2, &w, 0);
}
static void read_config(WR_Config *c) {
    wchar_t ini[520];
    unsigned char *z = (unsigned char *)c;
    unsigned i;
    for (i = 0; i < sizeof *c; i++) z[i] = 0;
    c->size = sizeof *c;
    path_of(ini, L"SAN14_WheelRegion.ini");
    c->trigger_vk = (int)GetPrivateProfileIntW(L"WheelRegion", L"TriggerKey", 4, ini);
    c->hold_ms = (int)GetPrivateProfileIntW(L"WheelRegion", L"HoldMs", 600, ini);
    c->normal_mode_only = (int)GetPrivateProfileIntW(L"WheelRegion", L"NormalModeOnly", 0, ini);
    c->game_callers_only = (int)GetPrivateProfileIntW(L"WheelRegion", L"GameCallersOnly", 0, ini);
}
static int enabled(void) {
    wchar_t ini[520];
    path_of(ini, L"SAN14_WheelRegion.ini");
    return GetPrivateProfileIntW(L"WheelRegion", L"Enabled", 1, ini) != 0;
}
static void load_targets(void) {
    wchar_t p[520];
    void *f, *buf;
    unsigned long got = 0;
    path_of(p, L"SAN14_WheelRegion_targets.bin");
    f = CreateFileW(p, 0x80000000 /*GENERIC_READ*/, 1, 0, 3 /*OPEN_EXISTING*/, 0x80, 0);
    if (f == INVALID) return;
    if (GetFileSize(f, 0) == 96800 && (buf = VirtualAlloc(0, 96800, 0x3000, 0x04))) {
        if (ReadFile(f, buf, 96800, &got, 0) && got == 96800 && WR_SetTargets((const unsigned short *)buf, 48400))
            file_log("WheelRegion: using SAN14_WheelRegion_targets.bin");
        VirtualFree(buf, 0, 0x8000);
    }
    CloseHandle(f);
}

/* ---------------------------------------------------------------- start paths */
static int begin(const WR_Host *host) {
    WR_Config c;
    if (__sync_val_compare_and_swap(&g_begun, 0, 1) != 0) return 1;
    if (!enabled()) { file_log("WheelRegion: disabled in SAN14_WheelRegion.ini"); return 1; }
    read_config(&c);
    load_targets();
    return WR_Start(host, &c);
}

__declspec(dllexport) int SAN14_WheelRegionStart(void) {
    WR_Host h = {sizeof(WR_Host), file_log, 0};
    return begin(&h);
}
__declspec(dllexport) void SAN14_WheelRegionStop(void) { WR_Stop(); }
__declspec(dllexport) unsigned int SAN14_WheelRegionState(void) { return WR_GetState(); }
__declspec(dllexport) unsigned int SAN14_WheelRegionCounter(int which) { return WR_GetCounter(which); }
__declspec(dllexport) void SAN14_WheelRegionSetEnabled(int on) { WR_SetEnabled(on); }

/* ---------------------------------------------------------------- SAN14 ModCore module */
typedef struct {
    uint32_t size, abi, layout_abi, symbol_count, offset_count, image_size;
    unsigned char *image;
    const uintptr_t *symbols;
    const uint32_t *offsets;
    void *ReadLimits;
    const char *profile;
} S14_Resolved;
typedef struct {
    uint32_t size, abi;
    const S14_Resolved *resolved;
    int32_t (*Readable)(const void *, uintptr_t);
    int32_t (*InstallHooks)(const char *, const WR_Patch *, uint32_t); /* S14_Hook == WR_Patch */
    unsigned char *(*MakeRelay)(const void *, const void *);
    void (*FreeRelay)(void *);
    void (*Log)(const char *);
} S14_CoreAPI;
static const S14_CoreAPI *g_core;
static void core_log(const char *line) { file_log(line); if (g_core && g_core->Log) g_core->Log(line); }
/* 2026-11-10 통합패치 P202 (교훈 L030): 고치는 자리 K(수비대 읽기 함수 입구, 새 판 exe+0x232F60)가 통합편의
 * (SAN14_IntegratedConvenience.dll)의 시작 확인 구간(전역 확인 115번) 안이다. 통합편의는 켜지고 1초쯤 뒤 그 자리가
 * 원래 모양인지 한 번 확인하고, 다르면 5번(수행·대열 이동)을 조용히 안 켠다. 누가 먼저냐는 PC 속도에 달려 일부 PC 에서만 났다.
 * → ModCore 로 고칠 때는 통합편의가 확인을 끝낸 뒤(SAN14_WaitConvenienceV1 = 1)에 고친다.
 *   통합편의가 안 깔렸거나 꺼져 있으면 바로. 60초 안에 안 올라오거나 10분이 지나도 안 끝나면 그냥 고친다(예전 동작).
 *   이 함수는 WheelRegion 워커 스레드에서 불리므로 여기서 기다려도 게임은 안 막힌다. (src/ic_after_restore.h 와 같은 규칙) */
#include "wr_ic_wait.h"
static int core_patch(const char *group, const WR_Patch *p, unsigned int n) {
    char m[96];
    const char *how = wait_ic_restore(), *a = "WheelRegion: installing after IC restore check (";
    unsigned i = 0;
    while (*a && i < 60) m[i++] = *a++;
    while (*how && i < 90) m[i++] = *how++;
    m[i++] = ')'; m[i++] = '.'; m[i] = 0;
    core_log(m);
    return g_core->InstallHooks(group, p, n) != 0;
}
typedef char wr_layout_check[(sizeof(WR_Patch) == 48 && offsetof(WR_Patch, expected) == 12 && offsetof(WR_Patch, replacement) == 28) ? 1 : -1];

__declspec(dllexport) uint32_t SAN14_ModInitV1(const S14_CoreAPI *api) {
    WR_Host h = {sizeof(WR_Host), core_log, 0};
    if (!api || api->size < sizeof(S14_CoreAPI) || api->abi != 0x00010000u || !api->InstallHooks) return 0;
    g_core = api;
    h.patch_group = core_patch;
    return (uint32_t)begin(&h);
}

/* ---------------------------------------------------------------- DllMain */
static unsigned long API autostart(void *unused) {
    (void)unused;
    Sleep(3000);
    if (!g_begun) SAN14_WheelRegionStart();
    return 0;
}

int API DllMain(void *inst, unsigned long reason, void *reserved) {
    (void)reserved;
    if (reason == 1 /*DLL_PROCESS_ATTACH*/) {
        unsigned long n, i;
        g_self = inst;
        DisableThreadLibraryCalls(inst);
        n = GetModuleFileNameW(inst, g_dir, 512);
        for (i = n; i > 0 && g_dir[i - 1] != L'\\' && g_dir[i - 1] != L'/'; i--) {}
        g_dirlen = i;
        g_dir[i] = 0;
        {
            void *t = CreateThread(0, 0, autostart, 0, 0, 0);
            if (t) CloseHandle(t);
        }
    }
    return 1;
}
