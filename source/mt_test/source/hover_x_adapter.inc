/* CEKO + exact bundled Hover .text SHA-256 only. Display-only extension of the
 * existing renderer: never register a fake trait, hook an effects getter, or
 * write a native trait slot. Native / other-mod descriptions stay authoritative.
 * Evidence and call-site inventory: analysis/hover_x_contract.md. */
static u8*hover_x_relays;
static CorePatch hover_x_patches[7];
static u32 hover_x_count;
static u8 hover_x_caption[0xc0],hover_empty_caption[0xc0];
#define HC(r,t) ((t)(hover_module+(r)))
#define XC(r,t) ((t)(game+(r)))

static u16 display_x_mask(u8*person){u16 id;u8*live;GTarget t;
 if(!core||!res||!game||!enabled||!metadata_ready||metadata_fault||!rd(person,OFF(O_OFFICER_ID)+2))return 0;
 id=S(person,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS||!states[id].initialized||!states[id].x_mask)return 0;
 if(!(live=officer(id)))return 0;
 /* Read the live physical slots even when an info layout supplies its base
    person interface. External edits take precedence; cleared X never moves. */
 if(!states[id].x_mask)return 0;target_read(live,&t);g_reconcile(&t,&states[id]);return states[id].x_mask;
}
__declspec(dllexport) int SAN14_MeritSlotDisplayV1(u32 id,u32 slot,u16*trait,u32*is_x){u8*o;
 if(trait)*trait=0;if(is_x)*is_x=0;
 /* Other modules may resolve this export before our initializer or a world
    load. Return unavailable, so their original native display can continue. */
 if(!trait||!is_x||!core||!res||!game||!enabled||!metadata_ready||metadata_fault||id>=MAX_OFFICERS||slot>=9||!(o=officer(id)))return 0;
 u16 mask=display_x_mask(o);*trait=S(o,OFF(O_TRAITS)+slot*2);if(*trait==NO_TRAIT)*trait=0;
 *is_x=!(*trait)&&((mask>>slot)&1);return 1;
}
/* Display ALL actual native traits, including red traits. The gacha candidate
   cache intentionally admits only blue/gold and must not govern this view. */
static u8*display_trait(u16 key){u8*d,*t;
 if(!res||!game||!key||key==NO_TRAIT||key>=res->trait_count||key>=MAX_TRAITS||!(d=db())||!rd(d+OFF(O_TRAIT_TABLE)+key*8,8))return 0;
 t=P(d,OFF(O_TRAIT_TABLE)+key*8);
 return rd(t,OFF(O_TRAIT_COLOR)+1)&&rd(t+OFF(O_TRAIT_NAME),2)&&S(t,OFF(O_TRAIT_NAME))?t:0;
}
static u32 hover_x_collect(void*person,void*context,u32 mode,u32 filter,u8**items,u32*slots,u32*active){
 PERF_HIT(PERF_HOVER_COLLECT);
 u32 n=HC(0x2080,u32(*)(void*,void*,u32,u32,u8**,u32*,u32*))(person,context,mode,filter,items,slots,active),i;
 u16 mask=display_x_mask(person);u8*copy[9]={0};u32 flags[9]={0};
 if(!mask||n>9||!items||!slots||!active)return n;
 for(i=0;i<n;i++)if(slots[i]<9){copy[slots[i]]=items[i];flags[slots[i]]=active[i];}
 /* A physical nine-cell view avoids compaction moving X onto another trait.
    Empty/X descriptors are text-only and are NEVER in a gameplay database. */
 for(i=0;i<9;i++){slots[i]=i;items[i]=copy[i]?copy[i]:(mask&(1u<<i))?hover_x_caption:hover_empty_caption;active[i]=copy[i]?flags[i]:0;}
 return 9;
}
static void hover_x_info(void*layout){u8*o,*cache,*base[5];u16 mask,id;u32 i;
 PERF_HIT(PERF_HOVER_INFO);
 HC(0x16a0,Void1)(layout);
 if(!rd(layout,0x138)||!rd(P(layout,0),0x1a8))return;
 o=V(layout,0x1a0,u8*(*)(void*))(layout);mask=display_x_mask(o);
 if(!enabled||!metadata_ready||metadata_fault||!rd(o,OFF(O_OFFICER_ID)+2))return;
 id=S(o,OFF(O_OFFICER_ID));if(id>=MAX_OFFICERS||(!mask&&!profiles[id].award_mask))return;
 o=officer(id);if(!o)return;
 /* R16: acquired real traits also need live physical-slot labels. The base
    person interface can contain stale/sorted original traits after a draw. */
 for(i=0;i<5;i++){base[i]=XC(0x92c390,u8*(*)(void*,int))((u8*)layout+0x130,31+2*i);if(!rd(base[i],0x148))return;}
 cache=HC(0x1190,u8*(*)(void*,int))(layout,1);if(!rd(cache,0x60)||D(cache,0x50))return;
 if(!P(cache,0x48)&&!HC(0x1420,int(*)(void*,void*))(cache,base))return;
 for(i=0;i<9;i++)if(!rd(P(cache,8+i*8),0x148))return;
 for(i=0;i<9;i++){u8*label=P(cache,8+i*8);u16 key=S(o,OFF(O_TRAITS)+i*2);u8*t=display_trait(key);int real=t!=0;
  D(label,0x80)=i;
  if(real||((mask>>i)&1)){
   XC(0x85c270,Text)(label,real?(const u16*)(t+OFF(O_TRAIT_NAME)):L"X");
   XC(0x856bc0,void(*)(void*,int,int))(label,real?B(t,OFF(O_TRAIT_COLOR)):0,1);
  }
  set_visible(label,real||((mask>>i)&1));
 }
 for(i=0;i<5;i++)set_visible(base[i],0);
}

