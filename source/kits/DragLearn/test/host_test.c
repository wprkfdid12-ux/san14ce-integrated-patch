/*
 * Linux host test for src/drag_learn.c and the machine-code block (dl_block.S).
 *
 * Part 1 (always): the real stubs S / X / Y / P / Q are executed through small fake game callers,
 *   against mock game functions (Windows x64 ABI, stack alignment checked on every call):
 *   list limits, replace mode, result chain reading, batch learning, guards.
 * Part 2 (with an unpacked game image): the real worker runs on the image mapped at
 *   0x140000000 with mocked Win32: pattern search, function/global resolution, patch bytes,
 *   restore on stop, refusal when a site is already patched, host patch_group path.
 *
 * usage: host_test [image.bin S X Y learn valid slot orders db cost head cnt npool P Q tid fid pspec pform gidx D enable lock] (hex RVAs)
 * (the image is made from the user's own exe for this test only; it is never shipped)
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
#include "../src/drag_learn.h"
#include "../src/dl_block.h"

#define MS __attribute__((ms_abi))
static int fails, checks;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; printf("  FAIL: %s (line %d)\n", msg, __LINE__); } else printf("  ok: %s\n", msg); } while (0)

/* ------------------------------------------------------------------ Win32 mocks (part 2) */
static uint8_t *IMG;
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
int VirtualProtect(void *a, size_t n, unsigned long p, unsigned long *old) { (void)a; (void)n; (void)p; *old = 0x20; return 1; }
int FlushInstructionCache(void *p, const void *a, size_t n) { (void)p; (void)a; (void)n; return 1; }
void *GetCurrentProcess(void) { return (void *)(intptr_t)-1; }
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

/* ------------------------------------------------------------------ mock game world (part 1) */
#define EMPTY 200 /* tactic id of an empty slot -> table entry 200 = invalid */
#define MAGIC 0x4F4B4F4Bu
typedef struct { uint32_t magic; } Obj;
typedef struct { uint32_t magic; uint8_t pad[0x68]; uint32_t formmask; } Tac;           /* +6C formation bits */
typedef struct { uint32_t magic; uint8_t pad[0x7A]; uint8_t kind; uint8_t pad2; int id; } Form; /* +7E kind */
static uint8_t *g_db;                       /* data root */
static uint8_t *g_dbvar[2];                 /* the "global" that holds the data root */
static int g_cost = 1, g_cost_var;
typedef struct { uint8_t pad[0x14]; uint8_t orders; } OrdersObj;
static OrdersObj g_orders;
typedef struct { uint8_t pad[0x130]; uint8_t slot[10]; uint8_t pad2[6]; uint32_t forms; int special; } Person;
static Person g_person, g_person2, g_pB, g_pC;
typedef struct { void *vt; uint32_t magic; uint8_t pad[0x52 - 12]; uint8_t tactic; } Gem;
static void *VT_GEM = (void *)0x11110000, *VT_OTHER = (void *)0x22220000;
static Gem g_gem[8];
static Tac g_tactic[201];
static Form g_form[21];
static struct { uint32_t magic; uint8_t pad[0x11C]; uint16_t count[32]; } g_force;   /* +120 + idx*2 */
static Obj g_window_sub;
static uint8_t g_window[0x500];
typedef struct { void *person, *gem, *replace, *force; } Cmd;
static int g_learn_calls, g_learn_gem[64];
static void *g_learn_person[64], *g_learn_repl[64];
static unsigned g_misaligned;

static int MS m_valid(void *o) {
    if (!o) return 0;
    if ((Gem *)o >= g_gem && (Gem *)o < g_gem + 8) return ((Gem *)o)->magic == MAGIC; /* books: magic after the vtable */
    return ((Obj *)o)->magic == MAGIC;
}
static void *MS m_orders(void *force) { return force == (void *)&g_force ? &g_orders : 0; }
static int MS m_slot(void *person, int i) { return ((Person *)person)->slot[i]; }
static int MS m_tid(void *t) { return (int)((Tac *)t - g_tactic); }
static int MS m_fid(void *f) { return ((Form *)f)->id; }
static int MS m_pspec(void *p) { return ((Person *)p)->special; }
static int MS m_pform(void *p, int id) { return (((Person *)p)->forms >> id) & 1; }
static int MS m_gidx(void *g) { return (int)((Gem *)g - g_gem) + 1; }
static int MS m_learn(Cmd *c) {
    Person *p = c->person;
    Gem *g = c->gem;
    int i, k;
    if (!g || g->magic != MAGIC || !p) return 0;
    if (g_learn_calls < 64) { g_learn_gem[g_learn_calls] = (int)(g - g_gem); g_learn_person[g_learn_calls] = p; g_learn_repl[g_learn_calls] = c->replace; }
    g_learn_calls++;
    k = m_gidx(g);
    if (g_force.count[k]) g_force.count[k]--;                                             /* game: clamp at 0 */
    g_orders.orders = g_orders.orders >= g_cost ? (uint8_t)(g_orders.orders - g_cost) : 0;
    if (m_valid(c->replace))
        for (i = 0; i < 10; i++) if (p->slot[i] == m_tid(c->replace)) { p->slot[i] = EMPTY; break; }
    for (i = 0; i < 10; i++) if (p->slot[i] == EMPTY) { p->slot[i] = g->tactic; break; }
    return 1;
}

/* thunk: count misaligned calls, scribble over the callee's home area (the game's functions
   store registers there), then jump to the mock */
