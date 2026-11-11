/* R2 controls are real native CCmnButton instances. They use the existing
 * compact button asset and font style, not a Win32 window/overlay renderer.
 * Ownership: register while merit layout is active; unregister on exit; release
 * with the exact resource context passed to the owning row's vslot 0x28. */
#define BASE_CONTROL_COUNT 9
#define CONTROL_COUNT 14
#define DETAIL_ROWS 20
static void notice_caption(u32,const u16*);
static void notice_reset(void);
static u8*controls[CONTROL_COUNT],*controls_parent;
static u8*detail_text[DETAIL_ROWS];
static u16 control_captions[CONTROL_COUNT][128],detail_captions[DETAIL_ROWS][128];
static int control_enabled[CONTROL_COUNT],control_shown[CONTROL_COUNT];
static u8 control_ready[CONTROL_COUNT],control_active[CONTROL_COUNT];
static int controls_registered,controls_failed,details_expanded,controls_teardown;
static u32 detail_scroll,detail_width,detail_rows;
static u64 controls_epoch;
static int miss_notice_dismiss;
static u32 control_generation=1,queued_action,queued_generation,mouse_capture;
static u16 controls_context_id=NO_TRAIT;static u8 controls_context_stage=255;
static u64 controls_context_seed,controls_context_epoch;
static int mouse_was_down,mouse_handled;
static u32 fallback_action,fallback_generation;static u64 fallback_at;
static void*control_callback_vtable[6];
static u16 control_hover_trait(void*row,u8*target,const int*point){
 if(row!=controls_owner_row||!controls_registered||!control_shown[2]||ui_mode==2||active_id>=MAX_OFFICERS)return 0;
 if(target&&merit_descendant(target,controls[2])&&merit_contains(controls[2],point))return selected_replacement[active_id];return 0;
}
static u8*control_copy(u8*src,u8*dst){if(!dst)dst=NCALL(N_ALLOC,u8*(*)(usize))(24);if(dst)mc_copy(dst,src,24);return dst;}
static void control_free(u8*self,u8 heap){if(heap)NCALL(N_FREE,void(*)(void*))(self);}
static void*control_type(void*self){(void)self;return NCALL(N_CALLBACK_TYPE,void*(*)(void))();}
static void*control_target(u8*self){return self+8;}
static void dispatch_control(u32 action){
 if(finalizing||thanks_active||ui_teardown||controls_teardown||picker_depth)return;
 if(exit_modal){if(exit_armed&&!exit_choice){if(action==ACTION_EXIT_YES)exit_choice=1;else if(action==ACTION_EXIT_NO)exit_choice=2;}return;}
 if(miss_modal){if(action==ACTION_NOTICE_CLOSE)miss_notice_dismiss=1;return;}
 if(action==ACTION_DETAILS){details_expanded=!details_expanded;detail_scroll=0;controls_refresh();return;}
 if(action==ACTION_UP){if(details_expanded&&detail_scroll)detail_scroll--;controls_refresh();return;}
 if(action==ACTION_DOWN){if(details_expanded&&detail_scroll+detail_rows<observation_line_count)detail_scroll++;controls_refresh();return;}
 if(action==ACTION_SKIP&&ui_mode==1){u8*o=officer(active_id);if(o)begin_slot(o,active_stage);paint_visible();return;}
 controls_action(action);
}
static int control_index(u32 action){switch(action){
 case ACTION_REROLL:return 0;case ACTION_REPLACE:return 2;case ACTION_SKIP:return 3;case ACTION_DETAILS:return 4;
 case ACTION_UP:return 6;case ACTION_DOWN:return 7;case ACTION_NOTICE_CLOSE:return 10;
 case ACTION_EXIT_YES:return 12;case ACTION_EXIT_NO:return 13;default:return -1;}}
