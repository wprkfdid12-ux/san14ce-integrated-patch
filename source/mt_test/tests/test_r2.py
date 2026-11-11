"""R2 production Win64 DLL regressions. All native OS/widget endpoints are
substituted; these assertions do NOT certify an actual Windows game session."""
from test_native_integration import EN
from test_rc4 import *

class Evaluation(C.Structure):
 _fields_=[(n,C.c_double) for n in ('value','surprise','rarity','special','score')]+[('grade',C.c_uint32),('version',C.c_uint32)]

from r2_fixture import R2

def immediate_and_separate():
 e=R2();o,r,s=e.prepare();old=o.get(0,0x200);rng=e.rng_calls
 check(len(e.widget_inits)==7,'seven native widgets constructed once after removing top accept/exit')
 check(all(x[5]==e.active_layout.addr for x in e.widget_inits),'all controls belong to the existing native merit layout')
 check(e.control(0) not in [x.addr for x in e.row_buttons] and not e.control(1) and not e.control(8),'reroll is separate; upper accept and exit widgets are not allocated')
 check(e.adapter_enable[e.control(0)]==0,'reroll disabled before a result')
 e.click(r,0)
 check(e.mod.val('ui_mode')==2 and e.rng_calls>rng and e.gsui(596).slots[0].phase==1,'available status zero tile starts roll immediately without bottom confirm')
 check(e.native_commits==0 and o.get(0,0x200)==old,'preview does not spend native merit, rank, gold or alter traits')
 check(e.visibility[e.row_labels[1].addr] and e.visibility[e.row_labels[2].addr],'second and third locked reward tiles preserve their native lock labels')
 check(e.texts[e.row_buttons[1].addr]=='' and e.texts[e.row_buttons[2].addr]=='','locked tiles never display reroll/accept captions')
 rng=e.rng_calls;calc=e.mod.val('calculation_count',fmt='<Q');e.click(r,0);e.click(r,1);e.action(0);e.accept_preview()
 check(e.rng_calls==rng and e.mod.val('calculation_count',fmt='<Q')==calc,'double click, other tile, reroll and accept blocked during animation')
 e.action(4)
 check('추첨 연출 중' in e.detail() and '선택확률' not in e.detail() and '이번 가챠 평가' not in e.detail(),'diagnostic expansion hides result/probability/evaluation until reveal')
 for i in range(12):e.action(4);e.action(4);e.action(7);e.action(6)
 check(e.rng_calls==rng and e.mod.val('calculation_count',fmt='<Q')==calc,'diagnostic expansion and scrolling consume no RNG or candidate calculation')
 e.reveal(s);result=e.gsui(596).slots[0].result
 check(e.mod.val('ui_mode')==3 and e.adapter_enable[e.control(0)],'1.2 second reveal enables separate reroll')
 check('선택확률' in e.detail() and '이번 가챠 평가' in e.detail(),'cached post-reveal diagnostics include probability and six-level assessment')
 e.action(0);check(e.gsui(596).rerolls_used==1 and e.mod.val('ui_mode')==2,'separate reroll button consumes one shared reroll')
 e.reveal(s);result=e.gsui(596).slots[0].result;e.accept_preview()
 check(e.mod.val('ui_mode')==0 and e.gsui(596).slots[0].phase==2 and result not in e.owned(o),'toolbar acceptance records intention; original bottom decision is final transaction')
 e.click(r,0);e.keys={1};e.fn('hook_state_tick',s.addr);e.keys=set();e.fn('hook_state_tick',s.addr)
 check(not e.talks and not e.dialogs and not e.pop_calls and e.native_commits==0,'completed tile click and button release never invoke thanks or close')
 before=e.fn('hook_save_prep',0,0,0,0,0);check(before==0 and not e.native_writes,'uncommitted UI result cannot be saved as mismatching native metadata')
 calc=e.mod.val('calculation_count',fmt='<Q');e.bottom(s)
 check(e.native_commits==1 and result in e.owned(o) and e.gs(596).slots[0].phase==2,'bottom decision commits native award and actual trait once')
 check(e.dialogs==1 and not e.talks and e.pop_calls==1,'original confirmation owns portrait dialogue and one native state return')
 check(e.mod.val('calculation_count',fmt='<Q')==calc,'bottom decision does not recalculate candidates')
 e.fn('hook_confirm',s.addr);e.click(r,0)
 check(e.native_commits==1 and e.dialogs==1 and e.pop_calls==1,'repeated confirm and tile click cannot duplicate finalization')
 e.fn('hook_row_destroy',r.addr,e.resources.addr)
 check(all(ctx==e.resources.addr for _,ctx in e.destroy_contexts),'owned widget and original row cleanup receive the exact original resource argument')
 check(e.destroy_contexts[-1][0]==r.addr and len(e.widget_deletes)==7,'controls released once before original row resource cleanup')
 check(not BR.errors,'new callback/widget/cleanup ABI boundaries have no stub errors')
 e.closeall()

