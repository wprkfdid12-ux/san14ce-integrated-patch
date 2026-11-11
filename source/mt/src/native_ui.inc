/* CP04 native integration. All game/DLL RVAs and byte fingerprints are supplied
 * by the version resolver. Reuses the already loaded Hover engine, not another
 * tooltip implementation. No game hooks or allocations in a global loop. */
#define PCALL(n,type) ((type)(game+res->popup.funcs[n]))
static int native_guard_check(void){
 u32 i;for(i=0;i<N_COUNT;i++)if(res->native.guard_lengths[i]<8||res->native.guard_lengths[i]>16||!rd(game+res->native.funcs[i],res->native.guard_lengths[i])||
   !mc_equal(game+res->native.funcs[i],res->native.guards[i],res->native.guard_lengths[i])){
  report("MeritTraits: native UI/save adapter fingerprint mismatch; refusing installation.");return 0;
 }
 if(res->selection.tree_offset!=0x198||!native_selection_call(res->selection.counts_site,res->selection.counts_call)||!native_selection_call(res->selection.refresh_site,res->selection.refresh_call)){
  report("MeritTraits R6: native reward selection binding mismatch; refusing installation.");return 0;
 }
 for(i=0;i<P_COUNT;i++)if(res->popup.funcs[i]>res->image_size-16||!rd(game+res->popup.funcs[i],16)||!mc_equal(game+res->popup.funcs[i],res->popup.guards[i],16)){report("MeritTraits R9: native popup fingerprint mismatch.\r\n");return 0;}
 if(res->popup.object_size!=0x180||res->popup.vtable>res->image_size-0xd8)return 0;
 return 1;
}
static int native_cancel_merit(void*layout){
 /* Exact instruction sequence in verified CEKO 0x732f50. Resolve the same
    two callees as native ESC; no undocumented state numbers are fabricated. */
 static const u8 code[]={0xe8,0xa1,0x38,0x12,0x00,0x48,0x83,0xc4,0x20,0x5b,0xe9,0xb7,0xf9,0xcb,0xff};
 u8*p=game+res->funcs[F_STATE_TICK]+0x3a,*button;
 if(!rd(layout,0x1d0)||!rd(p,sizeof(code))||!mc_equal(p,code,sizeof(code)))return 0;
 button=P(layout,0x1c8);if(!rd(button,0x198))return 0;
 void(*trigger)(void*,int)=(void*)(p+5+*(int*)(p+1));
 void(*consume)(void)=(void*)(p+15+*(int*)(p+11));
 if(!rd((void*)trigger,16)||!rd((void*)consume,16))return 0;
 trigger(button,4);consume();return cancel_delivered;
}
static int native_consume_cancel_input(void){
 /* Same verified native ESC tail, without invoking its cancel button. A
    buffered Escape must not reopen the warning after the player selects No. */
 static const u8 code[]={0xe8,0xa1,0x38,0x12,0x00,0x48,0x83,0xc4,0x20,0x5b,0xe9,0xb7,0xf9,0xcb,0xff};
 u8*p=game+res->funcs[F_STATE_TICK]+0x3a;
 if(!rd(p,sizeof(code))||!mc_equal(p,code,sizeof(code)))return 0;
 void(*consume)(void)=(void*)(p+15+*(int*)(p+11));
 if(!rd((void*)consume,16))return 0;consume();return 1;
}
static int bind_shared_hover(void){
 static const u16 *const names[]={L"SAN14_Traits9_UI.dll",L"UI_Traits9_Hover.dll",L"UI_Traits9.dll"};
 u8 *m,*address=game+OFF(O_UI9_GUARD),hash[32];u32 i;usize target;
 if(!rd(address,14)||address[0]!=0xff||address[1]!=0x25||D(address,2))return 0;
 target=Q(address,6);
 for(i=0;i<3;i++){
  m=GetModuleHandleW(names[i]);if(!address_in_module(m,target))continue;
  if(!rd(m+NX(X_HOVER_TEXT),NX(X_HOVER_TEXT_SIZE))||!rd(m+NX(X_HOVER_GAME),8))continue;
  mc_sha256(m+NX(X_HOVER_TEXT),NX(X_HOVER_TEXT_SIZE),hash);
  if(!mc_equal(hash,res->native.hover_text_hash,32)||P(m,NX(X_HOVER_GAME))!=game)continue;
  hover_module=m;
  shared_hover_show=(void(*)(void*,usize))(m+NX(X_HOVER_SHOW));
  shared_hover_release=(void(*)(void*,int,void*))(m+NX(X_HOVER_RELEASE));
  report("MeritTraits: existing Hover renderer fingerprint verified; original numeric descriptions shared.");return 1;
 }
 report("MeritTraits: loaded Hover engine is not a supported binary; existing mod was not overwritten.");return 0;
}
/* Verified native SteamUser020 GetSteamID call uses a hidden CSteamID result
 * pointer (rcx=user, rdx=&id), not a scalar u64 virtual return. */
