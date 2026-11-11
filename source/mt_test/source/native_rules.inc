/* R11: the original candidate-list predicate AND native commit call this entry.
 * Full native fingerprints are checked before installing a single hook. Auxiliary
 * callees/table offset are decoded from that verified function, not guessed.
 */
static int (*original_merit_eligible)(void*),(*original_merit_rank)(void*),(*original_merit_current_rank)(void*);
static int (*rule_special)(void*,int),(*rule_is_ruler)(void*,int);
static void*(*rule_assignment)(void*);
static void*(*rule_location)(void*,int);
static u32 rule_rank_table;
static int rules_native_ready;
/* Same lifetime as the immutable trait/bonus tables: invalidated at successful
 * load/new-game, refresh_tables and resolver setup. No timer or shared UI hook.
 * An unreadable/incomplete table is never cached as a valid result. */
static u8*rules_cached_database;
static int rules_cached_senior;
static void reset_rules_cache(void){rules_cached_database=0;rules_cached_senior=0;}
static const HookSpec*rule_hook(u32 kind){for(u32 i=0;i<res->hook_count;i++)if(res->hooks[i].kind==kind)return &res->hooks[i];return 0;}
static int rule_code_hash(u8*p,u32 size,const u8 hash[32]){u8 got[32];if(!rd(p,size))return 0;mc_sha256(p,size,got);return mc_equal(got,hash,32);}
static void*rule_call(u8*p){if(!rd(p,5)||*p!=0xe8)return 0;u8*t=p+5+*(int*)(p+1);return t>=game&&t<game+res->image_size&&rd(t,16)?t:0;}
static int rules_prepare_native(void){
 static const u8 r16_live_sex[32]={0xab,0x8b,0xcc,0xe9,0x97,0xf1,0x19,0xd2,0x3b,0xb9,0x82,0xea,0x68,0x2c,0x55,0x5f,0xb0,0xfd,0x34,0x9d,0x02,0x08,0x8d,0x1c,0xd8,0xc6,0x3c,0x1d,0x7a,0x10,0x37,0x8f};
 if(!rule_code_hash(game+0x361140,0x1f,r16_live_sex))return 0;
 static const u8 r16_trait_link[32]={0x73,0x0a,0xdf,0x01,0x9f,0x87,0xc6,0x0d,0xa0,0x75,0x2a,0xb8,0xf1,0x0d,0xfb,0xe5,0xa6,0xb0,0xca,0xa6,0x76,0x7e,0xf5,0x1b,0x98,0x3a,0x43,0x6a,0xf2,0x39,0x38,0x64};
 if(!rule_code_hash(game+0x20f870,0x20,r16_trait_link))return 0;
 static const u8 r16_edit_exclusion[32]={0x47,0x25,0xcd,0x6c,0xf9,0x61,0x91,0x1a,0x60,0xfb,0x2f,0x67,0x47,0xdb,0x95,0x85,0x7f,0x82,0xa0,0x1b,0xfd,0xa5,0x3c,0x13,0x99,0xd6,0xf1,0x12,0x1b,0x0d,0x56,0x8c};
 if(!rule_code_hash(game+0x216060,0xe00,r16_edit_exclusion))return 0;

 reset_rules_cache();
 /* R15 uses the native ruler's kind-2 result as an eligibility probe. Validate
    the whole original function once, including early exclusions and its fixed
    kind-2 value of two, before relying on that contract. Never patch its body. */
 static const u8 bh[32]={0xcc,0xe6,0x90,0x7e,0xc0,0x0b,0x65,0xdf,0xa2,0x57,0x54,0x00,0x70,0x47,0xa8,0xfe,0xca,0x3a,0x5e,0x9f,0x08,0x3d,0x58,0xf6,0x3a,0xb3,0x7b,0xeb,0xb2,0x75,0x2b,0x15};
 if(!rule_code_hash(game+res->funcs[F_BONUS],0x176,bh))return 0;
 static const u8 sh[32]={0x62,0x0c,0x0b,0xf5,0xc8,0x42,0xfb,0x1b,0x95,0x80,0x9e,0xb5,0xf0,0x75,0xcb,0x28,0x4f,0xcd,0x25,0x6f,0x7b,0xe7,0xb1,0x8a,0x7b,0x87,0x83,0x22,0x72,0xb5,0x25,0x5f};
 static const u8 eh[32]={0xfd,0xcc,0xcd,0x00,0xb2,0xf1,0xf1,0x63,0x02,0x88,0x19,0xcf,0x0f,0x0b,0x6b,0x6d,0x0c,0x9b,0x21,0x11,0xd3,0xea,0xfd,0x51,0x2d,0x3a,0x14,0xf9,0xc7,0xd1,0xde,0xa0};
 static const u8 rh[32]={0x57,0xc3,0x7b,0x1a,0xb4,0x58,0x32,0x0d,0xe0,0xb2,0xe2,0xb9,0x2f,0xde,0xb9,0x59,0xd4,0xb7,0x24,0xaf,0xff,0x5f,0x93,0x0f,0x42,0xd5,0xaf,0x94,0xd3,0x26,0x74,0xe5};
 static const u8 ch[32]={0x70,0x6e,0x33,0x26,0xbd,0x78,0x2c,0x42,0xd7,0x85,0x1b,0x7e,0x41,0x6a,0xb2,0x8c,0x29,0x6e,0x64,0x9a,0x2c,0x58,0x6a,0xf0,0x18,0x68,0xa0,0x8f,0x04,0x28,0x75,0x5e};
 const HookSpec*c=rule_hook(H_NATIVE_CURRENT_RANK);
 const HookSpec*h=rule_hook(H_NATIVE_RANK);u8*e=game+res->funcs[F_ELIGIBLE];u8*r=h?game+h->target:0;
 if(!h||!c||!rule_code_hash(game+c->target,72,ch)||!rule_code_hash(e,216,eh)||!rule_code_hash(r,149,rh))return 0;
 rule_is_ruler=rule_call(e+0x22);
 if(!rule_is_ruler||!rule_code_hash((u8*)rule_is_ruler,17,sh))return 0;
 rule_special=rule_call(e+0x30);rule_assignment=rule_call(e+0x5c);rule_location=rule_call(e+0x88);
 /* Native mov r8,[database+rank_table] at rank entry +0x5b. */
 rule_rank_table=D(r,0x5e);
 if(!rule_special||!rule_assignment||!rule_location||rule_rank_table>0x100000u)return 0;
 rules_native_ready=1;return 1;
}
static int senior_rank(void){u8*d;u32 n=0;
 /* The game image/global slot lives for this process. Read its CURRENT value
  * on every query; this is not a cached memory-permission result or officer.
  * Avoid VirtualQuery on that same fixed image address for every warm rank.
  * A different DB immediately takes the fully checked cold path. */
 if(rules_native_ready&&rules_cached_database&&rules_cached_senior>0&&game&&res&&
    P(game,OFF(O_DATABASE))==rules_cached_database)return rules_cached_senior;
 PERF_HIT(PERF_SENIOR_COLD);d=db();
 if(!rules_native_ready||!d)return 0;
 if(rules_cached_database==d&&rules_cached_senior>0)return rules_cached_senior;
 if(!rd(d+rule_rank_table,7*8))return 0;
 for(u32 rank=1;rank<=6;rank++){u8*r=P(d,rule_rank_table+rank*8);if(!rd(r,0x34))return 0;
  if(D(r,0x30))n++;if(n==3){rules_cached_database=d;rules_cached_senior=(int)rank;return (int)rank;}
 }return 0;
}
/* R12: a candidate/read-only rank query must not enumerate and reinitialize
 * every force's ruler. Most callers are ordinary officers; the verified native
 * 17-byte status getter rejects those in constant time. An unrecorded current
 * ruler alone searches for its own force record once, then keeps the durable
 * veteran bit. A cached negative result/TTL is deliberately NOT used: a fresh
 * succession is recognized immediately, even within the same timer tick.
 * Caller has checked the officer range or owns the verified native callback
 * contract. Only live table identity can acquire ruler history, never an
 * arbitrary object with a copied ID.
 */
