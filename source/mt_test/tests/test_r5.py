"""R5 continuation tests (latest rule: Back accepts the last result).
Runs production Win64 DLL + captured ORIGINAL native state-tick bytes.
External widgets/OS/cost/dialog endpoints are substituted; NOT a live game.
"""
from test_r3 import *

class R5(R3):
 def __init__(self):
  super().__init__();self.rects={};self.aligns={};self.fonts={};self.root_layout=None;self.list_owner=None
  BR.callbacks.update({'PrimSize':lambda p,w,h:self.rects.setdefault(p,{}).update(size=(w,h)),
   'PrimPos':lambda p,x,y:self.rects.setdefault(p,{}).update(pos=(x,y)),
   'Align':lambda p,a:self.aligns.__setitem__(p,a),'Font':lambda p,n:self.fonts.__setitem__(p,n)})
 def status(self,layout,person,out):
  if self.list_owner is not None:assert layout==self.list_owner.addr,'F_STATUS requires the MAIN merit selection owner'
  return super().status(layout,person,out)
 def prepare(self,id=649,owned=(4,37,51,62,65),top=0):
  p=PEOPLE[id-1];o=self.officer(id,tuple(p[k] for k in ('leadership','war','intelligence','politics','charisma')),owned,top)
  self.fn('hook_newgame',0,0);self.root_layout=self.alloc(0x300);self.list_owner=self.root_layout
  # R9: independent native constructor/refresh evidence; +1d8 is a transition
  # controller, and is never the row parent / selection tree owner.
  self.transition=self.alloc(0x198);self.root_layout.u64(0x1d8,self.transition.addr)
  self.root_layout.u64(0,self.game.addr+0x1447c90)
  self.root_layout.u32(0x40,1);r=self.row(o,self.list_owner)
  # A real-shaped native Selection button, distinct from row reward widgets.
  self.selection=self.alloc(0x198);self.ctor(self.selection.addr);self.widget_init(self.selection.addr,192,26,192,44,self.list_owner.addr,3)
  self.selection_prim=get(self.selection.addr+0x188);C.memset(self.selection_prim+0x200,0x35,0x78)
  children=self.alloc(8);children.u64(0,self.selection.addr);self.list_owner.u64(0x138,children.addr);self.list_owner.u32(0x140,1)
  self.widget_inits.clear();self.active_layout=self.root_layout
  s=self.alloc(0x600);s.u64(0x4c8,self.root_layout.addr);self.fn('hook_row_refresh',r.addr);self.fn('hook_state_tick',s.addr)
  self.cancel_button=self.alloc(0x198);self.root_layout.u64(0x1c8,self.cancel_button.addr);self.cancel_invocations=0;self.consumed=0
  def cancel(button,kind):
   assert button==self.cancel_button.addr and kind==4
   self.cancel_invocations+=1;self.fn('hook_close',self.root_layout.addr,0x7fffffff)
   if not self.mod.val('cancel_requested'):self.pop_state(0)
  BR.callbacks['CancelButton']=cancel;BR.callbacks['ConsumeInput']=lambda:setattr(self,'consumed',self.consumed+1)
  BR.callbacks['Extra']=lambda:int(27 in self.keys)
  for rva,name in [(0x856830,'CancelButton'),(0x3f2950,'ConsumeInput'),(0x3f2c40,'Extra'),(0x10f20,'Manager'),(0x12380,'PopState')]:
   self.game.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
  fragment=bytes.fromhex(json.loads((ROOT/'analysis/native_fragments_rc2.json').read_text())['state_tick'])
  self.game.put(self.res.funcs[15],fragment)
  relay=0x26ff000;self.game.put(relay,b'\xff\x25\0\0\0\0'+struct.pack('<Q',self.mod.sym('hook_confirm')))
  self.game.put(0x732fc0,b'\xe8'+struct.pack('<i',relay-0x732fc5))
  return o,r,s
 def confirm(self,state):
  # The actual native function builds its batch itself; the substituted cost
  # endpoint builds the corresponding per-ID selection tree on every request.
  self.confirm_batch=self.batch([(id,top,b(self.officers[id].addr+0x1c3)) for id,top in self.selected_upper.items()])
  return super().confirm(state)
 def second(self,id=619,top=0):
  o=self.officer(id,(65,68,53,51,60),(32,),top);r=self.row(o,self.list_owner);r.u32(0x24,302)
  self.active_layout=self.root_layout;self.fn('hook_row_refresh',r.addr);return o,r
 def back(self,s):
  self.fn('hook_close',self.root_layout.addr,0x7fffffff);self.fn('hook_state_tick',s.addr)
  self.answer_exit(s)

