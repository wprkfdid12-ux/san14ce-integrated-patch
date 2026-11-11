/*
 * RTK14 WheelRegion - core. See wheel_region.h and README_KO.md.
 *
 * How it works (same behaviour as the tested Cheat Engine table "test18"):
 *   - a worker thread waits until Steam has unpacked the game code, then finds 3 code sites
 *     and 2 globals by byte patterns (no fixed addresses; works for every CE language exe),
 *   - patches the 3 sites to jump into a small machine-code block (wr_block.S),
 *   - polls the trigger button. On a wheel click over an EMPTY tile (hover kind 0 / -1)
 *     whose region has a target tile, it sets the block flag and sends: 1px mouse jiggle,
 *     left button down/up, waits hold_ms, clears the flag, jiggles again.
 *   - while the flag is set the game's hover tile (A) and info target tile (B) become the
 *     region's target, so the synthetic left click selects that village/port/city/gate,
 *     and the garrison getter (K) reports "no army" for that target so an army standing
 *     on it does not take the click.
 */
#include <stddef.h>
#include <stdint.h>
#include "wheel_region.h"
#include "wr_block.h"
#ifndef WR_NO_EMBEDDED_TARGETS
#include "wr_targets.h"
#endif

/* ------------------------------------------------------------------ Win32 declarations */
#ifdef WR_USE_WINDOWS_H
#include <windows.h>
#define WR_WINAPI WINAPI
#else
#ifndef WR_IMPORT
#define WR_IMPORT __declspec(dllimport)
#endif
#ifndef WR_WINAPI
#define WR_WINAPI __stdcall
#endif
WR_IMPORT void *WR_WINAPI GetModuleHandleW(const wchar_t *);
WR_IMPORT void *WR_WINAPI VirtualAlloc(void *, size_t, unsigned long, unsigned long);
WR_IMPORT int WR_WINAPI VirtualFree(void *, size_t, unsigned long);
WR_IMPORT int WR_WINAPI VirtualProtect(void *, size_t, unsigned long, unsigned long *);
WR_IMPORT size_t WR_WINAPI VirtualQuery(const void *, void *, size_t);
WR_IMPORT int WR_WINAPI FlushInstructionCache(void *, const void *, size_t);
WR_IMPORT void *WR_WINAPI GetCurrentProcess(void);
WR_IMPORT unsigned long WR_WINAPI GetCurrentProcessId(void);
WR_IMPORT void *WR_WINAPI CreateThread(void *, size_t, unsigned long(WR_WINAPI *)(void *), void *, unsigned long, unsigned long *);
WR_IMPORT int WR_WINAPI CloseHandle(void *);
WR_IMPORT void WR_WINAPI Sleep(unsigned long);
WR_IMPORT short WR_WINAPI GetAsyncKeyState(int);
WR_IMPORT unsigned int WR_WINAPI SendInput(unsigned int, void *, int);
WR_IMPORT void *WR_WINAPI GetForegroundWindow(void);
WR_IMPORT unsigned long WR_WINAPI GetWindowThreadProcessId(void *, unsigned long *);
WR_IMPORT int WR_WINAPI GetSystemMetrics(int);
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#define WR_CAS64(p, o, n) _InterlockedCompareExchange64((volatile long long *)(p), (n), (o))
#define WR_CAS32(p, o, n) _InterlockedCompareExchange((volatile long *)(p), (n), (o))
#else
#define WR_CAS64(p, o, n) __sync_val_compare_and_swap((volatile long long *)(p), (o), (n))
#define WR_CAS32(p, o, n) __sync_val_compare_and_swap((volatile long *)(p), (o), (n))
#endif

typedef unsigned char U8;
typedef uint32_t U32;
typedef uintptr_t UPTR;

#define WR_TILES 48400u
#define MAP_KIND0 0x2F480u
#define MAP_KIND1 0x2F484u
#define MAP_INFO 0x2F4B0u
#define MAP_FOCUS 0x2F6A0u
#define GAME_MARK 0x57524731u /* "WRG1" in dwExtraInfo of our synthetic input */

/* ------------------------------------------------------------------ patterns
   (verified unique in SAN14CE_KO/CE/SC/TC.exe of the 2026-10 Steam build) */
