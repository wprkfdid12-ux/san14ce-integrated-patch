"""R7 dialog/lifetime regressions on the compiled Win64 DLL.
Native click and state tick are captured original machine code. OS/widget
endpoints are substituted; geometry/ownership tests are NOT a GPU visual test.
"""
from test_r6 import R6,settled,miss_seed
from test_native import *
from test_rc4 import PEOPLE

def logged_scenario():
 e=R6();yun,r,s=e.prepare(596,(88,67,161,4,166,62,153,177,204),lower=2,capacity=3)
 yun.put(0x130,bytes(PEOPLE[595]['unique_tactic_ids']))
 e.seed(18428173711460103543);e.click(r,0);settled(e,s)
 for seed in [10158966705624663401,12509697087430763507,15587902592931019370,8293600045877112224,14907993376053380040]:
  e.seed(seed);e.action(0);settled(e,s)
 for _ in range(11):
  if get(e.mod.sym('selected_replacement')+596*2,'<H')==161:break
  e.action(2)
 check(get(e.mod.sym('selected_replacement')+596*2,'<H')==161,'logged Zhao Yun replacement is 상승')
 tong,r2=e.second(649);tong.put(0x118,struct.pack('<9H',118,0,0,0,0,0,0,0,0))
 e.seed(7197012077424426978);e.click(r2,0);settled(e,s)
 e.seed(10193247928097410483);e.action(0);settled(e,s)
 check(e.gsui(596).slots[0].result==68 and e.gsui(649).slots[0].result==33,'log seeds reproduce 수전 and 불굴 exactly')
 check(e.gs(596).rerolls_used==5 and e.gs(649).rerolls_used==1 and e.selection_counts(596)==(1,2) and e.selection_counts(649)==(1,0) and e.decision_enabled,'logged two-officer selection and individual coupon totals preserved')
 return e,yun,tong,r,r2,s

def replacement_and_back():
 e,yun,tong,r,r2,s=logged_scenario();before=[e.owned(yun),e.owned(tong)]
 e.bottom(s);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and e.fn('notice_healthy',11) and e.dialog_text(11)=='조운의 개성을 교체합니다.\n기존: 상승\n획득: 수전\n진행하시겠습니까? (1/1)','logged Decision reaches exact visible-structure replacement dialog')
 e.answer_exit(s,False)
 check(not e.mod.val('exit_modal') and [e.owned(yun),e.owned(tong)]==before and not e.native_commits,'replacement No returns to unchanged current choices')
 e.bottom(s);e.fn('hook_state_tick',s.addr);e.answer_exit(s,True)
 check(e.native_commits==1 and e.pop_calls==1 and 68 in e.owned(yun) and 161 not in e.owned(yun) and 33 in e.owned(tong) and 118 in e.owned(tong),'explicit replacement Yes commits both logged officers once')
 check(not e.registered and not e.mod.val('exit_modal'),'confirmed exit unregisters all owned controls and dialog')
 e.closeall()
 # Back is independently tested before Decision, not inferred from a blocked modal.
 e,yun,tong,r,r2,s=logged_scenario();before=[yun.get(0,0x200),tong.get(0,0x200)]
 e.request_back(s)
 check(e.mod.val('warning_kind')==1 and '초기화' in e.dialog_text(11) and '복구되지 않습니다' in e.dialog_text(11),'standalone Back displays result-reset and no-refund text')
 e.answer_exit(s,True)
 check(e.pop_calls==1 and e.cancel_invocations==1 and not e.native_commits and [yun.get(0,0x200),tong.get(0,0x200)]==before,'standalone Back Yes performs one native cancel with zero trait/rank/cost changes')
 check(e.gs(596).rerolls_used==5 and e.gs(649).rerolls_used==1 and e.gs(596).slots[0].phase==0 and e.gs(649).slots[0].phase==0,'standalone cancellation discards both results but retains each officer spent coupons')
 e.closeall()

