/* SAN14 CE Merit Traits RC4 -- no CRT, native game IDs, content-bound save metadata.
 * Game layout is supplied by the separately replaceable, version-gated resolver.
 * No work, hooks, threads or loading is performed in DllMain.
 */
#include "winapi.h"
#include "resolver_abi.h"
#include "gacha.h"
#define P(p,o) (*(u8**)((u8*)(p)+(o)))
#define B(p,o) (*(u8*)((u8*)(p)+(o)))
#define S(p,o) (*(u16*)((u8*)(p)+(o)))
#define D(p,o) (*(u32*)((u8*)(p)+(o)))
#define Q(p,o) (*(u64*)((u8*)(p)+(o)))
#define CALL(f,t) ((t)(game+res->funcs[f]))
#define NCALL(f,t) ((t)(game+res->native.funcs[f]))
#define NX(o) (res->native.offsets[o])
#define OFF(o) (res->offsets[o])
#define V(p,o,t) ((t)(*(usize*)(P(p,0)+(o))))
typedef void(*Void1)(void*);
typedef void(*Flag)(void*,int);
typedef void(*Text)(void*,const u16*);
static const ModCore*core;
static const MeritResolver*res;
static u8*game;
static HANDLE self_module;
static int enabled,metadata_ready,metadata_fault;
static u32 r40_load_fallbacks,r40_officer_resets;static u8 r40_bad[MAX_OFFICERS];
/* R14 observation only: aligned, monotonic UI-thread counters. No timers,
 * logging, OS calls, allocations, or hooks are added by this data export.
 * A read-only external sampler can correlate native queries with menu changes.
 * Concurrent readers may see adjacent counters from different instants. */
enum {PERF_ELIGIBLE,PERF_CURRENT_RANK,PERF_RANK,PERF_AI,PERF_READABLE,
 PERF_QUERY_COLD,PERF_SENIOR_COLD,PERF_HOVER_COLLECT,PERF_HOVER_INFO,
 PERF_HOVER_COLUMN,PERF_BONUS,PERF_UI_ENTER,PERF_UI_EXIT,PERF_REPORT,PERF_COUNT};
typedef struct {char magic[8];u32 version,size;volatile u64 count[PERF_COUNT];} MeritPerf;
__declspec(dllexport) MeritPerf SAN14_MeritPerfV1={"MTPERF14",1,sizeof(MeritPerf),{0}};
#define PERF_HIT(n) (++SAN14_MeritPerfV1.count[n])
static int native_ui_ready;
static u32 ui_notification_depth;
static u8*hover_module;
static void (*shared_hover_show)(void*,usize);
static void (*shared_hover_release)(void*,int,void*);
static void hide_merit_hovers(int destroy);
static void probe_presets_x(void);
static volatile int transaction;
static SRWLOCK state_lock,io_lock;
static u64 clock_frequency,world_epoch;
static u8 active_save_hash[32];
static int active_save_hash_ok;
static u64 clock_start(void){long long t;if(clock_frequency&&QueryPerformanceCounter(&t)&&t>=0)return (u64)t;return 0;}
static double clock_elapsed(u64 start){long long t;if(start&&clock_frequency&&QueryPerformanceCounter(&t)&&t>=(long long)start)return (double)((u64)t-start)*1000.0/(double)clock_frequency;return -1.0;}
static Profile profiles[MAX_OFFICERS];
static GState states[MAX_OFFICERS],save_states[MAX_OFFICERS];
static u8 installed_colors[MAX_TRAITS];
static GDistribution distribution;
static u64 session_id,roll_sequence;
static void initialize_gstate(u8*o);
static int senior_rank(void);
static void reset_rules_cache(void);
static void sync_merit_history(u8*o);
static void sync_merit_history_checked(u8*o,u16 id);
static int runtime_diagnostic_deferred(const char*);
static void sync_all_merit_history(void);
static Profile save_profiles[MAX_OFFICERS],load_profiles[MAX_OFFICERS];
static u16 root_path[1024],state_path[1024];
static u32 state_path_length;
static void*pending_stream;
static u32 save_thread,load_thread;
static int pending_save,load_hash_ok;
static u8 load_hash[32];
static u64 generation;
static void*pending_ui;
static u64 pending_until;
static u32 player_depth;
/* RC2 presentation is owned only by the merit state. No background worker. */
static void*confirming_ui,*reveal_layout;
static int reveal_active,reveal_done,finish_armed;
static int deferred_close_code;
static u32 acknowledged_keys;
static u64 presentation_generation;
/* Assembly gate changes only the already-committed native presentation block. */
void *merit_presentation_skip, *merit_presentation_resume;
extern void merit_presentation_gate(void);
extern void __chkstk(void);
#define MAX_VISIBLE_ROWS 32
static void*visible_rows[MAX_VISIBLE_ROWS];
static u32 visible_row_count;
static u32 ui_row_logged,ui_row_rejection_logged;
static u32 ui_painted_labels;
static int bonus_cache_ready;
static u8 bonus_kind[3];
static u16 bonus_amount[3];
static u8 domestic_kind[3];
static u16 domestic_amount[3];
static int domestic_cache_ready;
static u8*trait_cache[MAX_TRAITS];
static int trait_cache_ready;
static void clear_reveal(void);
static void preview_reset(void);
static int ui_has_uncommitted(void);
static u64 roll_started[MAX_OFFICERS][3];
static u16 roll_pool[MAX_OFFICERS][3][MAX_CANDIDATES];
static u16 roll_count[MAX_OFFICERS][3];
static u8 visited[MAX_OFFICERS];
static int rollback_live;
static CorePatch installed[MAX_HOOKS+5];
static u8*relay_page;
static u32 relay_queries;
static u32 installed_count,relay_count;
/* R39 성능: 읽기 확인(rd)은 ModCore 의 VirtualQuery 시스템 호출이라 논공 열 때만 수천 번(실측 약 0.2~0.4초).
   ① 게임 이미지 안 주소는 늘 읽을 수 있으므로 바로 통과.
   ② 못 찾으면 그 주소를 감싼 64KB 단위 창 전체를 한 번 확인하고, 그 창을 약 16ms(한 프레임) 동안 기억.
      창 확인이 실패하면 원래대로 정확한 범위만 확인(캐시 안 함). 확인 뒤 실제 읽기까지의 틈은 원래도 있었음. */
#define RD_CACHE 64
static struct {usize b,e;long long t;} rd_cache[RD_CACHE];static u32 rd_cache_pos;static volatile long rd_lock;static u32 rd_hits,rd_queries;
static int rd(const void*p,usize n){
 if(!p||!core)return 0;
 usize a=(usize)p,z=a+n;if(z<a)return 0;
 if(game&&res&&a>=(usize)game&&z<=(usize)game+res->image_size)return 1;
 long long now=0;
 if(clock_frequency&&QueryPerformanceCounter(&now)&&now>0){long long win=(long long)(clock_frequency/64);int hit=0;
  while(__atomic_exchange_n(&rd_lock,1,__ATOMIC_ACQUIRE)){}
  for(u32 i=0;i<RD_CACHE;i++)if(a>=rd_cache[i].b&&z<=rd_cache[i].e&&now>=rd_cache[i].t&&now-rd_cache[i].t<win){hit=1;break;}
  __atomic_store_n(&rd_lock,0,__ATOMIC_RELEASE);
  if(hit){rd_hits++;return 1;}
  usize w0=a&~(usize)0xffff,w1=(z+0xffff)&~(usize)0xffff;
  if(w1>w0&&w1-w0<=0x40000){PERF_HIT(PERF_READABLE);rd_queries++;
   if(core->readable((const void*)w0,w1-w0)){
    while(__atomic_exchange_n(&rd_lock,1,__ATOMIC_ACQUIRE)){}
    u32 i=rd_cache_pos++%RD_CACHE;rd_cache[i].b=w0;rd_cache[i].e=w1;rd_cache[i].t=now;
    __atomic_store_n(&rd_lock,0,__ATOMIC_RELEASE);return 1;}
  }
 }
 PERF_HIT(PERF_READABLE);rd_queries++;return core->readable(p,n);
}
static u32 wlen(const u16*s){u32 n=0;while(s[n])++n;return n;}
static int append(u16*p,u32 cap,const u16*s){u32 a=wlen(p),b=wlen(s);if(a+b>=cap)return 0;mc_copy(p+a,s,(b+1)*2);return 1;}
static void report(const char*s){PERF_HIT(PERF_REPORT);if(enabled&&runtime_diagnostic_deferred(s))return;if(core&&core->log)core->log(s);}
static void log_award(u16 id,u8 slot,u16 trait,u32 pool){
 char s[128]="MeritTraits: award officer=";u32 n=26,i,v;char b[12];
 /* Bounded, plain numeric evidence; actual names are resolved by the game. */
 n=0;while(s[n])++n;v=id;i=0;do{b[i++]=(char)('0'+v%10);v/=10;}while(v);while(i)s[n++]=b[--i];
 const char*a=" stage=";while(*a)s[n++]=*a++;s[n++]=(char)('1'+slot);a=" trait=";while(*a)s[n++]=*a++;
 v=trait;i=0;do{b[i++]=(char)('0'+v%10);v/=10;}while(v);while(i)s[n++]=b[--i];a=" eligible=";while(*a)s[n++]=*a++;
 v=pool;i=0;do{b[i++]=(char)('0'+v%10);v/=10;}while(v);while(i)s[n++]=b[--i];s[n]=0;report(s);
}
static u8*db(void){return rd(game+OFF(O_DATABASE),8)?P(game,OFF(O_DATABASE)):0;}
static u8*officer(u32 id){u8*d=db(),*o;if(id>=res->officer_count||id>=MAX_OFFICERS||(!d||!rd(d+OFF(O_OFFICER_TABLE)+id*8,8)))return 0;o=P(d,OFF(O_OFFICER_TABLE)+id*8);if(!rd(o,OFF(O_LOWER)+1)||S(o,OFF(O_OFFICER_ID))!=id)return 0;return o;}
/* Read all colors for exclusion rules; red traits never enter the draw pool.
   One bounded snapshot per world/menu/AI commit, not per query or reroll. */
