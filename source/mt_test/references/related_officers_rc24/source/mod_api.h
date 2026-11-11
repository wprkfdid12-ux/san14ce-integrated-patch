/* Shared x64 C ABI, version 1. No C++/CRT ownership crosses module boundaries. */
#ifndef SAN14_MOD_API_H
#define SAN14_MOD_API_H
#define S14_ABI 0x00010000u
#define S14_LAYOUT_ABI 1u
#define S14_SYMBOL_CAP 256
#define S14_OFFSET_CAP 1536
typedef struct {
    U32 officer_max,formation_max,tactic_max,trait_max;
    U32 person_table,formation_table,tactic_table,trait_table;
    U32 trait_slots,tactic_slots;
} S14_Limits;
typedef struct {
    U32 size,abi,layout_abi,symbol_count,offset_count,image_size;
    U8 *image;
    const SIZE *symbols;
    const U32 *offsets;
    I32 (*ReadLimits)(S14_Limits *);
    const char *profile;
} S14_Resolved;
typedef struct {
    U8 *target;
    U32 length;
    U8 expected[16],replacement[16];
} S14_Hook;
typedef struct {
    U32 size,abi;
    const S14_Resolved *resolved;
    I32 (*Readable)(const void *,SIZE);
    I32 (*InstallHooks)(const char *,const S14_Hook *,U32);
    U8 *(*MakeRelay)(const void *,const void *);
    void (*FreeRelay)(void *);
    void (*Log)(const char *);
} S14_CoreAPI;
typedef U32 (*S14_ModInit)(const S14_CoreAPI *);
typedef I32 (*S14_ResolverInit)(U8 *,S14_Resolved *,I32 (*)(const void *,SIZE));
#endif