static u64 native_account_id(void){
 u64 id=0;u8*context,*user;void*(*init)(void*);
 if(!rd(game+NX(X_STEAM_CONTEXT_IAT),8))return 0;
 init=(void*(*)(void*))P(game,NX(X_STEAM_CONTEXT_IAT));if(!rd((void*)init,1))return 0;
 context=init(game+NX(X_STEAM_USER_CONTEXT));if(!rd(context,8))return 0;
 user=P(context,0);if(!rd(user,8)||!rd(P(user,0),24))return 0;
 V(user,0x10,void*(*)(void*,u64*))(user,&id);return id;
}
static void enable_native_tile(u8*row,u32 j,int on,int independent){
 u8 *group,*button;if(!native_ui_ready)return;
 group=P(row,NX(X_ROW_GROUP));button=P(row,OFF(O_ROW_BUTTON)+j*8);
 if(rd(group,0x24))NCALL(N_RADIO,void(*)(void*,int,int))(group,(int)j,on);
 /* Earned/unlocked tiles are now actions, not already selected native radios. */
 if(independent&&rd(button,8)&&rd(P(button,0)+0x208,8))V(button,0x208,Flag)(button,0);
}
static int merit_contains(u8*w,const int*point){
 long long x=0,y=0;int width,height;u32 depth=0;u8*p=w;
 if(!rd(w,0x44)||(width=(int)D(w,0x28))<=0||(height=(int)D(w,0x2c))<=0)return 0;
 while(p&&depth++<30){if(!rd(p,0x44)||!(D(p,0x40)&1))return 0;x+=(int)D(p,0x20);y+=(int)D(p,0x24);p=P(p,8);}
 return !p&&point[0]>=x&&point[0]<x+width&&point[1]>=y&&point[1]<y+height;
}
static int merit_descendant(u8*target,u8*ancestor){
 u32 depth=0;while(target&&depth++<30){if(target==ancestor)return 1;if(!rd(target,16))return 0;target=P(target,8);}return 0;
}
static void hover_merit_row(void*row){
 u8*r=row,*o,*app,*input,*target;u32 id,j;u16 key=0;u32 pid=0;
 if(!native_ui_ready||!shared_hover_show||!enabled||!rd(r,OFF(O_ROW_LOCK_ICON)+48)||P(r,0)!=game+OFF(O_ROW_VTABLE))return;
 if(thanks_active||finalizing||miss_modal||exit_modal||picker_depth||ui_mode==2){shared_hover_show(r,0);return;}
 HANDLE window=GetForegroundWindow();
 if(!window||!GetWindowThreadProcessId(window,&pid)||pid!=GetCurrentProcessId()){shared_hover_show(r,0);return;}
 app=NCALL(N_APP,void*(*)(void))();input=NCALL(N_INPUT,void*(*)(void))();
 target=app?NCALL(N_TARGET,void*(*)(void*))(app):0;
 o=P(r,OFF(O_ROW_OFFICER));
 if(rd(o,OFF(O_LOWER)+1)&&rd(input,0x3c)&&target&&merit_contains(r,(const int*)(input+0x34))){
  id=S(o,OFF(O_OFFICER_ID));
  if(id<MAX_OFFICERS)for(j=0;j<3;j++){
   u8*button=P(r,OFF(O_ROW_BUTTON)+j*8);if(!merit_contains(button,(const int*)(input+0x34)))continue;
   /* Disabled result buttons route native hit targets to the row/parent in
      CE. Accept either ancestry direction, never an unrelated overlay. */
   if(!merit_descendant(target,button)&&!merit_descendant(button,target))continue;
   GSlot*x=&ui_gstate(id)->slots[j];
   if(x->phase>=G_PENDING&&x->outcome==G_TRAIT)key=x->result;
   break;
  }
 }
 if(!key&&rd(input,0x3c))key=control_hover_trait(r,target,(const int*)(input+0x34));
 shared_hover_show(r,valid_trait(key)?key:0);
}
static void hide_merit_hovers(int destroy){
 u32 i;if(!shared_hover_release)return;
 for(i=0;i<visible_row_count;i++)shared_hover_release(visible_rows[i],destroy,0);
}
static void queue_thanks(u16 id){
 u32 i;if(id>=MAX_OFFICERS||thanks_count>=MAX_OFFICERS)return;
 for(i=0;i<thanks_count;i++)if(thanks_ids[i]==id)return;
 thanks_ids[thanks_count++]=id;
}
static void capture_thanks_ruler(void*state){
 u8*city,*force,*d;u32 fid;thanks_ruler=NO_TRAIT;
 if(!native_ui_ready||!rd(state,NX(X_STATE_FORCE)+8)||(city=P(state,NX(X_STATE_FORCE)))==0||!rd(city,0x11))return;
 fid=B(city,0x10);d=db();if(fid>NX(X_FORCE_MAX)||!d||!rd(d+NX(X_FORCE_TABLE)+fid*8,8))return;
 force=P(d,NX(X_FORCE_TABLE)+fid*8);
 if(rd(force,NX(X_FORCE_RULER)+2)){u16 id=S(force,NX(X_FORCE_RULER));if(id<res->officer_count)thanks_ruler=id;}
}
static void show_native_thanks(void*state){
 u32 i,n=thanks_count;u64 epoch=world_epoch;u8*manager=game+NX(X_TEXT_MANAGER);
 if(thanks_active||!n||!native_ui_ready)return;
 thanks_active=1;thanks_count=0;
 if(!rd(manager,8)||!rd(P(manager,0),16)){thanks_active=0;report("MeritTraits: native gratitude text manager unavailable.");return;}
 for(i=0;i<n;i++){
  u16 id=thanks_ids[i];u8*o=officer(id);const u16*text;
  if(world_epoch!=epoch||pending_ui!=state)break;
  if(!o||!CALL(F_VALID,int(*)(void*))(o))continue;
  NCALL(N_RESET_TEXT,Void1)(manager);
  NCALL(N_SET_SPEAKER,void(*)(void*,u32,u32))(manager,id,0);
  if(thanks_ruler!=NO_TRAIT)NCALL(N_SET_SPEAKER,void(*)(void*,u32,u32))(manager,thanks_ruler,1);
  text=V(manager,8,const u16*(*)(void*,u32))(manager,0x5c56);
  if(text)NCALL(N_TALK,void(*)(u32,const u16*))(id,text);
 }
 thanks_active=0;
}

