/* Native merit events own the interaction. No global keyboard polling worker. */
typedef struct {u8*o,*node;u16 id;u8 before_top,after_top,original_byte,changed_byte,before_lower,after_lower;} AwardPlan;
typedef struct {u8*node;u8 before,after;u16 id;} AiChoicePatch;
static AwardPlan plans[MAX_OFFICERS];
static AiChoicePatch ai_choice_patches[MAX_OFFICERS];
static u32 plan_count,ai_choice_patch_count;
static void*active_state;
static u16 active_id=NO_TRAIT,selected_replacement[MAX_OFFICERS];
static u8 active_stage;
static int ui_mode; /* 0 idle, 1 choose inheritance, 2 spin, 3 result */
static int cancel_requested,miss_modal,ui_teardown,picker_depth;
static int exit_back_requested,exit_choice_wait;
static void *closing_layout,*detached_layout;
static int cancel_delivered;
static int exit_modal,exit_choice,exit_armed,exit_approved,exit_resume_miss;
static int warning_kind,replace_requested,replace_approved,prepared_valid;
static u64 choice_revision,replace_revision;
typedef struct {u16 id,old_trait,new_trait;u8 stage,pad;} ReplacementNotice;
static ReplacementNotice replacements[192];static u32 replacement_count,replacement_index;
static u32 exit_generation;
static u64 exit_pause_started;
static u64 animation_start,last_caption;
static u32 active_keys;
static void*preselect_owner,*preselect_button;
static int committed_session;
static u8 deferred_slots[MAX_OFFICERS];
static u16 thanks_ids[MAX_OFFICERS],thanks_ruler=NO_TRAIT;
static u32 thanks_count;
static int thanks_active,ack_release_pending;
static void queue_thanks(u16);
static void capture_thanks_ruler(void*);
static void show_native_thanks(void*);
static void enable_native_tile(u8*,u32,int,int);
static void hover_merit_row(void*);
static int address_in_module(HANDLE,usize);
static void fx_clear(void);
static void fx_begin(int);
static void fx_reveal(void);
static void fx_tick(void);
static void fx_release_row(void*);
static void miss_notice_tick(void);
static void miss_notice_hide(void);
static int exit_notice_show(void);
static void exit_notice_hide(void);
static void exit_notice_resume(void);
static int exit_notice_healthy(void);
static void controls_suspend(void);
static int native_cancel_merit(void*);
static int native_consume_cancel_input(void);
static void paint_visible(void);
static void merit_status(void*,void*,int*);
static void paint_row(void*);
static void set_visible(void*p,int value){if(p&&rd(p,8))V(p,0xc8,Flag)(p,value);}
static void button_text(void*p,const u16*s,int enable,int color){if(!rd(p,0x138))return;V(p,0x1d8,Text)(p,s);V(p,0xd0,Flag)(p,enable);set_visible(p,1);if(color>=0)CALL(F_CAPTION_COLOR,void(*)(void*,int,int))(p+0x130,0x10,color);}
static u32 reveal_keys(void){u32 pid=0;HANDLE w=GetForegroundWindow();if(!w||!GetWindowThreadProcessId(w,&pid)||pid!=GetCurrentProcessId())return 0;return ((GetAsyncKeyState(1)&0x8000)?1u:0u)|((GetAsyncKeyState(13)&0x8000)?2u:0u)|((GetAsyncKeyState(27)&0x8000)?4u:0u);}
static int entropy(u64*out){u32 a,b;if(!random32(0,&a)||!random32(0,&b))return 0;*out=((u64)a<<32)|b;return 1;}
static void record_event(const char*,const GTarget*,const GState*,const GState*,u32,const GDistribution*,int,double);
/* Preview without changing native rank/cost/traits. R5 reserves spent personal
 * rerolls immediately; only bottom Decision commits. Back discards results. */
#define PREVIEW_CAP 512 /* R40 A12: was 64; large forces could not use the 65th+ upper-reward tile */
typedef struct {u16 id;u8 used,authorized,new_mask,ready;u8 original_top,original_lower;u16 original_flags;GTarget original,target;GState before,after;u16 replacement[3];void*native_owner;} Preview;
static Preview previews[PREVIEW_CAP];static u32 preview_count;
static Preview prepared_previews[PREVIEW_CAP]; /* UI transaction scratch, not saved. */
static u8 native_presented[MAX_OFFICERS];
static int finalizing,final_commit_seen;
static void*completed_state;
static u16 inspected_id=NO_TRAIT;static u8 inspected_stage;static int inspected_available;
static u64 ui_epoch;
static void controls_refresh(void);
static void controls_dispatch_tick(void);
static void controls_input_tick(void*);
static int controls_ensure(u8*);
static void controls_hide(void);
static void controls_destroy(void*);
static void*controls_owner_row;
static u16 control_hover_trait(void*,u8*,const int*);
static void capture_snapshot(const GTarget*,const GState*,const GState*,u32,const GDistribution*,double);
static void accept_snapshot(u16,u32,const GState*);
static void snapshot_reset(void);
static Preview*preview_for(u16 id){u32 i;for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used&&previews[i].id==id)return &previews[i];return 0;}
static GState*ui_gstate(u16 id){Preview*p=preview_for(id);return p?&p->after:&states[id];}
static void ui_target(u8*o,GTarget*t){Preview*p=preview_for(S(o,OFF(O_OFFICER_ID)));if(p)*t=p->target;else target_read(o,t);}
static int ui_has_uncommitted(void){return preview_count!=0;}
static void preview_reset(void){prepared_valid=replace_requested=replace_approved=0;replacement_count=replacement_index=0;choice_revision++;mc_zero(prepared_previews,sizeof(prepared_previews));mc_zero(previews,sizeof(previews));preview_count=0;completed_state=0;finalizing=final_commit_seen=0;snapshot_reset();}
static Preview*preview_create(u8*o){u16 id=S(o,OFF(O_OFFICER_ID));Preview*p=preview_for(id);u32 i;if(p)return p;reconcile_officer(o);repair_pending(o);
 for(i=0;i<PREVIEW_CAP;i++)if(!previews[i].used){p=&previews[i];mc_zero(p,sizeof(*p));p->id=id;p->used=1;p->original_top=B(o,OFF(O_UPPER));p->original_lower=B(o,OFF(O_LOWER));p->original_flags=S(o,OFF(O_FLAGS));target_read(o,&p->original);p->target=p->original;p->before=p->after=states[id];preview_count++;return p;}return 0;
}
#include "native_selection.inc"
static int previews_ready(void){Preview*a=prepared_valid?prepared_previews:previews;u32 i;for(i=0;i<PREVIEW_CAP;i++)if(a[i].used&&a[i].authorized!=a[i].ready)return 0;return 1;}
static int preview_has_result(void){u32 i,j;for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used)for(j=0;j<3;j++)if((previews[i].authorized&(1u<<j))&&previews[i].after.slots[j].phase>=G_PENDING)return 1;return 0;}
static void drop_unrolled_previews(void){u32 i,j;
 for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used){Preview*p=&previews[i];
  for(j=0;j<3;j++)if(p->after.slots[j].phase==G_EMPTY)p->authorized&=(u8)~(1u<<j);
  if(!p->authorized){p->used=0;preview_count--;}
 }
}
/* The removed toolbar acceptance is now owned by the ORIGINAL bottom decision.
 * Prepare copies of every officer independently; if any result is invalid,
 * none of the live previews, traits, X masks or passion states are changed. */
static int prepare_previews(void){u32 i,j;int ok=1;
 prepared_valid=0;replacement_count=0;
 if(save_active||metadata_fault||!__sync_bool_compare_and_swap(&transaction,0,1))return 0;
 AcquireSRWLockExclusive(&state_lock);refresh_trait_rules();mc_copy(prepared_previews,previews,sizeof(previews));
 for(i=0;i<PREVIEW_CAP&&ok;i++)if(prepared_previews[i].used){Preview*p=&prepared_previews[i];Preview*live=&previews[i];u8*o=officer(p->id);MeritChoice c;
  if(!o||!choice_read(p->native_owner,o,&c)){ok=0;break;}
  p->target=p->original;p->after=p->before;p->ready=p->authorized=p->new_mask=0;
  for(j=0;j<3;j++)if((live->authorized&(1u<<j))&&(j<c.earned_top||j<c.top)){
   GSlot x=live->after.slots[j];if(x.phase<G_PENDING){ok=0;break;}
   u32 spent=x.rerolls-p->before.slots[j].rerolls;if(spent>p->after.cancelled_rerolls){ok=0;break;}
   p->after.cancelled_rerolls-=(u8)spent;x.phase=G_PENDING;p->after.slots[j]=x;
   int decline=x.outcome!=G_MISS&&x.full&&x.star&&p->replacement[j]==NO_TRAIT;
   if(x.outcome!=G_MISS&&!decline&&g_free_slot(&p->target,&p->after)<0){
    if(replacement_count>=192){ok=0;break;}ReplacementNotice*n=&replacements[replacement_count++];
    n->id=p->id;n->stage=(u8)j;n->old_trait=x.star?p->replacement[j]:x.replace_trait;n->new_trait=x.result;
   }
   if(g_accept(&p->target,&p->after,j,p->replacement[j],decline)!=G_OK){ok=0;break;}
   p->authorized|=(u8)(1u<<j);p->ready|=(u8)(1u<<j);if(j>=c.earned_top)p->new_mask|=(u8)(1u<<j);
  }
  /* Native auto/military selection must not grant an unrolled trait silently. */
  for(j=c.earned_top;j<c.top;j++)if(!(p->authorized&(1u<<j))){ok=0;break;}
  if(!g_validate(&p->after))ok=0;
 }
 if(!ok){replacement_count=0;mc_zero(prepared_previews,sizeof(prepared_previews));}
 prepared_valid=ok;ReleaseSRWLockExclusive(&state_lock);transaction=0;return ok;
}
/* Normal confirmed cancellation and emergency world/layout detach.
 * Spent personal tickets are never refilled by discarding an unsafe preview. */