static u8*trait_rules_database,*trait_rule_objects[MAX_TRAITS];
static void refresh_trait_rules(void){
 GTraitRule rules[MAX_TRAITS];u8*d=db();u32 j;int warm=d&&d==trait_rules_database;mc_zero(rules,sizeof(rules));
 if(!d||res->trait_count>MAX_TRAITS||(!warm&&!rd(d+OFF(O_TRAIT_TABLE),res->trait_count*8))){trait_rules_database=0;return;}
 for(j=1;j<res->trait_count;j++){u8*x=P(d,OFF(O_TRAIT_TABLE)+j*8);GTraitRule*q=&rules[j];
  /* Native trait objects live until the world reload. Read current fields
     every time, but only validate new table/object identities with Core.
     This is an object lifetime cache, not a timed page-permission cache. */
  if(!x){trait_rule_objects[j]=0;continue;}
  if(!warm||x!=trait_rule_objects[j]){if(!rd(x,0xbf)){trait_rule_objects[j]=0;continue;}trait_rule_objects[j]=x;}
  if(!S(x,OFF(O_TRAIT_NAME)))continue;
  q->present=1;q->condition=B(x,0xa4);q->condition_value=S(x,0xb4);
  q->effect[0]=B(x,0xb6);q->effect[1]=B(x,0xba);q->amount[0]=(short)S(x,0xb8);q->amount[1]=(short)S(x,0xbc);
  u16 id=S(x,0xa2);u8*o=id<res->officer_count?officer(id):0;q->linked=o&&CALL(F_VALID,int(*)(void*))(o);
 }
 trait_rules_database=d;g_rules_update(rules);
}
/* Cache immutable table entries at load/new-game, not inside every bonus call. */
static void refresh_tables(void){
 u8*d=db();u32 j;bonus_cache_ready=0;domestic_cache_ready=0;trait_cache_ready=0;reset_rules_cache();
 mc_zero(trait_cache,sizeof(trait_cache));mc_zero(installed_colors,sizeof(installed_colors));
 if(!d)return;
 if(rd(d+OFF(O_BONUS_TABLE)+8,24)){
  for(j=0;j<3;j++){u8*b=P(d,OFF(O_BONUS_TABLE)+(j+1)*8);if(!rd(b,0x14))break;bonus_kind[j]=B(b,0x10);bonus_amount[j]=S(b,0x12);}
  bonus_cache_ready=(j==3);
 }
 if(rd(d+OFF(O_BONUS_TABLE)+4*8,24)){
  for(j=0;j<3;j++){u8*b=P(d,OFF(O_BONUS_TABLE)+(j+4)*8);if(!rd(b,0x14))break;domestic_kind[j]=B(b,0x10);domestic_amount[j]=S(b,0x12);}
  domestic_cache_ready=(j==3);
 }
 if(res->trait_count<=MAX_TRAITS&&rd(d+OFF(O_TRAIT_TABLE),res->trait_count*8)){
  for(j=1;j<res->trait_count;j++){u8*x=P(d,OFF(O_TRAIT_TABLE)+j*8);if(rd(x,OFF(O_TRAIT_COLOR)+1)&&S(x,OFF(O_TRAIT_NAME))&&(B(x,OFF(O_TRAIT_COLOR))==1||B(x,OFF(O_TRAIT_COLOR))==3)){trait_cache[j]=x;installed_colors[j]=B(x,OFF(O_TRAIT_COLOR));}}
  trait_cache_ready=1;refresh_trait_rules();
 }
}
static int valid_trait(u16 id){if(!trait_cache_ready)refresh_tables();return id>0&&id<res->trait_count&&id<MAX_TRAITS&&trait_cache[id]!=0;}
static const u16*trait_name(u16 id){return valid_trait(id)?(const u16*)(trait_cache[id]+OFF(O_TRAIT_NAME)):L"개성";}
static int random32(void*unused,u32*out){long status;(void)unused;status=BCryptGenRandom(0,(u8*)out,4,2);return status>=0;}
#define PROFILE_RULER_VETERAN 0x01u
static void ensure_profile(u8*o,u8 origin){u16 id=S(o,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS)return;if(!profiles[id].valid)mc_profile(&profiles[id],id,o+OFF(O_STATS),origin);initialize_gstate(o);}
static int ruler_veteran_id(u16 id){return id<MAX_OFFICERS&&profiles[id].valid&&!!(profiles[id].reserved[0]&PROFILE_RULER_VETERAN);}
static int ruler_veteran(u8*o){return o&&rd(o,OFF(O_OFFICER_ID)+2)&&ruler_veteran_id(S(o,OFF(O_OFFICER_ID)));}
/* Current ruler status is captured into Profile.reserved[0] and therefore the
   qualification survives demotion/capture/recruitment and exact-save reloads.
   Reappointment only reasserts the same bit; it never creates an award. */
static void refresh_ruler_profiles(u8 origin){u8*d=db();u32 fid;
 if(!native_ui_ready||!d||NX(X_FORCE_MAX)>255||!rd(d+NX(X_FORCE_TABLE),(NX(X_FORCE_MAX)+1)*8))return;
 for(fid=0;fid<=NX(X_FORCE_MAX);fid++){u8*force=P(d,NX(X_FORCE_TABLE)+fid*8);u16 id;u8*o;
  if(!rd(force,NX(X_FORCE_RULER)+2)||!CALL(F_VALID,int(*)(void*))(force))continue;id=S(force,NX(X_FORCE_RULER));if(id>=res->officer_count||id>=MAX_OFFICERS)continue;
  o=officer(id);if(!o)continue;ensure_profile(o,origin);profiles[id].reserved[0]|=PROFILE_RULER_VETERAN;
 }
}
static void capture_profiles(u8 origin){u32 id;u8*d=db();refresh_tables();
 g_prepare();
 if(d&&res->officer_count<=MAX_OFFICERS&&rd(d+OFF(O_OFFICER_TABLE),res->officer_count*8))
  for(id=0;id<res->officer_count;id++){u8*o=P(d,OFF(O_OFFICER_TABLE)+id*8);if(rd(o,OFF(O_LOWER)+1)&&S(o,OFF(O_OFFICER_ID))==id)ensure_profile(o,origin);}
 refresh_ruler_profiles(origin);sync_all_merit_history();metadata_ready=1;
}
#include "native_rules.inc"
#include "merit_history.inc"
static void clear_state(void){trait_rules_database=0;reset_rules_cache();mc_zero(profiles,sizeof(profiles));mc_zero(states,sizeof(states));clear_reveal();preview_reset();bonus_cache_ready=0;domestic_cache_ready=0;trait_cache_ready=0;metadata_ready=0;metadata_fault=0;active_save_hash_ok=0;mc_zero(active_save_hash,32);world_epoch++;generation++;}
static void initialize_gstate(u8*o){u16 id=S(o,OFF(O_OFFICER_ID));u32 j,k;if(states[id].initialized)return;g_init(&states[id],(const u16*)(o+OFF(O_TRAITS)));for(j=0;j<3;j++)if(profiles[id].award_mask&(1u<<j)){GSlot*x=&states[id].slots[j];x->phase=G_ACCEPTED;x->result=profiles[id].awards[j];x->cost=g_traits[x->result].color==3?g_params.gold_cost:g_params.blue_cost;states[id].budget-=x->cost;for(k=0;k<9;k++)if(states[id].base_traits[k]==x->result)states[id].base_traits[k]=0;}}
/* Runtime officer serializer verifies ten one-byte tactics and 16-bit parents.
 * Official targets AND comparison teachers use original DB abilities. Edited
 * or grown in-game abilities are never overwritten. Only a custom target with
 * no official DB row uses its native base abilities as a fallback. */
