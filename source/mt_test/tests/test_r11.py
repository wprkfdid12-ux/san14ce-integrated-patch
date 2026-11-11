"""R11 compiled Win64 DLL and original CE rule/commit/fade code regressions.
Native captured code is real; OS, graphics, member enumeration and manager
endpoints are substituted. This is NOT a live Windows/SAN14 certificate.
"""
from r11_fixture import *
from test_r10 import collab_39_candidates, ai_upper_first_and_full_fallback

def native_rules():
 e=EN();o=e.officer(596);o.put(0xe6,b'\x01');o.put(0x1be,struct.pack('<H',9999));force_ruler(e,596)
 install_native_rules(e);e.fn('hook_newgame',0,0)
 check(BR.call(e.game.addr+0x232310,o.addr,0)==1,'original membership filter includes status-1 ruler')
 check(BR.call(get(e.mod.sym('original_merit_eligible')),o.addr)==0,'unmodified native candidate/commit predicate rejects ruler: negative control')
 check(BR.call(e.game.addr+0x1eed10,o.addr)==1,'patched native candidate AND commit predicate accepts ruler with remaining rewards')
 check(BR.call(e.game.addr+0x3fb80,o.addr)==3,'original native capacity function returns three for ruler')
 check(BR.call(e.game.addr+0x3fa90,o.addr)==6,'native current-rank getter reports senior for ruler, consistent with selection capacity')
 for flag in (0x4000,0x8000):
  o.put(0x15e,struct.pack('<H',flag));check(BR.call(e.game.addr+0x1eed10,o.addr)==0,'ruler does not bypass native busy/acted restriction '+hex(flag))
 o.put(0x15e,b'\0\0');zero(e,0x232640);e.game.put(0x232640,b'\xb8\x01\0\0\0\xc3')
 check(BR.call(e.game.addr+0x1eed10,o.addr)==0,'native special-action restriction retained');zero(e,0x232640)
 ordinary=e.officer(619);ordinary.put(0xe6,b'\x04');e.fn('capture_profiles',2)
 check(not e.profile(619).reserved[0]&1 and BR.call(e.game.addr+0x1eed10,ordinary.addr)==0,'ordinary low-rank officer is not promoted into ruler privileges')
 rng=e.rng_calls;owned=e.owned(o)
 for _ in range(100):e.fn('hook_ai_visit',o.addr)
 check(e.rng_calls==rng and e.owned(o)==owned and not b(o.addr+0x1c2),'100 candidate visits do not draw, consume tickets or award traits')
 o.put(0x1c2,b'\x01\x02');e.fn('sync_merit_history',o.addr)
 check(BR.call(e.game.addr+0x1eed10,o.addr)==0,'mixed one upper/two domestic consumes exactly three and excludes further merit')
 e.closeall()

def ruler_native_ui_and_commit():
 e=NativeR11UI();o,row,state=e.prepare(id=952,owned=(97,59,88,21,66),capacity=1)
 e.setup_rules();force_ruler(e,952);o.put(0xe6,b'\x01');e.fn('hook_ui_enter',e.root_layout.addr);e.fn('hook_merit_eligible',o.addr)
 e.refresh_choices(e.list_owner.addr)
 out=(C.c_int*6)();BR.call(e.game.addr+0x6d90f0,e.list_owner.addr,o.addr,C.addressof(out))
 check(list(out)==[1,0,0,1,0,0],'original native row-status code exposes ruler three-choice prefix without Python capacity override')
 for j in range(3):e.click(row,j);settled(e,state)
 check(e.capacities[952]==1 and e.decision_enabled and e.selection_counts(952)==(3,0),'native rank-table capacity enables Decision despite deliberately unchanged legacy fixture cap1')
 e.setup_native_commit();e.bottom(state)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3),b(o.addr+0x1bc))==(3,0,6),'actual native 1ebca0 commit and 1ebc60 advance accept ruler and clamp rank6')
 check(e.profile(952).award_mask==7 and all(e.gs(952).slots[j].phase==2 for j in range(3)) and e.pop_calls==1,'ruler three gacha results finalized and original state popped once')
 check(b(e.force_cost.addr+0x14)==99,'actual native command cost charged once, not bypassed or duplicated')
 e.closeall()

def successful_exit_once():
 for mode in ('trait','domestic'):
  e=FadeR11();o,row,state=e.prepare(id=771,owned=(4,25,33,126,57),lower=1 if mode=='domestic' else 0,capacity=1);e.strict_fade(state)
  if mode=='trait':e.click(row,0);settled(e,state)
  e.bottom(state)
  check(e.native_commits==1 and e.pop_calls==1 and e.cancel_invocations==0,mode+': native success returns true and pops once even with disabled Cancel')
  check(get(e.root_layout.addr+0x180,'<I')==1 and not e.mod.val('closing_layout',fmt='<Q'),mode+': success marker retained and no cancel fence installed')
  e.finish_original_fade()
  for _ in range(10):e.fn('hook_state_tick',state.addr)
  e.fn('hook_native_exit_event',state.addr)
  check(e.pop_calls==1 and e.native_commits==1,mode+': actual fade completion, repeated ticks and late merit event cannot pop twice')
  e.closeall()
 # Native negative control: ORIGINAL completion emits an extra exit if old R9 reset set 180=0.
 e=FadeR11();o,row,state=e.prepare();e.strict_fade(state);e.pop_calls=1;e.root_layout.u32(0x180,0)
 e.finish_original_fade();check(e.pop_calls==2,'native fade negative control reproduces duplicate exit when original success marker is cleared')
 e.closeall()