static void cancel_previews(void){u32 i,j;
 for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used){Preview*p=&previews[i];
 for(j=0;j<3;j++)if(p->after.slots[j].phase&&p->after.slots[j].outcome==G_MISS&&(p->authorized&(1u<<j)))record_event("miss_rerolled_or_cancelled",&p->original,&p->before,&p->after,j,0,0,0);
 if(mc_equal(&states[p->id],&p->before,sizeof(GState))){u8 used=p->after.rerolls_used-p->before.rerolls_used;states[p->id].rerolls_used+=used;states[p->id].cancelled_rerolls+=used;}
 }
 preview_reset();cancel_requested=0;generation++;
}
static int preview_valid_one(u32 i,int committed){u32 j;
 Preview*p=prepared_valid?&prepared_previews[i]:&previews[i];u8*o=officer(p->id);GTarget t;if(!o)return 0;target_read(o,&t);
 if(!mc_equal(t.traits,p->original.traits,18)||!mc_equal(t.stats,p->original.stats,5)||t.star!=p->original.star||t.sex!=p->original.sex||t.father!=p->original.father||t.mother!=p->original.mother||!mc_equal(&states[p->id],&p->before,sizeof(GState)))return 0;
 if(p->authorized!=p->ready)return 0;
 if(!committed&&(B(o,OFF(O_UPPER))!=p->original_top||B(o,OFF(O_LOWER))!=p->original_lower))return 0;
 if(committed)for(j=0;j<3;j++)if((p->authorized&(1u<<j))&&B(o,OFF(O_UPPER))<=j)return 0;
 return 1;}
static int previews_validate(int committed){u32 i;for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used&&!preview_valid_one(i,committed))return 0;return 1;}
/* R40 A12: a preview that no longer matches the native commit is dropped for that
   officer only (no speculative trait write for him); all other officers apply. */
static void apply_previews(void){u32 i,j,dropped=0;
 for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used&&!preview_valid_one(i,1)){previews[i].used=0;prepared_previews[i].used=0;if(preview_count)preview_count--;dropped++;}
 if(dropped)report("MeritTraits R40: native commit differs from preview for some officers; only those officers skipped, others applied.");
 for(i=0;i<PREVIEW_CAP;i++)if(previews[i].used){Preview*p=&prepared_previews[i];u8*o=officer(p->id);
  states[p->id]=p->after;if(p->authorized){mc_copy(o+OFF(O_TRAITS),p->target.traits,18);S(o,OFF(O_FLAGS))|=0x8000;}
  for(j=0;j<3;j++)if(p->ready&(1u<<j)){GSlot*x=&p->after.slots[j];profiles[p->id].awards[j]=x->result;profiles[p->id].award_mask|=(u8)(1u<<j);record_event(x->outcome==G_MISS?"miss_committed":x->phase==G_DECLINED?"decline":"confirm",&p->original,&p->before,&p->after,j,0,0,0);if(x->outcome==G_MISS)record_event("passion_awarded",&p->original,&p->before,&p->after,j,0,0,0);else if(x->phase==G_ACCEPTED&&x->passion_applied)record_event("passion_consumed",&p->original,&p->before,&p->after,j,0,0,0);accept_snapshot(p->id,j,&p->after);}
  if(p->authorized)queue_thanks(p->id);p->used=0;previews[i].used=0;
 }
 preview_count=0;prepared_valid=0;generation++;
}
/* R13: ACCEPTED in a Preview is only a dependency snapshot, not a native
 * award. Rebuild from the original target and the persistent ticket ledger,
 * keeping every other revealed result/seed/roll ID. No later draw is repeated.
 * The highest revealed slot remains pending, matching the existing wire rules.
 */
static int preview_recompose(Preview*out,const Preview*src,u32 changed,const GSlot*replacement,int reserve){
 u32 j,last=0;*out=*src;out->target=src->original;out->after=src->before;out->ready=0;
 if(reserve){if(out->after.rerolls_used>=G_REROLLS)return 0;out->after.rerolls_used++;out->after.cancelled_rerolls++;out->before=out->after;}
 for(j=0;j<3;j++)if(src->authorized&(1u<<j))last=j;
 for(j=0;j<3;j++)if(src->authorized&(1u<<j)){
  GSlot x=j==changed?*replacement:src->after.slots[j];u32 spent;
  if(x.phase<G_PENDING||x.rerolls<src->before.slots[j].rerolls)return 0;
  spent=x.rerolls-src->before.slots[j].rerolls;if(spent>out->after.cancelled_rerolls)return 0;
  out->after.cancelled_rerolls-=(u8)spent;x.phase=G_PENDING;out->after.slots[j]=x;
  if(j!=last){int decline=x.outcome!=G_MISS&&x.full&&x.star&&src->replacement[j]==NO_TRAIT;
   if(g_accept(&out->target,&out->after,j,src->replacement[j],decline)!=G_OK)return 0;out->ready|=(u8)(1u<<j);
  }
 }
 if(!g_validate(&out->after))return 0;
 if(src->authorized&(1u<<last)){GTarget final_target=out->target;GState final_state=out->after;GSlot*x=&final_state.slots[last];
  int decline=x->outcome!=G_MISS&&x->full&&x->star&&src->replacement[last]==NO_TRAIT;
  if(g_accept(&final_target,&final_state,last,src->replacement[last],decline)!=G_OK||!g_validate(&final_state))return 0;
 }
 return 1;
}
static int preview_roll_prefix(const Preview*p,u32 stage,GTarget*t,GState*s){
 *t=p->original;*s=p->before;
 for(u32 j=0;j<=stage;j++)if(p->authorized&(1u<<j)){
  GSlot x=p->after.slots[j];u32 spent;if(x.phase<G_PENDING||x.rerolls<p->before.slots[j].rerolls)return 0;
  spent=x.rerolls-p->before.slots[j].rerolls;if(spent>s->cancelled_rerolls)return 0;
  s->cancelled_rerolls-=(u8)spent;x.phase=G_PENDING;s->slots[j]=x;
  if(j<stage){int decline=x.outcome!=G_MISS&&x.full&&x.star&&p->replacement[j]==NO_TRAIT;
   if(g_accept(t,s,j,p->replacement[j],decline)!=G_OK)return 0;
  }
 }
 return s->slots[stage].phase==G_PENDING;
}
/* Reject edits to the live officer before spending RNG/tickets or accepting a
   held preview. Original arrays remain authoritative through final commit. */
