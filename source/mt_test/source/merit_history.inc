/* R11 durable native reward ledger. Wire size stays exactly 20 bytes.
 * reserved[0]: bit0 ever-ruler, bit1 valid ledger; [1]: upper/lower nibbles;
 * [2]: highest observed native rank. Never derive spent rewards from trait count.
 */
#define PROFILE_HISTORY 0x02u
static int history_profile_valid(const Profile*p){
 if(p->reserved[0]&~(PROFILE_RULER_VETERAN|PROFILE_HISTORY))return 0;
 if(!(p->reserved[0]&PROFILE_HISTORY))return !p->reserved[1]&&!p->reserved[2];
 return (p->reserved[1]&15)+(p->reserved[1]>>4)<=3&&p->reserved[2]<=6;
}
/* Internal entry: caller either checked the live officer range in this same
   operation or owns the verified native CPersonData callback contract (R14).
   No cached page permissions or stale person pointers. */
/* R39 A1: recruited-officer (항장) reset. Native recruit 0x2717d0 (1.0.1,
   block 0x272003..0x272074) sets rank 1/2, merit(+0x1be) to the negative
   recruit value and both reward counts (+0x1c2 word) to 0; traits untouched.
   A plain merit deduction only lowers rank and keeps the counts, so it never
   matches and keeps the anti-regacha ledger. On a match: keep native rank and
   counts, delete ONLY traits this module's gacha actually installed (finalized
   ACCEPTED, not MISS, not declined), compact the nine slots, and wipe the
   ledger/gacha state so the officer starts again from stage one. */
