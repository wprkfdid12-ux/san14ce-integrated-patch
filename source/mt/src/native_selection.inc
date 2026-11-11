/* R6: original six-button reward selector. 0 means unlocked but disabled,
 * 1 means selectable (selected OR next), 2 earned, 3 rank-locked.
 * Read the original packed selection through its original getter. The native
 * row renderer independently confirms the meaning of all four states. */
typedef struct {u8 top,lower,earned_top,earned_lower,capacity;int status[6];} MeritChoice;
/* Captured 0x781810 reads the selection tree at MAIN layout+0x198 and
 * rows at +0x1f8/+0x200. MAIN+0x1d8 is the separately allocated transition
 * controller (0x6ef260), not the selection owner. Never traverse its tail. */
static int choice_owner_valid(void*owner){
 if(!rd(owner,0x208))return 0;
 for(u32 i=0;i<res->hook_count;i++)if(res->hooks[i].kind==H_UI_ENTER)
  return res->hooks[i].rva>=0x38&&P(owner,0)==game+res->hooks[i].rva-0x38;
 return 0;
}
static void*native_selection_call(u32 site,const u8 expected[8]){
 u8*p=game+site;if(site>res->image_size-5||!rd(p,5)||!mc_equal(p,expected,5)||p[0]!=0xe8)return 0;
 u8*target=p+5+*(int*)(p+1);return target>=game&&target<game+res->image_size&&rd(target,16)?target:0;
}
static int choice_read(void*owner,u8*o,MeritChoice*c){u8 pair[2]={255,255};u32 i;
 if(!rd(owner,res->selection.tree_offset+8)||!rd(o,OFF(O_LOWER)+1))return 0;
 void(*get)(void*,void*,u32)=native_selection_call(res->selection.counts_site,res->selection.counts_call);if(!get)return 0;
 mc_zero(c,sizeof(*c));get(owner,pair,S(o,OFF(O_OFFICER_ID)));
 c->top=pair[0];c->lower=pair[1];c->earned_top=B(o,OFF(O_UPPER));c->earned_lower=B(o,OFF(O_LOWER));
 if(c->top>3||c->lower>3||c->earned_top>c->top||c->earned_lower>c->lower)return 0;
 merit_status(owner,o,c->status);
 for(i=0;i<6;i++){if(c->status[i]<0||c->status[i]>3)return 0;if(c->status[i]!=3&&c->capacity<i%3+1)c->capacity=(u8)(i%3+1);}
 return c->top+c->lower<=c->capacity&&c->capacity<=3;
}
static u8*choice_node(void*owner,u16 id){u8*head,*n;u32 depth=0;
 if(!rd(owner,res->selection.tree_offset+8)||(head=P(owner,res->selection.tree_offset))==0||!rd(head,0x21))return 0;
 n=P(head,8);while(n&&n!=head&&depth++<64){if(!rd(n,0x21)||B(n,0x19))return 0;u32 key=D(n,0x1c);if(key==id)return n;n=P(n,id<key?0:16);}return 0;
}
static int choice_restore(void*owner,u8*o){u8*n=choice_node(owner,S(o,OFF(O_OFFICER_ID)));if(!n)return 0;
 /* Only the transient menu record is restored. Officer rank/earned counts,
    costs, stats and traits are never written during cancellation. */
 B(n,0x20)=(u8)(B(o,OFF(O_UPPER))|(B(o,OFF(O_LOWER))<<4));return 1;
}
static void choice_refresh(void*owner){void(*refresh)(void*)=native_selection_call(res->selection.refresh_site,res->selection.refresh_call);if(refresh&&rd(owner,res->selection.tree_offset+8)){ui_notification_depth++;refresh(owner);ui_notification_depth--;}}
static int choice_next_upper(const MeritChoice*c,const GState*s,u32 j){
 if(j>=3)return 0;
 /* Previously earned military rewards can be converted once, in order. */
 if(j<c->earned_top){for(u32 k=0;k<j;k++)if(s->slots[k].phase<G_ACCEPTED)return 0;return s->slots[j].phase<G_ACCEPTED;}
 return j==c->top&&c->top+c->lower<c->capacity&&c->status[j]==1;
}
static int choice_is_selected(const MeritChoice*c,u32 j){return j<3?j<c->top:j-3<c->lower;}

static int choice_cancel_walk(u8*n,u8*head,u32 depth){
 if(n==head)return 1;if(depth>64||!rd(n,0x21)||B(n,0x19))return 0;
 if(!choice_cancel_walk(P(n,0),head,depth+1))return 0;u8*o=officer(D(n,0x1c));
 if(o)B(n,0x20)=(u8)(B(o,OFF(O_UPPER))|(B(o,OFF(O_LOWER))<<4));
 return choice_cancel_walk(P(n,16),head,depth+1);
}
static void choice_cancel_all(void*owner){u8*head;
 if(!choice_owner_valid(owner)){report("MeritTraits R9: CANCEL_INVALID_SELECTION_OWNER\r\n");return;}
 if(!rd(owner,res->selection.tree_offset+8)||(head=P(owner,res->selection.tree_offset))==0||!rd(head,0x21))return;
 if(choice_cancel_walk(P(head,8),head,0))choice_refresh(owner);
}

static int choice_changed_walk(u8*n,u8*head,u32 depth){
 if(n==head)return 0;if(depth>64||!rd(n,0x21)||B(n,0x19))return -1;
 int left=choice_changed_walk(P(n,0),head,depth+1),right=choice_changed_walk(P(n,16),head,depth+1);
 if(left<0||right<0)return -1;u8*o=officer(D(n,0x1c));if(!o)return -1;
 return left||right||B(n,0x20)!=(u8)(B(o,OFF(O_UPPER))|(B(o,OFF(O_LOWER))<<4));
}
static int choice_has_changes(void*owner){u8*head;
 if(!choice_owner_valid(owner))return -1;
 if(!rd(owner,res->selection.tree_offset+8)||(head=P(owner,res->selection.tree_offset))==0||!rd(head,0x21))return -1;
 return choice_changed_walk(P(head,8),head,0);
}