static int preview_current(u8*o,const Preview*p){GTarget t;target_read(o,&t);return mc_equal(&t,&p->original,sizeof(t));}
static int preview_reroll_locked(u8*o,u32 stage,Preview*p){
 u16 id=p->id;GTarget t;GState before,after;MeritChoice c;Preview trial,next;GSlot probe;
 u8 candidates[MAX_TRAITS];u32 j,n=0;u64 seed,start=clock_start();int allow_miss;
 if(!preview_current(o,p)||!(p->authorized&(1u<<stage))||states[id].slots[stage].phase>=G_ACCEPTED||
    !mc_equal(&states[id],&p->before,sizeof(GState))||p->after.rerolls_used>=G_REROLLS||
    !choice_read(p->native_owner,o,&c)||!(stage<c.earned_top||stage<c.top)||
    !preview_roll_prefix(p,stage,&t,&before))return 0;
 /* Reserve fixed later results and validate physical-slot/replacement/MISS
    dependencies BEFORE entropy. Impossible candidates cannot cause a free
    visible retry, erase a later outcome, or silently change its replacement. */
 mc_copy(candidates,installed_colors,sizeof(candidates));probe=before.slots[stage];
 probe.phase=G_PENDING;probe.rerolls++;probe.full=g_free_slot(&t,&before)<0;probe.star=t.star;
 probe.anomaly=probe.transcend=0;probe.passion_applied=before.passion;probe.x_index=G_NO_X_SLOT;
 probe.outcome=G_TRAIT;probe.raw=probe.weight=probe.quality=1.0;
 for(j=1;j<MAX_TRAITS;j++)if(candidates[j]){
  probe.result=(u16)j;probe.cost=g_traits[j].color==3?g_params.gold_cost:g_params.blue_cost;
  int blocked=0;for(u32 k=stage+1;k<3;k++)if((p->authorized&(1u<<k))&&p->after.slots[k].outcome==G_TRAIT&&
    (g_blocks(p->after.slots[k].result,(u16)j)||g_blocks((u16)j,p->after.slots[k].result)))blocked=1;
  if(blocked||!preview_recompose(&trial,p,stage,&probe,1))candidates[j]=0;else n++;
 }
 probe.outcome=G_MISS;probe.result=0;probe.raw=probe.quality=probe.cost=0;probe.passion_applied=0;
 allow_miss=!before.passion&&preview_recompose(&trial,p,stage,&probe,1);
 if(!n&&!allow_miss)return 0;
 after=before;if(!entropy(&seed))return 0;
 if(g_roll_merit_filtered(&t,&after,stage,candidates,seed,session_id+roll_sequence+1,p->replacement[stage],1,&distribution,3-c.lower,allow_miss)!=G_OK)return 0;
 if(!preview_recompose(&next,p,stage,&after.slots[stage],1)){report("MeritTraits R13: PREVIEW_REPLAY_REJECTED; no result or ticket changed.\r\n");return 0;}
 states[id].rerolls_used++;states[id].cancelled_rerolls++;next.before=states[id];*p=next;
 generation++;choice_revision++;prepared_valid=replace_approved=0;
 double elapsed=clock_elapsed(start);capture_snapshot(&t,&before,&p->after,stage,&distribution,elapsed);
 if(before.slots[stage].outcome==G_MISS)record_event("miss_rerolled_or_cancelled",&t,&before,&before,stage,0,0,0);
 record_event("reroll",&t,&before,&p->after,stage,&distribution,0,elapsed);
 n=0;for(j=1;j<MAX_TRAITS;j++)if(distribution.weight[j]>0)roll_pool[id][stage][n++]=(u16)j;roll_count[id][stage]=(u16)n;
 return 1;
}
/* Only accessed while state_lock is held; cleared at each AI officer's award. */
static GDistributionCache ai_distribution_cache;
static int perform_roll_locked(u8*o,u32 stage,int reroll,int ai){
 u16 id=S(o,OFF(O_OFFICER_ID));Preview*p=ai?0:preview_for(id);GState*current=p?&p->after:&states[id];
 GTarget t;GState before=*current,after=before;u64 seed,start=clock_start();int r;
 if(metadata_fault||stage>2||(p&&!preview_current(o,p))||(p&&!mc_equal(&states[id],&p->before,sizeof(GState)))||(B(o,OFF(O_UPPER))<=stage&&(!p||!(p->authorized&(1u<<stage)))))return 0;
 if(!ai)refresh_trait_rules();
 if(p&&reroll)return preview_reroll_locked(o,stage,p);
 if(reroll&&(current->slots[stage].phase!=G_PENDING||current->rerolls_used>=G_REROLLS))return 0;
 if(p)t=p->target;else target_read(o,&t);if(!entropy(&seed))return 0;
 u32 max_upper=3-B(o,OFF(O_LOWER));
 if(p){MeritChoice c;if(!choice_read(p->native_owner,o,&c))return 0;max_upper=3-c.lower;}
 if(ai)r=g_roll_merit_cached(&t,&after,stage,installed_colors,seed,session_id+roll_sequence+1,selected_replacement[id],reroll,&distribution,max_upper,&ai_distribution_cache);
 else r=g_roll_merit(&t,&after,stage,installed_colors,seed,session_id+roll_sequence+1,p?p->replacement[stage]:selected_replacement[id],reroll,&distribution,max_upper);
 if(r!=G_OK)return 0;
 /* Charge a successful reroll to this officer immediately. Until the preview
    commits, cancelled_rerolls is the persistent reservation; committing after
    replaces it with the slot's actual reroll count, never charges twice. */
 if(p&&reroll){states[id].rerolls_used++;states[id].cancelled_rerolls++;p->before=states[id];generation++;}
 choice_revision++;prepared_valid=replace_approved=0;if(!reroll)roll_sequence++;if(reroll&&before.slots[stage].outcome==G_MISS)record_event("miss_rerolled_or_cancelled",&t,&before,&before,stage,0,ai,0);*current=after;double elapsed=clock_elapsed(start);
 capture_snapshot(&t,&before,&after,stage,&distribution,elapsed);
 record_event(after.slots[stage].outcome==G_MISS?"miss_previewed":reroll?"reroll":p?"preview":"unlock",&t,&before,&after,stage,&distribution,ai,elapsed);
 u32 n=0,j;for(j=1;j<MAX_TRAITS;j++)if(distribution.weight[j]>0)roll_pool[id][stage][n++]=(u16)j;roll_count[id][stage]=(u16)n;
 return 1;
}
static int perform_roll(u8*o,u32 stage,int reroll,int ai){int result;
 if(save_active||!enabled||!rd(o,OFF(O_LOWER)+1)||S(o,OFF(O_OFFICER_ID))>=MAX_OFFICERS||stage>2||!__sync_bool_compare_and_swap(&transaction,0,1))return 0;
 AcquireSRWLockExclusive(&state_lock);result=perform_roll_locked(o,stage,reroll,ai);ReleaseSRWLockExclusive(&state_lock);transaction=0;return result;
}
static int finish_result_locked(u8*o,u32 stage,int decline,int ai){
 u16 id=S(o,OFF(O_OFFICER_ID));Preview*p=ai?0:preview_for(id);GState*current=p?&p->after:&states[id];
 GTarget t,original;GState before=*current,after=before;if(p)t=p->target;else target_read(o,&t);original=t;
 if(metadata_fault||stage>2||(p&&!preview_current(o,p))||(B(o,OFF(O_UPPER))<=stage&&(!p||!(p->authorized&(1u<<stage)))))return 0;
 if(g_accept(&t,&after,stage,p?p->replacement[stage]:selected_replacement[id],decline)!=G_OK)return 0;
 *current=after;accept_snapshot(id,stage,&after);
 if(p){p->target=t;p->ready|=(u8)(1u<<stage);record_event("selection_confirm",&original,&before,&after,stage,0,0,0);return 1;}
 mc_copy(o+OFF(O_TRAITS),t.traits,18);S(o,OFF(O_FLAGS))|=0x8000;
 profiles[id].awards[stage]=after.slots[stage].result;profiles[id].award_mask|=(u8)(1u<<stage);
 record_event(after.slots[stage].outcome==G_MISS?"miss_committed":decline?"decline":"confirm",&original,&before,&after,stage,0,ai,0);if(after.slots[stage].outcome==G_MISS)record_event("passion_awarded",&original,&before,&after,stage,0,ai,0);else if(!decline&&after.slots[stage].passion_applied)record_event("passion_consumed",&original,&before,&after,stage,0,ai,0);if(!ai)queue_thanks(id);generation++;return 1;
}
static int finish_result(u8*o,u32 stage,int decline,int ai){int result;
 if(save_active||!enabled||!rd(o,OFF(O_LOWER)+1)||S(o,OFF(O_OFFICER_ID))>=MAX_OFFICERS||stage>2||!__sync_bool_compare_and_swap(&transaction,0,1))return 0;
 AcquireSRWLockExclusive(&state_lock);result=finish_result_locked(o,stage,decline,ai);ReleaseSRWLockExclusive(&state_lock);transaction=0;return result;
}
static void ai_process(u8*o){GTarget t;u32 j,id=S(o,OFF(O_OFFICER_ID));ensure_profile(o,2);
 ai_distribution_cache.valid=0;repair_pending(o);
 for(j=0;j<B(o,OFF(O_UPPER))&&j<3;j++){
  if(states[id].slots[j].phase==G_ACCEPTED||states[id].slots[j].phase==G_DECLINED)continue;
  target_read(o,&t);g_reconcile(&t,&states[id]);
  /* Re-read ACTUAL occupancy after each accepted trait or X. AI must never
     inherit/replace an incumbent, including stars, rulers and old pending slots. */
  if(g_free_slot(&t,&states[id])<0){report("MeritTraits R11: AI_FULL_NO_DRAW_NO_REPLACEMENT\r\n");break;}
  selected_replacement[id]=0;
  if(states[id].slots[j].phase==G_EMPTY&&!perform_roll_locked(o,j,0,1))break;
  while(g_ai_reroll(&states[id],j)){GState before=states[id];record_event("ai_reroll_decision",&t,&before,&before,j,0,1,0);if(!perform_roll_locked(o,j,1,1))break;}
  if(!finish_result_locked(o,j,0,1))break;
 }
}
/* Candidate visits are qualification checks, NEVER award/roll callbacks.
 * Only the already-verified native AI commit site can start automatic draws.
 * This also protects directly controlled officers from incidental AI visits. */
