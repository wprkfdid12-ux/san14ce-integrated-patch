#ifndef SAN14_PRESET_CORE_H
#define SAN14_PRESET_CORE_H
/* Independent preset model. This API never reads or writes the game process.
 * The caller must obtain Context from the current native sortie validation.
 * UTF-16 names are preset labels only; no localized entity names are persisted.
 */
typedef unsigned char P_U8;
typedef unsigned short P_U16;
typedef unsigned int P_U32;
typedef int P_I32;
enum { P_SLOTS=11, P_NAME=48, P_SCHEMA=1, P_JSON_MAX=32768 };
enum { P_FIXED=0, P_MAXIMUM=1 };
enum { P_READY=0, P_OFFICER_UNAVAILABLE=1, P_EMPTY=2, P_INVALID=3,
       P_NO_FORMATION=4 };
enum { P_TROOPS_ADJUSTED=1, P_FORMATION_REPLACED=2,
       P_TACTIC1_CLEARED=4, P_TACTIC2_CLEARED=8, P_TACTIC3_CLEARED=16 };
typedef struct {
    P_I32 officer_id, formation_id, troops, tactic_id[3];
} P_Selection;
typedef struct {
    P_U32 occupied;
    P_Selection selection;
    P_I32 troop_mode;
    P_U16 name[P_NAME];
} P_Preset;
typedef struct { P_U32 schema; P_Preset slots[P_SLOTS]; } P_Book;
typedef struct {
    P_I32 available, reason, max_troops, current_formation, default_formation;
    P_U32 allowed_formations; /* bits 0..20, populated by native validation */
    P_U32 tactic_formations[201]; /* usable formation bits per owned tactic */
} P_Context;
typedef struct {
    P_I32 applicable, reason;
    P_Selection selection;
    P_U32 adjustments;
} P_LoadPlan;
typedef struct { P_I32 trait_id[9]; } P_LiveTraits;
typedef P_I32 (*P_ReadTrait)(void *context,P_I32 officer_id,P_I32 slot);

P_U32 PresetAbiVersion(void);
void PresetInit(P_Book *book);
P_I32 PresetSave(P_Book *book,P_I32 slot,const P_Selection *selection,
    P_I32 mode,const P_U16 *name,const P_U16 *default_name,P_I32 overwrite);
P_I32 PresetRename(P_Book *book,P_I32 slot,const P_U16 *name);
P_I32 PresetResolve(const P_Book *book,P_I32 slot,const P_Context *context,
    P_LoadPlan *out);
P_I32 PresetReadLiveTraits(P_I32 officer_id,P_ReadTrait reader,void *context,
    P_LiveTraits *out);
/* Returns required byte count excluding NUL, or 0 on invalid input/overflow. */
P_U32 PresetToJson(const P_Book *book,char *output,P_U32 capacity);
/* Atomic in memory: malformed input leaves *book completely unchanged. */
P_I32 PresetFromJson(P_Book *book,const char *input,P_U32 bytes);
#endif
