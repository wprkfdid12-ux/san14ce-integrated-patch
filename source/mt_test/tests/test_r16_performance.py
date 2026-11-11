"""Current DLL host benchmark gate; intentionally does not compare FPS or old draw results."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[1];OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,name):
 if not ok:raise AssertionError(name)
 OUT.append({'name':name,'pass':True});print('PASS',name)
a=json.loads((R/'audit/r16_ai_benchmark_r15.json').read_text());b=json.loads((R/'audit/r16_ai_benchmark_r16.json').read_text())
check(a['dll_sha256']==sha(R/'audit/r15_release_baseline/SAN14_MeritTraits.dll'),'baseline benchmark identifies exact delivered R15 DLL')
check(b['dll_sha256']==sha(R/'source/build/SAN14_MeritTraits.dll'),'R16 benchmark identifies final compiled DLL')
check(a['officers']==b['officers']==100 and a['rulers']==b['rulers']==10,'same officer/ruler workload sizes')
for x,y in zip(a['outputs'],b['outputs']):
 check(y['accepted']==x['accepted']==300 and y['rerolls']>0 and y['rng_calls']>2*y['rerolls'],'seed %s: 300 upper awards, live rerolls and fresh entropy'%y['seed_trial'])
 check(y['calculations']['distributions']<x['calculations']['distributions'] and y['calculations']['cache_hits']>0 and y['calculations']['anchor_builds']==0,'seed %s: fewer full calculations, cache reused, no anchor rebuild'%y['seed_trial'])
check(b['median_ms']<a['median_ms'],'observed host AI award callback median improves')
sa=json.loads((R/'audit/r16_small_batch_r15.json').read_text());sb=json.loads((R/'audit/r16_small_batch_r16.json').read_text())
check(sa['officers']==sb['officers']==1 and sb['dll_sha256']==sha(R/'source/build/SAN14_MeritTraits.dll'),'small-batch risk check uses one officer and final DLL')
check(sb['median_ms']<sa['median_ms'],'one-officer callback also improves after native object lifetime cache')
report={'passed':len(OUT),'tests':OUT,'old_median_ms':a['median_ms'],'new_median_ms':b['median_ms'],'scope':__doc__,'live_game_tested':False}
(R/'audit/r16_performance_tests.json').write_text(json.dumps(report,indent=2)+'\n')
