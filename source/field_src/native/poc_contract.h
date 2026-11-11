#pragma once
#include <stdint.h>

/* Build-specific RVAs. The injector checks the disk SHA-256 and the generated
   instruction guards before loading or patching anything. No model IDs here. */
enum {
    FL_ANIMATION_RATE = 0x9352c0,
    FL_MOTION_EVALUATE = 0xd9e8c0,
    FL_MOTION_SET_FRAME = 0xbd14b0,
    FL_PROTOCOL = 0x464c0003,
    FL_IMAGE_SIZE = 0x29ff000,
    FL_UNIT_MANAGER = 0x2143260,
    FL_DATA_MANAGER = 0x277df18,
    FL_RESOURCE_CONTEXT = 0x27d1208,
    FL_APP = 0x1b9fb70,
    FL_ARMY_VTABLE = 0x13a03a8,
    FL_PERSON_VTABLE = 0x13a6b88,
    FL_ACTOR_CTOR = 0x1bf5a0,
    FL_ACTOR_DTOR = 0x1c0560,
    FL_ACTOR_CREATE = 0x1c5500,
    FL_ACTOR_BIND = 0x1c82b0,
    FL_ACTOR_UPDATE = 0x1cb520,
    FL_ACTOR_DRAW = 0x1c7010,
    FL_ACTOR_RELEASE = 0x1c99d0,
    FL_CURRENT_APPEARANCE = 0x373040,
    FL_REQUEST_COLOR = 0x1c8820,
    FL_BODY_SELECTOR = 0x3730c0,
    FL_MANAGER_UPDATE = 0x1cbaa0,
    FL_MANAGER_DRAW = 0x1c7160,
    FL_MANAGER_RELEASE = 0x1c9ba0,
    FL_RESOURCES_RELEASE = 0x3fe200,
    FL_DRAW_FLAGS = 0x11170,
    FL_ARRAY_VTABLE = 0x1691578,
    FL_CHARACTER_VTABLE = 0x16c9f80,
    FL_MODEL_GET_POSITION = 0xdb0000,
    FL_MODEL_GET_SCALE = 0xdb02f0,
    FL_MODEL_SET_SCALE = 0xdc0a20,
    FL_BODY_ATTACH = 0x1c2090,
    FL_WEAPON_ATTACH = 0x1c2220,
    FL_FIELD_GROUP_DRAW = 0x1c70e0,
    FL_FIELD_ACTION_DRAW = 0x1c6c60,
    FL_EVENT_SOUND_2D = 0xf0b8a0,
    FL_EVENT_SOUND_3D = 0xf0b9c0,
    FL_TACTIC_EFFECT = 0x172250,
    FL_TACTIC_DAMAGE = 0x1a3fe0,
    FL_NORMAL_DAMAGE = 0x16a1d0,
    FL_MOTION_EFFECT = 0xe536f0,
    FL_BATTLE_POWER = 0x2a8d40,
    FL_PAIR_ATTACK = 0x16ab50,
    FL_PAIR_DEFENSE = 0x16a990,
    FL_TACTIC_FACTOR = 0x16b790,
    FL_TACTIC_KIND = 0x16d510,
    FL_TOP_STATE = 0x56fe50,
    FL_STATE_MANAGER = 0x1b620b0,
    FL_AMBIENT_ENQUEUE = 0x404b40,
    FL_AMBIENT_CLEANUP = 0x405c50,
    FL_AMBIENT_START = 0x403e20,
    FL_AMBIENT_VOLUME = 0x405840,
    FL_SOUND_MANAGER = 0x27d26c8,
    FL_ARMY_UPDATE = 0x1ca600,
    FL_ARMY_UPDATE_VTABLE = 0x13a03c8,
    FL_ARMY_DETACH = 0x1c7940,
    FL_ARMY_DETACH_VTABLE = 0x13a03e8,
    FL_FIELD_EFFECT_CREATE = 0x37d150,
    FL_FIELD_EFFECT_RESET = 0x37e110,
    FL_PAIR_EFFECT_SET = 0x179e40,
    FL_DEFEAT_COMMIT = 0x31ecc0,
    FL_INFO_ANCHOR = 0x19e500,
    FL_INFO_ANCHOR_VTABLE = 0x1398ce8,
    FL_INFO_VTABLE = 0x1398a58,
    FL_INFO_DESTROY = 0x19b020,
    FL_INFO_SIZE = 0x19ec30,
    FL_UI_VISIBLE = 0x86f250,
    FL_PRIVATE_ACTOR_VTABLE = 0x13a05c0,
    FL_MODEL_SET_POSITION = 0xdc0550,
    FL_MODEL_BONE_TRANSFORM = 0xdacde0,
    FL_NATIVE_VALID = 0x32f500,
    FL_UI_POSITION = 0x92cc80,
    FL_UI_SIZE = 0x92cf90,
    FL_UI_ELEMENT_SIZE = 0x9303b0,
    FL_UI_ELEMENT_POSITION = 0x930000,
    FL_CAMERA_DISTANCE = 0xeb20, /* 1.0.1 (옛 0xe960) */
    FL_CAMERA_VISIBLE = 0x1c5eb0,
    FL_TACTIC_CUTSCENE_ACTIVE = 0x1b59290,
    FL_PROXY_COUNT = 500,
    FL_CREATE_PER_FRAME = 2,
    FL_MAX_LOADING = 8,
    FL_SNAPSHOT_SLOTS = 4,
    FL_DRAWS_PER_UNIT = 6,
    FL_MAX_OFFICER = 6000
};