typedef struct { const U8 *bytes; const char *mask; U32 len; } Pat;
#define PAT(name, ...) static const U8 name##_b[] = {__VA_ARGS__}
/* A: hover tile setter. if(r8d==[rdx+4])return 0;[rdx]=r9d;[rdx+4]=r8d;return 1  -> patch 6 bytes at +0 */
PAT(pA, 0x44,0x3B,0x42,0x04,0x75,0x03,0x33,0xC0,0xC3,0x44,0x89,0x0A,0xB8,0x01,0x00,0x00,0x00,0x44,0x89,0x42,0x04,0xC3);
/* B: info target store "mov [rbp+2F4B0],r14d" at +6 (7 bytes), then call 188D60-like */
PAT(pB, 0x44,0x8B,0xF0,0x48,0x8B,0xCD,0x44,0x89,0xB5,0xB0,0xF4,0x02,0x00,0xE8);
/* K caller 1 (info tile chooser): ...mov rcx,rbx / call K / dec eax / cmp eax,1F3 ; call at +15 */
PAT(pK1, 0x0F,0xB6,0x43,0x10,0x83,0xE8,0x0F,0x83,0xF8,0x03,0x76,0x00,0x48,0x8B,0xCB,0xE8,0,0,0,0,0xFF,0xC8,0x3D,0xF3,0x01,0x00,0x00);
/* K caller 2 (hover target calc): ...mov rcx,[rax+E270] / call K / test eax,eax ; call at +17 */
PAT(pK2, 0x48,0x8B,0x8C,0xC8,0x70,0xE2,0x00,0x00,0xEB,0x07,0x48,0x8B,0x88,0x70,0xE2,0x00,0x00,0xE8,0,0,0,0,0x85,0xC0,0x74,0x00,0x3D,0xF4,0x01,0x00,0x00);
/* globals: mov rax,[map]; ...; movsxd rcx,[rax+2F4B0]; ...; mov rax,[db]; mov rbx,[rax+rcx*8+E270] */
PAT(pG, 0x48,0x8B,0x05,0,0,0,0,0x48,0x89,0x5C,0x24,0x20,0x48,0x63,0x88,0xB0,0xF4,0x02,0x00,0x81,0xF9,0x0F,0xBD,0x00,0x00,0x77,0x00,0x48,0x8B,0x05,0,0,0,0,0x48,0x8B,0x9C,0xC8,0x70,0xE2,0x00,0x00);
/* game state stack: in the "is this state on top" check, "call GetStateMgr / cmp qword [rax+10],0 / jne / xor eax,eax ... / mov rdx,[rax+10] / mov rcx,[rax+20]" */
PAT(pSM, 0xE8,0,0,0,0,0x48,0x83,0x78,0x10,0x00,0x75,0x08,0x33,0xC0,0x48,0x83,0xC4,0x20,0x5B,0xC3,0x48,0x8B,0x50,0x10,0x48,0x8B,0x48,0x20);
/* inside GetStateMgr: "lea rax,[rip+mgr] / add rsp,30 / pop rbx / ret" */
PAT(pSM2, 0x48,0x8D,0x05,0,0,0,0,0x48,0x83,0xC4,0x30,0x5B,0xC3);
static const Pat PSM = {pSM_b, "x????xxxxxxxxxxxxxxxxxxxxxxx", sizeof pSM_b};
static const Pat PSM2 = {pSM2_b, "xxx????xxxxxx", sizeof pSM2_b};
typedef char psm_len_check[(sizeof("x????xxxxxxxxxxxxxxxxxxxxxxx") - 1 == sizeof pSM_b && sizeof("xxx????xxxxxx") - 1 == sizeof pSM2_b) ? 1 : -1];
static const Pat PA = {pA_b, "xxxxxxxxxxxxxxxxxxxxxx", sizeof pA_b};
static const Pat PB = {pB_b, "xxxxxxxxxxxxxx", sizeof pB_b};
static const Pat PK1 = {pK1_b, "xxxxxxxxxxx?xxxx????xxxxxxx", sizeof pK1_b};
static const Pat PK2 = {pK2_b, "xxxxxxxxxxxxxxxxxx????xxx?xxxxx", sizeof pK2_b};
static const Pat PG = {pG_b, "xxx????xxxxxxxxxxxxxxxxxxx?xxx????xxxxxxxx", sizeof pG_b};

static const U8 ORIG_A[6] = {0x44, 0x3B, 0x42, 0x04, 0x75, 0x03};
static const U8 ORIG_B[7] = {0x44, 0x89, 0xB5, 0xB0, 0xF4, 0x02, 0x00};
static const U8 ORIG_K[5] = {0x0F, 0xBF, 0x41, 0x1C, 0xC3};