def dialog_structure_and_input():
 e=R6();o,r,s=e.prepare();ancestor=e.alloc(0x80);ancestor.u32(0x20,80);ancestor.u32(0x24,50);ancestor.u32(0x40,1)
 e.root_layout.u64(8,ancestor.addr);e.root_layout.u32(0x20,110);e.root_layout.u32(0x24,70)
 e.click(r,0);settled(e,s);e.request_back(s);p=e.control(11)
 check(e.popup_inits==[(p,0)] and get(p)==e.game.addr+e.res.popup.vtable and not get(p+8),'notice uses independent native balloon layout, not a row child button')
 check(get(p+0x20,'<I')==450 and get(p+0x24,'<I')==302 and get(p+0x28,'<I')==780 and get(p+0x2c,'<I')==356,'notice centers in actual logical merit coordinates including all ancestors')
 check(e.layers[p]==6 and all(not get(e.control(i)+8) and e.layers[e.control(i)]==6 for i in [12,13]),'Yes and No are independent native controls on the same registered layer')
 check(p not in e.widget_callbacks and p not in e.texts,'layout is never passed to button caption or callback APIs')
 lines=[get(e.mod.sym('notice_text')+8*(6+j)) for j in range(6)]
 check(len(set(lines))==6 and all(e.dynamic_owner[t][0]==p for t in lines) and sum(bool(e.texts[t]) for t in lines)==4,'warning uses separate owned text primitives for all four lines')
 check(e.texts[e.control(12)]=='예' and e.texts[e.control(13)]=='아니오' and all(get(e.control(i)+0x2c,'<I')==44 for i in [12,13]),'native Yes/No labels and Selection height preserved')
 e.keys=set();e.fn('hook_state_tick',s.addr)
 no=e.control(13);e.input.u32(0x34,get(no+0x20,'<I')+8);e.input.u32(0x38,get(no+0x24,'<I')+8);e.current_target=no
 e.keys={1};e.fn('hook_state_tick',s.addr);e.keys=set();e.fn('hook_state_tick',s.addr)
 check(not e.mod.val('exit_modal') and not e.pop_calls,'logical-coordinate release over No works with an actual native No target')
 e.request_back(s);e.answer_exit(s,True);e.fn('hook_row_destroy',r.addr,e.resources.addr)
 deletes=e.widget_deletes
 check(not e.registered and len(deletes)==len(set(deletes)),'native popup teardown releases each owned object once')
 # Pointer values were detached; order comes from recorded register events.
 off=[x for x,on in e.register_events if not on]
 check(off.index(no)<off.index(p),'popup children unregister before their native root')
 e.closeall()

def failure_recovery():
 for index in [11,12,13]:
  e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);e.action(0);settled(e,s);before=bytes(e.gsui(596));e.request_back(s)
  p=e.control(index);put(p+0x40,get(p+0x40,'<I')&~1,'<I');e.fn('hook_state_tick',s.addr)
  check(not e.mod.val('exit_modal') and not e.pop_calls and not e.native_commits and bytes(e.gsui(596))==before and e.gs(596).rerolls_used==1,'lost native visibility unlocks without consent or refund: '+str(index))
  e.request_back(s);check(e.fn('notice_healthy',11)==1,'warning can be rebuilt/reopened after native visibility loss: '+str(index));e.answer_exit(s,False);e.closeall()
 # A background-creation failure must not leave an invisible input barrier.
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);native=BR.callbacks['Window'];BR.callbacks['Window']=lambda *a:0;e.request_back(s)
 check(not e.mod.val('exit_modal') and not e.pop_calls and not e.native_commits,'native background failure leaves no confirmation barrier')
 BR.callbacks['Window']=native;e.request_back(s);check(e.fn('notice_healthy',11)==1,'background allocation retries without abandoning preview');e.answer_exit(s,False);e.closeall()
 for line in [1,4,6]:
  e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);native=BR.callbacks['DynamicText'];count=[0]
  def dynamic(*args):
   count[0]+=1;return 0 if count[0]==line else native(*args)
  BR.callbacks['DynamicText']=dynamic;e.request_back(s)
  check(not e.mod.val('exit_modal') and not e.pop_calls,'partial notice text failure never locks screen: '+str(line))
  BR.callbacks['DynamicText']=native;e.request_back(s);check(e.fn('notice_healthy',11)==1,'partial notice text construction resumes safely: '+str(line));e.answer_exit(s,False);e.closeall()

def back_during_replacement():
 e=R6();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12));e.click(r,0);e.action(3);settled(e,s);before=e.owned(o)
 e.bottom(s);e.fn('hook_state_tick',s.addr);stale=e.widget_callbacks[e.control(12)];e.request_back(s)
 check(e.mod.val('exit_modal') and e.mod.val('warning_kind')==1 and '초기화' in e.dialog_text(11),'Back during replacement confirmation changes to cancellation warning')
 e.keys=set();e.fn('hook_state_tick',s.addr);e.fn('control_invoke',stale.addr,0);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and not e.native_commits and not e.pop_calls,'old replacement Yes cannot approve new Back warning')
 e.request_back(s);check(not e.mod.val('exit_modal') and e.owned(o)==before and not e.pop_calls,'fresh Back while cancellation warning is open acts as No')
 e.request_back(s);e.answer_exit(s,True);check(e.pop_calls==1 and not e.native_commits,'next explicit Back Yes cancels without applying replacement');e.closeall()