def locked_and_reentry():
 e=R2();o,r,s=e.prepare(top=1);e.status_override=[2,3,3,0,3,3];rng=e.rng_calls;old=o.get(0,0x200)
 e.click(r,1);e.click(r,2)
 check(e.rng_calls==rng and not e.mod.val('preview_count') and o.get(0,0x200)==old,'locked stages cannot start a preview or change real reward flags')
 e.click(r,0);e.reveal(s);pending=bytes(e.gsui(596));rng=e.rng_calls
 e.fn('hook_ui_exit',e.active_layout.addr);e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr);e.click(r,0)
 check(e.rng_calls>rng and e.gsui(596).slots[0].phase==1 and e.mod.val('ui_mode')==2,'layout exit cancels old preview; reopening starts a fresh uncommitted chance')
 e.reveal(s)
 e.accept_preview();e.bottom(s)
 check(e.native_commits==0 and len(e.talks)==1 and e.pop_calls==1,'historically unlocked reward finalizes with one original speaker message and no second merit cost')
 e.fn('hook_confirm',s.addr);check(len(e.talks)==1 and e.pop_calls==1,'historical reward cannot repeat gratitude on duplicate bottom callback')
 e.closeall()

def capacity_and_failures():
 e=R2();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),top=1);e.click(r,0)
 check(e.mod.val('ui_mode')==1 and e.gsui(1).slots[0].phase==0,'ordinary full officer chooses inheritance before any RNG')
 rng=e.rng_calls;e.action(2);chosen=get(e.mod.sym('selected_replacement')+2,'<H');e.action(3)
 check(e.rng_calls>rng and e.mod.val('ui_mode')==2,'dedicated inheritance action starts draw after selecting old trait')
 old=e.owned(o);e.reveal(s);result=e.gsui(1).slots[0].result;e.accept_preview()
 check(e.owned(o)==old,'inheritance is not destructively applied before bottom decision')
 e.bottom(s);check(result in e.owned(o) and chosen not in e.owned(o),'bottom decision replaces exactly chosen ordinary trait')
 e.closeall()
 e=R2();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),top=1);o.put(0x130,bytes([31]));old=e.owned(o);e.click(r,0);e.reveal(s)
 check(e.mod.val('ui_mode')==3,'live unique tactic permits result-first star capacity flow')
 e.accept_preview();e.bottom(s);check(e.owned(o)==old and e.gs(1).slots[0].phase==3,'star decline consumes one chance and preserves all incumbent traits')
 e.closeall()
 e=R2();o,r,s=e.prepare();e.rng_fail=True;e.click(r,0)
 check(e.native_commits==0 and b(o.addr+0x1c2)==0 and e.mod.val('preview_count')==0,'entropy failure leaves real merit and traits untouched')
 e.closeall()

def lifecycle_guards():
 e=R2();o,r,s=e.prepare();e.click(r,0);e.reveal(s);e.accept_preview();rng=e.rng_calls;pending=bytes(e.gsui(596))
 e.selected_upper[596]=0;e.bottom(s)
 check(e.native_commits==0 and not e.pop_calls and bytes(e.gsui(596))==pending,'deselected native award blocks finalization before fees or trait writes')
 e.selected_upper[596]=1;e.fail_commit=True;e.bottom(s)
 check(e.gs(596).slots[0].phase==0 and bytes(e.gsui(596))==pending and not e.pop_calls,'native transaction failure retains preview and prevents false completion')
 e.fail_commit=False;e.bottom(s)
 check(e.pop_calls==1 and e.gs(596).slots[0].phase==2 and e.rng_calls==rng,'retry commits the existing result without a new roll or extra entropy')
 # Reusing the original native state pointer after a layout exit is legitimate.
 e.active_layout.u32(0x180,0);e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr);e.click(r,1);e.reveal(s);e.accept_preview();e.bottom(s)
 check(e.gs(596).slots[1].phase==2 and e.pop_calls==2,'new session may reuse the same native state address and award its next unlocked stage')
 e.closeall()
 e=R2();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),top=1);old=o.get(0,0x200);rng=e.rng_calls;e.click(r,0);stale=e.widget_callbacks[e.control(2)];e.cancel_preview();e.fn('hook_state_tick',s.addr);e.answer_exit(s)
 check(e.mod.val('ui_mode')==0 and not e.mod.val('preview_count') and o.get(0,0x200)==old and e.rng_calls==rng,'inheritance cancel restores idle with no trait changes or RNG')
 e.fn('hook_ui_exit',e.active_layout.addr);e.fn('control_invoke',stale.addr,0)
 check(e.rng_calls==rng and not e.mod.val('ui_mode'),'stale owned callback after layout exit is harmless')
 e.fn('hook_row_destroy',r.addr,e.resources.addr);e.fn('hook_row_destroy',r.addr,e.resources.addr)
 check(len(e.widget_deletes)==7,'repeated row cleanup cannot destroy owned controls twice')
 e.closeall()
 e=R2();o,r,s=e.prepare(top=3);results=[]
 for stage in [2,0,1]:
  e.click(r,stage);e.reveal(s)
  if stage==2:
   for _ in range(5):e.action(0);e.reveal(s)
  results.append(e.gsui(596).slots[stage].result);e.accept_preview()
 check(e.gsui(596).rerolls_used==5 and all(x.phase==2 for x in e.gsui(596).slots),'all three original tiles can be completed in reverse order sharing exactly five rerolls')
 e.bottom(s);check(all(x in e.owned(o) for x in results) and len(e.talks)==1 and e.pop_calls==1,'three historical results apply once with one portrait thanks and one return')
 e.closeall()

