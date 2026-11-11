"""R10 regressions for handoff NEXT-001..NEXT-005.
Executes the rebuilt production Win64 DLL on Linux through the existing
Microsoft-x64 test bridge with synthetic game/OS boundaries. It does NOT claim
Windows/GPU/live SAN14 CE verification.
"""
from test_r6 import R6, settled
from test_native_integration import EN
from test_rc4 import E, Target, State, Distribution, COLORS, PEOPLE
from test_cp03 import finalized_history_survives_rank_drop
from test_native import *

COLLAB_IDS=list(range(155,181))+list(range(197,201))+list(range(202,211))
COLLAB_DUPLICATE_HOLDERS={155,161,162,163,164}

def same_turn_return():
 # Trait-preview path: the captured original 0x732f50 must not observe a true
 # hook_confirm return and therefore must not execute its Manager->PopState tail.
 e=R6();o,r,s=e.prepare(capacity=1);manager=[0]
 BR.callbacks['Manager']=lambda: manager.__setitem__(0,manager[0]+1) or 0x1234
 e.click(r,0);settled(e,s);result=e.gsui(596).slots[0].result
 e.bottom(s)
 check(e.native_commits==1 and e.gs(596).slots[0].phase==2 and result in e.owned(o),'NEXT-001 preview decision still commits the selected trait exactly once')
 check(manager[0]==0 and e.cancel_invocations==1 and e.pop_calls==1 and e.consumed==1,'NEXT-001 preview decision returns through verified native ESC tail, not outer Manager->PopState')
 check(not e.mod.val('preview_count') and any('SAME_TURN_RETURN_VIA_NATIVE_CANCEL' in x for x in e.logs),'NEXT-001 preview return clears owned UI and records protected same-turn route')
 e.closeall()
 # Native-only path (e.g. already-selected domestic reward) needs the same guard.
 e=R6();o,r,s=e.prepare(lower=1,capacity=1);manager=[0]
 BR.callbacks['Manager']=lambda: manager.__setitem__(0,manager[0]+1) or 0x1234
 check(e.mod.val('preview_count')==0 and e.decision_enabled,'NEXT-001 fixture has a valid native-only merit decision')
 e.bottom(s)
 check(e.native_commits==1 and manager[0]==0 and e.cancel_invocations==1 and e.pop_calls==1 and e.consumed==1,'NEXT-001 native-only decision also blocks the outer turn-transition tail')
 e.closeall()

def ai_upper_first_and_full_fallback():
 # Preserve native total choice count, but spend new choices on free trait slots first.
 e=EN();p=PEOPLE[618];o=e.officer(619,tuple(p[k] for k in ('leadership','war','intelligence','politics','charisma')),(32,),0,0);e.fn('hook_newgame',0,0)
 batch=e.batch([(619,0,1)]);node=e.nodes[0];before=b(node.addr+0x20)
 check(e.fn('hook_commit_ai',batch.addr)==1,'NEXT-003 AI award commits through production batch path')
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(1,0) and e.profile(619).award_mask&1,'NEXT-003 AI redirects a new domestic choice to trait unlock while a trait slot is free')
 check(b(node.addr+0x20)==before,'NEXT-003 temporary AI batch rewrite is restored after native commit')
 e.closeall()
 # At 9/9, never replace an existing trait: route the choice back to domestic.
 e=EN();p=PEOPLE[618];full=(4,5,6,7,8,9,10,11,12);o=e.officer(619,tuple(p[k] for k in ('leadership','war','intelligence','politics','charisma')),full,0,0);e.fn('hook_newgame',0,0)
 before_traits=e.owned(o);batch=e.batch([(619,1,0)]);node=e.nodes[0];before=b(node.addr+0x20)
 check(e.fn('hook_commit_ai',batch.addr)==1,'NEXT-003 full-trait AI batch still commits its native fallback reward')
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(0,1) and e.owned(o)==before_traits and e.profile(619).award_mask==0,'NEXT-003 9/9 officer receives domestic fallback with no trait replacement')
 check(b(node.addr+0x20)==before,'NEXT-003 full-trait fallback also restores original native batch byte')
 e.closeall()

def _set_force_ruler(e,fid,officer_id,force=None):
 if force is None:force=e.alloc(0x100)
 force.put(e.res.native.offsets[5],struct.pack('<H',officer_id))
 e.db.u64(e.res.native.offsets[3]+fid*8,force.addr)
 return force

def ruler_player_three_choices():
 e=R6();o,r,s=e.prepare(capacity=1)
 force=_set_force_ruler(e,1,596);e.fn('hook_ui_enter',e.root_layout.addr);e.fn('hook_row_refresh',r.addr)
 status=(C.c_int*6)();e.fn('hook_status',e.root_layout.addr,o.addr,C.addressof(status))
 check((e.profile(596).reserved[0]&1) and list(status)==[1,0,0,1,0,0],'NEXT-004 player ruler is marked without native pre-award and begins at the first prefix choices')
 for stage in range(3):e.click(r,stage);settled(e,s)
 check(e.selection_counts(596)==(3,0) and all(e.gsui(596).slots[j].phase in (1,2) for j in range(3)),'NEXT-004 player ruler can immediately select upper slots 1->2->3 under the original left-prefix UI')
 # The R6 fixture's independent native decision model was initialized with cap1;
 # align that substituted endpoint with the production veteran status (cap3) only
 # for the final Decision gate.
 e.capacities[596]=3;e.refresh_choices(e.list_owner.addr);check(e.decision_enabled,'NEXT-004 player ruler reaches original Decision at three total choices')
 e.bottom(s)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(3,0) and all(e.gs(596).slots[j].phase==2 for j in range(3)),'NEXT-004 player ruler commits all three selected trait rewards through one original Decision chain')
 e.closeall()