def ui_and_bottom():
 e=R5();o,r,s=e.prepare();reroll=e.control(0);calc=e.control(4)
 check(not e.control(1) and not e.control(8) and len(e.widget_inits)==7,'top accept/exit are absent objects, not invisible hit targets')
 check(get(reroll+0x24,'<I')==26 and get(reroll+0x2c,'<I')==44,'reroll exactly shares native Selection button baseline and height')
 check(e.texts[calc]=='계산 근거' and '\n' not in e.texts[calc],'calculation button has only the requested single-line wording')
 for w in [reroll,calc]:
  p=get(w+0x188);width=get(w+0x28,'<I');height=get(w+0x2c,'<I')
  check(C.string_at(p+0x200,0x78)==C.string_at(e.selection_prim+0x200,0x78) and p not in e.fonts,'button uses exact native Selection typography, no smaller forced font')
  check(e.aligns[p]==4 and e.rects[p]=={'size':(width-8,height-8),'pos':(4,4)},'caption is center/center with equal four-side insets')
 e.click(r,0);e.bottom(s);check(not e.native_commits,'bottom input during animation does not commit a hidden result')
 e.reveal(s);key=e.gsui(649).slots[0].result;e.keys={13};e.fn('hook_state_tick',s.addr)
 check(e.gsui(649).slots[0].phase==1 and not e.native_commits,'Enter does not act as removed upper acceptance')
 e.keys=set();e.bottom(s)
 check(key in e.owned(o) and e.native_commits==1 and e.pop_calls==1,'original bottom decision directly accepts and applies last result once')
 check(not e.mod.val('preview_count') and not e.registered,'successful return detaches all owned input widgets')
 e.closeall()

def two_officers():
 e=R5();tong,r,s=e.prepare();guang,r2=e.second();buttons=[get(r2.addr+0x1b8+j*8) for j in range(3)]
 untouched=bytes(e.gsui(619));native=guang.get(0,0x200);appearance=[(e.texts[b],e.adapter_enable[b]) for b in buttons]
 e.click(r,0)
 check(bytes(e.gsui(619))==untouched and guang.get(0,0x200)==native,'Jo Tong draw never mutates Jo Gwang state or real data')
 check([(e.texts[b],e.adapter_enable[b]) for b in buttons]==appearance,'Jo Tong animation does not disable Jo Gwang unlock tile')
 e.reveal(s);e.action(0);e.reveal(s);tongstate=bytes(e.gsui(649));e.click(r2,0);e.reveal(s)
 check(e.mod.val('active_id',fmt='<H')==619 and bytes(e.gsui(649))==tongstate,'other row opens while previous result remains independently parked')
 for _ in range(5):e.action(0);e.reveal(s)
 check(e.gs(649).rerolls_used==1 and e.gs(619).rerolls_used==5,'successful rerolls immediately reserve only the correct officer tickets')
 guangstate=bytes(e.gsui(619));rng=e.rng_calls;e.click(r,0)
 check(e.rng_calls==rng and '4/5' in e.texts[e.control(0)] and bytes(e.gsui(619))==guangstate,'returning to Jo Tong reopens same result and personal four tickets')
 keys=[e.gsui(id).slots[0].result for id in [649,619]];e.back(s)
 check(keys[0] in e.owned(tong) and keys[1] in e.owned(guang),'Back commits each row own LAST result, with no cross-officer replacement')
 check(e.native_commits==1 and e.pop_calls==1 and not e.cancel_invocations,'Back uses one native decision chain, not commit plus a second native cancel')
 check(e.gs(649).rerolls_used==1 and e.gs(619).rerolls_used==5 and all(e.fn('g_validate',C.addressof(e.gs(id))) for id in [649,619]),'final result preserves individual spent-ticket totals without double charge')
 e.closeall()

