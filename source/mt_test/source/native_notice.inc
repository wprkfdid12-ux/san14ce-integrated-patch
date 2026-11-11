/* R7: the same parentless CSimpleBalloonLayout construction/resize sequence
 * used by the supplied RC25 native panel. A common button is NOT a dialog:
 * its caption is single-line and its parent can clip/hide it. Text lines and
 * Yes/No are separately owned native roots, positioned with this window. */
#define NOTICE_LINES 6
#define NOTICE_WIDTH 780
#define NOTICE_HEIGHT 356
static u8 *notice_text[2][NOTICE_LINES],*notice_backdrop[2];
static u16 notice_lines[2][NOTICE_LINES][128];
static u8 notice_initialized[2];
static void notice_reset(void){mc_zero(notice_text,sizeof(notice_text));mc_zero(notice_backdrop,sizeof(notice_backdrop));mc_zero(notice_lines,sizeof(notice_lines));mc_zero(notice_initialized,sizeof(notice_initialized));}
static int notice_resource_room(u8*w){u8*ctx;
 if(!rd(w,0xa0)||(ctx=P(w,0x98))==0||!rd(ctx,0x18)||(ctx=P(ctx,0x10))==0||!rd(ctx+0x18,8))return 0;
 ctx+=0x18;usize n=V(ctx,0x18,usize(*)(void*))(ctx);if(n>4096)return 0;
 V(ctx,0x60,void(*)(void*,usize))(ctx,n+1);return 1;
}
static int notice_ensure(u32 index){u32 k=index==11?1:0;u8*p=controls[index],*template=controls[0],*r;int h;
 if(!rd(template,0x198))return 0;h=(int)D(template,0x2c);if(h<32||h>64)return 0;
 if(!p){
  p=NCALL(N_ALLOC,u8*(*)(usize))(res->popup.object_size);if(!p)return 0;
  PCALL(P_LAYOUT_CTOR,void*(*)(void*))(p);controls[index]=p;
  PCALL(P_COLLECTION_CTOR,Void1)(p+0x130);P(p,0)=game+res->popup.vtable;P(p,0x178)=0;
  PCALL(P_BALLOON_INIT,int(*)(void*,void*))(p,0);
  /* Base ctor/real layout destructor remain valid even if resource init fails. */
  if(!rd(p,0x180)||!P(p,0x138)||D(p,0x140)<4)return 0;
  D(p,0x40)&=~8u;PCALL(P_BALLOON_ALPHA,Flag)(p,255);set_visible(p,0);V(p,0x70,Flag)(p,6);
  D(p,0x28)=NOTICE_WIDTH;D(p,0x2c)=NOTICE_HEIGHT;
  PCALL(P_COLLECTION_WIDTH,void(*)(void*,int,int))(p+0x130,2,NOTICE_WIDTH);
  PCALL(P_COLLECTION_HEIGHT,void(*)(void*,int,int))(p+0x130,2,NOTICE_HEIGHT);
  if(!rd(P(p,0x138),32)||(r=P(P(p,0x138),8))==0)return 0;
  NCALL(N_PRIM_SIZE,void(*)(void*,int,int))(r,NOTICE_WIDTH,NOTICE_HEIGHT);
  PCALL(P_COLLECTION_VISIBLE,void(*)(void*,int,int))(p+0x130,3,0);
  PCALL(P_COLLECTION_VISIBLE,void(*)(void*,int,int))(p+0x130,2,0);
  notice_initialized[k]=1;
 }
 if(!notice_initialized[k])return 0;
 if(!notice_backdrop[k]){
  notice_backdrop[k]=PCALL(P_WINDOW,u8*(*)(void*,int,int,int,int))(p,0,0,NOTICE_WIDTH,NOTICE_HEIGHT);
  if(!notice_backdrop[k])return 0;PCALL(P_PRIM_ALPHA,Flag)(notice_backdrop[k],245);
 }
 for(u32 i=0;i<NOTICE_LINES;i++)if(!notice_text[k][i]){
  if(!notice_resource_room(p))return 0;
  u8*t=NCALL(N_DYNAMIC_TEXT,u8*(*)(void*,int,int,int,int,const u16*))(p,0,0,NOTICE_WIDTH-64,38,L"");
  if(!t)return 0;notice_text[k][i]=t;
  /* Copy Selection's typeface/metrics, keep the native text's readable palette. */
  u8*style=V(t,0x80,u8*(*)(void*,int))(t,0),palette[16];int keep=rd(style,16);
  if(keep)mc_copy(palette,style,16);control_font(t,template,0,32,22+(int)i*38,NOTICE_WIDTH-64,38,4);
  if(keep)mc_copy(style,palette,16);
 }
 control_ready[index]=1;control_enabled[index]=1;
 if(index==11){
  if(!controls[12]&&!create_control_parent(12,NOTICE_WIDTH/2-176,NOTICE_HEIGHT-76,160,h,3,ACTION_EXIT_YES,template,0))return 0;
  if(!controls[13]&&!create_control_parent(13,NOTICE_WIDTH/2+16,NOTICE_HEIGHT-76,160,h,3,ACTION_EXIT_NO,template,0))return 0;
  if(!control_ready[12]||!control_ready[13])return 0;
  V(controls[12],0x70,Flag)(controls[12],6);V(controls[13],0x70,Flag)(controls[13],6);
 }else{
  if(!controls[10]&&!create_control_parent(10,NOTICE_WIDTH/2-80,NOTICE_HEIGHT-76,160,h,3,ACTION_NOTICE_CLOSE,template,0))return 0;
  if(!control_ready[10])return 0;V(controls[10],0x70,Flag)(controls[10],6);
 }
 return 1;
}
static void notice_caption(u32 index,const u16*s){u32 k=index==11?1:0,line=0,n=0;
 mc_zero(notice_lines[k],sizeof(notice_lines[k]));
 /* Explicit lines: CCmnButton ignores newline. No caption API on this layout. */
 for(u32 i=0;s[i]&&line<NOTICE_LINES;i++){
  if(s[i]=='\n'){line++;n=0;continue;}
  if(n<127)notice_lines[k][line][n++]=s[i];
 }
 for(u32 i=0;i<NOTICE_LINES;i++)if(notice_text[k][i])NCALL(N_PRIM_TEXT,Text)(notice_text[k][i],notice_lines[k][i]);
}
static int notice_position(u32 index){u8*p=controls[index],*a=controls_parent;long long x=0,y=0;u32 depth=0;
 if(!control_ready[index]||!rd(a,0x44))return 0;
 int width=(int)D(a,0x28),height=(int)D(a,0x2c);if(width<960||height<500)return 0;
 while(a&&depth++<30){if(!rd(a,0x44)||!(D(a,0x40)&1))return 0;x+=(int)D(a,0x20);y+=(int)D(a,0x24);a=P(a,8);}
 if(a||x<0||y<0||x>8192||y>8192)return 0;
 int px=(int)x+(width-NOTICE_WIDTH)/2,py=(int)y+(height-NOTICE_HEIGHT)/2;
 V(p,0x78,void(*)(void*,int,int))(p,px,py);
 if(index==11){
  V(controls[12],0x78,void(*)(void*,int,int))(controls[12],px+NOTICE_WIDTH/2-176,py+NOTICE_HEIGHT-76);
  V(controls[13],0x78,void(*)(void*,int,int))(controls[13],px+NOTICE_WIDTH/2+16,py+NOTICE_HEIGHT-76);
 }else V(controls[10],0x78,void(*)(void*,int,int))(controls[10],px+NOTICE_WIDTH/2-80,py+NOTICE_HEIGHT-76);
 return 1;
}
static int notice_present(u32 index){u32 end=index==11?14:11;
 if(!notice_position(index))return 0;
 /* Root first, children after; inverse order on hide/destruction. */
 for(u32 i=index;i<end;i++){control_register(i,1);control_show(i,1);}
 return 1;
}
static void notice_hide(u32 index){u32 end=index==11?14:11;
 for(u32 i=end;i-->index;){control_show(i,0);control_register(i,0);}
}
static int notice_healthy(u32 index){u32 end=index==11?14:11;
 for(u32 i=index;i<end;i++){
  u8*p=controls[i];if(!p||!control_ready[i]||!control_active[i]||!control_shown[i]||!rd(p,0x44)||!(D(p,0x40)&1))return 0;
  if(i==index){if(P(p,8)||P(p,0)!=game+res->popup.vtable||D(p,0x28)!=NOTICE_WIDTH||D(p,0x2c)!=NOTICE_HEIGHT)return 0;}
  else {
   u8*root=controls[index];int dx=i==13?NOTICE_WIDTH/2+16:index==11?NOTICE_WIDTH/2-176:NOTICE_WIDTH/2-80;
   if(P(p,8)||D(p,0x20)!=D(root,0x20)+(u32)dx||D(p,0x24)!=D(root,0x24)+NOTICE_HEIGHT-76)return 0;
  }
 }
 return 1;
}
