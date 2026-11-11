"""R13 selected-only reroll on actual Win64 DLL / captured native click+commit.
Synthetic OS/UI world endpoints; NOT live Windows or full game FPS certification.
"""
from r11_fixture import *
from test_r3 import miss_seed
from test_rc4 import State, Distribution

def open_three(native=True, owned=(97,59,88,21,66), first_seed=None):
 e=NativeR11UI() if native else R6()
 o,row,state=e.prepare(id=952,owned=owned,capacity=1 if native else 3)
 if native:
  e.setup_rules();force_ruler(e,952);o.put(0xe6,b'\x01');e.fn('hook_ui_enter',e.root_layout.addr);e.fn('hook_merit_eligible',o.addr);e.refresh_choices(e.list_owner.addr)
 for j in range(3):
  if j==0 and first_seed is not None:e.seed(first_seed(e) if callable(first_seed) else first_seed)
  e.click(row,j)
  if e.mod.val('ui_mode')==1:e.action(3)
  settled(e,state)
 assert e.selection_counts(952)==(3,0)
 return e,o,row,state

def focus(e,row,j):
 e.click(row,j)
 if e.selection_counts(952)!=(3,0):e.click(row,j) # existing tail native radio toggles

def fixed_key(x):
 return (x.result,x.seed,x.roll_id,x.rerolls,x.outcome,x.raw,x.weight,x.quality,x.cost,x.anomaly,x.transcend,x.passion_applied)

def same_officer_all_slots():
 e,o,row,state=open_three();owned=e.owned(o);initial=[fixed_key(x) for x in e.gsui(952).slots]
 for number,j in enumerate((0,1,2,0,1),1):
  snapshots=[fixed_key(x) for x in e.gsui(952).slots];rng=e.rng_calls;focus(e,row,j)
  check(e.selection_counts(952)==(3,0) and e.rng_calls==rng,'focus stage %d preserves all three native selections without draw #%d'%(j+1,number))
  check(e.mod.val('active_stage',fmt='<B')==j and e.adapter_enable[e.control(0)]==1,'stage %d reroll control enabled before final Decision #%d'%(j+1,number))
  e.seed(1000+number);e.action(0);settled(e,state);ui=e.gsui(952)
  check(e.gs(952).rerolls_used==number and ui.rerolls_used==number,'one shared ticket spent for selected stage #%d'%number)
  check(all(fixed_key(ui.slots[k])==snapshots[k] for k in range(3) if k!=j),'other two result/seed/roll ID/odds/reroll metadata unchanged #%d'%number)
  d=Distribution.from_address(e.mod.sym('distribution'))
  if j<2:check(all(d.weight[ui.slots[k].result]==0 for k in range(j+1,3) if ui.slots[k].outcome==0),'later fixed trait results excluded before selected redraw #%d'%number)
  check(ui.slots[j].seed==1000+number and ui.slots[j].roll_id==snapshots[j][2],'exactly selected roll refreshed, original roll ID retained #%d'%number)
  check(e.selection_counts(952)==(3,0) and e.owned(o)==owned and b(o.addr+0x1c2)==0,'no native award or prefix truncation until Decision #%d'%number)
  check(e.fn('g_validate',C.addressof(ui)) and e.fn('g_validate',C.addressof(e.gs(952))),'preview and persistent ticket ledgers valid #%d'%number)
 rng=e.rng_calls;allui=bytes(e.gsui(952));e.action(0)
 check(e.rng_calls==rng and bytes(e.gsui(952))==allui and not e.adapter_enable[e.control(0)],'sixth total reroll rejected without entropy or result mutation')
 e.setup_native_commit();e.bottom(state)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3),b(o.addr+0x1bc))==(3,0,6),'actual native Liu Bei commit keeps rank6 and total three rewards')
 check(e.profile(952).award_mask==7 and e.gs(952).rerolls_used==5 and e.gs(952).cancelled_rerolls==0,'commit transfers reserved tickets into selected slots exactly once')
 check(e.pop_calls==1 and b(e.force_cost.addr+0x14)==99,'original close and command cost occur exactly once')
 committed=bytes(e.gs(952));check(e.save(b'R13 committed edited three slots')==1,'committed edited three-slot state saves without resetting ticket or outcomes')
 check(e.loadsave(b'R13 committed edited three slots')==1 and bytes(e.gs(952))==committed,'all final selected results and five-ticket ledger restore byte-for-byte')
 check(e.fn('g_validate',C.addressof(e.gs(952))) and all(x.phase==2 for x in e.gs(952).slots),'all committed stages pass persistent state validation')
 rng=e.rng_calls;nativebytes=o.get(0,0x200)
 for j in range(3):e.fn('perform_roll',o.addr,j,1,0)
 check(e.rng_calls==rng and o.get(0,0x200)==nativebytes,'finalized slots cannot be reopened through direct reroll entry')
 e.closeall()