static void target_read(u8*o,GTarget*t){
 u16 id=S(o,OFF(O_OFFICER_ID));const GTeacher*q=g_teacher(id);u32 k;
 mc_zero(t,sizeof(*t));t->id=id;mc_copy(t->stats,q?q->stats:o+OFF(O_STATS),5);mc_copy(t->traits,o+OFF(O_TRAITS),18);
 t->sex=B(o,0xa9);t->category=q?q->role:3;t->father=t->mother=NO_TRAIT;
 if(native_ui_ready){
  for(k=0;k<NX(X_TACTIC_COUNT);k++)if(g_unique_tactics[B(o,NX(X_TACTICS)+k)]){t->star=1;break;}
  u16 father=S(o,NX(X_FATHER)),mother=S(o,NX(X_MOTHER));
  if(father>0&&father<res->officer_count&&father!=id)t->father=father;
  if(mother>0&&mother<res->officer_count&&mother!=id)t->mother=mother;
 }else if(q){t->star=q->star;t->father=q->father;t->mother=q->mother;}
}
static void reconcile_officer(u8*o){if(!o)return;u16 id=S(o,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS||!states[id].initialized)return;GTarget t;target_read(o,&t);g_reconcile(&t,&states[id]);}
static void repair_pending(u8*o){
 u16 id=S(o,OFF(O_OFFICER_ID));GTarget t;if(id>=MAX_OFFICERS||!states[id].initialized)return;target_read(o,&t);
 for(u32 j=0;j<3;j++){GSlot*x=&states[id].slots[j];
  if(x->phase==G_PENDING&&x->outcome==G_TRAIT&&!g_allowed(&t,x->result)){
   states[id].cancelled_rerolls+=x->rerolls;mc_zero(x,sizeof(*x));generation++;
   report("MeritTraits R16: invalid legacy pending result cleared; existing traits and spent tickets preserved.");
  }
 }
}
static u32 pool_for(u8*o,u8 stage,u16 out[355]){u32 n=0,j;u16 id=S(o,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS||metadata_fault||stage>2)return 0;ensure_profile(o,2);for(j=1;j<MAX_TRAITS;j++)if(installed_colors[j]&&g_trait_has_source((u16)j))out[n++]=(u16)j;return n;}

/* Versioned companions bind exact native filename + Steam account + content hash.
 * The legacy digest reader is retained for existing RC3/CP03 saves only. */