static int control_focus(void){u32 pid=0;HANDLE w=GetForegroundWindow();return w&&GetWindowThreadProcessId(w,&pid)&&pid==GetCurrentProcessId();}
static int modal_button_hit(u32 index,u8*source){
 u8*app=NCALL(N_APP,void*(*)(void))(),*input=NCALL(N_INPUT,void*(*)(void))();
 u8*target=app?NCALL(N_TARGET,void*(*)(void*))(app):0;
 return index<CONTROL_COUNT&&controls[index]&&rd(input,0x3c)&&
  (source==controls[index]||merit_descendant(target,controls[index]))&&merit_contains(controls[index],(const int*)(input+0x34));
}
static void receive_control(u32 action,u8*source){int i=control_index(action);
 if(i<0||picker_depth||!controls_registered||ui_teardown||controls_teardown||!control_shown[i]||control_enabled[i]!=1||!control_active[i]||!control_focus())return;
 if(exit_modal||miss_modal){if(modal_button_hit((u32)i,source))dispatch_control(action);return;}
 if(!queued_action){queued_action=action;queued_generation=control_generation;}
}
static void control_invoke(u8*self,void*arg){(void)arg;
 if(ui_teardown||controls_teardown||picker_depth||!controls_registered||!controls_parent||P(self,8)!=controls_parent||D(self,20)!=control_generation)return;
 u32 action=D(self,16),kind=action&255u;
 if(kind==ACTION_EXIT_YES||kind==ACTION_EXIT_NO){if(!exit_modal||(action>>8)!=(exit_generation&0xffffffu))return;action=kind;}
 /* Some native dispatchers report release after our tick fallback. Ignore
    that delayed duplicate; a new physical press starts a fresh gesture. */
 u64 now=GetTickCount64();if(action==fallback_action&&fallback_generation==control_generation&&now>=fallback_at&&now-fallback_at<200&&!(reveal_keys()&1))return;
 mouse_handled=1;mouse_capture=0;receive_control(action,rd(arg,8)?P(arg,0):0);
}
static void controls_dispatch_tick(void){u32 action=queued_action,epoch=queued_generation;queued_action=0;
 if(action&&epoch==control_generation&&!ui_teardown&&!controls_teardown&&!exit_modal&&!miss_modal&&control_focus())dispatch_control(action);
}
static void bind_control(u8*w,u32 action){u8 cb[64];mc_zero(cb,sizeof(cb));
 control_callback_vtable[0]=control_copy;control_callback_vtable[1]=control_copy;control_callback_vtable[2]=control_invoke;
 control_callback_vtable[3]=control_type;control_callback_vtable[4]=control_free;control_callback_vtable[5]=control_target;
 if(action==ACTION_EXIT_YES||action==ACTION_EXIT_NO)action|=(exit_generation&0xffffffu)<<8;
 P(cb,0)=(u8*)control_callback_vtable;P(cb,8)=controls_parent;D(cb,16)=action;D(cb,20)=control_generation;P(cb,56)=cb;
 NCALL(N_BIND_ACTION,void(*)(void*,int,void*))(w,0x7ffffffe,cb);
}
static void control_register(u32 i,int on){if(i>=CONTROL_COUNT||!controls[i]||!control_ready[i]||control_active[i]==on)return;control_active[i]=(u8)on;V(controls[i],on?0x38:0x40,Void1)(controls[i]);}
static void control_show(u32 i,int on){if(i>=CONTROL_COUNT||!controls[i]||!control_ready[i])return;if(control_shown[i]!=on){set_visible(controls[i],on);control_shown[i]=on;}}
static void control_caption(u32 i,const u16*t,int enable){u32 j=0;u8*w;if(i>=CONTROL_COUNT||!(w=controls[i]))return;
 while(t[j]&&j<127&&control_captions[i][j]==t[j])j++;
 if(i==9||i==11){notice_caption(i,t);return;}
 if(control_captions[i][j]!=t[j]){mc_zero(control_captions[i],sizeof(control_captions[i]));for(j=0;t[j]&&j<127;j++)control_captions[i][j]=t[j];NCALL(N_BUTTON_TEXT,Text)(w,control_captions[i]);}
 if(control_enabled[i]!=enable){V(w,0xd0,Flag)(w,enable);control_enabled[i]=enable;}
}
static void control_font(u8*primitive,u8*template,u32 pixels,int x,int y,int width,int height,int align){u8*to,*from;
 if(!rd(primitive,0x113)||B(primitive,0x112)!=2)return;
 if(template&&(from=caption_primitive(template))!=0&&rd(P(from,0),0x88)&&rd(P(primitive,0),0x88)){
  from=V(from,0x80,u8*(*)(void*,int))(from,0);to=V(primitive,0x80,u8*(*)(void*,int))(primitive,0);
  if(rd(from,0x78)&&rd(to,0x78))mc_copy(to,from,0x78);
 }
 if(pixels)NCALL(N_TEXT_FONT,void(*)(void*,int))(primitive,(int)pixels);
 NCALL(N_TEXT_ALIGN,void(*)(void*,int))(primitive,align);
 NCALL(N_PRIM_SIZE,void(*)(void*,int,int))(primitive,width,height);
 NCALL(N_PRIM_POS,void(*)(void*,int,int))(primitive,x,y);
}
static u8*create_control_parent(u32 index,int x,int y,int width,int height,int style,u32 action,u8*template,u8*parent){u8*b,*prim,*font_style=0,palette[16];
 b=NCALL(N_ALLOC,u8*(*)(usize))(0x198);if(!b)return 0;NCALL(N_BUTTON_CTOR,void*(*)(void*))(b);controls[index]=b;
 NCALL(N_BUTTON_INIT,int(*)(void*,int,int,int,int,void*,int))(b,x,y,width,height,parent,style);
 if(!rd(b,0x198)||!P(b,0x138)||!P(b,0x188))return 0;
 set_visible(b,0);V(b,0x70,Flag)(b,6);
 if(action)bind_control(b,action);else D(b,0x40)|=8;
 /* Native alignment 4 is horizontal/vertical center. Equal insets keep the
    one-line calculation label at the exact center of the button rectangle. */
 prim=caption_primitive(b);
 /* The warning panel keeps its own native skin's readable text palette,
    while sharing Selection's typeface/metrics. Buttons keep Selection colors. */
 if(index==11&&prim){font_style=V(prim,0x80,u8*(*)(void*,int))(prim,0);if(rd(font_style,16))mc_copy(palette,font_style,16);else font_style=0;}
 control_font(prim,template,0,4,4,width-8,height-8,4);if(font_style)mc_copy(font_style,palette,16);
 control_ready[index]=1;
 return b;
}
static u8*create_control(u32 index,int x,int y,int width,int height,int style,u32 action,u8*template){return create_control_parent(index,x,y,width,height,style,action,template,controls_parent);}
static void controls_hide(void){u32 i;if(!controls_registered)return;
 /* Invalidate BEFORE native unregister: it may synchronously deliver an old
    callback or refresh a row. Neither may revive this toolbar mid-teardown. */
 controls_teardown++;controls_registered=0;ui_epoch++;control_generation++;queued_action=mouse_capture=0;mouse_was_down=mouse_handled=0;fallback_action=0;
 for(i=CONTROL_COUNT;i-->0;)if(controls[i]){control_show(i,0);control_register(i,0);}
 details_expanded=0;controls_teardown--;
}
static void controls_destroy(void*resources){u32 i;if(controls_teardown)return;report("MeritTraits R9: CONTROLS_RELEASE_BEGIN\r\n");controls_teardown++;controls_hide();ui_epoch++;
 if(exit_modal&&rd(active_state,OFF(O_STATE_LAYOUT)+8)){u8*layout=P(active_state,OFF(O_STATE_LAYOUT));if(rd(layout,OFF(O_MAIN_STATE)+4)&&(D(layout,OFF(O_MAIN_STATE))==1||D(layout,OFF(O_MAIN_STATE))==2))D(layout,OFF(O_MAIN_STATE))=0;}
 miss_modal=miss_notice_dismiss=0;exit_modal=exit_choice=exit_armed=exit_approved=exit_resume_miss=0;
 for(i=CONTROL_COUNT;i-->0;)if(controls[i]){u8*w=controls[i];controls[i]=0;control_ready[i]=control_active[i]=0;V(w,0x28,void(*)(void*,void*))(w,resources);V(w,0,Flag)(w,1);}
 controls_owner_row=0;controls_parent=0;controls_failed=0;details_expanded=0;detail_scroll=0;mc_zero(detail_text,sizeof(detail_text));notice_reset();controls_teardown--;report("MeritTraits R9: CONTROLS_RELEASE_END\r\n");
}
static int controls_ensure(u8*row){u8*parent,*template;int width,height,rowx,left,gutter,i,top=26,button_height=48;u32 actions[CONTROL_COUNT]={ACTION_REROLL,ACTION_ACCEPT,ACTION_REPLACE,ACTION_SKIP,ACTION_DETAILS,0,ACTION_UP,ACTION_DOWN,ACTION_CANCEL,ACTION_MODAL_BLOCK,ACTION_NOTICE_CLOSE,ACTION_MODAL_BLOCK,ACTION_EXIT_YES,ACTION_EXIT_NO};
 if(controls_teardown||ui_teardown||picker_depth)return 0;if(!native_ui_ready)return 1;if(!rd(row,OFF(O_ROW_BUTTON)+24))return 0;parent=P(row,8);template=P(row,OFF(O_ROW_BUTTON));
 if(!rd(parent,0x148)||!rd(template,0x198))return 0;
 rowx=(int)D(row,0x20);
 for(i=0;i<4&&rowx<144;i++){if(!rd(parent,0x44))return 0;rowx+=(int)D(parent,0x20);parent=P(parent,8);}
 if(!rd(parent,0x148))return 0;
 if(controls_parent&&controls_parent!=parent)return 0;
 if(controls_failed)return 0;
 if(controls[0]){if(!controls_registered){control_generation++;for(i=0;i<CONTROL_COUNT;i++)if(controls[i]&&control_ready[i]){if(i!=9&&i!=11)bind_control(controls[i],actions[i]);if(i<BASE_CONTROL_COUNT)control_register(i,1);}controls_registered=1;controls_epoch=ui_epoch;completed_state=0;}return 1;}
 width=(int)D(parent,0x28);height=(int)D(parent,0x2c);
 /* The left gutter must exist in this exact native layout. Never overlay a
  * portrait/tree if a different resource geometry is encountered. */
 gutter=rowx-32;if(gutter>168)gutter=168;
 if(width<960||width>4096||height<500||height>2160||gutter<112){controls_failed=1;report("MeritTraits R2: native merit geometry outside verified empty margins; toolbar not installed.");return 0;}
 controls_parent=parent;controls_owner_row=row;controls_epoch=ui_epoch;
 /* Reuse the native top Selection button's rectangle/font when its verified
    resource collection exposes it. Do not resize or move any native widget. */
 u8*children=P(parent,0x138);u32 count=D(parent,0x140);
 if(count&&count<=256&&rd(children,count*8))for(u32 k=0;k<count;k++){
  u8*w=P(children,k*8);if(!rd(w,0x198)||P(w,8)!=parent||!caption_primitive(w))continue;
  int x=(int)D(w,0x20),y=(int)D(w,0x24),w_=(int)D(w,0x28),h=(int)D(w,0x2c);
  if(x>=rowx-24&&x<=rowx+24&&y>=8&&y<=48&&w_>=140&&w_<=240&&h>=32&&h<=64){template=w;top=y;button_height=h;break;}
 }
 mc_zero(control_captions,sizeof(control_captions));mc_zero(detail_captions,sizeof(detail_captions));
 for(i=0;i<CONTROL_COUNT;i++){control_enabled[i]=-1;control_shown[i]=0;}
 left=width-666;
 if(!create_control(0,left,top,192,button_height,3,ACTION_REROLL,template)||
    !create_control(2,left+200,top,240,button_height,3,ACTION_REPLACE,template)||
    !create_control(3,left+448,top,156,button_height,3,ACTION_SKIP,template)||
    !create_control(4,16,72,gutter,button_height,3,ACTION_DETAILS,template))goto failed;
 detail_width=(u32)gutter-12;int panel_height=height-270;if(panel_height>440)panel_height=440;
 detail_rows=(u32)(panel_height-12)/20;if(detail_rows>DETAIL_ROWS)detail_rows=DETAIL_ROWS;
 if(!create_control(5,16,130,gutter,panel_height,7,0,template)||
    !create_control(6,16,138+panel_height,gutter/2-4,40,3,ACTION_UP,template)||
    !create_control(7,20+gutter/2,138+panel_height,gutter/2-4,40,3,ACTION_DOWN,template))goto failed;
 for(i=0;i<(int)detail_rows;i++){
  u8*ctx=P(controls[5],0x98);if(!rd(ctx,0x18)||(ctx=P(ctx,0x10))==0||!rd(ctx+0x18,8))goto failed;
  ctx+=0x18;usize n=V(ctx,0x18,usize(*)(void*))(ctx);if(n>4096)goto failed;V(ctx,0x60,void(*)(void*,usize))(ctx,n+1);
  detail_text[i]=NCALL(N_DYNAMIC_TEXT,u8*(*)(void*,int,int,int,int,const u16*))(controls[5],0,0,(int)detail_width,20,L"");
  if(!detail_text[i])goto failed;control_font(detail_text[i],template,16,6,6+i*20,(int)detail_width,20,3);
 }
 for(i=0;i<BASE_CONTROL_COUNT;i++)control_register(i,1);controls_registered=1;
 controls_refresh();return 1;
failed: controls_failed=1;report("MeritTraits R2: native widget construction failed; no gacha commit was attempted.");return 0;
}
static void controls_refresh(void){u32 i;W w={{0},0};if(!controls_registered||controls_teardown||picker_depth)return;
 u64 seed=active_id<MAX_OFFICERS&&active_stage<3?ui_gstate(active_id)->slots[active_stage].seed:0;
 if(controls_context_id!=active_id||controls_context_stage!=active_stage||controls_context_seed!=seed||controls_context_epoch!=ui_epoch){
  controls_context_id=active_id;controls_context_stage=active_stage;controls_context_seed=seed;controls_context_epoch=ui_epoch;
  control_generation++;queued_action=mouse_capture=0;
  u32 actions[9]={ACTION_REROLL,0,ACTION_REPLACE,ACTION_SKIP,ACTION_DETAILS,0,ACTION_UP,ACTION_DOWN,0};
  for(i=0;i<9;i++)if(controls[i]&&control_ready[i]&&actions[i])bind_control(controls[i],actions[i]);
  if(controls[10]&&control_ready[10])bind_control(controls[10],ACTION_NOTICE_CLOSE);
 }
 int active=active_id<MAX_OFFICERS&&active_stage<3;GState*s=active?ui_gstate(active_id):0;GSlot*x=active?&s->slots[active_stage]:0;
 GState*shown=s?s:inspected_id<MAX_OFFICERS?ui_gstate(inspected_id):0;
 wt(&w,L"재추첨 ");if(shown)wi(&w,5-shown->rerolls_used);else wt(&w,L"-");wt(&w,L"/5");control_caption(0,w.text,ui_mode==3&&s&&active_reroll_allowed()&&s->rerolls_used<5&&!finalizing&&!miss_modal&&!exit_modal);
 control_show(0,1);control_show(4,1);
 control_caption(4,L"계산 근거",!miss_modal&&!exit_modal);
 int replace=active&&(ui_mode==1||(ui_mode==3&&x->full&&x->outcome!=G_MISS));control_show(2,replace);
 if(replace){w.n=0;w.text[0]=0;wt(&w,ui_mode==1?L"승계: ":L"교체: ");
  u16 chosen=selected_replacement[active_id];wt(&w,chosen==NO_TRAIT?L"획득 포기":trait_name(chosen));
  control_caption(2,w.text,!miss_modal&&!exit_modal&&(ui_mode==1||(ui_mode==3&&x->star)));
 }
 control_show(3,ui_mode==1||ui_mode==2);control_caption(3,ui_mode==1?L"승계 시작":L"건너뛰기",!miss_modal&&!exit_modal&&(ui_mode==1||(ui_mode==2&&x&&x->outcome!=G_MISS)));
 control_show(5,details_expanded);control_show(6,details_expanded);control_show(7,details_expanded);
 if(!details_expanded)return;
 build_observation(inspected_id,inspected_stage,detail_width);
 if(detail_scroll>=observation_line_count)detail_scroll=0;
 control_caption(6,L"▲",!miss_modal&&!exit_modal&&detail_scroll>0);control_caption(7,L"▼",!miss_modal&&!exit_modal&&detail_scroll+detail_rows<observation_line_count);
 for(i=0;i<detail_rows;i++)if(detail_text[i]){const u16*t=detail_scroll+i<observation_line_count?observation_lines[detail_scroll+i]:L"";u32 n=wlen(t);
  if(n>=128)n=127;if(!mc_equal(detail_captions[i],t,(n+1)*2)){mc_zero(detail_captions[i],sizeof(detail_captions[i]));mc_copy(detail_captions[i],t,n*2);NCALL(N_PRIM_TEXT,Text)(detail_text[i],detail_captions[i]);}
 }
}
#include "native_notice.inc"
/* Both notices share the verified native root-window implementation. */
static void miss_notice_hide(void){notice_hide(9);miss_modal=miss_notice_dismiss=0;}
static void miss_notice_tick(void){Observation*o;
 if(exit_modal)return;
 if(miss_modal){if(miss_notice_dismiss||!notice_healthy(9))miss_notice_hide();return;}
 if(!native_ui_ready||!controls_registered||controls_epoch!=ui_epoch||ui_mode!=3||active_id>=MAX_OFFICERS||active_stage>=3||finalizing||thanks_active)return;
 o=&observations[active_id][active_stage];GSlot*x=&ui_gstate(active_id)->slots[active_stage];
 if(!o->valid||o->notice_shown||x->outcome!=G_MISS||o->result.seed!=x->seed||o->result.roll_id!=x->roll_id)return;
 if(!notice_ensure(9)){o->notice_shown=1;report("MeritTraits R9: MISS_NOTICE_UNAVAILABLE; result retained.\r\n");return;}
 control_caption(9,x->full?L"꽝입니다. 재추첨할 수 있습니다.\n기존 개성은 지워지지 않습니다.\n결정으로 확정하면 다음 추첨에\n열정 버프를 받습니다.\n확인은 설명만 닫습니다.":L"꽝입니다. 재추첨할 수 있습니다.\n결정으로 확정하면 빈 개성칸 하나가\nX로 소모됩니다. 다음 추첨에\n열정 버프를 받습니다.\n사용한 재추첨권은 돌아오지 않습니다.",1);
 control_caption(10,L"확인",1);
 if(!notice_present(9)){notice_hide(9);o->notice_shown=1;report("MeritTraits R9: MISS_NOTICE_PARENT_HIDDEN.\r\n");return;}
 o->notice_shown=1;miss_modal=1;miss_notice_dismiss=0;hide_merit_hovers(0);paint_visible();
}
static void exit_notice_hide(void){
 ui_teardown++;controls_teardown++;
 exit_modal=exit_choice=exit_armed=exit_back_requested=exit_choice_wait=0;exit_resume_miss=0;
 notice_hide(11);controls_teardown--;ui_teardown--;
}
static void exit_notice_resume(void){int resume_miss=exit_resume_miss;u64 paused=exit_pause_started,now=GetTickCount64();
 exit_notice_hide();cancel_requested=exit_approved=0;
 if(ui_mode==2&&now>=paused)animation_start+=now-paused;
 if(resume_miss&&controls[9]&&controls[10]&&notice_present(9)){miss_modal=1;miss_notice_dismiss=0;}
}
static int exit_notice_healthy(void){return notice_healthy(11);}
static int exit_notice_show(void){
 if(exit_modal)return 1;
 if(!native_ui_ready||!controls_registered||controls_epoch!=ui_epoch||!rd(controls_parent,0x148)||!control_ready[0])return 0;
 ui_teardown++;controls_teardown++;
 if(!notice_ensure(11))goto unavailable;
 queued_action=mouse_capture=0;exit_generation++;if(!(exit_generation&0xffffffu))exit_generation++;
 bind_control(controls[12],ACTION_EXIT_YES);bind_control(controls[13],ACTION_EXIT_NO);
 if(warning_kind==2){W w={{0},0};ReplacementNotice*n=&replacements[replacement_index];
  wperson(&w,n->id);wt(&w,L"의 개성을 교체합니다.\n기존: ");wt(&w,trait_name(n->old_trait));
  wt(&w,L"\n획득: ");wt(&w,trait_name(n->new_trait));wt(&w,L"\n진행하시겠습니까? (");wi(&w,replacement_index+1);wt(&w,L"/");wi(&w,replacement_count);wt(&w,L")");control_caption(11,w.text,1);
 }else control_caption(11,L"뒤로 나가면 아직 확정하지 않은\n개성 추첨 결과가 초기화됩니다.\n사용한 재추첨권은 복구되지 않습니다.\n진행하시겠습니까?",1);
 control_caption(12,L"예",1);control_caption(13,L"아니오",1);
 if(!notice_present(11))goto unavailable;
 exit_resume_miss=miss_modal;miss_notice_hide();exit_pause_started=GetTickCount64();exit_modal=1;exit_choice=exit_armed=exit_back_requested=exit_choice_wait=0;
 hide_merit_hovers(0);controls_teardown--;ui_teardown--;paint_visible();
 report(warning_kind==2?"MeritTraits R9: REPLACEMENT_NOTICE_OPEN; independent native window.\r\n":"MeritTraits R9: BACK_NOTICE_OPEN; independent native window.\r\n");return 1;
unavailable:notice_hide(11);controls_teardown--;ui_teardown--;report("MeritTraits R9: NOTICE_BUILD_OR_POSITION_FAILED; no consent, no input lock.\r\n");return 0;
}
static void controls_suspend(void){
 controls_teardown++;control_generation++;queued_action=mouse_capture=0;mouse_was_down=mouse_handled=0;
 for(u32 i=CONTROL_COUNT;i-->0;){control_show(i,0);control_register(i,0);}controls_registered=0;
 hide_merit_hovers(0);controls_teardown--;
}
/* Local fallback for native event routing failures. Uses the GAME's logical
   cursor coordinates and target ownership, never Windows pixel coordinates.
   One complete press/release on the same visible enabled widget is required;
   the native callback wins and suppresses fallback for the entire gesture. */