def original_database_stats():
 e=R2();o,r,s=e.prepare(id=619,owned=(32,),top=1);other=e.officer(1,(100,1,100,1,100),(),0);t=Target();state=e.gs(619);a=Distribution();b_=Distribution()
 axes=('leadership','war','intelligence','politics','charisma');original=[PEOPLE[618][k] for k in axes]
 e.fn('target_read',o.addr,C.addressof(t));e.fn('g_distribution',C.addressof(t),C.addressof(state),C.addressof(COLORS),0,0,C.addressof(a))
 check(list(t.stats)==[65,68,53,51,60]==original,'Zhao Guang uses his exact original official database ability vector')
 edited=[99,98,97,96,95];o.put(0xec,bytes(edited));other.put(0xec,bytes([1,100,1,100,1]));e.fn('target_read',o.addr,C.addressof(t));e.fn('g_distribution',C.addressof(t),C.addressof(state),C.addressof(COLORS),0,0,C.addressof(b_))
 check(list(t.stats)==original and bytes(a)==bytes(b_),'editing or growing target and comparison officers leaves DB-based similarities and probabilities unchanged')
 check(list(o.get(0xec,5))==edited,'reading gacha comparison data never overwrites edited in-game abilities')
 check(all(list(a.similar_stats[i])==[PEOPLE[q.id-1][k] for k in axes] for i,q in enumerate(a.similar) if q.id and q.id<=1000),'comparison snapshot uses original database vectors for other officers too')
 e.random_values=[123,456];e.click(r,0);e.reveal(s);e.action(4);text=e.detail().replace('\n','')
 check('65/68/53/51/60' in text and 'DB 원본' in text and '99/98/97/96/95' not in text,'foldout displays original comparison inputs rather than edited target abilities')
 e.accept_preview();e.bottom(s)
 check(list(o.get(0xec,5))==edited,'full gacha finalization preserves edited in-game abilities exactly')
 custom=e.officer(5001,(92,14,88,18,50),(),1);e.fn('target_read',custom.addr,C.addressof(t))
 check(list(t.stats)==[92,14,88,18,50] and t.category==3,'custom officer without an official DB row has an explicit current-base fallback')
 e.closeall()

def grades():
 e=E();t=target();state=State();dist=Distribution();e.fn('g_init',C.addressof(state),C.addressof(t.traits));assert call_roll(e,t,state,0,19,0,0,dist)==1
 a=Evaluation();b_=Evaluation();before=bytes(dist)+bytes(state);rng=e.rng_calls
 e.fn('g_evaluate',C.addressof(dist),C.addressof(state.slots[0]),C.addressof(a));e.fn('g_evaluate',C.addressof(dist),C.addressof(state.slots[0]),C.addressof(b_))
 check(bytes(a)==bytes(b_) and bytes(dist)+bytes(state)==before and e.rng_calls==rng,'six-level observation is deterministic and cannot alter RNG or draw weights')
 # A scalar-double call needs a real MS-ABI bridge, not the integer dispatcher.
 src=ROOT/'tests/grade_bridge.c';src.write_text('#include <stdint.h>\nuint32_t grade(uint64_t f,double d){return ((uint32_t(__attribute__((ms_abi))*)(double))f)(d);}')
 so=src.with_suffix('.so');subprocess.run(['gcc','-shared','-fPIC','-O2',str(src),'-o',str(so)],check=True);lib=C.CDLL(str(so));lib.grade.argtypes=[C.c_uint64,C.c_double];lib.grade.restype=C.c_uint32
 for v,want in [(0,0),(24.999,0),(25,1),(44.999,1),(45,2),(64.999,2),(65,3),(81.999,3),(82,4),(94.999,4),(95,5),(100,5)]:check(lib.grade(e.mod.sym('g_grade'),v)==want,'evaluation threshold '+str(v))
 state.slots[0].quality=0;e.fn('g_evaluate',C.addressof(dist),C.addressof(state.slots[0]),C.addressof(a));check(a.grade<=2,'poor-fit rarity cannot rise above 의외 without the value gate')
 e.closeall()

if __name__=='__main__':
 try:
  for f in [immediate_and_separate,locked_and_reentry,capacity_and_failures,lifecycle_guards,original_database_stats,grades]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r2_tests.json').write_text(json.dumps({'scope':'Actual production Win64 DLL under substituted native OS/game/widgets; not Windows or live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R2',len(REPORT))
