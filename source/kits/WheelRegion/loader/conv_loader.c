/*
 * SAN14Conv loader  -  version.dll proxy for Romance of the Three Kingdoms XIV CE
 *
 * The game imports VERSION.dll, so Windows loads this file from the game folder first.
 *   - every version.dll function is forwarded to the real System32\version.dll
 *   - only inside the game (exe name starts with "SAN14CE"), a thread loads every
 *     <game>\SAN14Conv\*.dll (the convenience mods: SAN14_WheelRegion.dll,
 *     SAN14_DragLearn.dll, ...). Each mod starts itself from its own DllMain.
 *   - log: <game>\SAN14Conv\SAN14Conv_loader.log
 * Nothing in the game is patched by the loader itself.
 *
 * v2: vanilla <-> mod switch for comparing with the original game.
 *   Ctrl+F12 (while the game window is in front) calls "<dll name>SetEnabled(0/1)" of every loaded
 *   mod, e.g. SAN14_DragLearn.dll -> SAN14_DragLearnSetEnabled. The game keeps running; the mods'
 *   patches stay in place but behave like the original game while off.
 *   The game window title shows the state ("[모드 ON]" / "[바닐라]") and a sound plays.
 *   Settings: <game>\SAN14Conv\SAN14Conv.ini  [Loader] ToggleKey=123 (F12), ToggleCtrl=1, ToggleShift=0
 *   New mods: export  void <dll name>SetEnabled(int on)  to take part.
 *
 * Other loaders in use (2026-10): dinput8.dll (Fog of War), xinput1_3.dll (Batteki).
 * version.dll / dxgi.dll / dinput8.dll are also used by the rollback integrated patch
 * (ModCore) - on that build use the ModCore module ini instead of this loader.
 */
#include <stddef.h>
#include <stdint.h>

#define IMP __declspec(dllimport)
#define API __stdcall
IMP unsigned int API GetSystemDirectoryW(wchar_t *, unsigned int);
IMP void *API LoadLibraryW(const wchar_t *);
IMP void *API GetProcAddress(void *, const char *);
IMP unsigned long API GetModuleFileNameW(void *, wchar_t *, unsigned long);
IMP void *API CreateThread(void *, size_t, unsigned long(API *)(void *), void *, unsigned long, unsigned long *);
IMP int API CloseHandle(void *);
IMP int API DisableThreadLibraryCalls(void *);
IMP void *API CreateFileW(const wchar_t *, unsigned long, unsigned long, void *, unsigned long, unsigned long, void *);
IMP int API WriteFile(void *, const void *, unsigned long, unsigned long *, void *);
IMP void *API FindFirstFileW(const wchar_t *, void *);
IMP int API FindNextFileW(void *, void *);
IMP int API FindClose(void *);
IMP int API WideCharToMultiByte(unsigned int, unsigned long, const wchar_t *, int, char *, int, const char *, int *);
IMP unsigned int API GetPrivateProfileIntW(const wchar_t *, const wchar_t *, int, const wchar_t *);
IMP void API Sleep(unsigned long);
IMP unsigned long API GetCurrentProcessId(void);


#define INVALID ((void *)(intptr_t)-1)

/* ---------------------------------------------------------------- forwarded exports */
#define NFN 17
static const char *const k_names[NFN] = {
    "GetFileVersionInfoA", "GetFileVersionInfoByHandle", "GetFileVersionInfoExA", "GetFileVersionInfoExW",
    "GetFileVersionInfoSizeA", "GetFileVersionInfoSizeExA", "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW",
    "GetFileVersionInfoW", "VerFindFileA", "VerFindFileW", "VerInstallFileA", "VerInstallFileW",
    "VerLanguageNameA", "VerLanguageNameW", "VerQueryValueA", "VerQueryValueW"};
void *conv_real[NFN];

static int64_t API missing_fn(void) { return 0; } /* a function the real DLL lacks: "failed" */

#define FWD(i, name) \
    ".globl " #name "\n" #name ":\n  jmpq *conv_real+" #i "*8(%rip)\n" \
    ".section .drectve,\"yn\"\n  .ascii \" -export:" #name "\"\n.text\n"
__asm__(".text\n"
        FWD(0, GetFileVersionInfoA) FWD(1, GetFileVersionInfoByHandle) FWD(2, GetFileVersionInfoExA)
        FWD(3, GetFileVersionInfoExW) FWD(4, GetFileVersionInfoSizeA) FWD(5, GetFileVersionInfoSizeExA)
        FWD(6, GetFileVersionInfoSizeExW) FWD(7, GetFileVersionInfoSizeW) FWD(8, GetFileVersionInfoW)
        FWD(9, VerFindFileA) FWD(10, VerFindFileW) FWD(11, VerInstallFileA) FWD(12, VerInstallFileW)
        FWD(13, VerLanguageNameA) FWD(14, VerLanguageNameW) FWD(15, VerQueryValueA) FWD(16, VerQueryValueW));

