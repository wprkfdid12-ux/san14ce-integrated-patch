/*
 * RTK14 DragLearn (1인 드래그습득) - core. See drag_learn.h and README_KO.md.
 *
 * v1 = the tested Cheat Engine table RTK14_gem_test8.CT (books); v2 adds several generals (P/Q).
 *   - a worker thread waits until Steam has unpacked the game code, finds 6 code sites,
 *     9 game functions and 5 globals by byte patterns (no fixed addresses),
 *   - copies the machine-code block (dl_block.S) next to the game image and redirects
 *     the 6 sites into it. After that the worker exits; everything runs on the game thread.
 */
#include <stddef.h>
#include <stdint.h>
#include "drag_learn.h"
#include "dl_block.h"

#ifdef DL_USE_WINDOWS_H
#include <windows.h>
#define DL_WINAPI WINAPI
#else
#ifndef DL_IMPORT
#define DL_IMPORT __declspec(dllimport)
#endif
#ifndef DL_WINAPI
#define DL_WINAPI __stdcall
#endif
DL_IMPORT void *DL_WINAPI GetModuleHandleW(const wchar_t *);
DL_IMPORT void *DL_WINAPI VirtualAlloc(void *, size_t, unsigned long, unsigned long);
DL_IMPORT int DL_WINAPI VirtualProtect(void *, size_t, unsigned long, unsigned long *);
DL_IMPORT int DL_WINAPI FlushInstructionCache(void *, const void *, size_t);
DL_IMPORT void *DL_WINAPI GetCurrentProcess(void);
DL_IMPORT void *DL_WINAPI CreateThread(void *, size_t, unsigned long(DL_WINAPI *)(void *), void *, unsigned long, unsigned long *);
DL_IMPORT int DL_WINAPI CloseHandle(void *);
DL_IMPORT void DL_WINAPI Sleep(unsigned long);
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#define DL_CAS64(p, o, n) _InterlockedCompareExchange64((volatile long long *)(p), (n), (o))
#define DL_CAS32(p, o, n) _InterlockedCompareExchange((volatile long *)(p), (n), (o))
#else
#define DL_CAS64(p, o, n) __sync_val_compare_and_swap((volatile long long *)(p), (o), (n))
#define DL_CAS32(p, o, n) __sync_val_compare_and_swap((volatile long *)(p), (o), (n))
#endif

typedef unsigned char U8;
typedef uint32_t U32;
typedef uintptr_t UPTR;

/* ------------------------------------------------------------------ patterns
   (each verified unique in SAN14CE_KO / SAN14CE / SAN14CE_SC / SAN14CE_TC.exe, 2026-10 Steam build) */
typedef struct { const U8 *bytes; const char *mask; U32 len; } Pat;
#define PAT(name, ...) static const U8 name##_b[] = {__VA_ARGS__}
/* S: list setup "mov edi,1 / mov [rbp-48],edi / mov [rbp-44],rdi / mov ecx,5BC / call" -> site +8, 9 bytes */
PAT(pS, 0xBF,0x01,0x00,0x00,0x00,0x89,0x7D,0xB8,0x48,0x89,0x7D,0xBC,0xB9,0xBC,0x05,0x00,0x00,0xE8);
/* S function start: "mov r13,rcx / mov rcx,[rcx+488] / call" (r13 = learn window) */
PAT(pSF, 0x4C,0x8B,0xE9,0x48,0x8B,0x89,0x88,0x04,0x00,0x00,0xE8);
/* X: "mov r8,[r13+4B8] / mov r9,[r8+1A0] / mov rdx,[rsp+38]" -> site +0, 7 bytes */
PAT(pX, 0x4D,0x8B,0x85,0xB8,0x04,0x00,0x00,0x4D,0x8B,0x88,0xA0,0x01,0x00,0x00,0x48,0x8B,0x54,0x24,0x38);
/* near X: "mov rax,[rip+cnt] / cmp qword [rax+rcx*8],0" */
PAT(pXC, 0x48,0x8B,0x05,0,0,0,0,0x48,0x83,0x3C,0xC8,0x00);
/* Y: learn command built from the window (+490 replace, +478 force), then "call LearnTactic" at +32 */
PAT(pY, 0x48,0x8B,0x83,0x90,0x04,0x00,0x00,0x48,0x89,0x44,0x24,0x40,0x48,0x8B,0x83,0x78,0x04,0x00,0x00,
        0x48,0x89,0x44,0x24,0x48,0x48,0x89,0xBC,0x24,0x80,0x08,0x00,0x00,0xE8,0,0,0,0,0x48,0x8B,0x8B,0x78,0x04,0x00,0x00);
/* inside LearnTactic */
PAT(pL1, 0x48,0x8B,0xF9,0x48,0x8B,0x09,0xE8,0,0,0,0);                                   /* call valid */
PAT(pL2, 0x0F,0xB6,0x48,0x52,0xBD,0xC8,0x00,0x00,0x00,0x48,0x8B,0x05,0,0,0,0);           /* db global */
PAT(pL3, 0x8B,0xD5,0x48,0x8B,0x0F,0xE8,0,0,0,0,0x41,0x3B,0xC7);                          /* call slot */
PAT(pL4, 0x48,0x8B,0x4F,0x18,0xE8,0,0,0,0,0x48,0x8B,0xD8,0x0F,0xB6,0x48,0x14,0x2B,0x0D,0,0,0,0); /* orders fn, cost */
PAT(pL5, 0x48,0x8B,0x0D,0,0,0,0,0x48,0x85,0xC9,0x74,0,0x3B,0x05,0,0,0,0);               /* result head, range */
PAT(pL6, 0x4C,0x8B,0xAC,0xC8,0xD0,0x74,0x07,0x00);                                      /* tactic table +774D0 */
/* P: general list "mov esi,1 / mov [rsp+58],esi / mov [rsp+5C],rsi / mov ecx,32E / call" -> site +9, 10 bytes */
PAT(pP, 0xBE,0x01,0x00,0x00,0x00,0x89,0x74,0x24,0x58,0x48,0x89,0x74,0x24,0x5C,0xB9,0x2E,0x03,0x00,0x00,0xE8);
/* Q: after the general list "lea rcx,[rsp+40] / call / mov r14d,14000 / mov rdx,[rsp+28] ..." -> site +10, 6 bytes */
PAT(pQ, 0x48,0x8D,0x4C,0x24,0x40,0xE8,0,0,0,0,0x41,0xBE,0x00,0x40,0x01,0x00,0x48,0x8B,0x54,0x24,0x28,0x48,0x85,0xD2,
        0x74,0x04,0x8B,0x02,0xEB,0x03,0x49,0x8B,0xC6,0x8B,0xC8);
