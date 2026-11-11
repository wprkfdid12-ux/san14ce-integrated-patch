"""R16 full regression gate. Synthetic OS/game boundaries, not a live game."""
from pathlib import Path
import hashlib,json,subprocess,sys,time,os
root=Path(__file__).resolve().parents[1]
suite=['test_r6.py','test_r7.py','test_r9.py','test_r11_compat.py','test_stack_probe.py','test_r11.py','test_r12.py','test_r13.py','test_r14.py','test_r15_domestic.py','test_r16_traits.py','test_r16_ui.py','test_r16_compat.py','test_r16_performance.py']
if '--with-package' in sys.argv:suite.append('test_r16_package.py')
results=[]
for name in suite:
 started=time.monotonic();path=root/'audit'/('r16_'+name.replace('.py','.log'))
 env=os.environ.copy();env['MERIT_MAIN_ONLY']='1'
 with path.open('w') as log:r=subprocess.run([sys.executable,'-u',str(root/'tests'/name)],cwd=root,stdout=log,stderr=subprocess.STDOUT,env=env)
 result={'test':name,'exit':r.returncode,'seconds':round(time.monotonic()-started,3)};results.append(result)
 print(json.dumps(result),flush=True)
 if r.returncode:print('\n'.join(path.read_text().splitlines()[-24:]),flush=True)
names=['r6_tests.json','r7_tests.json','r9_tests.json','r11_compat_tests.json','cp03_stack_tests.json','r11_tests.json','r12_tests.json','r13_tests.json','r14_tests.json','r15_domestic_tests.json','r16_trait_tests.json','r16_ui_tests.json','r16_compat_tests.json','r16_performance_tests.json']
if '--with-package' in sys.argv:names.append('r16_package_tests.json')
report={'version':'RC4-R16-Compatibility-TraitRules','scope':__doc__,'results':results,'dll_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'source/build').glob('SAN14_*.dll')}}
report['assertion_reports']={n:json.loads((root/'audit'/n).read_text())['passed'] for n in names if (root/'audit'/n).exists()}
report['total_passed_assertions']=sum(report['assertion_reports'].values())
(root/'audit/r16_suite_run.json').write_text(json.dumps(report,indent=2)+'\n')
sys.exit(any(x['exit'] for x in results))