def durable_history():
 e=EN();install_native_rules(e);o=e.officer(771,top=1,bottom=2);o.put(0xe6,b'\x01');force=force_ruler(e,771);e.fn('hook_newgame',0,0)
 # Existing one upper award materialized only for the fixture; source history not guessed.
 check(e.fn('perform_roll',o.addr,0,0,1)==1 and e.fn('finish_result',o.addr,0,0,1)==1,'fixture has one real finalized upper reward and two native domestic rewards')
 original=bytes(e.gs(771));traits=e.owned(o);force.put(e.res.native.offsets[5],b'\0\0');o.put(0xe6,b'\x04');o.put(0x1bc,b'\0');o.put(0x1c2,b'\0\0')
 e.fn('sync_merit_history',o.addr)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3),b(o.addr+0x1bc))==(1,2,6),'former ruler restores both spent reward types and senior rank after status reset')
 check(bytes(e.gs(771))==original and e.owned(o)==traits and e.profile(771).reserved[0]&1,'restoration preserves actual trait, exact gacha state and former-ruler marker')
 check(e.save(b'R11 durable mixed merit history')==1,'v3 sidecar serializes durable mixed ledger')
 o.put(0x1c2,b'\0\0');o.put(0x1bc,b'\0')
 check(e.loadsave(b'R11 durable mixed merit history')==1 and not e.mod.val('metadata_fault'),'strict save hash/account/slot roundtrip survives native rank-count drop')
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(1,2) and bytes(e.gs(771))==original,'load restores paid counts without duplicate trait or fresh ticket budget')
 e.profile(771).reserved[1]=0x33;e.fn('sync_merit_history',o.addr)
 check(e.mod.val('metadata_fault')==0 and e.profile(771).reserved[1]==(1|(2<<4)),'R40: corrupt six-reward ledger rebuilt from native counts (no free refill), no fault')
 e.closeall()

def ai_full_and_visits():
 for size in (8,9):
  e=EN();install_native_rules(e);o=e.officer(596,owned=tuple(range(4,4+size)));o.put(0xe6,b'\x01');force_ruler(e,596);e.fn('hook_newgame',0,0)
  # Substituted batch cost endpoint follows native rank clamping; no invalid rank7.
  oldcommit=BR.callbacks['NativeCommit']
  def clamped(ptr):
   v=oldcommit(ptr);o.put(0x1bc,b'\x06');return v
  BR.callbacks['NativeCommit']=clamped
  batch=e.batch([(596,0,0)]);before=e.owned(o);rng=e.rng_calls
  check(e.fn('hook_commit_ai',batch.addr)==1,'ruler AI real commit boundary accepts '+str(size)+' occupied slots')
  check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(9-size,3-(9-size)),'AI allocates only free physical slots and rest domestic: '+str(size))
  check(all(t in e.owned(o) for t in before if t),'AI never replaces incumbent trait at '+str(size)+'/9')
  if size==9:check(e.rng_calls==rng and not e.profile(596).award_mask,'full ruler consumes zero RNG and receives no hidden replacement')
  else:check(e.profile(596).award_mask==1 and sum(bool(t) for t in e.owned(o))==9,'8->9 recheck prevents second and third trait acquisition')
  e.closeall()
 e=EN();install_native_rules(e);o=e.officer(619,top=1);e.fn('hook_newgame',0,0);rng=e.rng_calls;before=e.owned(o)
 for _ in range(200):e.fn('hook_ai_visit',o.addr)
 check(e.rng_calls==rng and e.owned(o)==before and e.gs(619).slots[0].phase==0,'legacy unprocessed upper reward is not silently rolled by candidate scanning')
 e.mod.val('active_state',o.addr,'<Q');batch=e.batch([(619,2,0)])
 check(e.fn('hook_commit_ai',batch.addr)==0 and e.native_commits==0,'AI callback cannot commit while player merit UI is active')
 e.closeall()


def empty_and_internal_notifications():
 e=NativeR11UI();o,row,state=e.prepare();e.setup_rules();e.setup_native_commit()
 e.choice_nodes={};e.real_rows=[];e.decision_enabled=True
 rng=e.rng_calls;e.bottom(state)
 check(e.pop_calls==0 and e.rng_calls==rng and b(e.force_cost.addr+0x14)==100,'empty native batch spends no cost/RNG and does not pop underlying command state')
 check(get(e.root_layout.addr+0x180,'<I')==0,'failed/empty native Decision returns to retryable UI state instead of success fence')
 e.closeall()
 e=R6();o,row,state=e.prepare();rng=e.rng_calls;before=e.owned(o)
 old=BR.callbacks['RowRefresh'];injected=[]
 def refresh(address):
  if not injected:injected.append(True);e.click(row,0)
  return old(address)
 BR.callbacks['RowRefresh']=refresh;e.fn('hook_row_refresh',row.addr)
 check(e.rng_calls==rng and not e.mod.val('preview_count') and e.selection_counts(596)==(0,0),'button event injected during internal row refresh cannot create preview or select a reward')
 check(e.owned(o)==before and e.native_commits==0,'internal notification does not award/cost/change current traits')
 BR.callbacks['RowRefresh']=old;e.click(row,0)
 check(e.mod.val('preview_count')==1,'fresh normal native reward event still works after internal-notification guard')
 e.closeall()

FUNCTIONS=[empty_and_internal_notifications,native_rules,ruler_native_ui_and_commit,successful_exit_once,durable_history,ai_upper_first_and_full_fallback,ai_full_and_visits,collab_39_candidates]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r11_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R11',len(REPORT))
