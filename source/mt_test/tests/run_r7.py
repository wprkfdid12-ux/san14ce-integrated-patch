"""R7 current continuation gate. Not a live Windows/game certification."""
from pathlib import Path
import hashlib,json,subprocess,sys,time
root=Path(__file__).resolve().parents[1]
suite=['test_r6.py','test_r7.py','test_r7_compat.py','test_stack_probe.py']
if '--with-package' in sys.argv:suite.append('test_r7_package.py')
results=[]
for name in suite:
 started=time.monotonic();path=root/'audit'/('r7_'+name.replace('.py','.log'))
 with path.open('w') as log:r=subprocess.run([sys.executable,'-u',str(root/'tests'/name)],cwd=root,stdout=log,stderr=subprocess.STDOUT)
 result={'test':name,'exit':r.returncode,'seconds':round(time.monotonic()-started,3)};results.append(result)
 print(json.dumps(result),flush=True)
 if r.returncode:print('\n'.join(path.read_text().splitlines()[-18:]),flush=True)
report={'version':'RC4-R7-NativeDialogs','scope':'Win64 DLL and original native state tick, substituted OS/game/widgets; NOT Windows/live game','results':results,'dll_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'source/build').glob('SAN14_*.dll')}}
(root/'audit/r7_suite_run.json').write_text(json.dumps(report,indent=2)+'\n')
sys.exit(any(x['exit'] for x in results))
