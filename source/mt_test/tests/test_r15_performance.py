"""Check recorded exact-DLL AI A/B results, randomness and measured workload cost.
Host callback timing only; never treats this as live FPS or total-turn evidence.
"""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[1];OUT=[]
def check(ok,name):
 if not ok:raise AssertionError(name)
 OUT.append({'name':name,'pass':True});print('PASS',name)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
a=json.loads((R/'audit/r15_ai_benchmark_r14.json').read_text());b=json.loads((R/'audit/r15_ai_benchmark_r15.json').read_text())
check(a['dll_sha256']==sha(R/'audit/r14_release_baseline/SAN14_MeritTraits.dll'),'baseline is the exact delivered R14 binary')
check(b['dll_sha256']==sha(R/'source/build/SAN14_MeritTraits.dll'),'optimized timing and outputs refer to the current compiled R15')
check(a['officers']==b['officers']==100 and a['rulers']==b['rulers']==10,'identical 100-officer/10-ruler workloads')
for x,y in zip(a['outputs'],b['outputs']):
 check(x=={k:y[k] for k in x},'seed '+str(x['seed_trial'])+': exact final traits/states, RNG calls, accepted results and reroll counts preserved')
 check(y['rerolls']>0 and y['calculations']['cache_hits']>0 and y['calculations']['anchor_builds']==0,'seed '+str(x['seed_trial'])+': rerolls execute with reused probabilities and no neighbour rebuild')
check(b['median_ms']<a['median_ms'],'measured AI award callback median improved on this host')
d={'scope':__doc__,'passed':len(OUT),'tests':OUT,'baseline_median_ms':a['median_ms'],'optimized_median_ms':b['median_ms'],'reduction_percent':100*(1-b['median_ms']/a['median_ms']),'live_game_tested':False}
(R/'audit/r15_performance_tests.json').write_text(json.dumps(d,indent=2)+'\n')