static int hook_ai_visit(void*person){u8*o=person;PERF_HIT(PERF_AI);
 if(enabled&&!metadata_fault&&!save_active&&native_query_person(o)&&S(o,OFF(O_OFFICER_ID))<MAX_OFFICERS&&CALL(F_VALID,int(*)(void*))(o)){
  if(!metadata_ready)capture_profiles(2);else refresh_target_ruler(o,2);
  if(!transaction)sync_merit_history_checked(o,S(o,OFF(O_OFFICER_ID)));
  if(ruler_veteran_id(S(o,OFF(O_OFFICER_ID))))return senior_rank()>0&&rule_available_checked(o);
 }
 return NCALL(N_AI_ELIGIBLE,int(*)(void*))(person);
}
static int ai_free_trait_slots(u8*o){GTarget t;u16 id=S(o,OFF(O_OFFICER_ID));u32 k;int free=0;target_read(o,&t);g_reconcile(&t,&states[id]);
 for(k=0;k<9;k++)if((!t.traits[k]||t.traits[k]==NO_TRAIT)&&!(states[id].x_mask&(1u<<k)))free++;return free;
}
/* Rewrite only this native AI batch record.  Previously earned upper/lower
   counts are immutable and the total number of new ordinary choices stays the
   same.  Free trait slots consume new choices first; once no slot remains the
   rest stay/become lower-row rewards.  Ruler-veterans expose their persistent
   native-equivalent capacity of three without receiving any pre-awarded slot. */
/* R40 A12: AI officers skipped in the current AI batch (native game handles them, no gacha). */
static u16 r40_skip_ids[64];static u32 r40_skip_n,r40_ai_skips_total;
static int r40_ai_skipped(u16 id){for(u32 i=0;i<r40_skip_n;i++)if(r40_skip_ids[i]==id)return 1;return 0;}
static void r40_ai_skip(u16 id){if(!r40_ai_skipped(id)&&r40_skip_n<64)r40_skip_ids[r40_skip_n++]=id;r40_ai_skips_total++;if(r40_ai_skips_total<=3)report("MeritTraits R40: one AI officer left to the native award (no gacha); the rest of the AI batch commits.");}
static int ai_policy_walk(u8*node,u8*head,u32 depth){u8*o,packed,after;u16 id;u32 top,lower,earned_top,earned_lower,total,new_count,new_top,new_lower;int free;
 if(node==head)return 1;if(depth>64||!rd(node,0x21))return 0;if(B(node,0x19))return 1;
 if(!ai_policy_walk(P(node,0),head,depth+1))return 0;
 id=(u16)D(node,0x1c);o=officer(id);if(o){refresh_target_ruler(o,2);sync_merit_history_checked(o,id);if(metadata_fault)return 0;packed=B(node,0x20);top=packed&15;lower=packed>>4;earned_top=B(o,OFF(O_UPPER));earned_lower=B(o,OFF(O_LOWER));
  if(top>3||lower>3||top<earned_top||lower<earned_lower||top+lower>3||earned_top+earned_lower>3){r40_ai_skip(id);return ai_policy_walk(P(node,0x10),head,depth+1);}
  ensure_profile(o,2);total=top+lower;if(ruler_veteran(o)&&total<3)total=3;if(total<earned_top+earned_lower||total>3){r40_ai_skip(id);return ai_policy_walk(P(node,0x10),head,depth+1);}
  new_count=total-earned_top-earned_lower;free=ai_free_trait_slots(o);new_top=(u32)free<new_count?(u32)free:new_count;if(new_top>3-earned_top)new_top=3-earned_top;
  new_lower=new_count-new_top;if(earned_lower+new_lower>3){r40_ai_skip(id);return ai_policy_walk(P(node,0x10),head,depth+1);}after=(u8)((earned_top+new_top)|((earned_lower+new_lower)<<4));
  if(after!=packed){if(ai_choice_patch_count>=MAX_OFFICERS){r40_ai_skip(id);return ai_policy_walk(P(node,0x10),head,depth+1);}ai_choice_patches[ai_choice_patch_count++]=(AiChoicePatch){node,packed,after,id};B(node,0x20)=after;}
 }
 return ai_policy_walk(P(node,0x10),head,depth+1);
}
static void ai_policy_restore(void){while(ai_choice_patch_count){AiChoicePatch*p=&ai_choice_patches[--ai_choice_patch_count];if(rd(p->node,0x21))B(p->node,0x20)=p->before;}}
static int award_rules_refreshed;
static int walk_tree(u8*node,u8*head,u32 depth,int ai){u8*o;u16 id;u8 packed,top;AwardPlan*p;(void)ai;
 if(node==head)return 1;if(depth>64||!rd(node,0x21))return 0;if(B(node,0x19))return 1;
 if(!walk_tree(P(node,0),head,depth+1,ai))return 0;
 id=(u16)D(node,0x1c);o=officer(D(node,0x1c));if(o){if(visited[id])return 0;visited[id]=1;sync_merit_history_checked(o,id);if(metadata_fault)return 0;
  /* R40 A12: in an AI batch one unsuitable officer is left to the native game (no gacha for him); the rest commit. */
  if(!CALL(F_VALID,int(*)(void*))(o)||!CALL(F_ELIGIBLE,int(*)(void*))(o)){if(ai){r40_ai_skip(id);goto next;}return 0;}
  packed=B(node,0x20);top=packed&15;if(top<B(o,OFF(O_UPPER))||top>3||top+(packed>>4)>3||(packed>>4)<B(o,OFF(O_LOWER))){if(ai){r40_ai_skip(id);goto next;}return 0;}
  if(ai&&r40_ai_skipped(id))goto next;
  if(!ai)native_presented[id]=1;
  if(top>B(o,OFF(O_UPPER))){
   /* Empty/domestic-only AI commits need neither trait rule scans nor RNG. */
   if(ai&&!award_rules_refreshed){refresh_trait_rules();award_rules_refreshed=1;}
   if(!ai){Preview*live=preview_for(id);if(!finalizing||!prepared_valid||!live)return 0;
    Preview*ready=&prepared_previews[live-previews];for(u32 j=B(o,OFF(O_UPPER));j<top;j++)if(!(ready->ready&(1u<<j)))return 0;
   }
   GTarget t;ensure_profile(o,2);target_read(o,&t);if(metadata_fault||plan_count>=MAX_OFFICERS)return 0;
   if(!preview_for(id)&&!g_has_pool(&t,&states[id],installed_colors)){if(ai){r40_ai_skip(id);goto next;}return 0;}

  }
  if(plan_count>=MAX_OFFICERS)return 0;
  p=&plans[plan_count++];p->o=o;p->node=node;p->id=id;p->before_top=B(o,OFF(O_UPPER));p->after_top=top;p->before_lower=B(o,OFF(O_LOWER));p->after_lower=packed>>4;p->original_byte=p->changed_byte=packed;
 }
 next:
 return walk_tree(P(node,0x10),head,depth+1,ai);
}
static int award_commit(void*batch,int ai){u8*head;u32 i;int result;
 if(save_active||metadata_fault||!enabled||!__sync_bool_compare_and_swap(&transaction,0,1))return 0;
 AcquireSRWLockExclusive(&state_lock);if(!metadata_ready)capture_profiles(2);award_rules_refreshed=0;plan_count=0;ai_choice_patch_count=0;r40_skip_n=0;if(!ai){thanks_count=0;mc_zero(native_presented,sizeof(native_presented));}mc_zero(visited,sizeof(visited));
 if(metadata_fault||!rd(batch,0x18)||Q(batch,8)>MAX_OFFICERS||!(head=P(batch,0))||!rd(head,0x21)||(ai&&!ai_policy_walk(P(head,8),head,0))||!walk_tree(P(head,8),head,0,ai)){
  if(ai)ai_policy_restore();report("MeritTraits R10: native award not committed: invalid batch or policy rewrite.");ReleaseSRWLockExclusive(&state_lock);transaction=0;return 0;
 }
 int needs_entropy=0;for(i=0;i<plan_count;i++)if(plans[i].after_top>plans[i].before_top)needs_entropy=1;
 u64 probe_seed;if(needs_entropy&&(ai||!preview_count)&&!entropy(&probe_seed)){if(ai)ai_policy_restore();ReleaseSRWLockExclusive(&state_lock);transaction=0;report("MeritTraits RC4: unavailable system RNG; native award left uncommitted.");return 0;}
 ReleaseSRWLockExclusive(&state_lock);result=CALL(F_COMMIT,int(*)(void*))(batch);AcquireSRWLockExclusive(&state_lock);if(ai)ai_policy_restore();
 if(result){u32 diff=0;for(i=0;i<plan_count;i++){AwardPlan*p=&plans[i];u8*o=officer(p->id);if(o!=p->o||B(o,OFF(O_UPPER))!=p->after_top||B(o,OFF(O_LOWER))!=p->after_lower){diff++;continue;}if(ai&&p->after_top>p->before_top)ai_process(o);}
  if(diff){static u32 n;if(++n<=3)report("MeritTraits R40: native award result differs for some officers; those skipped, others processed.");}
  for(i=0;i<plan_count;i++)sync_merit_history(officer(plans[i].id));generation++;if(!ai&&finalizing&&!metadata_fault){final_commit_seen=1;if(preview_count)apply_previews();}}
 ReleaseSRWLockExclusive(&state_lock);transaction=0;return result;
}
static int hook_commit_player(void*b){return award_commit(b,0);}
static int hook_commit_ai(void*b){
 if(active_state||player_depth||confirming_ui||finalizing||preview_count){report("MeritTraits R11: AI_COMMIT_BLOCKED_DURING_DIRECT_MERIT_UI\r\n");return 0;}
 return award_commit(b,1);
}
static int hook_bonus(void*person,int kind){int native=CALL(F_BONUS,int(*)(void*,int))(person,kind),deduct=0;u8*o=person;u32 j;PERF_HIT(PERF_BONUS);if(!enabled||!metadata_ready||!bonus_cache_ready||!o)return native;
 /* The original returns early for ALL current rulers: a built-in +2 territory
    bonus, zero other kinds, without reading earned lower rewards. Only restore
    the earned domestic kinds omitted by that branch. Territory already has its
    native maximum (three entries in the original vector/UI); never stack more.
    Probe the original kind-2 result so special/faction/0x4000 exclusions remain
    authoritative. Current status, not ever-ruler history, prevents double grants
    after succession/demotion. This read-only hook never touches RNG or tickets. */
 if(!metadata_fault&&rules_native_ready&&domestic_cache_ready&&
    S(o,OFF(O_OFFICER_ID))<MAX_OFFICERS&&profiles[S(o,OFF(O_OFFICER_ID))].valid&&
    B(o,OFF(O_UPPER))+B(o,OFF(O_LOWER))<=3&&kind!=2&&
    rule_is_ruler&&rule_is_ruler(o,1)){
  int earned=0;for(j=0;j<B(o,OFF(O_LOWER));j++)if(domestic_kind[j]==kind)earned+=domestic_amount[j];
  if(earned&&CALL(F_BONUS,int(*)(void*,int))(person,2)==2)return native>earned?native:earned;
 }
 for(j=0;j<3&&j<B(o,OFF(O_UPPER));j++)if(bonus_kind[j]==kind)deduct+=bonus_amount[j];return native>deduct?native-deduct:0;
}
static void remember_row(void*r){u32 i;for(i=0;i<visible_row_count;i++)if(visible_rows[i]==r)return;if(visible_row_count<MAX_VISIBLE_ROWS)visible_rows[visible_row_count++]=r;}
static void choose_next(u8*o,int allow_decline){u16 id=S(o,OFF(O_OFFICER_ID)),*a=(u16*)(o+OFF(O_TRAITS));u32 i;u16 now=selected_replacement[id];if(now==NO_TRAIT){selected_replacement[id]=a[0];return;}for(i=0;i<9;i++)if(a[i]==now)break;
 for(i++;i<9;i++)if(a[i]&&a[i]!=NO_TRAIT){selected_replacement[id]=a[i];return;}selected_replacement[id]=allow_decline?NO_TRAIT:a[0];}
