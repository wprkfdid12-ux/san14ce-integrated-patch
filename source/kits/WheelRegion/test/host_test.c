/*
 * Linux host test for src/wheel_region.c and the machine-code block.
 *
 * The real (Steam-unpacked) game image is mapped at its preferred base 0x140000000, the
 * Win32 functions are replaced by the mocks below, and the WheelRegion worker runs for real:
 * pattern search, global resolution, near-page allocation, patch writing, middle-button edge,
 * synthetic input sequence, restore on stop. The patched game sites are CALLED through the
 * Windows x64 ABI to prove the jumps and stubs behave.
 *
 * build: see test/run_tests.sh       usage: host_test <image.bin> <rvaA> <rvaB> <rvaK> <rvaMap> <rvaDb> <rvaFocusVt>
 * (image.bin is produced from the user's own exe for this test only; it is never shipped.)
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
#include "../src/wheel_region.h"
#include "../src/wr_block.h"
#include "../src/wr_targets.h"

#define MS __attribute__((ms_abi))
typedef int(MS *FnA)(void *, int *, int, int);
typedef int(MS *FnK)(void *);
typedef int(MS *FnTramp)(void *, int *, int, int);
typedef void(MS *FnFakeB)(void *, int);

static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("  FAIL: %s (line %d)\n", msg, __LINE__); } else printf("  ok: %s\n", msg); } while (0)

/* ------------------------------------------------------------------ mocks */
static uint8_t *IMG;
static size_t IMGSZ;
static volatile int g_mdown, g_events, g_check_at_down;
static uint32_t g_ev_flags[32];
static int g_ev_dx[32];
static uint64_t g_ev_extra[32];
static int g_tile_empty, g_target;
static uint8_t *g_building;
static uintptr_t g_rvaA, g_rvaK, g_rvaB;
static uint8_t *g_fakeb, *g_mapobj;
static const uint8_t FAKE_B[35];
static int g_down_info;
static int g_down_flag, g_down_a_ret, g_down_slot[2], g_down_k;

