"""R8 command/input regressions carried forward with the intentional R10
same-turn return timing: successful Decision suppresses the outer
Manager->PopState and completes through the verified native cancel/fade route.
Synthetic Win64 ABI fixture; not a live game certification.
"""
from test_r8 import R8, exact_report, blank_and_release, guards_and_callbacks
from test_native import *

def decision_routing_r10():
 e,yun,guang,r,r2,s=exact_report();manager=[0]
 BR.callbacks['Manager']=lambda: manager.__setitem__(0,manager[0]+1) or 0x1234
 e.bottom_gesture(s,False)
 check(not e.mod.val('cancel_requested') and not e.mod.val('exit_modal') and not e.close_codes,'R8 carry-forward: bottom-group rectangle press/release never synthesizes Back')
 e.bottom_gesture(s)
 check(e.close_codes==[1] and not e.native_commits,'R8 carry-forward: native Decision event still waits for original fade/state event')
 e.finish_fade(s);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('warning_kind')==2,'R8 carry-forward: replacement warning still precedes commit')
 e.answer_exit(s,True)
 check(e.native_commits==1 and manager[0]==0 and e.cancel_invocations==1,'R10 same-turn route performs one commit, blocks outer Manager tail, and dispatches one verified native cancel')
 check(e.pop_calls==0 and e.mod.val('closing_layout',fmt='<Q')==e.root_layout.addr,'R10 asynchronous native cancel fences interim ticks until fade completion')
 # The substituted F_CONFIRM emits its own non-Back presentation fade first;
 # drain it, then the R10 native Back fade returns exactly once.
 while e.fade_events and not e.pop_calls:e.finish_fade(s)
 check(e.pop_calls==1 and 6 in e.owned(yun) and 161 not in e.owned(yun) and 46 in e.owned(guang),'R10 fade completion returns once with both officers exact results committed')
 check(not e.mod.val('preview_count') and not e.mod.val('exit_modal'),'R10 commit clears previews and warning state')
 e.closeall()

FUNCTIONS=[decision_routing_r10,blank_and_release,guards_and_callbacks]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:
  (ROOT/'audit/r10_r8_compat_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R8->R10',len(REPORT))