static int original_available(u8*row,u8*o,u32 stage){MeritChoice c;
 if(stage>2||!row||!choice_read(P(row,OFF(O_ROW_PARENT)),o,&c))return 0;
 return choice_next_upper(&c,ui_gstate(S(o,OFF(O_OFFICER_ID))),stage)||(stage<c.top&&stage>=c.earned_top);
}
static u16 next_real_replacement(const GTarget*t,u16 current,int decline){u32 i=0,j;
 if(current&&current!=NO_TRAIT){for(i=0;i<9&&t->traits[i]!=current;i++){}i++;}
 for(j=i;j<9;j++)if(t->traits[j]&&t->traits[j]!=NO_TRAIT)return t->traits[j];
 if(decline)return NO_TRAIT;
 for(j=0;j<9;j++)if(t->traits[j]&&t->traits[j]!=NO_TRAIT)return t->traits[j];return 0;
}
static void native_button(void*,void*);
static int select_rolled_reward(u8*o,u32 stage){u16 id=S(o,OFF(O_OFFICER_ID));Preview*p=preview_for(id);MeritChoice c;u32 i;
 if(!p||!choice_read(p->native_owner,o,&c))return 0;
 if(stage<c.top){if(stage>=c.earned_top)p->new_mask|=(u8)(1u<<stage);return 1;}
 if(!choice_next_upper(&c,&p->after,stage))return 0;
 for(i=0;i<visible_row_count;i++){u8*r=visible_rows[i];if(!rd(r,OFF(O_ROW_BUTTON)+24)||P(r,OFF(O_ROW_OFFICER))!=o||P(r,OFF(O_ROW_PARENT))!=p->native_owner)continue;
  native_button(p->native_owner,P(r,OFF(O_ROW_BUTTON)+stage*8));
  if(!choice_read(p->native_owner,o,&c)||c.top!=stage+1)return 0;
  p->new_mask|=(u8)(1u<<stage);choice_revision++;return 1;
 }return 0;
}
static void begin_slot(u8*o,u32 stage){GTarget t;u16 id=S(o,OFF(O_OFFICER_ID));Preview*p=preview_for(id);
 if(!p||stage>2||!(p->authorized&(1u<<stage))||metadata_fault)return;
 ui_target(o,&t);active_id=inspected_id=id;active_stage=inspected_stage=(u8)stage;active_keys=reveal_keys();
 selected_replacement[id]=p->replacement[stage];
 if(g_free_slot(&t,&p->after)<0&&t.star&&!selected_replacement[id])selected_replacement[id]=p->replacement[stage]=NO_TRAIT;
 if(p->after.slots[stage].phase==G_PENDING){ui_mode=3;reveal_active=1;return;}
 if(p->after.slots[stage].phase>=G_ACCEPTED){ui_mode=3;reveal_active=1;return;}
 if(g_free_slot(&t,&p->after)<0&&!t.star&&(!selected_replacement[id]||!mc_has(t.traits,selected_replacement[id]))){selected_replacement[id]=p->replacement[stage]=next_real_replacement(&t,0,0);ui_mode=1;reveal_active=1;return;}
 if(perform_roll(o,stage,0,0)){if(!select_rolled_reward(o,stage)){report("MeritTraits R9: NATIVE_SELECTION_REFUSED; preview retained, no award committed.");ui_mode=3;reveal_active=1;return;}ui_mode=2;animation_start=GetTickCount64();roll_started[id][stage]=animation_start;reveal_active=1;fx_begin(0);}
 else{ui_mode=0;active_id=NO_TRAIT;p->authorized&=(u8)~(1u<<stage);p->new_mask&=(u8)~(1u<<stage);if(!p->authorized){p->used=0;preview_count--;}report("MeritTraits R2: draw unavailable; no native reward or cost was committed.");}
}
static int next_in_session(void){return 0;} /* No implicit draws after bottom confirmation. */
static void end_slot(void){fx_clear();active_id=NO_TRAIT;ui_mode=0;reveal_active=0;preselect_owner=0;preselect_button=0;paint_visible();}
static void park_invisible_active(void){u32 i;if(active_id>=MAX_OFFICERS)return;
 for(i=0;i<visible_row_count;i++){u8*r=visible_rows[i],*o;if(!rd(r,OFF(O_ROW_OFFICER)+8))continue;o=P(r,OFF(O_ROW_OFFICER));if(rd(o,OFF(O_OFFICER_ID)+2)&&S(o,OFF(O_OFFICER_ID))==active_id)return;}
 /* A recycled row is not an officer identity. Keep its result in Preview,
    but detach the toolbar until that officer is explicitly selected again. */
 fx_clear();active_id=inspected_id=NO_TRAIT;ui_mode=0;reveal_active=0;miss_notice_hide();
}
static void paint_row(void*row){u8*r=row,*o;u16 id;u32 j;MeritChoice c;
 if(!enabled||!rd(r,OFF(O_ROW_LOCK_ICON)+48)||P(r,0)!=game+OFF(O_ROW_VTABLE))return;
 o=P(r,OFF(O_ROW_OFFICER));if(!rd(o,OFF(O_LOWER)+1))return;id=S(o,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS)return;ensure_profile(o,2);
 int valid=choice_read(P(r,OFF(O_ROW_PARENT)),o,&c);
 for(j=0;j<3;j++){
  u8*button=P(r,OFF(O_ROW_BUTTON)+j*8);GSlot*x=&ui_gstate(id)->slots[j];const u16*name=L"개성 해금";
  int earned=j<B(o,OFF(O_UPPER)),selected=valid&&choice_is_selected(&c,j),on=0;
  set_visible(P(r,OFF(O_ROW_TEXT)+j*8),0);
  if(valid)on=choice_next_upper(&c,ui_gstate(id),j)||(j<c.top&&!earned);
  if(earned&&states[id].slots[j].phase>=G_ACCEPTED)on=0;
  for(u32 k=0;k<j;k++)if(ui_gstate(id)->slots[k].phase==G_EMPTY)on=0;
  if(x->phase>=G_PENDING)name=x->outcome==G_MISS?L"X (꽝)":x->phase==G_DECLINED?L"획득 포기":trait_name(x->result);
  if(id==active_id&&j==active_stage&&ui_mode==2){u64 elapsed=GetTickCount64()-animation_start;u16 t=x->result;
   if(roll_count[id][j])t=roll_pool[id][j][mc_roll_index((u32)elapsed,roll_count[id][j])];name=trait_name(t);on=0;
  }
  if(valid&&c.status[j]==3&&!earned&&!x->phase){enable_native_tile(r,j,0,0);button_text(button,L"",0,-1);set_visible(P(r,OFF(O_ROW_LOCK_LABEL)+j*8),1);set_visible(P(r,OFF(O_ROW_LOCK_ICON)+j*8),1);continue;}
  if(finalizing||miss_modal||exit_modal||ui_mode==1||(ui_mode==2&&id==active_id&&j==active_stage))on=0;
  set_visible(P(r,OFF(O_ROW_LOCK_LABEL)+j*8),0);set_visible(P(r,OFF(O_ROW_LOCK_ICON)+j*8),0);
  enable_native_tile(r,j,on,0);button_text(button,name,on,-1);
  /* Original CCmnButton selected state owns the same orange/gold skin as the
     domestic row. Never recolor an unlocked result as a separate action. */
  if(rd(button,8)&&rd(P(button,0)+0x208,8))V(button,0x208,Flag)(button,selected);
  ui_painted_labels++;
 }
}
static void hook_row_refresh(void*r){if(ui_teardown||closing_layout)return;fx_release_row(r);ui_notification_depth++;CALL(F_ROW_REFRESH,Void1)(r);ui_notification_depth--;if(ui_teardown)return;remember_row(r);park_invisible_active();paint_row(r);if(native_ui_ready&&active_state)controls_ensure(r);controls_refresh();}
static void hook_row_destroy(void*r,void*resources){u32 i;ui_teardown++;
 report("MeritTraits R9: ROW_RELEASE_BEGIN\r\n");
 for(i=0;i<visible_row_count;i++)if(visible_rows[i]==r){visible_rows[i]=visible_rows[--visible_row_count];break;}
 fx_release_row(r);
 /* Hover +0x5ce0 forwards R8 (resource context) to its popup's vslot 0x28.
    It is NOT optional when destroy=1. The previous literal NULL discarded
    the same native context that we correctly forwarded to row/toolbar. */
 if(shared_hover_release)shared_hover_release(r,1,resources);
 report("MeritTraits R9: ROW_HOVER_RELEASE_END\r\n");
 if(controls_owner_row==r)controls_destroy(resources);
 park_invisible_active();
 /* Native vslot 0x28 forwards RDX to base resource cleanup. Never drop it. */
 CALL(F_ROW_DESTROY,void(*)(void*,void*))(r,resources);ui_teardown--;report("MeritTraits R9: ROW_RELEASE_END\r\n");
}
static void hook_row_tick(void*r){if(!ui_teardown&&!exit_modal&&!picker_depth)hover_merit_row(r);}
static void paint_visible(void){u32 i;if(ui_teardown)return;for(i=0;i<visible_row_count;i++)paint_row(visible_rows[i]);controls_refresh();}
static void merit_status(void*layout,void*person,int*out){u8*o=person;u32 j;CALL(F_STATUS,void(*)(void*,void*,int*))(layout,person,out);if(!o||!out||!ruler_veteran(o))return;
 /* Veteran capacity is three total choices, not three free awards.  Preserve
    earned prefixes and derive the current selected prefixes directly from the
    native packed selection record when the main merit owner is available. */
 u8 top=B(o,OFF(O_UPPER)),lower=B(o,OFF(O_LOWER));u8*n=choice_node(layout,S(o,OFF(O_OFFICER_ID)));if(n&&rd(n,0x21)){u8 v=B(n,0x20);if((v&15)<=3&&(v>>4)<=3&&(v&15)+(v>>4)<=3){top=v&15;lower=v>>4;}}
 u8 earned_top=B(o,OFF(O_UPPER)),earned_lower=B(o,OFF(O_LOWER));u8 total=(u8)(top+lower);
 for(j=0;j<3;j++)out[j]=j<earned_top?2:j<top?1:(j==top&&total<3)?1:0;
 for(j=0;j<3;j++)out[j+3]=j<earned_lower?2:j<lower?1:(j==lower&&total<3)?1:0;
}
static void hook_status(void*layout,void*person,int*out){merit_status(layout,person,out);}
static void native_button(void*owner,void*button){void*fn[2]={0,owner};void*p=button;CALL(F_BUTTON_EVENT,void(*)(void*,void*))(fn,&p);}
static void hook_button_event(void*functor,void*arg){u32 i,j;void*button=rd(arg,8)?P(arg,0):0;void*owner=rd(functor,16)?P(functor,8):0;
 if(!enabled||!button||!owner){CALL(F_BUTTON_EVENT,void(*)(void*,void*))(functor,arg);return;}
 if(finalizing||thanks_active||ui_teardown||ui_notification_depth||picker_depth||closing_layout||completed_state||ui_mode==1||ui_mode==2||miss_modal||exit_modal)return;
 for(i=0;i<visible_row_count;i++){u8*r=visible_rows[i];if(!rd(r,OFF(O_ROW_LOCK_ICON)+48)||P(r,0)!=game+OFF(O_ROW_VTABLE))continue;
  for(j=0;j<6;j++)if(P(r,OFF(O_ROW_BUTTON)+j*8)==button){u8*o=P(r,OFF(O_ROW_OFFICER));MeritChoice c;
   if(!rd(o,OFF(O_LOWER)+1)||P(r,OFF(O_ROW_PARENT))!=owner)return;
   u16 id=S(o,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS||!choice_read(owner,o,&c))return;ensure_profile(o,2);
   if(j>=3){u32 k=j-3;
    if(k<c.earned_lower||c.status[j]!=1)return;
    if(k>=c.lower&&(k!=c.lower||c.top+c.lower>=c.capacity))return;
    native_button(owner,button);choice_revision++;prepared_valid=replace_approved=0;paint_visible();return;
   }
   inspected_id=id;inspected_stage=(u8)j;inspected_available=choice_next_upper(&c,ui_gstate(id),j);
   if(states[id].slots[j].phase>=G_ACCEPTED){controls_refresh();return;}
   if(!active_state||(!inspected_available&&!(j<c.top&&j>=c.earned_top))){controls_refresh();return;}
   if(native_ui_ready&&!controls_ensure(r))return;
   Preview*p=preview_create(o);if(!p)return;p->native_owner=owner;
   GSlot*slot=&p->after.slots[j];
   if(slot->phase>=G_PENDING){
    /* A revealed reward is the original radio selection, not another draw.
       Deselecting/reselecting cannot obtain a free new result. */
    /* Selecting an earlier revealed tile focuses it; never truncate the
       native prefix and deselect its already-revealed successors. The tail
       retains native select/deselect when clicked while already focused.
       Switching from another stage only focuses the tail; it remains selected. */
    if(j>=c.earned_top&&j+1>=c.top&&(j>=c.top||active_id!=id||active_stage==j)){native_button(owner,button);choice_revision++;prepared_valid=replace_approved=0;}
    if(ui_mode==3)end_slot();active_id=inspected_id=id;active_stage=inspected_stage=(u8)j;
    selected_replacement[id]=p->replacement[j];ui_mode=3;reveal_active=ui_mode!=0;paint_visible();return;
   }
   for(u32 k=0;k<j;k++)if(p->after.slots[k].phase==G_EMPTY)return;
   for(u32 k=0;k<j;k++)if(p->after.slots[k].phase==G_PENDING){GSlot*x=&p->after.slots[k];
    if(k>=c.top)return;int decline=x->outcome!=G_MISS&&x->full&&x->star&&p->replacement[k]==NO_TRAIT;
    if(!finish_result(o,k,decline,0))return;
   }
   if(ui_mode==3)end_slot();p->authorized|=(u8)(1u<<j);
   pending_ui=active_state;reveal_layout=P(active_state,OFF(O_STATE_LAYOUT));completed_state=0;reveal_done=0;
   begin_slot(o,j);paint_visible();return;
  }
 }
 CALL(F_BUTTON_EVENT,void(*)(void*,void*))(functor,arg);
}
/* Owned toolbar actions are separate native widgets, not reward tiles. */
enum {ACTION_REROLL=1,ACTION_ACCEPT,ACTION_REPLACE,ACTION_DETAILS,ACTION_UP,ACTION_DOWN,ACTION_SKIP,ACTION_CANCEL,ACTION_NOTICE_CLOSE,ACTION_MODAL_BLOCK,ACTION_EXIT_YES,ACTION_EXIT_NO};
static int active_reroll_allowed(void){
 if(active_id>=MAX_OFFICERS||active_stage>2)return 0;Preview*p=preview_for(active_id);u8*o=officer(active_id);MeritChoice c;
 return p&&o&&choice_read(p->native_owner,o,&c)&&(active_stage<c.earned_top||active_stage<c.top)&&(p->authorized&(1u<<active_stage))&&states[active_id].slots[active_stage].phase<G_ACCEPTED&&p->after.slots[active_stage].phase>=G_PENDING;
}
static void controls_action(u32 action){u8*o=officer(active_id);GSlot*x=0;
 if(finalizing||thanks_active||ui_teardown||picker_depth||miss_modal||exit_modal)return;
 if(action==ACTION_CANCEL){cancel_requested=1;return;}
 if(action==ACTION_DETAILS||action==ACTION_UP||action==ACTION_DOWN){/* handled by native_controls */return;}
 if(!o||active_stage>2||save_active)return;x=&ui_gstate(active_id)->slots[active_stage];
 if(ui_mode==1){
  if(action==ACTION_REPLACE){GTarget t;ui_target(o,&t);Preview*p=preview_for(active_id);if(!p)return;selected_replacement[active_id]=p->replacement[active_stage]=next_real_replacement(&t,selected_replacement[active_id],0);}
  else if(action==ACTION_ACCEPT)begin_slot(o,active_stage);
  paint_visible();return;
 }
 if(ui_mode==2){if(action==ACTION_SKIP&&x->outcome!=G_MISS&&((g_traits[x->result].color!=3&&!x->anomaly&&!x->transcend)||GetTickCount64()-animation_start>=600)){ui_mode=3;fx_reveal();paint_visible();}return;}
 if(ui_mode!=3)return;
 if(action==ACTION_REROLL){if(active_reroll_allowed()&&perform_roll(o,active_stage,1,0)){ui_mode=2;animation_start=GetTickCount64();fx_begin(1);paint_visible();}return;}
 if(action==ACTION_REPLACE&&x->outcome!=G_MISS&&x->full&&x->star){GTarget t;ui_target(o,&t);Preview*p=preview_for(active_id);if(!p)return;selected_replacement[active_id]=p->replacement[active_stage]=next_real_replacement(&t,selected_replacement[active_id],1);choice_revision++;prepared_valid=replace_approved=0;paint_visible();return;}
 if(action==ACTION_ACCEPT){int decline=x->outcome!=G_MISS&&x->full&&x->star&&selected_replacement[active_id]==NO_TRAIT;if(finish_result(o,active_stage,decline,0))end_slot();}
}
static void clear_reveal(void){
 ui_teardown++;active_state=0;cancel_requested=0;ui_epoch++;
 controls_hide();exit_notice_hide();miss_notice_hide();exit_approved=replace_requested=replace_approved=prepared_valid=warning_kind=0;fx_clear();hide_merit_hovers(0);
 thanks_count=0;thanks_active=0;thanks_ruler=NO_TRAIT;ack_release_pending=0;pending_ui=0;pending_until=0;confirming_ui=0;
 reveal_layout=0;reveal_active=0;reveal_done=0;finish_armed=0;acknowledged_keys=0;merit_presentation_skip=0;
 visible_row_count=0;mc_zero(visible_rows,sizeof(visible_rows));ui_row_logged=0;ui_row_rejection_logged=0;ui_painted_labels=0;
 active_id=inspected_id=NO_TRAIT;active_stage=inspected_stage=0;inspected_available=0;ui_mode=0;committed_session=0;preselect_owner=0;preselect_button=0;
 mc_zero(selected_replacement,sizeof(selected_replacement));mc_zero(roll_started,sizeof(roll_started));mc_zero(deferred_slots,sizeof(deferred_slots));ui_teardown--;
}
static void hook_close(void*layout,int code){
 if(ui_teardown||picker_depth)return;
 if(layout==closing_layout){
  if(code!=0x7fffffff||cancel_delivered)return;
  if(!rd(layout,0x1e0))return;u8*inner=P(layout,0x1d8);
  if(!rd(inner,0x180)||D(inner,0x178)||D(inner,0x17c))return;
  cancel_delivered=1;
 }
 /* This is a shared end-event/fade handler, NOT a Back-only callback.
    Native 0x1c8 is the whole bottom command group. Only the explicit cancel
    event requests the discard warning; other events keep their native code. */
 if(layout==reveal_layout&&!finalizing&&preview_count){
  if(code==0x7fffffff){
   if(exit_modal){exit_back_requested=1;return;}
   replace_requested=replace_approved=0;
   if(!cancel_requested)report("MeritTraits R9: BACK_QUEUED; code=7fffffff\r\n");
   cancel_requested=1;return;
  }
  if(exit_modal||miss_modal||ui_mode==1||ui_mode==2)return;
  report("MeritTraits R9: NATIVE_EVENT_PASSTHROUGH; not cancel\r\n");
 }
 CALL(F_CLOSE,void(*)(void*,int))(layout,code);
}
static int return_same_turn(void*state,u8*layout){
 if(!layout||!rd(layout,OFF(O_MAIN_STATE)+4))return 0;
 completed_state=state;pending_ui=0;reveal_active=0;reveal_done=1;controls_hide();
 /* Native tick owns the ONE success pop. Native fade callback 0x7a8450 skips
    event100 when this value remains 1. R9 erased it; R10 suppressed the pop. */
 D(layout,OFF(O_MAIN_STATE))=1;native_consume_cancel_input();
 report("MeritTraits R11: SUCCESS_NATIVE_EXIT_ONCE\r\n");return 1;
}
static void hook_native_exit_event(void*state,void*event){
 /* The state-specific event100 callback is not the global turn engine. */
 if(completed_state==state||(finalizing&&confirming_ui==state)){report("MeritTraits R11: DUPLICATE_SUCCESS_FADE_EXIT_IGNORED\r\n");return;}
 const HookSpec*h=rule_hook(H_NATIVE_EXIT_EVENT);if(h)((void(*)(void*,void*))(game+h->target))(state,event);
}
static int hook_confirm(void*state){int result;u8*layout;u32 i,j;int needs_native=0;u8 consent_hash[32],current_hash[32];
 report("MeritTraits R10: CONFIRM_ENTER\r\n");
 if(finalizing||thanks_active||ui_teardown||picker_depth||miss_modal||exit_modal||confirming_ui||completed_state==state||ui_mode==1||ui_mode==2||metadata_fault){report("MeritTraits R10: CONFIRM_REJECT_BUSY_OR_FAULT\r\n");return 0;}
 if(!rd(state,OFF(O_STATE_LAYOUT)+8)){report("MeritTraits R10: CONFIRM_INVALID_STATE\r\n");return 0;}
 layout=P(state,OFF(O_STATE_LAYOUT));if(!rd(layout,OFF(O_MAIN_STATE)+4)){report("MeritTraits R10: CONFIRM_INVALID_LAYOUT\r\n");return 0;}
 /* Lower-only and empty screens use the same sole native success exit. */
 if(!preview_count){
  finalizing=1;confirming_ui=state;player_depth++;result=CALL(F_CONFIRM,int(*)(void*))(state);player_depth--;confirming_ui=0;
  if(result&&!metadata_fault)return_same_turn(state,layout);
  finalizing=0;report(result&&!metadata_fault?"MeritTraits R10: CONFIRM_NATIVE_ONLY_OK\r\n":"MeritTraits R10: CONFIRM_NATIVE_ONLY_REFUSED\r\n");
  return result&&!metadata_fault;
 }
 if(layout!=reveal_layout){report("MeritTraits R10: CONFIRM_INVALID_REVEAL_LAYOUT\r\n");return 0;}
 needs_native=choice_has_changes(layout);if(needs_native<0){report("MeritTraits R9: CONFIRM_INVALID_SELECTION_TREE\r\n");return 0;}
 /* Native auto-selection or another mod can change the map without passing
    our reward callback. Consent must cover the EXACT old/new replacement list,
    not merely our callback revision. Re-prepare and compare before committing. */
 u32 consent_count=replacement_count;int consent=replace_approved&&replace_revision==choice_revision&&consent_count;
 if(consent)mc_sha256(replacements,consent_count*sizeof(*replacements),consent_hash);
 if(!prepare_previews()){report("MeritTraits R9: CONFIRM_REJECT_PREPARE.\r\n");return 0;}
 if(!previews_ready()||!previews_validate(0)){report("MeritTraits R9: CONFIRM_REJECT_SNAPSHOT.\r\n");return 0;}
 if(consent){mc_sha256(replacements,replacement_count*sizeof(*replacements),current_hash);consent=consent_count==replacement_count&&mc_equal(consent_hash,current_hash,32);}
 for(i=0;i<PREVIEW_CAP;i++)if(prepared_previews[i].used){Preview*p=&prepared_previews[i];u8*o=officer(p->id);if(!o)return 0;
  if(p->new_mask||NCALL(N_AI_ELIGIBLE,int(*)(void*))(o))needs_native=1;
 }
 if(replacement_count&&!consent){
  replacement_index=0;replace_revision=choice_revision;replace_requested=1;replace_approved=0;return 0;
 }
 replace_approved=0;
 if(ui_mode==3)end_slot();
 report("MeritTraits R9: CONFIRM_BEGIN\r\n");
 capture_thanks_ruler(state);thanks_count=0;pending_ui=state;finalizing=1;final_commit_seen=0;confirming_ui=state;player_depth++;
 /* Original commit, original portrait dialogue and original return own one
  * continuous call chain. The presentation gate remains passthrough. */
 merit_presentation_skip=0;
 if(needs_native)result=CALL(F_CONFIRM,int(*)(void*))(state);
 else{mc_zero(native_presented,sizeof(native_presented));AcquireSRWLockExclusive(&state_lock);apply_previews();result=!metadata_fault;final_commit_seen=result;ReleaseSRWLockExclusive(&state_lock);}
 player_depth--;confirming_ui=0;
 if(result&&final_commit_seen&&!metadata_fault){
  for(i=0,j=0;i<thanks_count;i++)if(!native_presented[thanks_ids[i]])thanks_ids[j++]=thanks_ids[i];thanks_count=j;
  if(thanks_count)show_native_thanks(state);
  thanks_count=0;return_same_turn(state,layout);
 }
 finalizing=0;report(result&&final_commit_seen&&!metadata_fault?"MeritTraits R10: CONFIRM_OK\r\n":"MeritTraits R10: CONFIRM_REFUSED\r\n");
 return result&&final_commit_seen&&!metadata_fault;
}
static void original_state_tick(void*state,void*layout){u64 epoch=ui_epoch;
 int picker=rd(layout,OFF(O_MAIN_STATE)+4)&&D(layout,OFF(O_MAIN_STATE))==2;
 /* Native state 2 enters its synchronous officer selector. Our separately
    registered resources must not stay above that screen or consume input. */
 if(picker){picker_depth++;controls_suspend();report("MeritTraits R9: PICKER_UI_SUSPEND.\r\n");}
 CALL(F_STATE_TICK,Void1)(state);
 if(picker){picker_depth--;if(ui_epoch==epoch&&active_state==state&&rd(layout,OFF(O_MAIN_STATE)+4)){
  for(u32 i=0;i<visible_row_count;i++)if(controls_ensure(visible_rows[i]))break;
  controls_refresh();report("MeritTraits R9: PICKER_UI_RESUME.\r\n");
 }}
 if(completed_state!=state&&ui_epoch==epoch&&active_state==state&&rd(layout,OFF(O_MAIN_STATE)+4)&&D(layout,OFF(O_MAIN_STATE))==1)D(layout,OFF(O_MAIN_STATE))=0;
}
static void hook_state_tick(void*state){u64 now;u32 keys;u8*layout;
 if(thanks_active||finalizing||ui_teardown||picker_depth)return;active_state=state;
 if(!rd(state,OFF(O_STATE_LAYOUT)+8))return;layout=P(state,OFF(O_STATE_LAYOUT));
 if(layout==closing_layout||completed_state==state)return;
 if(reveal_layout&&layout!=reveal_layout){if(preview_count)cancel_previews();clear_reveal();return;}
 for(u32 i=0;i<visible_row_count;i++)if(native_ui_ready&&controls_ensure(visible_rows[i]))break;
 /* In native 0x732f50, layout state 1 is the bottom-confirm request and 0 is
    idle. Discard requests behind animation/inheritance/notice. A revealed
    result is now explicitly accepted by this original bottom decision. */
 if((ui_mode==1||ui_mode==2||miss_modal||exit_modal)&&rd(layout,OFF(O_MAIN_STATE)+4)&&D(layout,OFF(O_MAIN_STATE))==1)D(layout,OFF(O_MAIN_STATE))=0;
 keys=reveal_keys();u32 pressed=keys&~active_keys;active_keys=keys;
 controls_input_tick(layout);controls_dispatch_tick();
 if(exit_modal){
  if(!exit_notice_healthy()){
   report("MeritTraits R9: NOTICE_VISIBILITY_LOST; unlock without consent.\r\n");
   exit_notice_resume();replace_requested=replace_approved=0;warning_kind=0;paint_visible();return;
  }
  if(exit_back_requested){
   int was_replace=warning_kind==2;exit_notice_resume();replace_requested=replace_approved=0;warning_kind=0;
   if(rd(layout,OFF(O_MAIN_STATE)+4))D(layout,OFF(O_MAIN_STATE))=0;
   if(was_replace){warning_kind=1;exit_notice_show();}paint_visible();return;
  }
  /* Ignore native commands received behind the warning (decision/select).
     Neither holding Back/Enter nor a stale Yes callback implies consent. */
  if(rd(layout,OFF(O_MAIN_STATE)+4)&&(D(layout,OFF(O_MAIN_STATE))==1||D(layout,OFF(O_MAIN_STATE))==2))D(layout,OFF(O_MAIN_STATE))=0;
  if(!exit_armed&&!keys)exit_armed=1;
  if(exit_armed&&(pressed&4)&&!exit_choice){native_consume_cancel_input();exit_choice=2;}
  if(!exit_choice)return;
  /* Native actions may fire on mouse-down. Do not hide/unregister controls
     or start a native state transition inside that physical gesture. */
  if(exit_choice==1){if(!exit_choice_wait){exit_choice_wait=1;return;}if(keys&3u)return;}
  if(exit_choice==2){report("MeritTraits R9: EXIT_NO\r\n");exit_notice_resume();replace_requested=replace_approved=0;warning_kind=0;paint_visible();return;}
  if(warning_kind==2){
   if(replace_revision!=choice_revision){exit_notice_resume();replace_requested=replace_approved=0;return;}
   replacement_index++;if(replacement_index<replacement_count){exit_notice_hide();warning_kind=2;replace_requested=1;return;}
   exit_notice_hide();warning_kind=0;replace_approved=1;replace_requested=0;
   if(rd(layout,OFF(O_MAIN_STATE)+4)){D(layout,OFF(O_MAIN_STATE))=1;original_state_tick(state,layout);}return;
  }
  report("MeritTraits R9: CANCEL_YES\r\n");exit_notice_hide();exit_approved=1;cancel_requested=1;
 }
 if(cancel_requested||((pressed&4)&&(ui_mode||preview_count))){
  cancel_requested=0;replace_requested=replace_approved=0;
  if(preview_has_result()&&!exit_approved){
   warning_kind=1;if(pressed&4)native_consume_cancel_input();
   if(!exit_notice_show())report("MeritTraits R9: WARNING_UNAVAILABLE; retry Back, no implicit consent.\r\n");
   return;
  }
  exit_approved=0;report("MeritTraits R9: CANCEL_TICK\r\n");
  /* Restore menu choices BEFORE the original close guard. No trait/cost/rank
     commit belongs to Back. Private coupons have already been reserved. */
  ui_teardown++;miss_notice_hide();fx_clear();
  choice_cancel_all(layout);if(preview_count)cancel_previews();clear_reveal();ui_teardown--;
  closing_layout=layout;cancel_delivered=0;
  if(!native_cancel_merit(layout)){closing_layout=0;report("MeritTraits R9: native cancel adapter unavailable; original event retained.\r\n");CALL(F_STATE_TICK,Void1)(state);}
  return;
 }
 if(replace_requested){warning_kind=2;replace_requested=0;
  if(!exit_notice_show()){replace_approved=0;report("MeritTraits R9: replacement warning unavailable; no replacement committed.\r\n");}return;
 }
 if(miss_modal){miss_notice_tick();if(pressed&2)miss_notice_hide();if(!miss_modal)paint_visible();return;}
 /* Tick the Hover adapter while the result is held, not just the native row
    callback (which CE can omit while a layout reports an active selection). */
 if(ui_mode!=2){for(u32 i=0;i<visible_row_count;i++)hover_merit_row(visible_rows[i]);}
 if(ui_mode==2){fx_tick();now=GetTickCount64();GSlot*x=&ui_gstate(active_id)->slots[active_stage];
  if(now-animation_start>=1200||((pressed&2)&&x->outcome!=G_MISS&&((g_traits[x->result].color!=3&&!x->anomaly&&!x->transcend)||now-animation_start>=600))){ui_mode=3;fx_reveal();paint_visible();return;}
  if(now>=last_caption+50||now<last_caption){last_caption=now;paint_visible();}return;
 }
 if(ui_mode==3){miss_notice_tick();if(miss_modal)return;}
 /* Original ESC/cancel and page handling continue even during a held result.
    hook_confirm still blocks only premature committing, not all state ticks. */
 if(ui_mode==1||ui_mode==2)return;
 /* Finished results never trigger a gratitude message on a click/release.
  * Only the native bottom-confirm state can enter hook_confirm. */
 original_state_tick(state,layout);
}
static void hook_ui_enter(void*layout){
 PERF_HIT(PERF_UI_ENTER);
 closing_layout=detached_layout=0;cancel_delivered=0;completed_state=0;
 if(enabled&&!save_active&&!transaction){AcquireSRWLockExclusive(&state_lock);if(!metadata_ready)capture_profiles(2);else {refresh_ruler_profiles(2);refresh_trait_rules();}ReleaseSRWLockExclusive(&state_lock);}
 report("MeritTraits R9: UI_REGISTER\r\n");
 for(u32 i=0;i<res->hook_count;i++)if(res->hooks[i].kind==H_UI_ENTER){
  ui_notification_depth++;((Void1)(game+res->hooks[i].target))(layout);ui_notification_depth--;return;
 }
}
static void hook_ui_exit(void*layout){
 PERF_HIT(PERF_UI_EXIT);
 if(layout==detached_layout){report("MeritTraits R9: UI_UNREGISTER_DUPLICATE_IGNORED\r\n");return;}
 detached_layout=layout;
 /* Native vslot 0x40 unregisters this layout. It can be reached without the
    close hook. Keep the teardown guard across its child unregister callbacks. */
 report("MeritTraits R9: UI_UNREGISTER_BEGIN\r\n");
 ui_teardown++;controls_hide();report("MeritTraits R9: UI_OWNED_DETACHED.\r\n");if(!finalizing&&preview_count){report("MeritTraits R9: FORCED_DETACH; no commit during teardown; spent tickets retained.\r\n");cancel_previews();}clear_reveal();
 report("MeritTraits R9: UI_NATIVE_UNREGISTER_BEGIN.\r\n");CALL(F_UI_EXIT,Void1)(layout);ui_teardown--;report("MeritTraits R9: UI_UNREGISTER_END\r\n");
}
