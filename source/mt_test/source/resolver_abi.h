#ifndef MERIT_RESOLVER_ABI_H
#define MERIT_RESOLVER_ABI_H
#include "merit_core.h"
#define MERIT_ABI 0x10000u
#define MAX_HOOKS 48
/* Offsets live only in the resolver build; gameplay code uses this descriptor. */
enum {F_COMMIT,F_BONUS,F_ROW_REFRESH,F_STATUS,F_CONFIRM,F_STREAM_OPEN,F_LOAD,F_NEWGAME,F_SAVE_PREP,F_SAVE_END,F_VALID,F_LABEL_COLOR,F_CAPTION_COLOR,F_UI_EXIT,F_ELIGIBLE,F_STATE_TICK,F_CLOSE,F_PRESENT_RESUME,F_PRESENT_FINISH,F_BUTTON_EVENT,F_ROW_DESTROY,F_COUNT};
enum {O_DATABASE,O_OFFICER_TABLE,O_OFFICER_ID,O_STATS,O_TRAITS,O_RANK,O_UPPER,O_LOWER,O_FLAGS,O_TRAIT_TABLE,O_TRAIT_NAME,O_ROW_OFFICER,O_ROW_TEXT,O_ROW_BUTTON,O_ROW_PARENT,O_MAIN_STATE,O_STATE_LAYOUT,O_BONUS_TABLE,O_TRAIT_COLOR,O_UI9_GUARD,O_ROW_LOCK_LABEL,O_ROW_LOCK_ICON,O_ROW_VTABLE,O_COUNT};
enum {H_COMMIT_PLAYER=1,H_COMMIT_AI,H_BONUS,H_ROW_REFRESH,H_ROW_TICK,H_UI_CONFIRM,H_STATUS,H_LOAD,H_NEWGAME,H_SAVE_PREP,H_SAVE_END,H_STREAM_OPEN,H_WRITE_STEAM,H_WRITE_FILE,H_UI_EXIT,H_STATE_TICK,H_CLOSE,H_PRESENT_GATE,H_BUTTON_EVENT,H_ROW_DESTROY,H_AI_VISIT,H_SAFE_FLUSH,H_UI_ENTER,H_NATIVE_ELIGIBLE,H_NATIVE_RANK,H_NATIVE_EXIT_EVENT,H_NATIVE_CURRENT_RANK};
typedef struct {u32 rva;u8 length,kind,is_vtable,pad;u8 before[16];u32 target;} HookSpec;
enum {N_RADIO,N_RESET_TEXT,N_SET_SPEAKER,N_TALK,N_APP,N_INPUT,N_TARGET,N_STREAM_SEAL,N_STREAM_FLUSH,N_AI_ELIGIBLE,N_SOUND,N_TEXT_FONT,N_PERSON_SERIALIZE,N_COLOR_GET,N_COLOR_SET,N_ALLOC,N_FREE,N_BUTTON_CTOR,N_BUTTON_INIT,N_BUTTON_TEXT,N_BIND_ACTION,N_CALLBACK_TYPE,N_DYNAMIC_TEXT,N_PRIM_TEXT,N_PRIM_SIZE,N_PRIM_POS,N_TEXT_ALIGN,N_COUNT};
enum {X_ROW_GROUP,X_TEXT_MANAGER,X_STATE_FORCE,X_FORCE_TABLE,X_FORCE_MAX,X_FORCE_RULER,X_STEAM_USER_CONTEXT,X_STEAM_CONTEXT_IAT,X_HOVER_SHOW,X_HOVER_RELEASE,X_HOVER_GAME,X_HOVER_TEXT,X_HOVER_TEXT_SIZE,X_HOVER_IMAGE_SIZE,X_TACTICS,X_TACTIC_COUNT,X_FATHER,X_MOTHER,X_SOUND_MANAGER,X_COUNT};
typedef struct {u32 funcs[N_COUNT],offsets[X_COUNT];u8 guards[N_COUNT][16];u8 hover_text_hash[32];u8 guard_lengths[N_COUNT];} NativeConfig;
typedef struct {u32 counts_site,refresh_site,tree_offset;u8 counts_call[8],refresh_call[8];} SelectionConfig;
enum {P_LAYOUT_CTOR,P_COLLECTION_CTOR,P_BALLOON_INIT,P_BALLOON_ALPHA,P_COLLECTION_WIDTH,P_COLLECTION_HEIGHT,P_COLLECTION_VISIBLE,P_WINDOW,P_PRIM_ALPHA,P_COUNT};
typedef struct {u32 funcs[P_COUNT];u8 guards[P_COUNT][16];u32 vtable,object_size;} PopupConfig;
typedef struct {u32 size,abi,timestamp,image_size;u32 officer_count,trait_count;u32 funcs[F_COUNT];u32 offsets[O_COUNT];u8 function_guards[F_COUNT][16];u32 hook_count;HookSpec hooks[MAX_HOOKS];u32 trait_patch_rva[5];u8 trait_before[5][8],trait_after[5][8],trait_length[5];u8 pad[3];NativeConfig native;SelectionConfig selection;PopupConfig popup;} MeritResolver;
typedef struct {u32 size,abi,layout,fn_count,offset_count,image_size;u8 *game;void**functions;u32*offsets;void*extra;} BaseResolver;
typedef struct {void*address;u32 length;u8 before[16];u8 after[16];u32 pad;} CorePatch;
typedef struct {u32 size,abi;const BaseResolver*resolver;int(*readable)(const void*,usize);int(*patch)(const char*,CorePatch*,u32);void*(*relay)(void*,void*);void(*free_relay)(void*);void(*log)(const char*);} ModCore;
#endif