/* ------------------------------------------------------------------ state */
static struct {
    WR_Host host;
    WR_Config cfg;
    volatile long started, stop, trigger_req, worker_done, paused;
    volatile unsigned int state;
    unsigned int redirects;
    U8 *game, *text_lo, *text_hi;
    size_t image_size;
    U8 *block;
    unsigned short *table;
    int table_from_host;
    U8 *site_a, *site_b, *site_k;
    U8 **pmap, **pdb;
    void *vt_focus;
    void *vt_user;               /* CUserStrategyState: the plain map state */
    U8 *state_mgr;               /* game state stack (count +10, array +20) */
    U32 press_logs;
    void *vt_layout;             /* CUserStrategyLayout: holds the left command menu */
    U32 layout_off;              /* where the state keeps its layout pointer (found once) */
    WR_Patch patches[3];
    int self_patched;
} W;

/* ------------------------------------------------------------------ small helpers (no CRT) */
static void bcopy_(U8 *d, const U8 *s, size_t n) { while (n--) *d++ = *s++; }
static void bfill_(U8 *d, U8 v, size_t n) { while (n--) *d++ = v; }
static int bsame_(const U8 *a, const U8 *b, size_t n) { while (n--) if (*a++ != *b++) return 0; return 1; }

typedef struct { char s[256]; U32 n; } Line;
static void lput(Line *l, const char *t) { while (*t && l->n < sizeof l->s - 1) l->s[l->n++] = *t++; l->s[l->n] = 0; }
static void lhex(Line *l, uint64_t v) {
    char b[19]; int i = 18; b[i] = 0;
    do { b[--i] = "0123456789ABCDEF"[v & 15]; v >>= 4; } while (v && i > 2);
    b[--i] = 'x'; b[--i] = '0'; lput(l, b + i);
}
static void ldec(Line *l, uint64_t v) {
    char b[24]; int i = 23; b[i] = 0;
    do { b[--i] = (char)('0' + v % 10); v /= 10; } while (v && i);
    lput(l, b + i);
}
static void wr_line_out(Line *l) { if (W.host.log) W.host.log(l->s); }
static void wr_log(const char *t) { Line l; l.n = 0; l.s[0] = 0; lput(&l, "WheelRegion: "); lput(&l, t); wr_line_out(&l); }
static void log_rva(const char *t, const void *p) {
    Line l; l.n = 0; l.s[0] = 0; lput(&l, "WheelRegion: "); lput(&l, t);
    lput(&l, " exe+"); lhex(&l, (uint64_t)((const U8 *)p - W.game)); wr_line_out(&l);
}

typedef struct {
    void *base, *alloc_base; unsigned long alloc_protect; unsigned short partition, pad0;
    size_t region_size; unsigned long state, protect, type, pad1;
} Mbi;

static int readable(const void *p, size_t n) {
    Mbi m;
    const U8 *a = (const U8 *)p, *e = a + n;
    while (a < e) {
        if (!VirtualQuery(a, &m, sizeof m)) return 0;
        if (m.state != 0x1000 /*MEM_COMMIT*/ || (m.protect & 0x101 /*NOACCESS|GUARD*/) || !m.protect) return 0;
        a = (const U8 *)m.base + m.region_size;
    }
    return 1;
}

/* count matches of a pattern in [lo,hi); *first = first match. stops at 2. */
static int scan(const U8 *lo, const U8 *hi, const Pat *p, U8 **first) {
    int n = 0;
    const U8 *e, *q;
    U32 i;
    if (hi < lo + p->len) return 0;
    e = hi - p->len;
    for (q = lo; q <= e; q++) {
        if (*q != p->bytes[0]) continue;
        for (i = 1; i < p->len; i++)
            if (p->mask[i] == 'x' && q[i] != p->bytes[i]) break;
        if (i == p->len) {
            if (!n && first) *first = (U8 *)q;
            if (++n == 2) break;
        }
    }
    return n;
}
static U8 *rel_target(U8 *at /* first byte of rel32 */) {
    int32_t r = (int32_t)((U32)at[0] | (U32)at[1] << 8 | (U32)at[2] << 16 | (U32)at[3] << 24);
    return at + 4 + r;
}

