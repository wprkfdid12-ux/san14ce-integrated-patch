"""R12 real compiled Win64 performance-path and safety regressions.
Uses captured native predicates/status/commit code, substituted OS/world/UI.
No live Windows rendering or actual game frame-rate certification.
"""
from r11_fixture import *
from test_rc4 import State
from test_native import REPORT
import collections

def ordinary_and_recorded_queries():
 e=EN();install_native_rules(e)
 o=e.officer(952);o.put(0xe6,b'\x04');o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<H',9999))
 lord=e.officer(596);lord.put(0xe6,b'\x01');force=force_ruler(e,596);e.fn('hook_newgame',0,0)
 original=BR.callbacks['Readable'];counts=collections.Counter();ft=e.db.addr+e.res.native.offsets[3]
 def counted(p,n):
  if p==ft:counts['force_table']+=1
  if p==force.addr:counts['force_record']+=1
  if p in {x.addr for x in e.rank_entries}:counts['rank_record']+=1
  return original(p,n)
 BR.callbacks['Readable']=counted
 rng=e.rng_calls;logs=len(e.logs);files=dict(e.files);tick=e.api_counts['timer']
 ps=C.string_at(e.mod.sym('profiles'),6000*C.sizeof(Profile));gs=C.string_at(e.mod.sym('states'),6000*C.sizeof(State))
 for _ in range(250):
  for person in (o,lord):
   e.fn('hook_merit_eligible',person.addr);e.fn('hook_ai_visit',person.addr)
   e.fn('hook_merit_current_rank',person.addr);e.fn('hook_merit_rank',person.addr)
 check(not counts['force_table'] and not counts['force_record'],'2,000 steady queries perform zero full-force scans / ruler record revisits')
 check(not counts['rank_record'],'steady current/proposed rank uses prepared table result, not per-officer rank-record enumeration')
 check(C.string_at(e.mod.sym('profiles'),len(ps))==ps and C.string_at(e.mod.sym('states'),len(gs))==gs,'steady queries leave every Profile/GState byte unchanged')
 check(e.rng_calls==rng and len(e.logs)==logs and e.files==files,'steady queries make no RNG draws, logs or file writes')
 check(e.api_counts['timer']==tick,'query fast path has no timer polling, artificial delay or time-based stale result cache')
 check(e.owned(o)==(4,37,51,62,65,0,0,0,0) and b(o.addr+0x1c2)==0,'query optimization does not award traits or native reward counts')
 e.closeall()

def same_tick_succession():
 for entry in ('hook_merit_eligible','hook_merit_current_rank','hook_merit_rank','hook_ai_visit'):
  e=EN();install_native_rules(e);old=e.officer(596);old.put(0xe6,b'\x01');f=force_ruler(e,596)
  o=e.officer(952,top=1,bottom=1);o.put(0xe6,b'\x04');o.put(0x1bc,b'\x02');e.fn('hook_newgame',0,0)
  e.fn(entry,o.addr);check(not e.profile(952).reserved[0]&1,entry+' does not give ordinary officer ruler privilege')
  before=e.owned(o);rng=e.rng_calls;gs=bytes(e.gs(952));tick=e.tick
  old.put(0xe6,b'\x04');o.put(0xe6,b'\x01');f.put(e.res.native.offsets[5],struct.pack('<H',952))
  result=e.fn(entry,o.addr)
  check(e.profile(952).reserved[0]&1 and result in (1,6),entry+' recognizes a fresh succession immediately, no timer/next-screen wait')
  check(e.profile(596).reserved[0]&1 and e.tick==tick,entry+' preserves former ruler history within exactly the same tick')
  e.fn('hook_merit_eligible',o.addr)
  check((b(o.addr+0x1c2),b(o.addr+0x1c3),b(o.addr+0x1bc))==(1,1,6),entry+' preserves spent mixed rewards and only supplies senior eligibility')
  check(e.owned(o)==before and e.rng_calls==rng and bytes(e.gs(952))==gs,entry+' succession recognition grants neither traits nor replacement reroll tickets')
  e.closeall()

