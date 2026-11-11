#include "gacha.h"
#include "official_db.h"
#include "collab_db.h"
#include "trait_rules.inc"
const GParams g_params={
 900.0,0.45,0.09,0.20,0.15,
 0.035,0.010,0.10,0.80,0.70,1.40,
 3.00,0.38,2.00,0.045,0.035,
 0.16,0.48,0.45,0.0010,0.00010,0.24,
 0.05,0.50,1.35,1.50 /* R3 initial tuning, not a verified balance guarantee. */
};
static double min(double a,double b){return a<b?a:b;}
static double max(double a,double b){return a>b?a:b;}
const GTeacher*g_teacher(u16 id){return id&&id<=G_TEACHERS?&g_teachers[id-1]:0;}
u16 g_trait_holders(u16 id){if(!id||id>=MAX_TRAITS)return 0;return (u16)(g_traits[id].holders+g_collab_holders[id]);}
int g_trait_has_source(u16 id){return id&&id<MAX_TRAITS&&(g_trait_holders(id)||g_traits[id].anchor);}
void g_init(GState*s,const u16 owned[9]){mc_zero(s,sizeof(*s));s->initialized=1;s->budget=g_params.budget_start;mc_copy(s->base_traits,owned,18);}
int g_free_slot(const GTarget*t,const GState*s){u32 i;for(i=0;i<9;i++)if((!t->traits[i]||t->traits[i]==NO_TRAIT)&&!(s->x_mask&(1u<<i)))return (int)i;return -1;}
void g_reconcile(const GTarget*t,GState*s){u32 i;for(i=0;i<9;i++)if(t->traits[i]&&t->traits[i]!=NO_TRAIT)s->x_mask&=(u16)~(1u<<i);}
double g_miss_rate(const GState*s,u32 stage){u32 i,left=0;if(!s||stage>=2||s->passion)return 0;for(i=0;i<3;i++)if(s->slots[i].phase<G_ACCEPTED)left++;return left>1?g_params.miss_rate:0;}
u64 g_next(u64*s){u64 z=(*s+=0x9e3779b97f4a7c15ULL);z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
static double unit(u64*s){return (double)(g_next(s)>>11)*0x1.0p-53;}
static void top(GScore*a,u16 id,double score){u32 j,k;for(j=0;j<G_TOP;j++)if(score>a[j].score){for(k=G_TOP-1;k>j;k--)a[k]=a[k-1];a[j].id=id;a[j].score=score;break;}}
static double stat_sim(const u8*a,const u8*b){u32 j;double d=0;for(j=0;j<4;j++){double x=(double)a[j]-b[j];d+=x*x;}d=1.0+d/g_params.stat_scale;return 1.0/(d*d*d);}
/* Original-owner neighbours depend only on the immutable compiled DB. Keep
   nine in identical stable order; excluding the current target leaves exactly
   the original top eight, including ties. Publish once, never wait in gameplay. */
static GScore anchor_neighbours[MAX_TRAITS][G_TOP+1];
/* R17: membership is immutable, transposed by trait for contiguous byte
   additions. Counts preserve duplicate target slots and out-of-range IDs. */
static u8 teacher_has_trait[MAX_TRAITS][G_TEACHERS];
static u32 anchors_ready;
GCalcCounters g_calc_counters;
void g_prepare(void){
 if(__atomic_load_n(&anchors_ready,__ATOMIC_ACQUIRE)==2||!__sync_bool_compare_and_swap(&anchors_ready,0,1))return;
 for(u32 i=0;i<G_TEACHERS;i++)for(u32 j=0;j<9;j++){
  u16 id=g_teachers[i].traits[j];if(id&&id<MAX_TRAITS)teacher_has_trait[id][i]=1;
 }
 for(u32 i=1;i<MAX_TRAITS;i++)if(g_traits[i].holders==1&&g_traits[i].anchor){
  const GTeacher*a=g_teacher(g_traits[i].anchor);GScore*near=anchor_neighbours[i];
  for(u32 j=0;j<1000;j++)if(g_teachers[j].id!=a->id){double score=stat_sim(a->stats,g_teachers[j].stats);
   for(u32 k=0;k<G_TOP+1;k++)if(score>near[k].score){for(u32 n=G_TOP;n>k;n--)near[n]=near[n-1];near[k]=(GScore){g_teachers[j].id,score};break;}
  }
 }
 __sync_fetch_and_add(&g_calc_counters.anchor_builds,1);__atomic_store_n(&anchors_ready,2,__ATOMIC_RELEASE);
}
static double similarity(const GTarget*t,const GState*s,const GTeacher*q){u32 i,base=0,newtraits=0,nb=0,nn=0;double sim=stat_sim(t->stats,q->stats);for(i=0;i<9;i++)if(t->traits[i]&&t->traits[i]!=NO_TRAIT){if(mc_has(s->base_traits,t->traits[i])){nb++;if(mc_has(q->traits,t->traits[i]))base++;}else{nn++;if(mc_has(q->traits,t->traits[i]))newtraits++;}}
 sim*=1.0+(nb?g_params.base_overlap*base/nb:0)+(nn?g_params.gained_overlap*newtraits/nn:0);
 if(t->star)sim*=0.80+0.20*min(1.0,sim);return sim;
}
/* Partition the nine owned slots once per distribution, not 1,088 times.
   The exact scalar floating-point expression/order is unchanged below. The
   integer rows can be vectorized without changing any draw probabilities. */
static void overlap_counts(const GTarget*t,const GState*s,u8 base[G_TEACHERS],u8 gained[G_TEACHERS],u32*nb,u32*nn){
 *nb=*nn=0;mc_zero(base,G_TEACHERS);mc_zero(gained,G_TEACHERS);
 for(u32 j=0;j<9;j++){u16 id=t->traits[j];if(!id||id==NO_TRAIT)continue;
  u8*out;if(mc_has(s->base_traits,id)){(*nb)++;out=base;}else{(*nn)++;out=gained;}
  if(id<MAX_TRAITS)for(u32 i=0;i<G_TEACHERS;i++)out[i]+=teacher_has_trait[id][i];
 }
}
static u32 families(const GTarget*t,u16 ids[6],double w[6]){u16 parents[2]={t->father,t->mother};u32 n=0,i,j,k;double sum=0;for(i=0;i<2;i++){const GTeacher*p=g_teacher(parents[i]);if(!p||p->id==t->id)continue;for(k=0;k<n;k++)if(ids[k]==p->id)break;if(k==n){ids[n]=p->id;w[n++]=g_params.parent;sum+=g_params.parent;}u16 g[2]={p->father,p->mother};for(j=0;j<2&&n<6;j++){if(!g_teacher(g[j])||g[j]==t->id)continue;for(k=0;k<n;k++)if(ids[k]==g[j])break;if(k==n){ids[n]=g[j];w[n++]=g_params.grandparent;sum+=g_params.grandparent;}}}if(sum>g_params.family_cap)for(i=0;i<n;i++)w[i]*=g_params.family_cap/sum;return n;}
/* Commit preflight only needs existence, not teacher scores. Every sourced
   candidate has strictly positive similarity mass (including unique anchors
   and collaboration sources). Checked against full compiled distributions.
   This check consumes no RNG and changes no budget or reroll policy. */
int g_has_pool(const GTarget*t,const GState*s,const u8 installed[MAX_TRAITS]){
 if(!t||!s||!installed||!s->initialized)return 0;
 for(u16 i=1;i<MAX_TRAITS;i++)if((installed[i]==1||installed[i]==3)&&installed[i]==g_traits[i].color&&g_trait_has_source(i)&&g_allowed(t,i))return 1;
 return 0;
}
int g_distribution(const GTarget*t,const GState*s,const u8 installed[MAX_TRAITS],u8 anomaly,u8 transcend,GDistribution*d){
 double sims[G_TEACHERS],mass=0,fweight[6];u32 i,j,k;double maxraw=0;u32 oldgold=0;
 u8 base[G_TEACHERS],gained[G_TEACHERS];u32 nb=0,nn=0;int prepared;
 if(!t||!s||!d||!installed||!s->initialized)return G_INVALID;
 g_prepare();__sync_fetch_and_add(&g_calc_counters.distributions,1);
 prepared=__atomic_load_n(&anchors_ready,__ATOMIC_ACQUIRE)==2;
 if(prepared)overlap_counts(t,s,base,gained,&nb,&nn);
 mc_zero(d,sizeof(*d));d->anomaly=anomaly;d->transcend=transcend;
 for(i=0;i<9;i++)if(t->traits[i]<MAX_TRAITS&&g_traits[t->traits[i]].color==3)oldgold++;
 for(i=0;i<G_TEACHERS;i++){const GTeacher*q=&g_teachers[i];double sim;
  if(prepared){sim=stat_sim(t->stats,q->stats);sim*=1.0+(nb?g_params.base_overlap*base[i]/nb:0)+(nn?g_params.gained_overlap*gained[i]/nn:0);
   if(t->star)sim*=0.80+0.20*min(1.0,sim);
  }else sim=similarity(t,s,q); /* Nonblocking cold-init race fallback. */
  sims[i]=sim*(q->role==2?g_params.auxiliary:1.0);if(q->id==t->id)sims[i]=0; /* no self teacher shortcut */
 mass+=sims[i];top(d->similar,q->id,sims[i]);for(j=0;j<9;j++)if(q->traits[j]&&q->traits[j]<MAX_TRAITS)d->raw[q->traits[j]]+=sims[i];}
 for(i=0;i<G_TOP&&d->similar[i].id;i++){u16 n=d->similar[i].id-1;mc_copy(d->similar_stats[i],g_teachers[n].stats,5);}
 if(mass<=0)return G_NO_POOL;
 /* The uploaded 1088-officer DB omits collaboration officers.  Add only the
    documented exclusive trait supported by each collaboration profile, using
    the same auxiliary coefficient as bonus officers.  Do not inject their
    common traits or their mass: this makes the patch additive for the missing
    39 traits without silently rebasing every pre-existing candidate weight. */
 for(i=0;i<G_COLLAB_SOURCES;i++){const GCollabSource*q=&g_collab_sources[i];double sm=stat_sim(t->stats,q->stats)*g_params.auxiliary;if(q->trait&&q->trait<MAX_TRAITS)d->raw[q->trait]+=sm;}
 for(i=1;i<MAX_TRAITS;i++)d->raw[i]/=mass;
 /* A unique original owner remains the anchor; neighbouring official profiles
    contribute only a small supporting distribution, never a hand-written role. */
 for(i=1;i<MAX_TRAITS;i++)if(g_traits[i].holders==1&&g_traits[i].anchor){
  GScore near[G_TOP];mc_zero(near,sizeof(near));const GTeacher*a=g_teacher(g_traits[i].anchor);double sm=0,sw=0;
  if(__atomic_load_n(&anchors_ready,__ATOMIC_ACQUIRE)==2){
   for(j=0,k=0;j<G_TOP+1&&k<G_TOP;j++)if(anchor_neighbours[i][j].id&&anchor_neighbours[i][j].id!=t->id)near[k++]=anchor_neighbours[i][j];
  }else for(j=0;j<1000;j++)if(g_teachers[j].id!=a->id&&g_teachers[j].id!=t->id)top(near,g_teachers[j].id,stat_sim(a->stats,g_teachers[j].stats));
  for(j=0;j<G_TOP;j++)if(near[j].id){sm+=near[j].score*sims[near[j].id-1];sw+=near[j].score;}
  d->raw[i]+=(sw?sm/sw/mass:0)*g_params.anchor_support;
 }
 d->family_count=families(t,d->family,fweight);mc_copy(d->family_weight,fweight,d->family_count*sizeof(double));
 for(i=0;i<d->family_count;i++){const GTeacher*p=g_teacher(d->family[i]);for(j=0;j<9;j++)if(p->traits[j]&&p->traits[j]<MAX_TRAITS)d->raw[p->traits[j]]+=fweight[i]/9.0;}
 for(i=1;i<MAX_TRAITS;i++){
  const GTrait*m=&g_traits[i];if(!installed[i]||(m->color!=1&&m->color!=3)||installed[i]!=m->color||!d->raw[i])continue;
  if(!g_allowed(t,(u16)i))continue;
  if(d->raw[i]>maxraw)maxraw=d->raw[i];
  d->weight[i]=d->raw[i];d->count++;
 }
 if(!d->count)return G_NO_POOL;
 for(i=1;i<MAX_TRAITS;i++)if(d->weight[i]>0){
  const GTrait*m=&g_traits[i];u16 holders=g_trait_holders((u16)i);double rare=g_params.rarity_floor+(1.0-g_params.rarity_floor)*(double)holders/(holders+8.0);
  double w=anomaly?(0.10*d->raw[i]+0.90/(double)d->count):d->raw[i];
  d->quality[i]=min(1.0,d->raw[i]/maxraw);
  if(m->color==3){double budget=transcend?1.0:max(g_params.budget_floor,min(1.0,s->budget/g_params.gold_cost));w*=g_params.gold_multiplier*budget*(t->star?g_params.star_gold:1.0)*(1.0+s->heat*g_params.heat_multiplier)/(1.0+oldgold*g_params.existing_gold_cost);}
  if(s->passion){if(d->quality[i]>=g_params.passion_fit_threshold)w*=g_params.passion_fit_multiplier;if(m->color==3)w*=g_params.passion_gold_multiplier;}
  w*=rare;d->weight[i]=w;d->total+=w;top(d->candidates,(u16)i,w);
 }
 for(k=0;k<G_TOP;k++)if(d->candidates[k].id)d->candidates[k].score/=d->total;
 return G_OK;
}
int g_distribution_cached(const GTarget*t,const GState*s,const u8 installed[MAX_TRAITS],u8 anomaly,u8 transcend,GDistribution*d,GDistributionCache*c){
 if(!c||!t||!s||!installed||!d||!s->initialized)return g_distribution(t,s,installed,anomaly,transcend,d);
 if(c->valid&&c->rules_revision==g_rules_revision&&c->anomaly==anomaly&&c->transcend==transcend&&c->passion==s->passion&&
    c->budget==s->budget&&c->heat==s->heat&&mc_equal(&c->target,t,sizeof(*t))&&
    mc_equal(c->base_traits,s->base_traits,18)&&mc_equal(c->installed,installed,MAX_TRAITS)){
  *d=c->distribution;__sync_fetch_and_add(&g_calc_counters.cache_hits,1);return G_OK;
 }
 c->valid=0;int r=g_distribution(t,s,installed,anomaly,transcend,d);if(r!=G_OK)return r;
 c->target=*t;mc_copy(c->base_traits,s->base_traits,18);mc_copy(c->installed,installed,MAX_TRAITS);
 c->passion=s->passion;c->budget=s->budget;c->heat=s->heat;c->anomaly=anomaly;c->transcend=transcend;c->distribution=*d;c->rules_revision=g_rules_revision;c->valid=1;return r;
}
static int g_roll_limit(const GTarget*t,GState*s,u32 stage,const u8 installed[MAX_TRAITS],u64 seed,u64 roll_id,u16 inherit,int reroll,GDistribution*d,double miss_rate,GDistributionCache*cache){
 GSlot next;u32 i;double point;u64 rng=seed;
 if(!t||!s||stage>2||!s->initialized)return G_INVALID;
 GSlot*old=&s->slots[stage];
 if(reroll){if(old->phase!=G_PENDING||s->rerolls_used>=G_REROLLS)return G_NO_REROLL;inherit=old->replace_trait;roll_id=old->roll_id;}
 else{if(old->phase!=G_EMPTY)return G_INVALID;for(i=0;i<3;i++)if(s->slots[i].phase==G_PENDING)return G_CONFLICT;}
 mc_zero(&next,sizeof(next));next.full=g_free_slot(t,s)<0;next.star=t->star;next.replace_trait=inherit;next.x_index=G_NO_X_SLOT;next.passion_applied=s->passion;
 /* An independent stream preserves the non-miss RNG order from R2. MISS is
    an outcome, never a synthetic trait ID, and needs no replacement choice. */
 u64 miss_rng=seed^0x4d4953535f524333ULL;
 if(unit(&miss_rng)<miss_rate){
  mc_zero(d,sizeof(*d));next.outcome=G_MISS;next.phase=G_PENDING;next.seed=seed;next.roll_id=roll_id;
  next.rerolls=reroll?(u8)(old->rerolls+1):0;next.weight=miss_rate;
  if(reroll)s->rerolls_used++;*old=next;return G_OK;
 }
 if(next.full&&!t->star&&(!inherit||!mc_has(t->traits,inherit)))return G_NEED_INHERIT;
 next.transcend=unit(&rng)<g_params.transcend;next.anomaly=next.transcend||unit(&rng)<g_params.anomaly;
 int r=g_distribution_cached(t,s,installed,next.anomaly,next.transcend,d,cache);if(r!=G_OK)return r;
 point=unit(&rng)*d->total;
 for(i=1;i<MAX_TRAITS;i++)if(d->weight[i]>0){next.result=(u16)i;if(point<d->weight[i])break;point-=d->weight[i];}
 next.phase=G_PENDING;next.seed=seed;next.roll_id=roll_id;next.rerolls=reroll?(u8)(old->rerolls+1):0;
 next.raw=d->raw[next.result];next.weight=d->weight[next.result]/d->total;next.quality=d->quality[next.result];
 next.cost=g_traits[next.result].color==3?g_params.gold_cost:g_params.blue_cost;
 if(reroll)s->rerolls_used++;*old=next;return G_OK;
}
int g_roll(const GTarget*t,GState*s,u32 stage,const u8 installed[MAX_TRAITS],u64 seed,u64 roll_id,u16 inherit,int reroll,GDistribution*d){return g_roll_limit(t,s,stage,installed,seed,roll_id,inherit,reroll,d,g_miss_rate(s,stage),0);}
int g_roll_merit(const GTarget*t,GState*s,u32 stage,const u8 installed[MAX_TRAITS],u64 seed,u64 roll_id,u16 inherit,int reroll,GDistribution*d,u32 max_upper){
 if(!max_upper||max_upper>3||stage>=max_upper)return G_INVALID;
 return g_roll_limit(t,s,stage,installed,seed,roll_id,inherit,reroll,d,stage+1>=max_upper?0:g_miss_rate(s,stage),0);
}
int g_roll_merit_cached(const GTarget*t,GState*s,u32 stage,const u8 installed[MAX_TRAITS],u64 seed,u64 roll_id,u16 inherit,int reroll,GDistribution*d,u32 max_upper,GDistributionCache*cache){
 if(!max_upper||max_upper>3||stage>=max_upper)return G_INVALID;
 return g_roll_limit(t,s,stage,installed,seed,roll_id,inherit,reroll,d,stage+1>=max_upper?0:g_miss_rate(s,stage),cache);
}
/* R13 only: a held-result edit can exclude outcomes incompatible with fixed
   later previews. Ordinary/AI draws continue through the unchanged entry. */
int g_roll_merit_filtered(const GTarget*t,GState*s,u32 stage,const u8 installed[MAX_TRAITS],u64 seed,u64 roll_id,u16 inherit,int reroll,GDistribution*d,u32 max_upper,int allow_miss){
 if(!max_upper||max_upper>3||stage>=max_upper)return G_INVALID;
 return g_roll_limit(t,s,stage,installed,seed,roll_id,inherit,reroll,d,(!allow_miss||stage+1>=max_upper)?0:g_miss_rate(s,stage),0);
}
int g_accept(GTarget*t,GState*s,u32 stage,u16 replace,int decline){
 if(!t||!s||stage>2)return G_INVALID;GSlot*p=&s->slots[stage];int k;u32 j;
 if(p->phase!=G_PENDING)return G_INVALID;
 if(p->outcome==G_MISS){
  if(decline||p->result||stage==2||s->passion)return G_INVALID;
  g_reconcile(t,s);k=g_free_slot(t,s);p->x_index=k<0?G_NO_X_SLOT:(u8)k;
  if(k>=0)s->x_mask|=(u16)(1u<<k);p->phase=G_ACCEPTED;s->passion=1;return G_OK;
 }
 if(!p->result||p->result>=MAX_TRAITS)return G_INVALID;
 if(decline){if(!p->star||!p->full)return G_INVALID;p->phase=G_DECLINED;return G_OK;}
 if(!g_allowed(t,p->result))return G_CONFLICT;
 g_reconcile(t,s);k=g_free_slot(t,s);
 if(k<0){if(!p->star)replace=p->replace_trait;if(!replace)return G_NEED_INHERIT;for(k=0;k<9;k++)if(t->traits[k]==replace)break;if(k==9)return G_CONFLICT;}
 t->traits[k]=p->result;p->replace_trait=replace;p->phase=G_ACCEPTED;s->budget=max(-6.0,s->budget-p->cost);
 if(p->passion_applied)s->passion=0;
 if(g_traits[p->result].color==3)s->heat=0;else if(p->quality<0.5)s->heat=min(g_params.heat_cap,s->heat+g_params.heat_gain);
 return G_OK;
}
int g_ai_reroll(const GState*s,u32 stage){if(!s||stage>2)return 0;const GSlot*p=&s->slots[stage];if(p->phase!=G_PENDING||s->rerolls_used>=G_REROLLS)return 0;return p->outcome==G_MISS||(g_traits[p->result].color!=3&&p->quality<g_params.ai_threshold);}
u16 g_ai_replacement(const GTarget*t,const GDistribution*d){u32 j;u16 id=0;double low=1e100;for(j=0;j<9;j++){u16 v=t->traits[j];if(!v||v>=MAX_TRAITS)continue;double q=d->raw[v]+(g_traits[v].color==3?1.0:0);if(g_traits[v].color==2)q=-1;if(q<low){low=q;id=v;}}return id;}
/* Validate persisted values, not just the outer file checksum. Padding is ignored
   for initialized records; zero-initialized records must be entirely empty. */
int g_validate(const GState*s){
 u32 j,k,total=0,pending=0,misses=0;u16 awarded_x=0;double spent=0;
 if(!s)return 0;
 if(!s->initialized){const u8*b=(const u8*)s;for(j=0;j<sizeof(*s);j++)if(b[j])return 0;return 1;}
 if(s->initialized!=1||s->rerolls_used>G_REROLLS||!(s->budget>=-6&&s->budget<=g_params.budget_start)||!(s->heat>=0&&s->heat<=g_params.heat_cap))return 0;
 if(s->x_mask&~511u||s->passion>1||s->cancelled_rerolls>s->rerolls_used)return 0;
 total=s->cancelled_rerolls;
 for(j=0;j<9;j++)if(s->base_traits[j]>=MAX_TRAITS&&s->base_traits[j]!=NO_TRAIT)return 0;
 for(j=0;j<3;j++){
  const GSlot*x=&s->slots[j];
  if(x->phase>G_DECLINED||x->rerolls>G_REROLLS||x->anomaly>1||x->transcend>1||x->full>1||x->star>1||(x->transcend&&!x->anomaly))return 0;
  if(!x->phase){const u8*b=(const u8*)x;for(k=0;k<sizeof(*x);k++)if(b[k])return 0;continue;}
  if(x->outcome>G_MISS||x->passion_applied>1)return 0;
  if(x->outcome==G_MISS){
   if(j==2||x->result||x->cost||x->raw||x->quality||x->anomaly||x->transcend||x->passion_applied||x->phase==G_DECLINED||!(x->weight>0&&x->weight<=1))return 0;
   if(x->x_index>=9&&x->x_index!=G_NO_X_SLOT)return 0;
   if(x->phase==G_PENDING){if(x->x_index!=G_NO_X_SLOT||s->passion)return 0;pending++;}
   else{misses++;if(x->x_index<9)awarded_x|=(u16)(1u<<x->x_index);}total+=x->rerolls;continue;
  }
  if(!x->result||x->result>=MAX_TRAITS||(g_traits[x->result].color!=1&&g_traits[x->result].color!=3))return 0;
  if(x->replace_trait>=MAX_TRAITS&&x->replace_trait!=NO_TRAIT)return 0;
  if(!(x->raw>=0&&x->raw<=1e6)||!(x->weight>=0&&x->weight<=1)||!(x->quality>=0&&x->quality<=1))return 0;
  if(x->cost!=(g_traits[x->result].color==3?g_params.gold_cost:g_params.blue_cost))return 0;
  if(x->phase==G_PENDING){if(!(x->weight>0))return 0;pending++;}
  if(x->phase==G_DECLINED&&(!x->full||!x->star))return 0;
  if(x->phase==G_ACCEPTED)spent+=x->cost;
  total+=x->rerolls;
 }
 return !(s->x_mask&~awarded_x)&&(!s->passion||misses)&&pending<=1&&total==s->rerolls_used&&(s->budget-max(-6.0,g_params.budget_start-spent)<1e-9)&&(s->budget-max(-6.0,g_params.budget_start-spent)>-1e-9);
}

/* Candidate-relative evaluation; never used by g_distribution, g_roll or AI.
 * Utility reuses the engine's gold preference and fit quality. A poor fit is
 * value-gated so rarity alone cannot receive a dramatic caption. */
u32 g_grade(double score){if(!(score>=25))return 0;if(score<45)return 1;if(score<65)return 2;if(score<82)return 3;if(score<95)return 4;return 5;}
void g_evaluate(const GDistribution*d,const GSlot*x,GEvaluation*e){
 u32 i,n=0;double less=0,surprise=0,value;
 mc_zero(e,sizeof(*e));e->version=1;
 if(x&&x->outcome==G_MISS){e->version=0;e->grade=0xffffffffu;return;}
 if(!d||!x||!x->result||x->result>=MAX_TRAITS||!(d->total>0)||!(d->weight[x->result]>0))return;
 value=d->quality[x->result]+(g_traits[x->result].color==3?1.0:0.0);
 for(i=1;i<MAX_TRAITS;i++)if(d->weight[i]>0){
  double v=d->quality[i]+(g_traits[i].color==3?1.0:0.0);n++;
  less+=v<value?1.0:v==value?0.5:0.0;
  surprise+=d->weight[i]>d->weight[x->result]?1.0:d->weight[i]==d->weight[x->result]?0.5:0.0;
 }
 if(!n)return;e->value=less/n;e->surprise=surprise/n;
 e->rarity=8.0/(8.0+g_trait_holders(x->result));
 e->special=x->transcend?1.0:x->anomaly?0.7:g_traits[x->result].color==3?0.35:0.0;
 e->score=100.0*(.50*e->value+.25*e->surprise*e->value+.15*e->rarity+.10*e->special);
 if(e->value<.35)e->score=min(44.0,e->score);
 else if(e->value<.60||x->quality<.20)e->score=min(64.0,e->score);
 if(x->transcend&&e->value>=.80&&x->quality>=.50)e->score=max(e->score,95.0);
 else if(x->anomaly&&e->value>=.80&&x->quality>=.50)e->score=max(e->score,82.0);
 e->score=max(0.0,min(100.0,e->score));e->grade=g_grade(e->score);
}