/* The original native text primitive stores normalized glyph scale at +10/+14,
 * verified in N_TEXT_FONT. Preserve and restore the exact values: no widget
 * position, portrait, click rectangle or font resource is resized. */
static u8*fx_primitive,*fx_parameters;static void*fx_owner_row;
static float fx_scale[2];
static u64 fx_started,fx_punch;
static u32 fx_ticks;
static int fx_revealed,fx_running;
static void native_sound(u32 cue){u8*m;
 if(!native_ui_ready||!rd(game+NX(X_SOUND_MANAGER),8)||(m=P(game,NX(X_SOUND_MANAGER)))==0||!rd(m,0x2a8))return;
 /* Original audio manager owns mute/volume, sample lifetime and async playback. */
 NCALL(N_SOUND,int(*)(void*,u32,float,float))(m+0x2a0,cue,0.0f,0.0f); /* 1.0.1: thunk add rcx,0x2a0 removed; call real fn with adjusted this */
}
static u8*caption_primitive(u8*button){u8*list,*prims,*p;
 if(!rd(button,0x188))return 0;list=button+0x130;
 if(D(list,0x10)<=16||D(list,0x10)>256||(prims=P(list,8))==0||!rd(prims+16*8,8))return 0;
 p=P(prims,16*8);
 if(!rd(p,0x113)||B(p,0x112)!=2||!rd(P(p,0),0x88))return 0;
 return p;
}
/* Native textured button primitives expose four vertex colors through verified
 * virtual accessors. Only these original resources are tinted; all exact RGBA
 * values are restored. Unsupported primitive classes remain untouched. */