static uint8_t *thunk(uint8_t *at, void *target) {
    uint8_t *p = at;
    static const uint8_t pre[] = {0x48, 0x8D, 0x44, 0x24, 0x08, 0xA8, 0x0F, 0x74, 0x0D}; /* lea rax,[rsp+8]; test al,15; jz +13 */
    static const uint8_t home[] = {0x49, 0xBB, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE,  /* mov r11,DEADBEEFDEADBEEF */
                                   0x4C, 0x89, 0x5C, 0x24, 0x08, 0x4C, 0x89, 0x5C, 0x24, 0x10,  /* mov [rsp+8],r11; [rsp+10] */
                                   0x4C, 0x89, 0x5C, 0x24, 0x18, 0x4C, 0x89, 0x5C, 0x24, 0x20}; /* [rsp+18]; [rsp+20] */
    memcpy(p, pre, sizeof pre); p += sizeof pre;
    *p++ = 0x49; *p++ = 0xBB; { uint64_t a = (uint64_t)&g_misaligned; memcpy(p, &a, 8); p += 8; } /* mov r11,&cnt */
    *p++ = 0x41; *p++ = 0xFF; *p++ = 0x03;                                                        /* inc dword [r11] */
    memcpy(p, home, sizeof home); p += sizeof home;
    *p++ = 0x49; *p++ = 0xBB; { uint64_t a = (uint64_t)target; memcpy(p, &a, 8); p += 8; }       /* mov r11,target */
    *p++ = 0x41; *p++ = 0xFF; *p++ = 0xE3;                                                        /* jmp r11 */
    return at;
}

/* fake callers ------------------------------------------------------------ */
typedef void(MS *FakeS)(void *window, void *frame_rbp);
typedef void *(MS *FakeX)(void *window, void *handle_ptr);
typedef int(MS *FakeY)(Cmd *cmd);
typedef void(MS *FakeP)(void *window, int *out4);
typedef int(MS *FakeQ)(void *handle_ptr);
typedef int(MS *FakeD)(void *window, void *book);
static uint8_t *emit_abs_jmp(uint8_t *p, void *to) { uint64_t a = (uint64_t)to; *p++ = 0x48; *p++ = 0xB8; memcpy(p, &a, 8); p += 8; *p++ = 0xFF; *p++ = 0xE0; return p; }
static uint8_t *align16(uint8_t *p) { return (uint8_t *)(((uintptr_t)p + 15) & ~(uintptr_t)15); }

uint64_t g_sent[7];
static uint8_t *BLK;
static FakeS fS; static FakeX fX; static FakeY fY; static FakeP fP; static FakeQ fQ; static FakeD fD;
static int g_lock;
static void build_fakes(uint8_t *code) {
    uint8_t *p = code;
    /* S: push rbp; push r13; push rdi; sub rsp,20; mov r13,rcx; mov rbp,rdx; mov edi,1; mov [rbp-48],edi; jmp stub
          back: mov [rbp-50],ecx; add rsp,20; pop rdi; pop r13; pop rbp; ret */
    static const uint8_t s0[] = {0x55, 0x41, 0x55, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x49, 0x89, 0xCD, 0x48, 0x89, 0xD5,
                                 0xBF, 0x01, 0x00, 0x00, 0x00, 0x89, 0x7D, 0xB8};
    static const uint8_t s1[] = {0x89, 0x4D, 0xB0, 0x48, 0x83, 0xC4, 0x20, 0x5F, 0x41, 0x5D, 0x5D, 0xC3};
    /* X: push r13; sub rsp,40; mov r13,rcx; mov [rsp+38],rdx; jmp stub   back: mov rax,r8; add rsp,40; pop r13; ret */
    static const uint8_t x0[] = {0x41, 0x55, 0x48, 0x83, 0xEC, 0x40, 0x49, 0x89, 0xCD, 0x48, 0x89, 0x54, 0x24, 0x38};
    static const uint8_t x1[] = {0x4C, 0x89, 0xC0, 0x48, 0x83, 0xC4, 0x40, 0x41, 0x5D, 0xC3};
    /* Y: push rsi rdi r12 r13 rbx r14 r15; sub rsp,20 (7 pushes: 8+56=64, +32 -> aligned); sentinels; jmp stub
          back: store the 7 registers to g_sent area, restore, ret */
    static const uint8_t y0[] = {0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x53, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x20,
                                 0x48, 0xBE, 1, 0, 0, 0, 0, 0, 0, 0x5A,  /* rsi */
                                 0x48, 0xBF, 2, 0, 0, 0, 0, 0, 0, 0x5A,  /* rdi */
                                 0x49, 0xBC, 3, 0, 0, 0, 0, 0, 0, 0x5A,  /* r12 */
                                 0x49, 0xBD, 4, 0, 0, 0, 0, 0, 0, 0x5A,  /* r13 */
                                 0x48, 0xBB, 5, 0, 0, 0, 0, 0, 0, 0x5A,  /* rbx */
                                 0x49, 0xBE, 6, 0, 0, 0, 0, 0, 0, 0x5A,  /* r14 */
                                 0x49, 0xBF, 7, 0, 0, 0, 0, 0, 0, 0x5A}; /* r15 */
    /* P: push rbx; push rsi; sub rsp,78; mov rbx,rcx; mov esi,1; mov [rsp+58],esi; mov [rsp+70],rdx; jmp stub
          back: rdx=[rsp+70]; out = mode,max,min,ecx; add rsp,78; pop rsi; pop rbx; ret */
    static const uint8_t p0[] = {0x53, 0x56, 0x48, 0x83, 0xEC, 0x78, 0x48, 0x89, 0xCB, 0xBE, 0x01, 0x00, 0x00, 0x00,
                                 0x89, 0x74, 0x24, 0x58, 0x48, 0x89, 0x54, 0x24, 0x70};
    static const uint8_t p1[] = {0x48, 0x8B, 0x54, 0x24, 0x70, 0x8B, 0x44, 0x24, 0x58, 0x89, 0x02, 0x8B, 0x44, 0x24, 0x5C, 0x89, 0x42, 0x04,
                                 0x8B, 0x44, 0x24, 0x60, 0x89, 0x42, 0x08, 0x89, 0x4A, 0x0C, 0x48, 0x83, 0xC4, 0x78, 0x5E, 0x5B, 0xC3};
    /* Q: push r14; sub rsp,30; mov [rsp+28],rcx; jmp stub   back: mov eax,r14d; add rsp,30; pop r14; ret */
    static const uint8_t q0[] = {0x41, 0x56, 0x48, 0x83, 0xEC, 0x30, 0x48, 0x89, 0x4C, 0x24, 0x28};
    static const uint8_t q1[] = {0x44, 0x89, 0xF0, 0x48, 0x83, 0xC4, 0x30, 0x41, 0x5E, 0xC3};
    uint8_t *back;
    uint64_t sent_addr = (uint64_t)g_sent;

    fS = (FakeS)p; memcpy(p, s0, sizeof s0); p += sizeof s0; p = emit_abs_jmp(p, BLK + DL_OFF_STUB_S);
    *(void **)(BLK + DL_OFF_R_S) = p; memcpy(p, s1, sizeof s1); p += sizeof s1;
    p = align16(p);
    fX = (FakeX)p; memcpy(p, x0, sizeof x0); p += sizeof x0; p = emit_abs_jmp(p, BLK + DL_OFF_STUB_X);
    *(void **)(BLK + DL_OFF_R_X) = p; memcpy(p, x1, sizeof x1); p += sizeof x1;
    p = align16(p);
    fY = (FakeY)p; memcpy(p, y0, sizeof y0); p += sizeof y0; p = emit_abs_jmp(p, BLK + DL_OFF_STUB_Y);
    back = p;
    *p++ = 0x49; *p++ = 0xBB; memcpy(p, &sent_addr, 8); p += 8;   /* mov r11,&g_sent (7 qwords) */
    { static const uint8_t st[] = {0x49, 0x89, 0x33, 0x49, 0x89, 0x7B, 0x08, 0x4D, 0x89, 0x63, 0x10, 0x4D, 0x89, 0x6B, 0x18,
                                   0x49, 0x89, 0x5B, 0x20, 0x4D, 0x89, 0x73, 0x28, 0x4D, 0x89, 0x7B, 0x30,
                                   0x48, 0x83, 0xC4, 0x20, 0x41, 0x5F, 0x41, 0x5E, 0x5B, 0x41, 0x5D, 0x41, 0x5C, 0x5F, 0x5E, 0xC3};
      memcpy(p, st, sizeof st); p += sizeof st; }
    *(void **)(BLK + DL_OFF_R_Y) = back;
    p = align16(p);
    fP = (FakeP)p; memcpy(p, p0, sizeof p0); p += sizeof p0; p = emit_abs_jmp(p, BLK + DL_OFF_STUB_P);
    *(void **)(BLK + DL_OFF_R_P) = p; memcpy(p, p1, sizeof p1); p += sizeof p1;
    p = align16(p);
    fQ = (FakeQ)p; memcpy(p, q0, sizeof q0); p += sizeof q0; p = emit_abs_jmp(p, BLK + DL_OFF_STUB_Q);
    *(void **)(BLK + DL_OFF_R_Q) = p; memcpy(p, q1, sizeof q1); p += sizeof q1;
    p = align16(p);
    /* D: push r12; push r13; sub rsp,28; mov r13,rcx; mov r12,rdx; jmp stub
          back (original je): sete al; movzx eax,al; epilogue   back (selectable): mov eax,2; epilogue */
    { static const uint8_t d0[] = {0x41, 0x54, 0x41, 0x55, 0x48, 0x83, 0xEC, 0x28, 0x49, 0x89, 0xCD, 0x49, 0x89, 0xD4};
      static const uint8_t ep[] = {0x48, 0x83, 0xC4, 0x28, 0x41, 0x5D, 0x41, 0x5C, 0xC3};
      fD = (FakeD)p; memcpy(p, d0, sizeof d0); p += sizeof d0; p = emit_abs_jmp(p, BLK + DL_OFF_STUB_D);
      *(void **)(BLK + DL_OFF_R_D) = p; *p++ = 0x0F; *p++ = 0x94; *p++ = 0xC0; *p++ = 0x0F; *p++ = 0xB6; *p++ = 0xC0;
      memcpy(p, ep, sizeof ep); p += sizeof ep;
      *(void **)(BLK + DL_OFF_R_EN) = p; *p++ = 0xB8; *p++ = 2; *p++ = 0; *p++ = 0; *p++ = 0;
      memcpy(p, ep, sizeof ep); p += sizeof ep; }
}
static uint64_t g_sent7[7];