typedef struct {u8 magic[8];u32 version,record_size,count,origin_flags;u8 save_hash[32],body_hash[32];Profile records[MAX_OFFICERS];GState gacha[MAX_OFFICERS];} DiskState;
static DiskState disk,existing_disk;
static int sidecar_collision;
static volatile int save_active;
static void hash_path(const u8 h[32],u16 out[1024],int tmp){static const char hex[]="0123456789abcdef";u32 i;mc_copy(out,state_path,(state_path_length+1)*2);for(i=0;i<32;i++){out[state_path_length+i*2]=(u16)hex[h[i]>>4];out[state_path_length+i*2+1]=(u16)hex[h[i]&15];}out[state_path_length+64]=0;append(out,1024,tmp?L".mt4tmp":L".mt4dat");}
#include "slot_store.inc"
static int write_sidecar_unlocked(const u8*bytes,u32 size){u16 path[1024],tmp[1024];u8 hash[32];HANDLE f;u32 wrote=0;int ok;
 if(!pending_save||!size||size>256u*1024u*1024u||!rd(bytes,size))return 0;
 mc_sha256(bytes,size,hash);mc_zero(&disk,sizeof(disk));mc_copy(disk.magic,"S14MT001",8);disk.version=3;disk.record_size=sizeof(Profile);disk.count=MAX_OFFICERS;mc_copy(disk.save_hash,hash,32);mc_copy(disk.records,save_profiles,sizeof(save_profiles));mc_copy(disk.gacha,save_states,sizeof(save_states));mc_sha256(disk.records,sizeof(disk.records)+sizeof(disk.gacha),disk.body_hash);
 hash_path(hash,path,0);hash_path(hash,tmp,1);
 /* The compressed native stream does not encode a pending/declined roll.
    Different native slots can therefore share this digest. Until native slot
    identity is verified, never overwrite another state bound to this digest. */
 sidecar_collision=0;
 f=CreateFileW(path,0x80000000,1,0,3,0x80,0);
 if(f!=INVALID_HANDLE){long long previous_size=0;u32 got=0;
  int identical=GetFileSizeEx(f,&previous_size)&&previous_size==(long long)sizeof(existing_disk)&&ReadFile(f,&existing_disk,sizeof(existing_disk),&got,0)&&got==sizeof(existing_disk)&&mc_equal(&existing_disk,&disk,sizeof(disk));CloseHandle(f);
  if(identical)return 1;
  sidecar_collision=1;report("MeritTraits RC4: identical native save digest has different metadata; existing companion preserved. Native slot identity is not resolved.");return 0;
 }
 if(GetLastError()!=2)return 0;
 /* A legacy companion under the same digest must not lose its original awards. */
 {u16 legacy[1024];mc_copy(legacy,path,(wlen(path)+1)*2);legacy[state_path_length+64]=0;append(legacy,1024,L".mtdat");
  f=CreateFileW(legacy,0x80000000,1,0,3,0x80,0);
  if(f!=INVALID_HANDLE){long long n=0;u32 got=0,id,j;usize legacy_size=__builtin_offsetof(DiskState,gacha);u8 h[32];
   mc_zero(&existing_disk,sizeof(existing_disk));int same=GetFileSizeEx(f,&n)&&n==(long long)legacy_size&&ReadFile(f,&existing_disk,(u32)legacy_size,&got,0)&&got==legacy_size;CloseHandle(f);
   if(same){mc_sha256(existing_disk.records,sizeof(existing_disk.records),h);same=existing_disk.version==1&&mc_equal(existing_disk.magic,"S14MT001",8)&&mc_equal(existing_disk.save_hash,hash,32)&&mc_equal(h,existing_disk.body_hash,32)&&mc_equal(existing_disk.records,disk.records,sizeof(disk.records));}
   for(id=0;same&&id<MAX_OFFICERS;id++){if(disk.gacha[id].rerolls_used)same=0;for(j=0;j<3;j++)if(disk.gacha[id].slots[j].phase==G_PENDING||disk.gacha[id].slots[j].phase==G_DECLINED)same=0;}
   if(!same){sidecar_collision=1;report("MeritTraits RC4: legacy companion collision; RC3 metadata and native save preserved.");return 0;}
  }else if(GetLastError()!=2)return 0;
 }
 f=CreateFileW(tmp,0x40000000,0,0,2,0x80,0);if(f==INVALID_HANDLE)return 0;
 ok=WriteFile(f,&disk,sizeof(disk),&wrote,0)&&wrote==sizeof(disk)&&FlushFileBuffers(f);CloseHandle(f);
 if(ok)ok=MoveFileExW(tmp,path,1|8);if(!ok)DeleteFileW(tmp);return ok;
}
static int write_sidecar(const u8*bytes,u32 size){int result;AcquireSRWLockExclusive(&io_lock);result=write_sidecar_unlocked(bytes,size);ReleaseSRWLockExclusive(&io_lock);return result;}
static int read_sidecar_unlocked(const u8 h[32]){u16 path[1024];HANDLE f;long long size;u32 got=0,id,j;u8 hash[32];int ok;usize legacy_size=__builtin_offsetof(DiskState,gacha);
 hash_path(h,path,0);f=CreateFileW(path,0x80000000,1,0,3,0x80,0);if(f==INVALID_HANDLE){if(GetLastError()!=2)return -1;path[state_path_length+64]=0;append(path,1024,L".mtdat");f=CreateFileW(path,0x80000000,1,0,3,0x80,0);if(f==INVALID_HANDLE)return GetLastError()==2?0:-1;}
 mc_zero(&disk,sizeof(disk));ok=GetFileSizeEx(f,&size)&&(size==(long long)sizeof(disk)||size==(long long)legacy_size)&&ReadFile(f,&disk,(u32)size,&got,0)&&got==(u32)size;CloseHandle(f);
 if(!ok)return -1;return validate_loaded_disk(h,size);
}
static int validate_loaded_disk(const u8*h,long long size){
 u32 id,j;u8 hash[32];usize legacy_size=__builtin_offsetof(DiskState,gacha);
 if(!mc_equal(disk.magic,"S14MT001",8)||(disk.version<1||disk.version>3)||disk.record_size!=sizeof(Profile)||disk.count!=MAX_OFFICERS||!mc_equal(disk.save_hash,h,32))return -1;
 if((disk.version==1&&size!=(long long)legacy_size)||(disk.version>=2&&size!=(long long)sizeof(disk)))return -1;mc_sha256(disk.records,(usize)size-__builtin_offsetof(DiskState,records),hash);if(!mc_equal(hash,disk.body_hash,32))return -1;
 mc_zero(r40_bad,sizeof(r40_bad));
 for(id=0;id<MAX_OFFICERS;id++){Profile*p=&disk.records[id];u8*o;
  if(!p->valid)continue;if(p->valid!=1||p->id!=id||!p->mask||(p->mask&~31)||p->award_mask&~7||p->mask!=mc_type(p->initial)||p->origin<1||p->origin>2||!history_profile_valid(p)){r40_bad[id]=1;continue;}
  o=officer(id);if(!o){if(p->award_mask)r40_bad[id]=1;continue;}
  /* A saved award is historical metadata. Native O_UPPER is a current-state
     value and can legitimately decrease after capture/recruitment/ruler/status
     transitions. Binding integrity is already enforced by the exact native
     save hash and, for v2+, by the Profile <-> GState consistency checks below.
     Do not turn a legitimate rank/status transition into a global save fault. */
  for(j=0;j<3;j++)if(p->award_mask&(1u<<j)){int miss=disk.version==3&&disk.gacha[id].slots[j].outcome==G_MISS;if((!miss&&!valid_trait(p->awards[j]))||(miss&&p->awards[j]))r40_bad[id]=1;}
 }
 if(disk.version>=2){
  mc_zero(save_states,sizeof(save_states));
  for(id=0;id<MAX_OFFICERS;id++){
   GState*s=&disk.gacha[id];Profile*p=&disk.records[id];u8*o=officer(id);
   /* v2 used these bytes as ignored padding. Do not mutate disk: its checked
      bytes may be pinned by the two-generation journal or copy-save binding. */
   GState migrated=*s;if(disk.version==2){migrated.x_mask=0;migrated.passion=0;migrated.cancelled_rerolls=0;for(j=0;j<3;j++){migrated.slots[j].outcome=G_TRAIT;migrated.slots[j].passion_applied=0;migrated.slots[j].x_index=0;mc_zero(migrated.slots[j].pad,3);}}s=&migrated;
   if(r40_bad[id])continue;
   if(!g_validate(s)||s->initialized!=p->valid){r40_bad[id]=1;continue;}
   if(!s->initialized)continue;{int bad=0;
   for(j=0;j<3;j++){
    GSlot*x=&s->slots[j];int finalized=x->phase==G_ACCEPTED||x->phase==G_DECLINED;
    if(finalized!=!!(p->award_mask&(1u<<j))){bad=1;break;}
    if(finalized&&p->awards[j]!=x->result){bad=1;break;}
    /* Finalized slot history may outlive the officer's current native upper
       count after capture/recruitment/ruler/status changes. A pending slot is
       different: it claims a currently active, uncommitted upper-stage draw,
       so retain the native-stage sanity check for that transient state. */
    if(x->phase&&(!o||(x->outcome!=G_MISS&&!valid_trait(x->result)))){bad=1;break;}
    if(x->phase==G_PENDING&&B(o,OFF(O_UPPER))<=j&&(!(p->reserved[0]&PROFILE_HISTORY)||(p->reserved[1]&15)<=j)){bad=1;break;}
   }
   if(bad){r40_bad[id]=1;continue;}}
   save_states[id]=*s;
  }
  mc_copy(states,save_states,sizeof(states));
  for(id=0;id<MAX_OFFICERS;id++)if(states[id].initialized)reconcile_officer(officer(id));
 }
 if(disk.version==1)mc_zero(states,sizeof(states));mc_copy(load_profiles,disk.records,sizeof(load_profiles));
 /* R40 A2/A12: officers whose companion record did not match start fresh; everyone else keeps theirs. */
 {u32 n=0;for(id=0;id<MAX_OFFICERS;id++)if(r40_bad[id]){mc_zero(&load_profiles[id],sizeof(Profile));mc_zero(&states[id],sizeof(GState));mc_zero(&save_states[id],sizeof(GState));n++;}
  if(n){r40_officer_resets+=n;report("MeritTraits R40: some officers' companion records did not match this save; those officers start fresh, all others kept.");}}
 return 1;
}
static int read_sidecar(const u8 h[32]){int result;AcquireSRWLockExclusive(&io_lock);result=load_slot_key_ok?read_slot_unlocked(h):0;if(result==0)result=read_sidecar_unlocked(h);ReleaseSRWLockExclusive(&io_lock);return result;}
static u8 hook_stream_open(void*s,void*name,int mode,u8 special,u32 a,u32 b,int c){
 if(mode==1&&load_thread==GetCurrentThreadId()){load_slot_key_ok=slot_identity(name,load_slot_key,load_account_tag);}
 u8 ok=CALL(F_STREAM_OPEN,u8(*)(void*,void*,int,u8,u32,u32,int))(s,name,mode,special,a,b,c);
 if(ok&&mode==1&&load_thread==GetCurrentThreadId()&&rd(s,0x98)){u8*buffer=P(s,0x40);u32 size=D(s,0x24);if(size&&size<=256u*1024u*1024u&&rd(buffer,size)){mc_sha256(buffer,size,load_hash);load_hash_ok=1;}}
 return ok;
}
static int hook_load(void*d,int slot,const char*name){int ok,status=-1;
 if(save_active){report("MeritTraits: overlapping load/save refused.");return 0;}load_thread=GetCurrentThreadId();load_hash_ok=0;load_slot_key_ok=0;
 ok=CALL(F_LOAD,int(*)(void*,int,const char*))(d,slot,name);
 load_thread=0;
 if(!ok){report("MeritTraits: failed native load; prior live reward metadata preserved.");return ok;}
 AcquireSRWLockExclusive(&state_lock);clear_state();
 if(ok){refresh_tables();if(load_hash_ok&&(!native_ui_ready||load_slot_key_ok))status=read_sidecar(load_hash);if(status==1){mc_copy(profiles,load_profiles,sizeof(profiles));capture_profiles(2);metadata_ready=1;report("MeritTraits: exact-save metadata restored; acquired results will not reroll.");}
  else if(status==0){capture_profiles(2);report("MeritTraits: legacy/import save; first-observed unbonused base stats frozen. Previously activated upper flags now provide unrolled unlocks; native rank unchanged.");}
  else{capture_profiles(2);r40_load_fallbacks++;report("MeritTraits R40: companion metadata for this save could not be matched; merit gacha starts fresh from this save (acquired traits stay, no error box, saving allowed).");}}
 if(!metadata_fault&&load_hash_ok){mc_copy(active_save_hash,load_hash,32);active_save_hash_ok=1;}ReleaseSRWLockExclusive(&state_lock);probe_presets_x();return ok;
}
static int hook_newgame(void*d,int index){int ok=CALL(F_NEWGAME,int(*)(void*,int))(d,index);AcquireSRWLockExclusive(&state_lock);clear_state();if(ok){capture_profiles(1);report("MeritTraits: scenario/restart base-stat profiles captured.");}ReleaseSRWLockExclusive(&state_lock);if(ok)probe_presets_x();return ok;}
static void*hook_save_prep(void*d,int mode,const char*name,const char*description,void*extra){void*a;
 if(transaction||ui_has_uncommitted()){report("MeritTraits: save requested inside award transaction; save deferred safely.");MessageBoxW(0,L"논공 결과가 아직 결정되지 않아 저장하지 않았습니다. 하단 결정으로 결과를 적용한 뒤 다시 저장해 주세요.",L"논공 개성 - 저장하지 않음",0x30);return 0;}
 if(!__sync_bool_compare_and_swap(&save_active,0,1)){report("MeritTraits: overlapping save refused; previous snapshot kept.");return 0;}
 AcquireSRWLockExclusive(&state_lock);if(!metadata_ready)capture_profiles(2);else{refresh_ruler_profiles(2);sync_all_merit_history();}if(metadata_fault){metadata_fault=0;report("MeritTraits R40: earlier metadata problem cleared before save; saving current state (no block).");}
 for(u32 id=0;id<MAX_OFFICERS;id++)if(states[id].initialized)reconcile_officer(officer(id));
 mc_copy(save_profiles,profiles,sizeof(profiles));mc_copy(save_states,states,sizeof(states));ReleaseSRWLockExclusive(&state_lock);
 pending_save=1;save_thread=GetCurrentThreadId();pending_stream=0;save_slot_key_ok=0;slot_journal_active=0;
 a=CALL(F_SAVE_PREP,void*(*)(void*,int,const char*,const char*,void*))(d,mode,name,description,extra);
 if(a&&rd(a,0x18)){pending_stream=P(a,0x10);save_slot_key_ok=slot_identity(pending_stream,save_slot_key,save_account_tag);}else{pending_save=0;save_active=0;save_thread=0;}return a;
}
static int hook_save_end(void*d,void*a){int r=CALL(F_SAVE_END,int(*)(void*,void*))(d,a);if(save_thread==GetCurrentThreadId()){pending_save=0;pending_stream=0;save_thread=0;save_active=0;}return r;}
/* Called from tiny unwind-described assembly bridges at the two original writes.
 * Failure to durably save metadata makes the native save fail, not silently diverge. */
