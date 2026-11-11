"""Final DLL benchmark checks; host callback timings only, not game FPS."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[1];OUT=[]
def check(ok,name):
 assert ok,name
 OUT.append({'name':name,'pass':True});print('PASS',name,flush=True)
def read(name):return json.loads((R/'audit'/name).read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
base=sha(R/'audit/r16_release_baseline/SAN14_MeritTraits.dll');current=sha(R/'source/build/SAN14_MeritTraits.dll')
metrics={}
for workload in ('ai_benchmark','small_batch'):
 a=read('r17_'+workload+'_r16.json');b=read('r17_'+workload+'_r17.json')
 check(a['dll_sha256']==base and b['dll_sha256']==current,workload+': exact R16 release and final R17 DLL hashes')
 check(a['officers']==b['officers'] and a['rulers']==b['rulers'],workload+': same officer and ruler counts')
 check(a['outputs']==b['outputs'],workload+': five seeds retain every state digest, RNG count, reroll, accepted award and distribution count')
 check(b['median_ms']<a['median_ms'],workload+': measured game-callback time decreases with AI rerolls enabled')
 metrics[workload]={'r16_median_ms':a['median_ms'],'r17_median_ms':b['median_ms']}
a=read('r17_transitions_r16.json');b=read('r17_transitions_r17.json')
check(a['dll_sha256']==base and b['dll_sha256']==current,'turn-adjacent measurements use exact baseline/final DLL')
check(a['save_state_sha256']==b['save_state_sha256'],'saved/restored domestic officer state equals R16')
for name in a['workloads']:
 x=a['workloads'][name];y=b['workloads'][name]
 check(x['rng_calls']==y['rng_calls']==0,name+': no extra gacha or entropy')
 if name=='four_queries_x1000':
  check(x['readable_calls']==y['readable_calls']==[0]*5,'warm menu/AI qualification retains zero OS memory queries')
 else:check(y['median_ms']<x['median_ms'],name+': observed host cost decreases')
 metrics[name]={'r16_median_ms':x['median_ms'],'r17_median_ms':y['median_ms']}
(R/'audit/r17_performance_tests.json').write_text(json.dumps({'passed':len(OUT),'tests':OUT,'scope':__doc__,'metrics':metrics,'live_game_tested':False},indent=2)+'\n')
