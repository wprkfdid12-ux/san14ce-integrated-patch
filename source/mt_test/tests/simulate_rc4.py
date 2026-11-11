from test_rc4 import *
import time,statistics
# Actual C engine, no reimplementation. Teachers fixed; targets vary.
subprocess.run(['gcc','-O2','-fno-builtin','-std=c11','-shared','-fPIC',str(ROOT/'source/merit_core.c'),str(ROOT/'source/gacha.c'),'-o',str(ROOT/'source/build/gacha_sim.so')],check=True)
lib=C.CDLL(str(ROOT/'source/build/gacha_sim.so'))
lib.g_init.argtypes=[C.POINTER(State),C.POINTER(C.c_uint16)]
lib.g_roll.argtypes=[C.POINTER(Target),C.POINTER(State),C.c_uint32,C.POINTER(C.c_uint8),C.c_uint64,C.c_uint64,C.c_uint16,C.c_int,C.POINTER(Distribution)]
lib.g_accept.argtypes=[C.POINTER(Target),C.POINTER(State),C.c_uint32,C.c_uint16,C.c_int]
lib.g_distribution.argtypes=[C.POINTER(Target),C.POINTER(State),C.POINTER(C.c_uint8),C.c_uint8,C.c_uint8,C.POINTER(Distribution)]
seed=0x6a6d9f72;counts=collections.Counter();stars=collections.Counter();times=[];summaries=[]
for p in PEOPLE:
 t=target(p['officer_id']);st=State();d=Distribution();lib.g_init(C.byref(st),t.traits);gold=0
 for stage in range(3):
  seed=(seed*6364136223846793005+1442695040888963407)&((1<<64)-1)
  start=time.perf_counter_ns();r=lib.g_roll(C.byref(t),C.byref(st),stage,COLORS,seed,p['officer_id']*3+stage,0,0,C.byref(d));times.append((time.perf_counter_ns()-start)/1e6)
  assert r==1,(p['officer_id'],stage,r)
  x=st.slots[stage]
  if x.outcome==1:assert stage<2 and x.result==0 and not st.passion
  else:assert COLORS[x.result] in (1,3) and x.result not in t.traits
  counts['miss']+=x.outcome==1;counts['passion_rolls']+=bool(x.passion_applied)
  gold+=COLORS[x.result]==3;counts['rolls']+=1;counts['gold']+=COLORS[x.result]==3;counts['anomaly']+=x.anomaly;counts['transcend']+=x.transcend;stars['star_rolls' if t.star else 'ordinary_rolls']+=1;stars['star_gold' if t.star else 'ordinary_gold']+=COLORS[x.result]==3
  assert lib.g_accept(C.byref(t),C.byref(st),stage,0,0)==1
 counts['triple_gold']+=gold==3
 if p['officer_id'] in [147,179,183,203,393,440,521,596,945,952,987]:
  t0=target(p['officer_id']);s0=State();lib.g_init(C.byref(s0),t0.traits);lib.g_distribution(C.byref(t0),C.byref(s0),COLORS,0,0,C.byref(d));names={m['trait_id']:m['name'] for m in MASTER};summaries.append({'id':p['officer_id'],'name':p['name'],'top_similar':[{'id':x.id,'name':PEOPLE[x.id-1]['name'],'score':x.score} for x in d.similar if x.id<=1000 and x.id],'top_candidates':[{'trait':names[x.id],'probability':x.score} for x in d.candidates if x.id]})
version=json.loads((ROOT/'package.manifest.json').read_text())['Version']
result={'version':version,'scope':'1000 official officers x 3 sequential accepted rolls; actual C engine compiled for Linux; deterministic seed, initial tuning; not final balance or game FPS','rolls':dict(counts),'groups':dict(stars),'calculation_ms':{'median':statistics.median(times),'p95':sorted(times)[int(len(times)*.95)],'max':max(times)},'examples':summaries}
(ROOT/'audit'/(version.split('-')[-1].lower()+'_simulation.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='examples'},ensure_ascii=False,indent=2))