static int our_save_stream(void*stream){return pending_save&&pending_stream==stream&&save_thread==GetCurrentThreadId();}
static int hook_safe_flush(void*stream){
 if(our_save_stream(stream)&&native_ui_ready&&!save_slot_key_ok){
  report("MeritTraits: missing filename/account; flush rejected before native seek/write.");return 0;
 }
 return NCALL(N_STREAM_FLUSH,int(*)(void*))(stream);
}
static int prepare_save_metadata(const u8*buf,u32 size,u8 hash[32]){
 if(native_ui_ready&&!save_slot_key_ok){report("MeritTraits: native save identity unavailable; write refused rather than guessing a slot.");return 0;}
 /* Reuse the digest of the exact, const native buffer already included in
    the durable companion. Copy it under the I/O lock before native I/O; do not
    hash the same whole save a second time after the successful write. */
 AcquireSRWLockExclusive(&io_lock);
 int ok=save_slot_key_ok?slot_prepare_unlocked(buf,size):write_sidecar_unlocked(buf,size);
 if(ok)mc_copy(hash,disk.save_hash,32);ReleaseSRWLockExclusive(&io_lock);return ok;
}
static void save_warning(int native_wrote) __attribute__((unused));
static void save_warning(int native_wrote){
 MessageBoxW(0,native_wrote?L"원본 세이브 기록 뒤 보조 저장 연결을 완료하지 못했습니다.\n이전·새 추첨 상태를 모두 보존했습니다. 자동 추측 복구는 하지 않습니다.\n게임을 종료하고 저장정보 백업 및 문제자료모으기를 실행해 주세요.":L"논공 보조 저장정보 또는 저장 경로를 확인하지 못해 이번 저장을 중지했습니다.\n기존 보조 저장정보는 보존했습니다. 저장공간·권한을 확인하고 문제자료를 추출해 주세요.",L"논공 개성 - 저장 확인 필요",0x30);
}
u8 merit_write_steam(void*obj,const char*name,const u8*buf,u32 size,void*stream){
 u8 prepared_hash[32];int ours=our_save_stream(stream);
 int prepared=1;
 if(ours&&!prepare_save_metadata(buf,size,prepared_hash)){prepared=0;report("MeritTraits R40: companion metadata not written for this save; native save continues (this save will load as a fresh merit start).");}
 u8 ok=V(obj,0,u8(*)(void*,const char*,const void*,u32))(obj,name,buf,size);
 if(ours&&prepared&&save_slot_key_ok&&!slot_complete(ok))report("MeritTraits R40: journal commit after native write failed; both generations kept, next load/save resolves it (no block).");
 if(ok&&ours&&prepared){AcquireSRWLockExclusive(&state_lock);mc_copy(active_save_hash,prepared_hash,32);active_save_hash_ok=1;ReleaseSRWLockExclusive(&state_lock);}return ok;
}
int merit_write_file(void*obj,const u8*buf,u32 size,void*stream){
 u8 prepared_hash[32];int ours=our_save_stream(stream);
 /* This verified Steam build writes its named game saves through RemoteStorage.
  * An anonymous custom file backend cannot safely invent the missing filename. */
 int prepared=1;
 if(ours&&(!prepare_save_metadata(buf,size,prepared_hash))){prepared=0;report("MeritTraits R40: companion metadata not written for this save; native save continues (this save will load as a fresh merit start).");}
 int ok=V(obj,0x58,int(*)(void*,const void*,u32))(obj,buf,size);
 if(ours&&prepared&&save_slot_key_ok&&!slot_complete(ok))report("MeritTraits R40: journal commit after native write failed; both generations kept, next load/save resolves it (no block).");
 if(ok&&ours&&prepared){AcquireSRWLockExclusive(&state_lock);mc_copy(active_save_hash,prepared_hash,32);active_save_hash_ok=1;ReleaseSRWLockExclusive(&state_lock);}return ok;
}
extern void merit_steam_bridge(void),merit_file_bridge(void);

#include "runtime_flow.inc"
#include "observation.inc"
#include "native_ui.inc"
#include "native_controls.inc"
#include "display_patch.inc"
#include "hover_x_adapter.inc"
#include "presets_x_adapter.inc"
#include "event_log.inc"

static void*target_for(u8 kind){switch(kind){case H_NATIVE_CURRENT_RANK:return hook_merit_current_rank;case H_NATIVE_ELIGIBLE:return hook_merit_eligible;case H_NATIVE_RANK:return hook_merit_rank;case H_NATIVE_EXIT_EVENT:return hook_native_exit_event;case H_UI_ENTER:return hook_ui_enter;case H_SAFE_FLUSH:return hook_safe_flush;case H_AI_VISIT:return hook_ai_visit;case H_ROW_DESTROY:return hook_row_destroy;case H_BUTTON_EVENT:return hook_button_event;case H_COMMIT_PLAYER:return hook_commit_player;case H_COMMIT_AI:return hook_commit_ai;case H_BONUS:return hook_bonus;case H_ROW_REFRESH:return hook_row_refresh;case H_ROW_TICK:return hook_row_tick;case H_UI_CONFIRM:return hook_confirm;case H_STATUS:return hook_status;case H_LOAD:return hook_load;case H_NEWGAME:return hook_newgame;case H_SAVE_PREP:return hook_save_prep;case H_SAVE_END:return hook_save_end;case H_STREAM_OPEN:return hook_stream_open;case H_WRITE_STEAM:return merit_steam_bridge;case H_WRITE_FILE:return merit_file_bridge;case H_UI_EXIT:return hook_ui_exit;case H_STATE_TICK:return hook_state_tick;case H_CLOSE:return hook_close;case H_PRESENT_GATE:return merit_presentation_gate;default:return 0;}}
/* One near allocation for all rel32 stubs. Walk MEMORY REGIONS, not 64K
 * addresses per hook. Bounded queries/deadline; never stall startup indefinitely. */
