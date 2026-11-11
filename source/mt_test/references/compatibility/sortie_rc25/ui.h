/* Native resources only: cmn_btn_hanyou, cmn_btn_hanyou_defo, cmn_btn_list, cmn_face_type_04,
 * cmn_face_frame, cmn_ani_chara_3parts_cn, native formation select buttons,
 * trait labels and balloon.
 * Positions are in the captured 1920x1080 logical canvas, not screenshot pixels.
 */
enum {A_OPEN=1,A_CLOSE=2,A_SAVE=3,A_LOAD=4,A_OVERWRITE=5,A_CLEAR=6,A_MODE=7,
      A_RESET=8,A_RESET_CANCEL=9,A_SELECT_ALL=10,A_DELETE=11,
      A_DELETE_CONFIRM=12,A_DELETE_CANCEL=13,A_SAVE_CONFIRM=14,A_SAVE_CANCEL=15,
      A_ROW=100,A_GROUP=200,A_GROUP_RENAME=220,A_RENAMED=300};
/* The Session ABI remains unchanged. Its old rename fields are reused only
 * for the native group-name modal; preset record names remain unchanged. */
typedef struct {U8 *w;I32 active;} Owned;
typedef struct {U8 *button,*face,*formation,*tactics[3];I32 reason;} Row;
typedef struct {
    U8 *state,*entry,*panel,*tip,*detail_name,*detail_face,*detail_formation,*detail_troops;
    U8 *detail_tactics[3],*labels[9],*status,*mode,*load,*save,*overwrite,*rename;
    I32 trait_ids[9];
    Row rows[P_SLOTS];Owned owned[128];I32 count,selected,mode_value,want_open,shown,failed;
    I32 pending,rename_slot,rename_waiting,rename_ok;U32 generation;
    U16 renamed[P_NAME];unsigned long long last_refresh;
    void *resources;
} Session;
static Session ui;
#include "group_native_api.h"
static G_Native group_native;
static U8 *group_buttons[G_COUNT];
static U32 group_rename_epoch;
static char *group_rename_result;
static I32 group_right_down,group_right_capture=-1;
static void *group_vtable[0x208/8+1];
static I32 group_vtable_ready;
static void group_rename_callback(U8 *,U8 **);
static I32 hit(U8 *,const I32 *);
static void refresh_groups(void);
static const char *ui_build_stage;
static I32 ui_build_style,ui_build_width,ui_build_height;
static void trace_ui(const char *stage,I32 style,I32 width,I32 height);
static void trace_input(I32 action);
static void log_layout(const char *status,U8 *s,I32 code);
static I32 panel_ready;
/* Cache only our owned widgets, cleared at each screen lifetime. The game
 * still supplies every caption, portrait, formation and native trait value. */
typedef struct {I32 valid;U16 text[160];U8 *person,*formation;I32 portrait;
    I32 image_w,image_h;float uv[4][2];} DisplayCache;
static DisplayCache display_cache[128];
/* Only the right visual owns this presentation cache. The game's component
 * owns/releases its texture handles and its animation children. */
static struct {U8 *fallback,*parts[3],*special_image,*person;I32 portrait,ready,loaded;} detail_visual;
/* Native text children are released by their owning row. The existing slot
 * structure is retained; RC11 expands only its bounded array to 11 entries. */
typedef struct {U8 *r;U16 text[160];U32 colors[4];I32 valid;} TextLine;
static TextLine group_title;
/* RC25: one immutable batch snapshot; never a field in the sidecar schema. */
typedef struct {I32 count;P_Selection units[9];U16 names[9][P_NAME];} SaveSource;
typedef struct {
    I32 pending,busy,group,save_count,empty[P_SLOTS];
    U32 epoch,screen_generation;U8 *state;
    SaveSource source;P_Book before,candidate;U16 group_name[P_NAME];
} SaveBatch;
static SaveBatch save_batch;
static U32 save_epoch;
static U8 *save_warning,*save_yes,*save_no;
static TextLine save_warning_lines[12];
static U8 group_empty_font[0x78];
static I32 group_empty_font_ready;
static void cancel_save(void);
static void present_save_confirmation(void);
static I32 save_snapshot_matches(void);
static void save_confirm(I32);

static TextLine row_names[P_SLOTS],row_troops[P_SLOTS],notice_first,notice_second;
static TextLine row_checks[P_SLOTS],row_checkmarks[P_SLOTS],row_detailmarks[P_SLOTS];
enum { ROW_X=12,ROW_Y=12,ROW_W=578,ROW_H=63,ROW_STEP=64,
       NUMBER_X=6,CHECK_FONT=36,CHECK_X=2,CHECK_Y=20,CHECK_W=36,CHECK_H=40,CHECK_HIT_W=40,FACE_X=40,FACE_SIZE=62,NAME_X=104,NAME_W=106,FORMATION_X=218,
       TACTIC_X=282,TACTIC_STEP=96,TACTIC_W=92,
       DETAIL_X=604,DETAIL_W=336,DETAIL_FACE_Y=14,DETAIL_FACE_H=266,
       DETAIL_NAME_Y=286,DETAIL_NAME_H=40,DETAIL_INFO_Y=332,DETAIL_INFO_H=52,
       DETAIL_TROOPS_X=670,DETAIL_TROOPS_W=270,
       DETAIL_TACTIC_Y=390,DETAIL_TRAIT_Y=438,
       GRID_W=108,GRID_STEP=114,GRID_H=42,GRID_Y_STEP=48,GRID_FONT=24, GROUP_TITLE_Y=578,GROUP_Y=600,
       MESSAGE_Y=742,TOOLBAR_Y=724,TOOLBAR_H=48,TOOLBAR_GAP=6,
       TOOLBAR_W=(ROW_W-3*TOOLBAR_GAP)/4,TOOLBAR_STEP=TOOLBAR_W+TOOLBAR_GAP };
/* RC18: only preset tactic widgets own this presentation adapter. Native
 * event handlers and callback bindings stay intact. No shared game vtable
 * or material is modified. One copy includes MSVC RTTI and all 65 methods. */
enum { TACTIC_VIEW_CAPACITY=P_SLOTS*3+3, TACTIC_VTABLE_BYTES=0x208 };
typedef struct { U8 *w;I32 id,available; } TacticView;
static TacticView tactic_views[TACTIC_VIEW_CAPACITY];
static I32 tactic_view_count,tactic_vtable_ready;
static void *tactic_vtable[TACTIC_VTABLE_BYTES/8+1];
static U8 *tactic_tip,*last_tactic_target;
static I32 tactic_tip_ready,last_tactic_id=-1;
/* RC21: a separate instance of the ORIGINAL CCmnTacticsBalloon and its
 * ORIGINAL hover manager. The army tab's instance/list/layer are never borrowed.
 * Owned by the existing panel lifetime, exactly like the native trait balloon. */
static U8 *tactic_tip_manager;
static U8 tactic_manager_storage[0x80];
static void *tactic_hover_vtable[6];
static U8 *tactic_hover_alias;
static I32 tactic_tip_using,tactic_tip_diag;
static void hide_tactic_tooltip(void);
static I32 inputs_valid;
static I32 status_two_lines;
/* RC6 controls are presentation/session state, never fields in saved JSON. */
static U8 *panel_backdrop,*detail_outline,*clear_button,*selection_summary,*delete_button;
/* RC17: confirmation uses two ordinary children of the EXISTING panel.
 * No separate balloon root, registration, popup positioning or modal lock. */
static U8 *notice_strip,*delete_yes,*delete_no;
static P_Book delete_snapshot;
static U32 delete_epoch,delete_screen_generation,delete_unavailable_mask;
static U8 hud_font_style[0x78];
static I32 hud_font_ready;
/* RC22: read-only copy of our existing, idle Select All caption style.
 * Never copy a rendered glyph texture or modify a shared font asset. */
static U8 selection_font_style[0x78];
static I32 selection_font_ready;
static I32 reset_slot=-1,batch_mode;
/* RC15: selection is management state, independent of city/eligibility.
 * The two deployment partitions are derived by refresh_selection only. */
static U32 selected_mask,batch_mask,unavailable_checks,delete_pending_mask;
/* RC7 transient selection only. Retain batch_mask/batch_mode as the existing
 * batch adapter's inputs. Neither these fields nor the gesture enter JSON. */
static I32 lastSelectedSlotIndex=-1,isDraggingSelection;
static U32 dragVisitedSlots;
static struct {I32 left_down,pressed_slot,start[2],previous[2],shift,detail,
                   suppress_release,have_previous,wait_for_release;} selection_gesture;
static struct {
    short (*GetAsyncKeyState)(I32);
    HANDLE (*GetForegroundWindow)(void);
    U32 (*GetWindowThreadProcessId)(HANDLE,U32 *);
    U32 (*GetCurrentProcessId)(void);
} selection_api;
static I32 selection_api_ready;
/* RC16: native callbacks are queued, never overwritten by a later row event.
 * The queue and gesture ownership are transient UI state, not saved data. */
typedef struct { I32 action,detail,shift; } PresetAction;
enum { ACTION_QUEUE_CAPACITY=32 };
static PresetAction action_queue[ACTION_QUEUE_CAPACITY];
static I32 action_head,action_count,input_row_consumed,input_drag_consumed;
static struct { U32 callbacks,dispatched,row_duplicates,drag_blocks,overflows; I32 last_action; } input_diagnostics;
static void clear_action_queue(void) {
    action_head=action_count=0;ui.pending=0;
}
static I32 queue_action(I32 action,I32 detail,I32 shift) {
    I32 at;
    if(action==A_DELETE_CONFIRM || action==A_DELETE_CANCEL) {
        if(!delete_pending_mask)return 0;
        /* These actions do not use row detail. Stamp the confirmation epoch
         * at callback receipt so an old queued confirmation cannot delete a
         * later selection. No persistent file format changes. */
        detail=(I32)delete_epoch;
    }
    if(action==A_SAVE_CONFIRM || action==A_SAVE_CANCEL) {
        if(!save_batch.pending)return 0;
        detail=(I32)save_batch.epoch;
    }
    if(action_count>=ACTION_QUEUE_CAPACITY){++input_diagnostics.overflows;return 0;}
    at=(action_head+action_count)%ACTION_QUEUE_CAPACITY;
    action_queue[at].action=action;action_queue[at].detail=detail;action_queue[at].shift=shift;
    ++action_count;return 1;
}
static struct {I32 count,index,troops,gold,limit;U8 *city;
               P_Selection current,records[9];} ui_inputs;
static const U16 *last_tip_text;
static U8 *last_tip_target;
static U32 generation;
static void *callback_vt[6];
static void close_panel(void);
static void refresh_panel(I32 full);
static void cancel_reset(void);
static void cancel_selection_gesture(void);
static void selection_input_init(void);
static I32 update_selection_input(void);
static void selection_click(I32 slot,I32 shift,I32 detail);
static I32 selection_row_at(const I32 *xy);
static I32 selection_detail_at(I32 slot,const I32 *xy);
static void refresh_selection(void);
static void cancel_delete(void);
static I32 ensure_delete_dialog(void);
static void present_delete_confirmation(void);
static void delete_notice(void);