void *GetModuleHandleW(const wchar_t *n) { (void)n; return IMG; }
void *VirtualAlloc(void *a, size_t n, unsigned long t, unsigned long p) {
    void *r;
    (void)t; (void)p;
    if (!a) { r = mmap(0, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); return r == MAP_FAILED ? 0 : r; }
    r = mmap(a, n, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (r == MAP_FAILED) return 0;
    if (r != a) { munmap(r, n); return 0; }
    return r;
}
int VirtualFree(void *a, size_t n, unsigned long t) { (void)a; (void)n; (void)t; return 1; }
static int g_protects;
int VirtualProtect(void *a, size_t n, unsigned long p, unsigned long *old) { (void)a; (void)n; (void)p; *old = 0x20; g_protects++; return 1; }
typedef struct { void *base, *alloc_base; unsigned int alloc_protect; unsigned short partition, pad0; size_t region_size; unsigned int state, protect, type, pad1; } MbiLinux;
size_t VirtualQuery(const void *a, void *m, size_t n) {
    /* the core's Mbi uses "unsigned long" fields (32-bit on Windows). On Linux they are 64-bit,
       so fill the layout the core sees when compiled here. */
    struct { void *base, *alloc_base; unsigned long alloc_protect; unsigned short partition, pad0; size_t region_size; unsigned long state, protect, type, pad1; } *x = m;
    (void)n;
    x->base = (void *)((uintptr_t)a & ~(uintptr_t)0xFFF);
    x->region_size = (size_t)1 << 40;
    x->state = 0x1000;
    x->protect = 0x04;
    return sizeof *x;
}
int FlushInstructionCache(void *p, const void *a, size_t n) { (void)p; (void)a; (void)n; return 1; }
void *GetCurrentProcess(void) { return (void *)(intptr_t)-1; }
unsigned long GetCurrentProcessId(void) { return 1234; }
typedef struct { unsigned long (*fn)(void *); void *arg; } ThreadArg;
static void *thread_main(void *p) { ThreadArg a = *(ThreadArg *)p; free(p); a.fn(a.arg); return 0; }
void *CreateThread(void *s, size_t z, unsigned long (*fn)(void *), void *arg, unsigned long f, unsigned long *id) {
    pthread_t t;
    ThreadArg *a = malloc(sizeof *a);
    (void)s; (void)z; (void)f; (void)id;
    a->fn = fn; a->arg = arg;
    if (pthread_create(&t, 0, thread_main, a)) return 0;
    pthread_detach(t);
    return (void *)1;
}
int CloseHandle(void *h) { (void)h; return 1; }
void Sleep(unsigned long ms) { usleep(ms * 1000); }
short GetAsyncKeyState(int vk) { return (vk == 4 && g_mdown) ? (short)0x8000 : 0; }
void *GetForegroundWindow(void) { return (void *)1; }
unsigned long GetWindowThreadProcessId(void *w, unsigned long *pid) { (void)w; *pid = 1234; return 1; }
int GetSystemMetrics(int i) { (void)i; return 0; }

static uint8_t *block_of(uintptr_t rva, unsigned stub_off) {
    uint8_t *s = IMG + rva;
    int32_t r;
    memcpy(&r, s + 1, 4);
    return s + 5 + r - stub_off;
}
unsigned int SendInput(unsigned int n, void *in, int sz) {
    uint8_t *p = in;
    uint32_t flags;
    int32_t dx;
    uint64_t extra;
    (void)n; (void)sz;
    memcpy(&dx, p + 8, 4);
    memcpy(&flags, p + 20, 4);
    memcpy(&extra, p + 32, 8);
    if (g_events < 32) { g_ev_flags[g_events] = flags; g_ev_dx[g_events] = dx; g_ev_extra[g_events] = extra; }
    g_events++;
    if (flags == 2 && g_check_at_down) { /* LEFTDOWN: the game would now process the click */
        uint8_t *blk = block_of(g_rvaA, WR_OFF_STUB_A);
        int slot[2] = {0, 0};
        g_down_flag = *(volatile int *)(blk + WR_OFF_FLAG);
        g_down_a_ret = ((FnA)(IMG + g_rvaA))(0, slot, g_tile_empty, g_tile_empty);
        g_down_slot[0] = slot[0];
        g_down_slot[1] = slot[1];
        /* the game now runs site B (info tile store). Run the REAL stub B with rbp = map
           object and r14d = hovered tile through a fake caller whose return point is ours. */
        {
            uint8_t *blkb = block_of(g_rvaB, WR_OFF_STUB_B);
            void *saved;
            g_fakeb = blkb + 0x800; /* same page as the block, so the rel32 jump reaches it */
            memcpy(g_fakeb, FAKE_B, sizeof FAKE_B);
            saved = *(void **)(blkb + WR_OFF_RETB);
            int32_t rel = (int32_t)((blkb + WR_OFF_STUB_B) - (g_fakeb + 0x10 + 5));
            memcpy(g_fakeb + 0x11, &rel, 4);
            *(void **)(blkb + WR_OFF_RETB) = g_fakeb + 0x17;
            ((FnFakeB)g_fakeb)(g_mapobj, g_tile_empty);
            *(void **)(blkb + WR_OFF_RETB) = saved;
            g_down_info = *(int *)(g_mapobj + 0x2F4B0);
        }
        g_down_k = ((FnK)(IMG + g_rvaK))(g_building);
    }
    return 1;
}

static void host_log(const char *l) { printf("    [log] %s\n", l); }
static int g_hp_calls, g_hp_count, g_hp_ok, g_hp_lens[4];
static char g_hp_name[64];
static int host_patch_group(const char *name, const WR_Patch *p, unsigned int n) {
    unsigned i;
    g_hp_calls++; g_hp_count = (int)n; g_hp_ok = 1;
    strncpy(g_hp_name, name, 63);
    for (i = 0; i < n && i < 4; i++) { g_hp_lens[i] = (int)p[i].length; if (memcmp(p[i].target, p[i].expected, p[i].length)) g_hp_ok = 0; }
    if (!g_hp_ok) return 0;
    for (i = 0; i < n; i++) memcpy(p[i].target, p[i].replacement, p[i].length);
    return 1;
}

/* ------------------------------------------------------------------ stub-only tests (block in RWX memory) */
static const uint8_t FAKE_B[] = {0x55,0x41,0x56,0x48,0x89,0xCD,0x41,0x89,0xD6,0x48,0xC7,0xC0,0x11,0x11,0x00,0x00,0xE9,0x00,0x00,0x00,0x00,0x90,0x90,0x48,0x89,0x45,0x08,0x48,0x89,0x4D,0x10,0x41,0x5E,0x5D,0xC3};
static const uint8_t TRAMP[] = {0x48,0x89,0xC8,0x49,0xBA,0x00,0x00,0x00,0x00,0xEF,0xBE,0xAD,0xDE,0x4D,0x09,0xD0,0x4D,0x09,0xD1,0x31,0xC9,0xFF,0xE0};

static void stub_tests(void) {
    uint8_t *m = mmap(0, 0x3000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t *blk = m, *code = m + 0x1000;
    static unsigned short table[48400];
    uint8_t *mapobj = calloc(1, 0x30000), *db = calloc(1, 0xE270 + 48400 * 8 + 64);
    uint8_t *bld = calloc(1, 64), *other = calloc(1, 64);
    uint8_t *pmapvar = calloc(1, 16), *pdbvar = calloc(1, 16);
    int slot[2], r;
    unsigned i;
    printf("== block stubs (A/B/K) executed directly\n");
    for (i = 0; i < 48400; i++) table[i] = 0xFFFF;
    table[500] = 777; table[501] = 777; table[777] = 777;
    memcpy(blk, WR_BLOCK, WR_BLOCK_SIZE);
    *(void **)(blk + WR_OFF_TABLE) = table;
    *(void **)(pmapvar) = mapobj;
    *(void **)(pdbvar) = db;
    *(void **)(blk + WR_OFF_PMAP) = pmapvar;
    *(void **)(blk + WR_OFF_PDB) = pdbvar;
    *(int16_t *)(bld + 0x1C) = 42;
    *(int16_t *)(other + 0x1C) = -7;
    *(void **)(db + 0xE270 + 777 * 8) = bld;
    *(int *)(mapobj + 0x2F4B0) = 777;
    FnA A = (FnA)(blk + WR_OFF_STUB_A);
    FnK K = (FnK)(blk + WR_OFF_STUB_K);
    FnTramp T;
    memcpy(code + 0x200, TRAMP, sizeof TRAMP);
    T = (FnTramp)(code + 0x200);

    /* A, flag 0: original semantics */
    slot[0] = slot[1] = 0;
    r = A(0, slot, 500, 501);
    CHECK(r == 1 && slot[0] == 501 && slot[1] == 500, "A flag0: new tile -> stored as is, returns 1");
    r = A(0, slot, 500, 9);
    CHECK(r == 0 && slot[0] == 501 && slot[1] == 500, "A flag0: same tile -> returns 0, unchanged");
    /* A, flag 1: mapped through table */
    *(int *)(blk + WR_OFF_FLAG) = 1;
    slot[0] = slot[1] = 0;
    r = A(0, slot, 500, 501);
    CHECK(r == 1 && slot[0] == 777 && slot[1] == 777, "A flag1: tiles mapped to region target");
    r = A(0, slot, 501, 500);
    CHECK(r == 0, "A flag1: mapped tile equal to current -> returns 0");
    slot[0] = slot[1] = 0;
    r = A(0, slot, 600, 48400);
    CHECK(r == 1 && slot[1] == 600 && slot[0] == 48400, "A flag1: no target / out of range -> unchanged");
    slot[0] = slot[1] = 0;
    r = T((void *)A, slot, 500, 501); /* r8/r9 upper 32 bits = garbage */
    CHECK(r == 1 && slot[0] == 777 && slot[1] == 777, "A flag1: garbage in upper register halves ignored");
    *(int *)(blk + WR_OFF_FLAG) = 0;

    /* K */
    CHECK(K(bld) == 42 && K(other) == -7, "K flag0: returns [rcx+1C] sign-extended");
    *(int *)(blk + WR_OFF_FLAG) = 1;
    CHECK(K(bld) == 0, "K flag1: target building -> 0 (no garrison)");
    CHECK(K(other) == -7, "K flag1: other building -> original");
    *(int *)(mapobj + 0x2F4B0) = -1;
    CHECK(K(bld) == 42, "K flag1: info tile -1 -> original");
    *(int *)(mapobj + 0x2F4B0) = 777;
    *(uint32_t *)(blk + WR_OFF_KFILTER) = 1;
    *(void **)(blk + WR_OFF_TEXTLO) = (void *)0x1000;
    *(void **)(blk + WR_OFF_TEXTHI) = (void *)0x2000;
    CHECK(K(bld) == 42, "K filter on, caller outside game text -> original");
    *(void **)(blk + WR_OFF_TEXTLO) = (void *)0;
    *(void **)(blk + WR_OFF_TEXTHI) = (void *)~(uintptr_t)0;
    CHECK(K(bld) == 0, "K filter on, caller inside game text -> 0");
    *(uint32_t *)(blk + WR_OFF_KFILTER) = 0;
    *(void **)(pmapvar) = 0;
    CHECK(K(bld) == 42, "K flag1: no map object -> original");
    *(void **)(pmapvar) = mapobj;
    CHECK(*(uint32_t *)(blk + WR_OFF_KHITS) == 2, "K hit counter = 2");
    *(int *)(blk + WR_OFF_FLAG) = 0;

    /* B: fake game function with the 7-byte store replaced by a jump to stub B */
    {
        uint8_t *f = code;
        uint8_t *frame = calloc(1, 0x30000);
        int32_t rel;
        memcpy(f, FAKE_B, sizeof FAKE_B);
        rel = (int32_t)((blk + WR_OFF_STUB_B) - (f + 0x10 + 5));
        memcpy(f + 0x11, &rel, 4);
        *(void **)(blk + WR_OFF_RETB) = f + 0x17;
        ((FnFakeB)f)(frame, 500);
        CHECK(*(int *)(frame + 0x2F4B0) == 500, "B flag0: stores r14d");
        CHECK(*(uint64_t *)(frame + 8) == 0x1111 && *(uint8_t **)(frame + 16) == frame, "B flag0: rax/rcx preserved, returns to B+7");
        *(int *)(blk + WR_OFF_FLAG) = 1;
        *(uint64_t *)(frame + 8) = 0;
        ((FnFakeB)f)(frame, 500);
        CHECK(*(int *)(frame + 0x2F4B0) == 777, "B flag1: stores region target");
        CHECK(*(uint64_t *)(frame + 8) == 0x1111 && *(uint8_t **)(frame + 16) == frame, "B flag1: rax/rcx preserved");
        ((FnFakeB)f)(frame, 600);
        CHECK(*(int *)(frame + 0x2F4B0) == 600, "B flag1: tile without target stored as is");
        ((FnFakeB)f)(frame, 60000);
        CHECK(*(int *)(frame + 0x2F4B0) == 60000, "B flag1: out-of-range tile stored as is");
        *(void **)(blk + WR_OFF_TABLE) = 0;
        ((FnFakeB)f)(frame, 500);
        CHECK(*(int *)(frame + 0x2F4B0) == 500, "B flag1: no table -> stores r14d");
        *(int *)(blk + WR_OFF_FLAG) = 0;
    }
}

/* ------------------------------------------------------------------ full run against the game image */
static int wait_state(unsigned bits, int ms) {
    while (ms > 0) { if ((WR_GetState() & bits) == bits) return 1; usleep(10000); ms -= 10; }
    return 0;
}
static void press(void) { g_mdown = 1; usleep(60000); g_mdown = 0; usleep(30000); }
static int wait_idle(int ms) {
    usleep(50000);
    while (ms > 0 && (WR_GetState() & WR_STATE_BUSY)) { usleep(10000); ms -= 10; }
    usleep(50000);
    return !(WR_GetState() & WR_STATE_BUSY);
}

int main(int argc, char **argv) {
    FILE *f;
    uintptr_t rvaB, rvaMap, rvaDb, rvaVt;
    uint8_t origA[6], origB[7], origK[5];
    uint8_t *mapobj, *db;
    static unsigned short tbl[48400];
    unsigned i;
    stub_tests();
    if (argc < 8) { printf("(no image given - full test skipped)\n"); goto done; }
    g_rvaA = strtoul(argv[2], 0, 16); rvaB = strtoul(argv[3], 0, 16); g_rvaB = rvaB; g_rvaK = strtoul(argv[4], 0, 16);
    rvaMap = strtoul(argv[5], 0, 16); rvaDb = strtoul(argv[6], 0, 16); rvaVt = strtoul(argv[7], 0, 16);
    printf("== full worker run on %s\n", argv[1]);
    f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); IMGSZ = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    IMG = mmap((void *)0x140000000, IMGSZ, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (IMG != (void *)0x140000000) { printf("cannot map image at 0x140000000\n"); return 2; }
    if (fread(IMG, 1, IMGSZ, f) != IMGSZ) return 2;
    fclose(f);
    memcpy(origA, IMG + g_rvaA, 6); memcpy(origB, IMG + rvaB, 7); memcpy(origK, IMG + g_rvaK, 5);

    /* the table the core will decode: pick an empty tile whose region has another target */
    { /* decode the same embedded table the core uses */
        unsigned k = 0, j;
        for (j = 0; j + 2 < WR_TARGET_RUN_BYTES; j += 3) {
            unsigned n = WR_TARGET_RUNS[j];
            while (n-- && k < 48400) tbl[k++] = (unsigned short)(WR_TARGET_RUNS[j + 1] | WR_TARGET_RUNS[j + 2] << 8);
        }
        CHECK(k == 48400, "embedded table decodes to 48400 tiles");
    }
    for (i = 0; i < 48400; i++) if (tbl[i] != 0xFFFF && tbl[i] != i) { g_tile_empty = (int)i; g_target = tbl[i]; break; }
    printf("  test tile %d -> target %d\n", g_tile_empty, g_target);

    mapobj = calloc(1, 0x30000);
    g_mapobj = mapobj;
    db = calloc(1, 0xE270 + 48400 * 8 + 64);
    g_building = calloc(1, 64);
    *(int16_t *)(g_building + 0x1C) = 123; /* an army is garrisoned in the target */
    *(void **)(db + 0xE270 + (size_t)g_target * 8) = g_building;
    *(int *)(mapobj + 0x2F480) = 0;
    *(int *)(mapobj + 0x2F484) = -1;
    *(int *)(mapobj + 0x2F4B0) = g_tile_empty;
    *(void **)(mapobj + 0x2F6A0) = IMG + rvaVt; /* nothing selected */
    /* game state stack (exe+1B620B0, KO): top = CUserStrategyState (exe+13D63A0) = plain map */
    static void *g_stk[2], *g_userobj[0x100], *g_menuobj[0x100];
    g_userobj[0] = IMG + 0x13D63A0; g_menuobj[0] = IMG + 0x13D63A0 + 0x100;
    g_stk[0] = g_userobj;
    *(uint64_t *)(IMG + 0x1B620B0 + 0x10) = 1;
    *(void **)(IMG + 0x1B620B0 + 0x20) = g_stk;
    *(void **)(IMG + rvaMap) = mapobj;
    *(void **)(IMG + rvaDb) = db;

    {
        WR_Host h = {sizeof h, host_log, 0};
        WR_Config c;
        memset(&c, 0, sizeof c);
        c.size = sizeof c;
        c.normal_mode_only = 1;
        c.hold_ms = 300;
        CHECK(WR_Start(&h, &c) == 1, "WR_Start");
    }
    CHECK(wait_state(WR_STATE_HOOKED, 20000), "worker found all sites and hooked");
    CHECK(WR_GetState() & WR_STATE_FOCUS_CHECK, "CHexFocusModeHex vtable found by RTTI");
    CHECK(IMG[g_rvaA] == 0xE9 && IMG[g_rvaA + 5] == 0x90, "A site = jmp + nop");
    CHECK(IMG[rvaB] == 0xE9 && IMG[rvaB + 5] == 0x90 && IMG[rvaB + 6] == 0x90, "B site = jmp + 2 nop");
    CHECK(IMG[g_rvaK] == 0xE9 && IMG[g_rvaK + 5] == 0xCC, "K site = jmp, int3 padding after it untouched");
    {
        uint8_t *bA = block_of(g_rvaA, WR_OFF_STUB_A), *bB = block_of(rvaB, WR_OFF_STUB_B), *bK = block_of(g_rvaK, WR_OFF_STUB_K);
        CHECK(bA == bB && bB == bK, "all three jumps land in the same block");
        CHECK(*(void **)(bA + WR_OFF_RETB) == IMG + rvaB + 7, "B returns to site+7");
        CHECK(*(void **)(bA + WR_OFF_PMAP) == IMG + rvaMap && *(void **)(bA + WR_OFF_PDB) == IMG + rvaDb, "globals resolved from code");
    }
    /* patched game functions, flag 0 = original behaviour */
    {
        int slot[2] = {0, 0};
        int r = ((FnA)(IMG + g_rvaA))(0, slot, g_tile_empty, g_tile_empty);
        CHECK(r == 1 && slot[1] == g_tile_empty, "game A via patch, idle: original behaviour");
        CHECK(((FnK)(IMG + g_rvaK))(g_building) == 123, "game K via patch, idle: original value");
    }
    /* wheel click on the empty tile */
    g_check_at_down = 1;
    press();
    CHECK(wait_idle(5000), "redirect finished");
    CHECK(g_events == 6, "6 synthetic mouse events");
    CHECK(g_ev_flags[0] == 1 && g_ev_dx[0] == 1 && g_ev_flags[1] == 1 && g_ev_dx[1] == -1, "jiggle +1/-1 first");
    CHECK(g_ev_flags[2] == 2 && g_ev_flags[3] == 4, "then left down / left up");
    CHECK(g_ev_flags[4] == 1 && g_ev_flags[5] == 1, "then jiggle again");
    CHECK(g_ev_extra[2] == 0x57524731u, "events tagged WRG1");
    CHECK(g_down_flag == 1, "flag on while the click happens");
    CHECK(g_down_a_ret == 1 && g_down_slot[0] == g_target && g_down_slot[1] == g_target, "during click: hover tile = region target");
    CHECK(g_down_info == g_target, "during click: info tile (real stub B) = region target");
    CHECK(g_down_k == 0, "during click: target garrison hidden");
    *(int *)(mapobj + 0x2F4B0) = g_tile_empty; /* the game would recompute it after the flag */
    CHECK(*(volatile int *)(block_of(g_rvaA, WR_OFF_STUB_A) + WR_OFF_FLAG) == 0, "flag off afterwards");
    CHECK(WR_GetCounter(0) == 1, "redirect counter = 1");

    /* no redirect when the mouse is over something (army, city, ...) */
    g_events = 0;
    *(int *)(mapobj + 0x2F480) = 8;
    *(int *)(mapobj + 0x2F484) = 10;
    press(); wait_idle(2000);
    CHECK(g_events == 0, "hover kind 8/10 (army): nothing happens");
    *(int *)(mapobj + 0x2F480) = 0;
    *(int *)(mapobj + 0x2F484) = -1;
    /* no redirect while a unit is selected (normal_mode_only) */
    *(void **)(mapobj + 0x2F6A0) = IMG + rvaVt + 0x38;
    usleep(60000); /* the state is sampled before the press */
    press(); wait_idle(2000);
    CHECK(g_events == 0, "not normal focus mode: nothing happens");
    *(void **)(mapobj + 0x2F6A0) = IMG + rvaVt;
    usleep(60000);
    /* the game leaves the selection on the same press (right click = cancel): use the state before the press */
    g_events = 0;
    *(void **)(mapobj + 0x2F6A0) = IMG + rvaVt + 0x38;
    usleep(60000);
    *(void **)(mapobj + 0x2F6A0) = IMG + rvaVt; /* changes back at the same moment as the press */
    press(); wait_idle(2000);
    CHECK(g_events == 0, "selection cancelled by the same press: nothing happens");
    usleep(60000);
    /* a unit command menu (another state on top of the plain map state) */
    g_events = 0;
    g_stk[1] = g_menuobj; *(uint64_t *)(IMG + 0x1B620B0 + 0x10) = 2;
    usleep(60000);
    press(); wait_idle(2000);
    CHECK(g_events == 0, "another game state on top (unit menu): nothing happens");
    g_stk[1] = g_menuobj; *(uint64_t *)(IMG + 0x1B620B0 + 0x10) = 2;
    usleep(60000);
    *(uint64_t *)(IMG + 0x1B620B0 + 0x10) = 1; /* the same right click closes the menu */
    press(); wait_idle(2000);
    CHECK(g_events == 0, "menu closed by the same press: nothing happens");
    usleep(60000);
    /* left command menu open (layout +260 = entry, +270 = 1) */
    {
        static uint8_t g_layout[0x800];
        *(void **)g_layout = IMG + 0x13D66E8;
        *(int *)(g_layout + 0x260) = -1;
        g_userobj[0x20] = g_layout;   /* the state keeps a pointer to its layout */
        usleep(60000);
        g_events = 0;
        press(); wait_idle(3000);
        CHECK(g_events == 6, "plain map with the layout, menu closed: redirect");
        *(int *)(g_layout + 0x260) = 2; *(int *)(g_layout + 0x270) = 1;
        usleep(60000);
        g_events = 0;
        *(int *)(g_layout + 0x260) = -1; *(int *)(g_layout + 0x270) = 0; /* the same right click closes it */
        press(); wait_idle(2000);
        CHECK(g_events == 0, "left command menu open before the press: nothing happens");
        usleep(60000);
    }
    /* clicking the target tile itself: nothing to redirect */
    *(int *)(mapobj + 0x2F4B0) = g_target;
    press(); wait_idle(2000);
    CHECK(g_events == 0, "info tile already the target: nothing happens");
    *(int *)(mapobj + 0x2F4B0) = g_tile_empty;
    /* WR_Trigger path */
    g_check_at_down = 0;
    WR_Trigger(); usleep(100000); wait_idle(3000);
    CHECK(g_events == 6, "WR_Trigger runs one redirect");

    WR_Stop();
    CHECK(memcmp(IMG + g_rvaA, origA, 6) == 0 && memcmp(IMG + rvaB, origB, 7) == 0 && memcmp(IMG + g_rvaK, origK, 5) == 0,
          "WR_Stop restored the original bytes");

    /* another mod already patched site K -> WheelRegion must refuse and touch nothing */
    printf("== conflict: K already patched by someone else\n");
    {
        uint8_t saveK[5];
        WR_Host h = {sizeof h, host_log, 0};
        memcpy(saveK, IMG + g_rvaK, 5);
        IMG[g_rvaK] = 0xE9; IMG[g_rvaK + 1] = 0x11; IMG[g_rvaK + 2] = 0x22; IMG[g_rvaK + 3] = 0x33; IMG[g_rvaK + 4] = 0x00;
        CHECK(WR_Start(&h, 0) == 1, "WR_Start");
        usleep(1500000);
        CHECK(WR_GetState() & WR_STATE_FAIL_PATTERN, "K caller resolution fails (target is no longer the getter)");
        CHECK(memcmp(IMG + g_rvaA, origA, 6) == 0 && memcmp(IMG + rvaB, origB, 7) == 0, "A and B left untouched");
        WR_Stop();
        memcpy(IMG + g_rvaK, saveK, 5);
    }
    printf("== conflict: B already patched by someone else\n");
    {
        WR_Host h = {sizeof h, host_log, 0};
        IMG[rvaB] = 0xE9;
        CHECK(WR_Start(&h, 0) == 1, "WR_Start");
        usleep(1500000);
        CHECK(WR_GetState() & WR_STATE_FAIL_PATTERN, "B pattern not found -> feature off");
        CHECK(memcmp(IMG + g_rvaA, origA, 6) == 0 && memcmp(IMG + g_rvaK, origK, 5) == 0, "A and K left untouched");
        WR_Stop();
        memcpy(IMG + rvaB, origB, 7);
    }
    /* host-managed patching (ModCore InstallHooks style) */
    printf("== host patch_group (ModCore style)\n");
    {
        WR_Host h = {sizeof h, host_log, host_patch_group};
        CHECK(WR_Start(&h, 0) == 1, "WR_Start");
        CHECK(wait_state(WR_STATE_HOOKED, 20000), "hooked through the host");
        CHECK(g_hp_calls == 1 && g_hp_count == 3, "one group with 3 patches");
        CHECK(strcmp(g_hp_name, "SAN14CE.WheelRegion") == 0, "group id SAN14CE.WheelRegion");
        CHECK(g_hp_lens[0] == 6 && g_hp_lens[1] == 7 && g_hp_lens[2] == 5, "patch lengths 6/7/5");
        CHECK(g_hp_ok, "expected bytes matched the game for every patch");
        CHECK(IMG[g_rvaA] == 0xE9 && IMG[rvaB] == 0xE9 && IMG[g_rvaK] == 0xE9, "host wrote the jumps");
        WR_Stop();
        CHECK(IMG[g_rvaA] == 0xE9, "host-owned patches are not removed by WR_Stop (host decides)");
        memcpy(IMG + g_rvaA, origA, 6); memcpy(IMG + rvaB, origB, 7); memcpy(IMG + g_rvaK, origK, 5);
    }
done:
    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