def unknown_or_mismatched_person():
 e=EN();install_native_rules(e);o=e.officer(952);o.put(0xe6,b'\x01');e.fn('hook_newgame',0,0)
 e.fn('hook_merit_eligible',o.addr)
 check(not e.profile(952).reserved[0]&1,'status-1 alone cannot fabricate ruler history without matching force record')
 f=force_ruler(e,952);clone=e.alloc(0x200,o.get(0,0x200));e.fn('refresh_target_ruler',clone.addr,2)
 check(not e.profile(952).reserved[0]&1,'detached object with copied ruler ID is not allowed to establish history')
 e.fn('refresh_target_ruler',o.addr,2)
 check(e.profile(952).reserved[0]&1,'matching live officer-table pointer and force ruler record establish valid history')
 native(e,'ruler_status');e.mod.val('metadata_fault',1);x=e.officer(771);x.put(0xe6,b'\x01');force_ruler(e,771,2)
 e.fn('hook_merit_current_rank',x.addr)
 check(not e.profile(771).valid,'metadata fault does not initialize new profiles from a rank query')
 e.closeall()

def table_cache_lifetime():
 e=EN();install_native_rules(e)
 original=BR.callbacks['Readable'];count=collections.Counter();record_addrs={x.addr for x in e.rank_entries}
 def counted(p,n):
  if p in record_addrs:count['rank']+=1
  return original(p,n)
 BR.callbacks['Readable']=counted;e.fn('reset_rules_cache');check(e.fn('senior_rank')==6,'cold rank cache obtains result from actual verified native rank table')
 first=count['rank']
 for _ in range(500):assert e.fn('senior_rank')==6
 check(count['rank']==first,'500 warm rank queries avoid every rank-record reread')
 e.rank_entries[3].u32(0x30,1);e.fn('refresh_tables')
 check(e.fn('senior_rank')==5,'explicit immutable-table refresh invalidates cached rank')
 e.rank_entries[3].u32(0x30,0);e.fn('hook_newgame',0,0)
 check(e.fn('senior_rank')==6,'new game invalidates previous rank-table snapshot')
 e.rank_entries[3].u32(0x30,1)
 check(e.fn('hook_load',0,0,0)==1 and e.fn('senior_rank')==5,'successful legacy load invalidates cache before preparing new metadata')
 e.rank_entries[3].u32(0x30,0);e.fn('reset_rules_cache');p=e.db.u64(0x85db8+8);e.db.u64(0x85db8+8,0)
 check(e.fn('senior_rank')==0 and e.mod.val('rules_cached_senior')==0,'incomplete rank table failure is not cached as valid data')
 e.db.u64(0x85db8+8,p)
 check(e.fn('senior_rank')==6,'repaired readable rank table can succeed immediately after uncached failure')
 newdb=e.alloc(e.db.size,e.db.get(0,e.db.size));entries=[]
 for i in range(7):
  r=e.alloc(0x40,e.rank_entries[i].get(0,0x40));r.u32(0x30,int(i>=3));newdb.u64(0x85db8+i*8,r.addr);entries.append(r)
 e.game.u64(e.res.offsets[0],newdb.addr)
 check(e.fn('senior_rank')==5,'different live database address cannot reuse previous cached senior rank')
 e.game.u64(e.res.offsets[0],e.db.addr)
 check(e.fn('senior_rank')==6,'return to original database address rebuilds original rank result')
 e.closeall()

def status_guard_is_real():
 e=EN()
 for name in ('eligible','rank','current_rank','ruler_status'):native(e,name)
 bonus=json.loads((ROOT/'analysis/r15_domestic_native.json').read_text())['blocks']['merit_bonus']
 e.game.put(bonus['rva'],bytes.fromhex(bonus['bytes']))
 native(e,'ruler_status');e.game.put(EVIDENCE['ruler_status']['rva'],b'\x90')
 check(e.fn('rules_prepare_native')==0 and e.mod.val('installed_count')==0,'altered native status accessor is rejected before any hook installation')
 native(e,'ruler_status')
 check(e.fn('rules_prepare_native')==1,'restored captured 17-byte status accessor passes exact fingerprint')
 e.game.put(bonus['rva']+0x59,b'\x90')
 check(e.fn('rules_prepare_native')==0 and e.mod.val('installed_count')==0,'altered bonus body beyond its original 16-byte prologue is rejected before hooking')
 e.game.put(bonus['rva'],bytes.fromhex(bonus['bytes']))
 check(e.fn('rules_prepare_native')==1,'restored full native bonus fingerprint accepts the original ruler probe contract')
 e.closeall()

if __name__=='__main__':
 try:
  for f in (ordinary_and_recorded_queries,same_tick_succession,unknown_or_mismatched_person,table_cache_lifetime,status_guard_is_real):
   print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r12_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R12',len(REPORT))