static void refresh_target_ruler(u8*o,u8 origin){u32 fid;u16 id=S(o,OFF(O_OFFICER_ID));u8*d;
 if(id>=MAX_OFFICERS||id>=res->officer_count||!native_ui_ready||!rules_native_ready)return;
 if(profiles[id].valid&&(profiles[id].reserved[0]&PROFILE_RULER_VETERAN))return;
 if(!rule_is_ruler||!rule_is_ruler(o,1)||NX(X_FORCE_MAX)>255||(d=db())==0||
    !rd(d+NX(X_FORCE_TABLE),(NX(X_FORCE_MAX)+1)*8))return;
 for(fid=0;fid<=NX(X_FORCE_MAX);fid++){u8*force=P(d,NX(X_FORCE_TABLE)+fid*8);
  if(!rd(force,NX(X_FORCE_RULER)+2)||S(force,NX(X_FORCE_RULER))!=id||!CALL(F_VALID,int(*)(void*))(force))continue;
  if(officer(id)!=o)return;
  ensure_profile(o,origin);profiles[id].reserved[0]|=PROFILE_RULER_VETERAN;return;
 }
}
static int rule_available_checked(u8*o){u8*a,*loc;
 if(!rules_native_ready||!CALL(F_VALID,int(*)(void*))(o))return 0;
 if(rule_special(o,0)||(S(o,OFF(O_FLAGS))&0xc000u))return 0;
 a=rule_assignment(o);if(CALL(F_VALID,int(*)(void*))(a)){
  if(!rd(a,8)||!rd(P(a,0)+0xb0,8)||V(a,0xb0,int(*)(void*))(a))return 0;
 }
 loc=rule_location(o,1);if(CALL(F_VALID,int(*)(void*))(loc)&&(!rd(loc,0x38)||B(loc,0x37)))return 0;
 return B(o,OFF(O_UPPER))+B(o,OFF(O_LOWER))<3;
}
static int rule_available(u8*o){return rd(o,OFF(O_LOWER)+1)&&rule_available_checked(o);}
/* ONLY for the four verified native CPersonData callbacks below. The original
 * rank functions already dereference this object directly, and the original
 * eligibility function calls F_VALID on it before field access. After world
 * preparation, keep that same native lifetime/type contract instead of making
 * a Windows VirtualQuery syscall per officer per menu refresh. No officer or
 * eligibility-result cache is introduced. Cold initialization, force ownership,
 * external display APIs, saves, and arbitrary helper inputs remain checked.
 * Do not use this helper to validate untrusted/external pointers. */