/* list result tables (game globals) */
static void **g_headtab, **g_cnttab;
static void *g_headvar, *g_cntvar;
static int g_npool = 64;
typedef struct Node { void *obj; struct Node *next; } Node;
static Node g_nodes[2][16];
static uint32_t g_handle[2] = {7, 9};

static void set_result(int which, int n, void *const *objs) {
    int i;
    Node *nd = g_nodes[which];
    for (i = 0; i < n; i++) { nd[i].obj = objs[i]; nd[i].next = i + 1 < n ? &nd[i + 1] : 0; }
    g_headtab[g_handle[which]] = n ? &nd[0] : 0;
    g_cnttab[g_handle[which]] = (void *)(uintptr_t)n;
}
static void reset_person(Person *p, int used) {
    int i;
    for (i = 0; i < 10; i++) p->slot[i] = (uint8_t)(i < used ? 100 + i : EMPTY);
    p->forms = 0xFFFFFFFFu; p->special = -1;
}
static int used(Person *p) { int i, n = 0; for (i = 0; i < 10; i++) n += p->slot[i] != EMPTY; return n; }
static int has(Person *p, int t) { int i; for (i = 0; i < 10; i++) if (p->slot[i] == t) return 1; return 0; }
static void books(int n) { int i; for (i = 0; i < 8; i++) g_force.count[i + 1] = (uint16_t)n; }

