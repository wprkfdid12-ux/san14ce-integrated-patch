"""Exact shipped R13 negative control vs rebuilt R14; operation counts, NOT FPS."""
from pathlib import Path
import hashlib,json,os,subprocess,sys
R=Path(__file__).resolve().parents[1];OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,text):
 if not ok:raise AssertionError(text)
 OUT.append({'name':text,'pass':True});print('PASS',text,flush=True)
base=R/'audit/r13_release_baseline/SAN14_MeritTraits.dll';cur=R/'source/build/SAN14_MeritTraits.dll'
check(sha(base)=='4c88b9de8ce71b10c40632b01eb44b30c38d2b218165668ca28250c3dadb88e9','negative control matches the user-supplied R13 installer DLL')
for version,path in [('R13',base),('R14',cur)]:
 env=os.environ.copy();env.update(MERIT_BENCH_VERSION=version,MERIT_BENCH_DLL=str(path),MERIT_BENCH_N='1000')
 log=R/'audit'/('r14_'+version+'_queries.log')
 with log.open('w') as f:p=subprocess.run([sys.executable,'-u',str(R/'tests/bench_r14_queries.py')],cwd=R,env=env,stdout=f,stderr=subprocess.STDOUT)
 if p.returncode:print(log.read_text()[-5000:])
 check(p.returncode==0,version+' actual Win64 DLL finishes identical 1000-officer/50-ruler workload')
a=json.loads((R/'audit/r14_query_benchmark_r13.json').read_text());b=json.loads((R/'audit/r14_query_benchmark_r14.json').read_text())
check(a['dll_sha256']==sha(base) and b['dll_sha256']==sha(cur),'both operation reports identify exact tested DLL hashes')
expected={'hook_merit_eligible':3000,'hook_ai_visit':2050,'hook_merit_current_rank':1050,'hook_merit_rank':1050}
for name,n in expected.items():
 x,y=a['workloads'][name],b['workloads'][name]
 check(x['calls']==y['calls']==1000 and x['results_sha256']==y['results_sha256'],name+' same calls and byte-identical native results')
 check(x['counts'].get('readable',0)==n and y['counts'].get('readable',0)==0,name+' redundant Core readable checks '+str(n)+' -> 0')
 check(x['counts'].get('native_valid',0)==y['counts'].get('native_valid',0),name+' original native validity checks preserved')
 check(not y['counts'].get('rank_table_checks',0) and not y['counts'].get('full_force_table_checks',0),name+' no repeated rank/force table scan')
d={'version':'RC4-R14-MainMenuQueryPerf','scope':__doc__,'passed':len(OUT),'tests':OUT,'baseline_dll_sha256':sha(base),'current_dll_sha256':sha(cur)}
(R/'audit/r14_performance_tests.json').write_text(json.dumps(d,indent=2)+'\n');print('PASS R14 PERFORMANCE',len(OUT))
