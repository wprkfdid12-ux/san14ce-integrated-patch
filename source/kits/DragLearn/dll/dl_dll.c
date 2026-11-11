/*
 * SAN14_DragLearn.dll - ready-made wrapper around src/drag_learn.c  (1인 드래그습득)
 *
 * One DLL, four ways to load it:
 *   1. SAN14Conv loader (version.dll in the game folder, this kit's loader/): put the DLL and
 *      its ini in <game>\SAN14Conv\. The loader LoadLibrary()s it; DllMain starts it.
 *   2. SAN14 ModCore (rollback integrated patch): SAN14Mods\modules\DragLearn.ini.
 *      ModCore calls SAN14_ModInitV1 and the patches go through its hook registry.
 *   3. Any loader that calls an export: SAN14_DragLearnStart().
 *   4. Cheat Engine "Inject DLL" / plain LoadLibrary: DllMain starts it after 3 s.
 *
 * Files next to the DLL (optional): SAN14_DragLearn.ini (settings), SAN14_DragLearn.log (written)
 */
#include <stddef.h>
#include <stdint.h>
#include "../src/drag_learn.h"

#define IMP __declspec(dllimport)
#define API __stdcall
IMP unsigned long API GetModuleFileNameW(void *, wchar_t *, unsigned long);
IMP void *API CreateFileW(const wchar_t *, unsigned long, unsigned long, void *, unsigned long, unsigned long, void *);
IMP int API WriteFile(void *, const void *, unsigned long, unsigned long *, void *);
IMP int API CloseHandle(void *);
IMP unsigned int API GetPrivateProfileIntW(const wchar_t *, const wchar_t *, int, const wchar_t *);
IMP void *API CreateThread(void *, size_t, unsigned long(API *)(void *), void *, unsigned long, unsigned long *);
IMP void API Sleep(unsigned long);
IMP int API DisableThreadLibraryCalls(void *);

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
        path_of(p, L"SAN14_DragLearn.log");
        g_logfile = CreateFileW(p, 0x40000000 /*GENERIC_WRITE*/, 1 /*SHARE_READ*/, 0, 2 /*CREATE_ALWAYS*/, 0x80, 0);
        if (g_logfile == INVALID) return;
    }
    while (line[n]) n++;
    WriteFile(g_logfile, line, n, &w, 0);
    WriteFile(g_logfile, "\r\n", 2, &w, 0);
}
static void read_config(DL_Config *c) {
    wchar_t ini[520];
    unsigned char *z = (unsigned char *)c;
    unsigned i;
    for (i = 0; i < sizeof *c; i++) z[i] = 0;
    c->size = sizeof *c;
    path_of(ini, L"SAN14_DragLearn.ini");
    c->max_select = (int)GetPrivateProfileIntW(L"DragLearn", L"MaxSelect", 10, ini);
    c->max_generals = (int)GetPrivateProfileIntW(L"DragLearn", L"MaxGenerals", 10, ini);
}
static int enabled(void) {
    wchar_t ini[520];
    path_of(ini, L"SAN14_DragLearn.ini");
    return GetPrivateProfileIntW(L"DragLearn", L"Enabled", 1, ini) != 0;
}
/* ---------------------------------------------------------------- start paths */
static int begin(const DL_Host *host) {
    DL_Config c;
    if (__sync_val_compare_and_swap(&g_begun, 0, 1) != 0) return 1;
    if (!enabled()) { file_log("DragLearn: disabled in SAN14_DragLearn.ini"); return 1; }
    read_config(&c);
    return DL_Start(host, &c);
}

__declspec(dllexport) int SAN14_DragLearnStart(void) {
    DL_Host h = {sizeof(DL_Host), file_log, 0};
    return begin(&h);
}
__declspec(dllexport) void SAN14_DragLearnStop(void) { DL_Stop(); }
__declspec(dllexport) unsigned int SAN14_DragLearnState(void) { return DL_GetState(); }
__declspec(dllexport) unsigned int SAN14_DragLearnCounter(int which) { return DL_GetCounter(which); }
__declspec(dllexport) void SAN14_DragLearnSetEnabled(int on) { DL_SetEnabled(on); }

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
    int32_t (*InstallHooks)(const char *, const DL_Patch *, uint32_t); /* S14_Hook == DL_Patch */
    unsigned char *(*MakeRelay)(const void *, const void *);
    void (*FreeRelay)(void *);
    void (*Log)(const char *);
} S14_CoreAPI;
static const S14_CoreAPI *g_core;
static void core_log(const char *line) { file_log(line); if (g_core && g_core->Log) g_core->Log(line); }
static int core_patch(const char *group, const DL_Patch *p, unsigned int n) {
    return g_core->InstallHooks(group, p, n) != 0;
}
typedef char dl_layout_check[(sizeof(DL_Patch) == 48 && offsetof(DL_Patch, expected) == 12 && offsetof(DL_Patch, replacement) == 28) ? 1 : -1];

__declspec(dllexport) uint32_t SAN14_ModInitV1(const S14_CoreAPI *api) {
    DL_Host h = {sizeof(DL_Host), core_log, 0};
    if (!api || api->size < sizeof(S14_CoreAPI) || api->abi != 0x00010000u || !api->InstallHooks) return 0;
    g_core = api;
    h.patch_group = core_patch;
    return (uint32_t)begin(&h);
}

/* ---------------------------------------------------------------- DllMain */
static unsigned long API autostart(void *unused) {
    (void)unused;
    Sleep(3000);
    if (!g_begun) SAN14_DragLearnStart();
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