def miss_replay_and_cancel():
 e,o,row,state=open_three(first_seed=miss_seed)
 check(e.gsui(952).slots[0].outcome==1 and e.gsui(952).x_mask,'fixture begins with first revealed MISS plus fixed later rewards')
 held=[fixed_key(x) for x in e.gsui(952).slots];focus(e,row,0);e.seed(1001);e.action(0);settled(e,state)
 check(e.gsui(952).slots[0].outcome==0 and e.gsui(952).x_mask==0,'earlier MISS reroll to trait removes only provisional X')
 check(all(fixed_key(e.gsui(952).slots[k])==held[k] for k in (1,2)),'MISS-to-trait keeps both later actual draw snapshots')
 check(e.fn('g_validate',C.addressof(e.gsui(952))) and not e.gs(952).x_mask,'MISS recomposition valid without native X leakage')
 focus(e,row,0);seed=miss_seed(e);e.seed(seed);e.action(0);settled(e,state)
 check(e.gsui(952).slots[0].outcome==1 and e.gsui(952).x_mask,'earlier trait can reroll back to MISS when replay remains feasible')
 check(e.gs(952).rerolls_used==2 and all(fixed_key(e.gsui(952).slots[k])==held[k] for k in (1,2)),'opposite MISS transition consumes one ticket and holds both later results')
 original=e.owned(o);e.request_back(state);e.answer_exit(state,True)
 check(e.gs(952).rerolls_used==2 and e.gs(952).cancelled_rerolls==2 and not e.gs(952).x_mask and e.owned(o)==original,'cancel discards all provisional results/X but never refunds two rerolls')
 check(e.fn('g_validate',C.addressof(e.gs(952))),'cancelled earlier-slot ticket state persistable')
 check(e.save(b'R13 cancelled earlier rerolls')==1,'cancelled ticket reservations save with unchanged v3 wire format')
 check(e.loadsave(b'R13 cancelled earlier rerolls')==1 and e.gs(952).rerolls_used==2,'cancelled reroll count survives save/load without refund')
 e.closeall()

def reject_changed_world_and_entropy():
 e,o,row,state=open_three();focus(e,row,0)
 before=bytes(e.gsui(952));ledger=bytes(e.gs(952));rng=e.rng_calls
 e.rng_fail=True;e.action(0);settled(e,state)
 check(bytes(e.gsui(952))==before and bytes(e.gs(952))==ledger,'entropy failure leaves all revealed outcomes and ticket reservations unchanged')
 e.rng_fail=False
 # Stored base state changed by another valid event: stale preview must not edit.
 e.gs(952).rerolls_used=1;e.gs(952).cancelled_rerolls=1;rng=e.rng_calls
 e.action(0)
 check(e.rng_calls==rng and bytes(e.gsui(952))==before,'stale persistent-ledger snapshot rejects edit before drawing')
 e.gs(952).rerolls_used=e.gs(952).cancelled_rerolls=0
 e.mod.val('metadata_fault',1);rng=e.rng_calls;e.action(0)
 check(e.rng_calls==rng,'metadata fault cannot be bypassed by selecting an earlier preview')
 e.closeall()