static int allocate_relay_page(void){
 usize start=(usize)game+res->image_size,upper=(usize)game+0x70000000u,lower=(usize)game>0x70000000u?(usize)game-0x70000000u:0x10000u;
 u64 began=GetTickCount64();u32 pass;relay_queries=0;
 for(pass=0;pass<2;pass++){
  usize cur=(pass?lower:start);usize stop=pass?(usize)game:upper;
  while(cur<stop&&relay_queries<192&&GetTickCount64()-began<=150){
   MemoryRegion m;usize next,candidate;relay_queries++;
   if(VirtualQuery((const void*)cur,&m,sizeof(m))!=sizeof(m)||!m.region_size)break;
   next=(usize)m.base+m.region_size;if(next<=cur)break;
   candidate=(cur+0xffffu)&~(usize)0xffffu;
   if(m.state==0x10000u&&candidate<stop&&next>=candidate+4096){
    relay_page=VirtualAlloc((void*)candidate,4096,0x3000,0x04);
    if(relay_page){if((usize)relay_page!=candidate){VirtualFree(relay_page,0,0x8000);relay_page=0;return 0;}return 1;}
   }
   cur=next;
  }
 }
 report("MeritTraits RC4: near relay allocation unavailable within startup safety budget; no merit hooks installed.");return 0;
}
static int prepare_hooks(void){u32 i,j,old;relay_count=0;
 /* Validate every site before allocating or writing any patch. */
 for(i=0;i<res->hook_count;i++){
  const HookSpec*h=&res->hooks[i];CorePatch*p=&installed[i];
  if(!target_for(h->kind)||h->length>16||!h->length||h->rva+h->length>res->image_size)return 0;
  p->address=game+h->rva;p->length=h->length;mc_copy(p->before,h->before,h->length);
  if(h->is_vtable)*(usize*)p->before=(usize)(game+h->target);
  if(!rd(p->address,p->length)||!mc_equal(p->address,p->before,p->length)){report("MeritTraits: hook conflict/version mismatch; no merit hooks installed.");return 0;}
 }
 if(!allocate_relay_page())return 0;
 for(i=0;i<res->hook_count;i++){
  const HookSpec*h=&res->hooks[i];CorePatch*p=&installed[i];void*fn=target_for(h->kind);
  for(j=0;j<h->length;j++)p->after[j]=0x90;
  if(h->is_vtable)*(usize*)p->after=(usize)fn;
  else{
   u8*relay=relay_page+relay_count++*32;iptr delta=relay-((u8*)p->address+5);
   if(delta<(-2147483647LL-1)||delta>2147483647LL||relay_count*32>4096)return 0;
   relay[0]=0xff;relay[1]=0x25;D(relay,2)=0;Q(relay,6)=(usize)fn;
   int entry=h->kind==H_NATIVE_ELIGIBLE||h->kind==H_NATIVE_RANK||h->kind==H_NATIVE_CURRENT_RANK;
   if(entry){
    /* Both verified prologues are complete 2+4 byte instructions with no RIP
       operands. Execute them once, then jump back to the original body. */
    if(h->length!=6||relay_count>=128)return 0;
    u8*orig=relay_page+relay_count++*32;mc_copy(orig,h->before,6);
    orig[6]=0xff;orig[7]=0x25;D(orig,8)=0;Q(orig,12)=(usize)(game+h->target+6);
    if(h->kind==H_NATIVE_ELIGIBLE)original_merit_eligible=(void*)orig;else if(h->kind==H_NATIVE_RANK)original_merit_rank=(void*)orig;else original_merit_current_rank=(void*)orig;
   }
   p->after[0]=(entry||h->before[0]==0xe9||h->kind==H_PRESENT_GATE)?0xe9:0xe8;*(int*)(p->after+1)=(int)delta;
  }
 }
 if(!VirtualProtect(relay_page,4096,0x20,&old)||!FlushInstructionCache(GetCurrentProcess(),relay_page,4096))return 0;
 merit_presentation_resume=game+res->funcs[F_PRESENT_RESUME];installed_count=res->hook_count;return 1;
}
static void rollback(u32 n){u32 i,old,tmp;enabled=0;for(i=0;i<n;i++){CorePatch*p=&installed[i];if(mc_equal(p->address,p->after,p->length)){if(VirtualProtect(p->address,p->length,0x40,&old)){mc_copy(p->address,p->before,p->length);int flushed=FlushInstructionCache(GetCurrentProcess(),p->address,p->length),protected=VirtualProtect(p->address,p->length,old,&tmp);if(!flushed||!protected)rollback_live=1;}else rollback_live=1;}if(!mc_equal(p->address,p->before,p->length))rollback_live=1;}}
static int ensure_nine(void){CorePatch p[5];u32 n=0,i;for(i=0;i<5;i++){u8*address=game+res->trait_patch_rva[i];u32 len=res->trait_length[i];if(!rd(address,len))return 0;if(mc_equal(address,res->trait_after[i],len))continue;if(!mc_equal(address,res->trait_before[i],len))return 0;mc_zero(&p[n],sizeof(p[n]));p[n].address=address;p[n].length=len;mc_copy(p[n].before,res->trait_before[i],len);mc_copy(p[n].after,res->trait_after[i],len);n++;}return !n||core->patch("SAN14CE.MeritTraits.Traits9",p,n);}
static int paths(void){u32 n,i;u16 p[1024];n=GetModuleFileNameW(0,root_path,1024);if(!n||n>=1024)return 0;while(n&&root_path[n-1]!='\\'&&root_path[n-1]!='/')n--;root_path[n]=0;
 n=GetEnvironmentVariableW(L"LOCALAPPDATA",state_path,1024);if(!n||n>820)return 0;if(!append(state_path,1024,L"\\SAN14CE_MeritTraits"))return 0;CreateDirectoryW(state_path,0);if(!append(state_path,1024,L"\\CEKO_6a6d9f72"))return 0;CreateDirectoryW(state_path,0);if(!append(state_path,1024,L"\\"))return 0;state_path_length=wlen(state_path);
 mc_copy(p,state_path,(state_path_length+1)*2);append(p,1024,L"write_test.tmp");HANDLE f=CreateFileW(p,0x40000000,0,0,2,0x80,0);if(f==INVALID_HANDLE)return 0;CloseHandle(f);DeleteFileW(p);for(i=0;i<1;i++){}return 1;
}
static int address_in_module(HANDLE module,usize address){u8*m=module;u32 nt,n;if(!rd(m,4096)||S(m,0)!=0x5a4d)return 0;nt=D(m,0x3c);if(nt>4096||D(m,nt)!=0x4550)return 0;n=D(m,nt+80);return address>=(usize)m&&address-(usize)m<n;}
static int known_ui_hook(void){u8*address=game+OFF(O_UI9_GUARD);usize target;if(!rd(address,14)||address[0]!=0xff||address[1]!=0x25||D(address,2)!=0)return 0;target=*(usize*)(address+6);return address_in_module(GetModuleHandleW(L"SAN14_Traits9_UI.dll"),target)||address_in_module(GetModuleHandleW(L"UI_Traits9_Hover.dll"),target)||address_in_module(GetModuleHandleW(L"UI_Traits9.dll"),target);}
static int support_ui(void){HANDLE m;u16 path[1024];u32(*apply)(void);u8*address=game+OFF(O_UI9_GUARD);
 if(known_ui_hook()){report("MeritTraits: existing verified Traits9 UI module preserved.");return 1;}
 if(address[0]==0xff&&address[1]==0x25){report("MeritTraits: unrecognized UI hook owner; refusing to overwrite.");return 0;}
 m=GetModuleHandleW(L"SAN14_Traits9_UI.dll");if(!m){mc_copy(path,root_path,(wlen(root_path)+1)*2);if(!append(path,1024,L"SAN14Mods\\MeritTraits\\UI_Traits9_Hover.dll"))return 0;m=LoadLibraryExW(path,0,0x800);}
 if(!m)return 0;apply=(u32(*)(void))GetProcAddress(m,"SAN14_ApplyTraits9UI");if(!apply)return 0;apply();return known_ui_hook();
}
__declspec(dllexport) int SAN14_ModInitV1(const ModCore*c){u16 path[1024];HANDLE m;const MeritResolver*(*resolve)(const void*,u32);u32 i,n;u64 new_session;long long frequency;
 if(enabled)return 1;if(!c||c->size<56||c->abi!=MERIT_ABI||!c->resolver||!c->readable||!c->patch||!c->relay||!c->free_relay)return 0;
 core=c;game=c->resolver->game;if(!rd(game,4096)||!paths()){report("MeritTraits: initialization failed (game image or writable metadata folder).");return 0;}
 mc_copy(path,root_path,(wlen(root_path)+1)*2);append(path,1024,L"SAN14Mods\\MeritTraits\\SAN14_MeritResolver_CEKO.dll");m=LoadLibraryExW(path,0,0x800);if(!m)return 0;
 resolve=(const MeritResolver*(*)(const void*,u32))GetProcAddress(m,"SAN14_MeritResolveV1");if(!resolve||(res=resolve(game,c->resolver->image_size))==0||res->size!=sizeof(*res)||res->abi!=MERIT_ABI||res->hook_count>MAX_HOOKS||res->officer_count>MAX_OFFICERS||res->trait_count>MAX_TRAITS)return 0;
 for(i=0;i<F_COUNT;i++)if(!rd(game+res->funcs[i],16)||!mc_equal(game+res->funcs[i],res->function_guards[i],16)){report("MeritTraits: native target function was modified; refusing to hook.");return 0;}
 if(!entropy(&new_session)){report("MeritTraits: Windows system RNG unavailable; no time-seeded fallback used.");return 0;}
 if(!native_guard_check()||!rules_prepare_native()||!prepare_hooks()||!ensure_nine()||!support_ui()||!bind_shared_hover()||!prepare_hover_x()){report("MeritTraits: prerequisite/hook verification failed; merit awards not enabled.");goto failed;}
 for(i=0;i<installed_count;i+=n){n=installed_count-i;if(n>16)n=16;if(!core->patch("SAN14CE.MeritTraits",installed+i,n)){rollback(i+n);report("MeritTraits: all installed merit hooks rolled back after transaction failure.");goto failed;}}
 if(!install_display_calls(hover_module,NX(X_HOVER_IMAGE_SIZE),hover_x_patches,6)||!core->patch("SAN14CE.MeritTraits.HoverX",hover_x_patches+6,1)){rollback_hover_x();rollback(installed_count);goto failed;}
 native_ui_ready=1;session_id=new_session;if(QueryPerformanceFrequency(&frequency)&&frequency>0)clock_frequency=(u64)frequency;start_logger();enabled=1;probe_presets_x();report("MeritTraits RC4-R17-TurnTransitionPerf: ACTIVE. Exact R16 draw probabilities and AI rerolls retained; transposed immutable teacher membership; JSON on existing worker; incremental live rule updates; no trait refresh for empty/domestic-only AI awards; prepared save digest reused. Native command-menu queries reuse the verified live-object contract; warm senior-rank checks avoid OS memory queries. Read-only performance counters available. All revealed uncommitted stages independently rerollable; held results preserved. Bounded queued runtime diagnostics; batch-local history checks; X-free display fast path; checked-leaf candidate queries; cached immutable senior rank; no force-wide scan per ordinary candidate. Single native success exit; actual ruler candidate/rank/commit predicates; durable reward ledger; AI upper-first with 9/9 domestic fallback; ruler-veteran capacity 3; collaboration trait sources 39/44; five rerolls PER OFFICER; guarded native decision/cancel; original DB plus documented collaboration supplement; v3 journal.\r\n");return 1;
failed:rollback_hover_x();if(!rollback_live){if(relay_page)VirtualFree(relay_page,0,0x8000);relay_page=0;relay_count=0;}else report("MeritTraits: rollback protection failed; relay memory retained until game exit.");return 0;
}
int DllMain(HANDLE m,u32 reason,void*r){(void)r;if(reason==1)self_module=m;return 1;}