typedef const u16*(*ColumnText)(void*);
static ColumnText hover_column_original[3][9];
static const u16*hover_x_column(void*context,u32 kind,u32 slot){u8*p=context;
 PERF_HIT(PERF_HOVER_COLUMN);
 if(kind==2){if(!rd(p,0x14))return L"";p=HC(0x1fd0,u8*(*)(u32))(S(p,0x12));}
 if(display_x_mask(p)&(1u<<slot))return L"X";
 return hover_column_original[kind][slot]?hover_column_original[kind][slot](context):L"";
}
#define XCOLUMN(k,s) static const u16*xcol_##k##_##s(void*p){return hover_x_column(p,k,s);}
#define XCOLSET(k) XCOLUMN(k,0) XCOLUMN(k,1) XCOLUMN(k,2) XCOLUMN(k,3) XCOLUMN(k,4) XCOLUMN(k,5) XCOLUMN(k,6) XCOLUMN(k,7) XCOLUMN(k,8)
XCOLSET(0) XCOLSET(1) XCOLSET(2)
#define XCOLPTR(k) {xcol_##k##_0,xcol_##k##_1,xcol_##k##_2,xcol_##k##_3,xcol_##k##_4,xcol_##k##_5,xcol_##k##_6,xcol_##k##_7,xcol_##k##_8}
static ColumnText const hover_column_replacement[3][9]={XCOLPTR(0),XCOLPTR(1),XCOLPTR(2)};
static void hover_x_columns(u8*table,u8*source,u32 kind,u32 count){u32 k,i,index;
 HC(0x3610,void(*)(void*,void*,u32,u32))(table,source,kind,count);
 if(kind>1||!rd(table,0x6300))return;
 for(k=0;k<3;k++){
  if((kind==1&&k!=1)||(kind==0&&k==1))continue;
  for(i=0;i<9;i++){
   index=i<5?(k==0?0x3d:k==1?0x23:0xaa)+i:(k==2?0x104:0x100)+i-5;
   if(i<5&&index>=count)continue;u8*entry=table+index*0x60;
   ColumnText old=(ColumnText)P(entry,0x18);if(!old||D(entry,0)!=index||old==hover_column_replacement[k][i])continue;
   /* Other mods may supply their own columns. Chain only the same previously
      observed function; an incompatible callback is never overwritten. */
   if(hover_column_original[k][i]&&hover_column_original[k][i]!=old)continue;
   hover_column_original[k][i]=old;P(entry,0x18)=(u8*)hover_column_replacement[k][i];
  }
 }
}
static void*display_allocate_near(u8*module,u32 image_size){usize anchor=(usize)module;u64 start=GetTickCount64();u32 pass,queries=0;
 for(pass=0;pass<2;pass++){
  usize cur=pass?(anchor>0x60000000?anchor-0x60000000:0x10000):anchor+image_size;
  usize stop=pass?anchor:anchor+0x60000000;
  while(cur<stop&&queries++<192&&GetTickCount64()-start<=150){MemoryRegion m;
   if(VirtualQuery((void*)cur,&m,sizeof(m))!=sizeof(m)||!m.region_size)break;
   usize end=(usize)m.base+m.region_size,at=(cur+0xffff)&~(usize)0xffff;if(end<=cur)break;
   if(m.state==0x10000&&at<stop&&end>=at+4096){void*p=VirtualAlloc((void*)at,4096,0x3000,4);if(p){if((usize)p==at)return p;VirtualFree(p,0,0x8000);return 0;}}
   cur=end;
  }
 }
 return 0;
}
static int prepare_hover_x(void){
 static const u32 sites[]={0x2740,0x29a3,0x2e79,0x4355,0x398b,0x53ec};u32 i,old;u8*page;
 if(!hover_module||!rd(hover_module,NX(X_HOVER_IMAGE_SIZE)))return 0;
 mc_zero(hover_x_patches,sizeof(hover_x_patches));hover_x_count=0;
 for(i=0;i<6;i++){u8*p=hover_module+sites[i];u32 to=i<4?0x2080:0x3610;
  if(p[0]!=0xe8||p+5+*(int*)(p+1)!=hover_module+to)return 0;
 }
 u8*p=game+OFF(O_UI9_GUARD);if(!rd(p,14)||p[0]!=0xff||p[1]!=0x25||D(p,2)||P(p,6)!=hover_module+0x16a0)return 0;
 page=display_allocate_near(hover_module,NX(X_HOVER_IMAGE_SIZE));if(!page)return 0;hover_x_relays=page;
 for(i=0;i<2;i++){page[i*32]=0xff;page[i*32+1]=0x25;D(page,i*32+2)=0;Q(page,i*32+6)=i?(usize)hover_x_columns:(usize)hover_x_collect;}
 for(i=0;i<6;i++){CorePatch*q=&hover_x_patches[i];q->address=hover_module+sites[i];q->length=5;mc_copy(q->before,q->address,5);q->after[0]=0xe8;
  iptr delta=(page+(i<4?0:32))-((u8*)q->address+5);if(delta<(-2147483647LL-1)||delta>2147483647LL)return 0;*(int*)(q->after+1)=(int)delta;
 }
 CorePatch*q=&hover_x_patches[6];q->address=p;q->length=14;mc_copy(q->before,p,14);q->after[0]=0xff;q->after[1]=0x25;Q(q->after,6)=(usize)hover_x_info;
 S(hover_x_caption,0x10)='X';
 if(!VirtualProtect(page,4096,0x20,&old)||!FlushInstructionCache(GetCurrentProcess(),page,4096))return 0;
 hover_x_count=7;return 1;
}
static void rollback_hover_x(void){u32 i,old,tmp;
 for(i=0;i<hover_x_count;i++){CorePatch*p=&hover_x_patches[i];if(mc_equal(p->address,p->after,p->length)){
  if(!VirtualProtect(p->address,p->length,0x40,&old)){rollback_live=1;continue;}
  mc_copy(p->address,p->before,p->length);int flushed=FlushInstructionCache(GetCurrentProcess(),p->address,p->length),protected=VirtualProtect(p->address,p->length,old,&tmp);if(!flushed||!protected)rollback_live=1;
 }
 if(!mc_equal(p->address,p->before,p->length))rollback_live=1;
 }
 if(!rollback_live&&hover_x_relays){VirtualFree(hover_x_relays,0,0x8000);hover_x_relays=0;}hover_x_count=0;
}