def picker_and_reentry():
 for teardown in [False,True]:
  e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);e.action(0);settled(e,s);before=bytes(e.gsui(596));rng=e.rng_calls;stale=e.widget_callbacks[e.control(0)];calls=[]
  def picker(state):
   calls.append(state);check(e.mod.val('picker_depth')==1 and not e.registered and all(not e.visibility.get(e.control(i),0) for i in range(14) if e.control(i)),'all merit controls suspend before original officer picker: '+str(teardown))
   e.fn('control_invoke',stale.addr,0);e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr);e.fn('hook_close',e.root_layout.addr,0x7fffffff)
   check(e.rng_calls==rng and not e.registered and not e.close_calls,'picker rejects stale toolbar/reentrant tick and Back callback: '+str(teardown))
   if teardown:e.fn('hook_row_destroy',r.addr,e.resources.addr)
   e.root_layout.u32(0x180,0);return 0
  BR.callbacks['NativePicker']=picker;e.game.put(0x7492c0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('NativePicker')))
  e.root_layout.u32(0x180,2);e.fn('hook_state_tick',s.addr)
  check(calls==[s.addr] and not e.mod.val('picker_depth') and bytes(e.gsui(596))==before,'native picker returns once without changing parked gacha: '+str(teardown))
  if teardown:e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr)
  check(e.mod.val('controls_registered') and e.visibility[e.control(0)],'toolbar restored after picker or recreated after owner destruction: '+str(teardown))
  if teardown:
   # Destroying the displayed row parks its result; refocus by native deselect/reselect.
   e.click(r,0);e.click(r,0)
  used=e.gs(596).rerolls_used;e.action(0);settled(e,s);check(e.gs(596).rerolls_used==used+1,'restored toolbar binds a fresh actionable callback: '+str(teardown));e.closeall()
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);e.request_back(s);stale=e.widget_callbacks[e.control(12)];original=BR.callbacks['WidgetUnregister'];injected=[]
 def unregister(p):
  if not injected:
   injected.append(p);e.fn('control_invoke',stale.addr,0);e.fn('hook_close',e.root_layout.addr,0x7fffffff);e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr)
  return original(p)
 BR.callbacks['WidgetUnregister']=unregister;e.fn('hook_ui_exit',e.root_layout.addr);e.fn('hook_row_destroy',r.addr,e.resources.addr)
 check(injected and not e.registered and not e.native_commits and not e.close_calls and not e.mod.val('exit_modal') and not e.mod.val('preview_count'),'unregister-time callback cannot commit, close twice or resurrect controls')
 check(len(e.widget_deletes)==len(set(e.widget_deletes)) and all(ctx==e.resources.addr for _,ctx in e.destroy_contexts),'reentrant cleanup releases native resources once with original context');e.closeall()

def miss_window_transition():
 e=R6();o,r,s=e.prepare();e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s)
 check(e.mod.val('miss_modal') and e.fn('notice_healthy',9) and '열정 버프' in e.dialog_text(9),'MISS description uses independent native text window')
 e.request_back(s);check(e.mod.val('exit_modal') and not e.mod.val('miss_modal') and not e.visibility[e.control(9)],'Back temporarily hides MISS description before warning')
 e.answer_exit(s,False)
 check(e.mod.val('miss_modal') and e.fn('notice_healthy',9) and not e.mod.val('exit_modal'),'Back No restores same MISS description without hidden modal state')
 e.action(10);check(not e.mod.val('miss_modal') and not e.gs(596).passion and not e.gs(596).x_mask,'description OK only closes text and grants no reward')
 e.bottom(s);check(e.pop_calls==1 and e.native_commits==1 and e.gs(596).passion and e.gs(596).x_mask,'original bottom Decision is sole MISS/X/passion commit')
 e.fn('hook_row_destroy',r.addr,e.resources.addr);check(not e.registered and len(e.widget_deletes)==len(set(e.widget_deletes)),'two independent popup roots and children clean up exactly once');e.closeall()

def popup_guards():
 e=Env();evidence=json.loads((ROOT/'analysis/r7_popup_evidence.json').read_text())
 check(e.fn('native_guard_check')==1 and e.res.popup.object_size==384 and e.res.popup.vtable==evidence['vtable'],'popup exact-build bindings and object size accepted')
 for i,f in enumerate(evidence['functions']):
  p=e.res.popup.funcs[i];old=e.game.get(p,16);check(p==f['rva'] and old.hex()==f['first16'],'compiled popup guard matches supplied original capture '+str(i));e.game.put(p,b'\x90')
  check(e.fn('native_guard_check')==0,'foreign popup function bytes reject patch before hooks '+str(i));e.game.put(p,old)
 e.closeall()

FUNCTIONS=[replacement_and_back,dialog_structure_and_input,failure_recovery,back_during_replacement,picker_and_reentry,miss_window_transition,popup_guards]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r7_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R7',len(REPORT))