/* std::function<void(NativeEvent)> small closure, cloned by 0x1a3300/463340. */
static NI U8 *callback_copy(U8 *src,U8 *dst) {
    if(!dst)dst=alloc(24);
    if(!dst)return 0;
    /* The native std::function owns its +0x38 manager pointer. Clone only
     * the 24-byte callable, just like the game's own small lambda clones. */
    copy(dst,src,24);return dst;
}
/* Native callers define only DL for this C++ bool (setne dl). */
static NI void callback_destroy(U8 *self,U8 heap) { if(heap)release(self); }
static NI U8 *callback_target(U8 *self) { return self+8; }
static NI U8 *callback_type(U8 *self) { (void)self;return FN(0x801080,U8 *(*)(void))(); }
static NI void callback_invoke(U8 *self,U8 **argument) {
    I32 action=INT(self,OF(126)),detail=1,shift=0,slot;U8 *input;(void)argument;
    if(action==A_RENAMED){group_rename_callback(self,argument);return;}
    if(PTR(self,OF(127))!=ui.state || (U32)INT(self,OF(128))!=ui.generation || !ui.state)return;
    if(!((action>=A_OPEN && action<=A_CLEAR) ||
         (action>=A_SELECT_ALL && action<=A_SAVE_CANCEL) ||
         (action>=A_ROW && action<A_ROW+P_SLOTS) ||
         (action>=A_GROUP && action<A_GROUP+G_COUNT)))return;
    if(action!=A_OPEN && action!=A_CLOSE && !ui.shown)return;
    /* RC17: a pending confirmation is NOT a global input lock. Other
     * callbacks must reach dispatch to cancel the prompt and act normally. */
    if(action>=A_ROW && action<A_ROW+P_SLOTS) {
        /* Capture the clicked region now: a later UI tick may already see
         * the cursor over the next button. Do not alter selection here. */
        slot=action-A_ROW;input=FN(0x110d0,U8 *(*)(void))();
        if(input && selection_row_at((const I32 *)(input+OF(135)))==slot)
            detail=selection_detail_at(slot,(const I32 *)(input+OF(136)));
        if(selection_api_ready)shift=selection_api.GetAsyncKeyState(0x10)<0;
        if(detail==2)shift=0;
    }
    ++input_diagnostics.callbacks;queue_action(action,detail,shift);
}
static void make_callback(U8 *out,I32 action) {
    zero(out,64);PTR(out,0)=(U8 *)callback_vt;PTR(out,OF(129))=ui.state;
    INT(out,OF(130))=action;INT(out,OF(131))=(I32)ui.generation;PTR(out,OF(132))=out;
}
static void bind_action(U8 *w,I32 action) {
    U8 cb[64];make_callback(cb,action);
    FN(0x1a3300,void (*)(void *,I32,void *))(w,0x7ffffffe,cb);
}
static U8 *own(U8 *w) {
    if(!w || ui.count>=128){ui.failed=1;return 0;}
    ui.owned[ui.count++].w=w;return w;
}
static DisplayCache *display_for(U8 *w) {
    I32 i;for(i=0;i<ui.count;++i)if(ui.owned[i].w==w)return &display_cache[i];
    return 0;
}
static void visible(U8 *w,I32 yes) {
    if(w && !!(INT(w,OF(133))&1)!=!!yes)VFN(w,OF(134),FLAG)(w,yes);
}
static void caption(U8 *w,const U16 *t) {
    DisplayCache *c;I32 n=0,i,same;
    if(!w)return;
    if(!t)t=L"";
    c=display_for(w);
    while(n<160 && t[n])++n;
    if(c && n<160) {
        same=c->valid;
        for(i=0;i<=n;++i)if(c->text[i]!=t[i])same=0;
        if(same)return;
        copy(c->text,t,(SIZE)(n+1)*2);c->valid=1;
    } else if(c)c->valid=0;
    FN(0x85c250,TEXT)(w,t);
}
static void layer(U8 *w,I32 n) { if(w)VFN(w,OF(137),FLAG)(w,n); }
static void enable(U8 *w,I32 yes) {
    if(!w || !!(INT(w,OF(138))&4)==!!yes)return;
    VFN(w,OF(139),FLAG)(w,yes);
    VFN(w,OF(140),FLAG)(w,FN(0x84dca0,I1)(w));
}
static void selected(U8 *w,I32 yes) {
    if(!w || !!(INT(w,OF(141))&0x100)==!!yes)return;
    INT(w,OF(142))&=~0x100;if(yes)INT(w,OF(143))|=0x100;
    VFN(w,OF(144),FLAG)(w,FN(0x84dca0,I1)(w));
}
static NI TacticView *tactic_view_for(U8 *w) {
    I32 i;for(i=0;i<tactic_view_count;++i)if(tactic_views[i].w==w)return &tactic_views[i];
    return 0;
}
static NI void tactic_display_state(U8 *w,I32 interaction_state) {
    TacticView *view=tactic_view_for(w);I32 state;
    if(!view) {
        /* Fail closed to the original method, never recursively dispatch. */
        FN(0x84c5b0,FLAG)(w,interaction_state);return;
    }
    /* Native CCmnButton::ChangeState 84C5B0: 2=orange selected appearance,
     * 4=disabled. This state is derived ONLY from tactic content/eligibility,
     * never flags 0x80/0x100 written by native pointer enter/leave/press.
     * The native method itself caches its applied state; unchanged requests
     * perform no material/animation update. Not a per-frame recolor loop. */
    state=view->available?2:4;
    FN(0x84c5b0,FLAG)(w,state);
}
static NI I32 attach_tactic_display(U8 *w) {
    TacticView *v;
    if(!w || PTR(w,0)!=GA(0x14680a0) || tactic_view_count>=TACTIC_VIEW_CAPACITY)return 0;
    if(!tactic_vtable_ready) {
        copy(tactic_vtable,GA(0x14680a0)-8,TACTIC_VTABLE_BYTES+8);
        tactic_vtable[1+0x1c0/8]=(void *)tactic_display_state;
        tactic_vtable_ready=1;
    }
    v=&tactic_views[tactic_view_count++];v->w=w;v->id=0;v->available=0;
    PTR(w,0)=(U8 *)(tactic_vtable+1);
    return 1;
}
static void resource_scale(U8 *r,float sx,float sy) {
    /* Same scale/dirty fields used by native 92CA55..92CA68. Scaling the
     * rectangle with 92F950 alone does not scale the rendered font. */
    *(float *)(r+OF(150))=sx;*(float *)(r+OF(151))=sy;
    INT(r,OF(145))=(INT(r,OF(146))&~4)|0x20000;
}
static U8 *text_resource(U8 *w) {
    return w && PTR(w,OF(147)) && INT(w,OF(148))>16?PTR(PTR(w,OF(149)),16*8):0;
}
static void read_hud_font(U8 *state) {
    U8 *w,*r,*style;I32 i,n,j;
    hud_font_ready=0;
    /* Live sortie HUD layouts, in native construction order (7DB2E0).
     * Read only: never alter the HUD, font assets or any shared texture. */
    for(j=0;j<2 && !hud_font_ready;++j) {
        w=PTR(state,j?0x478:0x470);
        if(!w || !PTR(w,OF(152)) || (n=INT(w,OF(153)))<=0 || n>256)continue;
        for(i=0;i<n;++i) {
            r=PTR(PTR(w,OF(154)),i*8);
            if(!r || r[OF(157)]!=2)continue;
            style=VFN(r,OF(155),U8 *(*)(void *,I32))(r,0);
            if(style){copy(hud_font_style,style,sizeof(hud_font_style));hud_font_ready=1;break;}
        }
    }
}
static void hud_font(U8 *r) {
    U8 *style;U32 palette[4];
    if(!hud_font_ready || !r || r[OF(158)]!=2)return;
    style=VFN(r,OF(156),U8 *(*)(void *,I32))(r,0);if(!style)return;
    /* Native FontStyle contains the typography used by ACA7D0/AD3DA0.
     * Preserve each CE control's palette; set its size/alignment below.
     * A rendered TextBox texture is not a font and must never be copied. */
    copy(palette,style,16);copy(style,hud_font_style,sizeof(hud_font_style));
    copy(style,palette,16);
}
static NI void font_rect(U8 *r,I32 x,I32 y,I32 width,I32 height,I32 pixels,I32 align) {
    if(!r || r[OF(159)]!=2 || width<=0 || height<=0 || pixels<12)return;
    trace_ui("text.size",-1,width,height);
    /* Native font metric setter, also used by 86F200. Rasterize at the
     * intended integer size, never downsample the finished glyph texture. */
    hud_font(r);resource_scale(r,1.0f,1.0f);
    FN(0x930710,FLAG)(r,pixels);
    FN(0x9304c0,FLAG)(r,align); /* 4: center/center; 3: left/center. */
    FN(0x92f870,void (*)(void *,I32,I32))(r,width,height);
    trace_ui("text.position",-1,width,height);
    FN(0x92f670,void (*)(void *,I32,I32))(r,x,y);
}
static NI void text_rect(U8 *w,I32 x,I32 y,I32 width,I32 height,I32 pixels) {
    font_rect(text_resource(w),x,y,width,height,pixels,4);
}
static NI void read_selection_font(void) {
    U8 *r=text_resource(clear_button),*style;
    selection_font_ready=0;
    if(!r || r[OF(162)]!=2)return;
    style=VFN(r,OF(160),U8 *(*)(void *,I32))(r,0);if(!style)return;
    copy(selection_font_style,style,sizeof(selection_font_style));
    selection_font_ready=1;
}
static NI I32 selection_font(U8 *r,I32 keep_palette) {
    U8 *style,wanted[0x78];
    if(!selection_font_ready || !r || r[OF(164)]!=2)return 0;
    style=VFN(r,OF(161),U8 *(*)(void *,I32))(r,0);if(!style)return 0;
    copy(wanted,selection_font_style,sizeof(wanted));
    if(keep_palette)copy(wanted,style,16);
    /* 930710 stores raster metrics at +10/+14; 9304C0 stores alignment
     * at +28/+29. Keep the destination's metrics and fixed text rectangle.
     * All font/effect/antialias settings otherwise come from Select All. */
    copy(wanted+OF(166),style+OF(167),8);copy(wanted+OF(168),style+OF(169),2);
    if(equal(wanted,style,sizeof(wanted)))return 0;
    copy(style,wanted,sizeof(wanted));return 1;
}
static NI void load_caption(const U16 *text,I32 multiple) {
    U8 *r=text_resource(ui.load),*style,previous[0x78];DisplayCache *c;
    if(r && r[OF(171)]==2 && (style=VFN(r,OF(165),U8 *(*)(void *,I32))(r,0))) {
        copy(previous,style,sizeof(previous));
        /* RC24 keeps the short Apply caption at the same 28px as the
         * other three toolbar controls, regardless of checked count. */
        (void)multiple;FN(0x930710,FLAG)(r,28);selection_font(r,1);
        if(!equal(previous,style,sizeof(previous))) {
            c=display_for(ui.load);if(c)c->valid=0;
        }
    }
    (void)text;caption(ui.load,L"적용");
}
static void text_line(TextLine *line,U8 *parent,I32 x,I32 y,I32 width,I32 height,I32 pixels,I32 align) {
    U8 *src,*dst,*context;SIZE count;
    if(!parent || !PTR(parent,OF(172)) || !(context=PTR(PTR(parent,OF(173)),0x10))){ui.failed=1;return;}
    /* Match 86F0A0's native dynamic-child capacity reservation. Font children
     * use the same context collection but 86F200 does not reserve it itself. */
    context+=0x18;count=VFN(context,OF(174),SIZE (*)(void *))(context);
    VFN(context,OF(175),void (*)(void *,SIZE))(context,count+1);
    line->r=FN(0x86f200,U8 *(*)(void *,I32,I32,I32,I32,const U16 *))(parent,0,0,width,height,L"");
    if(!line->r){ui.failed=1;return;}
    src=text_resource(parent);
    if(src && src[OF(178)]==2) {
        src=VFN(src,OF(176),U8 *(*)(void *,I32))(src,0);
        dst=VFN(line->r,OF(177),U8 *(*)(void *,I32))(line->r,0);
        if(src && dst)copy(dst,src,0x78); /* Same native font/effects as row. */
    }
    font_rect(line->r,x,y,width,height,pixels,align);
}
static void line_caption(TextLine *line,U8 *parent,const U16 *s) {
    U8 *src=text_resource(parent),*dst;I32 i,n=0,same=line->valid;
    if(!line->r)return;
    while(s[n] && n<159)++n;
    for(i=0;i<=n;++i)if(line->text[i]!=s[i])same=0;
    if(src && src[OF(189)]==2) {
        src=VFN(src,OF(179),U8 *(*)(void *,I32))(src,0);
        dst=VFN(line->r,OF(180),U8 *(*)(void *,I32))(line->r,0);
        if(src && dst && !equal(line->colors,src,16)) {
            copy(line->colors,src,16);copy(dst,src,16);same=0;
        }
    }
    if(same)return;
    copy(line->text,s,(SIZE)(n+1)*2);line->valid=1;
    FN(0x9302f0,TEXT)(line->r,s);
}
static void short_text(U16 *out,const U16 *s,I32 pixels,I32 width) {
    I32 n=0,used=0,cost,units;
    while(s && *s && n<158) {
        units=(*s>=0xd800 && *s<=0xdbff && s[1]>=0xdc00 && s[1]<=0xdfff)?2:1;
        cost=*s<128?(pixels+1)/2:pixels;
        if(n+units>158 || used+cost+(s[units]?pixels:0)>width){out[n++]=0x2026;break;}
        out[n++]=*s++;if(units==2)out[n++]=*s++;used+=cost;
    }
    out[n]=0;
}
static NI void fit_portrait(U8 *w) {
    U8 *r,*root,*p,*primitive;DisplayCache *c;I32 rw,rh,width,height,n;
    float cropx=0,cropy=0,uv[2],base[4][2],lo_x,hi_x,lo_y,hi_y;
    if(!w || !(r=PTR(w,OF(181))))return;
    rw=*(short *)(r+OF(187));rh=*(short *)(r+OF(188));
    width=INT(w,OF(182))-4;height=INT(w,OF(183))-4;
    if(r[OF(190)]!=1 || rw<=0 || rh<=0 || width<=0 || height<=0)return;
    c=display_for(w);
    if(c && c->image_w){rw=c->image_w;rh=c->image_h;copy(base,c->uv,sizeof(base));}
    else {
        for(n=0;n<4;++n)VFN(r,OF(184),void *(*)(void *,void *,I32,I32,I32))(r,base[n],0,n,0);
        if(c){c->image_w=rw;c->image_h=rh;copy(c->uv,base,sizeof(base));}
    }
    root=PTR(w,OF(185));p=r;
    for(n=0;p && p!=root && n<16;++n)p=PTR(p,OF(186));
    if(!root || p!=root)return;
    p=r;
    /* F0/F2 is a layout rectangle, not the final rendered extent. Remove
     * only this portrait layout's inherited template scale as well. */
    for(n=0;p && n<16;++n) {
        resource_scale(p,1.0f,1.0f);if(p==root)break;p=PTR(p,OF(191));
    }
    /* Crop the existing native headshot UV rectangle to cover the frame.
     * UV setters belong to this image; no shared texture pixels are edited.
     * Standard headshots are square, retaining their native face position. */
    if((long long)rw*height>(long long)rh*width)cropx=(1.0f-(float)(rh*width)/(rw*height))*0.5f;
    else cropy=(1.0f-(float)(rw*height)/(rh*width))*0.5f;
    lo_x=hi_x=base[0][0];lo_y=hi_y=base[0][1];
    for(n=1;n<4;++n) {
        if(base[n][0]<lo_x)lo_x=base[n][0];
        if(base[n][0]>hi_x)hi_x=base[n][0];
        if(base[n][1]<lo_y)lo_y=base[n][1];
        if(base[n][1]>hi_y)hi_y=base[n][1];
    }
    for(n=0;n<4;++n) {
        /* Inward interpolation preserves native UV orientation/corner order. */
        uv[0]=base[n][0]+(lo_x+hi_x-2.0f*base[n][0])*cropx;
        uv[1]=base[n][1]+(lo_y+hi_y-2.0f*base[n][1])*cropy;
        VFN(r,OF(192),void (*)(void *,I32,I32,void *))(r,0,n,uv);
    }
    FN(0x92f870,void (*)(void *,I32,I32))(r,width,height);
    FN(0x92f670,void (*)(void *,I32,I32))(r,2,2);
    /* B16B70's cached primitive stores its own old size and UVs. Read this
     * image's current rectangle instead of retaining that stale snapshot. */
    primitive=PTR(r,OF(193));if(primitive)primitive[OF(195)]&=~1;
}
static void set_detail_visual(U8 *w,U8 *person);
static void fit_slot_portrait(U8 *w);
static void set_face(U8 *w,U8 *person) {
    if(w && w==ui.detail_face){set_detail_visual(w,person);return;}
    DisplayCache *c;U8 *r;I32 id,n;if(!w)return;visible(w,person!=0);if(!person)return;
    c=display_for(w);id=*(U16 *)(person+OF(194));
    if(c && c->person==person && c->portrait==id)return;
    FN(0x8583b0,VOID2)(w,person);
    /* Each native binding can choose a different atlas rectangle. Refresh
     * its original UVs before cropping; never reuse another officer's UVs.
     * Keep the uncropped template aspect captured before the visual resize. */
    r=PTR(w,OF(196));
    if(c && r && r[OF(202)]==1) {
        if(!c->image_w) {
            c->image_w=*(short *)(r+OF(200));c->image_h=*(short *)(r+OF(201));
        }
        for(n=0;n<4;++n)VFN(r,OF(197),void *(*)(void *,void *,I32,I32,I32))(r,c->uv[n],0,n,0);
    }
    if(INT(w,OF(198))==FACE_SIZE && INT(w,OF(199))==ROW_H)fit_slot_portrait(w);
    else fit_portrait(w);
    if(c){c->person=person;c->portrait=id;}
}
static void set_formation(U8 *w,U8 *f) {
    DisplayCache *c;if(!w)return;visible(w,f!=0);if(!f)return;
    c=display_for(w);if(c && c->formation==f)return;
    FN(0x7e4de0,VOID2)(w,f);if(c)c->formation=f;
}
static NI U8 *button(U8 *parent,I32 x,I32 y,I32 w,I32 h,I32 style,const U16 *text,I32 action) {
    U8 *b;if(ui.failed)return 0;
    /* Native INIT7 always stretches children in mode 1: reducing the 85px
     * hanyou root to 48px subtracts 37px from its text child. A zero-height
     * texture makes the next text resize dereference NULL at game ACCCF4.
     * Use the game's 48px hanyou_defo family before initialization instead.
     * Preserve its native text layout; the 160x85 entry remains style 2. */
    if(style==2 && h<70)style=3;
    trace_ui("button.construct",style,w,h);
    b=alloc(OF(203));if(!b){ui.failed=1;return 0;}
    FN(0x84a390,GET1)(b);own(b);
    trace_ui("button.init",style,w,h);
    FN(0x84f360,INIT7)(b,x,y,w,h,parent,style);
    if(!PTR(b,OF(204)) || !PTR(b,OF(205))){ui.failed=1;return b;}
    trace_ui("button.caption",style,w,h);
    /* RC16: the bottom entry is the same 160x85 cmn_btn_hanyou as the
     * game's own bottom buttons. Its template already positions its visible
     * caption. Do not replace that font/anchor with a HUD TextBox rectangle. */
    if(action!=A_OPEN)hud_font(text_resource(b));
    caption(b,text);visible(b,0);layer(b,6);
    if(h<=62 && (style!=7 || (text && *text)))text_rect(b,6,4,w-12,h-8,28);
    trace_ui("button.bind",style,w,h);
    if(action)bind_action(b,action);else INT(b,OF(206))|=8;
    return b;
}
/* RC21: isolate the row's native skin before adding any preset children.
 * A layout rectangle includes invisible HitArea/animation nodes. It is NOT
 * the drawable's rectangle. Resize the active painted panes themselves and
 * synchronize the native Image draw snapshot, without touching portrait roots.
 * No texture pixels, UVs, material colors or shared vtables are replaced. */
