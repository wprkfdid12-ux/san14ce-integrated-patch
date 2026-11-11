#ifndef MERIT_GACHA_H
#define MERIT_GACHA_H
#include "merit_core.h"
#define G_TEACHERS 1088
#define G_TOP 8
#define G_REROLLS 5
#define G_ALGO_VERSION 6
/* All balancing knobs are centralized; no per-officer or per-trait quality ranks. */
typedef struct {
 double stat_scale,base_overlap,gained_overlap,auxiliary,anchor_support;
 double parent,grandparent,family_cap,rarity_floor,gold_multiplier,star_gold;
 double budget_start,blue_cost,gold_cost,budget_floor,existing_gold_cost;
 double heat_gain,heat_cap,heat_multiplier,anomaly,transcend,ai_threshold;
 double miss_rate,passion_fit_threshold,passion_fit_multiplier,passion_gold_multiplier;
} GParams;
extern const GParams g_params;
typedef struct {u16 id,father,mother;u8 stats[5],star,role;u16 traits[9];const char*name;} GTeacher;
typedef struct {u8 color;u16 holders;u16 anchor;const char*name;} GTrait;
extern const GTeacher g_teachers[G_TEACHERS];
extern const GTrait g_traits[MAX_TRAITS];
enum {G_EMPTY,G_PENDING,G_ACCEPTED,G_DECLINED};
enum {G_TRAIT=0,G_MISS=1};
#define G_NO_X_SLOT 255u
enum {G_OK=1,G_INVALID=-1,G_NO_POOL=-2,G_NEED_INHERIT=-3,G_NO_REROLL=-4,G_CONFLICT=-5};
typedef struct {
 u64 roll_id,seed;
 u16 result,replace_trait;
 u8 phase,rerolls,anomaly,transcend,full,star,outcome,passion_applied;
 u8 x_index,pad[3]; /* Uses the old alignment padding: wire size remains 64. */
 double raw,weight,quality,cost;
} GSlot;
typedef struct {
 u16 base_traits[9];u8 initialized,rerolls_used;
 u16 x_mask;u8 passion,cancelled_rerolls; /* Former alignment padding, wire v3. */
 double budget,heat;
 GSlot slots[3];
} GState;
typedef struct {u16 id,traits[9];u8 stats[5],star;u16 father,mother;u8 category,sex;} GTarget;
typedef struct {u16 condition_value;short amount[2];u8 condition,effect[2],linked,present,pad;} GTraitRule;
void g_rules_update(const GTraitRule*);
extern u32 g_rules_revision;
int g_blocks(u16,u16);
int g_allowed(const GTarget*,u16);
int g_has_pool(const GTarget*,const GState*,const u8[MAX_TRAITS]);
typedef struct {u16 id;double score;} GScore;
typedef struct {
 GScore similar[G_TOP],candidates[G_TOP];
 double raw[MAX_TRAITS],weight[MAX_TRAITS],quality[MAX_TRAITS];
 u16 family[6];u32 family_count,count;
 double total;u8 anomaly,transcend;double family_weight[6];
 u8 similar_stats[G_TOP][5];
} GDistribution;
const GTeacher*g_teacher(u16 id);
u16 g_trait_holders(u16 id);
int g_trait_has_source(u16 id);
void g_init(GState*,const u16[9]);
int g_distribution(const GTarget*,const GState*,const u8[MAX_TRAITS],u8,u8,GDistribution*);
/* Caller-owned, per-AI-award cache. No saved state, timers or cached RNG. */
typedef struct {
 GTarget target;u16 base_traits[9];u8 installed[MAX_TRAITS],passion,anomaly,transcend,valid;
 double budget,heat;GDistribution distribution;u32 rules_revision;
} GDistributionCache;
typedef struct {u64 distributions,cache_hits,anchor_builds;} GCalcCounters;
extern GCalcCounters g_calc_counters;
void g_prepare(void);
int g_distribution_cached(const GTarget*,const GState*,const u8[MAX_TRAITS],u8,u8,GDistribution*,GDistributionCache*);
int g_roll_merit_cached(const GTarget*,GState*,u32,const u8[MAX_TRAITS],u64,u64,u16,int,GDistribution*,u32,GDistributionCache*);
int g_roll(const GTarget*,GState*,u32,const u8[MAX_TRAITS],u64,u64,u16,int,GDistribution*);
int g_roll_merit(const GTarget*,GState*,u32,const u8[MAX_TRAITS],u64,u64,u16,int,GDistribution*,u32);
int g_roll_merit_filtered(const GTarget*,GState*,u32,const u8[MAX_TRAITS],u64,u64,u16,int,GDistribution*,u32,int);
int g_accept(GTarget*,GState*,u32,u16,int);
int g_ai_reroll(const GState*,u32);
u16 g_ai_replacement(const GTarget*,const GDistribution*);
u64 g_next(u64*);
int g_validate(const GState*);
int g_free_slot(const GTarget*,const GState*);
void g_reconcile(const GTarget*,GState*);
double g_miss_rate(const GState*,u32);
/* Observation only: no trait-specific scores and no RNG calls. v1 is initial tuning. */
typedef struct {double value,surprise,rarity,special,score;u32 grade,version;} GEvaluation;
void g_evaluate(const GDistribution*,const GSlot*,GEvaluation*);
u32 g_grade(double score);
extern const unsigned char g_unique_tactics[256];
#endif