#define R39_MERIT_FROM_RANK 2 /* merit i16 sits right after rank: +0x1be */
#define R39_SLOT_BITS_FROM_TRAITS 0x6c /* +0x184 per-slot bit mask */
#define R39_SLOT_WORDS_FROM_TRAITS 0x6e /* +0x186 per-slot words */
static u32 r39_recruit_resets,r39_recruit_removed;
static int recruit_reset_match(u8*o,const Profile*p,u32 top,u32 low,u32 rank){
 if(!(p->reserved[0]&PROFILE_HISTORY)||top||low||rank>2)return 0;
 if((short)S(o,OFF(O_RANK)+R39_MERIT_FROM_RANK)>=0)return 0;
 return (p->reserved[1]!=0)||p->award_mask||p->reserved[2]>rank||(p->reserved[0]&PROFILE_RULER_VETERAN);
}
static int recruit_reset_apply(u8*o,u16 id,u32 rank){Profile*p=&profiles[id];GState*g=&states[id];u16*t=(u16*)(o+OFF(O_TRAITS));
 u16 drop[3],nt[9],nw[9],fill=0,bits,nb=0;u16*w=(u16*)(o+OFF(O_TRAITS)+R39_SLOT_WORDS_FROM_TRAITS);u32 j,k,n=0,nd=0,removed=0;
 for(j=0;j<3;j++)if(g->initialized&&g->slots[j].phase==G_PENDING)return 0; /* uncommitted draw: defer */
 for(j=0;j<3;j++){u16 a=p->awards[j];if(!(p->award_mask&(1u<<j))||!a||a==NO_TRAIT)continue;
  if(g->initialized&&(g->slots[j].phase!=G_ACCEPTED||g->slots[j].outcome!=G_TRAIT||g->slots[j].result!=a))continue;
  drop[nd++]=a;}
 bits=S(o,OFF(O_TRAITS)+R39_SLOT_BITS_FROM_TRAITS);
 for(k=0;k<9;k++)if(t[k]==NO_TRAIT){fill=NO_TRAIT;break;}
 for(k=0;k<9;k++){u16 v=t[k];int gone=0;
  if(!v||v==NO_TRAIT)continue;
  for(j=0;j<nd;j++)if(drop[j]==v){gone=1;drop[j]=0;break;}
  if(gone){removed++;continue;}
  if(bits&(1u<<k))nb|=(u16)(1u<<n);nw[n]=w[k];nt[n++]=v;}
 if(removed){for(k=0;k<9;k++){t[k]=k<n?nt[k]:fill;w[k]=k<n?nw[k]:0;}
  S(o,OFF(O_TRAITS)+R39_SLOT_BITS_FROM_TRAITS)=(u16)((bits&~0x1ffu)|nb);S(o,OFF(O_FLAGS))|=0x8000;}
 p->award_mask=0;p->awards[0]=p->awards[1]=p->awards[2]=NO_TRAIT;
 p->reserved[0]=PROFILE_HISTORY;p->reserved[1]=0;p->reserved[2]=(u8)rank;
 mc_zero(g,sizeof(*g));initialize_gstate(o);
 r39_recruit_resets++;r39_recruit_removed+=removed;
 report(removed?"MeritTraits R39: RECRUITED_OFFICER_RESET; gacha traits removed, ledger cleared.\r\n":"MeritTraits R39: RECRUITED_OFFICER_RESET; ledger cleared.\r\n");
 return 1;
}
static void sync_merit_history_checked(u8*o,u16 id){Profile*p;u32 top,low,oldtop,oldlow,rank,j;
 if(id>=MAX_OFFICERS||!profiles[id].valid)return;
 p=&profiles[id];top=B(o,OFF(O_UPPER));low=B(o,OFF(O_LOWER));rank=B(o,OFF(O_RANK));
 if(top+low>3||rank>6){static u32 n;if(++n<=3)report("MeritTraits R40: native reward counts out of range for one officer; ledger left untouched for that officer.\r\n");return;}
 if(!history_profile_valid(p)){ /* R40 A12: rebuild this officer's ledger from native counts instead of disabling the module */
  p->reserved[0]=(u8)((p->reserved[0]&PROFILE_RULER_VETERAN)|PROFILE_HISTORY);p->reserved[1]=(u8)(top|(low<<4));p->reserved[2]=(u8)rank;r40_officer_resets++;
  {static u32 n;if(++n<=3)report("MeritTraits R40: invalid reward ledger for one officer rebuilt from native counts (others unchanged).\r\n");}
 }
 if(!(p->reserved[0]&PROFILE_HISTORY)){
  /* Migrate R9/R10 without rerolling a finalized trait/MISS/decline. */
  for(j=0;j<3;j++)if((p->award_mask&(1u<<j))&&top<j+1)top=j+1;
  if(top+low>3){static u32 n;if(++n<=3)report("MeritTraits R40: legacy history conflict for one officer; that officer left unmigrated (others continue).\r\n");return;}
  p->reserved[0]|=PROFILE_HISTORY;p->reserved[1]=(u8)(top|(low<<4));p->reserved[2]=(u8)rank;
 }else if(recruit_reset_match(o,p,top,low,rank)&&recruit_reset_apply(o,id,rank)){
  return; /* native rank/counts stay exactly as the recruit set them */
 }else{
  oldtop=p->reserved[1]&15;oldlow=p->reserved[1]>>4;
  /* Adopt only a monotonic native increase; a transfer/reset or crossed pair
     cannot erase already-paid domestic/upper choices or mint extra rewards. */
  if(top>=oldtop&&low>=oldlow)p->reserved[1]=(u8)(top|(low<<4));
  else{top=oldtop;low=oldlow;report("MeritTraits R11: REWARD_HISTORY_RESTORED_AFTER_STATUS_CHANGE\r\n");}
 }
 if(rank<p->reserved[2])rank=p->reserved[2];
 if(ruler_veteran_id(id)){int sr=senior_rank();if(sr>0&&rank<(u32)sr)rank=(u32)sr;}
 if(p->reserved[2]<rank)p->reserved[2]=(u8)rank;
 B(o,OFF(O_UPPER))=(u8)top;B(o,OFF(O_LOWER))=(u8)low;B(o,OFF(O_RANK))=(u8)rank;
}
static void sync_merit_history(u8*o){if(o&&rd(o,OFF(O_LOWER)+1))sync_merit_history_checked(o,S(o,OFF(O_OFFICER_ID)));}
static void sync_all_merit_history(void){for(u32 id=0;id<res->officer_count;id++)if(profiles[id].valid)sync_merit_history(officer(id));}