typedef struct { int mode, max, min, ecx; } ListParams;
static ListParams open_books(void) {
    uint8_t frame[0x100];
    uint8_t *rbp = frame + 0x80;
    ListParams r;
    memset(frame, 0xCC, sizeof frame);
    fS(g_window, rbp);
    r.mode = *(int *)(rbp - 0x48); r.max = *(int *)(rbp - 0x44); r.min = *(int *)(rbp - 0x40); r.ecx = *(int *)(rbp - 0x50);
    return r;
}
static ListParams open_generals(void) { ListParams r; fP(g_window, (int *)&r); return r; }
static int pick_generals(int n, void *const *ps) {
    *(void **)(g_window + 0x488) = n ? ps[0] : *(void **)(g_window + 0x488);
    set_result(1, n, ps);
    return fQ(&g_handle[1]);
}
static void pick_books(int n, const int *idx) {
    void *objs[16];
    int i;
    for (i = 0; i < n; i++) objs[i] = &g_gem[idx[i]];
    set_result(0, n, objs);
    if (fX(g_window, &g_handle[0]) != &g_window_sub) printf("  (X lost r8)\n");
}
/* [예]: the game builds the command from the window and calls LearnTactic (= stub Y) */
static int confirm(int replace) {
    Cmd c;
    int r;
    c.person = *(void **)(g_window + 0x488);
    c.gem = *(void **)(g_window + 0x480);
    c.replace = replace ? (void *)&g_tactic[100] : 0;
    c.force = &g_force;
    r = fY(&c);
    memcpy(g_sent7, g_sent, sizeof g_sent);
    if (c.person != *(void **)(g_window + 0x488) || c.gem != *(void **)(g_window + 0x480) || c.replace != (replace ? (void *)&g_tactic[100] : 0)) return -2;
    return r;
}
static int sentinels_ok(void) {
    int i;
    for (i = 0; i < 7; i++) if (g_sent[i] != 0x5A00000000000000ull + (uint64_t)(i + 1)) return 0;
    return 1;
}
static void learn_books(int n, const int *idx) { pick_books(n, idx); *(void **)(g_window + 0x480) = &g_gem[idx[0]]; }
#define U32AT(o) (*(volatile uint32_t *)(BLK + (o)))