/* ------------------------------------------------------------------ setup steps */
static int find_text(void) {
    U8 *g = (U8 *)GetModuleHandleW(0);
    U32 pe, nsec, i, optsz;
    if (!g) return 0;
    pe = *(U32 *)(g + 0x3C);
    if (*(U32 *)(g + pe) != 0x4550) return 0;
    nsec = *(unsigned short *)(g + pe + 6);
    optsz = *(unsigned short *)(g + pe + 20);
    W.image_size = *(U32 *)(g + pe + 24 + 56);
    W.game = g;
    for (i = 0; i < nsec; i++) {
        U8 *s = g + pe + 24 + optsz + 40 * i;
        if (s[0] == '.' && s[1] == 't' && s[2] == 'e' && s[3] == 'x' && s[4] == 't' && !s[5]) {
            W.text_lo = g + *(U32 *)(s + 12);
            W.text_hi = W.text_lo + *(U32 *)(s + 8);
            return 1;
        }
    }
    return 0;
}

static int resolve_sites(void) {
    U8 *a = 0, *b = 0, *k1 = 0, *k2 = 0, *gl = 0, *t1, *t2;
    int na = scan(W.text_lo, W.text_hi, &PA, &a);
    int nb = scan(W.text_lo, W.text_hi, &PB, &b);
    int nk1 = scan(W.text_lo, W.text_hi, &PK1, &k1);
    int nk2 = scan(W.text_lo, W.text_hi, &PK2, &k2);
    int ng = scan(W.text_lo, W.text_hi, &PG, &gl);
    Line l; l.n = 0; l.s[0] = 0;
    lput(&l, "WheelRegion: pattern matches A="); ldec(&l, (U32)na); lput(&l, " B="); ldec(&l, (U32)nb);
    lput(&l, " K1="); ldec(&l, (U32)nk1); lput(&l, " K2="); ldec(&l, (U32)nk2); lput(&l, " G="); ldec(&l, (U32)ng);
    wr_line_out(&l);
    if (na != 1 || nb != 1 || nk1 != 1 || nk2 != 1 || ng != 1) return 0;
    t1 = rel_target(k1 + 16);
    t2 = rel_target(k2 + 18);
    if (t1 != t2 || t1 < W.text_lo || t1 + 16 > W.text_hi || !bsame_(t1, ORIG_K, 5)) { wr_log("K callers disagree"); return 0; }
    W.site_a = a;
    W.site_b = b + 6;
    W.site_k = t1;
    W.pmap = (U8 **)rel_target(gl + 3);
    W.pdb = (U8 **)rel_target(gl + 30);
    if ((U8 *)W.pmap < W.game || (U8 *)W.pmap >= W.game + W.image_size ||
        (U8 *)W.pdb < W.game || (U8 *)W.pdb >= W.game + W.image_size) { wr_log("globals out of image"); return 0; }
    log_rva("A (hover tile)", W.site_a);
    log_rva("B (info tile)", W.site_b);
    log_rva("K (garrison)", W.site_k);
    log_rva("map global", W.pmap);
    log_rva("data root global", W.pdb);
    return 1;
}

/* RTTI: vtable of CHexFocusModeHex (map+2F6A0 points to it when nothing is selected) */
static void *find_vtable(const char *name /* ".?AVxxx@@" */) {
    U8 *lo = W.game, *hi = W.game + W.image_size, *p, *td = 0;
    U32 nlen = 0, td_rva, col_rva = 0;
    while (name[nlen]) nlen++;
    if (!readable(lo, (size_t)(hi - lo))) return 0;
    for (p = lo + 0x10; p + nlen + 1 < hi; p++)
        if (p[0] == '.' && p[1] == '?' && bsame_(p, (const U8 *)name, nlen) && p[nlen] == 0) { td = p - 0x10; break; }
    if (!td) return 0;
    td_rva = (U32)(td - W.game);
    for (p = lo; p + 24 <= hi; p += 4) {
        U32 *c = (U32 *)p;
        if (c[0] == 1 && c[1] == 0 && c[3] == td_rva && c[5] == (U32)(p - W.game)) { col_rva = (U32)(p - W.game); break; }
    }
    if (!col_rva) return 0;
    for (p = lo; p + 8 <= hi; p += 8)
        if (*(uint64_t *)p == (uint64_t)(UPTR)(W.game + col_rva)) return p + 8;
    return 0;
}

static int build_table(void) {
    if (W.table) return 1; /* host supplied */
#ifndef WR_NO_EMBEDDED_TARGETS
    {
        unsigned short *t = (unsigned short *)VirtualAlloc(0, WR_TILES * 2, 0x3000, 0x04);
        U32 i, k = 0;
        if (!t) return 0;
        for (i = 0; i + 2 < WR_TARGET_RUN_BYTES; i += 3) {
            U32 n = WR_TARGET_RUNS[i];
            unsigned short v = (unsigned short)(WR_TARGET_RUNS[i + 1] | WR_TARGET_RUNS[i + 2] << 8);
            while (n-- && k < WR_TILES) t[k++] = v;
        }
        if (k != WR_TILES) { VirtualFree(t, 0, 0x8000); return 0; }
        W.table = t;
        return 1;
    }
#else
    return 0;
#endif
}