static void controls_input_tick(void*layout){
 (void)layout;
 if(!native_ui_ready||!controls_registered||ui_teardown||controls_teardown||picker_depth||!control_focus()){mouse_capture=queued_action=0;mouse_was_down=mouse_handled=0;return;}
 int down=(reveal_keys()&1)!=0;u8*app=NCALL(N_APP,void*(*)(void))(),*input=NCALL(N_INPUT,void*(*)(void))();
 u8*target=app?NCALL(N_TARGET,void*(*)(void*))(app):0;u32 hit=0;
 if(target&&rd(input,0x3c)){
  const int*xy=(const int*)(input+0x34);
  for(u32 i=0;i<CONTROL_COUNT;i++)if(control_shown[i]&&control_enabled[i]==1&&control_active[i]&&i!=5&&i!=9&&i!=11&&controls[i]&&merit_contains(controls[i],xy)){
   if(exit_modal&&i!=12&&i!=13)continue;if(miss_modal&&!exit_modal&&i!=10)continue;
   if(merit_descendant(target,controls[i])||(!exit_modal&&!miss_modal&&target==controls_parent)){hit=i+1;break;}
  }
  /* The native bottom group contains Decision and Back. Never infer a
     command from its rectangle: leave those events to native dispatch. */
 }
 if(down&&!mouse_was_down){fallback_action=0;if(!mouse_handled)mouse_capture=hit;}
 if(!down&&mouse_was_down){u32 captured=mouse_capture;mouse_capture=0;
  if(captured&&captured==hit&&!mouse_handled){
   {u32 actions[14]={ACTION_REROLL,0,ACTION_REPLACE,ACTION_SKIP,ACTION_DETAILS,0,ACTION_UP,ACTION_DOWN,0,0,ACTION_NOTICE_CLOSE,0,ACTION_EXIT_YES,ACTION_EXIT_NO};fallback_action=actions[hit-1];fallback_generation=control_generation;fallback_at=GetTickCount64();receive_control(fallback_action,0);}
  }
  mouse_handled=0;
 }else if(!down)mouse_handled=0;
 mouse_was_down=down;
}