/* Read-only diagnostics; no game hooks or mutations occur in this export. */
typedef struct {const char*name;const void*address;} MeritSymbol;
__declspec(dllexport) const MeritSymbol*SAN14_MeritDiagnosticsV1(void){static const MeritSymbol symbols[]={
{"g_has_pool",(const void*)&g_has_pool},
{"g_allowed",(const void*)&g_allowed},
{"g_blocks",(const void*)&g_blocks},
{"g_rules_update",(const void*)&g_rules_update},
{"g_rules_revision",(const void*)&g_rules_revision},
{"refresh_trait_rules",(const void*)&refresh_trait_rules},
{"repair_pending",(const void*)&repair_pending},
{"active_state",(const void*)&active_state},
{"runtime_diagnostic_deferred",(const void*)&runtime_diagnostic_deferred},
{"logger_attempted",(const void*)&logger_attempted},
{"distribution",(const void*)&distribution},
{"preview_recompose",(const void*)&preview_recompose},
{"preview_roll_prefix",(const void*)&preview_roll_prefix},

{"rule_is_ruler",(const void*)&rule_is_ruler},
{"refresh_target_ruler",(const void*)&refresh_target_ruler},
{"reset_rules_cache",(const void*)&reset_rules_cache},
{"rules_cached_senior",(const void*)&rules_cached_senior},
{"rules_cached_database",(const void*)&rules_cached_database},
{"original_merit_current_rank",(const void*)&original_merit_current_rank},
{"hook_merit_current_rank",(const void*)&hook_merit_current_rank},
{"installed",(const void*)&installed},
{"rules_prepare_native",(const void*)&rules_prepare_native},
{"rules_native_ready",(const void*)&rules_native_ready},
{"rule_special",(const void*)&rule_special},
{"rule_assignment",(const void*)&rule_assignment},
{"rule_location",(const void*)&rule_location},
{"rule_rank_table",(const void*)&rule_rank_table},
{"original_merit_eligible",(const void*)&original_merit_eligible},
{"original_merit_rank",(const void*)&original_merit_rank},
{"hook_merit_eligible",(const void*)&hook_merit_eligible},
{"hook_merit_rank",(const void*)&hook_merit_rank},
{"hook_native_exit_event",(const void*)&hook_native_exit_event},
{"senior_rank",(const void*)&senior_rank},
{"sync_merit_history",(const void*)&sync_merit_history},
{"sync_all_merit_history",(const void*)&sync_all_merit_history},
{"r39_recruit_resets",(const void*)&r39_recruit_resets},
{"r39_recruit_removed",(const void*)&r39_recruit_removed},
{"history_profile_valid",(const void*)&history_profile_valid},
{"original_state_tick",(const void*)&original_state_tick},
{"ui_notification_depth",&ui_notification_depth},

{"__chkstk",(const void*)&__chkstk},
{"g_evaluate",(const void*)&g_evaluate},
{"g_grade",(const void*)&g_grade},
{"preview_for",(const void*)&preview_for},
{"ui_gstate",(const void*)&ui_gstate},
{"ui_target",(const void*)&ui_target},
{"preview_count",(const void*)&preview_count},
{"previews",(const void*)&previews},
{"previews_ready",(const void*)&previews_ready},
{"prepare_previews",(const void*)&prepare_previews},
{"ui_teardown",(const void*)&ui_teardown},
{"exit_modal",(const void*)&exit_modal},
{"exit_armed",(const void*)&exit_armed},
{"exit_choice",(const void*)&exit_choice},
{"exit_generation",(const void*)&exit_generation},
{"warning_kind",&warning_kind},{"replace_requested",&replace_requested},{"replace_approved",&replace_approved},
{"replacement_count",&replacement_count},{"choice_revision",&choice_revision},{"control_generation",&control_generation},
{"choice_read",&choice_read},{"choice_cancel_all",&choice_cancel_all},{"controls_dispatch_tick",&controls_dispatch_tick},
{"previews_validate",(const void*)&previews_validate},
{"apply_previews",(const void*)&apply_previews},
{"controls_action",(const void*)&controls_action},
{"dispatch_control",(const void*)&dispatch_control},
{"controls_ensure",(const void*)&controls_ensure},
{"controls_refresh",(const void*)&controls_refresh},
{"controls_destroy",(const void*)&controls_destroy},
{"controls_hide",(const void*)&controls_hide},
{"controls",(const void*)&controls},
{"controls_parent",(const void*)&controls_parent},
{"controls_owner_row",(const void*)&controls_owner_row},
{"controls_registered",(const void*)&controls_registered},
{"notice_lines",notice_lines},{"notice_text",notice_text},{"notice_healthy",&notice_healthy},{"picker_depth",&picker_depth},
{"details_expanded",(const void*)&details_expanded},
{"detail_scroll",(const void*)&detail_scroll},
{"detail_rows",(const void*)&detail_rows},
{"observations",(const void*)&observations},
{"calculation_count",(const void*)&calculation_count},
{"build_observation",(const void*)&build_observation},
{"observation_lines",(const void*)&observation_lines},
{"observation_line_count",(const void*)&observation_line_count},
{"inspected_id",(const void*)&inspected_id},
{"inspected_stage",(const void*)&inspected_stage},
{"finalizing",(const void*)&finalizing},
{"completed_state",(const void*)&completed_state},
{"native_presented",(const void*)&native_presented},
{"ui_epoch",(const void*)&ui_epoch},
{"control_invoke",(const void*)&control_invoke},

{"fx_reveal",(const void*)&fx_reveal},
{"fx_tick",(const void*)&fx_tick},
{"fx_clear",(const void*)&fx_clear},
{"target_read",(const void*)&target_read},
{"hook_ai_visit",(const void*)&hook_ai_visit},
{"hook_safe_flush",(const void*)&hook_safe_flush},
{"save_slot_key_ok",(const void*)&save_slot_key_ok},
{"load_slot_key_ok",(const void*)&load_slot_key_ok},
{"save_slot_key",(const void*)&save_slot_key},
{"load_slot_key",(const void*)&load_slot_key},
{"save_account_tag",(const void*)&save_account_tag},
{"load_account_tag",(const void*)&load_account_tag},
{"slot_identity",(const void*)&slot_identity},
{"slot_prepare",(const void*)&slot_prepare},
{"slot_complete",(const void*)&slot_complete},
{"slot_choose",(const void*)&slot_choose},
{"read_slot_unlocked",(const void*)&read_slot_unlocked},
{"slot_journal_active",(const void*)&slot_journal_active},
{"staged_slot",(const void*)&staged_slot},
{"validate_loaded_disk",(const void*)&validate_loaded_disk},

{"native_ui_ready",(const void*)&native_ui_ready},
{"shared_hover_show",(const void*)&shared_hover_show},
{"shared_hover_release",(const void*)&shared_hover_release},
{"bind_shared_hover",(const void*)&bind_shared_hover},
{"native_guard_check",(const void*)&native_guard_check},
{"native_account_id",(const void*)&native_account_id},
{"hover_merit_row",(const void*)&hover_merit_row},
{"thanks_count",(const void*)&thanks_count},
{"thanks_active",(const void*)&thanks_active},
{"ack_release_pending",(const void*)&ack_release_pending},
{"thanks_ids",(const void*)&thanks_ids},
{"thanks_ruler",(const void*)&thanks_ruler},

{"sidecar_collision",(const void*)&sidecar_collision},
{"save_active",(const void*)&save_active},
{"log_io_errors",(const void*)&log_io_errors},
{"clock_frequency",(const void*)&clock_frequency},
{"world_epoch",(const void*)&world_epoch},
{"active_save_hash_ok",(const void*)&active_save_hash_ok},
{"active_save_hash",(const void*)&active_save_hash},
{"visible_row_count",(const void*)&visible_row_count},
{"visible_rows",(const void*)&visible_rows},
{"installed_count",(const void*)&installed_count},
{"relay_page",(const void*)&relay_page},
{"start_logger",(const void*)&start_logger},
{"log_append",(const void*)&log_append},
{"game",(const void*)&game},
{"res",(const void*)&res},
{"core",(const void*)&core},
{"enabled",(const void*)&enabled},
{"profiles",(const void*)&profiles},
{"states",(const void*)&states},
{"save_profiles",(const void*)&save_profiles},
{"save_states",(const void*)&save_states},
{"metadata_ready",(const void*)&metadata_ready},
{"metadata_fault",(const void*)&metadata_fault},
{"generation",(const void*)&generation},
{"transaction",(const void*)&transaction},
{"pending_save",(const void*)&pending_save},
{"pending_ui",(const void*)&pending_ui},
{"pending_stream",(const void*)&pending_stream},
{"reveal_active",(const void*)&reveal_active},
{"reveal_done",(const void*)&reveal_done},
{"roll_started",(const void*)&roll_started},
{"roll_count",(const void*)&roll_count},
{"plan_count",(const void*)&plan_count},
{"plans",(const void*)&plans},
{"active_id",(const void*)&active_id},
{"active_stage",(const void*)&active_stage},
{"ui_mode",(const void*)&ui_mode},
{"selected_replacement",(const void*)&selected_replacement},
{"committed_session",(const void*)&committed_session},
{"paths",(const void*)&paths},
{"refresh_tables",(const void*)&refresh_tables},
{"capture_profiles",(const void*)&capture_profiles},
{"clear_state",(const void*)&clear_state},
{"hook_newgame",(const void*)&hook_newgame},
{"hook_load",(const void*)&hook_load},
{"hook_save_prep",(const void*)&hook_save_prep},
{"hook_save_end",(const void*)&hook_save_end},
{"write_sidecar",(const void*)&write_sidecar},
{"read_sidecar",(const void*)&read_sidecar},
{"hook_stream_open",(const void*)&hook_stream_open},
{"merit_write_file",(const void*)&merit_write_file},
{"merit_write_steam",(const void*)&merit_write_steam},
{"award_commit",(const void*)&award_commit},
{"hook_commit_player",(const void*)&hook_commit_player},
{"hook_commit_ai",(const void*)&hook_commit_ai},
{"hook_bonus",(const void*)&hook_bonus},
{"hook_confirm",(const void*)&hook_confirm},
{"hook_state_tick",(const void*)&hook_state_tick},
{"hook_row_refresh",(const void*)&hook_row_refresh},
{"hook_row_destroy",(const void*)&hook_row_destroy},
{"hook_row_tick",(const void*)&hook_row_tick},
{"hook_status",(const void*)&hook_status},
{"hook_ui_exit",(const void*)&hook_ui_exit},
{"hook_close",(const void*)&hook_close},
{"hook_button_event",(const void*)&hook_button_event},
{"prepare_hooks",(const void*)&prepare_hooks},
{"ensure_nine",(const void*)&ensure_nine},
{"support_ui",(const void*)&support_ui},
{"roll_sequence",(const void*)&roll_sequence},
{"session_id",(const void*)&session_id},
{"log_queue",(const void*)&log_queue},
{"log_lengths",(const void*)&log_lengths},
{"log_read",(const void*)&log_read},
{"log_write",(const void*)&log_write},
{"log_dropped",(const void*)&log_dropped},
{"log_thread",(const void*)&log_thread},
{"log_path",(const void*)&log_path},
{"record_event",(const void*)&record_event},
{"format_log_item",(const void*)&format_log_item},
{"log_kinds",(const void*)&log_kinds},
{"event_sequence",(const void*)&event_sequence},
{"log_worker",(const void*)&log_worker},
{"initialize_gstate",(const void*)&initialize_gstate},
{"target_read",(const void*)&target_read},
{"slot_load_snapshot",(const void*)&slot_load_snapshot},
{"slot_ref_valid",(const void*)&slot_ref_valid},
{"prior_slot",(const void*)&prior_slot},
{"paint_row",(const void*)&paint_row},
{"enable_native_tile",(const void*)&enable_native_tile},
{"capture_thanks_ruler",(const void*)&capture_thanks_ruler},
{"show_native_thanks",(const void*)&show_native_thanks},
{"perform_roll",(const void*)&perform_roll},
{"finish_result",(const void*)&finish_result},
{"begin_slot",(const void*)&begin_slot},
{"mc_type",(const void*)&mc_type},
{"mc_pool",(const void*)&mc_pool},
{"mc_draw",(const void*)&mc_draw},
{"mc_sha256",(const void*)&mc_sha256},
{"mc_uniform",(const void*)&mc_uniform},
{"mc_roll_index",(const void*)&mc_roll_index},
{"mc_profile",(const void*)&mc_profile},
{"g_roll",(const void*)&g_roll},
{"hover_module",(const void*)&hover_module},
{"hover_x_collect",(const void*)&hover_x_collect},
{"hover_x_info",(const void*)&hover_x_info},
{"hover_x_columns",(const void*)&hover_x_columns},
{"hover_x_column",(const void*)&hover_x_column},
{"display_x_mask",(const void*)&display_x_mask},
{"prepare_hover_x",(const void*)&prepare_hover_x},
{"rollback_hover_x",(const void*)&rollback_hover_x},
{"hover_x_patches",(const void*)&hover_x_patches},
{"hover_x_count",(const void*)&hover_x_count},
{"hover_x_relays",(const void*)&hover_x_relays},
{"hover_x_caption",(const void*)&hover_x_caption},
{"display_trait",(const void*)&display_trait},
{"install_display_calls",(const void*)&install_display_calls},
{"presets_x_overlay",(const void*)&presets_x_overlay},
{"presets_x_refresh",(const void*)&presets_x_refresh},
{"probe_presets_x",(const void*)&probe_presets_x},
{"presets_module",(const void*)&presets_module},
{"presets_x_relay",(const void*)&presets_x_relay},
{"presets_x_status",(const void*)&presets_x_status},
{"presets_x_count",(const void*)&presets_x_count},
{"presets_rollback_live",(const void*)&presets_rollback_live},
{"hook_ui_enter",(const void*)&hook_ui_enter},
{"closing_layout",(const void*)&closing_layout},
{"native_cancel_merit",(const void*)&native_cancel_merit},
{"g_free_slot",(const void*)&g_free_slot},
{"g_reconcile",(const void*)&g_reconcile},
{"g_params",(const void*)&g_params},
{"g_calc_counters",(const void*)&g_calc_counters},
{"g_distribution_cached",(const void*)&g_distribution_cached},
{"g_prepare",(const void*)&g_prepare},
{"reconcile_officer",(const void*)&reconcile_officer},
{"cancel_previews",(const void*)&cancel_previews},
{"cancel_requested",(const void*)&cancel_requested},
{"miss_modal",(const void*)&miss_modal},
{"miss_notice_tick",(const void*)&miss_notice_tick},
{"miss_notice_hide",(const void*)&miss_notice_hide},
{"g_accept",(const void*)&g_accept},
{"g_distribution",(const void*)&g_distribution},
{"g_init",(const void*)&g_init},
{"g_validate",(const void*)&g_validate},
{"g_next",(const void*)&g_next},
{"g_ai_reroll",(const void*)&g_ai_reroll},
{"merit_presentation_skip",&merit_presentation_skip},{"merit_presentation_resume",&merit_presentation_resume},{0,0}};return symbols;}