typedef struct {u8*p;u32 rgba[4];} FxFrame;
static FxFrame fx_frames[16];static u32 fx_frame_count;static int fx_teased;
static int fx_frame_valid(u8*p){
 return native_ui_ready&&rd(p,0x190)&&rd(P(p,0),0x28)&&
  P(P(p,0),0x18)==game+res->native.funcs[N_COLOR_GET]&&
  P(P(p,0),0x20)==game+res->native.funcs[N_COLOR_SET];
}
static u8*fx_button(void){u32 i;for(i=0;i<visible_row_count;i++){
 u8*r=visible_rows[i],*o;if(!rd(r,OFF(O_ROW_BUTTON)+8))continue;
 o=P(r,OFF(O_ROW_OFFICER));if(rd(o,OFF(O_OFFICER_ID)+2)&&S(o,OFF(O_OFFICER_ID))==active_id)return P(r,OFF(O_ROW_BUTTON)+active_stage*8);
 }return 0;
}
static void fx_frame_capture(void){u8*b=fx_button(),*list,*prims;u32 i,j,n;
 if(fx_frame_count||!native_ui_ready||!rd(b,0x148))return;list=b+0x130;n=D(list,0x10);
 if(n<1||n>256||(prims=P(list,8))==0)return;n=n<16?n:16;if(!rd(prims,n*8))return;
 fx_owner_row=P(b,8);
 for(i=0;i<n;i++){u8*p=P(prims,i*8);if(!fx_frame_valid(p)||B(p,0x112)==2)continue;
  /* Shared resource pointers must not be captured a second time after tint. */
  for(j=0;j<fx_frame_count;j++)if(fx_frames[j].p==p)break;if(j<fx_frame_count)continue;
  FxFrame*f=&fx_frames[fx_frame_count++];f->p=p;
  for(j=0;j<4;j++)f->rgba[j]=V(p,0x18,u32(*)(void*,u32))(p,j);
 }
}
static void fx_frame_tint(int reveal){u32 i,j,k,mix,color;GSlot*x;
 if(active_id>=MAX_OFFICERS||active_stage>=3)return;x=&ui_gstate(active_id)->slots[active_stage];
 int rare=x->anomaly||x->transcend,gold=g_traits[x->result].color==3;
 if(!reveal&&!rare&&!gold)return;fx_frame_capture();mix=reveal?(rare||gold?176u:128u):64u;
 for(i=0;i<fx_frame_count;i++){FxFrame*f=&fx_frames[i];if(!fx_frame_valid(f->p))continue;
  for(j=0;j<4;j++){color=rare&&j%2?0xdc92d9u:gold||rare?0xffe6a2u:0xc4e5ffu;
   u32 v=f->rgba[j]&0xff000000u;
   for(k=0;k<3;k++){u32 shift=k*8,a=(f->rgba[j]>>shift)&255,b=(color>>shift)&255;v|=((a*(256-mix)+b*mix)/256)<<shift;}
   V(f->p,0x20,void(*)(void*,u32,u32))(f->p,j,v);
  }
 }
}
static void fx_frames_restore(void){u32 i,j;for(i=0;i<fx_frame_count;i++){
 FxFrame*f=&fx_frames[i];if(fx_frame_valid(f->p))for(j=0;j<4;j++)V(f->p,0x20,void(*)(void*,u32,u32))(f->p,j,f->rgba[j]);
 }fx_frame_count=0;fx_teased=0;
}
static void fx_restore(void){
 if(fx_primitive&&rd(fx_primitive,0x113)&&rd(fx_parameters,0x18)){
  *(float*)(fx_parameters+0x10)=fx_scale[0];*(float*)(fx_parameters+0x14)=fx_scale[1];
 }
 fx_primitive=0;fx_parameters=0;fx_punch=0;fx_frames_restore();fx_owner_row=0;
}
/* Restore while the row's resources still exist, before native refresh/rebind
   or destruction can replace/free them. Other officers' rows are untouched. */