/* Initial field-size calibration, not a claimed match to the approved image.
   About 1336 world units for the original 445-unit horse bounding length. */
static constexpr float FL_FIELD_SCALE = 3.0f;

enum FLPhase {
    FL_WAITING = 1, FL_LOADING = 2, FL_SUBMITTED = 3,
    FL_STOPPED = 4, FL_BLOCKED = 5
};
enum FLReason {
    FL_OK = 0, FL_NO_MAP = 1, FL_NO_LAND = 2,
    FL_CONTEXT_CHANGED = 4, FL_OTHER_THREAD = 5, FL_INVALID_APPEARANCE = 6,
    FL_NOT_READY = 7, FL_BAD_LIST = 8, FL_CALLBACK_CHANGED = 9,
    FL_DUEL_OR_OTHER_VIEW = 10, FL_LOAD_TIMEOUT = 11,
    FL_APPEND_FAILED = 12, FL_SCALE_FAILED = 13,
    FL_OVERLAPPING_CALL = 14
};
typedef struct FLConfig {
    uint32_t protocol;
    uint32_t reserved;
    uint64_t module_base;
    /* Windows UTF-16, not the host compiler's wchar_t. */
    uint16_t log_path[1024];
    uint64_t info_destroy_next;
} FLConfig;

typedef struct FLStatus {
    uint32_t protocol, phase, reason, frame;
    uint32_t draw_frames, ready_count, creates, releases;
    uint32_t officer[FL_PROXY_COUNT];
    int32_t appearance[FL_PROXY_COUNT];
    uint32_t thread_id, stopped;
    uint64_t callback_vtable_rva;
    uint64_t body_handle[FL_PROXY_COUNT], horse_handle[FL_PROXY_COUNT], weapon_handle[FL_PROXY_COUNT];
    uint32_t target_count, water_count, excluded_count, pending_count;
} FLStatus;

/* Separate export, paired by the exact DLL hash. Flags deliberately separate
   original selection from actual field readiness/submission and from pixels. */
enum FLRosterFlags { FL_ROSTER_PRESENT=1, FL_ROSTER_RESOLVED=2, FL_ROSTER_UNRESOLVED=4,
                     FL_ROSTER_COLOR_READY=8, FL_ROSTER_SUBMITTED=16 };
typedef struct FLRosterRow {
    int16_t body, weapon, body_color, horse_color;
    uint32_t flags;
} FLRosterRow;
typedef struct FLRosterAudit {
    uint32_t protocol, generation, scanned_slots, registered, resolved, unresolved, complete, reserved;
    FLRosterRow rows[FL_MAX_OFFICER+1];
} FLRosterAudit;

/* Verified E8 calls plus complete relocated presentation/lifetime vtable pointers:
   no copied prologs, instruction relocation or guessed instruction lengths.
   The external loader stops target threads for commit and verifies rollback. */