static U8 *alloc_near(U8 *at) {
    UPTR base = (UPTR)at & ~(UPTR)0xFFFF, off;
    for (off = 0x10000; off < 0x7FF00000; off += 0x10000) {
        U8 *b = (U8 *)VirtualAlloc((void *)(base + off), 0x1000, 0x3000, 0x40);
        if (b) return b;
        if (base > off && (b = (U8 *)VirtualAlloc((void *)(base - off), 0x1000, 0x3000, 0x40))) return b;
    }
    return 0;
}
static int near_ok(U8 *from, U8 *to) {
    intptr_t d = to - (from + 5);
    return d >= -(intptr_t)0x7FFFFFF0 && d <= (intptr_t)0x7FFFFFF0;
}

static void make_patch(WR_Patch *p, U8 *site, const U8 *orig, U32 len, U8 *dest) {
    int32_t d = (int32_t)(dest - (site + 5));
    bfill_((U8 *)p, 0, sizeof *p);
    p->target = site;
    p->length = len;
    bcopy_(p->expected, orig, len);
    bfill_(p->replacement, 0x90, len);
    p->replacement[0] = 0xE9;
    bcopy_(p->replacement + 1, (const U8 *)&d, 4);
}

/* write bytes into code; one 8-byte CAS when the bytes sit in one aligned qword */
static void put_code(U8 *at, const U8 *bytes, U32 n) {
    UPTR a = (UPTR)at, q = a & ~(UPTR)7;
    if (((a + n - 1) & ~(UPTR)7) == q) {
        volatile long long *pq = (volatile long long *)q;
        long long old, nw;
        do {
            old = *pq;
            nw = old;
            bcopy_((U8 *)&nw + (a - q), bytes, n);
        } while (WR_CAS64(pq, old, nw) != old);
    } else {
        U32 i;
        for (i = n; i-- > 1;) ((volatile U8 *)at)[i] = bytes[i]; /* tail first, opcode last */
        ((volatile U8 *)at)[0] = bytes[0];
    }
}

static int self_install(WR_Patch *ps, U32 n, int restore) {
    unsigned long prot[3];
    U32 i, ok = 1;
    for (i = 0; i < n; i++) {
        const U8 *want = restore ? ps[i].replacement : ps[i].expected;
        if (!VirtualProtect(ps[i].target, ps[i].length, 0x40, &prot[i])) { ok = 0; n = i; break; }
        if (!bsame_(ps[i].target, want, ps[i].length)) ok = 0;
    }
    if (ok)
        for (i = 0; i < n; i++) {
            put_code(ps[i].target, restore ? ps[i].expected : ps[i].replacement, ps[i].length);
            FlushInstructionCache(GetCurrentProcess(), ps[i].target, ps[i].length);
        }
    for (i = 0; i < n; i++) { unsigned long u; VirtualProtect(ps[i].target, ps[i].length, prot[i], &u); }
    return (int)ok;
}

static int install(void) {
    U8 *blk = W.block;
    if (!near_ok(W.site_a, blk + WR_OFF_STUB_A) || !near_ok(W.site_b, blk + WR_OFF_STUB_B) || !near_ok(W.site_k, blk + WR_OFF_STUB_K))
        return 0;
    make_patch(&W.patches[0], W.site_a, ORIG_A, 6, blk + WR_OFF_STUB_A);
    make_patch(&W.patches[1], W.site_b, ORIG_B, 7, blk + WR_OFF_STUB_B);
    make_patch(&W.patches[2], W.site_k, ORIG_K, 5, blk + WR_OFF_STUB_K);
    if (!bsame_(W.site_a, ORIG_A, 6) || !bsame_(W.site_b, ORIG_B, 7) || !bsame_(W.site_k, ORIG_K, 5)) {
        wr_log("a site is already patched (another mod?) - not installed");
        return 0;
    }
    if (W.host.patch_group) {
        if (!W.host.patch_group("SAN14CE.WheelRegion", W.patches, 3)) { wr_log("host refused the patch group"); return 0; }
        W.self_patched = 0;
    } else {
        if (!self_install(W.patches, 3, 0)) { wr_log("patch write failed"); return 0; }
        W.self_patched = 1;
    }
    return 1;
}