enum { ROW_SKIN_PANES=48 };
typedef struct {U8 *r;I32 x,y,width,height;float sx,sy;} RowSkinPane;
typedef struct {U8 *w,*root;I32 count,applying;RowSkinPane panes[ROW_SKIN_PANES];} RowSkin;
static RowSkin row_skins[P_SLOTS];
static void *row_skin_vtable[TACTIC_VTABLE_BYTES/8+1];
static void *row_image_command_vtable[5];
static void *row_window_vtable[0x220/8+1];
static I32 row_skin_vtable_ready;
static I32 resource_descends(U8 *r,U8 *root) {
    I32 i;for(i=0;r && i<32;++i,r=PTR(r,OF(207)))if(r==root)return 1;
    return 0;
}
static RowSkin *row_skin_for(U8 *w) {
    I32 i;for(i=0;i<P_SLOTS;++i)if(row_skins[i].w==w)return &row_skins[i];
    return 0;
}
static I32 skin_painted(U8 *r,U8 *root) {
    U8 *p;I32 n;
    /* AC86B0's exact ancestor visibility bit. Alpha-zero helpers such as
     * HitArea must not determine the visible union either. 113 is the
     * native effective opacity populated before drawable submission. */
    for(p=r,n=0;p && n<32;++n,p=PTR(p,OF(208))) {
        if(INT(p,OF(209))&0x40)return 0;
        if(p==root)break;
    }
    return r[OF(210)]!=0;
}
static NI19 void apply_row_skin(U8 *w) {
    RowSkin *skin=row_skin_for(w);U8 *r,*p,*command,*snapshot;
    I32 i,j,top=2147483647,bottom=-2147483647,span,y,h,local_h,rootxy[2];
    float parent_sy;unsigned long long xy;
    if(!skin || !skin->root || skin->applying)return;
    skin->applying=1;
    for(i=0;i<skin->count;++i)if(skin_painted(skin->panes[i].r,skin->root)) {
        RowSkinPane *pane=&skin->panes[i];
        if(pane->y<top)top=pane->y;
        if(pane->y+pane->height>bottom)bottom=pane->y+pane->height;
    }
    if(top>=bottom)goto done; /* all native states hidden/not yet loaded */
    span=bottom-top;
    xy=FN(0x92c1b0,unsigned long long (*)(void *))(skin->root);copy(rootxy,&xy,8);
    for(i=0;i<skin->count;++i) {
        RowSkinPane *pane=&skin->panes[i];r=pane->r;
        if(!skin_painted(r,skin->root))continue;
        y=((pane->y-top)*FACE_SIZE+span/2)/span;
        h=((pane->y+pane->height-top)*FACE_SIZE+span/2)/span-y;
        if(h<1)continue;
        parent_sy=1.0f;
        for(p=PTR(r,OF(211)),j=0;p && j<32;++j,p=PTR(p,OF(212))) {
            float v=*(float *)(p+OF(213));
            if(!(v>0.0f && v<=16.0f)){parent_sy=0;break;}
            parent_sy*=v;if(p==skin->root)break;
        }
        if(!(parent_sy>0.0001f && parent_sy<=64.0f))continue;
        local_h=(I32)((float)h/parent_sy+0.5f);if(local_h<1)local_h=1;
        /* Change the actual F0/F2 quad consumed by B16570/B16B70, not just
         * the container or a template scale. Exact residual scaling avoids
         * rounding gaps with fractional inherited canvas scaling. */
        FN(0x92f870,void (*)(void *,I32,I32))(r,pane->width,local_h);
        resource_scale(r,pane->sx,(float)h/(local_h*parent_sy));
        FN(0x92f670,void (*)(void *,I32,I32))(r,rootxy[0]+pane->x,rootxy[1]+y);
        if(r[OF(214)]==3 && (PTR(r,0)==GA(0x168d650) || PTR(r,0)==(U8 *)(row_window_vtable+1))) {
            /* Native Window has NINE internal pieces at +190, not child
             * scene nodes. Its original three-strip mode (B1E080, +188=1)
             * uses the SAME left/middle/right textures across the full height,
             * avoiding transparent top/bottom frame margins. Recompute actual
             * piece sizes and matrices; F2 or scale alone is insufficient. */
            INT(r,OF(215))=1;
            FN(0xb1c480,I32 (*)(void *,void *,U8))(r,0,0);
        }
        if(r[OF(218)]==1 && (command=PTR(r,OF(216))) && PTR(command,OF(217))==r &&
           (PTR(command,0)==GA(0x168c6b8) || PTR(command,0)==(U8 *)row_image_command_vtable)) {
            /* Retain every native UV/material/cache flag. A cached command
             * has its OWN width/height; changing only F2 cannot update it. */
            if((command[OF(222)]&1) && (snapshot=PTR(command,OF(219))))
                INT(snapshot,OF(220))=INT(r,OF(221));
            PTR(command,0)=(U8 *)row_image_command_vtable;
        }
    }
done:
    skin->applying=0;
}
static NI19 U8 row_window_submit(U8 *r,U8 *renderer) {
    I32 i,j;
    /* Original Window draw entry runs AFTER native layout/animation. Only
     * this row-owned Window gets an adapter; shared CE vtables stay intact. */
    for(i=0;i<P_SLOTS;++i)for(j=0;j<row_skins[i].count;++j)
        if(row_skins[i].panes[j].r==r){apply_row_skin(row_skins[i].w);goto found;}
found:
    return FN(0xb1d7d0,U8 (*)(void *,void *))(r,renderer);
}
static NI19 U8 *row_skin_snapshot(U8 *command,U8 *destination) {
    U8 *r=PTR(command,OF(223));I32 i,j;
    /* B16570 is the REAL final Image snapshot factory. Native animation can
     * run after CCmnButton::Submit; correct the owned skin at this boundary.
     * The original factory still copies colors, UVs and all texture handles. */
    for(i=0;i<P_SLOTS;++i)for(j=0;j<row_skins[i].count;++j)
        if(row_skins[i].panes[j].r==r){apply_row_skin(row_skins[i].w);goto found;}
found:
    return FN(0xb16570,U8 *(*)(void *,void *))(command,destination);
}
static NI19 void row_skin_state(U8 *w,I32 state) {
    FN(0x84c5b0,FLAG)(w,state);apply_row_skin(w);
}
static NI19 void row_skin_submit(U8 *w) {
    apply_row_skin(w);FN(0x856450,VOID1)(w);
}
static NI19 I32 fill_row_skin(U8 *w) {
    U8 *root,*font,*r,*p,*touch=0,*context;RowSkin *skin=0;
    I32 i,j,n,rootxy[2],xy[2],h;float sy,parent_sy;unsigned long long packed;
    if(!w || !(root=PTR(w,OF(224))) || !PTR(w,OF(225)) || !PTR(w,OF(226)) ||
       PTR(PTR(w,OF(227)),0)!=GA(0x14570f0) ||
       (n=INT(w,OF(228)))<17 || n>128)return 0;
    for(i=0;i<P_SLOTS;++i)if(!row_skins[i].w){skin=&row_skins[i];break;}
    if(!skin)return 0;
    zero(skin,sizeof(*skin));skin->w=w;skin->root=root;
    font=PTR(PTR(w,OF(229)),16*8);context=PTR(w,OF(230));
    if(context && PTR(context,OF(231)))
        touch=FN(0xad0e80,U8 *(*)(void *,const char *))(PTR(context,OF(232)),"HitArea");
    packed=FN(0x92c1b0,unsigned long long (*)(void *))(root);copy(rootxy,&packed,8);
    for(i=0;i<n;++i) {
        r=PTR(PTR(w,OF(233)),i*8);
        if(!r || r==root || r==touch || (r[OF(234)]!=1 && r[OF(235)]!=3) ||
           !resource_descends(r,root) || (font && resource_descends(font,r)))continue;
        if(r[OF(236)]==3 && PTR(r,0)!=GA(0x168d650))continue;
        /* A Window owns its internal slices; do not resize those twice. */
        for(p=PTR(r,OF(237)),j=0;p && p!=root && j<32;++j,p=PTR(p,OF(238)))
            if(p[OF(247)]==3)break;
        if(p!=root)continue;
        for(j=0;j<skin->count;++j)if(skin->panes[j].r==r)break;
        if(j<skin->count)continue;
        if(skin->count>=ROW_SKIN_PANES)goto unsupported;
        parent_sy=1.0f;
        for(p=PTR(r,OF(239)),j=0;p && j<32;++j,p=PTR(p,OF(240))) {
            float v=*(float *)(p+OF(241));if(!(v>0.0f && v<=16.0f))goto unsupported;
            parent_sy*=v;if(p==root)break;
        }
        sy=*(float *)(r+OF(242));
        if(!(sy>0.0f && sy<=16.0f) || !(parent_sy>0.0f && parent_sy<=64.0f))goto unsupported;
        h=(I32)(*(short *)(r+OF(243))*sy*parent_sy+0.5f);
        if(h<1 || h>4096 || *(short *)(r+OF(244))<1)continue;
        packed=FN(0x92c1b0,unsigned long long (*)(void *))(r);copy(xy,&packed,8);
        xy[0]-=rootxy[0];xy[1]-=rootxy[1];
        if(xy[1]<-4096 || xy[1]>4096)goto unsupported;
        skin->panes[skin->count].r=r;skin->panes[skin->count].x=xy[0];
        skin->panes[skin->count].y=xy[1];skin->panes[skin->count].height=h;
        skin->panes[skin->count].width=*(short *)(r+OF(245));
        skin->panes[skin->count].sx=*(float *)(r+OF(246));
        skin->panes[skin->count].sy=sy;++skin->count;
    }
    if(!skin->count)goto unsupported;
    if(!row_skin_vtable_ready) {
        copy(row_skin_vtable,GA(0x14680a0)-8,TACTIC_VTABLE_BYTES+8);
        row_skin_vtable[1+0x1c0/8]=row_skin_state;
        row_skin_vtable[1+0x18/8]=row_skin_submit;
        copy(row_image_command_vtable,GA(0x168c6b8),sizeof(row_image_command_vtable));
        row_image_command_vtable[0x10/8]=row_skin_snapshot;
        copy(row_window_vtable,GA(0x168d650)-8,sizeof(row_window_vtable));
        row_window_vtable[1+0x08/8]=row_window_submit;
        row_skin_vtable_ready=1;
    }
    for(i=0;i<skin->count;++i)if(skin->panes[i].r[OF(248)]==3)
        PTR(skin->panes[i].r,0)=(U8 *)(row_window_vtable+1);
    PTR(w,0)=(U8 *)(row_skin_vtable+1);apply_row_skin(w);
    trace_ui("row.skin.quad_and_snapshot",skin->count,ROW_H,FACE_SIZE);
    return skin->count;
unsupported:
    trace_ui("row.skin.unsupported",skin->count,0,0);zero(skin,sizeof(*skin));return 0;
}
static NI U8 *face(U8 *parent,I32 x,I32 y,I32 width,I32 height) {
    U8 *w,*r;DisplayCache *c;
    if(ui.failed)return 0;
    trace_ui("portrait.construct",4,width,height);
    w=alloc(OF(249));if(!w){ui.failed=1;return 0;}
    FN(0x84a5d0,GET1)(w);own(w);
    trace_ui("portrait.init",4,96,96);
    /* Instantiate the existing square headshot at its canonical size first.
     * A wide INIT rectangle must not become the supposed source aspect. */
    FN(0x84fef0,INIT7)(w,x,y,96,96,parent,4);
    if(!PTR(w,OF(250))){ui.failed=1;return w;}
    r=PTR(w,OF(251));c=display_for(w);
    if(c && r[OF(254)]==1) {
        c->image_w=*(short *)(r+OF(252));c->image_h=*(short *)(r+OF(253));
    }
    trace_ui("portrait.size",4,width,height);
    INT(w,OF(255))=width;INT(w,OF(256))=height;
    /* Existing native layout resizing and native cmn_face_frame. Only the
     * frame rectangle changes; fit_portrait applies a bounded UV crop to
     * its interior, so image pixels never stretch or spill over siblings. */
    FN(0x9307d0,void (*)(void *,I32,I32))(w+OF(258),2,width);
    FN(0x92e890,void (*)(void *,I32,I32))(w+OF(259),2,height);
    FN(0x85f6e0,void (*)(void *,I32,I32,I32,I32))(w,0,0,width,height);
    INT(w,OF(257))|=8;visible(w,0);layer(w,6);return w;
}
static void crop_slot_sides(U8 *r,DisplayCache *c,I32 width,I32 height) {
    float current[4][2],expected[4][2],lo,hi,top,bottom,crop;I32 n,same=1;
    /* The standard square headshot never enters this path. For a wider
     * source, crop only the sides of the currently bound native UVs. Never
     * replay a previous/unfinished binding, or accumulate an earlier crop. */
    if((long long)c->image_w*height<=(long long)c->image_h*width)return;
    crop=(1.0f-(float)(c->image_h*width)/(c->image_w*height))*0.5f;
    lo=hi=c->uv[0][0];
    for(n=1;n<4;++n){if(c->uv[n][0]<lo)lo=c->uv[n][0];if(c->uv[n][0]>hi)hi=c->uv[n][0];}
    for(n=0;n<4;++n) {
        expected[n][0]=c->uv[n][0]+(lo+hi-2*c->uv[n][0])*crop;
        expected[n][1]=c->uv[n][1];
        VFN(r,OF(260),void *(*)(void *,void *,I32,I32,I32))(r,current[n],0,n,0);
        if(current[n][0]!=expected[n][0] || current[n][1]!=expected[n][1])same=0;
    }
    if(same)return;
    lo=hi=current[0][0];top=bottom=current[0][1];
    for(n=1;n<4;++n) {
        if(current[n][0]<lo)lo=current[n][0];
        if(current[n][0]>hi)hi=current[n][0];
        if(current[n][1]<top)top=current[n][1];
        if(current[n][1]>bottom)bottom=current[n][1];
    }
    if(!(hi-lo>0.000001f) || !(bottom-top>0.000001f))return;
    copy(c->uv,current,sizeof(current));
    for(n=0;n<4;++n) {
        current[n][0]+=(lo+hi-2*current[n][0])*crop;
        VFN(r,OF(261),void (*)(void *,I32,I32,void *))(r,0,n,current[n]);
    }
}
static NI void fit_slot_portrait(U8 *w) {
    DisplayCache *c=display_for(w);U8 *r,*p,*root;I32 width,height,n;
    if(!w || !(r=PTR(w,OF(262))) || r[OF(263)]!=1 || !c ||
       c->image_w<=0 || c->image_h<=0)return;
    /* CCmnImage has a real Image primitive directly below its blank root,
     * without the native face template's padding/animation transforms. */
    /* RC18: the row grows; its existing 62px portrait MUST NOT grow. */
    height=INT(w,OF(264));
    if(height<=0 || height>1024)return;
    width=(I32)((float)c->image_w*height/c->image_h);
    if(width>INT(w,OF(265)))width=INT(w,OF(266));
    if(width<=0)return;
    root=PTR(w,OF(267));p=r;
    for(n=0;p && p!=root && n<16;++n)p=PTR(p,OF(268));
    if(!root || p!=root)return;
    for(p=r,n=0;p && n<16;++n) {
        resource_scale(p,1.0f,1.0f);if(p==root)break;
        p=PTR(p,OF(269));
    }
    crop_slot_sides(r,c,width,height);
    /* Resize the actual native image quad, used by B16570 when the renderer
     * snapshots this frame's B16B70 draw command. The square headshot's UVs
     * remain untouched, as do every native primitive/cache flag and texture
     * handle. A wider source uses only the bounded side crop above. */
    FN(0x92f870,void (*)(void *,I32,I32))(r,width,height);
    FN(0x92f670,void (*)(void *,I32,I32))(r,(INT(w,OF(270))-width)/2,0);
}
static U8 *slot_face(U8 *parent) {
    U8 *w,*r;DisplayCache *c;
    if(ui.failed)return 0;
    w=alloc(OF(271));if(!w){ui.failed=1;return 0;}
    FN(0x864620,GET1)(w);own(w);
    /* Native CCmnImage (7D03B2/86BA40), its own resource context and a single
     * Image primitive. 0x2c6 is the native portrait loader's staging asset.
     * No face frame, hidden face container, copied pixels or custom atlas. */
    FN(0x86ba40,I32 (*)(void *,I32,I32,I32,I32,I32,void *))
        (w,FACE_X,0,FACE_SIZE,FACE_SIZE,0x2c6,parent);
    if(!(r=PTR(w,OF(272))) || r[OF(279)]!=1 || !PTR(w,OF(273))) {ui.failed=1;return w;}
    c=display_for(w);
    if(c){c->image_w=*(short *)(r+OF(276));c->image_h=*(short *)(r+OF(277));}
    INT(w,OF(274))|=8;visible(w,0);layer(w,6);return w;
}
static NI void set_slot_face(U8 *w,U8 *p) {
    DisplayCache *c;U8 *r;I32 id;
    if(!w || !(r=PTR(w,OF(275))))return;
    c=display_for(w);
    if(!p || !FN(0x32e4c0,I1)(p)) {
        visible(w,0);if(c){c->person=0;c->portrait=-1;}return;
    }
    id=*(U16 *)(p+OF(278));
    if(c && (c->person!=p || c->portrait!=id)) {
        visible(w,0);
        /* Exact CCmnFace::SetPerson mapping and flags (8583B0). Only the
         * destination is the plain Image above. Native handles and releases
         * remain owned by this image widget's private resource context. */
        FN(0x92fd00,void (*)(void *,I32,I32,void *,I32,I32,I32,I32,I32))
            (r,10,id,w,1,1,0,1,0);
        c->person=p;c->portrait=id;
    }
    fit_slot_portrait(w);
    visible(w,!(INT(r,OF(280))&0x40));
}
static NI void sync_slot_portrait(U8 *w) {
    U8 *r,*p,*root;DisplayCache *c;I32 n,width,height,changed=0;
    if(!w || !(INT(w,OF(281))&1) || !(r=PTR(w,OF(282))) || r[OF(283)]!=1)return;
    c=display_for(w);if(!c || c->image_w<=0 || c->image_h<=0)return;
    /* RC18: the row grows; its existing 62px portrait MUST NOT grow. */
    height=INT(w,OF(284));
    if(height<=0 || height>1024)return;
    width=(I32)((float)c->image_w*height/c->image_h);
    if(width>INT(w,OF(285)))width=INT(w,OF(286));
    if(width<1)return;
    changed=*(short *)(r+OF(289))!=width || *(short *)(r+OF(290))!=height;
    root=PTR(w,OF(287));
    for(p=r,n=0;p && n<16;++n) {
        if(*(float *)(p+OF(291))!=1.0f || *(float *)(p+OF(292))!=1.0f)changed=1;
        if(p==root)break;
        p=PTR(p,OF(288));
    }
    if(changed)fit_slot_portrait(w);
    else crop_slot_sides(r,c,width,height);
}
static NI U8 *blur_background(U8 *parent) {
    U8 *w;I32 width,height;
    if(ui.failed)return 0;
    w=alloc(OF(293));if(!w){ui.failed=1;return 0;}
    FN(0x84a2c0,GET1)(w);own(w);
    /* CCmnBlur is already used by the sortie screen. Native level 0 chooses
     * blur_strong_in. The game's BlurPane/renderer supplies the effect. */
    FN(0x84f160,void (*)(void *,void *,I32,I32))(w,parent,0,0);
    if(!PTR(w,OF(294)) || !PTR(w,OF(295)) || (width=INT(w,OF(296)))<=0 || (height=INT(w,OF(297)))<=0) {
        ui.failed=1;return w;
    }
    FN(0x84c540,void (*)(void *,float,float))(w,954.0f/width,783.0f/height);
    VFN(w,OF(298),void (*)(void *,I32,I32))(w,0,0);
    INT(w,OF(299))|=8;visible(w,0);layer(w,5);return w;
}
static NI U8 *row_outline(U8 *parent) {
    U8 *w=alloc(OF(301));if(!w){ui.failed=1;return 0;}
    zero(w,OF(302));FN(0x864680,GET1)(w);FN(0x92a020,VOID1)(w+OF(300));
    PTR(w,0)=GA(0x146b388);own(w);
    /* Reuse the game's border-only cmn_face_frame at the complete row bounds.
     * It is a separate passive outline, never a frame around the portrait. */
    FN(0x850110,I32 (*)(void *,I32,I32,I32,I32,void *))(w,ROW_X+1,ROW_Y,ROW_W,ROW_H,parent);
    if(!PTR(w,OF(303))){ui.failed=1;return w;}
    INT(w,OF(304))|=8;visible(w,0);layer(w,7);return w;
}
static U8 *bust_part(U8 *w,U32 name) {
    U8 *context=VFN(w,OF(305),GET1)(w);
    if(!context || !PTR(context,OF(306)))return 0;
    return FN(0xad0e80,U8 *(*)(void *,const char *))(PTR(context,OF(307)),(const char *)native_addresses[name]);
}
static I32 bust_asset_matches(U8 *w,I32 category,I32 portrait) {
    U8 *manager=PTR(GA(0x27d4390),0),*begin=PTR(w,OF(308)),*end=PTR(w,OF(309)),*entry;
    I32 expected;SIZE n,i;
    if(!manager || !begin || (SIZE)end<(SIZE)begin)return 0;
    n=((SIZE)end-(SIZE)begin)/8;if(n>64)return 0;
    /* Same native category/portrait mapper as 92FD00 and 858220. 8719F0
     * replaces the component's resource registrations on each binding.
     * Reject a native generic placeholder substituted for an absent asset. */
    expected=FN(0x8a7e10,I32 (*)(void *,I32,I32))(manager,category,portrait);
    /* 8A81E0 also resolves the mapped pack through manager+178. A resource
     * registration alone is insufficient when that pack was not loaded. */
    if(expected<=0 || !PTR(manager,OF(310)) || !PTR(PTR(manager,OF(311)),(SIZE)expected*8))return 0;
    for(i=0;i<n;++i) {
        entry=PTR(begin,i*8);if(entry && INT(entry,0)==expected)return 1;
    }
    return 0;
}
static I32 bust_image_loaded(U8 *r) {
    /* AC9140 hides failed loads with bit 0x40; old texture bytes may still
     * exist in the hidden image and must never count as a successful bind. */
    return r && r[OF(313)]==1 && !(INT(r,OF(312))&0x40);
}
static NI U8 *detail_bust(U8 *parent,I32 x,I32 y,I32 width,I32 height) {
    U8 *w,*special;I32 sw,sh;float scale,sx,sy;
    zero(&detail_visual,sizeof(detail_visual));
    if(ui.failed)return 0;
    trace_ui("bust.construct",-1,width,height);
    w=alloc(OF(314));if(!w){ui.failed=1;return 0;}
    FN(0x831100,GET1)(w);own(w);
    trace_ui("bust.init",-1,width,height);
    /* CInfoDetailAreaDefault 8D9213..8D9242 creates this exact component.
     * It loads cmn_ani_chara_3parts_cn and its native special-portrait child.
     * No external image I/O, copied textures, or private officer table. */
    FN(0x834d70,I32 (*)(void *,I32,I32,void *))(w,0,0,parent);
    visible(w,0);INT(w,OF(315))|=8;layer(w,6);
    detail_visual.parts[0]=bust_part(w,A_0x1464fe8);
    detail_visual.parts[1]=bust_part(w,A_0x1465008);
    detail_visual.parts[2]=bust_part(w,A_0x1465028);
    special=PTR(w,OF(316));
    if(special && PTR(special,OF(317)) && INT(special,OF(318))>3)
        detail_visual.special_image=PTR(PTR(special,OF(319)),3*8);
    sw=INT(w,OF(320));sh=INT(w,OF(321));
    if(sw>0 && sh>0 && sw<=8192 && sh<=8192 && special &&
       detail_visual.parts[0] && detail_visual.parts[1] && detail_visual.parts[2] &&
       detail_visual.special_image) {
        /* Fit the native illustration layout, including its special child,
         * inside the full visual region. The same factor on both axes keeps
         * the original art intact and its complete bounds inside the panel.
         * Do not apply the headshot frame/UV crop or resize shared textures. */
        sx=(float)width/sw;sy=(float)height/sh;scale=sx<sy?sx:sy;
        FN(0x841220,void (*)(void *,float,float))(w,scale,scale);
        VFN(w,OF(322),void (*)(void *,I32,I32))(w,
            x+(width-INT(w,OF(323)))/2,y+(height-INT(w,OF(324)))/2);
        detail_visual.ready=1;
    }
    /* Exceptional missing resources use the existing small 96px headshot;
     * it is never enlarged to fill the right illustration region. */
    detail_visual.fallback=face(parent,x+(width-96)/2,y+(height-96)/2,96,96);
    return w;
}
static void set_detail_visual(U8 *w,U8 *person) {
    U8 *special;I32 id,ok;
    if(!person) {
        visible(w,0);visible(detail_visual.fallback,0);
        detail_visual.person=0;detail_visual.loaded=0;return;
    }
    id=*(U16 *)(person+OF(325));
    if(detail_visual.person!=person || detail_visual.portrait!=id) {
        visible(w,0);detail_visual.loaded=0;
        if(detail_visual.ready) {
            /* Exact shared officer/city information binding (90697D/901CE8).
             * Its native table selects category 9 (three parts) or category
             * 12 (special animation). Keep that decision wholly in the game.
             * Flag 0 selects its native still pose, bounded by the layout. */
            FN(0x8400b0,void (*)(void *,I32,I32))(w,id,0);
            special=PTR(w,OF(326));
            if(special && (INT(special,OF(327))&1))
                ok=bust_image_loaded(detail_visual.special_image) && bust_asset_matches(special,12,id);
            else ok=bust_image_loaded(detail_visual.parts[0]) && bust_asset_matches(w,9,id);
            detail_visual.loaded=ok;
        }
        detail_visual.person=person;detail_visual.portrait=id;
    }
    /* show_panel may have made all owned widgets visible; enforce exactly
     * one visual even on a cached bind/reopen, with no resource reload. */
    visible(w,detail_visual.loaded);
    if(detail_visual.loaded)visible(detail_visual.fallback,0);
    else set_face(detail_visual.fallback,person);
}
static NI U8 *formation_button(U8 *parent,I32 x,I32 y,float scale) {
    U8 *w,*inner;if(ui.failed)return 0;trace_ui("formation.construct",-1,100,86);
    w=alloc(OF(331));if(!w){ui.failed=1;return 0;}
    FN(0x864680,GET1)(w);FN(0x92a020,VOID1)(w+OF(330));
    PTR(w,0)=GA(0x14592e0);PTR(w,OF(328))=0;INT(w,OF(329))=0;own(w);
    trace_ui("formation.init",-1,100,86);
    FN(0x7d1950,I32 (*)(void *,I32,I32,I32,I32,void *))(w,x,y,100,86,parent);
    inner=PTR(w,OF(332));
    if(!inner || !PTR(w,OF(333))){ui.failed=1;return w;}
    trace_ui("formation.scale",-1,100,86);
    FN(0x84c540,void (*)(void *,float,float))(w,scale,scale);
    FN(0x84c540,void (*)(void *,float,float))(inner,scale,scale);
    if(INT(w,OF(334))>4)font_rect(PTR(PTR(w,OF(335)),4*8),2,
        INT(w,OF(336))-(scale<1.0f?22:34),INT(w,OF(337))-4,
        scale<1.0f?20:32,scale<1.0f?16:28,4);
    INT(w,OF(338))|=8;INT(inner,OF(339))|=8;visible(w,0);layer(w,6);return w;
}
static NI U8 *balloon(I32 passive) {
    U8 *w;if(ui.failed)return 0;trace_ui(passive?"tooltip.construct":"panel.construct",-1,0,0);
    w=alloc(OF(342));if(!w){ui.failed=1;return 0;}
    FN(0x864680,GET1)(w);FN(0x92a020,VOID1)(w+OF(341));
    PTR(w,0)=GA(0x139c4f0);PTR(w,OF(340))=0;own(w);
    trace_ui(passive?"tooltip.init":"panel.init",-1,0,0);
    FN(0x89beb0,I32 (*)(void *,void *))(w,0);
    if(!PTR(w,OF(343))){ui.failed=1;return w;}
    trace_ui(passive?"tooltip.alpha":"panel.alpha",-1,0,0);
    if(passive)INT(w,OF(344))|=8;else INT(w,OF(345))&=~8;
    FN(0x8a13d0,void (*)(void *,U8))(w,255);
    visible(w,0);layer(w,6);return w;
}
static NI void tactic_native_hover(U8 *self,U8 **argument) {
    U8 *t; (void)self;(void)argument;
    if(!ui.shown || !tactic_tip || !last_tactic_target || !(t=tactic(last_tactic_id)))return;
    /* 7C43A0's original data binding, followed by 8A2660's complete native
     * description/layout. No strings or effect interpretation in this mod.
     * Do not append the army tab's CURRENT formation error to a saved tactic. */
    FN(0x8a0ec0,VOID2)(tactic_tip,t);
}
static void release_tactic_manager(void) {
    if(tactic_tip_manager)list_free((NList *)(tactic_tip_manager+OF(346)));
    tactic_tip_manager=0;zero(tactic_manager_storage,sizeof(tactic_manager_storage));
    tactic_hover_alias=0;
}
static NI19 U8 *create_tactic_tooltip(void) {
    U8 *w,*m=tactic_manager_storage;
    if(tactic_tip_ready)return tactic_tip;
    if(!ui.state || ui.failed)return 0;
    zero(m,sizeof(tactic_manager_storage));
    if(!list_init((NList *)(m+OF(347))))return 0;
    tactic_tip_manager=m;
    FN(0x89b2a0,VOID1)(m); /* same native manager initialization as 7D3578 */
    w=alloc(OF(348));if(!w){release_tactic_manager();return 0;}
    FN(0x899f60,GET1)(w);
    FN(0x89bf40,I1)(w); /* exact CCmnTacticsBalloon::Initialize, no guessed args */
    if(!PTR(w,OF(349)) || !PTR(w,OF(350)) || INT(w,OF(351))<1 || INT(w,OF(352))>128) {
        VFN(w,OF(353),VOID2)(w,ui.resources);VFN(w,0,FLAG)(w,1);
        release_tactic_manager();trace_ui("tactic.native.init.failed",0,0,0);return 0;
    }
    own(w);layer(w,6);FN(0x8a0ce0,VOID2)(m,w);
    /* Same inline std::function ABI as the native manager and our existing
     * native button callbacks. It is never installed on the army tab. */
    copy(tactic_hover_vtable,callback_vt,sizeof(tactic_hover_vtable));
    tactic_hover_vtable[0x10/8]=(void *)tactic_native_hover;
    PTR(m,OF(354))=(U8 *)tactic_hover_vtable;PTR(m,OF(355))=ui.state;
    PTR(m,OF(356))=m+OF(357);
    tactic_tip=w;tactic_tip_ready=1;tactic_tip_diag=1;
    trace_ui("tactic.native.owned.ready",INT(w,OF(358)),INT(w,OF(359)),INT(w,OF(360)));
    return w;
}
static void hide_tactic_tooltip(void) {
    if(tactic_tip)visible(tactic_tip,0);
    if(tactic_tip_manager) {
        PTR(tactic_tip_manager,OF(361))=0;
        if(tactic_hover_alias)FN(0x7b0ac0,VOID1)(tactic_tip_manager+OF(362));
    }
    tactic_hover_alias=0;tactic_tip_using=0;last_tactic_target=0;last_tactic_id=-1;
}
static NI U8 *trait_label(U8 *parent,I32 x,I32 y) {
    U8 *w;if(ui.failed)return 0;trace_ui("trait.construct",0,GRID_W,GRID_H);
    w=alloc(OF(363));if(!w){ui.failed=1;return 0;}
    FN(0x84a940,GET1)(w);own(w);
    trace_ui("trait.init",0,GRID_W,GRID_H);
    FN(0x8509b0,INIT7)(w,x,y,GRID_W,GRID_H,parent,0);
    if(!PTR(w,OF(364))){ui.failed=1;return w;}
    font_rect(PTR(w,OF(365)),4,4,GRID_W-8,GRID_H-8,GRID_FONT,4);
    INT(w,OF(366))=0;INT(w,OF(367))=1;visible(w,0);layer(w,6);return w;
}
static void register_widget(Owned *o,I32 yes) {
    if(!o->w || o->active==yes)return;
    VFN(o->w,yes?0x38:0x40,VOID1)(o->w);o->active=yes;
}
#include "group_ui.h"
#include "save_warning_ui.h"