def replacement_chain_safety():
 e,o,row,state=open_three(native=False,owned=(4,5,6,7,8,9,10,11,12));old=e.owned(o)
 held=[fixed_key(x) for x in e.gsui(952).slots];focus(e,row,0);e.seed(333);e.action(0);settled(e,state)
 check(e.gs(952).rerolls_used==1 and e.owned(o)==old,'full-star early preview reroll spends only ticket, never immediately replaces native trait')
 check(all(fixed_key(e.gsui(952).slots[k])==held[k] for k in (1,2)),'full-capacity early reroll holds later result and rarity metadata')
 check(e.fn('g_validate',C.addressof(e.gsui(952))) and e.fn('prepare_previews')==1,'full-capacity recomposition is valid through final replacement preparation')
 e.bottom(state);e.fn('hook_state_tick',state.addr)
 check(e.mod.val('exit_modal') and not e.native_commits,'changed full-capacity result still requires original replacement consent')
 for _ in range(4):
  if not e.mod.val('exit_modal'):break
  e.answer_exit(state,True);e.fn('hook_state_tick',state.addr)
 check(e.native_commits==1 and e.pop_calls==1 and e.gs(952).rerolls_used==1,'replacement consent finalizes the recomposed batch only once')
 e.closeall()

def ai_succession_no_global_scan():
 e=EN();install_native_rules(e);o=e.officer(596,owned=tuple(range(4,12)));o.put(0xe6,b'\x04');e.fn('hook_newgame',0,0)
 o.put(0xe6,b'\x01');force_ruler(e,596)
 oldcommit=BR.callbacks['NativeCommit']
 def clamped(ptr):
  v=oldcommit(ptr);o.put(0x1bc,b'\x06');return v
 BR.callbacks['NativeCommit']=clamped
 check(e.fn('hook_commit_ai',e.batch([(596,0,0)]).addr)==1,'AI first batch recognizes a just-succeeded ruler without prior candidate scan')
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(1,2) and e.profile(596).reserved[0]&1,'fresh ruler retains capacity3, fills final trait cell, then two domestic rewards')
 e.closeall()

def mixed_officers_and_repeat_seeds():
 e=R6();o,r,s=e.prepare(id=649,capacity=3);o2,r2=e.second(id=619,capacity=3)
 for row in (r,r2):
  for j in range(3):e.click(row,j);settled(e,s)
 other=bytes(e.gsui(619));e.click(r,0);e.seed(1033);e.action(0);settled(e,s)
 check(bytes(e.gsui(619))==other and e.gs(649).rerolls_used==1 and not e.gs(619).rerolls_used,'editing officer A first slot does not touch officer B three revealed outcomes or five tickets')
 held=bytes(e.gsui(649));e.click(r2,1);e.seed(2047);e.action(0);settled(e,s)
 check(bytes(e.gsui(649))==held and e.gs(619).rerolls_used==1,'editing officer B second slot leaves officer A unchanged')
 e.bottom(s)
 check(e.native_commits==1 and e.pop_calls==1 and all(e.fn('g_validate',C.addressof(e.gs(i))) for i in (649,619)),'two independently edited three-slot previews commit once through native batch')
 e.closeall()
 # Deterministic seeds at all selected stages, including naturally occurring
 # MISS, replay into two held outcomes. Assertions check actual resulting DLL
 # state, not an independently reimplemented engine or forced expected outcome.
 for seed in range(12):
  e,o,r,s=open_three(native=False,first_seed=seed)
  for turn,j in enumerate((0,1,2,0,1)):
   fixed=[fixed_key(x) for x in e.gsui(952).slots];focus(e,r,j);before=e.gs(952).rerolls_used
   e.seed(seed*50+turn);e.action(0);settled(e,s)
   assert e.gs(952).rerolls_used==before+1 and e.fn('g_validate',C.addressof(e.gsui(952)))
   assert all(fixed_key(e.gsui(952).slots[k])==fixed[k] for k in range(3) if k!=j)
  e.bottom(s)
  check(e.pop_calls==1 and e.native_commits==1 and e.gs(952).rerolls_used==5 and e.fn('g_validate',C.addressof(e.gs(952))),'five mixed-stage edits preserve held outcomes and commit valid ledger, deterministic set %d'%seed)
  e.closeall()

FUNCTIONS=[same_officer_all_slots,miss_replay_and_cancel,reject_changed_world_and_entropy,replacement_chain_safety,ai_succession_no_global_scan,mixed_officers_and_repeat_seeds]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r13_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R13',len(REPORT))