static void fx_release_row(void*row){if(fx_owner_row==row)fx_restore();}
static void fx_clear(void){fx_restore();fx_running=fx_revealed=0;fx_ticks=0;}
static void fx_begin(int reroll){fx_clear();fx_started=GetTickCount64();fx_running=1;active_keys=reveal_keys();native_sound(reroll?17u:0u);}
static void fx_reveal(void){u32 i;GSlot*x;
 if(!fx_running||fx_revealed||active_id>=MAX_OFFICERS||active_stage>=3)return;
 fx_revealed=1;x=&ui_gstate(active_id)->slots[active_stage];fx_punch=GetTickCount64();fx_frame_tint(1);
 /* Native UI sound samples only; acoustic balance requires a live listening pass. */
 native_sound(x->anomaly||x->transcend?8u:g_traits[x->result].color==3?6u:4u);
 for(i=0;i<visible_row_count;i++){
  u8*r=visible_rows[i],*o,*button,*p,*params;if(!rd(r,OFF(O_ROW_BUTTON)+8))continue;
  o=P(r,OFF(O_ROW_OFFICER));if(!rd(o,OFF(O_OFFICER_ID)+2)||S(o,OFF(O_OFFICER_ID))!=active_id)continue;
  button=P(r,OFF(O_ROW_BUTTON)+active_stage*8);if(!(p=caption_primitive(button)))break;
  params=V(p,0x80,u8*(*)(void*,int))(p,0);
  if(!rd(params,0x18))break;float a=*(float*)(params+0x10),b=*(float*)(params+0x14);
  if(!(a>0&&a<32&&b>0&&b<32))break;
  fx_owner_row=r;fx_primitive=p;fx_parameters=params;fx_scale[0]=a;fx_scale[1]=b;fx_punch=GetTickCount64();
  *(float*)(params+0x10)=a*1.045f;*(float*)(params+0x14)=b*1.045f;break;
 }
}
static void fx_tick(void){u64 now=GetTickCount64();
 if(fx_punch&&now-fx_punch>=100)fx_restore();
 if(!fx_running||fx_revealed||ui_mode!=2)return;
 /* No catch-up bursts after a pause; constant, limited native UI clicks. */
 u64 elapsed=now-fx_started;u32 step=(u32)(elapsed/150);
 if(elapsed>=950&&!fx_teased){fx_teased=1;fx_frame_tint(0);}
 if(elapsed<900&&step>fx_ticks&&step<6){fx_ticks=step;native_sound(0);}
}