static void bind_real(void) {
    wchar_t p[300];
    unsigned n = GetSystemDirectoryW(p, 260), i;
    const wchar_t *tail = L"\\version.dll";
    void *h = 0;
    if (n && n < 260) {
        for (i = 0; tail[i]; i++) p[n + i] = tail[i];
        p[n + i] = 0;
        h = LoadLibraryW(p);
    }
    for (i = 0; i < NFN; i++) {
        void *f = h ? GetProcAddress(h, k_names[i]) : 0;
        conv_real[i] = f ? f : (void *)missing_fn;
    }
}

/* ---------------------------------------------------------------- plugin folder */
static wchar_t g_dir[520]; /* "<game>\SAN14Conv\" */
static unsigned g_dirlen;
static void *g_log = INVALID;

static void wcat(wchar_t *out, unsigned *n, const wchar_t *s) { while (*s && *n < 515) out[(*n)++] = *s++; out[*n] = 0; }
static void log_line(const char *a, const wchar_t *w) {
    char buf[600];
    unsigned long n = 0, wr;
    if (g_log == INVALID) {
        wchar_t p[540];
        unsigned k = 0;
        wcat(p, &k, g_dir);
        wcat(p, &k, L"SAN14Conv_loader.log");
        g_log = CreateFileW(p, 0x40000000, 1, 0, 2, 0x80, 0);
        if (g_log == INVALID) return;
    }
    while (a[n] && n < 200) { buf[n] = a[n]; n++; }
    if (w) { int k = WideCharToMultiByte(65001, 0, w, -1, buf + n, 380, 0, 0); if (k > 1) n += (unsigned long)(k - 1); }
    buf[n++] = '\r';
    buf[n++] = '\n';
    WriteFile(g_log, buf, n, &wr, 0);
}

typedef struct {
    unsigned long attr;
    unsigned long t[6];
    unsigned long size_hi, size_lo, r0, r1;
    wchar_t name[260];
    wchar_t alt[14];
    unsigned long extra[3];
} FindData; /* WIN32_FIND_DATAW (592 bytes) + slack */

/* ---------------------------------------------------------------- vanilla <-> mod switch */
typedef void(API *SetEnabledFn)(int); /* x64: one calling convention */
typedef short(API *FnKey)(int);
typedef void *(API *FnFg)(void);
typedef unsigned long(API *FnPid)(void *, unsigned long *);
typedef int(API *FnGetText)(void *, wchar_t *, int);
typedef int(API *FnSetText)(void *, const wchar_t *);
typedef int(API *FnBeep)(unsigned int);
#define MAXMOD 32
static SetEnabledFn g_set[MAXMOD];
static unsigned g_nset;
static wchar_t g_title[300];
static void *g_titled;

static void title_state(void *w, int on, FnGetText gt, FnSetText st) {
    static const wchar_t s_on[] = L"  [\xBAA8\xB4DC ON]";                       /* [모드 ON] */
    static const wchar_t s_off[] = L"  [\xBC14\xB2D0\xB77C - \xBAA8\xB4DC OFF]"; /* [바닐라 - 모드 OFF] */
    wchar_t t[400];
    unsigned n = 0, i;
    const wchar_t *add = on ? s_on : s_off;
    if (!w || !gt || !st) return;
    if (g_titled != w) { /* remember the game's own title once */
        int k = gt(w, g_title, 280);
        if (k < 0) k = 0;
        g_title[k] = 0;
        g_titled = w;
    }
    for (i = 0; g_title[i] && n < 300; i++) t[n++] = g_title[i];
    for (i = 0; add[i] && n < 390; i++) t[n++] = add[i];
    t[n] = 0;
    st(w, t);
}