/* in the book-list function (S-0x300..S): grey-out rule */
PAT(pE1, 0x49,0x8B,0xCF,0xE8,0,0,0,0,0x8B,0xF0,0x4D,0x8B,0xB5,0x88,0x04,0x00,0x00,0x33,0xFF);       /* call tactic id */
PAT(pE2, 0x48,0x8D,0xB0,0x28,0x74,0x07,0x00,0x48,0x05,0xD0,0x74,0x07,0x00,0x48,0x3B,0xF0,0x74,0,0x4C,0x8B,0x36,
         0x49,0x8B,0xCE,0xE8,0,0,0,0,0x85,0xC0,0x74,0,0x49,0x8B,0xCE,0xE8,0,0,0,0,0x8B,0xF8,0x83,0xF8,0x14,0x77,0,
         0x41,0x8B,0x4F,0x6C,0x33,0xD2,0x0F,0xA3,0xC1,0x0F,0x92,0xC2,0x85,0xD2,0x74,0,0x49,0x8B,0x8D,0x88,0x04,0x00,0x00,
         0x41,0x80,0x7E,0x7E,0x02,0x75,0,0xE8,0,0,0,0,0x3B,0xC7,0x0F,0x84,0,0,0,0,0xEB,0,0x8B,0xD7,0xE8,0,0,0,0,0x85,0xC0);
/* D: right after E2 - "jne ok / add rsi,8 / ... / jne loop / cmp dword [rip+lock],0 / je" ; site +0x18, 7 bytes */
PAT(pD, 0x75,0,0x48,0x83,0xC6,0x08,0x48,0x8B,0x05,0,0,0,0,0x48,0x05,0xD0,0x74,0x07,0x00,0x48,0x3B,0xF0,0x75,0,
        0x83,0x3D,0,0,0,0,0x00,0x74);
/* in LearnTactic: book index + force book counts at +120 */
PAT(pG, 0x48,0x8B,0x4F,0x08,0xE8,0,0,0,0,0x4C,0x8B,0x47,0x18,0x8D,0x50,0xFF,0x83,0xFA,0x1D,0x77,0,0x48,0x63,0xC8,
        0x41,0x0F,0xB7,0x8C,0x48,0x20,0x01,0x00,0x00);
