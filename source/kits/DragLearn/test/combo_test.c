/*
 * WheelRegion + DragLearn in one process (the two convenience mods together).
 * Both cores run their real workers on the same game image; checks that both hook,
 * that their 6 patched byte ranges and their blocks do not overlap, and that both restore.
 * build: see test/run_combo.sh  (needs the WheelRegion kit next to this kit)
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wchar.h>
#include "drag_learn.h"
#include "wheel_region.h"

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("  FAIL: %s\n", msg); } else printf("  ok: %s\n", msg); } while (0)
static uint8_t *IMG;
void *GetModuleHandleW(const wchar_t *n) { (void)n; return IMG; }
void *VirtualAlloc(void *a, size_t n, unsigned long t, unsigned long p) {
    void *r; (void)t; (void)p;
    if (!a) { r = mmap(0, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); return r == MAP_FAILED ? 0 : r; }
    r = mmap(a, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (r == MAP_FAILED) return 0;
    if (r != a) { munmap(r, n); return 0; }
    return r;
}
int VirtualFree(void *a, size_t n, unsigned long t) { (void)a; (void)n; (void)t; return 1; }
int VirtualProtect(void *a, size_t n, unsigned long p, unsigned long *old) { (void)a; (void)n; (void)p; *old = 0x20; return 1; }
size_t VirtualQuery(const void *a, void *m, size_t n) {
    struct { void *base, *ab; unsigned long ap; unsigned short pa, p0; size_t rs; unsigned long st, pr, ty, p1; } *x = m;
    (void)n; x->base = (void *)((uintptr_t)a & ~(uintptr_t)0xFFF); x->rs = (size_t)1 << 40; x->st = 0x1000; x->pr = 4; return sizeof *x;
}
int FlushInstructionCache(void *p, const void *a, size_t n) { (void)p; (void)a; (void)n; return 1; }
void *GetCurrentProcess(void) { return (void *)(intptr_t)-1; }
unsigned long GetCurrentProcessId(void) { return 1; }
typedef struct { unsigned long (*fn)(void *); void *arg; } TA;
static void *tm(void *p) { TA a = *(TA *)p; free(p); a.fn(a.arg); return 0; }
void *CreateThread(void *s, size_t z, unsigned long (*fn)(void *), void *arg, unsigned long f, unsigned long *id) {
    pthread_t t; TA *a = malloc(sizeof *a); (void)s; (void)z; (void)f; (void)id;
    a->fn = fn; a->arg = arg; if (pthread_create(&t, 0, tm, a)) return 0; pthread_detach(t); return (void *)1;
}
int CloseHandle(void *h) { (void)h; return 1; }
void Sleep(unsigned long ms) { usleep(ms * 1000); }
short GetAsyncKeyState(int vk) { (void)vk; return 0; }
unsigned int SendInput(unsigned int n, void *i, int s) { (void)n; (void)i; (void)s; return 1; }
void *GetForegroundWindow(void) { return 0; }
unsigned long GetWindowThreadProcessId(void *w, unsigned long *p) { (void)w; *p = 0; return 0; }
int GetSystemMetrics(int i) { (void)i; return 0; }

static uint8_t *snap;
static size_t SZ;
typedef struct { uintptr_t lo, hi; } R;
static int changed(R *out, int max) { /* byte ranges that differ from the original image */
    int n = 0; size_t i = 0;
    while (i < SZ && n < max) {
        if (IMG[i] != snap[i]) { size_t j = i; while (j < SZ && (IMG[j] != snap[j] || (j + 1 < SZ && IMG[j + 1] != snap[j + 1]))) j++; out[n].lo = i; out[n].hi = j + 1; n++; i = j + 1; }
        else i++;
    }
    return n;
}
static void lg(const char *l) { printf("    [log] %s\n", l); }
int main(int argc, char **argv) {
    FILE *f = fopen(argv[1], "rb");
    R r[16];
    int n, i, w = 0;
    fseek(f, 0, SEEK_END); SZ = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    IMG = mmap((void *)0x140000000, SZ, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (fread(IMG, 1, SZ, f) != SZ) return 2;
    fclose(f);
    snap = malloc(SZ); memcpy(snap, IMG, SZ);
    printf("== WheelRegion + DragLearn together on %s\n", argv[1]);
    { WR_Host h = {sizeof h, lg, 0}; WR_Start(&h, 0); }
    { DL_Host h = {sizeof h, lg, 0}; DL_Start(&h, 0); }
    while (w < 30000 && (!(WR_GetState() & WR_STATE_HOOKED) || !(DL_GetState() & DL_STATE_HOOKED))) { usleep(10000); w += 10; }
    CHECK(WR_GetState() & WR_STATE_HOOKED, "WheelRegion hooked");
    CHECK(DL_GetState() & DL_STATE_HOOKED, "DragLearn hooked");
    n = changed(r, 16);
    for (i = 0; i < n; i++) printf("    patched exe+%lX..+%lX\n", (unsigned long)r[i].lo, (unsigned long)r[i].hi);
    CHECK(n == 9, "9 separate patched ranges (WheelRegion 3 + DragLearn 6), none shared");
    DL_Stop(); WR_Stop();
    CHECK(memcmp(IMG, snap, SZ) == 0, "after both stop: image identical to the original");
    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
