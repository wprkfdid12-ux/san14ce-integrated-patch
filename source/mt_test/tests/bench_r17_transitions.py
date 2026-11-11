"""Patch paths used around turns, not a measured game turn or FPS.
Actual Win64 code; 1,000 officers; native rule/commit fragments. Host OS, file
storage, Steam, force endpoints and scheduler are substitutes. Five repetitions.
"""
from r11_fixture import *
from test_rc4 import State
import bisect,statistics,time,os,collections,test_native
VERSION=os.environ.get('MERIT_BENCH_VERSION','R17')
DLL=Path(os.environ.get('MERIT_BENCH_DLL',ROOT/'source/build/SAN14_MeritTraits.dll'))
loader=test_native.Loaded
test_native.Loaded=lambda path,bridge=None,mapfile=None:loader(DLL if Path(path).name=='SAN14_MeritTraits.dll' else path,bridge,mapfile)

def run():
 e=EN();install_native_rules(e);NativeR11UI.setup_native_commit(e)
 for id in range(1,1001):
  o=e.officer(id,owned=(4,37,51,62,65));o.put(0xe6,b'\x04');o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<H',9999))
 for fid in range(1,21):e.officers[fid].put(0xe6,b'\x01');force_ruler(e,fid,fid)
 size=0;ranges=[];starts=[];counts=collections.Counter()
 def readable(p,n):
  nonlocal size,ranges,starts
  counts['readable']+=1
  if len(e.mem)!=size:
   size=len(e.mem);ranges=sorted((m.addr,m.addr+m.size) for m in e.mem);starts=[x[0] for x in ranges]
  k=bisect.bisect_right(starts,p)-1
  return bool(p and n and k>=0 and p+n<=ranges[k][1])
 BR.callbacks['Readable']=readable;e.fn('hook_newgame',0,0)
 report={'version':VERSION,'dll_sha256':hashlib.sha256(DLL.read_bytes()).hexdigest(),'scope':__doc__,'workloads':{}}
 def measure(name,action,repeats=5):
  durations=[];reads=[];rng=e.rng_calls
  for i in range(repeats):
   counts.clear();start=time.perf_counter_ns();action(i);durations.append((time.perf_counter_ns()-start)/1e6);reads.append(counts['readable'])
  report['workloads'][name]={'median_ms':statistics.median(durations),'durations_ms':durations,'readable_calls':reads,'rng_calls':e.rng_calls-rng}
 # No gacha during the repeated native menu / AI candidate queries.
 before=C.string_at(e.mod.sym('states'),6000*C.sizeof(State))
 def candidates(_):
  for o in e.officers.values():
   for key in ('hook_ai_visit','hook_merit_eligible','hook_merit_current_rank','hook_merit_rank'):e.fn(key,o.addr)
 measure('four_queries_x1000',candidates)
 assert C.string_at(e.mod.sym('states'),len(before))==before and report['workloads']['four_queries_x1000']['rng_calls']==0
 empty=e.batch([]);empty.u64(16,e.city.addr)
 def empty_batches(_):
  for j in range(100):assert e.fn('hook_commit_ai',empty.addr)==0 and not e.mod.val('metadata_fault')
 measure('empty_ai_commit_x100',empty_batches)
 assert report['workloads']['empty_ai_commit_x100']['rng_calls']==0
 # Existing full 9/9 officers must still receive domestic choices without RNG.
 o=e.officers[952];o.put(0x118,struct.pack('<9H',4,37,51,62,65,1,2,3,5))
 batch=e.batch([(952,0,3)]);batch.u64(16,e.city.addr)
 person=o.get(0,0x200);profile=C.string_at(e.mod.sym('profiles')+952*C.sizeof(Profile),C.sizeof(Profile));state=bytes(e.gs(952))
 def domestic(_):
  o.put(0,person);C.memmove(e.mod.sym('profiles')+952*C.sizeof(Profile),profile,len(profile));C.memmove(C.addressof(e.gs(952)),state,len(state))
  assert e.fn('hook_commit_ai',batch.addr)==1
  assert o.get(0x1c2,2)==b'\0\3' and not e.gs(952).rerolls_used
 measure('domestic_only_ai_commit',domestic)
 assert report['workloads']['domestic_only_ai_commit']['rng_calls']==0
 # Force a real live rule change each repetition (e.g. edited activation or
 # linked officer validity) instead of measuring an already-warm no-op.
 def rule_change(i):
  e.traits.put(94*0x100+0xa4,bytes([0 if i%2==0 else 21]));e.fn('refresh_trait_rules')
 measure('one_live_trait_rule_change',rule_change)
 # Same native content in five saves exercises both first commit and retained
 # idempotent journal. Includes all 1,000 live officer checks; writes are in RAM.
 data=bytes(range(256))*8192
 def saving(_):
  assert e.save(data)==1
  assert C.string_at(e.mod.sym('active_save_hash'),32)==hashlib.sha256(data).digest()
 measure('native_save_2MiB_1000_officers',saving)
 state=bytes(e.gs(952));assert e.loadsave(data)==1 and bytes(e.gs(952))==state
 report['save_state_sha256']=hashlib.sha256(state).hexdigest()
 assert not e.mod.val('metadata_fault') and not BR.errors
 (ROOT/'audit'/('r17_transitions_'+VERSION.lower()+'.json')).write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2),flush=True);e.closeall()

if __name__=='__main__':run()
