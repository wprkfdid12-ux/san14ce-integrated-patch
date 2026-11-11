#pragma once
#include <stdint.h>

/* Build-specific RVAs. The injector checks the disk SHA-256 and the generated
   instruction guards before loading or patching anything. No model IDs here. */
enum {
    FL_ANIMATION_RATE = 0x937a00,
    FL_MOTION_EVALUATE = 0xda0be0,
    FL_MOTION_SET_FRAME = 0xbd3b80,
    FL_PROTOCOL = 0x464c0003,
    FL_IMAGE_SIZE = 0x2a02000,
    FL_UNIT_MANAGER = 0x21462a8,
    FL_DATA_MANAGER = 0x2780f60,
    FL_RESOURCE_CONTEXT = 0x27d4258,
    FL_APP = 0x1ba2bc0,
    FL_ARMY_VTABLE = 0x13a33e8,
    FL_PERSON_VTABLE = 0x13a9bc8,
    FL_ACTOR_CTOR = 0x1c0940,
    FL_ACTOR_DTOR = 0x1c18f0,
    FL_ACTOR_CREATE = 0x1c6880,
    FL_ACTOR_BIND = 0x1c9600,
    FL_ACTOR_UPDATE = 0x1cc740,
    FL_ACTOR_DRAW = 0x1c8390,
    FL_ACTOR_RELEASE = 0x1cad10,
    FL_CURRENT_APPEARANCE = 0x3753b0,
    FL_REQUEST_COLOR = 0x1c9b70,
    FL_BODY_SELECTOR = 0x375430,
    FL_MANAGER_UPDATE = 0x1cccc0,
    FL_MANAGER_DRAW = 0x1c84e0,
    FL_MANAGER_RELEASE = 0x1caee0,
    FL_RESOURCES_RELEASE = 0x400530,
    FL_DRAW_FLAGS = 0x110d0,
    FL_ARRAY_VTABLE = 0x1694578,
    FL_CHARACTER_VTABLE = 0x16ccfc8,
    FL_MODEL_GET_POSITION = 0xdb24e0,
    FL_MODEL_GET_SCALE = 0xdb27e0,
    FL_MODEL_SET_SCALE = 0xdc2fb0,
    FL_BODY_ATTACH = 0x1c3420,
    FL_WEAPON_ATTACH = 0x1c35b0,
    FL_FIELD_GROUP_DRAW = 0x1c8460,
    FL_FIELD_ACTION_DRAW = 0x1c7fe0,
    FL_EVENT_SOUND_2D = 0xf0dd30,
    FL_EVENT_SOUND_3D = 0xf0de50,
    FL_TACTIC_EFFECT = 0x173200,
    FL_TACTIC_DAMAGE = 0x1a52e0,
    FL_NORMAL_DAMAGE = 0x16b180,
    FL_MOTION_EFFECT = 0xe55ad0,
    FL_BATTLE_POWER = 0x2aaf30,
    FL_PAIR_ATTACK = 0x16bb00,
    FL_PAIR_DEFENSE = 0x16b940,
    FL_TACTIC_FACTOR = 0x16c740,
    FL_TACTIC_KIND = 0x16e4c0,
    FL_TOP_STATE = 0x5726b0,
    FL_STATE_MANAGER = 0x1b65100,
    FL_AMBIENT_ENQUEUE = 0x406e70,
    FL_AMBIENT_CLEANUP = 0x407f80,
    FL_AMBIENT_START = 0x406150,
    FL_AMBIENT_VOLUME = 0x407b70,
    FL_SOUND_MANAGER = 0x27d5718,
    FL_ARMY_UPDATE = 0x1cb940,
    FL_ARMY_UPDATE_VTABLE = 0x13a3408,
    FL_ARMY_DETACH = 0x1c8cc0,
    FL_ARMY_DETACH_VTABLE = 0x13a3428,
    FL_FIELD_EFFECT_CREATE = 0x37f520,
    FL_FIELD_EFFECT_RESET = 0x3804e0,
    FL_PAIR_EFFECT_SET = 0x17adf0,
    FL_DEFEAT_COMMIT = 0x320d30,
    FL_INFO_ANCHOR = 0x19f800,
    FL_INFO_ANCHOR_VTABLE = 0x139bd28,
    FL_INFO_VTABLE = 0x139ba98,
    FL_INFO_DESTROY = 0x19c2f0,
    FL_INFO_SIZE = 0x19ff30,
    FL_UI_VISIBLE = 0x871600,
    FL_PRIVATE_ACTOR_VTABLE = 0x13a3600,
    FL_MODEL_SET_POSITION = 0xdc2ae0,
    FL_MODEL_BONE_TRANSFORM = 0xdaf2c0,
    FL_NATIVE_VALID = 0x331780,
    FL_UI_POSITION = 0x92f130,
    FL_UI_SIZE = 0x92f440,
    FL_UI_ELEMENT_SIZE = 0x932940,
    FL_UI_ELEMENT_POSITION = 0x932590,
    FL_CAMERA_DISTANCE = 0xea80, /* 1.0.1 (옛 0xe960) */
    FL_CAMERA_VISIBLE = 0x1c7230,
    FL_TACTIC_CUTSCENE_ACTIVE = 0x1b5c2e0,
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
    {0x44d668, FL_MANAGER_UPDATE},
    {0x44d67a, FL_MANAGER_DRAW},
    {0x1cb09e, FL_MANAGER_RELEASE},
    {0x14cb41, FL_RESOURCES_RELEASE},
    {0x1c82bf, FL_FIELD_GROUP_DRAW},
    {0x1c82d1, FL_FIELD_GROUP_DRAW},
    {0x1c82ff, FL_FIELD_GROUP_DRAW},
    {0x1c831a, FL_FIELD_GROUP_DRAW},
    {0x1c8335, FL_FIELD_ACTION_DRAW},
    {0x1c826a, FL_FIELD_GROUP_DRAW},
    {0x1c827c, FL_FIELD_GROUP_DRAW},
    {0x1c828e, FL_FIELD_GROUP_DRAW},
    {0x1c82a0, FL_FIELD_GROUP_DRAW},
    {0xe5368c, FL_EVENT_SOUND_2D},
    {0xe539cd, FL_EVENT_SOUND_3D},
    {0x172ef4, FL_TACTIC_EFFECT},
    {0x173015, FL_TACTIC_DAMAGE},
    {0x403754, FL_AMBIENT_ENQUEUE},
    {0x403780, FL_AMBIENT_ENQUEUE},
    {0x403864, FL_AMBIENT_ENQUEUE},
    {0x408e3f, FL_AMBIENT_CLEANUP},
    {0x16b86c, FL_NORMAL_DAMAGE},
    {0xe50362, FL_MOTION_EFFECT},
    {0x172f0f, FL_TACTIC_EFFECT},
    {0x408eb0, FL_AMBIENT_START},
    {0x3a6ef3, FL_AMBIENT_VOLUME},
    {0x13a3408, FL_ARMY_UPDATE},
    {0x1c76f5, FL_FIELD_EFFECT_CREATE},
    {0x1c2f2c, FL_FIELD_EFFECT_CREATE},
    {0x25d6a0, FL_DEFEAT_COMMIT},
    {0x13a3428, FL_ARMY_DETACH},
    {0x139bd28, FL_INFO_ANCHOR},
    {0x139ba98, FL_INFO_DESTROY},
    {0x37dce5,FL_ANIMATION_RATE},
    {0x37ddda,FL_ANIMATION_RATE},
    {0x37dd4f,FL_MOTION_EVALUATE},
    {0x37ddf1,FL_MOTION_SET_FRAME}
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
