#ifndef MERIT_CORE_H
#define MERIT_CORE_H
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef __SIZE_TYPE__ usize;
typedef __INTPTR_TYPE__ iptr;
typedef struct {u16 id;u8 tier,mask;} TraitDef;
typedef struct {u16 held,candidate;} BlockPair;
#define MAX_OFFICERS 6000
#define MAX_TRAITS 355
#define TRAIT_SLOTS 9
#define MAX_CANDIDATES 355
#define NO_TRAIT 0xffffu
/* Fixed wire format, deliberately no native addresses or pointers. */
typedef struct {u16 id;u8 mask,origin;u8 initial[5];u8 valid;u16 awards[3];u8 award_mask;u8 reserved[3];} Profile;
typedef int (*Random32)(void*,u32*);
void *mc_copy(void*,const void*,usize);
void *mc_zero(void*,usize);
int mc_equal(const void*,const void*,usize);
u8 mc_type(const u8 stats[5]);
int mc_blocks(u16 held,u16 candidate);
u32 mc_pool(u8 mask,u8 stage,const u16 owned[9],u16 out[355]);
int mc_uniform(Random32 random,void *ctx,u32 n,u32 *out);
int mc_draw(Random32 random,void *ctx,u8 mask,u8 stage,const u16 owned[9],u16 *out);
int mc_slot(const u16 owned[9]);
int mc_has(const u16 owned[9],u16 id);
int mc_insert(u16 owned[9],u16 id);
void mc_profile(Profile*,u16,const u8[5],u8);
void mc_sha256(const void*,usize,u8 out[32]);
u32 mc_roll_index(u32 milliseconds,u32 candidates);
#endif
