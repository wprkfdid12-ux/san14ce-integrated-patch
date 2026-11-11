"""R2 native-widget fixture, with OS/game endpoints substituted."""
from test_native_integration import EN
from test_rc4 import *

class R2(EN):
 def __init__(self):
  super().__init__();self.widget_callbacks={};self.widget_inits=[];self.registered=set();self.widget_deletes=[];self.destroy_contexts=[];self.selected_upper={};self.rank_eligible=False;self.popup_inits=[];self.layers={};self.register_events=[];self.dynamic_owner={};self.alpha={};self.collection_ops=[]
  self.widget_vt=self.alloc(0x240)
  for off,name in {0:'WidgetDelete',0x28:'WidgetDestroy',0x38:'WidgetRegister',0x40:'WidgetUnregister',0x70:'WidgetLayer',0x78:'WidgetPosition',0xc8:'Visible',0xd0:'Enabled',0x1d8:'WidgetText'}.items():self.widget_vt.u64(off,BR.addr(name))
  self.game.put(self.res.popup.vtable,self.widget_vt.get(0,self.widget_vt.size))
  self.prim_vt=self.alloc(0x90);self.prim_vt.u64(0x80,BR.addr('TextParam'))
  self.context_vt=self.alloc(0x70);self.context_vt.u64(0x18,BR.addr('CollectionSize'));self.context_vt.u64(0x60,BR.addr('CollectionReserve'))
  self.adapter_enable={};self.resources=self.alloc(64)
  BR.callbacks.update({'WidgetAlloc':lambda n:self.alloc(n).addr,'WidgetFree':lambda p:None,'WidgetCtor':self.ctor,'WidgetInit':self.widget_init,'WidgetText':self.settext,'WidgetBind':self.bind,'CallbackType':lambda:0,'DynamicText':self.dynamic_text,'PrimText':self.settext,'PrimSize':lambda p,w,h:None,'PrimPos':lambda p,x,y:None,'Align':lambda p,a:None,'WidgetRegister':lambda p:self.registered.add(p),'WidgetUnregister':lambda p:self.registered.discard(p),'WidgetDestroy':lambda p,r:self.destroy_contexts.append((p,r)),'WidgetDelete':lambda p,f:self.widget_deletes.append(p),'WidgetLayer':lambda p,n:self.layers.__setitem__(p,n),'WidgetPosition':lambda p,x,y:(put(p+0x20,x & 0xffffffff,'<I'),put(p+0x24,y & 0xffffffff,'<I')) and 0,'Visible':self.visible,'Font':lambda p,n:None,'CollectionSize':lambda p:0,'CollectionReserve':lambda p,n:None,'TextParam':lambda p,n:p+0x200,'AIEligible':lambda p:int(self.rank_eligible),'ButtonEvent':self.select,'RowDestroy':lambda row,ctx:self.destroy_contexts.append((row,ctx)),'Enabled':lambda p,e:self.adapter_enable.__setitem__(p,e)})
  # The C dispatcher expects integer return values.
  BR.callbacks['WidgetRegister']=lambda p:self.register(p,1)
  BR.callbacks['WidgetUnregister']=lambda p:self.register(p,0)
  names={9:'AIEligible',11:'Font',15:'WidgetAlloc',16:'WidgetFree',17:'WidgetCtor',18:'WidgetInit',19:'WidgetText',20:'WidgetBind',21:'CallbackType',22:'DynamicText',23:'PrimText',24:'PrimSize',25:'PrimPos',26:'Align'}
  for idx,name in names.items():self.game.put(self.res.native.funcs[idx],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
  BR.callbacks.update({'LayoutCtor':self.ctor,'CollectionCtor':lambda p:0,'BalloonInit':self.balloon_init,'BalloonAlpha':lambda p,a:self.alpha.__setitem__(p,a),'CollectionWidth':lambda p,i,v:self.collection_ops.append(('width',p,i,v)),'CollectionHeight':lambda p,i,v:self.collection_ops.append(('height',p,i,v)),'CollectionVisible':lambda p,i,v:self.collection_ops.append(('visible',p,i,v)),'Window':self.window,'PrimAlpha':lambda p,a:self.alpha.__setitem__(p,a)})
  for idx,name in enumerate(['LayoutCtor','CollectionCtor','BalloonInit','BalloonAlpha','CollectionWidth','CollectionHeight','CollectionVisible','Window','PrimAlpha']):self.game.put(self.res.popup.funcs[idx],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
 def visible(self,p,v):
  self.visibility[p]=v;put(p+0x40,(get(p+0x40,'<I')|1) if v else (get(p+0x40,'<I') & ~1),'<I');return 0
 def balloon_init(self,p,parent):
  self.popup_inits.append((p,parent));put(p+8,parent);put(p+0x40,1,'<I')
  arr=self.alloc(4*8)
  for i in range(4):arr.u64(8*i,self.primitive().addr)
  put(p+0x138,arr.addr);put(p+0x140,4,'<I')
  ctx=self.alloc(0x90);ctx.u64(0x18,self.context_vt.addr);wrap=self.alloc(32);wrap.u64(0x10,ctx.addr);put(p+0x98,wrap.addr);return 1
 def window(self,p,x,y,w,h):
  t=self.primitive();self.dynamic_owner[t.addr]=(p,x,y,w,h);return t.addr
 def dialog_text(self,index):
  k=int(index==11);base=self.mod.sym('notice_lines')+k*6*256
  return '\n'.join(wide(base+i*256) for i in range(6)).rstrip('\n')
 def register(self,p,on):
  self.register_events.append((p,on));(self.registered.add if on else self.registered.discard)(p);return 0
 def ctor(self,p):put(p,self.widget_vt.addr);return p
 def primitive(self):
  p=self.alloc(0x300);p.u64(0,self.prim_vt.addr);p.put(0x112,b'\x02');p.put(0x210,struct.pack('<ff',1.,1.));return p
 def widget_init(self,p,x,y,w,h,parent,style):
  x,y,w,h,style=(v&0xffffffff for v in (x,y,w,h,style))
  self.widget_inits.append((p,x,y,w,h,parent,style));put(p+8,parent);put(p+0x20,x,'<I');put(p+0x24,y,'<I');put(p+0x28,w,'<I');put(p+0x2c,h,'<I');put(p+0x40,1,'<I')
  prim=self.primitive();arr=self.alloc(17*8);arr.u64(128,prim.addr);put(p+0x138,arr.addr);put(p+0x140,17,'<I');put(p+0x188,prim.addr)
  ctx=self.alloc(0x90);ctx.u64(0x18,self.context_vt.addr);wrap=self.alloc(32);wrap.u64(0x10,ctx.addr);put(p+0x98,wrap.addr);return 1
 def dynamic_text(self,*a):
  p=self.primitive();self.dynamic_owner[p.addr]=a[:5];return p.addr
 def bind(self,p,event,cb):
  copy=self.alloc(64);C.memmove(copy.addr,cb,64);self.widget_callbacks[p]=copy;return 0
 def status(self,layout,person,out):
  if self.status_override:vals=self.status_override
  else:
   top=b(person+0x1c2);id=get(person+0x10,'<H');selected=self.selected_upper.get(id,top)
   vals=[2 if i<top else (1 if i<selected else 0) if i==top else 3 for i in range(3)]+[0,3,3]
  C.memmove(out,struct.pack('<6i',*vals),24)
 def row(self,o,layout=None):
  r=super().row(o,layout);self.active_layout.u32(0x28,1300);self.active_layout.u32(0x2c,720);r.u32(0x20,192);r.u32(0x24,92)
  return r
 def select(self,functor,arg):
  button=get(arg)
  for id,o in self.officers.items():
   for r in self.mem:
    if r.size==0x280 and get(r.addr+0x178)==o.addr:
     for j in range(3):
      if get(r.addr+0x1b8+8*j)==button:self.selected_upper[id]=j+1;return 0
  return 0
 def control(self,index):return get(self.mod.sym('controls')+8*index)
 def action(self,index):
  p=self.control(index);cb=self.widget_callbacks[p]
  if index in [10,12,13]:
   x=y=0;node=p
   while node:
    x+=get(node+0x20,'<i');y+=get(node+0x24,'<i');node=get(node+8)
   self.current_target=p;self.input.u32(0x34,x+8);self.input.u32(0x38,y+8)
  self.fn('control_invoke',cb.addr,0)
 def accept_preview(self):
  # Legacy R2/R3 engine-transaction tests only. R5 has no rendered acceptance
  # control; test_r5 exercises the real bottom-decision/row-switch paths.
  self.fn('controls_action',2)
 def cancel_preview(self):
  # Internal request regression, not a visible button in R5.
  self.fn('controls_action',8)
 def answer_exit(self,s,yes=True):
  # Explicit user consent in the current native Yes/No warning. Never bypass
  # production warning logic when migrating older transaction regressions.
  if not self.mod.val('exit_modal'):return
  self.keys=set();self.fn('hook_state_tick',s.addr)
  self.action(12 if yes else 13);self.fn('hook_state_tick',s.addr);self.fn('hook_state_tick',s.addr)
 def gsui(self,id):return State.from_address(self.fn('ui_gstate',id))
 def reveal(self,s):self.tick+=1200;self.fn('hook_state_tick',s.addr)
 def bottom(self,s):
  items=[(id,top,b(self.officers[id].addr+0x1c3)) for id,top in self.selected_upper.items()]
  self.confirm_batch=self.batch(items);self.active_layout.u32(0x180,1);self.fn('hook_state_tick',s.addr)
 def detail(self):
  n=self.mod.val('observation_line_count');return '\n'.join(wide(self.mod.sym('observation_lines')+i*256) for i in range(n))
