/* R16: CE edit eligibility, separate from the mod's directed hierarchy.
 * Evidence: analysis/r16_trait_native.json and the supplied R15 audit.
 * Rule snapshots change only on world/UI/AI-commit events, never UI queries.
 * Neither existing traits nor the game's effect/eligibility routines are edited.
 */
#include "trait_rules_db.h"
static GTraitRule trait_rules[MAX_TRAITS];
static u64 trait_blocks[MAX_TRAITS][6];
static int trait_rules_ready;
u32 g_rules_revision;
static const u16 native_id_groups[][7]={{71,72,73,74,75,76,0},{69,71,72,73,74,0,0},{70,75,76,0,0,0,0},{187,188,0,0,0,0,0},{193,194,0,0,0,0,0}};
static const u8 native_effect_groups[][6]={{1,2},{2,3},{9,21},{29,30},{30,31},{10,21,22},{11,21,23},{12,21,24},{9,10},{9,11},{9,12},{25,26,56,102},{83,84,85,86,87},{56,112},{115,116}};
static int rule_pair(u16 a,u16 b){
 const GTraitRule*x=&trait_rules[a],*y=&trait_rules[b];u32 i,j,k;
 if(mc_blocks(a,b))return 1;
 for(i=0;i<sizeof(native_id_groups)/sizeof(native_id_groups[0]);i++){
  int p=0,q=0;for(j=0;j<7&&native_id_groups[i][j];j++){p|=native_id_groups[i][j]==a;q|=native_id_groups[i][j]==b;}if(p&&q)return 1;
 }
 if(!x->present||!y->present)return 0;
 for(i=0;i<2;i++)for(j=0;j<2;j++){
  u8 p=x->effect[i],q=y->effect[j];if(!p||!q)continue;
  /* Native custom-trait branch is independent of activation condition. */
  if(p==q&&(a>=256||b>=256))return 1;
  if(x->condition!=y->condition)continue;
  if(p!=q)for(k=0;k<sizeof(native_effect_groups)/sizeof(native_effect_groups[0]);k++){
   int m=0,n=0;for(u32 h=0;h<6&&native_effect_groups[k][h];h++){m|=native_effect_groups[k][h]==p;n|=native_effect_groups[k][h]==q;}if(m&&n)return 1;
  }
  /* Native sign test treats zero as nonnegative. Linked-officer validity is
     read from the live trait object; no speculative eleven-pair blacklist. */
  if(p==q&&!x->linked&&((x->condition_value==NO_TRAIT)==(y->condition_value==NO_TRAIT))&&
     ((x->amount[i]<0)!=(y->amount[j]<0)))return 1;
 }
 return 0;
}
static void rules_build(void){
 mc_zero(trait_blocks,sizeof(trait_blocks));
 for(u16 a=1;a<MAX_TRAITS;a++)for(u16 b=1;b<MAX_TRAITS;b++)if(rule_pair(a,b))trait_blocks[a][b>>6]|=1ULL<<(b&63);
 trait_rules_ready=1;g_rules_revision++;
}
void g_rules_update(const GTraitRule*rules){
 if(!rules)rules=original_trait_rules;
 if(!trait_rules_ready){mc_copy(trait_rules,rules,sizeof(trait_rules));rules_build();return;}
 u8 changed[MAX_TRAITS];u16 ids[MAX_TRAITS];u32 count=0;
 for(u32 i=0;i<MAX_TRAITS;i++){changed[i]=(u8)!mc_equal(&trait_rules[i],&rules[i],sizeof(GTraitRule));if(changed[i])ids[count++]=(u16)i;}
 if(!count)return;mc_copy(trait_rules,rules,sizeof(trait_rules));
 /* A pair depends only on its two current trait records. A changed link,
    condition or effect therefore invalidates that row AND that column, never
    unrelated pairs. Preserve directed hierarchy and native asymmetry exactly. */
 for(u16 a=1;a<MAX_TRAITS;a++){
  if(changed[a]){mc_zero(trait_blocks[a],sizeof(trait_blocks[a]));
   for(u16 b=1;b<MAX_TRAITS;b++)if(rule_pair(a,b))trait_blocks[a][b>>6]|=1ULL<<(b&63);
  }else for(u32 i=0;i<count;i++){u16 b=ids[i];if(!b)continue;u64 bit=1ULL<<(b&63);
   if(rule_pair(a,b))trait_blocks[a][b>>6]|=bit;else trait_blocks[a][b>>6]&=~bit;
  }
 }
 g_rules_revision++;
}
int g_blocks(u16 held,u16 candidate){
 if(!held||held>=MAX_TRAITS||!candidate||candidate>=MAX_TRAITS)return 0;
 if(!trait_rules_ready)g_rules_update(0);
 return (int)((trait_blocks[held][candidate>>6]>>(candidate&63))&1);
}
int g_allowed(const GTarget*t,u16 candidate){
 if(!t||!candidate||candidate>=MAX_TRAITS)return 0;
 if(!trait_rules_ready)g_rules_update(0);
 if(!trait_rules[candidate].present)return 0;
 u8 c=trait_rules[candidate].condition;
 if((c==21||c==22)&&t->sex!=1)return 0;
 for(u32 j=0;j<9;j++)if(g_blocks(t->traits[j],candidate))return 0;
 return 1;
}