static NI void destroy_ui(void *resources) {
    I32 i;U8 *w;
    cancel_save();
    ++group_rename_epoch;ui.rename_waiting=0;group_rename_result=0;
    hide_tactic_tooltip();release_tactic_manager();
    cancel_delete();cancel_selection_gesture();selected_mask=batch_mask=unavailable_checks=0;batch_mode=0;lastSelectedSlotIndex=-1;
    clear_action_queue();ui.want_open=0;ui.shown=0;
    for(i=ui.count-1;i>=0;--i) {
        w=ui.owned[i].w;if(!w)continue;
        visible(w,0);register_widget(&ui.owned[i],0);
        VFN(w,OF(368),VOID2)(w,resources);VFN(w,0,FLAG)(w,1);
        ui.owned[i].w=0;
    }
    zero(row_skins,sizeof(row_skins));
    ui.count=0;ui.state=0;active_state=0;panel_ready=0;
    save_warning=save_yes=save_no=0;zero(save_warning_lines,sizeof(save_warning_lines));
    tactic_tip=last_tactic_target=tactic_tip_manager=0;tactic_tip_ready=0;last_tactic_id=-1;
    tactic_tip_using=tactic_tip_diag=0;tactic_hover_alias=0;
    zero(tactic_views,sizeof(tactic_views));tactic_view_count=0;
    last_tip_text=0;last_tip_target=0;zero(pins,sizeof(pins));
}
static NI I32 create_ui(U8 *s,void *resources) {
    I32 y,x;U8 *bottom=PTR(s,OF(369));
    zero(&ui,sizeof(ui));clear_action_queue();
    zero(&save_batch,sizeof(save_batch));++save_epoch;
    save_warning=save_yes=save_no=0;zero(save_warning_lines,sizeof(save_warning_lines));
    group_empty_font_ready=0;
    ++group_rename_epoch;group_rename_result=0;
    zero(group_buttons,sizeof(group_buttons));zero(&group_title,sizeof(group_title));
    group_right_down=0;group_right_capture=-1;
    zero(&input_diagnostics,sizeof(input_diagnostics));
    input_row_consumed=input_drag_consumed=0;
    ui.state=s;ui.resources=resources;ui.selected=0;ui.rename_slot=-1;
    zero(row_skins,sizeof(row_skins));
    zero(display_cache,sizeof(display_cache));panel_ready=0;last_tip_text=0;last_tip_target=0;
    zero(row_names,sizeof(row_names));zero(row_troops,sizeof(row_troops));
    selection_font_ready=0;
    zero(&notice_first,sizeof(notice_first));zero(&notice_second,sizeof(notice_second));
    zero(row_checks,sizeof(row_checks));zero(row_checkmarks,sizeof(row_checkmarks));
    zero(row_detailmarks,sizeof(row_detailmarks));
    zero(tactic_views,sizeof(tactic_views));tactic_view_count=0;
    tactic_tip=last_tactic_target=tactic_tip_manager=0;tactic_tip_ready=0;last_tactic_id=-1;
    tactic_tip_using=tactic_tip_diag=0;tactic_hover_alias=0;
    inputs_valid=0;status_two_lines=-1;
    panel_backdrop=detail_outline=clear_button=selection_summary=delete_button=0;
    notice_strip=delete_yes=delete_no=0;
    zero(&delete_snapshot,sizeof(delete_snapshot));
    delete_epoch=delete_screen_generation=delete_unavailable_mask=0;
    selected_mask=unavailable_checks=delete_pending_mask=0;
    reset_slot=-1;batch_mode=0;batch_mask=0;
    lastSelectedSlotIndex=-1;selection_input_init();cancel_selection_gesture();
    ui.generation=++generation;active_state=s;
    read_hud_font(s);
    /* Keep the native button dimensions; anchor within the actual bottom bar.
     * The panel follows the current tab origin, clamped to the native canvas. */
    x=INT(bottom,OF(370))-330;y=INT(bottom,OF(371))-85;if(y>44)y=44;
    ui.entry=button(bottom,x,y,160,85,2,L"프리셋",A_OPEN);
    if(ui.failed)return 0;
    /* Keep cmn_btn_hanyou's original caption geometry and font style.
     * Its visible face is not the entire 160x85 widget rectangle; forcing
     * a 148x77 HUD TextBox here caused the caption to sit above the face. */
    trace_ui("entry.register",2,160,85);
    register_widget(&ui.owned[0],1);visible(ui.entry,1);ui_build_stage="entry_ready";
    return 1;
}
static NI I32 build_panel(void) {
    I32 i,k,y,px,py;U8 *p,*r,*tab=PTR(ui.state,OF(372)),*root=PTR(ui.state,OF(373));
    ui.panel=balloon(0);p=ui.panel;if(ui.failed)return 0;
    /* Resize exactly as native CSimpleBalloonLayout::AdjustSize (0x8a5180). */
    INT(p,OF(374))=954;INT(p,OF(375))=783;
    trace_ui("panel.width",-1,954,783);
    FN(0x9307d0,void (*)(void *,I32,I32))(p+OF(376),2,954);
    trace_ui("panel.height",-1,954,783);
    FN(0x92e890,void (*)(void *,I32,I32))(p+OF(378),2,783);
    r=PTR(PTR(p,OF(377)),8);
    if(!r){ui.failed=1;return 0;}
    trace_ui("panel.size",-1,954,783);
    FN(0x92f870,void (*)(void *,I32,I32))(r,954,783);
    FN(0x92ecd0,void (*)(void *,I32,I32))(p+OF(379),3,0);
    /* Native dynamic window helper: existing atlas, nine slices and corners.
     * Keep this panel's owner/input rectangle/lifetime; replace only its
     * tooltip backdrop with the game's own generated-window background. */
    panel_backdrop=FN(0x86f330,U8 *(*)(void *,I32,I32,I32,I32))(p,0,0,954,783);
    if(!panel_backdrop){ui.failed=1;return 0;}
    /* Alpha applies only to this background resource; never to the parent
     * panel or its text, buttons, portraits or fixed right illustration. */
    FN(0x92e150,void (*)(void *,U8))(panel_backdrop,150);
    blur_background(p);
    if(ui.failed)return 0;
    FN(0x92ecd0,void (*)(void *,I32,I32))(p+OF(395),2,0);
    px=INT(tab,OF(380));py=INT(tab,OF(381));
    if(px<INT(root,OF(382)))px=INT(root,OF(383));
    if(py<INT(root,OF(384)))py=INT(root,OF(385));
    if(px>INT(root,OF(386))+INT(root,OF(387))-954)px=INT(root,OF(388))+INT(root,OF(389))-954;
    if(py>INT(root,OF(390))+INT(root,OF(391))-783)py=INT(root,OF(392))+INT(root,OF(393))-783;
    VFN(p,OF(394),void (*)(void *,I32,I32))(p,px,py);
    detail_outline=row_outline(p);
    for(i=0;i<P_SLOTS && !ui.failed;++i) {
        Row *row=&ui.rows[i];y=ROW_Y+i*ROW_STEP;
        row->button=button(p,ROW_X,y,ROW_W,ROW_H,7,L"",A_ROW+i);
        if(ui.failed)break;
        fill_row_skin(row->button);
        /* Only the existing 36px number gutter changes: number above, check
         * below. Text children avoid shrinking/introducing button templates. */
        text_rect(row->button,NUMBER_X,0,30,20,18);
        /* Detail focus is the independent native outline, not a gutter glyph. */
        text_line(&row_checks[i],row->button,CHECK_X,CHECK_Y,CHECK_W,CHECK_H,CHECK_FONT,4);
        text_line(&row_checkmarks[i],row->button,CHECK_X,CHECK_Y,CHECK_W,CHECK_H,CHECK_FONT,4);
        row->face=slot_face(row->button);
        text_line(&row_names[i],row->button,NAME_X,5,NAME_W,26,22,3);
        text_line(&row_troops[i],row->button,NAME_X,31,NAME_W,26,20,3);
        row->formation=formation_button(row->button,FORMATION_X,5,0.6f);
        for(k=0;k<3 && !ui.failed;++k) {
            row->tactics[k]=button(row->button,TACTIC_X+k*TACTIC_STEP,9,TACTIC_W,44,7,L"",A_ROW+i);
            if(!ui.failed) {
                if(!attach_tactic_display(row->tactics[k])){ui.failed=1;break;}
                text_rect(row->tactics[k],4,4,TACTIC_W-8,36,20);
            }
        }
    }
    if(ui.failed)return 0;
    ui.save=button(p,ROW_X,TOOLBAR_Y,TOOLBAR_W,TOOLBAR_H,2,L"저장",A_SAVE);
    selection_summary=0; /* no widget and no hit target */
    delete_button=button(p,ROW_X+TOOLBAR_STEP,TOOLBAR_Y,TOOLBAR_W,TOOLBAR_H,2,L"삭제",A_DELETE);
    ui.detail_name=button(p,DETAIL_X,DETAIL_NAME_Y,DETAIL_W,DETAIL_NAME_H,7,L"",0);
    if(!ui.failed)text_rect(ui.detail_name,6,4,DETAIL_W-12,DETAIL_NAME_H-8,28);
    ui.detail_face=detail_bust(p,DETAIL_X,DETAIL_FACE_Y,DETAIL_W,DETAIL_FACE_H);
    ui.detail_formation=formation_button(p,DETAIL_X,DETAIL_INFO_Y,0.6f);
    ui.detail_troops=button(p,DETAIL_TROOPS_X,DETAIL_INFO_Y,DETAIL_TROOPS_W,DETAIL_INFO_H,7,L"",0);
    if(!ui.failed)text_rect(ui.detail_troops,6,4,DETAIL_TROOPS_W-12,DETAIL_INFO_H-8,28);
    for(i=0;i<3 && !ui.failed;++i) {
        ui.detail_tactics[i]=button(p,DETAIL_X+i*GRID_STEP,DETAIL_TACTIC_Y,GRID_W,GRID_H,7,L"",0);
        if(!ui.failed) {
            if(!attach_tactic_display(ui.detail_tactics[i])){ui.failed=1;break;}
            text_rect(ui.detail_tactics[i],4,4,GRID_W-8,GRID_H-8,GRID_FONT);
        }
    }
    for(i=0;i<9 && !ui.failed;++i)ui.labels[i]=trait_label(p,DETAIL_X+(i%3)*GRID_STEP,DETAIL_TRAIT_Y+(i/3)*GRID_Y_STEP);
    if(ui.failed || !ui.labels[8])return 0;
    /* Nine new native buttons, matching the trait grid geometry exactly. */
    read_group_empty_font();
    text_line(&group_title,p,DETAIL_X,GROUP_TITLE_Y,DETAIL_W,20,20,4);
    group_title_style();
    line_caption(&group_title,0,L"프리셋 그룹");
    for(i=0;i<G_COUNT && !ui.failed;++i) {
        group_buttons[i]=button(p,DETAIL_X+(i%3)*GRID_STEP,GROUP_Y+(i/3)*GRID_Y_STEP,
                                GRID_W,GRID_H,7,L"",A_GROUP+i);
        if(!ui.failed) {
            text_rect(group_buttons[i],4,4,GRID_W-8,GRID_H-8,GRID_FONT);
            if(!attach_group_button(group_buttons[i])){ui.failed=1;break;}
        }
    }
    if(ui.failed)return 0;
    /* Messages alone move; existing portraits/traits/tactics do not. */
    notice_strip=FN(0x86f330,U8 *(*)(void *,I32,I32,I32,I32))(p,DETAIL_X,MESSAGE_Y-2,DETAIL_W,42);
    if(!notice_strip){ui.failed=1;return 0;}
    FN(0x92e150,void (*)(void *,U8))(notice_strip,210);
    for(i=0;i<2 && !ui.failed;++i) {
        TextLine *line=i?&notice_second:&notice_first;U8 *src,*dst;
        text_line(line,p,DETAIL_X+6,MESSAGE_Y+i*20,DETAIL_W-12,20,20,4);
        src=text_resource(ui.detail_name);
        if(line->r && src && src[OF(398)]==2) {
            src=VFN(src,OF(396),U8 *(*)(void *,I32))(src,0);
            dst=VFN(line->r,OF(397),U8 *(*)(void *,I32))(line->r,0);
            if(src && dst)copy(dst,src,0x78);
            font_rect(line->r,DETAIL_X+6,MESSAGE_Y+i*20,DETAIL_W-12,20,20,4);
        }
    }
    ui.load=button(p,ROW_X+2*TOOLBAR_STEP,TOOLBAR_Y,TOOLBAR_W,TOOLBAR_H,2,L"적용",A_LOAD);
    if(!ui.failed)text_rect(ui.load,6,4,TOOLBAR_W-12,TOOLBAR_H-8,28);
    clear_button=button(p,ROW_X+3*TOOLBAR_STEP,TOOLBAR_Y,TOOLBAR_W,TOOLBAR_H,2,L"일괄 선택",A_SELECT_ALL);
    if(!ui.failed) {
        DisplayCache *c;
        text_rect(ui.save,6,4,TOOLBAR_W-12,TOOLBAR_H-8,28);
        text_rect(delete_button,6,4,TOOLBAR_W-12,TOOLBAR_H-8,28);
        text_rect(clear_button,6,4,TOOLBAR_W-12,TOOLBAR_H-8,28);
        read_selection_font();
        selection_font(text_resource(ui.detail_troops),1);
        selection_font(text_resource(ui.load),1);
        /* The constructor already cached its caption at the old size. */
        c=display_for(ui.load);if(c)c->valid=0;
        caption(ui.load,L"적용");
        selection_font(text_resource(ui.save),1);selection_font(text_resource(delete_button),1);
    }
    /* RC17: build ordinary panel-owned confirmation buttons ONCE, exactly
     * where the existing Load/Clear pair already renders and receives input.
     * Normal pair is hidden only while this pair is shown. No new popup. */
    delete_yes=button(p,ROW_X+2*TOOLBAR_STEP,TOOLBAR_Y,TOOLBAR_W,TOOLBAR_H,2,L"삭제 확인",A_DELETE_CONFIRM);
    delete_no=button(p,ROW_X+3*TOOLBAR_STEP,TOOLBAR_Y,TOOLBAR_W,TOOLBAR_H,2,L"취소",A_DELETE_CANCEL);
    if(!ui.failed)text_rect(delete_yes,6,4,TOOLBAR_W-12,TOOLBAR_H-8,28);
    /* RC15: the existing entry toggles open/closed; no second Close widget. */
    ui.tip=balloon(1);
    tactic_tip=create_tactic_tooltip();
    if(!ui.failed)build_save_warning(p);
    if(ui.failed)return 0;
    panel_ready=1;ui_build_stage="panel_ready";
    return 1;
}
static NI I32 ensure_panel(void) {
    I32 i;U8 *w;if(panel_ready)return 1;if(ui.failed)return 0;
    log_layout("PANEL_BUILD_BEGIN",ui.state,0);
    if(build_panel()){log_layout("PANEL_READY",ui.state,0);return 1;}
    log_layout("PANEL_BUILD_FAILED",ui.state,0);
    hide_tactic_tooltip();release_tactic_manager();
    /* Keep the native sortie screen and its entry alive if optional content
     * fails. No incomplete rows are registered or retried every frame. */
    for(i=ui.count-1;i>=1;--i) {
        w=ui.owned[i].w;if(!w)continue;
        visible(w,0);register_widget(&ui.owned[i],0);
        VFN(w,OF(399),VOID2)(w,ui.resources);VFN(w,0,FLAG)(w,1);
        ui.owned[i].w=0;
    }
    zero(row_skins,sizeof(row_skins));
    ui.count=1;ui.panel=0;ui.tip=0;ui.want_open=0;
    save_warning=save_yes=save_no=0;zero(save_warning_lines,sizeof(save_warning_lines));
    tactic_tip=last_tactic_target=tactic_tip_manager=0;tactic_tip_ready=0;last_tactic_id=-1;
    tactic_tip_using=tactic_tip_diag=0;tactic_hover_alias=0;
    zero(tactic_views,sizeof(tactic_views));tactic_view_count=0;
    caption(ui.entry,L"프리셋 오류");return 0;
}
static NI void show_panel(void) {
    I32 i;if(ui.failed || ui.rename_waiting || !ui.state)return;
    if(!ensure_panel())return;
    ui.want_open=1;if(ui.shown)return;ui.shown=1;cancel_selection_gesture();
    for(i=1;i<ui.count;++i){register_widget(&ui.owned[i],1);visible(ui.owned[i].w,1);}
    cancel_delete();cancel_save();
    visible(ui.tip,0);hide_tactic_tooltip();last_tip_text=0;last_tip_target=0;refresh_panel(1);
}
static NI void close_panel(void) {
    I32 i;cancel_save();cancel_delete();cancel_reset();cancel_selection_gesture();clear_action_queue();ui.shown=0;
    for(i=ui.count-1;i>0;--i){visible(ui.owned[i].w,0);register_widget(&ui.owned[i],0);}
    last_tip_text=0;last_tip_target=0;hide_tactic_tooltip();
}
static void set_tactic(U8 *w,I32 id,I32 available) {
    U8 *t=tactic(id);TacticView *view=tactic_view_for(w);
    if(view){view->id=t?id:0;view->available=t && available;}
    INT(w,OF(400))=t?id:0; /* Native tactic-hover callback's real data ID. */
    caption(w,t?(const U16 *)(t+OF(401)):L"—");
    visible(w,1);enable(w,t && available);
    if(view)tactic_display_state(w,0);
    else selected(w,t && available);
}
static void status_caption(const U16 *s) {
    U16 out[80],second[80];I32 n=0,units=0,line=0,weight;
    /* Common button captions render one line. Use a native text child for
     * the second line instead of embedding an ignored newline character. */
    while(s && *s && n<77) {
        weight=*s<128?1:2;
        if(*s==L'\n' || units+weight>26) {
            if(line){out[n++]=0x2026;break;}
            out[n++]=L'\n';line=1;units=0;
            if(*s==L'\n' || *s==L' '){++s;continue;}
        }
        out[n++]=*s++;units+=weight;
    }
    out[n]=0;
    for(n=0;out[n] && out[n]!=L'\n';++n){}
    if(out[n]){out[n++]=0;append(second,0,80,out+n);}
    else second[0]=0;
    /* A single line stays centered in the former bar's area, with no frame. */
    if(!!second[0]!=status_two_lines) {
        status_two_lines=!!second[0];
        font_rect(notice_first.r,DETAIL_X+6,MESSAGE_Y,DETAIL_W-12,second[0]?20:40,20,4);
    }
    line_caption(&notice_first,ui.detail_name,out);line_caption(&notice_second,ui.detail_name,second);
    FN(0x92ed50,FLAG)(notice_strip,out[0]!=0);
    /* Four vertex glyph colors, not parent/image alpha. Contrast remains
     * stable when selection changes the native row/button text palette. */
    {
        U8 *style;I32 i;
        TextLine *lines[2]={&notice_first,&notice_second};
        for(i=0;i<2;++i)if(lines[i]->r) {
            style=VFN(lines[i]->r,OF(402),U8 *(*)(void *,I32))(lines[i]->r,0);
            if(style)for(I32 k=0;k<4;++k)INT(style,k*4)=(I32)0xfff5ebd0u;
        }
    }
}
static NI I32 context_changed(void) {
    I32 i,n,changed;U8 *city;
    /* Sortie simulation is paused; native edits are visible in these inputs.
     * Compare compact selections, not ten preset/context rebuilds per tick.
     * Native modal return and every preset mutation already refresh in full. */
    typeof(ui_inputs) now;
    zero(&now,sizeof(now));city=PTR(ui.state,OF(403));n=records(ui.state);
    now.city=city;now.count=n;now.index=INT(ui.state,OF(404));
    now.troops=INT(city,OF(405));now.gold=VFN(city,OF(406),I1)(city);now.limit=unit_limit(ui.state);
    current_selection(ui.state,&now.current);
    for(i=0;i<n && i<9;++i)selection_from(record_at(ui.state,i),&now.records[i]);
    changed=!inputs_valid || !equal(&now,&ui_inputs,sizeof(now));
    copy(&ui_inputs,&now,sizeof(now));inputs_valid=1;return changed;
}
static I32 can_select_slot(I32 slot) {
    return slot>=0 && slot<P_SLOTS && book.slots[slot].occupied;
}
static I32 can_apply_slot(I32 slot) {
    return can_select_slot(slot) && ui.rows[slot].reason==N_OK;
}
static U32 checked_slots(void) { return selected_mask; }
static void row_tone(I32 i) {
    U8 *w=ui.rows[i].button;I32 style;
    if(!w)return;
    /* Native list Hover is the weak full-row material. Keep the checked bit
     * independent, with the full-strength native border marking detail. */
    /* A native selected bit chooses the stronger orange press/active state
     * during native input updates. Checks use hover material only; detail is
     * represented exclusively by detail_outline. */
    INT(w,OF(407))&=~0x100;
    style=(checked_slots()&(1u<<i))?1:
        book.slots[i].occupied && !can_apply_slot(i)?4:(INT(w,OF(408))&0x80)?1:0;
    VFN(w,OF(409),FLAG)(w,style);
    VFN(w,OF(410),FLAG)(w,style==4?150:255);
}
static void row_hover(I32 slot) {
    I32 i;U8 *w;
    for(i=0;i<P_SLOTS;++i)if((w=ui.rows[i].button)) {
        INT(w,OF(411))&=~0x80;if(i==slot)INT(w,OF(412))|=0x80;
        row_tone(i);
    }
}
static NI void refresh_selection(void) {
    I32 i,count=0,deploy_count=0,stored_count=0;U32 bit;
    selected_mask&=(1u<<P_SLOTS)-1;
    batch_mask=unavailable_checks=0;
    for(i=0;i<P_SLOTS;++i) {
        bit=1u<<i;
        if(!can_select_slot(i)){selected_mask&=~bit;continue;}
        ++stored_count;
        if(selected_mask&bit) {
            if(can_apply_slot(i))batch_mask|=bit;
            else unavailable_checks|=bit;
        }
    }
    batch_mode=selected_mask!=0;
    if(!can_select_slot(lastSelectedSlotIndex))lastSelectedSlotIndex=-1;
    for(i=0;i<P_SLOTS;++i) {
        Row *r=&ui.rows[i];I32 checked=!!(checked_slots()&(1u<<i));
        if(checked)++count;
        if(batch_mask&(1u<<i))++deploy_count;
        row_tone(i);
        /* Borrow the neutral native caption palette for gutter symbols so
         * selected-row gold text cannot hide them on its gold background. */
        line_caption(&row_checks[i],book.slots[i].occupied?ui.detail_name:r->button,L"□");
        /* All eleven rows, including empty slot 0, share the same checkbox.
         * Only empty checks are inert/dim; unavailable records can be managed. */
        FN(0x92e150,void (*)(void *,U8))(row_checks[i].r,book.slots[i].occupied?255:110);
        line_caption(&row_checkmarks[i],ui.detail_name,checked?L"√":L"");
        /* Names retain Select All's neutral ink; the existing row alpha
         * still dims unavailable entries. Empty captions keep their style. */
        line_caption(&row_names[i],book.slots[i].occupied && selection_font_ready?0:r->button,row_names[i].text);
        line_caption(&row_troops[i],r->button,row_troops[i].text);
    }
    if(detail_outline && ui.selected>=0 && ui.selected<P_SLOTS) {
        VFN(detail_outline,OF(413),void (*)(void *,I32,I32))(detail_outline,ROW_X,ROW_Y+ui.selected*ROW_STEP);
        visible(detail_outline,ui.shown);
    }
    enable(delete_button,count>0 && storage_writable);
    caption(clear_button,count?L"일괄 해제":L"일괄 선택");enable(clear_button,count>0 || stored_count>0);
    load_caption(L"적용",count>1);
    enable(ui.load,deploy_count>0 || (!count && can_apply_slot(ui.selected)));
    if(delete_pending_mask)present_delete_confirmation();
    if(save_batch.pending)present_save_confirmation();
}
static NI void refresh_panel(I32 full) {
    P_Context ctx;P_LoadPlan plan;U8 *p,*f,*t;P_Preset *slot;
    U16 text[160];U32 at;I32 i,k,id,exists,reason;
    if(!ui.shown)return;
    if(full)hide_tactic_tooltip();
    /* Row portraits only: repair a late native draw cache without rebinding
     * assets, moving columns, refreshing detail or touching the right bust. */
    for(i=0;i<P_SLOTS;++i)if(book.slots[i].occupied)sync_slot_portrait(ui.rows[i].face);
    if(full){ui.last_refresh=api.GetTickCount64();context_changed();}
    if(full && !refresh_eligibility(ui.state)) {
        for(i=0;i<6001;++i)availability[i]=N_INVALID;
    }
    if(full)for(i=0;i<P_SLOTS;++i) {
        Row *r=&ui.rows[i];slot=&book.slots[i];p=slot->occupied?person(slot->selection.officer_id):0;
        at=append_number(text,0,160,(U32)i);
        caption(r->button,text);
        short_text(text,p?person_name(p):slot->occupied?slot->name:L"빈 슬롯",slot->occupied?24:22,NAME_W);
        font_rect(row_names[i].r,NAME_X,slot->occupied?5:0,NAME_W,
                  slot->occupied?26:INT(r->button,OF(414)),slot->occupied?24:22,3);
        if(slot->occupied && selection_font_ready) {
            if(selection_font(row_names[i].r,0))row_names[i].valid=0;
            copy(row_names[i].colors,selection_font_style,16);
        }
        line_caption(&row_names[i],slot->occupied && selection_font_ready?0:r->button,text);
        at=0;text[0]=0;
        reason=N_INVALID;
        if(slot->occupied) {
            reason=make_context(ui.state,slot->selection.officer_id,&ctx);
            /* A selected existing row returns troops to the GROUP budget.
             * Do not classify the last officer as invalid merely because
             * other selected rows currently occupy the troops. */
            if(reason==N_OK) {
                ctx.max_troops=p?FN(0x2b1d00,I1)(p):0;
                if(ctx.max_troops<=0 || !PresetResolve(&book,i,&ctx,&plan) || !plan.applicable)
                    reason=N_INVALID;
            }
            at=append_number(text,at,160,(U32)slot->selection.troops);
        }
        r->reason=reason;line_caption(&row_troops[i],r->button,text);
        /* Native disabled material for the WHOLE occupied row. Empty rows
         * retain the normal light material and their explicit empty caption.
         * Eligibility and the selection state machine are unchanged. */
        /* Deployment eligibility is separate from clicking a stored record
         * for detail/deletion. Native disabled visuals still come from row_tone. */
        enable(r->button,1);
        set_slot_face(r->face,p);
        f=slot->occupied?formation(slot->selection.formation_id):0;
        set_formation(r->formation,f);
        for(k=0;k<3;++k) {
            if(slot->occupied)set_tactic(r->tactics[k],slot->selection.tactic_id[k],reason==N_OK);
            else visible(r->tactics[k],0);
        }
    }
    slot=&book.slots[ui.selected];exists=slot->occupied!=0;
    p=exists?person(slot->selection.officer_id):0;
    if(full) {
        at=0;
        at=append(text,at,160,p?person_name(p):exists?slot->name:L"빈 슬롯");
        short_text(text,text,28,DETAIL_W-12);
        caption(ui.detail_name,text);set_face(ui.detail_face,p);
        f=exists?formation(slot->selection.formation_id):0;
        set_formation(ui.detail_formation,f);
        at=append(text,0,160,L"병력: ");
        if(exists)at=append_number(text,at,160,(U32)slot->selection.troops);
        else at=append(text,at,160,L"—");
        caption(ui.detail_troops,text);
        reason=exists?make_context(ui.state,slot->selection.officer_id,&ctx):N_INVALID;
        for(k=0;k<3;++k) {
            if(exists)set_tactic(ui.detail_tactics[k],slot->selection.tactic_id[k],reason==N_OK);
            else visible(ui.detail_tactics[k],0);
        }
        refresh_selection();
        enable(ui.overwrite,exists && storage_writable);
        enable(ui.save,storage_writable && !save_batch.pending && !save_batch.busy);
    }
    /* Keep all NINE physical/native-filtered slots and their tooltip IDs.
     * Empty slots retain their grid positions. Assign labels only on changes.
     */
    for(i=0;i<9;++i) {
        id=p && i<(I32)native_limits.trait_slots?FN(0x22fb00,I2)(p,i):0;t=trait(id);
        if(t && id!=ui.trait_ids[i]) {
            FN(0x856bc0,void (*)(void *,I32,I32))(ui.labels[i],t[OF(417)],1);
            FN(0x85c270,TEXT)(ui.labels[i],(const U16 *)(t+OF(416)));
            INT(ui.labels[i],OF(415))=id;
        }
        ui.trait_ids[i]=t?id:0;visible(ui.labels[i],t!=0);
    }
    if(full)refresh_groups();
    if(delete_pending_mask)delete_notice();
    else if(api.GetTickCount64()<notice_until)status_caption(notice);
    else if(!storage_writable)status_caption(L"저장 불가 · 프리셋 파일 확인 필요");
    else status_caption(L"");
}
static I32 hit(U8 *w,const I32 *xy) {
    U8 *p=w;I32 x=0,y=0,n=0;
    if(!w || INT(w,OF(418))<=0 || INT(w,OF(419))<=0)return 0;
    while(p && n++<30) {
        if((INT(p,OF(420))&3)!=3)return 0;
        x+=INT(p,OF(421));y+=INT(p,OF(422));p=PTR(p,OF(423));
    }
    return !p && xy[0]>=x && xy[0]-x<INT(w,OF(424)) && xy[1]>=y && xy[1]-y<INT(w,OF(425));
}
static NI void selection_input_init(void) {
    HANDLE u,k;
    if(selection_api_ready)return;
    zero(&selection_api,sizeof(selection_api));
    if(!win.GetModuleHandleW || !win.GetProcAddress)return;
    u=win.GetModuleHandleW(L"user32.dll");k=win.GetModuleHandleW(L"kernel32.dll");
    if(!u || !k)return;
#define SEL_API(module,name) selection_api.name=(__typeof__(selection_api.name))win.GetProcAddress(module,#name)
    SEL_API(u,GetAsyncKeyState);SEL_API(u,GetForegroundWindow);
    SEL_API(u,GetWindowThreadProcessId);SEL_API(k,GetCurrentProcessId);
#undef SEL_API
    selection_api_ready=selection_api.GetAsyncKeyState && selection_api.GetForegroundWindow &&
        selection_api.GetWindowThreadProcessId && selection_api.GetCurrentProcessId;
}
static void cancel_selection_gesture(void) {
    zero(&selection_gesture,sizeof(selection_gesture));
    selection_gesture.pressed_slot=-1;
    if(selection_api_ready)selection_gesture.left_down=selection_api.GetAsyncKeyState(1)<0;
    /* Waiting for a held opening/focus-restoration button only prevents a
     * NEW raw row gesture. It must not suppress valid native callbacks. */
    selection_gesture.suppress_release=0;
    selection_gesture.wait_for_release=selection_gesture.left_down;
    isDraggingSelection=0;dragVisitedSlots=0;
}
static I32 selection_row_at(const I32 *xy) {
    I32 i;for(i=0;i<P_SLOTS;++i)if(hit(ui.rows[i].button,xy))return i;
    return -1;
}
static I32 selection_rect(U8 *w,I32 *rect) {
    U8 *p=w;I32 n=0;
    if(!w)return 0;
    rect[0]=rect[1]=0;rect[2]=INT(w,OF(426));rect[3]=INT(w,OF(427));
    if(rect[2]<=0 || rect[3]<=0)return 0;
    while(p && n++<30) {
        if((INT(p,OF(428))&3)!=3)return 0;
        rect[0]+=INT(p,OF(429));rect[1]+=INT(p,OF(430));p=PTR(p,OF(431));
    }
    return p==0;
}
static I32 selection_detail_at(I32 slot,const I32 *xy) {
    I32 rect[4],x,y,k;
    if(slot<0 || slot>=P_SLOTS || !selection_rect(ui.rows[slot].button,rect))return 0;
    x=xy[0]-rect[0];y=xy[1]-rect[1];
    /* Number/portrait open detail. Name/troops/gaps toggle checks. The
     * 40x44 check hit area exceeds its 36x40 glyph rectangle. */
    if(x>=0 && x<CHECK_HIT_W && y>=18)return 0;
    if(x>=0 && x<CHECK_HIT_W && y<18)return 1;
    if(x>=FACE_X && x<FACE_X+FACE_SIZE && y>=0 && y<ROW_H)return 1;
    if(x>=FORMATION_X && x<FORMATION_X+60 && y>=5 && y<57)return 2;
    for(k=0;k<3;++k)if(x>=TACTIC_X+k*TACTIC_STEP && x<TACTIC_X+k*TACTIC_STEP+TACTIC_W &&
                          y>=9 && y<53)return 2;
    return 3; /* Body: check toggle plus detail on click, never during drag. */
}
static NI void selection_click(I32 slot,I32 shift,I32 detail) {
    I32 lo,hi,i;
    if(slot<0 || slot>=P_SLOTS)return;
    if(delete_pending_mask)cancel_delete();
    cancel_reset();notice_until=0;
    if(shift) {
        if(!can_select_slot(slot))return;
        lo=can_select_slot(lastSelectedSlotIndex)?lastSelectedSlotIndex:slot;hi=slot;
        if(lo>hi){i=lo;lo=hi;hi=i;}
        for(i=lo;i<=hi;++i)if(can_select_slot(i))selected_mask|=1u<<i;
        if(lastSelectedSlotIndex<0)lastSelectedSlotIndex=slot;
    } else if(detail==1 || detail==2 || !book.slots[slot].occupied) {
        ui.selected=slot;
        if(book.slots[slot].occupied)ui.mode_value=book.slots[slot].troop_mode;
        refresh_panel(1);return;
    } else if(can_select_slot(slot)) {
        selected_mask^=1u<<slot;lastSelectedSlotIndex=slot;
    }
    if(detail==3) {
        ui.selected=slot;ui.mode_value=book.slots[slot].troop_mode;
        refresh_panel(1);return;
    }
    refresh_selection();
}
static void selection_visit(I32 slot) {
    U32 bit;
    if(slot<0 || slot>=P_SLOTS)return;
    bit=1u<<slot;if(dragVisitedSlots&bit)return;
    dragVisitedSlots|=bit;
    if(can_select_slot(slot))selected_mask|=bit;
}
static I32 selection_segment_hits(I32 slot,const I32 *a,const I32 *b) {
    I32 rect[4],i;float lo=0,hi=1,start,delta,p,q,tmp;
    if(!selection_rect(ui.rows[slot].button,rect))return 0;
    /* Intersect the pointer's line segment, not a rectangle selection box.
     * This includes rows crossed between two UI ticks in either direction. */
    for(i=0;i<2;++i) {
        start=(float)a[i];delta=(float)b[i]-start;
        if(delta==0) {if(start<rect[i] || start>=rect[i]+rect[i+2])return 0;continue;}
        p=((float)rect[i]-start)/delta;q=((float)(rect[i]+rect[i+2])-start)/delta;
        if(p>q){tmp=p;p=q;q=tmp;}
        if(p>lo)lo=p;
        if(q<hi)hi=q;
        if(lo>hi)return 0;
    }
    return 1;
}
static NI I32 selection_pointer_step(const I32 *xy,I32 down,I32 shift,I32 own_target) {
    I32 row=own_target?selection_row_at(xy):-1,i,was_down=selection_gesture.left_down;
    I32 dx,dy,consumed=0;
    selection_gesture.left_down=down;
    if(selection_gesture.wait_for_release) {
        if(!down)selection_gesture.wait_for_release=0;
        return 0;
    }
    if(down && !was_down) {
        /* A new press starts a new gesture, including presses on toolbars.
         * Never carry a stale row owner into an unrelated button click. */
        selection_gesture.pressed_slot=-1;selection_gesture.have_previous=0;
        isDraggingSelection=0;dragVisitedSlots=0;
        selection_gesture.suppress_release=0;
        if(row<0)return 0; /* Scrollbar/other-widget presses never arm selection. */
        if(selection_detail_at(row,xy)==2)return 0; /* Native formation/tactic action. */
        /* A fresh row gesture cancels a warning BEFORE it can change checks.
         * Never keep polling blocked merely because deletion is pending. */
        if(delete_pending_mask)cancel_delete();
        refresh_panel(1);cancel_reset();notice_until=0;
        selection_gesture.pressed_slot=row;selection_gesture.shift=shift;
        selection_gesture.detail=selection_detail_at(row,xy);
        copy(selection_gesture.start,xy,8);copy(selection_gesture.previous,xy,8);
        selection_gesture.have_previous=1;dragVisitedSlots=0;isDraggingSelection=0;
        return 1; /* Defer toggle/detail until release, so drag never toggles. */
    }
    if(selection_gesture.pressed_slot>=0) {
        consumed=1;
        dx=xy[0]-selection_gesture.start[0];dy=xy[1]-selection_gesture.start[1];
        if(down && !isDraggingSelection && (dx>=4 || dx<=-4 || dy>=4 || dy<=-4 ||
             (row>=0 && row!=selection_gesture.pressed_slot))) {
            isDraggingSelection=1;
            if(selection_gesture.shift)selection_click(selection_gesture.pressed_slot,1,0);
            else if(can_select_slot(selection_gesture.pressed_slot))
                lastSelectedSlotIndex=selection_gesture.pressed_slot;
            selection_visit(selection_gesture.pressed_slot);
        }
        if(isDraggingSelection) {
            input_drag_consumed=1;
            if(own_target) {
                if(selection_gesture.have_previous)for(i=0;i<P_SLOTS;++i)
                    if(selection_segment_hits(i,selection_gesture.previous,xy))selection_visit(i);
                selection_visit(row);
            }
            /* Leaving the panel breaks interpolation, preventing a sweep
             * through rows while the actual pointer is outside the list. */
            selection_gesture.have_previous=own_target;
            copy(selection_gesture.previous,xy,8);refresh_selection();
        }
        if(!down) {
            if(!isDraggingSelection && row==selection_gesture.pressed_slot)
                selection_click(row,selection_gesture.shift,selection_gesture.detail);
            selection_gesture.pressed_slot=-1;selection_gesture.have_previous=0;
            /* Remember a true drag for exactly its extra native release tick.
             * A fresh MouseDown clears this before arming another gesture. */
            selection_gesture.suppress_release=isDraggingSelection?2:1;
            isDraggingSelection=0;dragVisitedSlots=0;
        }
    } else if(!down && selection_gesture.suppress_release) {
        /* Also swallow a native release callback delivered on the next tick.
         * A completed drag cannot activate a toolbar under its release point. */
        if(selection_gesture.suppress_release==2)input_drag_consumed=1;
        selection_gesture.suppress_release=0;consumed=1;
    }
    return consumed;
}
static NI I32 update_selection_input(void) {
    U8 *app,*input,*target,*p;HANDLE foreground;U32 process=0;I32 down,shift,i,n,owned=0;
    input_row_consumed=input_drag_consumed=0;
    if(!selection_api_ready || !ui.shown || ui.rename_waiting)return 0;
    down=selection_api.GetAsyncKeyState(1)<0;shift=selection_api.GetAsyncKeyState(0x10)<0;
    foreground=selection_api.GetForegroundWindow();
    if(foreground)selection_api.GetWindowThreadProcessId(foreground,&process);
    if(!foreground || !process || process!=selection_api.GetCurrentProcessId()) {
        row_hover(-1);cancel_selection_gesture();selection_gesture.wait_for_release=1;return 0;
    }
    app=FN(0x10e00,U8 *(*)(void))();input=FN(0x110d0,U8 *(*)(void))();
    if(!app || !input){cancel_selection_gesture();selection_gesture.wait_for_release=1;return 0;}
    target=FN(0x86ac90,GET1)(app);
    for(p=target,n=0;p && n++<30 && !owned;p=PTR(p,OF(432)))
        for(i=1;i<ui.count;++i)if(ui.owned[i].active && p==ui.owned[i].w){owned=1;break;}
    row_hover(owned?selection_row_at((const I32 *)(input+OF(433))):-1);
    /* Use the game's existing logical cursor coordinates. No HWND subclass,
     * mouse capture, wheel interception, new hook or screen-pixel conversion. */
    input_row_consumed=selection_pointer_step((const I32 *)(input+OF(435)),down,shift,owned);
    return input_row_consumed;
}
static NI19 I32 update_tactic_tooltip(U8 *target,const I32 *xy) {
    U8 *w=0,*t;I32 i,k,id=0;
    if(!ui.shown || !xy || !ui.state || delete_pending_mask || isDraggingSelection ||
       selection_gesture.pressed_slot>=0 || !hit(ui.panel,xy)) {
        hide_tactic_tooltip();return 0;
    }
    for(i=0;i<P_SLOTS && !w;++i)if(book.slots[i].occupied) {
        for(k=0;k<3;++k)if(hit(ui.rows[i].tactics[k],xy)) {
            w=ui.rows[i].tactics[k];id=book.slots[i].selection.tactic_id[k];break;
        }
    }
    if(!w && ui.selected>=0 && ui.selected<P_SLOTS && book.slots[ui.selected].occupied) {
        for(k=0;k<3;++k)if(hit(ui.detail_tactics[k],xy)) {
            w=ui.detail_tactics[k];id=book.slots[ui.selected].selection.tactic_id[k];break;
        }
    }
    t=w?tactic(id):0;
    if(!t || !tactic_tip || !tactic_tip_ready || !tactic_tip_manager) {
        hide_tactic_tooltip();return 0;
    }
    visible(ui.tip,0);last_tip_text=0;last_tip_target=0;
    if(target!=tactic_hover_alias || w!=last_tactic_target || id!=last_tactic_id ||
       PTR(tactic_tip,OF(434))!=t) {
        hide_tactic_tooltip();
        last_tactic_target=w;last_tactic_id=id;
        /* Passive detail text/disabled buttons can resolve to the owning row
         * or panel. Register that REAL hit target only for this validated
         * tactic rectangle. Clear it immediately on Leave; never change the
         * row's hit policy, callback, selection state or native tactic color. */
        if(target) {
            FN(0x89ab40,VOID2)(tactic_tip_manager,target);
            tactic_hover_alias=target;
        }
        trace_ui("tactic.native.target",id,target==w,target!=0);
    }
    if(target)FN(0x89fcd0,VOID1)(tactic_tip_manager);
    else {
        /* No native hit target during an input transition: no phantom popup. */
        hide_tactic_tooltip();return 0;
    }
    if(PTR(tactic_tip,OF(436))!=t || INT(tactic_tip,OF(437))<=0 || INT(tactic_tip,OF(438))>1920 ||
       INT(tactic_tip,OF(439))<=0 || INT(tactic_tip,OF(440))>1080) {
        hide_tactic_tooltip();return 0;
    }
    tactic_tip_using=1;return 1;
}
static NI void update_tooltip(void) {
    U8 *app,*input,*target,*t;const U16 *text=0;U8 manager[64];I32 i;
    if(!ui.shown || !ui.tip){hide_tactic_tooltip();return;}
    if(delete_pending_mask || save_batch.pending){visible(ui.tip,0);hide_tactic_tooltip();return;}
    if(isDraggingSelection || selection_gesture.pressed_slot>=0){visible(ui.tip,0);hide_tactic_tooltip();return;}
    app=FN(0x10e00,U8 *(*)(void))();input=FN(0x110d0,U8 *(*)(void))();
    target=app?FN(0x86ac90,GET1)(app):0;
    if(update_tactic_tooltip(target,input?(const I32 *)(input+OF(444)):0))return;
    if(input && target) {
        for(i=0;i<9;++i)if(target==ui.labels[i] && hit(ui.labels[i],(I32 *)(input+OF(445)))) {
            t=trait(ui.trait_ids[i]);if(t)text=FN(0x358420,const U16 *(*)(void *))(t);break;
        }
        if(!text)for(i=0;i<P_SLOTS;++i)if(target==ui.rows[i].button && hit(target,(I32 *)(input+OF(446)))) {
            if(book.slots[i].occupied)text=reason_text(ui.rows[i].reason);
            break;
        }
    }
    if(!text || !*text){visible(ui.tip,0);return;}
    if(text!=last_tip_text || target!=last_tip_target) {
        FN(0x8a1c50,TEXT)(ui.tip,text);last_tip_text=text;last_tip_target=target;
    }
    zero(manager,sizeof(manager));
    PTR(manager,OF(441))=ui.tip;INT(manager,OF(442))=20;INT(manager,OF(443))=20;
    FN(0x8a5810,VOID1)(manager);visible(ui.tip,1);
}
#include "save_batch.h"
static NI void load_selected(void) {
    P_Context ctx;P_LoadPlan plan;I32 id;U16 msg[160];U32 at;
    if(!book.slots[ui.selected].occupied)return;
    if(!commit_current(ui.state) || !refresh_eligibility(ui.state)) {
        notify_text(L"현재 출진 설정을 확인할 수 없습니다.");return;
    }
    id=book.slots[ui.selected].selection.officer_id;
    make_context(ui.state,id,&ctx);
    if(!ctx.available){notify_text(reason_text(ctx.reason));return;}
    if(!PresetResolve(&book,ui.selected,&ctx,&plan) || !plan.applicable) {
        notify_text(L"사용할 수 있는 진형이 없습니다.");return;
    }
    if(!apply_plan(ui.state,&plan)){notify_text(L"불러오지 못했습니다. 현재 출진 설정을 확인하세요.");return;}
    at=append(msg,0,160,L"불러오기 완료");
    if(plan.adjustments&P_TROOPS_ADJUSTED) {
        at=append(msg,at,160,L" · 병력 ");at=append_number(msg,at,160,(U32)plan.selection.troops);
    }
    if(plan.adjustments&P_FORMATION_REPLACED)at=append(msg,at,160,L" · 진형 대체");
    if(plan.adjustments&28)at=append(msg,at,160,L" · 불가 전법만 비움");
    notify_text(msg);
}
static void cancel_reset(void) { reset_slot=-1; }
static NI void delete_checked(U32 mask) {
    I32 i,count=0;U16 text[80];U32 at;mask&=(1u<<P_SLOTS)-1;
    if(!ui.shown || !mask || !storage_writable)return;
    /* One existing atomic transaction. Only checked slot records are cleared;
     * disk failure leaves both the live book and the selection untouched. */
    copy(&transaction,&book,sizeof(book));
    for(i=0;i<P_SLOTS;++i)if((mask&(1u<<i)) && book.slots[i].occupied) {
        zero(&transaction.slots[i],sizeof(transaction.slots[i]));++count;
    }
    if(!count || !persist_book())return;
    selected_mask&=~mask;batch_mask&=~mask;unavailable_checks&=~mask;
    at=append(text,0,80,L"선택한 프리셋 ");at=append_number(text,at,80,(U32)count);
    append(text,at,80,L"개를 삭제했습니다.");notify_text(text);
}
static void cancel_delete(void) {
    delete_pending_mask=delete_unavailable_mask=0;
    delete_screen_generation=0;++delete_epoch;
    /* Only visibility changes. The original normal button captions, actions,
     * coordinates and right-side portrait code are never rebound or resized. */
    visible(delete_yes,0);visible(delete_no,0);
    visible(ui.load,ui.shown);visible(clear_button,ui.shown);
}
static NI I32 ensure_delete_dialog(void) {
    /* Historical function name retained for development tooling. It DOES
     * NOT construct a dialog. All resources were created with the panel. */
    return ui.panel && ui.load && clear_button && delete_yes && delete_no &&
        notice_strip && notice_first.r && notice_second.r &&
        PTR(delete_yes,OF(447))==ui.panel && PTR(delete_no,OF(448))==ui.panel &&
        INT(delete_yes,OF(449))>0 && INT(delete_yes,OF(450))>0 &&
        INT(delete_no,OF(451))>0 && INT(delete_no,OF(452))>0;
}
static NI void delete_notice(void) {
    U16 text[80];U32 at;I32 i,count=0;
    for(i=0;i<P_SLOTS;++i)if(delete_pending_mask&(1u<<i))++count;
    /* Fit the existing two-line, 26-unit status caption WITHOUT truncating
     * either the unavailable warning or the irreversible-delete message. */
    at=append(text,0,80,delete_unavailable_mask?
        L"출진 불가 프리셋 포함\n":L"선택한 프리셋 삭제\n");
    at=append_number(text,at,80,(U32)count);
    append(text,at,80,L"개 삭제? 복구 불가");
    status_caption(text);
}
static NI void present_delete_confirmation(void) {
    if(!delete_pending_mask)return;
    visible(ui.load,0);visible(clear_button,0);
    visible(delete_yes,ui.shown);visible(delete_no,ui.shown);
    enable(delete_yes,storage_writable);enable(delete_no,1);
    visible(ui.tip,0);
}
static NI void request_delete(void) {
    U32 mask;I32 rect[4];
    if(!ui.shown || !storage_writable)return;
    refresh_panel(1);mask=checked_slots();if(!mask)return;
    if(!unavailable_checks){delete_checked(mask);return;}
    if(!ensure_delete_dialog()) {
        cancel_delete();notify_text(L"삭제 확인 표시 실패 · 삭제하지 않았습니다.");return;
    }
    copy(&delete_snapshot,&book,sizeof(book));
    cancel_selection_gesture();
    ++delete_epoch;delete_screen_generation=ui.generation;
    delete_unavailable_mask=unavailable_checks;delete_pending_mask=mask;
    present_delete_confirmation();delete_notice();
    /* These checks are only a lifecycle/geometry guard, NOT proof of GPU
     * visibility. Even if native rendering fails, row/toolbar/entry actions
     * remain accepted and cancel this pending request instead of locking. */
    if(!selection_rect(delete_yes,rect) || !selection_rect(delete_no,rect)) {
        cancel_delete();notify_text(L"삭제 확인 표시 실패 · 삭제하지 않았습니다.");
    }
}
static NI I32 delete_snapshot_matches(void) {
    I32 i;
    if(delete_screen_generation!=ui.generation ||
       checked_slots()!=delete_pending_mask)return 0;
    for(i=0;i<P_SLOTS;++i)if(delete_pending_mask&(1u<<i))
        if(!equal(&book.slots[i],&delete_snapshot.slots[i],sizeof(P_Preset)))return 0;
    return 1;
}
static NI void load_batch(void) {
    I32 result,applied=0;U16 msg[160];U32 at;NativeBatch *b=&deployment_batch;
    if(!batch_mode || !batch_mask)return;
    if(!commit_current(ui.state)) {notify_text(L"현재 출진 설정을 확인할 수 없습니다.");return;}
    result=prepare_batch(ui.state,&book,checked_slots(),b);
    if(result==2) {notify_text(L"전체 부대에 나눌 병력이 부족합니다. 적용하지 않았습니다.");return;}
    if(!result) {notify_text(L"일괄 적용 전 설정 확인 실패 · 적용하지 않았습니다.");return;}
    if(b->count && !apply_batch(ui.state,b)) {
        notify_text(L"일괄 적용 준비 실패 · 부대를 변경하지 않았습니다.");return;
    }
    applied=b->count;
    trace_ui("batch.equal.applied",applied,b->pool,b->leftover);
    at=append(msg,0,160,L"적용 ");at=append_number(msg,at,160,(U32)applied);
    at=append(msg,at,160,L" · 중복 ");at=append_number(msg,at,160,(U32)b->duplicate);
    at=append(msg,at,160,L" · 불가 ");append_number(msg,at,160,(U32)b->unavailable);
    notify_text(msg);
}
/* RC16: this dispatcher is intentionally independent of raw mouse polling.
 * A release/focus guard may suppress a duplicate ROW callback, but never
 * silently eat a queued Select All/Clear/Delete/Load/Entry command. */
