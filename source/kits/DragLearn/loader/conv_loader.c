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
        if (LoadLibraryW(path)) { log_line("SAN14Conv loader: loaded ", fd.name); count++; }
        else log_line("SAN14Conv loader: FAILED to load ", fd.name);
    } while (FindNextFileW(f, &fd));
    FindClose(f);
    log_line(count ? "SAN14Conv loader: done" : "SAN14Conv loader: nothing loaded", 0);
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
