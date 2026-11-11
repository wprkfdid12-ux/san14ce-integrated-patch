"""Compare actual R11/R12 module query operations, not live game FPS."""
from pathlib import Path
import json,hashlib,subprocess,sys,os
R=Path(__file__).resolve().parents[1];OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,text):
 if not ok:raise AssertionError(text)
 OUT.append({'name':text,'pass':True});print('PASS',text,flush=True)
base=R/'audit/r11_release_baseline/SAN14_MeritTraits.dll';current=R/'source/build/SAN14_MeritTraits.dll'
check(sha(base)=='3f5bf5ce433f2bf4308c96171a6224625fe06b47d02ea59898a27f16061be5de','comparison uses exact user-confirmed R11 DLL')
for version,path in [('R11',base),('R12',current)]:
 env=os.environ.copy();env.update(MERIT_BENCH_VERSION=version,MERIT_BENCH_DLL=str(path),MERIT_BENCH_N='1000')
 logfile=R/'audit'/('r12_benchmark_'+version+'.log')
 with logfile.open('w') as log:run=subprocess.run([sys.executable,'-u',str(R/'tests/bench_r12_queries.py')],cwd=R,env=env,stdout=log,stderr=subprocess.STDOUT)
 if run.returncode:print(logfile.read_text()[-5000:])
 check(run.returncode==0,version+' actual DLL workload executed; no Profile/GState/native bytes, RNG or logs changed')
a=json.loads((R/'audit/r12_query_benchmark_r11.json').read_text());b=json.loads((R/'audit/r12_query_benchmark_r12.json').read_text())
check(a['dll_sha256']==sha(base) and b['dll_sha256']==sha(current),'comparison reports fingerprint the exact tested module DLLs')
check((a['officers'],a['active_forces'],a['force_table_slots'])==(1000,50,52) and all(a[k]==b[k] for k in ['officers','active_forces','force_table_slots']),'identical 1,000 officers / 50 active forces / 52 slots fixture')
for name in a['workloads']:
 x=a['workloads'][name];y=b['workloads'][name];cx=x['counts'];cy=y['counts']
 check(x['calls']==y['calls']==1000,name+' equal query count')
 check(x['results_sha256']==y['results_sha256'],name+' R11/R12 eligibility/rank return values identical')
 check(cy['readable']<cx['readable'],name+' native readable boundary calls reduced')
 check(cy.get('rank_table_checks',0)==0,name+' warm immutable rank table not enumerated per query')
 if name in ('hook_merit_eligible','hook_ai_visit'):
  check(cx['full_force_table_checks']==1000 and cy.get('full_force_table_checks',0)==0,name+' full-force table checks 1000 -> 0')
  check(cx['ruler_record_checks']==50000 and cy.get('ruler_record_checks',0)==0,name+' repeated ruler record checks 50000 -> 0')
  check(cy['readable']<cx['readable']*0.06,name+' readable calls below 6% of R11 in this fixture (not FPS claim)')
check(b['workloads']['hook_merit_eligible']['counts']['readable']==7950 and a['workloads']['hook_merit_eligible']['counts']['readable']==210650,'documented candidate readable counts reproduce exactly')
check(b['workloads']['hook_ai_visit']['counts']['readable']==6050 and a['workloads']['hook_ai_visit']['counts']['readable']==208750,'documented AI readable counts reproduce exactly')
d={'version':'RC4-R12-UIFastPath','scope':__doc__,'passed':len(OUT),'tests':OUT,'baseline_dll_sha256':sha(base),'current_dll_sha256':sha(current)}
(R/'audit/r12_benchmark_tests.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n');print('PASS R12 QUERY COMPARISON',len(OUT))