def ruler_veteran_cap_and_persistence():
 e=EN();p=PEOPLE[101];o=e.officer(102,tuple(p[k] for k in ('leadership','war','intelligence','politics','charisma')),(),0,0)
 p2=PEOPLE[102];o2=e.officer(103,tuple(p2[k] for k in ('leadership','war','intelligence','politics','charisma')),(),0,0)
 force=_set_force_ruler(e,1,102);e.fn('hook_newgame',0,0)
 check(e.profile(102).reserved[0]&1 and (b(o.addr+0x1c2),b(o.addr+0x1c3))==(0,0),'NEXT-004 ruler gets persistent veteran eligibility without pre-awarding native rank counters')
 status=(C.c_int*6)();e.fn('hook_status',0,o.addr,C.addressof(status))
 check(list(status)==[1,0,0,1,0,0],'NEXT-004 fresh ruler exposes a three-total-choice prefix while retaining original left-to-right selection semantics')
 e.eligible=False
 check(e.fn('hook_ai_visit',o.addr)==1 and (b(o.addr+0x1c2),b(o.addr+0x1c3))==(0,0),'NEXT-004 ruler remains AI-eligible even when ordinary native eligibility is false, without repeated free awards')
 batch=e.batch([(102,0,0)])
 check(e.fn('hook_commit_ai',batch.addr)==1 and (b(o.addr+0x1c2),b(o.addr+0x1c3))==(3,0),'NEXT-004 ruler AI consumes its three-choice capacity on trait unlocks when slots are free')
 check((e.profile(102).award_mask&7)==7 and sum(1 for x in e.owned(o) if x)==3,'NEXT-004 ruler three upper choices finalize through normal gacha state, not direct trait injection')
 # Change the force ruler. The old ruler keeps the veteran bit; the new ruler gains it.
 force.put(e.res.native.offsets[5],struct.pack('<H',103));e.fn('hook_ai_visit',o2.addr)
 check((e.profile(102).reserved[0]&1) and (e.profile(103).reserved[0]&1),'NEXT-004 current-to-former ruler transition preserves old veteran flag and marks the new ruler')
 old_rank=(b(o.addr+0x1c2),b(o.addr+0x1c3));old_traits=e.owned(o)
 for _ in range(3):e.fn('hook_ai_visit',o.addr)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==old_rank and e.owned(o)==old_traits,'NEXT-004 repeated veteran refresh/visit does not grant duplicate rewards')
 # Reserved profile bytes were already part of the sidecar wire record; verify the
 # new semantic bit survives save/load without a wire-format bump.
 native=b'R10 ruler-veteran persistence';check(e.save(native)==1,'NEXT-004 veteran flags save in existing v3 sidecar')
 e.profile(102).reserved[0]=0;e.profile(103).reserved[0]=0
 check(e.loadsave(native)==1 and not e.mod.val('metadata_fault'),'NEXT-004 veteran sidecar reload passes strict metadata validation')
 check((e.profile(102).reserved[0]&1) and (e.profile(103).reserved[0]&1),'NEXT-004 current and former ruler veteran flags survive save/load')
 e.closeall()

def collab_39_candidates():
 e=E();t=Target();t.id=5001;t.sex=1;t.stats[:]=(70,70,70,70,70);st=State();d=Distribution();e.fn('g_init',C.addressof(st),C.addressof(t.traits))
 check(len(COLLAB_IDS)==39 and e.fn('g_distribution',C.addressof(t),C.addressof(st),C.addressof(COLORS),0,0,C.addressof(d))==1,'NEXT-005 production distribution builds with the 39 collaboration trait sources')
 missing=[i for i in COLLAB_IDS if d.raw[i]<=0 or d.weight[i]<=0]
 check(not missing,'NEXT-005 all 39 DLC/collaboration-exclusive traits receive positive raw and final weight in the production distribution',{'missing':missing})
 collab=json.loads((ROOT/'data/SAN14CE_콜라보39_교사44_DB.json').read_text())['records'];counts=collections.Counter(x['trait_id'] for x in collab)
 check(len(collab)==44 and set(counts)==set(COLLAB_IDS),'NEXT-005 generated collaboration teacher data contains exactly 44 documented records covering exactly 39 traits')
 check(all(counts[i]==2 for i in COLLAB_DUPLICATE_HOLDERS) and all(counts[i]==1 for i in COLLAB_IDS if i not in COLLAB_DUPLICATE_HOLDERS),'NEXT-005 documented duplicate/single holder counts match the 44 collaboration teacher records')
 e.closeall()

FUNCTIONS=[finalized_history_survives_rank_drop,same_turn_return,ai_upper_first_and_full_fallback,ruler_player_three_choices,ruler_veteran_cap_and_persistence,collab_39_candidates]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:
  (ROOT/'audit/r10_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R10',len(REPORT))