/* ------------------------------------------------------------------ input */
typedef struct {
    U32 type, pad;
    int32_t dx, dy;
    U32 mouseData, dwFlags, time, pad2;
    uint64_t dwExtraInfo;
} MouseInput; /* == INPUT (40 bytes on x64) */

static void send_mouse(int dx, U32 flags) {
    MouseInput in;
    bfill_((U8 *)&in, 0, sizeof in);
    in.type = 0; /* INPUT_MOUSE */
    in.dx = dx;
    in.dwFlags = flags;
    in.dwExtraInfo = GAME_MARK;
    SendInput(1, &in, (int)sizeof in);
}
static int game_in_front(void) {
    unsigned long pid = 0;
    void *w = GetForegroundWindow();
    if (!w) return 0;
    GetWindowThreadProcessId(w, &pid);
    return pid == GetCurrentProcessId();
}

static volatile U32 *block_flag(void) { return (volatile U32 *)(W.block + WR_OFF_FLAG); }

/* top of the game's state stack (0 if unknown) */
static void *top_state(void) {
    U8 *m = W.state_mgr, *arr;
    uint64_t n;
    if (!m || !readable(m + 0x10, 0x18)) return 0;
    n = *(volatile uint64_t *)(m + 0x10);
    arr = *(U8 *volatile *)(m + 0x20);
    if (!n || n > 256 || !arr || !readable(arr + (n - 1) * 8, 8)) return 0;
    return *(void *volatile *)(arr + (n - 1) * 8);
}
static void *vtable_of(void *o) { return (o && readable(o, 8)) ? *(void **)o : 0; }

/* is the map in the plain state (nothing selected, no target / region selection, no menu)?
   focus mode = CHexFocusModeHex and the top game state = CUserStrategyState */
static int focus_normal(void) {
    U8 *map = *W.pmap;
    if (W.vt_focus) {
        if (!map || !readable(map + MAP_FOCUS, 8)) return 0;
        if (*(void *volatile *)(map + MAP_FOCUS) != W.vt_focus) return 0;
    }
    if (W.vt_user && W.state_mgr) {
        U8 *st = (U8 *)top_state(), *ly = 0;
        if (vtable_of(st) != W.vt_user) return 0;
        /* left command menu (인재/계략/...) open? It belongs to the plain map state, so check its layout:
           +260 = opened main entry (-1 none), +270 = sub menu shown (seen in game, 2026-10-13) */
        if (W.vt_layout) {
            if (W.layout_off && readable(st + W.layout_off, 8)) {
                ly = *(U8 **)(st + W.layout_off);
                if (vtable_of(ly) != W.vt_layout) ly = 0;
            }
            if (!ly && readable(st, 0x800)) {
                U32 o;
                for (o = 8; o < 0x800; o += 8) {
                    U8 *q = *(U8 **)(st + o);
                    if (q && vtable_of(q) == W.vt_layout) { ly = q; W.layout_off = o; break; }
                }
            }
            if (ly && readable(ly + 0x260, 0x18)) {
                if (*(volatile int *)(ly + 0x260) != -1 || *(volatile int *)(ly + 0x270) != 0) return 0;
            }
        }
    }
    return 1;
}
/* "the top state is CXxx" for the log (RTTI name from the vtable) */
static void log_top_state(int ok_before) {
    U8 *vt = (U8 *)vtable_of(top_state()), *col;
    Line l;
    l.n = 0; l.s[0] = 0;
    lput(&l, "WheelRegion: press, state before = ");
    lput(&l, ok_before ? "plain map" : "other");
    lput(&l, ", top state now = ");
    if (vt && readable(vt - 8, 8) && (col = *(U8 **)(vt - 8)) >= W.game && col + 16 <= W.game + W.image_size) {
        U32 td = *(U32 *)(col + 12);
        const char *nm = (const char *)(W.game + td + 0x10);
        if (td + 0x30u < W.image_size && readable(nm, 64)) { char b[64]; int i; for (i = 0; i < 63 && nm[i]; i++) b[i] = nm[i]; b[i] = 0; lput(&l, b); }
    } else lput(&l, "?");
    wr_line_out(&l);
}

/* focus_before: focus_normal() sampled before the button went down. The game reacts to the same
   press (a right click cancels a target / region selection), so the state at fire time is not
   reliable; the state just before the press is. */
