"""R8: real shared end-event bytes + asynchronous fade endpoint + bottom GROUP.
This corrects the R7 fixture's single Back-widget assumption. External resource
rendering/OS/fade completion/cost endpoints remain substituted, NOT a game run.
"""
from test_r6 import R6,settled
from test_native import *
from test_rc4 import PEOPLE

class R8(R6):
 def prepare(self,*a,**kw):
  out=super().prepare(*a,**kw);self.fade_events=[];self.native_unreg=0;self.close_codes=[]
  # Original constructor allocates a command GROUP at layout+1c8.
  g=self.cancel_button;self.ctor(g.addr);self.widget_init(g.addr,600,900,500,120,0,3)
  self.root_layout.u32(0x178,0);self.root_layout.u32(0x180,0)
  block=json.loads((ROOT/'analysis/r6_native_evidence.json').read_text())['blocks']['fn_6be850.asm']
  self.game.put(block['rva'],bytes.fromhex(block['bytes']))
  self.game.put(0x856090,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('NativeClose')))
  def fade(inner,code):
   assert inner==self.transition.addr
   recorded=get(self.root_layout.addr+0x178,'<I')
   assert recorded==(code&0xffffffff)
   self.close_codes.append(recorded);self.fade_events.append(recorded);self.close_calls+=1;return 0
  BR.callbacks['NativeClose']=fade
  BR.callbacks['UIExit']=lambda p:setattr(self,'native_unreg',self.native_unreg+1)
  def cancel(group,kind):
   assert group==g.addr and kind==4
   self.cancel_invocations+=1;self.fn('hook_close',self.root_layout.addr,0x7fffffff)
  BR.callbacks['CancelButton']=cancel
  # Actual lifecycle hook has the captured original vtable pointer, not a
  # fabricated field in the game layout. Only its external register is mocked.
  h=next(x for x in self.res.hooks[:self.res.hook_count] if x.kind==23)
  self.game.put(h.target,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('WidgetRegister')))
  self.fn('hook_ui_enter',self.root_layout.addr)
  return out
 def bottom_gesture(self,s,dispatch=True):
  self.current_target=self.cancel_button.addr
  self.input.u32(0x34,640);self.input.u32(0x38,950)
  self.keys={1};self.fn('hook_state_tick',s.addr)
  self.keys=set()
  if dispatch:self.fn('hook_close',self.root_layout.addr,1)
  self.fn('hook_state_tick',s.addr)
 def finish_fade(self,s):
  code=self.fade_events.pop(0)
  if code==1:self.root_layout.u32(0x180,1);self.fn('hook_state_tick',s.addr)
  elif code==0x7fffffff:self.pop_state(0)
  return code

def exact_report():
 e=R8();yun,r,s=e.prepare(596,(88,67,161,4,166,62,153,177,204),lower=2,capacity=3)
 yun.put(0x130,bytes(PEOPLE[595]['unique_tactic_ids']))
 e.seed(7476374893248464071);e.click(r,0);settled(e,s)
 e.seed(9106355430018118111);e.action(0);settled(e,s)
 for _ in range(11):
  if get(e.mod.sym('selected_replacement')+596*2,'<H')==161:break
  e.action(2)
 guang,r2=e.second(619)
 e.seed(11544960271038789809);e.click(r2,0);settled(e,s)
 e.seed(5350142522450800474);e.action(0);settled(e,s)
 check(e.gsui(596).slots[0].result==6 and e.gsui(619).slots[0].result==46,'latest R7 log seeds reproduce Zhao Yun 원시 and Jo Gwang 독려')
 check(e.selection_counts(596)==(1,2) and e.selection_counts(619)==(1,0) and e.gs(596).rerolls_used==e.gs(619).rerolls_used==1,'original shared-rank allowance and each officer own spent coupon retained')
 return e,yun,guang,r,r2,s

def decision_routing():
 e,yun,guang,r,r2,s=exact_report()
 e.bottom_gesture(s,False)
 check(not e.mod.val('cancel_requested') and not e.mod.val('exit_modal') and not e.close_codes,'a bottom-group rectangle press/release never synthesizes Back')
 e.bottom_gesture(s)
 check(e.close_codes==[1] and not e.mod.val('exit_modal') and not e.mod.val('cancel_requested'),'native Decision code passes unchanged through captured shared end-event function')
 check(not e.native_commits,'Decision waits for original fade/state event rather than synthetic immediate state mutation')
 e.finish_fade(s);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('warning_kind')==2 and '획득: 원시' in e.dialog_text(11) and '초기화' not in e.dialog_text(11),'Decision opens only named replacement confirmation, never the Back discard warning')
 e.answer_exit(s,True)
 check(e.native_commits==1 and e.pop_calls==1 and 6 in e.owned(yun) and 161 not in e.owned(yun) and 46 in e.owned(guang),'approved Decision applies exact two-officer outcomes once')
 check(not e.mod.val('preview_count') and not e.mod.val('exit_modal'),'commit clears previews and warning state')
 e.closeall()