def exit_phases():
 for mode in ['spin','result','reroll_spin','reroll_result','miss_notice']:
  for route in ['back','esc']:
   e=R5();o,r,s=e.prepare(top=1)
   if mode=='miss_notice':e.seed(miss_seed(e))
   e.click(r,0)
   if mode!='spin':e.reveal(s)
   if mode.startswith('reroll'):e.action(0)
   if mode=='reroll_result':e.reveal(s)
   if mode=='miss_notice':e.notice(s)
   key=e.gsui(649).slots[0].result;miss=e.gsui(649).slots[0].outcome;used=int(mode.startswith('reroll'))
   stale=e.widget_callbacks[e.control(0)];rng=e.rng_calls
   if route=='back':
    e.fn('hook_close',e.root_layout.addr,0x7fffffff)
    check(not e.pop_calls and not e.talks,'Back queues safely without destroying inside native callback: '+mode)
   else:e.keys={27}
   e.fn('hook_state_tick',s.addr)
   check(e.mod.val('exit_modal') and not e.pop_calls,'Back/ESC displays Yes/No before any exit: '+mode+'/'+route)
   e.answer_exit(s)
   check(e.pop_calls==1 and e.gs(649).slots[0].phase==2 and not e.mod.val('preview_count'),'last result accepted and one native return: '+mode+'/'+route)
   check(e.gs(649).rerolls_used==used and (bool(e.gs(649).x_mask) if miss else key in e.owned(o)),'exit retains exact last outcome and spent tickets: '+mode+'/'+route)
   check(not e.cancel_invocations and not e.mod.val('miss_modal'),'result exit never invokes both native cancel and decision: '+mode+'/'+route)
   e.fn('control_invoke',stale.addr,0)
   check(e.rng_calls==rng and e.pop_calls==1,'stale callback after exit cannot reroll or close twice: '+mode+'/'+route)
   e.closeall()

def teardown_and_recycling():
 e=R5();o,r,s=e.prepare(top=1);other,r2=e.second(top=1);e.click(r2,0);e.reveal(s);e.action(0);e.reveal(s)
 stale=e.widget_callbacks[e.control(0)];rng=e.rng_calls;old=bytes(e.gsui(619));e.fn('hook_row_destroy',r.addr,e.resources.addr)
 e.fn('hook_row_refresh',r2.addr);e.fn('control_invoke',stale.addr,0)
 check(e.rng_calls==rng and bytes(e.gsui(619))==old,'destroying toolbar anchor row preserves OTHER officer preview and rejects old callback')
 e.action(0);e.reveal(s);check(e.gs(619).rerolls_used==2 and not e.gs(649).rerolls_used,'rebuilt toolbar still belongs to active officer, not former owner row')
 # Recycle the actual active row allocation to another officer.
 r2.u64(0x178,o.addr);e.fn('hook_row_refresh',r2.addr)
 check(e.mod.val('active_id',fmt='<H')==65535 and not e.adapter_enable[e.control(0)],'recycled row parks old active ID instead of rerolling through new occupant')
 e.click(r2,0);e.reveal(s)
 check(e.gsui(649).slots[0].phase==1 and e.gsui(619).rerolls_used==2,'reused row starts its new officer without overwriting parked previous officer')
 # Inject late callbacks and row refresh DURING native unregister.
 stale=e.widget_callbacks[e.control(0)];rng=e.rng_calls;register=e.register;injected=[]
 def unregister(p,on):
  if not on:
   injected.append(p);e.fn('control_invoke',stale.addr,0);e.fn('hook_row_refresh',r2.addr)
  return register(p,on)
 BR.callbacks['WidgetUnregister']=lambda p:unregister(p,0)
 e.back(s)
 check(injected and e.rng_calls==rng and not e.registered and not e.mod.val('preview_count'),'teardown rejects reentrant callbacks and cannot re-register a dying toolbar')
 e.fn('hook_row_destroy',r2.addr,e.resources.addr)
 check(len(e.widget_deletes)==len(set(e.widget_deletes)) and all(ctx==e.resources.addr for _,ctx in e.destroy_contexts),'each owned widget destroyed once using exact native resource context')
 e.closeall()

def failure_and_no_result():
 e=R5();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),top=0);native=o.get(0,0x200);e.click(r,0)
 check(e.mod.val('ui_mode')==1 and not e.selected_upper,'unrolled inheritance choice does not select/spend a native award')
 e.back(s);check(e.pop_calls==1 and o.get(0,0x200)==native and not e.mod.val('preview_count'),'Back before any roll uses normal cancellation with no fake result or fee')
 e.closeall()
 e=R5();o,r,s=e.prepare();e.click(r,0);e.reveal(s);e.action(0);e.reveal(s);key=e.gsui(649).slots[0].result;e.fail_commit=True;e.back(s)
 check(not e.pop_calls and e.gs(649).rerolls_used==1 and e.mod.val('preview_count')==1,'failed native award preserves result and spent coupon, without pretending exit succeeded')
 calls=e.native_commits;e.fn('hook_state_tick',s.addr)
 check(e.native_commits==calls and e.root_layout.u32(0x180)==0,'failed decision is not retried forever on every idle frame')
 e.fail_commit=False;e.back(s)
 check(e.pop_calls==1 and key in e.owned(o) and e.gs(649).rerolls_used==1,'retry accepts same result once with no ticket refund or extra roll')
 e.closeall()

