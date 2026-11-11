"""Compare exact R12/R13 DLL operations, not live-game FPS/time."""
from pathlib import Path
import json,hashlib,subprocess,sys,os
R=Path(__file__).resolve().parents[1];OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,text):
 if not ok:raise AssertionError(text)
 OUT.append({'name':text,'pass':True});print('PASS',text,flush=True)
base=R/'audit/r12_release_baseline/SAN14_MeritTraits.dll';current=R/'source/build/SAN14_MeritTraits.dll'
check(sha(base)=='1de57229e56cfe01485bfd0bcf7e3dff691742f9d7bac1a32639b3d390e2a83d','negative control is exact R12-UIFastPath distributed DLL')
for version,path in [('R12',base),('R13',current)]:
 for script in ('bench_r13_queries.py','bench_r13_ui.py'):
  env=os.environ.copy();env.update(MERIT_BENCH_VERSION=version,MERIT_BENCH_DLL=str(path),MERIT_BENCH_N='1000')
  logfile=R/'audit'/('r13_'+version+'_'+script.replace('.py','.log'))
  with logfile.open('w') as log:run=subprocess.run([sys.executable,'-u',str(R/'tests'/script)],cwd=R,env=env,stdout=log,stderr=subprocess.STDOUT)
  if run.returncode:print(logfile.read_text()[-5000:])
  check(run.returncode==0,version+' actual DLL workload completed: '+script)
a=json.loads((R/'audit/r13_query_benchmark_r12.json').read_text());b=json.loads((R/'audit/r13_query_benchmark_r13.json').read_text())
check(a['dll_sha256']==sha(base) and b['dll_sha256']==sha(current),'query reports fingerprint exact tested modules')
for name in a['workloads']:
 x=a['workloads'][name];y=b['workloads'][name];cx=x['counts'];cy=y['counts']
 check(x['calls']==y['calls']==1000,name+' same count and same 1000-officer/50-force world')
 check(x['results_sha256']==y['results_sha256'],name+' identical eligibility/rank results')
 check(cy['readable']<cx['readable'],name+' fewer repeated checked-memory calls within callback')
 check(not cy.get('rank_table_checks',0) and not cy.get('full_force_table_checks',0),name+' retains R12 warm rank/force-table optimization')
a=json.loads((R/'audit/r13_ui_workload_r12.json').read_text());b=json.loads((R/'audit/r13_ui_workload_r13.json').read_text())
check(a['dll_sha256']==sha(base) and b['dll_sha256']==sha(current),'UI/commit reports fingerprint exact tested modules')
x=a['workloads']['single_officer_commit_1000_profiles'];y=b['workloads']['single_officer_commit_1000_profiles']
check(x['officer_table_lookup']==2003 and y['officer_table_lookup']==4,'one actual batch visits 4 officer-table slots instead of 2003; unrelated officers unchanged')
check(x['readable']==10023 and y['readable']==22,'single-officer native commit boundary checks 10023 -> 22')
x=a['workloads']['x_free_9000_display_checks'];y=b['workloads']['x_free_9000_display_checks']
check(x['officer_table_lookup']==9000 and not y.get('officer_table_lookup',0),'9000 X-free cell checks avoid 9000 unnecessary live officer lookups')
check(x['readable']==36000 and y['readable']==9000,'X-free checked-memory calls 36000 -> 9000, same zero mask results')
x=a['actual_shipped_core_logger'];y=b['actual_shipped_core_logger']
check(x['gameplay_io']=={'CreateFileW':61,'WriteFile':61,'CloseHandle':61},'actual shipped Core code reproduces one sync file triplet per UI diagnostic')
check(not any(y['gameplay_io'].values()) and y['queued_records']==61,'same 61 diagnostics queued with zero file calls on gameplay callback')
check(y['overflow_dropped']>0 and not any(y['worker_absent_gameplay_io'].values()),'full queue/absent logger drop diagnostics without sync fallback or disabling gameplay')
d={'version':'RC4-R13-PreviewReroll_Perf','scope':__doc__,'passed':len(OUT),'tests':OUT,'baseline_dll_sha256':sha(base),'current_dll_sha256':sha(current)}
(R/'audit/r13_performance_tests.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n');print('PASS R13 PERFORMANCE',len(OUT))