typedef struct FLPatchSpec { uint32_t site, original; } FLPatchSpec;
static const FLPatchSpec fl_patch_specs[] = {
    {0x44b428, FL_MANAGER_UPDATE},
    {0x44b43a, FL_MANAGER_DRAW},
    {0x1c9d5e, FL_MANAGER_RELEASE},
    {0x14bb61, FL_RESOURCES_RELEASE},
    {0x1c6f3f, FL_FIELD_GROUP_DRAW},
    {0x1c6f51, FL_FIELD_GROUP_DRAW},
    {0x1c6f7f, FL_FIELD_GROUP_DRAW},
    {0x1c6f9a, FL_FIELD_GROUP_DRAW},
    {0x1c6fb5, FL_FIELD_ACTION_DRAW},
    {0x1c6eea, FL_FIELD_GROUP_DRAW},
    {0x1c6efc, FL_FIELD_GROUP_DRAW},
    {0x1c6f0e, FL_FIELD_GROUP_DRAW},
    {0x1c6f20, FL_FIELD_GROUP_DRAW},
    {0xe512ac, FL_EVENT_SOUND_2D},
    {0xe515ed, FL_EVENT_SOUND_3D},
    {0x171f44, FL_TACTIC_EFFECT},
    {0x172065, FL_TACTIC_DAMAGE},
    {0x401414, FL_AMBIENT_ENQUEUE},
    {0x401440, FL_AMBIENT_ENQUEUE},
    {0x401524, FL_AMBIENT_ENQUEUE},
    {0x406b0f, FL_AMBIENT_CLEANUP},
    {0x16a8bc, FL_NORMAL_DAMAGE},
    {0xe4df82, FL_MOTION_EFFECT},
    {0x171f5f, FL_TACTIC_EFFECT},
    {0x406b80, FL_AMBIENT_START},
    {0x3a3b53, FL_AMBIENT_VOLUME},
    {0x13a03c8, FL_ARMY_UPDATE},
    {0x1c6375, FL_FIELD_EFFECT_CREATE},
    {0x1c1b9c, FL_FIELD_EFFECT_CREATE},
    {0x25bd00, FL_DEFEAT_COMMIT},
    {0x13a03e8, FL_ARMY_DETACH},
    {0x1398ce8, FL_INFO_ANCHOR},
    {0x1398a58, FL_INFO_DESTROY},
    {0x37b995,FL_ANIMATION_RATE},
    {0x37ba8a,FL_ANIMATION_RATE},
    {0x37b9ff,FL_MOTION_EVALUATE},
    {0x37baa1,FL_MOTION_SET_FRAME}
};
#define FL_PATCH_COUNT (sizeof(fl_patch_specs) / sizeof(fl_patch_specs[0]))

static inline unsigned fl_patch_width(const FLPatchSpec &spec) {
    return spec.site==FL_ARMY_UPDATE_VTABLE || spec.site==FL_ARMY_DETACH_VTABLE ||
           spec.site==FL_INFO_ANCHOR_VTABLE || spec.site==FL_INFO_VTABLE ? 8 : 5;
}
static inline bool fl_encode_patch(const FLPatchSpec &spec,uint64_t image,uint64_t destination,
                                  unsigned char *original,unsigned char *replacement) {
    if(fl_patch_width(spec)==8) {
        uint64_t before=image+spec.original;
        for(unsigned j=0;j<8;++j) {original[j]=(unsigned char)(before>>(j*8)); replacement[j]=(unsigned char)(destination>>(j*8));}
        return true;
    }
    int64_t distance=(int64_t)destination-(int64_t)(image+spec.site+5);
    if(distance<INT32_MIN || distance>INT32_MAX) return false;
    uint32_t before=spec.original-spec.site-5,after=(uint32_t)(int32_t)distance;
    original[0]=replacement[0]=0xe8;
    for(unsigned j=0;j<4;++j) {original[j+1]=(unsigned char)(before>>(j*8)); replacement[j+1]=(unsigned char)(after>>(j*8));}
    return true;
}

// R23 user amendment: siege mode2, like ships, keeps ALL native presentation.
static inline bool fl_land_mode(uint32_t mode) { return mode==0 || mode==3; }
static inline int fl_same_unit(uint64_t old_actor, uint64_t old_army,
                               uint64_t old_person, uint32_t old_id,
                               uint64_t actor, uint64_t army,
                               uint64_t person, uint32_t id) {
    return old_actor == actor && old_army == army && old_person == person && old_id == id;
}