static void stub_tests(void) {
    uint8_t *m = mmap(0, 0x5000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t *code = m + 0x2000, *th = m + 0x4000;
    int i, r;
    printf("== block stubs S / X / Y / P / Q with mock game functions\n");
    BLK = m;
    memcpy(BLK, DL_BLOCK, DL_BLOCK_SIZE);
    for (i = 1; i < 200; i++) { g_tactic[i].magic = MAGIC; g_tactic[i].formmask = 1u << 3; } /* every tactic needs formation 3 */
    g_force.magic = MAGIC;
    g_db = calloc(1, 0x774D0 + 201 * 8);
    for (i = 0; i <= 200; i++) *(void **)(g_db + 0x774D0 + i * 8) = (i >= 1 && i < 200) ? (void *)&g_tactic[i] : 0;
    for (i = 0; i < 21; i++) { g_form[i].magic = MAGIC; g_form[i].id = i; g_form[i].kind = (uint8_t)(i == 7 ? 2 : 1); *(void **)(g_db + 0x77428 + i * 8) = &g_form[i]; }
    g_dbvar[0] = g_db;
    g_cost_var = g_cost;
    for (i = 0; i < 8; i++) { g_gem[i].vt = VT_GEM; g_gem[i].magic = MAGIC; g_gem[i].tactic = (uint8_t)(10 + i); }
    books(3);
    g_headtab = calloc(64, 8); g_cnttab = calloc(64, 8);
    g_headvar = g_headtab; g_cntvar = g_cnttab;
    *(void **)(g_window + 0x478) = &g_force;
    *(void **)(g_window + 0x488) = &g_person;
    *(void **)(g_window + 0x490) = 0;
    *(void **)(g_window + 0x4B8) = &g_window_sub;

    *(void **)(BLK + DL_OFF_F_VALID) = thunk(th + 0x000, (void *)m_valid);
    *(void **)(BLK + DL_OFF_F_ORDERS) = thunk(th + 0x080, (void *)m_orders);
    *(void **)(BLK + DL_OFF_F_SLOT) = thunk(th + 0x100, (void *)m_slot);
    *(void **)(BLK + DL_OFF_F_LEARN) = thunk(th + 0x180, (void *)m_learn);
    *(void **)(BLK + DL_OFF_F_TID) = thunk(th + 0x200, (void *)m_tid);
    *(void **)(BLK + DL_OFF_F_FID) = thunk(th + 0x280, (void *)m_fid);
    *(void **)(BLK + DL_OFF_F_PSPEC) = thunk(th + 0x300, (void *)m_pspec);
    *(void **)(BLK + DL_OFF_F_PFORM) = thunk(th + 0x380, (void *)m_pform);
    *(void **)(BLK + DL_OFF_F_GIDX) = thunk(th + 0x400, (void *)m_gidx);
    *(void **)(BLK + DL_OFF_P_DB) = g_dbvar;
    *(void **)(BLK + DL_OFF_P_COST) = &g_cost_var;
    *(void **)(BLK + DL_OFF_P_HEAD) = &g_headvar;
    *(void **)(BLK + DL_OFF_P_CNT) = &g_cntvar;
    *(void **)(BLK + DL_OFF_P_NPOOL) = &g_npool;
    *(void **)(BLK + DL_OFF_P_LOCK) = &g_lock;
    build_fakes(code);

    /* ================= 1안: one general, several books ================= */
    {
        ListParams lp;
        reset_person(&g_person, 7); g_orders.orders = 12;
        lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 3 && lp.min == 0, "S: 7 slots used, 12 orders -> multi, max 3 (free slots)");
        CHECK(lp.ecx == 0x5BC, "S: replaced 'mov ecx,5BC' still done");
        reset_person(&g_person, 1); g_orders.orders = 2;
        lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 2, "S: 2 orders, 9 free -> max 2 (orders)");
        reset_person(&g_person, 0); g_orders.orders = 50;
        lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 10, "S: plenty -> max 10 (cap)");
        U32AT(DL_OFF_MAX) = 16; lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 10, "S: cap 16 but only 10 slots -> 10");
        U32AT(DL_OFF_MAX) = 10;
        g_orders.orders = 1; lp = open_books();
        CHECK(lp.mode == 1 && lp.max == 1, "S: 1 order -> original single list");
        g_orders.orders = 0; lp = open_books();
        CHECK(lp.mode == 1 && lp.max == 1, "S: 0 orders -> original single list (game refuses by itself)");
        reset_person(&g_person, 9); g_orders.orders = 9; lp = open_books();
        CHECK(lp.mode == 1, "S: 1 free slot -> original single list");
        reset_person(&g_person, 10); lp = open_books();
        CHECK(lp.mode == 1, "S: 10 slots used -> original single list");
        *(void **)(g_window + 0x490) = &g_tactic[100]; lp = open_books();
        CHECK(lp.mode == 1 && U32AT(DL_OFF_LIMIT) == 1, "S: replace mode -> original single list");
        *(void **)(g_window + 0x490) = 0;
        g_cost_var = 2; reset_person(&g_person, 0); g_orders.orders = 7; lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 3, "S: cost 2, 7 orders -> max 3");
        g_cost_var = 1;
        U32AT(DL_OFF_ON) = 0; lp = open_books();
        CHECK(lp.mode == 1 && lp.max == 1, "S: feature off -> original");
        U32AT(DL_OFF_ON) = 1;
        g_dbvar[0] = 0; g_orders.orders = 9; lp = open_books();
        CHECK(lp.mode == 1, "S: no data root -> treated as full -> single");
        g_dbvar[0] = g_db;
    }
    {
        int sel3[3] = {0, 1, 2}, sel5[5] = {0, 1, 2, 3, 4};
        reset_person(&g_person, 7); g_orders.orders = 12; books(3); g_learn_calls = 0; U32AT(DL_OFF_LEARNED) = 0;
        open_books(); learn_books(3, sel3);
        r = confirm(0);
        CHECK(r == 1, "Y: returns the first learn's result, command restored");
        CHECK(sentinels_ok(), "Y: rbx rsi rdi r12-r15 preserved even though callees overwrite their home area");
        CHECK(U32AT(DL_OFF_SELN) == 0, "Y: remembered books cleared afterwards");
        CHECK(g_learn_calls == 3 && g_learn_gem[0] == 0 && g_learn_gem[1] == 1 && g_learn_gem[2] == 2, "3 books learned in selection order");
        CHECK(used(&g_person) == 10 && has(&g_person, 10) && has(&g_person, 11) && has(&g_person, 12), "general got the 3 tactics");
        CHECK(g_orders.orders == 9 && g_force.count[1] == 2 && g_force.count[3] == 2, "orders 12 -> 9, each book -1");

        reset_person(&g_person, 7); g_orders.orders = 12; books(3); g_learn_calls = 0; U32AT(DL_OFF_STOPS) = 0;
        learn_books(5, sel5); confirm(0);
        CHECK(g_learn_calls == 3 && used(&g_person) == 10, "slot guard: 5 selected, 3 free -> 3 learned");
        CHECK(g_force.count[4] == 3 && g_force.count[5] == 3 && g_orders.orders == 9, "slot guard: books 4-5 and their orders not spent");

        reset_person(&g_person, 0); g_orders.orders = 2; books(3); g_learn_calls = 0;
        learn_books(5, sel5); confirm(0);
        CHECK(g_learn_calls == 2 && g_orders.orders == 0 && used(&g_person) == 2, "orders guard: 2 orders -> 2 learned, never below 0");

        reset_person(&g_person, 10); g_orders.orders = 9; g_learn_calls = 0;
        learn_books(3, sel3); confirm(1);
        CHECK(g_learn_calls == 1, "replace mode, one general: no extra book");

        reset_person(&g_person, 0); g_orders.orders = 9; g_learn_calls = 0;
        learn_books(1, sel3); confirm(0);
        CHECK(g_learn_calls == 1, "one book, one general: only the game's own learn");

        reset_person(&g_person, 0); g_orders.orders = 9; g_learn_calls = 0;
        pick_books(3, sel3); *(void **)(g_window + 0x480) = &g_gem[5]; confirm(0);
        CHECK(g_learn_calls == 1, "stale selection (different first book): no batch");
        *(void **)(g_window + 0x480) = &g_gem[0];

        g_gem[1].vt = VT_OTHER;
        reset_person(&g_person, 0); g_orders.orders = 9; g_learn_calls = 0;
        learn_books(3, sel3); confirm(0);
        CHECK(g_learn_calls == 2 && g_learn_gem[1] == 2, "object with another vtable skipped");
        g_gem[1].vt = VT_GEM;

        g_force.count[2] = 0;
        reset_person(&g_person, 0); g_orders.orders = 9; g_learn_calls = 0;
        learn_books(3, sel3); confirm(0);
        CHECK(g_learn_calls == 2 && !has(&g_person, 11), "a book with count 0 is skipped (game would learn it for free)");
        books(3);

        g_npool = 5;
        reset_person(&g_person, 0); g_orders.orders = 9; g_learn_calls = 0;
        learn_books(3, sel3); confirm(0);
        CHECK(g_learn_calls == 1, "result handle out of range -> nothing remembered");
        g_npool = 64;
    }

    /* ================= 2안: several generals ================= */
    {
        ListParams lp;
        void *abc[3] = {&g_person, &g_pB, &g_pC};
        int one[1] = {0}, two[2] = {0, 1};
        g_orders.orders = 12;
        lp = open_generals();
        CHECK(lp.mode == 2 && lp.max == 10 && lp.min == 0 && lp.ecx == 0x32E, "P: 12 orders -> general list multi, max 10 (cap)");
        g_orders.orders = 3; lp = open_generals();
        CHECK(lp.mode == 2 && lp.max == 3, "P: 3 orders -> max 3 generals");
        g_orders.orders = 1; lp = open_generals();
        CHECK(lp.mode == 1 && lp.max == 1 && lp.min == 0, "P: 1 order -> original single list");
        U32AT(DL_OFF_PMAX) = 1; g_orders.orders = 9; lp = open_generals();
        CHECK(lp.mode == 1 && lp.max == 1, "P: MaxGenerals=1 -> original single list");
        U32AT(DL_OFF_PMAX) = 10;
        U32AT(DL_OFF_ON) = 0; lp = open_generals();
        CHECK(lp.mode == 1, "P: feature off -> original");
        U32AT(DL_OFF_ON) = 1;

        r = pick_generals(3, abc);
        CHECK(r == 0x14000 && U32AT(DL_OFF_NPERS) == 3 && *(void **)(BLK + DL_OFF_PERSONS) == &g_person &&
              *(void **)(BLK + DL_OFF_PERSONS + 16) == &g_pC, "Q: 3 generals remembered in order, r14d still set");
        r = pick_generals(0, abc);
        CHECK(U32AT(DL_OFF_NPERS) == 3, "Q: cancelled list keeps the previous generals");

        /* book list limit with 3 generals: 12 orders / 3 = 4, first general has 3 free -> 3 */
        reset_person(&g_person, 7); reset_person(&g_pB, 9); reset_person(&g_pC, 8); g_orders.orders = 12;
        lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 3, "S with 3 generals: min(12/3 orders, most free general 3) = 3");
        reset_person(&g_person, 0); g_orders.orders = 6;
        lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 2, "S with 3 generals: 6 orders -> 2 books each");

        /* one book -> three generals */
        reset_person(&g_person, 2); reset_person(&g_pB, 5); reset_person(&g_pC, 0);
        g_orders.orders = 12; books(5); g_learn_calls = 0; U32AT(DL_OFF_LEARNED) = 0; U32AT(DL_OFF_SKIPS) = 0;
        learn_books(1, one);
        r = confirm(0);
        CHECK(r == 1 && sentinels_ok(), "2안 Y: result + all registers preserved");
        CHECK(g_learn_calls == 3 && g_learn_person[0] == &g_person && g_learn_person[1] == &g_pB && g_learn_person[2] == &g_pC,
              "one book -> learned by the 3 generals in order");
        CHECK(has(&g_person, 10) && has(&g_pB, 10) && has(&g_pC, 10), "all three have the tactic");
        CHECK(g_force.count[1] == 2 && g_orders.orders == 9, "3 books and 3 orders spent");
        CHECK(g_learn_repl[1] == 0 && g_learn_repl[2] == 0, "extra learns pass no replace target");
        CHECK(U32AT(DL_OFF_NPERS) == 3, "generals kept for 연속입력");

        /* 연속입력: same generals, next book; B already... none have tactic 11 */
        g_learn_calls = 0;
        *(void **)(g_window + 0x480) = &g_gem[1]; U32AT(DL_OFF_SELN) = 0;
        confirm(0);
        CHECK(g_learn_calls == 3 && has(&g_pC, 11), "연속입력: next single book again for all 3");

        /* 2 books x 3 generals with eligibility: C already knows book 0's tactic, B lacks the formation for book 1 */
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_pC.slot[0] = 10;                          /* C knows tactic of book 0 */
        g_tactic[11].formmask = 1u << 4;            /* book 1 needs formation 4 */
        g_pB.forms = ~(1u << 4);                    /* B lacks it */
        g_orders.orders = 20; books(5); g_learn_calls = 0; U32AT(DL_OFF_SKIPS) = 0;
        learn_books(2, two);
        confirm(0);
        CHECK(g_learn_calls == 4, "2 books x 3 generals: 6 pairs, 2 not allowed -> 4 learns");
        CHECK(has(&g_person, 10) && has(&g_person, 11) && has(&g_pB, 10) && !has(&g_pB, 11) && has(&g_pC, 11), "right pairs learned");
        CHECK(U32AT(DL_OFF_SKIPS) == 2, "2 pairs skipped (already learned / no formation)");
        g_tactic[11].formmask = 1u << 3;

        /* special formation (kind 2): allowed only if special id matches */
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_tactic[10].formmask = 1u << 7;            /* book 0 needs formation 7 (kind 2) */
        g_pB.special = 7; g_pC.special = 2;
        g_orders.orders = 20; books(5); g_learn_calls = 0;
        learn_books(1, one); confirm(0);
        CHECK(g_learn_calls == 1 && has(&g_pB, 10) && !has(&g_pC, 10) && !has(&g_person, 10),
              "special formation rule: B (special 7) yes, C and the first general (no special 7) no");
        g_tactic[10].formmask = 1u << 3;

        /* books run out */
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_orders.orders = 20; books(5); g_force.count[1] = 2; g_learn_calls = 0;
        learn_books(1, one); confirm(0);
        CHECK(g_learn_calls == 2 && !has(&g_pC, 10) && g_force.count[1] == 0, "2 copies of the book -> only 2 generals learn");

        /* orders run out */
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_orders.orders = 2; books(5); g_learn_calls = 0;
        learn_books(1, one); confirm(0);
        CHECK(g_learn_calls == 2 && g_orders.orders == 0, "2 orders -> 2 generals, then stop");

        /* a general with full slots is passed over, the next one still learns */
        reset_person(&g_person, 0); reset_person(&g_pB, 10); reset_person(&g_pC, 0);
        g_orders.orders = 20; books(5); g_learn_calls = 0;
        learn_books(1, one); confirm(0);
        CHECK(g_learn_calls == 2 && has(&g_pC, 10) && used(&g_pB) == 10, "full general skipped, next general learns");

        /* replace mode for the first general: others still learn normally */
        reset_person(&g_person, 10); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_orders.orders = 20; books(5); g_learn_calls = 0;
        learn_books(1, one); confirm(1);
        CHECK(g_learn_calls == 3 && g_learn_repl[0] != 0 && g_learn_repl[1] == 0 && has(&g_pB, 10), "replace for the first, normal learn for the others");

        /* window shows another general than the first selected -> only that general */
        *(void **)(g_window + 0x488) = &g_person2; reset_person(&g_person2, 0);
        g_orders.orders = 20; books(5); g_learn_calls = 0;
        learn_books(1, one); confirm(0);
        CHECK(g_learn_calls == 1, "window general != first selected -> no batch over generals");
        *(void **)(g_window + 0x488) = &g_person;

        /* duplicates / null in the list */
        { void *dup[3] = {&g_person, &g_person, 0};
          pick_generals(3, dup); reset_person(&g_person, 0); g_orders.orders = 20; g_learn_calls = 0;
          learn_books(1, one); confirm(0);
          CHECK(g_learn_calls == 1, "duplicate / empty entries ignored"); }
    }
    /* ================= 2안 fixes: grey-out for several generals, first general skipped ================= */
    {
        void *abc[3] = {&g_person, &g_pB, &g_pC};
        int one[1] = {0}, two[2] = {0, 1};
        ListParams lp;
        /* D, one general: the game's own decision */
        pick_generals(1, abc);
        g_lock = 0; CHECK(fD(g_window, &g_gem[0]) == 1, "D, one general: original cmp (lock 0 -> je taken)");
        g_lock = 1; CHECK(fD(g_window, &g_gem[0]) == 0, "D, one general: original cmp (lock 1 -> not taken)");
        g_lock = 0;
        /* D, three generals: book 0 known by the first general, B may learn it -> stays selectable */
        pick_generals(3, abc);
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_person.slot[0] = 10;
        CHECK(fD(g_window, &g_gem[0]) == 2, "D, 3 generals: book the first general knows stays selectable for the others");
        g_pB.slot[0] = 10; g_pC.slot[0] = 10;
        CHECK(fD(g_window, &g_gem[0]) == 1, "D, 3 generals: nobody can learn it -> greyed as usual");
        /* Y: first general cannot learn book 0 -> only he is skipped */
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_person.slot[0] = 10;
        g_orders.orders = 20; books(5); g_learn_calls = 0; U32AT(DL_OFF_FSKIP) = 0;
        learn_books(1, one);
        r = confirm(0);
        CHECK(r == 1 && sentinels_ok(), "Y, first general skipped: result 1, registers preserved");
        CHECK(g_learn_calls == 2 && g_learn_person[0] == &g_pB && g_learn_person[1] == &g_pC && U32AT(DL_OFF_FSKIP) == 1,
              "Y: first general (already knows it) skipped, B and C learn");
        CHECK(used(&g_person) == 1 && g_force.count[1] == 3 && g_orders.orders == 18, "nothing spent for the first general");
        /* two books: first general skips book 0 but learns book 1 */
        reset_person(&g_person, 0); reset_person(&g_pB, 0); reset_person(&g_pC, 0);
        g_person.slot[0] = 10;
        g_orders.orders = 20; books(5); g_learn_calls = 0;
        learn_books(2, two); confirm(0);
        CHECK(g_learn_calls == 5 && has(&g_person, 11) && has(&g_pB, 10) && has(&g_pB, 11) && has(&g_pC, 10) && has(&g_pC, 11),
              "2 books x 3 generals, first knows book 0: 5 learns");
        /* S with several generals: free slots of the most free general */
        reset_person(&g_person, 9); reset_person(&g_pB, 4); reset_person(&g_pC, 8);
        g_orders.orders = 30;
        lp = open_books();
        CHECK(lp.mode == 2 && lp.max == 6, "S, 3 generals: free slots = the most free general (B: 6)");
        pick_generals(1, abc);
    }
    CHECK(g_misaligned == 0, "every call into game functions had a 16-byte aligned stack");
}

