/* Optional, exact supplied SortiePresets RC25 display bridge.
 * Do not replace its DLL, selection/input logic, tooltip implementation, or
 * gameplay trait getter. See analysis/presets_x_contract.md for the evidence.
 * Registration is retried only at successful load/new-game boundaries, never
 * in a busy per-frame module search. Unsupported builds remain untouched. */
enum {PX_IMAGE=0x9d000,PX_REFRESH=0x10280,PX_UI=0x34040,PX_BOOK=0x35360,
      PX_GAME=0x958e8,PX_INSTALLED=0x1b028,PX_READY=0x33d40};
static u8*presets_module,*presets_checked,*presets_x_relay;
static int presets_x_status,presets_rollback_live;
static volatile int presets_probe_busy;
static CorePatch presets_x_patches[13];
static u32 presets_x_count;
static const u32 presets_x_sites[13]={0x1101b,0x11062,0x11176,0x119a1,0x12a13,0x131dd,0x13615,0x1395a,0x139dd,0x13a1d,0x13a53,0x13b50,0x14045};

static void presets_x_overlay(void){u8*u,*b,*o,*label;u32 selected,id,i;u16 mask;
 if(presets_x_status!=1||!enabled||!metadata_ready||metadata_fault||!presets_module)return;
 u=presets_module+PX_UI;b=presets_module+PX_BOOK;
 if(!rd(u,0xc00)||!rd(b,0x584)||!rd(presets_module+PX_READY,4)||!D(presets_module,PX_READY)||D(u,0xb70)!=1)return;
 selected=D(u,0xb64);if(selected>=11||D(b,4+selected*0x80)!=1)return;
 id=D(b,8+selected*0x80);if(id>=MAX_OFFICERS||!(o=officer(id))||!(mask=display_x_mask(o)))return;
 for(i=0;i<9;i++)if(mask&(1u<<i)){
  /* RC25 just rendered live, physical slots. A filtered/current real trait
     always wins, even if another mod chooses a different native projection. */
  if(D(u,0xd0+i*4))continue;
  label=P(u,0x58+i*8);
  if(!rd(label,0x148)||P(label,8)!=P(u,0x10)||!rd(P(label,0),0xd0))continue;
  XC(0x85c270,Text)(label,L"X");
  XC(0x856bc0,void(*)(void*,int,int))(label,0,1);set_visible(label,1);
  /* trait_ids stays zero. RC25's existing tooltip never sees a fake ID. */
 }
}
static void presets_x_refresh(int full){
 /* Only call sites are redirected; the original function entry is intact. */
 if(!presets_module)return;
 ((void(*)(int))(presets_module+PX_REFRESH))(full);presets_x_overlay();
}
static void rollback_presets_x(void){u32 i,old,tmp;presets_rollback_live=0;
 for(i=0;i<presets_x_count;i++){CorePatch*p=&presets_x_patches[i];
  if(mc_equal(p->address,p->after,p->length)){
   if(!VirtualProtect(p->address,p->length,0x40,&old)){presets_rollback_live=1;continue;}
   mc_copy(p->address,p->before,p->length);int flushed=FlushInstructionCache(GetCurrentProcess(),p->address,p->length),protected=VirtualProtect(p->address,p->length,old,&tmp);if(!flushed||!protected)presets_rollback_live=1;
  }
  if(!mc_equal(p->address,p->before,p->length))presets_rollback_live=1;
 }
 /* If protection/ownership changed mid-transaction, keep callable memory
    alive until process exit. Never free a possibly referenced relay. */
 if(!presets_rollback_live&&presets_x_relay){VirtualFree(presets_x_relay,0,0x8000);presets_x_relay=0;presets_module=0;}
 presets_x_count=0;
}
static void probe_presets_x_unlocked(void){u8*m,*page;u32 nt,i,old;u8 hash[32];
 static const u8 known_hash[32]={0x8a,0xa4,0x8b,0x5a,0xd8,0x0a,0x49,0xca,0x59,0x81,0x81,0xdb,0x90,0x73,0x1c,0x73,0xc8,0x46,0xc5,0xa4,0x43,0x1c,0x28,0x55,0xf2,0xd0,0x00,0xeb,0xa2,0xeb,0xb5,0x56};
 if(!enabled||!native_ui_ready||!res||!game||presets_x_status==1||presets_rollback_live)return;
 m=GetModuleHandleW(L"SAN14_SortiePresets.dll");if(!m||m==presets_checked)return;
 if(!rd(m,0x1000)||S(m,0)!=0x5a4d)goto unknown;
 nt=D(m,0x3c);if(nt>0xe00||D(m,nt)!=0x4550||S(m,nt+4)!=0x8664||S(m,nt+24)!=0x20b||D(m,nt+80)!=PX_IMAGE)goto unknown;
 if(!rd(m+0x1000,0x15168)||!rd(m+PX_GAME,8)||!rd(m+PX_INSTALLED,4))goto unknown;
 /* ModCore initializes registrations one at a time, in unspecified order.
    A not-yet-initialized module is pending, not an incompatible build. */
 if(!D(m,PX_INSTALLED))return;
 if(D(m,PX_INSTALLED)!=1||P(m,PX_GAME)!=game)goto unknown;
 mc_sha256(m+0x1000,0x15168,hash);if(!mc_equal(hash,known_hash,32))goto unknown;
 for(i=0;i<13;i++){u8*p=m+presets_x_sites[i];if(p[0]!=0xe8||p+5+*(int*)(p+1)!=m+PX_REFRESH)goto unknown;}
 presets_checked=m;presets_x_status=-1;page=display_allocate_near(m,PX_IMAGE);
 if(!page){report("MeritTraits R4: RC25 display bridge unavailable (bounded relay allocation); original presets preserved.");return;}
 presets_module=m;presets_x_relay=page;mc_zero(presets_x_patches,sizeof(presets_x_patches));presets_x_count=0;
 page[0]=0xff;page[1]=0x25;D(page,2)=0;Q(page,6)=(usize)presets_x_refresh;
 for(i=0;i<13;i++){CorePatch*p=&presets_x_patches[i];p->address=m+presets_x_sites[i];p->length=5;mc_copy(p->before,p->address,5);p->after[0]=0xe8;
  iptr delta=page-((u8*)p->address+5);if(delta<(-2147483647LL-1)||delta>2147483647LL)goto failed;
  *(int*)(p->after+1)=(int)delta;
 }
 if(!VirtualProtect(page,4096,0x20,&old)||!FlushInstructionCache(GetCurrentProcess(),page,4096))goto failed;
 presets_x_count=13;if(!install_display_calls(m,PX_IMAGE,presets_x_patches,presets_x_count))goto failed;
 presets_x_status=1;report("MeritTraits R4: exact RC25 preset physical-slot X display ACTIVE; original real-trait names/hover/input preserved.");return;
failed:
 rollback_presets_x();report("MeritTraits R4: RC25 display bridge failed; owned changes rolled back where permitted. Gacha remains active.");return;
unknown:
 presets_checked=m;presets_x_status=-1;report("MeritTraits R4: unrecognized/modified SortiePresets build; no preset bytes overwritten. Shared native Hover remains active.");
}
static void probe_presets_x(void){
 if(!__sync_bool_compare_and_swap(&presets_probe_busy,0,1))return;
 probe_presets_x_unlocked();__sync_lock_release(&presets_probe_busy);
}
