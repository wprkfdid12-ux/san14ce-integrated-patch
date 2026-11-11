"""R9 object/context regressions. Real shipped feature + actual Hover release
machine code, captured original selection handler/state tick. Game allocation,
GPU, UI input, rank/cost and widget cleanup endpoints remain substituted.
No claim of live crash reproduction or Windows/game verification.
"""
from test_r6 import R6,settled
from test_native import *
from test_rc4 import PEOPLE

def hover_context_case(e,r):
 # Execute unchanged shipped Hover's +5ce0, not a Python HoverRelease mock.
 # This function has RIP-relative cache access and no imported callees.
 popup=e.alloc(0x198);e.ctor(popup.addr);popup.u32(0x40,1)
 cache=e.ui.addr+0xc020;put(cache,r.addr);put(cache+8,popup.addr);put(cache+0xe8,1,'<I')
 e.registered.add(popup.addr)
 e.mod.val('shared_hover_release',e.ui.addr+0x5ce0,'<Q')
 e.fn('hook_ui_exit',e.root_layout.addr)
 assert get(cache+8)==popup.addr and not get(cache+0xe8,'<I')
 e.fn('hook_row_destroy',r.addr,e.resources.addr)
 received=[ctx for p,ctx in e.destroy_contexts if p==popup.addr]
 assert not get(cache) and not get(cache+8)
 return received

def context_forwarding():
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s)
 received=hover_context_case(e,r)
 check(received==[e.resources.addr],'actual unchanged Hover DLL forwards the exact row resource context to its popup cleanup')
 check(len(e.widget_deletes)==len(set(e.widget_deletes)),'tooltip and owned toolbar receive one delete each')
 check(all(ctx==e.resources.addr for _,ctx in e.destroy_contexts),'all row/toolbar/tooltip cleanup endpoints receive native context')
 e.closeall()

def latest_logged_case():
 e=R6();yun,r,s=e.prepare(596,(88,67,161,4,166,62,153,177,204),lower=2,capacity=3)
 yun.put(0x130,bytes(PEOPLE[595]['unique_tactic_ids']))
 e.seed(16790060193861231533);e.click(r,0);settled(e,s)
 tong,r2=e.second(649);tong.put(0x118,struct.pack('<9H',118,*([0]*8)));tong.put(0xb4,struct.pack('<H',596))
 e.seed(7966506940269555422);e.click(r2,0);settled(e,s)
 check((e.gsui(596).slots[0].result,e.gsui(649).slots[0].result)==(11,8),'latest supplied R8 seeds reproduce Zhao Yun 용장 and Jo Tong 붕벽')
 check(e.root_layout.addr==get(r.addr+8)==get(r2.addr+8) and get(e.root_layout.addr+0x1d8)==e.transition.addr,'native main layout owns both rows and tree; transition controller is a separate 0x198-byte object')
 check(e.selection_counts(596)==(1,2) and e.selection_counts(649)==(1,0),'two lower rewards leave Zhao Yun only one upper choice; Jo Tong is independent')
 return e,yun,tong,r,r2,s

def decisions():
 e,yun,tong,r,r2,s=latest_logged_case()
 # Actual selection context, never synthetic transition->selection alias.
 e.bottom(s)
 check(e.native_commits==1 and e.pop_calls==1 and 8 in e.owned(tong),'Decision reaches one native commit with real-shaped ownership')
 check(e.gs(596).slots[0].phase==3,'full starred officer with explicit no replacement retains prior decline policy')
 check(not e.mod.val('preview_count') and not e.mod.val('exit_modal'),'Decision without replacement never shows Back discard warning')
 e.closeall()
 e,yun,tong,r,r2,s=latest_logged_case()
 e.click(r,0);e.click(r,0);settled(e,s);e.action(2)
 old=get(e.mod.sym('selected_replacement')+596*2,'<H');e.bottom(s);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('warning_kind')==2 and not e.native_commits,'explicit replacement requires its own named Yes/No warning')
 e.answer_exit(s,True)
 check(e.native_commits==1 and 11 in e.owned(yun) and old not in e.owned(yun) and 8 in e.owned(tong),'replacement approval applies each officer own result in one native batch')
 e.closeall()

def cancel_selection():
 e,yun,tong,r,r2,s=latest_logged_case()
 e.click(r,0);e.click(r,0);settled(e,s);e.seed(4761435919079054850);e.action(0);settled(e,s)
 e.click(r2,0);e.click(r2,0);settled(e,s);e.seed(16059306090247300967);e.action(0);settled(e,s)
 check((e.gsui(596).slots[0].result,e.gsui(649).slots[0].result)==(189,69),'latest R8 reroll seeds reproduce 감지 and 산전')
 before=(yun.get(0,0x200),tong.get(0,0x200));transition=e.transition.get(0,0x198)
 e.request_back(s);e.answer_exit(s,False)
 check(e.selection_counts(596)==(1,2) and e.selection_counts(649)==(1,0),'Back No preserves both original menu selections')
 e.request_back(s);e.answer_exit(s,True)
 check(e.selection_counts(596)==(0,2) and e.selection_counts(649)==(0,0),'Back Yes restores the MAIN selection tree for all officers')
 check(e.transition.get(0,0x198)==transition,'selection cancellation never writes inside transition controller')
 check((yun.get(0,0x200),tong.get(0,0x200))==before and not e.native_commits,'Back does not change officer traits/rank/stats or call award commit')
 check(e.gs(596).rerolls_used==e.gs(649).rerolls_used==1 and not e.mod.val('preview_count'),'Back discards outcomes and retains separate spent reroll tickets')
 e.closeall()

FUNCTIONS=[context_forwarding,decisions,cancel_selection]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r9_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R9',len(REPORT))