static void redirect_once(int focus_before) {
    U8 *map;
    int k0, k1, tile;
    unsigned short v;
    int swapped = GetSystemMetrics(23 /*SM_SWAPBUTTON*/) != 0;
    U32 down = swapped ? 0x0008u : 0x0002u, up = swapped ? 0x0010u : 0x0004u;
    if (!game_in_front()) return;
    map = *W.pmap;
    if (!map || !readable(map + MAP_KIND0, MAP_FOCUS + 8 - MAP_KIND0)) return;
    k0 = *(volatile int *)(map + MAP_KIND0);
    k1 = *(volatile int *)(map + MAP_KIND1);
    tile = *(volatile int *)(map + MAP_INFO);
    if (k0 != 0 || k1 != -1) return;                 /* only an empty tile under the mouse */
    if (tile < 0 || (U32)tile >= WR_TILES) return;
    v = W.table[tile];
    if (v == 0xFFFF || v == (unsigned short)tile) return;
    if (W.cfg.normal_mode_only && W.vt_focus && !focus_before) return;

    W.state |= WR_STATE_BUSY;
    W.redirects++;
    *block_flag() = 1;
    send_mouse(1, 0x0001);  /* MOVE: let the game recompute the hover */
    Sleep(30);
    send_mouse(-1, 0x0001);
    Sleep(60);
    send_mouse(0, down);
    Sleep(40);
    send_mouse(0, up);
    Sleep((unsigned long)W.cfg.hold_ms);
    *block_flag() = 0;
    send_mouse(1, 0x0001);
    Sleep(30);
    send_mouse(-1, 0x0001);
    W.state &= ~WR_STATE_BUSY;
}

static unsigned long WR_WINAPI worker(void *unused) {
    int waited = 0, prev = 0;
    (void)unused;
    if (!find_text()) { W.state |= WR_STATE_FAIL_PATTERN; wr_log("no .text section"); goto out; }
    /* wait for Steam to unpack the code: the A pattern appears when it is done */
    for (;;) {
        if (W.stop) goto out;
        if (scan(W.text_lo, W.text_hi, &PA, 0) >= 1) break;
        if (waited >= W.cfg.wait_seconds * 1000) { W.state |= WR_STATE_FAIL_TIMEOUT; wr_log("game code not found (timeout)"); goto out; }
        Sleep(500);
        waited += 500;
    }
    if (!resolve_sites()) { W.state |= WR_STATE_FAIL_PATTERN; wr_log("unsupported game build - feature off, game untouched"); goto out; }
    W.state |= WR_STATE_FOUND;
    if (!build_table()) { W.state |= WR_STATE_FAIL_TABLE; wr_log("target table missing"); goto out; }
    if (W.cfg.normal_mode_only) {
        W.vt_focus = find_vtable(".?AVCHexFocusModeHex@@");
        if (W.vt_focus) { W.state |= WR_STATE_FOCUS_CHECK; log_rva("CHexFocusModeHex vtable", W.vt_focus); }
        else wr_log("CHexFocusModeHex vtable not found - normal_mode_only ignored");
        {
            U8 *sm = 0, *f, *q = 0;
            if (scan(W.text_lo, W.text_hi, &PSM, &sm) == 1) {
                f = sm + 5 + *(int32_t *)(sm + 1);
                if (f >= W.text_lo && f < W.text_hi && scan(f, f + 0x90 < W.text_hi ? f + 0x90 : W.text_hi, &PSM2, &q) == 1)
                    W.state_mgr = q + 7 + *(int32_t *)(q + 3);
            }
            W.vt_user = find_vtable(".?AVCUserStrategyState@@");
            W.vt_layout = find_vtable(".?AVCUserStrategyLayout@@");
            if (W.state_mgr && W.vt_user) { log_rva("state stack", W.state_mgr); log_rva("CUserStrategyState vtable", W.vt_user); }
            else { W.state_mgr = 0; W.vt_user = 0; wr_log("state stack not found - plain map check uses the focus mode only"); }
        }
    }
    W.block = alloc_near(W.site_a);
    if (!W.block) { W.state |= WR_STATE_FAIL_MEMORY; wr_log("no memory near the game image"); goto out; }
    bcopy_(W.block, WR_BLOCK, WR_BLOCK_SIZE);
    *(U32 *)(W.block + WR_OFF_KFILTER) = W.cfg.game_callers_only ? 1u : 0u;
    *(void **)(W.block + WR_OFF_TABLE) = W.table;
    *(void **)(W.block + WR_OFF_PMAP) = W.pmap;
    *(void **)(W.block + WR_OFF_PDB) = W.pdb;
    *(void **)(W.block + WR_OFF_TEXTLO) = W.text_lo;
    *(void **)(W.block + WR_OFF_TEXTHI) = W.text_hi;
    *(void **)(W.block + WR_OFF_RETB) = W.site_b + 7;
    FlushInstructionCache(GetCurrentProcess(), W.block, WR_BLOCK_SIZE);
    if (!install()) { W.state |= WR_STATE_FAIL_PATCH; goto out; }
    W.state |= WR_STATE_HOOKED;
    wr_log("ready (wheel click on an empty tile selects the region's village/port/city/gate)");

    {
        int focus_prev = focus_normal();
        while (!W.stop) {
            int fire = 0, focus_now;
            Sleep((unsigned long)W.cfg.poll_ms);
            focus_now = focus_normal();
            if (W.cfg.trigger_vk > 0) {
                int down = (GetAsyncKeyState(W.cfg.trigger_vk) & 0x8000) != 0;
                fire = down && !prev;
                prev = down;
            }
            if (W.trigger_req) { W.trigger_req = 0; fire = 1; focus_prev = focus_now; }
            if (fire && !W.paused) {
                if (W.cfg.normal_mode_only && W.press_logs < 40) { W.press_logs++; log_top_state(focus_prev); }
                redirect_once(focus_prev);
            }
            if (!prev) focus_prev = focus_now; /* keep the pre-press sample while the button is held */
        }
    }
    *block_flag() = 0;
    if (W.self_patched && self_install(W.patches, 3, 1)) { W.state &= ~WR_STATE_HOOKED; wr_log("stopped, original bytes restored"); }
out:
    W.state &= ~WR_STATE_STARTED;
    W.worker_done = 1;
    return 0;
}