/* ------------------------------------------------------------------ part 2: worker on a real image */
static void host_log(const char *l) { printf("    [log] %s\n", l); }
static int wait_state(unsigned bits, int ms) {
    while (ms > 0) { if ((DL_GetState() & bits) == bits) return 1; usleep(10000); ms -= 10; }
    return 0;
}
static uint8_t *jmp_dest(uint8_t *s) { int32_t r; memcpy(&r, s + 1, 4); return s + 5 + r; }
static int g_hp_calls, g_hp_n, g_hp_ok, g_hp_len[9];
static char g_hp_name[64];
static int host_patch_group(const char *name, const DL_Patch *p, unsigned n) {
    unsigned i;
    g_hp_calls++; g_hp_n = (int)n; g_hp_ok = 1;
    strncpy(g_hp_name, name, 63);
    for (i = 0; i < n && i < 9; i++) { g_hp_len[i] = (int)p[i].length; if (memcmp(p[i].target, p[i].expected, p[i].length)) g_hp_ok = 0; }
    if (!g_hp_ok) return 0;
    for (i = 0; i < n; i++) memcpy(p[i].target, p[i].replacement, p[i].length);
    return 1;
}

int main(int argc, char **argv) {
    FILE *f;
    size_t sz;
    uintptr_t rv[22];
    int i;
    uint8_t oS[9], oX[7], oY[5];
    stub_tests();
    if (argc < 24) { printf("(no image given - full test skipped)\n"); goto done; }
    for (i = 0; i < 22; i++) rv[i] = strtoul(argv[2 + i], 0, 16);
    printf("== full worker run on %s\n", argv[1]);
    f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); sz = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    IMG = mmap((void *)0x140000000, sz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (IMG != (void *)0x140000000) { printf("cannot map image\n"); return 2; }
    if (fread(IMG, 1, sz, f) != sz) return 2;
    fclose(f);
    {
        uint8_t *S = IMG + rv[0], *X = IMG + rv[1], *Y = IMG + rv[2], *P = IMG + rv[12], *Q = IMG + rv[13];
        uint8_t oP[10], oQ[6], oD[7], *Dd = IMG + rv[19];
        memcpy(oD, Dd, 7);
        memcpy(oS, S, 9); memcpy(oX, X, 7); memcpy(oY, Y, 5); memcpy(oP, P, 10); memcpy(oQ, Q, 6);
        {
            DL_Host h = {sizeof h, host_log, 0};
            DL_Config c = {sizeof c, 10, 8, 0};
            CHECK(DL_Start(&h, &c) == 1, "DL_Start");
        }
        CHECK(wait_state(DL_STATE_HOOKED, 30000), "worker resolved everything and hooked");
        if (!(DL_GetState() & DL_STATE_HOOKED)) { printf("  (not hooked - remaining image checks skipped)\n"); goto done; }
        CHECK(S[0] == 0xE9 && S[5] == 0x90 && S[8] == 0x90, "S = jmp + 4 nop");
        CHECK(X[0] == 0xE9 && X[5] == 0x90 && X[6] == 0x90, "X = jmp + 2 nop");
        CHECK(Y[0] == 0xE9, "Y call -> jmp");
        CHECK(P[0] == 0xE9 && P[5] == 0x90 && P[9] == 0x90, "P = jmp + 5 nop");
        CHECK(Q[0] == 0xE9 && Q[5] == 0x90, "Q = jmp + nop");
        CHECK(Dd[0] == 0xE9 && Dd[5] == 0x90 && Dd[6] == 0x90, "D = jmp + 2 nop");
        {
            uint8_t *b = jmp_dest(S) - DL_OFF_STUB_S;
            CHECK(jmp_dest(X) == b + DL_OFF_STUB_X && jmp_dest(Y) == b + DL_OFF_STUB_Y && jmp_dest(P) == b + DL_OFF_STUB_P &&
                  jmp_dest(Q) == b + DL_OFF_STUB_Q, "all 5 jumps land in one block");
            CHECK(*(uint8_t **)(b + DL_OFF_R_P) == P + 10 && *(uint8_t **)(b + DL_OFF_R_Q) == Q + 6, "P/Q return addresses");
            CHECK(*(uint8_t **)(b + DL_OFF_F_TID) == IMG + rv[14] && *(uint8_t **)(b + DL_OFF_F_FID) == IMG + rv[15] &&
                  *(uint8_t **)(b + DL_OFF_F_PSPEC) == IMG + rv[16] && *(uint8_t **)(b + DL_OFF_F_PFORM) == IMG + rv[17] &&
                  *(uint8_t **)(b + DL_OFF_F_GIDX) == IMG + rv[18], "eligibility / book-count functions resolved");
            CHECK(*(uint32_t *)(b + DL_OFF_PMAX) == 8, "MaxGenerals written");
            CHECK(jmp_dest(Dd) == b + DL_OFF_STUB_D && *(uint8_t **)(b + DL_OFF_R_D) == Dd + 7 &&
                  *(uint8_t **)(b + DL_OFF_R_EN) == IMG + rv[20] && *(uint8_t **)(b + DL_OFF_P_LOCK) == IMG + rv[21], "D jump, targets and lock global");
            CHECK(*(uint8_t **)(b + DL_OFF_R_S) == S + 9 && *(uint8_t **)(b + DL_OFF_R_X) == X + 7 && *(uint8_t **)(b + DL_OFF_R_Y) == Y + 5, "return addresses");
            CHECK(*(uint8_t **)(b + DL_OFF_F_LEARN) == IMG + rv[3], "LearnTactic resolved");
            CHECK(*(uint8_t **)(b + DL_OFF_F_VALID) == IMG + rv[4], "valid() resolved");
            CHECK(*(uint8_t **)(b + DL_OFF_F_SLOT) == IMG + rv[5], "tactic slot() resolved");
            CHECK(*(uint8_t **)(b + DL_OFF_F_ORDERS) == IMG + rv[6], "orders() resolved");
            CHECK(*(uint8_t **)(b + DL_OFF_P_DB) == IMG + rv[7], "data root global resolved");
            CHECK(*(uint8_t **)(b + DL_OFF_P_COST) == IMG + rv[8] && *(int *)(IMG + rv[8]) == 1, "orders cost global resolved (=1)");
            CHECK(*(uint8_t **)(b + DL_OFF_P_HEAD) == IMG + rv[9] && *(uint8_t **)(b + DL_OFF_P_CNT) == IMG + rv[10] && *(uint8_t **)(b + DL_OFF_P_NPOOL) == IMG + rv[11],
                  "result tables resolved");
            CHECK(*(uint32_t *)(b + DL_OFF_MAX) == 10 && *(uint32_t *)(b + DL_OFF_ON) == 1, "settings written");
        }
        DL_Stop();
        CHECK(!memcmp(S, oS, 9) && !memcmp(X, oX, 7) && !memcmp(Y, oY, 5) && !memcmp(P, oP, 10) && !memcmp(Q, oQ, 6) && !memcmp(Dd, oD, 7),
              "DL_Stop restored the original bytes (6 sites)");

        printf("== conflict: X already patched by someone else\n");
        X[0] = 0xE9;
        {
            DL_Host h = {sizeof h, host_log, 0};
            DL_Start(&h, 0);
            usleep(1500000);
            CHECK(DL_GetState() & DL_STATE_FAIL_PATTERN, "X pattern gone -> feature off");
            CHECK(!memcmp(S, oS, 9) && !memcmp(Y, oY, 5), "S and Y untouched");
            DL_Stop();
        }
        memcpy(X, oX, 7);
        printf("== conflict: Y call already redirected by someone else\n");
        Y[0] = 0xE9;
        {
            DL_Host h = {sizeof h, host_log, 0};
            DL_Start(&h, 0);
            usleep(1500000);
            CHECK(DL_GetState() & DL_STATE_FAIL_PATTERN, "Y pattern gone -> feature off");
            CHECK(!memcmp(S, oS, 9) && !memcmp(X, oX, 7), "S and X untouched");
            DL_Stop();
        }
        memcpy(Y, oY, 5);
        printf("== host patch_group (ModCore style)\n");
        {
            DL_Host h = {sizeof h, host_log, host_patch_group};
            DL_Start(&h, 0);
            CHECK(wait_state(DL_STATE_HOOKED, 30000), "hooked through the host");
            CHECK(g_hp_calls == 1 && g_hp_n == 9 && !strcmp(g_hp_name, "SAN14CE.DragLearn"), "one group SAN14CE.DragLearn with 9 patches (6 + display R1/R2/R3)");
            CHECK(g_hp_len[0] == 9 && g_hp_len[1] == 7 && g_hp_len[2] == 5 && g_hp_len[3] == 10 && g_hp_len[4] == 6 && g_hp_len[5] == 7 && g_hp_len[6] == 5 && g_hp_len[7] == 5 && g_hp_len[8] == 5 && g_hp_ok,
                  "lengths 9/7/5/10/6/7/5/5/5, expected bytes matched");
            DL_Stop();
            CHECK(S[0] == 0xE9, "host-owned patches stay (host decides)");
        }
    }
done:
    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
