"""Actual Win64 AI award callback + captured native commit, deterministic entropy.
Host timings include Python-substituted OS/faction endpoints; NOT game FPS or
total AI-turn timing. No reroll threshold, budget, RNG or native choices changed.
"""
from r11_fixture import *
from test_rc4 import State
from test_r15_gacha import all_targets
import os,time,bisect,statistics,hashlib
VERSION=os.environ.get('MERIT_BENCH_VERSION','R15')
DLL=Path(os.environ.get('MERIT_BENCH_DLL',str(ROOT/'source/build/SAN14_MeritTraits.dll')))
import test_native
original_loader=test_native.Loaded
def selected_loader(path,bridge=None,mapfile=None):
 return original_loader(DLL if Path(path).name=='SAN14_MeritTraits.dll' else path,bridge,mapfile)
test_native.Loaded=selected_loader

def run():
 e=EN();install_native_rules(e);NativeR11UI.setup_native_commit(e)
 targets=all_targets();ids=[]
 # 100 fully eligible officers. All have room for exactly three new traits.
 for t in targets[:100]:
  owned=tuple(x for x in t.traits if x)[:5]
  o=e.officer(t.id,tuple(t.stats),owned);o.put(0xe6,b'\x04');o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<H',9999));ids.append(t.id)
 # Ten current rulers exercise the same native AI eligibility/commit route.
 for fid,id in enumerate(ids[:10],1):e.officers[id].put(0xe6,b'\x01');force_ruler(e,id,fid)
 e.fn('hook_newgame',0,0)
 batch=e.batch([(id,0,3) for id in ids]);batch.u64(16,e.city.addr)
 # Replace only the expensive Python linear readable fixture with an equivalent
 # interval lookup, in BOTH versions. This is outside the measured game module.
 ranges=sorted((m.addr,m.addr+m.size) for m in e.mem);starts=[x[0] for x in ranges]
 def readable(p,n):
  k=bisect.bisect_right(starts,p)-1
  return bool(p and n and k>=0 and p+n<=ranges[k][1])
 BR.callbacks['Readable']=readable
 states=C.string_at(e.mod.sym('states'),6000*C.sizeof(State));profiles=C.string_at(e.mod.sym('profiles'),6000*C.sizeof(Profile));people={id:o.get(0,0x200) for id,o in e.officers.items()}
 durations=[];outputs=[]
 for trial in range(5):
  C.memmove(e.mod.sym('states'),states,len(states));C.memmove(e.mod.sym('profiles'),profiles,len(profiles))
  for id,data in people.items():e.officers[id].put(0,data)
  e.mod.val('metadata_fault',0);e.mod.val('roll_sequence',0,'<Q');e.mod.val('generation',0,'<Q');e.mod.val('log_read',0);e.mod.val('log_write',0);e.mod.val('log_dropped',0);e.rng_calls=0
  rng=random.Random(0x150000+trial)
  def entropy(h,p,n,flags):
   assert n==4 and flags==2;e.rng_calls+=1;put(p,rng.getrandbits(32),'<I');return 0
  BR.callbacks['BCryptGenRandom']=entropy
  prior=struct.unpack('<3Q',C.string_at(e.mod.sym('g_calc_counters'),24)) if VERSION=='R15' else None
  start=time.perf_counter_ns();result=e.fn('hook_commit_ai',batch.addr);ms=(time.perf_counter_ns()-start)/1e6
  assert result==1 and not e.mod.val('metadata_fault'),(VERSION,result,e.mod.val('metadata_fault'),e.logs[-4:],e.rng_calls)
  snapshot=b''.join(bytes(e.gs(id))+struct.pack('<9H',*e.owned(e.officers[id]))+e.officers[id].get(0x1c2,2) for id in ids)
  data={'seed_trial':trial,'rng_calls':e.rng_calls,'rerolls':sum(e.gs(id).rerolls_used for id in ids),'accepted':sum(x.phase==2 for id in ids for x in e.gs(id).slots),'sha256':hashlib.sha256(snapshot).hexdigest()}
  assert data['rerolls']>0 and data['accepted']==300
  if VERSION=='R15':
   now=struct.unpack('<3Q',C.string_at(e.mod.sym('g_calc_counters'),24));data['calculations']={k:b-a for k,a,b in zip(('distributions','cache_hits','anchor_builds'),prior,now)}
   assert data['calculations']['cache_hits']>0 and data['calculations']['anchor_builds']==0
  outputs.append(data);durations.append(ms)
 check(True,'five seeded batches complete 300 AI upper awards each with rerolls enabled')
 report={'version':VERSION,'scope':__doc__,'dll_sha256':hashlib.sha256(DLL.read_bytes()).hexdigest(),'officers':100,'rulers':10,'durations_ms':durations,'median_ms':statistics.median(durations),'outputs':outputs}
 (ROOT/'audit'/('r15_ai_benchmark_'+VERSION.lower()+'.json')).write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2),flush=True);e.closeall()

if __name__=='__main__':run()