/* ------------------------------------------------------------------ API */
int WR_Start(const WR_Host *host, const WR_Config *cfg) {
    void *h;
    if (WR_CAS32(&W.started, 0, 1) != 0) return 1;
    bfill_((U8 *)&W.host, 0, sizeof W.host);
    bfill_((U8 *)&W.cfg, 0, sizeof W.cfg);
    W.vt_focus = 0;
    W.self_patched = 0;
    if (host) bcopy_((U8 *)&W.host, (const U8 *)host, host->size < sizeof W.host ? host->size : sizeof W.host);
    W.host.size = sizeof W.host;
    if (cfg) bcopy_((U8 *)&W.cfg, (const U8 *)cfg, cfg->size < sizeof W.cfg ? cfg->size : sizeof W.cfg);
    W.cfg.size = sizeof W.cfg;
    if (W.cfg.trigger_vk == 0) W.cfg.trigger_vk = 0x04; /* VK_MBUTTON */
    if (W.cfg.hold_ms <= 0 || W.cfg.hold_ms > 5000) W.cfg.hold_ms = 600;
    if (W.cfg.wait_seconds <= 0) W.cfg.wait_seconds = 300;
    if (W.cfg.poll_ms <= 0 || W.cfg.poll_ms > 200) W.cfg.poll_ms = 15;
    W.stop = 0;
    W.worker_done = 0;
    W.state = WR_STATE_STARTED;
    h = CreateThread(0, 0, worker, 0, 0, 0);
    if (!h) { W.state = 0; W.started = 0; return 0; }
    CloseHandle(h);
    return 1;
}

void WR_Stop(void) {
    int i;
    if (!W.started) return;
    W.stop = 1;
    for (i = 0; i < 400 && !W.worker_done; i++) Sleep(10);
    W.state |= WR_STATE_STOPPED;
    W.started = 0;
}

unsigned int WR_GetState(void) { return W.state; }

unsigned int WR_GetCounter(int which) {
    static const U32 off[5] = {0, WR_OFF_AHITS, WR_OFF_BHITS, WR_OFF_KCALLS, WR_OFF_KHITS};
    if (which == 0) return W.redirects;
    if (which < 0 || which > 4 || !W.block) return 0;
    return *(volatile U32 *)(W.block + off[which]);
}

int WR_SetTargets(const unsigned short *tiles, unsigned int count) {
    unsigned short *t;
    if (!tiles || count != WR_TILES || W.started) return 0;
    t = (unsigned short *)VirtualAlloc(0, WR_TILES * 2, 0x3000, 0x04);
    if (!t) return 0;
    bcopy_((U8 *)t, (const U8 *)tiles, WR_TILES * 2);
    W.table = t;
    W.table_from_host = 1;
    return 1;
}

void WR_Trigger(void) { W.trigger_req = 1; }
void WR_SetEnabled(int on) { W.paused = on ? 0 : 1; }
