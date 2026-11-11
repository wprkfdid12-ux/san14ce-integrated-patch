"""R4 gate runner. Actual Win64 DLL with substituted endpoints, not live game."""
from pathlib import Path
import hashlib,json,subprocess,sys,time
root=Path(__file__).resolve().parents[1]
suite=['test_rc4.py','test_cp03.py','test_stack_probe.py','test_native_integration.py','test_r1_skip.py','test_r2.py','test_r3.py','test_r4.py','test_r4_package.py']
results=[]
for name in suite:
 started=time.monotonic();path=root/'audit'/('r4_'+name.replace('.py','.log'))
 with path.open('w') as log:r=subprocess.run([sys.executable,'-u',str(root/'tests'/name)],cwd=root,stdout=log,stderr=subprocess.STDOUT)
 result={'test':name,'exit':r.returncode,'seconds':round(time.monotonic()-started,3)};results.append(result)
 print(json.dumps(result),flush=True)
 if r.returncode:print('\n'.join(path.read_text().splitlines()[-16:]),flush=True)
report={'version':'RC4-R4','scope':'Actual Win64 DLL with substituted OS/game; package static checks; NOT Windows/live game','results':results,'dll_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'source/build').glob('SAN14_*.dll')}}
(root/'audit/r4_suite_run.json').write_text(json.dumps(report,indent=2)+'\n')
sys.exit(any(x['exit'] for x in results))