def ticket_persistence_and_stage_order():
 e=R5();o,r,s=e.prepare(owned=(118,),top=3);native=o.get(0,0x200)
 e.click(r,0);e.reveal(s);e.action(0);e.reveal(s)
 e.fn('hook_ui_exit',e.root_layout.addr)
 check(e.gs(649).rerolls_used==1 and e.gs(649).cancelled_rerolls==1 and o.get(0,0x200)==native,'forced detach never commits from destructor or refunds reserved ticket')
 expected=bytes(e.gs(649));rng=e.rng_calls
 check(e.save(b'R5 forced detach original save')==1 and e.loadsave(b'R5 forced detach original save')==1 and bytes(e.gs(649))==expected,'reserved ticket remains valid through exact-save v3 journal roundtrip')
 check(e.rng_calls==rng,'forced-detach metadata reload never draws another trait')
 e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr);e.click(r,2);e.reveal(s)
 values=[e.gsui(649).slots[2].result]
 for stage in [0,1]:
  e.click(r,stage);e.reveal(s);values.append(e.gsui(649).slots[stage].result)
 check(e.gsui(649).slots[2].phase==2 and e.gsui(649).slots[0].phase==2 and e.gsui(649).slots[1].phase==1,'same-officer next stage parks accepted preview internally without an upper confirmation button')
 check(o.get(0,0x200)==native and all(e.gs(649).slots[j].phase==0 for j in range(3)),'stage switching never applies speculative traits to game data')
 e.back(s)
 check(all(v in e.owned(o) for v in values) and e.gs(649).rerolls_used==1 and e.fn('g_validate',C.addressof(e.gs(649))),'Back accepts all three chronological outcomes and preserves preexisting spent ticket')
 e.closeall()

def reported_video_sequence():
 # Interaction and RNG seeds from user video + final log session. Native UI,
 # OS and other officers remain substituted, so this is NOT a live replay.
 e=R5();o,r,s=e.prepare(owned=(118,));other,r2=e.second();native=other.get(0,0x200)
 e.seed(13566493149584141705);e.click(r,0);e.reveal(s)
 e.seed(11608571727616103415);e.action(0);e.reveal(s);last=e.gsui(649).slots[0].result
 check(e.gs(649).rerolls_used==1 and e.gs(619).rerolls_used==0,'reported first draw/reroll sequence reserves exactly one Jo Tong coupon')
 callback=e.widget_callbacks[e.control(0)];rng=e.rng_calls;forward=[]
 original=BR.callbacks['ButtonEvent'];BR.callbacks['ButtonEvent']=lambda fn,arg:forward.append((fn,arg)) or 0
 dummy=e.alloc(0x198);f=e.alloc(16);f.u64(8,e.list_owner.addr);a=e.alloc(8);a.u64(0,dummy.addr)
 e.fn('hook_button_event',f.addr,a.addr);BR.callbacks['ButtonEvent']=original
 check(len(forward)==1 and e.rng_calls==rng,'revealed preview does not swallow unrelated native lower-row action')
 e.click(r2,0);e.reveal(s)
 check(e.mod.val('active_id',fmt='<H')==619 and e.gsui(619).slots[0].phase==1,'Jo Gwang unlock actually responds after Jo Tong reroll, not just enabled paint')
 otherlast=e.gsui(619).slots[0].result;e.back(s)
 check(last in e.owned(o) and otherlast in e.owned(other) and e.gs(649).rerolls_used==1,'video-like Back commits both latest results and does not refund reroll')
 # Original unregister and deferred row destruction occur separately.
 e.fn('hook_row_destroy',r2.addr,e.resources.addr);e.fn('hook_row_destroy',r.addr,e.resources.addr)
 e.fn('control_invoke',callback.addr,0)
 check(not e.registered and len(e.widget_deletes)==len(set(e.widget_deletes)) and not BR.errors,'deferred native row cleanup and late callback leave no live input widget or duplicate deletion')
 check(all(x['mod']=='RC4-R5-SafeExit' for x in events(e)),'new events identify SafeExit build instead of inherited misleading R3 label')
 e.closeall()

if __name__=='__main__':
 try:
  for f in [ui_and_bottom,two_officers,exit_phases,teardown_and_recycling,failure_and_no_result,ticket_persistence_and_stage_order,reported_video_sequence]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r5_tests.json').write_text(json.dumps({'scope':'Production Win64 DLL plus original native state tick, substituted OS/widgets; NOT live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R5',len(REPORT))