static NI void dispatch_action(I32 action,I32 detail,I32 shift) {
    I32 slot;U32 deleting;
    if(!ui.state)return;
    if(action!=A_OPEN && action!=A_CLOSE && !ui.shown)return;
    ++input_diagnostics.dispatched;input_diagnostics.last_action=action;
    if(action==A_SAVE_CONFIRM || action==A_SAVE_CANCEL) {
        if(!save_batch.pending || (U32)detail!=save_batch.epoch)return;
        if(action==A_SAVE_CONFIRM)save_confirm(1);
        else {cancel_save();notify_text(L"저장을 취소했습니다.");}
        cancel_selection_gesture();if(ui.shown)refresh_panel(1);
        trace_input(action);return;
    }
    if(save_batch.pending || save_batch.busy) {
        /* Close is always available, even if the GPU cannot display the prompt.
         * Other group/data actions cannot retarget a delayed confirmation. */
        if(action==A_OPEN || action==A_CLOSE)cancel_save();
        else return;
    }
    if(action==A_DELETE_CONFIRM || action==A_DELETE_CANCEL) {
        if(!delete_pending_mask || (U32)detail!=delete_epoch)return;
        if(action==A_DELETE_CONFIRM) {
            if(!ensure_delete_dialog() || !delete_snapshot_matches()) {
                cancel_delete();notify_text(L"삭제 대상이 바뀌었습니다. 다시 선택해 주세요.");
            } else {
                deleting=delete_pending_mask;cancel_delete();delete_checked(deleting);
            }
        } else {cancel_delete();notify_text(L"삭제를 취소했습니다.");}
        cancel_selection_gesture();if(ui.shown)refresh_panel(1);
        trace_input(action);return;
    }
    /* Fail open for UI input, fail closed for data: another ordinary action
     * cancels deletion and still runs. No invisible-modal early return. */
    if(delete_pending_mask) {
        cancel_delete();notice_until=0;
        /* A delayed Load callback from the now-hidden normal button must
         * never become deletion confirmation or deploy unexpectedly. */
        if(action==A_LOAD) {refresh_panel(1);trace_input(action);return;}
    }
    if(action>=A_ROW && action<A_ROW+P_SLOTS) {
        refresh_panel(1);selection_click(action-A_ROW,shift,detail);trace_input(action);return;
    }
    /* Toolbar callbacks delimit a separate gesture. Reset any stale row
     * ownership, without discarding the command that caused this reset. */
    cancel_selection_gesture();cancel_reset();
    if(action==A_CLEAR) {
        selected_mask=batch_mask=unavailable_checks=0;lastSelectedSlotIndex=-1;
        refresh_selection();trace_input(action);return;
    }
    if(action==A_SELECT_ALL) {
        if(checked_slots())selected_mask=batch_mask=unavailable_checks=0;
        else {
            refresh_panel(1);
            for(slot=0;slot<P_SLOTS;++slot)if(can_select_slot(slot))selected_mask|=1u<<slot;
        }
        lastSelectedSlotIndex=-1;refresh_selection();trace_input(action);return;
    }
    if(action>=A_GROUP && action<A_GROUP+G_COUNT){switch_group(action-A_GROUP);return;}
    if(action>=A_GROUP_RENAME && action<A_GROUP_RENAME+G_COUNT){group_rename_begin(action-A_GROUP_RENAME);return;}
    switch(action) {
    case A_OPEN:if(ui.shown){ui.want_open=0;close_panel();}else show_panel();trace_input(action);return;
    case A_CLOSE:ui.want_open=0;close_panel();break;
    case A_SAVE:save_current(0);break;
    case A_LOAD:
        refresh_panel(1);
        if(checked_slots()){if(batch_mask)load_batch();}
        else if(can_apply_slot(ui.selected))load_selected();
        break;
    case A_OVERWRITE:return; /* Retired: batch Save NEVER overwrites. */
    case A_DELETE:request_delete();break;
    default:return;
    }
    if(ui.shown)refresh_panel(1);
    trace_input(action);
}
static NI void actions(void) {
    I32 polled=0,remaining,action,detail=1,shift=0,slot;U8 *input;
    PresetAction event;
    /* Retain compatibility with the single pending field, but native
     * callbacks now use the FIFO. Move it before polling; never abandon it. */
    if((action=ui.pending)!=0) {
        ui.pending=0;
        if(action==A_RENAMED){group_rename_finish();return;}
        if(action>=A_ROW && action<A_ROW+P_SLOTS) {
            slot=action-A_ROW;input=FN(0x110d0,U8 *(*)(void))();
            if(input && selection_row_at((const I32 *)(input+OF(453)))==slot)
                detail=selection_detail_at(slot,(const I32 *)(input+OF(454)));
            if(selection_api_ready)shift=selection_api.GetAsyncKeyState(0x10)<0;
            if(detail==2)shift=0;
        }
        queue_action(action,detail,shift);
    }
    input_row_consumed=input_drag_consumed=0;
    /* No deletion state may disable pointer polling or eat other commands. */
    if(delete_pending_mask && (!ensure_delete_dialog() || !delete_snapshot_matches())) {
        cancel_delete();notify_text(L"삭제 확인을 취소했습니다. 다시 선택해 주세요.");
        if(ui.shown)refresh_selection();
    }
    polled=save_batch.pending?0:update_selection_input();
    remaining=action_count; /* Events created during dispatch wait until next tick. */
    while(remaining-- && action_count) {
        event=action_queue[action_head];action_head=(action_head+1)%ACTION_QUEUE_CAPACITY;--action_count;
        action=event.action;
        if(action>=A_ROW && action<A_ROW+P_SLOTS && polled) {
            ++input_diagnostics.row_duplicates;continue;
        }
        /* A real row drag crossing a toolbar must never load/delete. This
         * applies only to that drag tick, not wait_for_release or a stale
         * suppress_release flag. A subsequent toolbar press always works. */
        if(input_drag_consumed) {
            ++input_diagnostics.drag_blocks;continue;
        }
        dispatch_action(action,event.detail,event.shift);
        if(action==A_OPEN || action==A_CLOSE || action==A_SAVE || save_batch.pending || delete_pending_mask || ui.rename_waiting ||
           (action>=A_GROUP && action<A_GROUP+G_COUNT)) {
            clear_action_queue();break;
        }
    }
}
