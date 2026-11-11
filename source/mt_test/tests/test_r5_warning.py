"""Current Yes/No warning: production DLL, native tick, substituted widgets/OS."""
from test_r5 import *

def request(e,s):
 e.fn('hook_close',e.root_layout.addr,0x7fffffff);e.fn('hook_state_tick',s.addr)

def warning_no_and_style():
 e=R5();o,r,s=e.prepare();g,r2=e.second();e.click(r,0);e.reveal(s);e.action(0);e.reveal(s)
 state=bytes(e.gsui(649));native=o.get(0,0x200);other=bytes(e.gsui(619));rng=e.rng_calls;calc=e.mod.val('calculation_count',fmt='<Q')
 e.keys={1};request(e,s)
 check(e.mod.val('exit_modal') and not e.pop_calls and not e.native_commits,'Back opens warning without committing, charging merit or exiting')
 text=e.texts[e.control(11)]
 check('개성이 확정됩니다' in text and '재추첨권은 복구되지 않습니다' in text and '그래도 진행하시겠습니까?' in text,'warning explicitly explains confirmation, no ticket refund and asks consent')
 check(e.texts[e.control(12)]=='예' and e.texts[e.control(13)]=='아니오' and not e.control(1) and not e.control(8),'Yes/No belongs to warning; removed upper accept/exit remains absent')
 for i in [11,12,13]:
  p=get(e.control(i)+0x188)
  skip=16 if i==11 else 0
  check(C.string_at(p+0x200+skip,0x78-skip)==C.string_at(e.selection_prim+0x200+skip,0x78-skip) and e.aligns[p]==4,'warning uses same native font and center alignment '+str(i))
 check(C.string_at(get(e.control(11)+0x188)+0x200,16)==bytes(16),'panel preserves native skin palette instead of copying dark button text colors')
 yes,no=e.control(12),e.control(13)
 check(get(yes+0x24,'<I')==get(no+0x24,'<I') and get(yes+0x2c,'<I')==44 and get(no+0x2c,'<I')==44,'Yes and No share native Selection height and symmetric baseline')
 check([x[-1] for x in e.widget_inits[-3:]]==[7,3,3],'warning reuses native panel/button skins, without custom colors or OS dialog')
 e.action(12);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and not e.mod.val('exit_choice') and not e.pop_calls,'held opening mouse input cannot choose Yes')
 e.action(0);e.click(r2,0);e.action(4);e.root_layout.u32(0x180,1);e.fn('hook_state_tick',s.addr)
 check(e.rng_calls==rng and e.mod.val('calculation_count',fmt='<Q')==calc and bytes(e.gsui(619))==other and not e.native_commits,'warning blocks underlying reroll, row selection, details and native decision')
 e.root_layout.u32(0x180,2);e.keys=set();e.fn('hook_state_tick',s.addr)
 check(e.root_layout.u32(0x180)==0 and e.mod.val('exit_armed'),'native select request behind warning is discarded after release')
 e.keys={13};e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and not e.pop_calls,'Enter alone never silently selects Yes')
 stale=e.widget_callbacks[yes];e.answer_exit(s,False)
 check(not e.mod.val('exit_modal') and bytes(e.gsui(649))==state and o.get(0,0x200)==native and e.gs(649).rerolls_used==1,'No restores same result and remaining personal tickets without committing')
 check(not any(e.control(i) in e.registered for i in [11,12,13]) and e.control(0) in e.registered,'No unregisters only modal inputs and restores ordinary toolbar')
 count=len(e.widget_inits);request(e,s);e.keys=set();e.fn('hook_state_tick',s.addr);e.fn('control_invoke',stale.addr,0);e.fn('hook_state_tick',s.addr)
 check(len(e.widget_inits)==count and e.mod.val('exit_modal') and not e.pop_calls,'reopened warning reuses widgets but rejects previous warning Yes callback')
 e.action(13);e.action(12);e.fn('hook_state_tick',s.addr)
 check(not e.mod.val('exit_modal') and not e.pop_calls,'rapid No then Yes honors first queued choice only')
 request(e,s);e.keys=set();e.fn('hook_state_tick',s.addr);e.action(12);e.action(12);e.action(13);e.fn('hook_state_tick',s.addr)
 check(e.pop_calls==1 and e.native_commits==1 and e.gs(649).rerolls_used==1,'rapid Yes/Yes/No commits and exits exactly once without refund')
 e.closeall()