static const Pat PP = {pP_b, "xxxxxxxxxxxxxxxxxxxx", sizeof pP_b};
typedef char PP_mask_len_check[(sizeof("xxxxxxxxxxxxxxxxxxxx") - 1 == sizeof pP_b) ? 1 : -1];
static const Pat PQ = {pQ_b, "xxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxx", sizeof pQ_b};
typedef char PQ_mask_len_check[(sizeof("xxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxx") - 1 == sizeof pQ_b) ? 1 : -1];
static const Pat PE1 = {pE1_b, "xxxx????xxxxxxxxxxx", sizeof pE1_b};
typedef char PE1_mask_len_check[(sizeof("xxxx????xxxxxxxxxxx") - 1 == sizeof pE1_b) ? 1 : -1];
static const Pat PE2 = {pE2_b, "xxxxxxxxxxxxxxxxx?xxxxxxx????xxx?xxxx????xxxxxx?xxxxxxxxxxxxxxx?xxxxxxxxxxxxx?x????xxxx????x?xxx????xx", sizeof pE2_b};
typedef char PE2_mask_len_check[(sizeof("xxxxxxxxxxxxxxxxx?xxxxxxx????xxx?xxxx????xxxxxx?xxxxxxxxxxxxxxx?xxxxxxxxxxxxx?x????xxxx????x?xxx????xx") - 1 == sizeof pE2_b) ? 1 : -1];
static const Pat PG = {pG_b, "xxxxx????xxxxxxxxxxx?xxxxxxxxxxxx", sizeof pG_b};
typedef char PG_mask_len_check[(sizeof("xxxxx????xxxxxxxxxxx?xxxxxxxxxxxx") - 1 == sizeof pG_b) ? 1 : -1];
static const Pat PD = {pD_b, "x?xxxxxxx????xxxxxxxxxx?xx????xx", sizeof pD_b};
typedef char PD_mask_len_check[(sizeof("x?xxxxxxx????xxxxxxxxxx?xx????xx") - 1 == sizeof pD_b) ? 1 : -1];
static const Pat PS = {pS_b, "xxxxxxxxxxxxxxxxxx", sizeof pS_b};
typedef char PS_mask_len_check[(sizeof("xxxxxxxxxxxxxxxxxx") - 1 == sizeof pS_b) ? 1 : -1];
static const Pat PSF = {pSF_b, "xxxxxxxxxxx", sizeof pSF_b};
typedef char PSF_mask_len_check[(sizeof("xxxxxxxxxxx") - 1 == sizeof pSF_b) ? 1 : -1];
static const Pat PX = {pX_b, "xxxxxxxxxxxxxxxxxxx", sizeof pX_b};
typedef char PX_mask_len_check[(sizeof("xxxxxxxxxxxxxxxxxxx") - 1 == sizeof pX_b) ? 1 : -1];
static const Pat PXC = {pXC_b, "xxx????xxxxx", sizeof pXC_b};
typedef char PXC_mask_len_check[(sizeof("xxx????xxxxx") - 1 == sizeof pXC_b) ? 1 : -1];
static const Pat PY = {pY_b, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx", sizeof pY_b};
typedef char PY_mask_len_check[(sizeof("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx") - 1 == sizeof pY_b) ? 1 : -1];
static const Pat PL1 = {pL1_b, "xxxxxxx????", sizeof pL1_b};
typedef char PL1_mask_len_check[(sizeof("xxxxxxx????") - 1 == sizeof pL1_b) ? 1 : -1];
static const Pat PL2 = {pL2_b, "xxxxxxxxxxxx????", sizeof pL2_b};
typedef char PL2_mask_len_check[(sizeof("xxxxxxxxxxxx????") - 1 == sizeof pL2_b) ? 1 : -1];
static const Pat PL3 = {pL3_b, "xxxxxx????xxx", sizeof pL3_b};
typedef char PL3_mask_len_check[(sizeof("xxxxxx????xxx") - 1 == sizeof pL3_b) ? 1 : -1];
static const Pat PL4 = {pL4_b, "xxxxx????xxxxxxxxx????", sizeof pL4_b};
typedef char PL4_mask_len_check[(sizeof("xxxxx????xxxxxxxxx????") - 1 == sizeof pL4_b) ? 1 : -1];
static const Pat PL5 = {pL5_b, "xxx????xxxx?xx????", sizeof pL5_b};
typedef char PL5_mask_len_check[(sizeof("xxx????xxxx?xx????") - 1 == sizeof pL5_b) ? 1 : -1];
static const Pat PL6 = {pL6_b, "xxxxxxxx", sizeof pL6_b};
typedef char PL6_mask_len_check[(sizeof("xxxxxxxx") - 1 == sizeof pL6_b) ? 1 : -1];

static const U8 ORIG_S[9] = {0x48, 0x89, 0x7D, 0xBC, 0xB9, 0xBC, 0x05, 0x00, 0x00};
static const U8 ORIG_X[7] = {0x4D, 0x8B, 0x85, 0xB8, 0x04, 0x00, 0x00};
static const U8 ORIG_P[10] = {0x48, 0x89, 0x74, 0x24, 0x5C, 0xB9, 0x2E, 0x03, 0x00, 0x00};
static const U8 ORIG_Q[6] = {0x41, 0xBE, 0x00, 0x40, 0x01, 0x00};
#define NPATCH 9   /* 6 for the feature + 3 optional display fixes (R1, R2, R3) */

/* display fixes (2026-10-15): learn window = "state" with +4B8 sub window L.
   R1 (after the general list): "mov rcx,[rbx+4B8] / test / je / mov rdx,[rbx+488] / call ShowGeneral / mov rax,[rbx+4B8] /
       mov dword [rax+180],1" -> call at +19
   R2 (연속입력 reopens L): "mov rcx,[rdi+478] / lea rdx,[rdi+4A8] / call / mov rcx,[rdi+4B8] / call Reopen /
       mov rbx,[rsp+30] / mov dword [rdi+470],1" -> call at +0x1A
   LP (in Reopen): orders preview texts of L+130: B5 now, B7 after ("%3d"), B8 "(^03-n^00)" ("(-n)" when 0)
   NM (general card refresh): "mov rcx,[rbx+178] / call Name / ... / lea r15,[rbx+130] / mov edx,2 / call GetW /
       ... / lea edx,[r9+1] (2) / test rax,rax / jne / lea edx,[r9+6] (7) / call SetText" */
static const U8 pR1_b[] = {0x48,0x8B,0x8B,0xB8,0x04,0x00,0x00,0x48,0x85,0xC9,0x74,0,0x48,0x8B,0x93,0x88,0x04,0x00,0x00,0xE8,0,0,0,0,
                           0x48,0x8B,0x83,0xB8,0x04,0x00,0x00,0xC7,0x80,0x80,0x01,0x00,0x00,0x01,0x00,0x00,0x00};
static const Pat PR1 = {pR1_b, "xxxxxxxxxxx?xxxxxxxx????xxxxxxxxxxxxxxxxx", sizeof pR1_b};
typedef char PR1_mask_len_check[(sizeof("xxxxxxxxxxx?xxxxxxxx????xxxxxxxxxxxxxxxxx") - 1 == sizeof pR1_b) ? 1 : -1];
static const U8 pR2_b[] = {0x48,0x8B,0x8F,0x78,0x04,0x00,0x00,0x48,0x8D,0x97,0xA8,0x04,0x00,0x00,0xE8,0,0,0,0,0x48,0x8B,0x8F,0xB8,0x04,0x00,0x00,
                           0xE8,0,0,0,0,0x48,0x8B,0x5C,0x24,0x30,0xC7,0x87,0x70,0x04,0x00,0x00,0x01,0x00,0x00,0x00};
static const Pat PR2 = {pR2_b, "xxxxxxxxxxxxxxx????xxxxxxxx????xxxxxxxxxxxxxxx", sizeof pR2_b};
typedef char PR2_mask_len_check[(sizeof("xxxxxxxxxxxxxxx????xxxxxxxx????xxxxxxxxxxxxxxx") - 1 == sizeof pR2_b) ? 1 : -1];
/* R3 (after the book list): "mov rcx,[rbx+480] / call BookTactic / mov rcx,[rbx+4B8] / mov rdx,rax / mov r9d,1 /
   mov r8d,edi / call ShowTactic / mov rax,[rbx+4B8]" -> call at +0x1F. ShowTactic(L, t): L+190 = t, text 51 = t's name */
static const U8 pR3_b[] = {0x48,0x8B,0x8B,0x80,0x04,0x00,0x00,0xE8,0,0,0,0,0x48,0x8B,0x8B,0xB8,0x04,0x00,0x00,0x48,0x8B,0xD0,
                           0x41,0xB9,0x01,0x00,0x00,0x00,0x44,0x8B,0xC7,0xE8,0,0,0,0,0x48,0x8B,0x83,0xB8,0x04,0x00,0x00};
static const Pat PR3 = {pR3_b, "xxxxxxxx????xxxxxxxxxxxxxxxxxxxx????xxxxxxx", sizeof pR3_b};
typedef char PR3_mask_len_check[(sizeof("xxxxxxxx????xxxxxxxxxxxxxxxxxxxx????xxxxxxx") - 1 == sizeof pR3_b) ? 1 : -1];
static const U8 pLP_b[] = {0x48,0x8D,0xAF,0x30,0x01,0x00,0x00,0x41,0xB9,0x01,0x00,0x00,0x00,0x48,0x8B,0xCD,0x4C,0x8B,0xC3,0xBA,0xB5,0x00,0x00,0x00,0xE8};
static const Pat PLP = {pLP_b, "xxxxxxxxxxxxxxxxxxxxxxxxx", sizeof pLP_b};
typedef char PLP_mask_len_check[(sizeof("xxxxxxxxxxxxxxxxxxxxxxxxx") - 1 == sizeof pLP_b) ? 1 : -1];
static const U8 pNM_b[] = {0x48,0x8B,0x8B,0x78,0x01,0x00,0x00,0xE8,0,0,0,0,0x48,0x8B,0xF8,0xEB,0x07,0x48,0x8B,0x3D,0,0,0,0,
                           0x4C,0x8D,0xBB,0x30,0x01,0x00,0x00,0xBA,0x02,0x00,0x00,0x00,0x49,0x8B,0xCF,0xE8,0,0,0,0,
                           0x41,0xB9,0x01,0x00,0x00,0x00,0x4C,0x8B,0xC7,0x49,0x8B,0xCF,0x41,0x8D,0x51,0x01,0x48,0x85,0xC0,0x75,0x04,
                           0x41,0x8D,0x51,0x06,0xE8};
static const Pat PNM = {pNM_b, "xxxxxxxx????xxxxxxxx????xxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxx", sizeof pNM_b};
typedef char PNM_mask_len_check[(sizeof("xxxxxxxx????xxxxxxxx????xxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxx") - 1 == sizeof pNM_b) ? 1 : -1];

#define GROUP_ID "SAN14CE.DragLearn"

/* ------------------------------------------------------------------ state */
static struct {
    DL_Host host;
    DL_Config cfg;
    volatile long started, stop, worker_done;
    volatile unsigned int state;
    U8 *game, *text_lo, *text_hi;
    size_t image_size;
    U8 *block;
    U8 *site_s, *site_x, *site_y, *site_p, *site_q, *site_d, *d_enable, *p_lock;
    U8 orig_d[7];
    U8 orig_y[5];
    U8 *f_valid, *f_orders, *f_slot, *f_learn, *f_tid, *f_fid, *f_pspec, *f_pform, *f_gidx;
    U8 *p_db, *p_cost, *p_head, *p_cnt, *p_npool;
    U8 *site_r1, *site_r2, *site_r3, orig_r1[5], orig_r2[5], orig_r3[5];
    U8 *f_name, *f_getw, *f_text;  /* display fix: 0 = off */
    U32 npatch;
    int pv_shown, bk_shown, lbl_shown;
    DL_Patch patches[NPATCH];
    int self_patched;
} D;

/* ------------------------------------------------------------------ helpers (no CRT) */
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
static void out(Line *l) { if (D.host.log) D.host.log(l->s); }
static void dl_log(const char *t) { Line l; l.n = 0; l.s[0] = 0; lput(&l, "DragLearn: "); lput(&l, t); out(&l); }
static void log_rva(const char *t, const void *p) {
    Line l; l.n = 0; l.s[0] = 0; lput(&l, "DragLearn: "); lput(&l, t);
    lput(&l, " exe+"); lhex(&l, (uint64_t)((const U8 *)p - D.game)); out(&l);
}

/* matches of p in [lo,hi); *first = first match; stops counting at 2 */
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
static U8 *rel_target(U8 *at /* rel32 */, U32 tail /* bytes after the rel32 to the next instruction */) {
    int32_t r = (int32_t)((U32)at[0] | (U32)at[1] << 8 | (U32)at[2] << 16 | (U32)at[3] << 24);
    return at + 4 + tail + r;
}
static int in_text(const U8 *p) { return p >= D.text_lo && p < D.text_hi; }
static int in_image(const U8 *p) { return p >= D.game && p < D.game + D.image_size; }
static U8 *clamp_lo(U8 *p) { return p < D.text_lo ? D.text_lo : p; }
static U8 *clamp_hi(U8 *p) { return p > D.text_hi ? D.text_hi : p; }

static int find_text(void) {
    U8 *g = (U8 *)GetModuleHandleW(0);
    U32 pe, nsec, i, optsz;
    if (!g) return 0;
    pe = *(U32 *)(g + 0x3C);
    if (*(U32 *)(g + pe) != 0x4550) return 0;
    nsec = *(unsigned short *)(g + pe + 6);
    optsz = *(unsigned short *)(g + pe + 20);
    D.image_size = *(U32 *)(g + pe + 24 + 56);
    D.game = g;
    for (i = 0; i < nsec; i++) {
        U8 *s = g + pe + 24 + optsz + 40 * i;
        if (s[0] == '.' && s[1] == 't' && s[2] == 'e' && s[3] == 'x' && s[4] == 't' && !s[5]) {
            D.text_lo = g + *(U32 *)(s + 12);
            D.text_hi = D.text_lo + *(U32 *)(s + 8);
            return 1;
        }
    }
    return 0;
}

/* one unique match inside [lo,hi), else 0 */
static U8 *one(U8 *lo, U8 *hi, const Pat *p, const char *name, Line *l) {
    U8 *m = 0;
    int n = scan(clamp_lo(lo), clamp_hi(hi), p, &m);
    lput(l, " "); lput(l, name); lput(l, "="); ldec(l, (U32)n);
    return n == 1 ? m : 0;
}

/* ------------------------------------------------------------------ display fixes
   called from the block (X, R1, R2) with the learn window (state): the general card shows
   "우금 외 2명" when several generals are picked, and the orders preview shows books x generals */
typedef void(*FnText)(U8 *box, U32 id, const unsigned short *t, U32 one);
typedef U8 *(*FnGetW)(U8 *box, U32 id);
typedef const unsigned short *(*FnName)(U8 *person);
typedef U8 *(*FnObj)(U8 *o);
typedef int (*FnValidI)(U8 *o);
typedef int (*FnIdx)(U8 *book);
static int put_w(unsigned short *o, int k, const char *a) { while (*a) o[k++] = (unsigned short)*a++; return k; }
static int put_i(unsigned short *o, int k, int v, int width) {
    unsigned short r[12]; int i = 0, neg = v < 0; unsigned int u = neg ? (unsigned int)(-v) : (unsigned int)v;
    do { r[i++] = (unsigned short)('0' + u % 10); u /= 10; } while (u && i < 11);
    while (i + neg < width) { o[k++] = ' '; width--; }
    if (neg) o[k++] = '-';
    while (i) o[k++] = r[--i];
    return k;
}
#define BQ(off) (*(void **)(D.block + (off)))
#define BD(off) (*(volatile U32 *)(D.block + (off)))
/* books of this kind the force still has, -1 = unknown */
static int book_count(U8 *st, U8 *gem) {
    int idx;
    U8 *force = *(U8 **)(st + 0x478);
    if (!gem || !force || !D.f_gidx) return -1;
    idx = ((FnIdx)(void *)D.f_gidx)(gem);
    if (idx < 1 || idx > 30) return -1;
    return *(unsigned short *)(force + 0x120 + idx * 2);
}
void dl_fix(U8 *st) {
    unsigned short b[64];
    U8 *L, *ord, *card, *box;
    int k, nb = 1, np = 1, now, tot;
    if (!st || !D.block || !D.f_text) return;
    L = *(U8 **)(st + 0x4B8);
    if (!L) return;
    if (BD(DL_OFF_SELN) > 1 && *(U8 **)(st + 0x480) && *(U8 **)(st + 0x480) == BQ(DL_OFF_GEMS)) nb = (int)BD(DL_OFF_SELN);
    if (BD(DL_OFF_NPERS) > 1 && *(U8 **)(st + 0x488) && *(U8 **)(st + 0x488) == BQ(DL_OFF_PERSONS)) np = (int)BD(DL_OFF_NPERS);
    /* general card: "<name> 외 n명" */
    card = *(U8 **)(L + 0x1B0);
    if ((np > 1 || D.lbl_shown) && card && D.f_name && D.f_getw && *(U8 **)(st + 0x488) &&
        *(U8 **)(card + 0x178) == *(U8 **)(st + 0x488)) {
        const unsigned short *nm = ((FnName)(void *)D.f_name)(*(U8 **)(st + 0x488));
        k = 0;
        if (nm) while (nm[k] && k < 40) { b[k] = nm[k]; k++; }
        if (np > 1) {
            b[k++] = ' '; b[k++] = 0xC678; b[k++] = ' ';               /* 외 */
            k = put_i(b, k, np - 1, 0); b[k++] = 0xBA85;              /* n명 */
        }
        b[k] = 0;
        box = card + 0x130;
        ((FnText)(void *)D.f_text)(box, ((FnGetW)(void *)D.f_getw)(box, 2) ? 2u : 7u, b, 1);
        D.lbl_shown = np > 1;
    }
    /* picked tactic (text 51): "돌격 외 2" when several books are picked */
    if (nb > 1) {
        U8 *t = *(U8 **)(L + 0x190);
        if (t && ((FnValidI)(void *)D.f_valid)(t)) {
            const unsigned short *nm = (const unsigned short *)(t + 0x10);
            k = 0;
            while (nm[k] && k < 40) { b[k] = nm[k]; k++; }
            b[k++] = ' '; b[k++] = 0xC678; b[k++] = ' ';               /* 외 */
            k = put_i(b, k, nb - 1, 0); b[k] = 0;
            ((FnText)(void *)D.f_text)(L + 0x130, 0x51, b, 1);
        }
    }
    /* uses: a book is used once per general, but never more than the books left
       (the learn stops when a book runs out) -> per book min(generals, count) */
    {
        int i, uses = 0, first = 1;
        for (i = 0; i < nb; i++) {
            U8 *gm = i ? (U8 *)BQ(DL_OFF_GEMS + 8 * i) : *(U8 **)(st + 0x480);
            int u = np, c = book_count(st, gm);
            if (c >= 0 && u > c) u = c;
            if (!i) first = u;
            uses += u;
        }
        /* book use preview (AC now / AE after / AF "(^03-n^00)") */
        if ((np > 1 || D.bk_shown) && *(U8 **)(st + 0x480)) {
            int c = book_count(st, *(U8 **)(st + 0x480));
            if (c >= 0) {
                k = put_i(b, 0, c, 3); b[k] = 0;
                ((FnText)(void *)D.f_text)(L + 0x130, 0xAC, b, 1);
                k = put_i(b, 0, c - first, 3); b[k] = 0;
                ((FnText)(void *)D.f_text)(L + 0x130, 0xAE, b, 1);
                k = put_w(b, 0, "(^03-"); k = put_i(b, k, first, 0); k = put_w(b, k, "^00)"); b[k] = 0;
                ((FnText)(void *)D.f_text)(L + 0x130, 0xAF, b, 1);
                D.bk_shown = np > 1;
            }
        }
        if (uses < 1) uses = 1;
        nb = uses; np = 1;   /* orders below = uses x cost */
    }
    /* orders preview (the game draws it once, for one learn) */
    if (nb * np == 1 && !D.pv_shown) return;
    ord = ((FnObj)(void *)D.f_orders)(*(U8 **)(st + 0x478));
    if (!ord) return;
    now = ord[0x14];
    tot = *(volatile int *)D.p_cost * nb * np;
    box = L + 0x130;
    k = put_i(b, 0, now, 3); b[k] = 0;
    ((FnText)(void *)D.f_text)(box, 0xB5, b, 1);
    k = put_i(b, 0, now - tot, 3); b[k] = 0;
    ((FnText)(void *)D.f_text)(box, 0xB7, b, 1);
    k = put_w(b, 0, now ? "(^03-" : "(-"); k = put_i(b, k, tot, 0); k = put_w(b, k, now ? "^00)" : ")"); b[k] = 0;
    ((FnText)(void *)D.f_text)(box, 0xB8, b, 1);
    D.pv_shown = nb * np > 1;
}

static void resolve_fix(void) {
    Line l;
    U8 *r1, *r2, *r3, *lp, *nm;
    l.n = 0; l.s[0] = 0;
    lput(&l, "DragLearn: display fix matches");
    r1 = one(D.text_lo, D.text_hi, &PR1, "R1", &l);
    r2 = one(D.text_lo, D.text_hi, &PR2, "R2", &l);
    r3 = one(D.text_lo, D.text_hi, &PR3, "R3", &l);
    lp = one(D.text_lo, D.text_hi, &PLP, "LP", &l);
    nm = one(D.text_lo, D.text_hi, &PNM, "NM", &l);
    out(&l);
    D.site_r1 = D.site_r2 = D.site_r3 = 0; D.f_text = D.f_name = D.f_getw = 0;
    if (!r1 || !r2 || !r3 || !lp || !nm) { dl_log("display fix off (game code differs) - the feature itself is not affected"); return; }
    D.f_text = rel_target(nm + 70, 0);
    D.f_name = rel_target(nm + 8, 0);
    D.f_getw = rel_target(nm + 40, 0);
    D.site_r1 = r1 + 19;
    D.site_r2 = r2 + 0x1A;
    D.site_r3 = r3 + 0x1F;
    /* R2 must call the function that holds LP (preview B5/B7/B8) and uses the same SetText */
    if (!in_text(D.f_text) || !in_text(D.f_name) || !in_text(D.f_getw) || D.site_r1[0] != 0xE8 || D.site_r2[0] != 0xE8 ||
        !in_text(rel_target(D.site_r1 + 1, 0)) || rel_target(D.site_r2 + 1, 0) > lp || lp - rel_target(D.site_r2 + 1, 0) > 0x200 ||
        rel_target(lp + 25, 0) != D.f_text || D.site_r3[0] != 0xE8 || !in_text(rel_target(D.site_r3 + 1, 0))) {
        D.site_r1 = D.site_r2 = D.site_r3 = 0; D.f_text = D.f_name = D.f_getw = 0;
        dl_log("display fix off (checks failed) - the feature itself is not affected");
        return;
    }
    bcopy_(D.orig_r1, D.site_r1, 5);
    bcopy_(D.orig_r2, D.site_r2, 5);
    bcopy_(D.orig_r3, D.site_r3, 5);
    log_rva("R1 (general shown)", D.site_r1);
    log_rva("R2 (window reopens)", D.site_r2);
    log_rva("R3 (tactic shown)", D.site_r3);
    log_rva("set text", D.f_text);
    log_rva("general name", D.f_name);
}

static int resolve(void) {
    Line l;
    U8 *s, *sf, *x, *xc, *y, *l1, *l2, *l3, *l4, *l5, *l6, *x5, *pp, *q, *e1, *e2, *g, *pd;
    l.n = 0; l.s[0] = 0;
    lput(&l, "DragLearn: pattern matches");
    s = one(D.text_lo, D.text_hi, &PS, "S", &l);
    x = one(D.text_lo, D.text_hi, &PX, "X", &l);
    y = one(D.text_lo, D.text_hi, &PY, "Y", &l);
    pp = one(D.text_lo, D.text_hi, &PP, "P", &l);
    q = one(D.text_lo, D.text_hi, &PQ, "Q", &l);
    if (!s || !x || !y || !pp || !q) { out(&l); return 0; }
    e1 = one(s - 0x300, s, &PE1, "E1", &l);
    e2 = one(s - 0x300, s, &PE2, "E2", &l);
    pd = e2 ? one(e2 + 0x60, e2 + 0x100, &PD, "D", &l) : 0;
    sf = one(s - 0x600, s, &PSF, "SF", &l);
    xc = one(x, x + 0x40, &PXC, "XC", &l);
    D.site_s = s + 8;
    D.site_x = x;
    D.site_y = y + 32;
    D.f_learn = rel_target(D.site_y + 1, 0);
    if (!in_text(D.f_learn)) { out(&l); dl_log("learn call outside code"); return 0; }
    l1 = one(D.f_learn, D.f_learn + 0x600, &PL1, "L1", &l);
    l2 = one(D.f_learn, D.f_learn + 0x600, &PL2, "L2", &l);
    l3 = one(D.f_learn, D.f_learn + 0x600, &PL3, "L3", &l);
    l4 = one(D.f_learn, D.f_learn + 0x600, &PL4, "L4", &l);
    l5 = one(D.f_learn, D.f_learn + 0x600, &PL5, "L5", &l);
    l6 = one(D.f_learn, D.f_learn + 0x600, &PL6, "L6", &l);
    g = one(D.f_learn, D.f_learn + 0x600, &PG, "G", &l);
    x5 = one(x, x + 0x80, &PL5, "X5", &l);
    out(&l);
    if (!sf || !xc || !l1 || !l2 || !l3 || !l4 || !l5 || !l6 || !x5 || !e1 || !e2 || !g || !pd) return 0;
    if (pd != e2 + 0x66) { dl_log("D not right after E2"); return 0; }
    D.site_d = pd + 0x18;
    D.d_enable = pd + 2 + (signed char)pd[1];
    D.p_lock = rel_target(D.site_d + 2, 1);
    D.site_p = pp + 9;
    D.site_q = q + 10;
    D.f_tid = rel_target(e1 + 4, 0);
    D.f_fid = rel_target(e2 + 37, 0);
    D.f_pspec = rel_target(e2 + 79, 0);
    D.f_pform = rel_target(e2 + 96, 0);
    D.f_gidx = rel_target(g + 5, 0);
    D.f_valid = rel_target(l1 + 7, 0);
    D.p_db = rel_target(l2 + 12, 0);
    D.f_slot = rel_target(l3 + 6, 0);
    D.f_orders = rel_target(l4 + 5, 0);
    D.p_cost = rel_target(l4 + 18, 0);
    D.p_head = rel_target(l5 + 3, 0);
    D.p_npool = rel_target(l5 + 14, 0);
    D.p_cnt = rel_target(xc + 3, 0);
    if (rel_target(x5 + 3, 0) != D.p_head || rel_target(x5 + 14, 0) != D.p_npool) { dl_log("result tables disagree"); return 0; }
    if (rel_target(e2 + 25, 0) != D.f_valid) { dl_log("valid() disagrees"); return 0; }
    if (!in_text(D.f_valid) || !in_text(D.f_slot) || !in_text(D.f_orders) || !in_text(D.f_tid) || !in_text(D.f_fid) ||
        !in_text(D.f_pspec) || !in_text(D.f_pform) || !in_text(D.f_gidx)) { dl_log("function outside code"); return 0; }
    if (!in_text(D.d_enable) || D.d_enable[0] != 0xBE) { dl_log("D target unexpected"); return 0; }
    if (!in_image(D.p_lock) || !in_image(D.p_cost) || !in_image(D.p_head) || !in_image(D.p_cnt) || !in_image(D.p_npool)) {
        dl_log("global outside image"); return 0;
    }
    bcopy_(D.orig_y, D.site_y, 5);
    bcopy_(D.orig_d, D.site_d, 7);
    log_rva("S (list opens)", D.site_s);
    log_rva("X (list closed)", D.site_x);
    log_rva("Y (learn call)", D.site_y);
    log_rva("P (general list opens)", D.site_p);
    log_rva("Q (general list closed)", D.site_q);
    log_rva("D (book grey-out)", D.site_d);
    log_rva("LearnTactic", D.f_learn);
    log_rva("valid", D.f_valid);
    log_rva("tactic slot", D.f_slot);
    log_rva("orders", D.f_orders);
    log_rva("tactic id", D.f_tid);
    log_rva("formation id", D.f_fid);
    log_rva("general special formation", D.f_pspec);
    log_rva("general has formation", D.f_pform);
    log_rva("book index", D.f_gidx);
    log_rva("data root", D.p_db);
    log_rva("orders cost", D.p_cost);
    log_rva("result head", D.p_head);
    log_rva("result count", D.p_cnt);
    resolve_fix();
    return 1;
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
static void make_patch(DL_Patch *p, U8 *site, const U8 *orig, U32 len, U8 *dest) {
    int32_t d = (int32_t)(dest - (site + 5));
    bfill_((U8 *)p, 0, sizeof *p);
    p->target = site;
    p->length = len;
    bcopy_(p->expected, orig, len);
    bfill_(p->replacement, 0x90, len);
    p->replacement[0] = 0xE9;
    bcopy_(p->replacement + 1, (const U8 *)&d, 4);
}
static void put_code(U8 *at, const U8 *bytes, U32 n) {
    UPTR a = (UPTR)at, q = a & ~(UPTR)7;
    if (((a + n - 1) & ~(UPTR)7) == q) {
        volatile long long *pq = (volatile long long *)q;
        long long old, nw;
        do {
            old = *pq;
            nw = old;
            bcopy_((U8 *)&nw + (a - q), bytes, n);
        } while (DL_CAS64(pq, old, nw) != old);
    } else {
        U32 i;
        for (i = n; i-- > 1;) ((volatile U8 *)at)[i] = bytes[i]; /* tail first, opcode last */
        ((volatile U8 *)at)[0] = bytes[0];
    }
}
static int self_install(DL_Patch *ps, U32 n, int restore) {
    unsigned long prot[NPATCH];
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
    U8 *b = D.block;
    if (!near_ok(D.site_s, b + DL_OFF_STUB_S) || !near_ok(D.site_x, b + DL_OFF_STUB_X) || !near_ok(D.site_y, b + DL_OFF_STUB_Y) ||
        !near_ok(D.site_p, b + DL_OFF_STUB_P) || !near_ok(D.site_q, b + DL_OFF_STUB_Q) || !near_ok(D.site_d, b + DL_OFF_STUB_D))
        return 0;
    make_patch(&D.patches[0], D.site_s, ORIG_S, 9, b + DL_OFF_STUB_S);
    make_patch(&D.patches[1], D.site_x, ORIG_X, 7, b + DL_OFF_STUB_X);
    make_patch(&D.patches[2], D.site_y, D.orig_y, 5, b + DL_OFF_STUB_Y);
    make_patch(&D.patches[3], D.site_p, ORIG_P, 10, b + DL_OFF_STUB_P);
    make_patch(&D.patches[4], D.site_q, ORIG_Q, 6, b + DL_OFF_STUB_Q);
    make_patch(&D.patches[5], D.site_d, D.orig_d, 7, b + DL_OFF_STUB_D);
    D.npatch = 6;
    if (D.site_r1 && D.site_r2 && D.site_r3 && near_ok(D.site_r1, b + DL_OFF_STUB_R1) && near_ok(D.site_r2, b + DL_OFF_STUB_R2) &&
        near_ok(D.site_r3, b + DL_OFF_STUB_R3) && D.site_r1[0] == 0xE8 && D.site_r2[0] == 0xE8 && D.site_r3[0] == 0xE8) {
        make_patch(&D.patches[6], D.site_r1, D.orig_r1, 5, b + DL_OFF_STUB_R1);
        make_patch(&D.patches[7], D.site_r2, D.orig_r2, 5, b + DL_OFF_STUB_R2);
        make_patch(&D.patches[8], D.site_r3, D.orig_r3, 5, b + DL_OFF_STUB_R3);
        D.npatch = 9;
    } else if (D.f_text) {
        D.f_text = 0;
        BQ(DL_OFF_F_FIX) = 0;
        dl_log("display fix off (sites already patched)");
    }
    if (!bsame_(D.site_s, ORIG_S, 9) || !bsame_(D.site_x, ORIG_X, 7) || D.site_y[0] != 0xE8 ||
        !bsame_(D.site_p, ORIG_P, 10) || !bsame_(D.site_q, ORIG_Q, 6) || D.site_d[0] != 0x83 || D.site_d[1] != 0x3D || D.site_d[6] != 0) {
        dl_log("a site is already patched (another mod?) - not installed");
        return 0;
    }
    if (D.host.patch_group) {
        if (!D.host.patch_group(GROUP_ID, D.patches, D.npatch)) { dl_log("host refused the patch group"); return 0; }
        D.self_patched = 0;
    } else {
        if (!self_install(D.patches, D.npatch, 0)) { dl_log("patch write failed"); return 0; }
        D.self_patched = 1;
    }
    return 1;
}

static unsigned long DL_WINAPI worker(void *unused) {
    int waited = 0;
    (void)unused;
    if (!find_text()) { D.state |= DL_STATE_FAIL_PATTERN; dl_log("no .text section"); goto done; }
    for (;;) { /* Steam unpacks the code after start: wait until one of our patterns is readable */
        if (D.stop) goto done;
        if (scan(D.text_lo, D.text_hi, &PS, 0) || scan(D.text_lo, D.text_hi, &PX, 0) || scan(D.text_lo, D.text_hi, &PY, 0)) break;
        if (waited >= D.cfg.wait_seconds * 1000) { D.state |= DL_STATE_FAIL_TIMEOUT; dl_log("game code not found (timeout)"); goto done; }
        Sleep(500);
        waited += 500;
    }
    if (!resolve()) { D.state |= DL_STATE_FAIL_PATTERN; dl_log("unsupported game build - feature off, game untouched"); goto done; }
    D.state |= DL_STATE_FOUND;
    D.block = alloc_near(D.site_s);
    if (!D.block) { D.state |= DL_STATE_FAIL_MEMORY; dl_log("no memory near the game image"); goto done; }
    bcopy_(D.block, DL_BLOCK, DL_BLOCK_SIZE);
    BD(DL_OFF_ON) = 1;
    BD(DL_OFF_MAX) = (U32)D.cfg.max_select;
    BQ(DL_OFF_F_VALID) = D.f_valid;
    BQ(DL_OFF_F_ORDERS) = D.f_orders;
    BQ(DL_OFF_F_SLOT) = D.f_slot;
    BQ(DL_OFF_F_LEARN) = D.f_learn;
    BQ(DL_OFF_F_TID) = D.f_tid;
    BQ(DL_OFF_F_FID) = D.f_fid;
    BQ(DL_OFF_F_PSPEC) = D.f_pspec;
    BQ(DL_OFF_F_PFORM) = D.f_pform;
    BQ(DL_OFF_F_GIDX) = D.f_gidx;
    BD(DL_OFF_PMAX) = (U32)D.cfg.max_generals;
    BQ(DL_OFF_P_DB) = D.p_db;
    BQ(DL_OFF_P_COST) = D.p_cost;
    BQ(DL_OFF_P_HEAD) = D.p_head;
    BQ(DL_OFF_P_CNT) = D.p_cnt;
    BQ(DL_OFF_P_NPOOL) = D.p_npool;
    BQ(DL_OFF_R_S) = D.site_s + 9;
    BQ(DL_OFF_R_X) = D.site_x + 7;
    BQ(DL_OFF_R_Y) = D.site_y + 5;
    BQ(DL_OFF_R_P) = D.site_p + 10;
    BQ(DL_OFF_R_Q) = D.site_q + 6;
    BQ(DL_OFF_R_D) = D.site_d + 7;
    BQ(DL_OFF_R_EN) = D.d_enable;
    BQ(DL_OFF_P_LOCK) = D.p_lock;
    D.pv_shown = D.bk_shown = D.lbl_shown = 0;
    if (D.f_text) {
        BQ(DL_OFF_F_R1) = rel_target(D.site_r1 + 1, 0);
        BQ(DL_OFF_F_R2) = rel_target(D.site_r2 + 1, 0);
        BQ(DL_OFF_R_R1) = D.site_r1 + 5;
        BQ(DL_OFF_R_R2) = D.site_r2 + 5;
        BQ(DL_OFF_F_R3) = rel_target(D.site_r3 + 1, 0);
        BQ(DL_OFF_R_R3) = D.site_r3 + 5;
        BQ(DL_OFF_F_FIX) = (void *)dl_fix;
    }
    FlushInstructionCache(GetCurrentProcess(), D.block, DL_BLOCK_SIZE);
    if (!install()) { D.state |= DL_STATE_FAIL_PATCH; goto done; }
    D.state |= DL_STATE_HOOKED;
    {
        Line l; l.n = 0; l.s[0] = 0;
        lput(&l, "DragLearn: ready (multi-select: up to ");
        ldec(&l, (U32)D.cfg.max_select); lput(&l, " books, up to ");
        ldec(&l, (U32)D.cfg.max_generals); lput(&l, " generals)");
        out(&l);
    }
done:
    D.worker_done = 1;
    return 0;
}

/* ------------------------------------------------------------------ API */
int DL_Start(const DL_Host *host, const DL_Config *cfg) {
    void *h;
    if (DL_CAS32(&D.started, 0, 1) != 0) return 1;
    bfill_((U8 *)&D.host, 0, sizeof D.host);
    bfill_((U8 *)&D.cfg, 0, sizeof D.cfg);
    D.self_patched = 0;
    if (host) bcopy_((U8 *)&D.host, (const U8 *)host, host->size < sizeof D.host ? host->size : sizeof D.host);
    D.host.size = sizeof D.host;
    if (cfg) bcopy_((U8 *)&D.cfg, (const U8 *)cfg, cfg->size < sizeof D.cfg ? cfg->size : sizeof D.cfg);
    D.cfg.size = sizeof D.cfg;
    if (D.cfg.max_select <= 0) D.cfg.max_select = 10;
    if (D.cfg.max_select < 2) D.cfg.max_select = 2;
    if (D.cfg.max_select > 16) D.cfg.max_select = 16;
    if (D.cfg.max_generals == 0) D.cfg.max_generals = 10;
    if (D.cfg.max_generals < 1) D.cfg.max_generals = 1;
    if (D.cfg.max_generals > 16) D.cfg.max_generals = 16;
    if (D.cfg.wait_seconds <= 0) D.cfg.wait_seconds = 300;
    D.stop = 0;
    D.worker_done = 0;
    D.state = DL_STATE_STARTED;
    h = CreateThread(0, 0, worker, 0, 0, 0);
    if (!h) { D.state = 0; D.started = 0; return 0; }
    CloseHandle(h);
    return 1;
}

void DL_Stop(void) {
    int i;
    if (!D.started) return;
    D.stop = 1;
    for (i = 0; i < 400 && !D.worker_done; i++) Sleep(10);
    if (D.block) BD(DL_OFF_ON) = 0;
    if (D.self_patched && self_install(D.patches, D.npatch, 1)) {
        D.self_patched = 0;
        D.state &= ~DL_STATE_HOOKED;
        dl_log("stopped, original bytes restored");
    }
    D.state |= DL_STATE_STOPPED;
    D.state &= ~DL_STATE_STARTED;
    D.started = 0;
}

unsigned int DL_GetState(void) { return D.state; }

unsigned int DL_GetCounter(int which) {
    static const U32 off[17] = {DL_OFF_OPENS, DL_OFF_MULTI, DL_OFF_XHITS, DL_OFF_LEARNED, DL_OFF_STOPS,
                                DL_OFF_FAILS, DL_OFF_LIMIT, DL_OFF_ORDERS, DL_OFF_FILLED, DL_OFF_SKIPS,
                                DL_OFF_POPENS, DL_OFF_PMULTI, DL_OFF_QHITS, DL_OFF_PLIMIT, DL_OFF_NPERS,
                                DL_OFF_UNHID, DL_OFF_FSKIP};
    if (which < 0 || which > 16 || !D.block) return 0;
    return BD(off[which]);
}

void DL_SetEnabled(int on) { if (D.block) BD(DL_OFF_ON) = on ? 1u : 0u; }
