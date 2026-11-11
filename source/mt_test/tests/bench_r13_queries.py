"""Operation-count comparison using the actual release Win64 DLLs.
This is not Windows/GPU/game frame timing. OS and game services are substituted.
Use MERIT_BENCH_DLL to select the exact R12 module DLL baseline without modifying payload.
"""
from r11_fixture import *
from test_rc4 import State
import os,time,bisect,collections
VERSION=os.environ.get('MERIT_BENCH_VERSION','R13')
N=int(os.environ.get('MERIT_BENCH_N','1000'))
BENCH_DLL=Path(os.environ.get('MERIT_BENCH_DLL',str(ROOT/'source/build/SAN14_MeritTraits.dll'))).resolve()
if 'MERIT_BENCH_DLL' in os.environ:
 import test_native
 original_loader=test_native.Loaded
 def selected_loader(path,bridge=None,mapfile=None):
  return original_loader(BENCH_DLL if Path(path).name=='SAN14_MeritTraits.dll' else path,bridge,mapfile)
 test_native.Loaded=selected_loader
def main():
 e=EN();install_native_rules(e)
 people=[];forces=[]
 for id in range(1,N+1):
  o=e.officer(id);o.put(0xe6,b'\x04');o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<H',9999));people.append(o)
 for fid in range(1,min(int(e.res.native.offsets[4]),N//20)+1):
  id=fid*20;people[id-1].put(0xe6,b'\x01');forces.append(force_ruler(e,id,fid))
 ranges=sorted((m.addr,m.addr+m.size) for m in e.mem);starts=[x[0] for x in ranges]
 force_set={x.addr for x in forces};rank_set={x.addr for x in e.rank_entries}
 counts=collections.Counter();ft=e.db.addr+int(e.res.native.offsets[3]);rt=e.db.addr+0x85db8
 def readable(p,n):
  counts['readable']+=1
  if p==ft:counts['full_force_table_checks']+=1
  if p in force_set:counts['ruler_record_checks']+=1
  if rt<=p<rt+56 or p in rank_set:counts['rank_table_checks']+=1
  if not p or not n:return 0
  k=bisect.bisect_right(starts,p)-1
  return k>=0 and p+n<=ranges[k][1]
 BR.callbacks['Readable']=readable
 def valid(p):
  counts['native_valid']+=1
  if p in force_set:counts['force_valid']+=1
  return bool(p)
 BR.callbacks['Valid']=valid
 e.fn('hook_newgame',0,0)
 initial_profiles=C.string_at(e.mod.sym('profiles'),6000*C.sizeof(Profile));initial_states=C.string_at(e.mod.sym('states'),6000*C.sizeof(State))
 initial_people=[o.get(0,0x200) for o in people]
 result={'version':VERSION,'scope':__doc__,'officers':N,'active_forces':len(forces),'force_table_slots':int(e.res.native.offsets[4])+1,'dll_sha256':hashlib.sha256(BENCH_DLL.read_bytes()).hexdigest(),'workloads':{}}
 for name in ['hook_merit_eligible','hook_ai_visit','hook_merit_current_rank','hook_merit_rank']:
  counts.clear();logs=len(e.logs);rng=e.rng_calls;start=time.perf_counter()
  values=[e.fn(name,o.addr) for o in people]
  elapsed=(time.perf_counter()-start)*1000
  check(C.string_at(e.mod.sym('profiles'),len(initial_profiles))==initial_profiles,name+' repeat-query Profile bytes unchanged')
  check(C.string_at(e.mod.sym('states'),len(initial_states))==initial_states,name+' repeat-query GState bytes unchanged')
  check(all(o.get(0,0x200)==b for o,b in zip(people,initial_people)),name+' repeat-query native officer bytes unchanged')
  check(len(e.logs)==logs and e.rng_calls==rng,name+' no logs, RNG or awards for ordinary repeated queries')
  result['workloads'][name]={'calls':N,'counts':dict(counts),'results_sha256':hashlib.sha256(struct.pack('<'+'Q'*len(values),*values)).hexdigest(),'synthetic_ms_not_live_game':round(elapsed,3)}
 p=ROOT/'audit'/('r13_query_benchmark_'+VERSION.lower()+'.json');p.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2),flush=True)
 e.closeall()
if __name__=='__main__':main()