static unsigned long API toggle_thread(void *unused) {
    wchar_t ini[540];
    unsigned k = 0, i;
    int key, ctrl, shift, on = 1, prev = 0;
    void *u = LoadLibraryW(L"user32.dll");
    FnKey ks = u ? (FnKey)GetProcAddress(u, "GetAsyncKeyState") : 0;
    FnFg fg = u ? (FnFg)GetProcAddress(u, "GetForegroundWindow") : 0;
    FnPid wp = u ? (FnPid)GetProcAddress(u, "GetWindowThreadProcessId") : 0;
    FnGetText gt = u ? (FnGetText)GetProcAddress(u, "GetWindowTextW") : 0;
    FnSetText st = u ? (FnSetText)GetProcAddress(u, "SetWindowTextW") : 0;
    FnBeep bp = u ? (FnBeep)GetProcAddress(u, "MessageBeep") : 0;
    (void)unused;
    wcat(ini, &k, g_dir);
    wcat(ini, &k, L"SAN14Conv.ini");
    key = (int)GetPrivateProfileIntW(L"Loader", L"ToggleKey", 0x7B, ini);
    ctrl = (int)GetPrivateProfileIntW(L"Loader", L"ToggleCtrl", 1, ini);
    shift = (int)GetPrivateProfileIntW(L"Loader", L"ToggleShift", 0, ini);
    if (!ks || !fg || !wp || key <= 0 || !g_nset) { log_line("SAN14Conv loader: switch off (no key or no mod with SetEnabled)", 0); return 0; }
    log_line("SAN14Conv loader: vanilla/mod switch ready (default Ctrl+F12)", 0);
    for (;;) {
        int down;
        Sleep(30);
        down = (ks(key) & 0x8000) != 0 && (!ctrl || (ks(0x11) & 0x8000)) && (!shift || (ks(0x10) & 0x8000));
        if (down && !prev) {
            void *w = fg();
            unsigned long pid = 0;
            if (w) wp(w, &pid);
            if (w && pid == GetCurrentProcessId()) {
                on = !on;
                for (i = 0; i < g_nset; i++) g_set[i](on);
                title_state(w, on, gt, st);
                if (bp) bp(on ? 0x40u : 0x10u); /* asterisk / hand sound */
                log_line(on ? "SAN14Conv loader: mods ON" : "SAN14Conv loader: mods OFF (vanilla)", 0);
            }
        }
        prev = down;
    }
}

static void find_switch(void *h, const wchar_t *file) {
    char name[160];
    unsigned n = 0, i;
    void *f;
    for (i = 0; file[i] && n < 120; i++) { /* "SAN14_X.dll" -> "SAN14_XSetEnabled" (ASCII names) */
        if (file[i] == L'.') break;
        if (file[i] > 0x7E) return;
        name[n++] = (char)file[i];
    }
    for (i = 0; "SetEnabled"[i]; i++) name[n++] = "SetEnabled"[i];
    name[n] = 0;
    f = GetProcAddress(h, name);
    if (f && g_nset < MAXMOD) { g_set[g_nset++] = (SetEnabledFn)f; log_line("SAN14Conv loader: switchable ", file); }
}

static unsigned long API load_plugins(void *unused) {
    wchar_t pat[540], path[540];
    FindData fd;
    void *f;
    unsigned k = 0, count = 0;
    (void)unused;
    log_line("SAN14Conv loader: start, folder ", g_dir);
    wcat(pat, &k, g_dir);
    wcat(pat, &k, L"*.dll");
    f = FindFirstFileW(pat, &fd);
    if (f == INVALID) { log_line("SAN14Conv loader: no plugin DLL found", 0); return 0; }
    do {
        if (fd.attr & 0x10) continue; /* directory */
        k = 0;
        wcat(path, &k, g_dir);
        wcat(path, &k, fd.name);
        {
            void *h = LoadLibraryW(path);
            if (h) { log_line("SAN14Conv loader: loaded ", fd.name); count++; find_switch(h, fd.name); }
            else log_line("SAN14Conv loader: FAILED to load ", fd.name);
        }
    } while (FindNextFileW(f, &fd));
    FindClose(f);
    log_line(count ? "SAN14Conv loader: done" : "SAN14Conv loader: nothing loaded", 0);
    if (g_nset) toggle_thread(0); /* this thread stays as the switch */
    return 0;
}

static int is_game(const wchar_t *exe, unsigned n) {
    unsigned i = n;
    static const wchar_t want[] = L"SAN14CE";
    while (i > 0 && exe[i - 1] != L'\\' && exe[i - 1] != L'/') i--;
    for (n = 0; want[n]; n++) {
        wchar_t c = exe[i + n];
        if (c >= L'a' && c <= L'z') c = (wchar_t)(c - 32);
        if (c != want[n]) return 0;
    }
    return 1;
}

int API DllMain(void *inst, unsigned long reason, void *reserved) {
    (void)reserved;
    if (reason == 1) {
        wchar_t exe[520];
        unsigned long n, i;
        DisableThreadLibraryCalls(inst);
        bind_real();
        n = GetModuleFileNameW(0, exe, 512);
        if (!n || !is_game(exe, (unsigned)n)) return 1;
        n = GetModuleFileNameW(inst, g_dir, 480);
        for (i = n; i > 0 && g_dir[i - 1] != L'\\' && g_dir[i - 1] != L'/'; i--) {}
        g_dirlen = (unsigned)i;
        g_dir[g_dirlen] = 0;
        wcat(g_dir, &g_dirlen, L"SAN14Conv\\");
        {
            void *t = CreateThread(0, 0, load_plugins, 0, 0, 0);
            if (t) CloseHandle(t);
        }
    }
    return 1;
}