def escape_and_paused_modes():
 for mode in ['spin','result','reroll_spin','miss']:
  e=R5();o,r,s=e.prepare(top=1)
  if mode=='miss':e.seed(miss_seed(e))
  e.click(r,0)
  if mode!='spin':e.reveal(s)
  if mode=='reroll_spin':e.action(0)
  if mode=='miss':e.notice(s)
  state=bytes(e.gsui(649));mode_before=e.mod.val('ui_mode');rng=e.rng_calls;used=e.gs(649).rerolls_used
  # Simulate a native queued ESC independent from physical key-up.
  queued=[1];BR.callbacks['Extra']=lambda:queued[0]
  BR.callbacks['ConsumeInput']=lambda:queued.__setitem__(0,0)
  e.keys={27};e.fn('hook_state_tick',s.addr)
  check(e.mod.val('exit_modal') and not queued[0] and not e.pop_calls,'ESC opens warning and consumes native queued input: '+mode)
  e.tick+=5000;e.fn('hook_state_tick',s.addr)
  check(e.mod.val('exit_modal') and not e.mod.val('exit_armed'),'held ESC cannot immediately dismiss its own warning: '+mode)
  e.keys=set();e.fn('hook_state_tick',s.addr);queued[0]=1;e.keys={27};e.fn('hook_state_tick',s.addr)
  check(not e.mod.val('exit_modal') and not queued[0] and bytes(e.gsui(649))==state and e.gs(649).rerolls_used==used,'fresh ESC means No, keeps exact preview/tickets: '+mode)
  e.keys=set();e.fn('hook_state_tick',s.addr)
  check(not e.mod.val('exit_modal') and not e.pop_calls and e.rng_calls==rng and e.mod.val('ui_mode')==mode_before,'No does not reopen from buffered ESC or advance paused animation: '+mode)
  if mode=='miss':check(e.mod.val('miss_modal') and not e.gs(649).x_mask and not e.gs(649).passion,'No restores MISS explanation without applying X or passion')
  e.back(s);check(e.pop_calls==1 and e.gs(649).slots[0].phase==2,'subsequent explicit Yes still exits correctly: '+mode)
  e.closeall()

def allocation_failure_and_teardown():
 for fail_at in [1,2,3]:
  e=R5();o,r,s=e.prepare();e.click(r,0);e.reveal(s);e.action(0);e.reveal(s)
  state=bytes(e.gsui(649));rng=e.rng_calls;original=BR.callbacks['WidgetAlloc'];n=[0]
  def alloc(size):
   n[0]+=1
   return 0 if n[0]==fail_at else original(size)
  BR.callbacks['WidgetAlloc']=alloc;request(e,s)
  check(not e.mod.val('exit_modal') and not e.pop_calls and not e.native_commits and bytes(e.gsui(649))==state,'warning allocation failure never implies Yes or mutates result: '+str(fail_at))
  check(not any(e.control(i) in e.registered for i in [11,12,13]) and e.gs(649).rerolls_used==1,'partially built warning is hidden/unregistered and ticket stays spent: '+str(fail_at))
  BR.callbacks['WidgetAlloc']=original;request(e,s)
  check(e.mod.val('exit_modal') and e.rng_calls==rng,'safe retry finishes only missing warning allocations: '+str(fail_at))
  e.answer_exit(s);e.fn('hook_row_destroy',r.addr,e.resources.addr)
  check(e.pop_calls==1 and len(e.widget_deletes)==len(set(e.widget_deletes)) and not BR.errors,'retry commits once; partial/complete widgets all destroyed once: '+str(fail_at))
  e.closeall()
 e=R5();o,r,s=e.prepare();e.click(r,0);e.reveal(s);e.action(0);e.reveal(s);request(e,s)
 stale=e.widget_callbacks[e.control(12)];rng=e.rng_calls;e.fn('hook_ui_exit',e.root_layout.addr);e.fn('hook_row_destroy',r.addr,e.resources.addr);e.fn('control_invoke',stale.addr,0)
 check(not e.mod.val('exit_modal') and not e.registered and e.gs(649).rerolls_used==1 and not e.native_commits and e.rng_calls==rng,'forced teardown while warning open never treats stale Yes as consent or refunds ticket')
 check(len(e.widget_deletes)==10 and len(set(e.widget_deletes))==10 and not BR.errors,'all seven base and three warning widgets are released once')
 e.closeall()
 e=R5();o,r,s=e.prepare();e.click(r,0);e.reveal(s);request(e,s);e.root_layout.u32(0x180,1)
 e.fn('hook_row_destroy',r.addr,e.resources.addr);e.fn('hook_state_tick',s.addr)
 check(not e.pop_calls and not e.native_commits and e.mod.val('preview_count') and e.root_layout.u32(0x180)==0,'row destruction with warning open discards hidden decision instead of implicitly choosing Yes')
 e.bottom(s);check(e.pop_calls==1,'fresh explicit bottom decision remains available after warning owner teardown')
 e.closeall()

if __name__=='__main__':
 try:
  for f in [warning_no_and_style,escape_and_paused_modes,allocation_failure_and_teardown]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r5_warning_tests.json').write_text(json.dumps({'scope':'Production DLL and captured native state tick; substituted OS/widgets, NOT live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R5 warning',len(REPORT))