def blank_and_release():
 e,yun,guang,r,r2,s=exact_report();before=(yun.get(0,0x200),guang.get(0,0x200))
 e.request_back(s);e.keys=set();e.fn('hook_state_tick',s.addr)
 p=e.control(11);yes=e.control(12);no=e.control(13)
 check(e.mod.val('warning_kind')==1 and e.fn('notice_healthy',11),'explicit Back alone opens a healthy discard warning')
 check(not get(yes+8) and not get(no+8) and e.layers[p]==e.layers[yes]==e.layers[no]==6,'Yes/No have no hidden/clipped popup parent; native roots use one ordered render layer')
 check(e.texts[yes]=='예' and e.texts[no]=='아니오','both native buttons have explicit Korean captions')
 # Exact failure mode: native target is blank window, while cursor lies within
 # the old invisible Yes rectangle. Both callback and fallback must reject it.
 e.current_target=p;e.input.u32(0x34,get(yes+0x20,'<I')+8);e.input.u32(0x38,get(yes+0x24,'<I')+8)
 for _ in range(6):
  e.keys={1};e.fn('hook_state_tick',s.addr);e.fn('control_invoke',e.widget_callbacks[yes].addr,0)
  e.keys=set();e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and not e.mod.val('exit_choice') and not e.cancel_invocations and not e.pop_calls,'repeated blank-space clicks cannot grant Yes consent or start teardown')
 e.fn('hook_close',e.root_layout.addr,1);e.fn('hook_state_tick',s.addr)
 check(not e.close_codes and e.mod.val('warning_kind')==1,'background Decision cannot start a fade behind the Back warning')
 e.keys={1};e.raw_action(12);e.fn('hook_state_tick',s.addr);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and not e.cancel_invocations,'Yes mouse-down never unregisters or cancels while button is held')
 e.keys=set();e.fn('hook_state_tick',s.addr)
 check(e.cancel_invocations==1 and e.close_codes==[0x7fffffff] and not e.pop_calls,'released Yes starts only the original cancel fade, without forced state pop')
 e.fn('hook_close',e.root_layout.addr,0x7fffffff);e.fn('hook_close',e.root_layout.addr,1);e.fn('hook_state_tick',s.addr)
 check(e.close_codes==[0x7fffffff],'late native Back/Decision callbacks cannot restart the closing transition')
 e.finish_fade(s);e.fn('hook_ui_exit',e.root_layout.addr)
 check(e.native_unreg==1 and e.pop_calls==1,'duplicate native UI unregister is ignored until genuine next registration')
 check((yun.get(0,0x200),guang.get(0,0x200))==before and not e.native_commits,'Back fade changes no officer data and spends no original rank/cost')
 check(e.gs(596).rerolls_used==e.gs(619).rerolls_used==1 and e.gs(596).slots[0].phase==e.gs(619).slots[0].phase==0,'discard clears results while preserving both spent individual tickets')
 stale=e.widget_callbacks[yes];e.fn('hook_row_destroy',r.addr,e.resources.addr);e.fn('control_invoke',stale.addr,0)
 check(len(e.widget_deletes)==len(set(e.widget_deletes)) and not e.mod.val('controls_registered'),'queued stale Yes after cleanup cannot access or recreate controls')
 e.fn('hook_ui_enter',e.root_layout.addr);e.fn('hook_ui_exit',e.root_layout.addr)
 check(e.native_unreg==2,'reused layout address can register and unregister normally in a new visit')
 e.closeall()

def guards_and_callbacks():
 e=R8();o,r,s=e.prepare();e.click(r,0);settled(e,s)
 for code in [1,2,0x7ffffffd]:
  e.fn('hook_close',e.root_layout.addr,code)
  check(e.close_codes[-1]==code and not e.mod.val('cancel_requested'),'shared end-event preserves non-Back code '+hex(code))
 e.request_back(s);e.keys=set();e.fn('hook_state_tick',s.addr)
 no=e.control(13);argument=e.alloc(8);argument.u64(0,no)
 e.current_target=e.control(11);e.input.u32(0x34,get(no+0x20,'<I')+8);e.input.u32(0x38,get(no+0x24,'<I')+8)
 e.fn('control_invoke',e.widget_callbacks[no].addr,argument.addr);e.fn('hook_state_tick',s.addr)
 check(not e.mod.val('exit_modal') and not e.cancel_invocations and e.gsui(596).slots[0].phase==1,'native No event argument works even after hover target resets to root; preview retained')
 check(e.res.hook_count==35 and any(x.rva==0x1447cc8 and x.target==0x6b4f20 and x.kind==23 for x in e.res.hooks[:35]),'native registration lifecycle hook exactly matches captured original vtable')
 e.closeall()

FUNCTIONS=[decision_routing,blank_and_release,guards_and_callbacks]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r8_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R8',len(REPORT))