static int native_query_person(u8*o){
 if(!o)return 0;
 if(metadata_ready&&rules_native_ready)return 1;
 PERF_HIT(PERF_QUERY_COLD);return rd(o,OFF(O_LOWER)+1);
}
static int hook_merit_eligible(void*person){u8*o=person;PERF_HIT(PERF_ELIGIBLE);
 if(!enabled||metadata_fault||!native_query_person(o)||!CALL(F_VALID,int(*)(void*))(o))
  return original_merit_eligible?original_merit_eligible(person):0;
 if(!metadata_ready)capture_profiles(2);else refresh_target_ruler(o,2);
 if(!transaction)sync_merit_history_checked(o,S(o,OFF(O_OFFICER_ID)));
 if(metadata_fault)return 0;
 if(ruler_veteran_id(S(o,OFF(O_OFFICER_ID))))return senior_rank()>0&&rule_available_checked(o);
 return original_merit_eligible?original_merit_eligible(person):0;
}
static int hook_merit_rank(void*person){u8*o=person;PERF_HIT(PERF_RANK);
 if(enabled&&!metadata_fault&&native_query_person(o)){
  if(metadata_ready)refresh_target_ruler(o,2);
  if(ruler_veteran_id(S(o,OFF(O_OFFICER_ID)))){int rank=senior_rank();if(rank>0)return rank;}
 }
 return original_merit_rank?original_merit_rank(person):0;
}

/* Native merit rank labels use the current-rank getter, which also rejects
 * status-1 rulers in the original. Keep its display consistent with capacity. */
static int hook_merit_current_rank(void*person){u8*o=person;PERF_HIT(PERF_CURRENT_RANK);
 if(enabled&&!metadata_fault&&native_query_person(o)){
  if(metadata_ready)refresh_target_ruler(o,2);
  if(ruler_veteran_id(S(o,OFF(O_OFFICER_ID)))){int rank=senior_rank();if(rank>0)return rank;}
 }
 return original_merit_current_rank?original_merit_current_rank(person):0;
}
