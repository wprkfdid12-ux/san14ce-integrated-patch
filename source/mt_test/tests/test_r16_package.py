"""R16 release integrity/build gate. No claim of live Windows DLL loading/gameplay."""
from pathlib import Path
import hashlib,json,struct,subprocess,sys,os,shutil
from package_syntax import balanced
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'analysis'))
from peutil import PE
OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,name):
 if not ok:raise AssertionError(name)
 OUT.append({'name':name,'pass':True});print('PASS',name,flush=True)
pkg=json.loads((R/'package.manifest.json').read_text());meta=json.loads((R/'payload/SAN14Mods/manifests/MeritTraits.json').read_text())
check(pkg['Version']==meta['version']=='RC4-R16-Compatibility-TraitRules' and meta['gacha_algorithm']==6,'R16 version and algorithm metadata agree')
check(pkg['Format']==2 and pkg['ReleaseReady'] and meta['release_ready'] and not pkg['LiveGameTested'] and not meta['live_game_tested'],'ready installer format 2, with live-game verification explicitly false')
files={p.relative_to(R/'payload').as_posix():sha(p) for p in (R/'payload').rglob('*') if p.is_file()}
expected={**pkg['Files'],**{x['Source']:x['SHA256'] for x in pkg['Loaders'].values()}}
check(files==expected and len(files)==10,'exact ten payload files match manifest, including three alternative loaders')
old=json.loads((R/'audit/r15_release_baseline/package.manifest.json').read_text(encoding='utf-8-sig'))
for n in ('SAN14_MeritTraits.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini'):
 check(old['Files'][n] in pkg['RecognizedPrevious'][n],'R15 no-state upgrade recognizes '+n)
for n in ('SAN14Mods/Core/SAN14_ModCore.dll','SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/MeritTraits/UI_Traits9_Hover.dll'):
 check(files[n]==old['Files'][n],'shared/private support bytes unchanged: '+n)
check(sha(R/'payload/loaders/dxgi.dll')==old['Files']['dxgi.dll'],'default loader is original common loader, only package location changed')
before={p.name:sha(p) for p in (R/'source/build').glob('SAN14_*.dll')}
subprocess.run(['bash',str(R/'source/build.sh'),'--main-only'],check=True,stdout=subprocess.DEVNULL)
for n,h in before.items():
 check(sha(R/'source/build'/n)==h,'deterministic main-only rebuild '+n)
 rel='SAN14Mods/MeritTraits/'+n if 'Resolver' in n else n
 check(files[rel]==h,'tested DLL equals payload '+n)
for p in (R/'payload').rglob('*.dll'):
 pe=PE(p)
 check(struct.unpack_from('<H',pe.b,pe.nt+4)[0]==0x8664 and struct.unpack_from('<H',pe.b,pe.opt)[0]==0x20b,'Win64 PE '+p.name)
 check(all(not(flags&0x80000000 and flags&0x20000000) for _,_,_,_,_,flags in pe.sections),'no writable executable section '+p.name)
current=PE(R/'payload/SAN14_MeritTraits.dll');prior=PE(R/'audit/r15_release_baseline/SAN14_MeritTraits.dll')
check(set(current.imports.values())==set(prior.imports.values()) and set(current.exports)==set(prior.exports),'public export/import ABI unchanged')
check(len(meta['hooks'])==39 and meta['trait_rules_r16']['male_excluded_ids']==[94,95,96,168,171],'complete resolver hook inventory and five female-only traits recorded')
for p in R.glob('*.ps1'):
 raw=p.read_bytes();check(raw.startswith(b'\xef\xbb\xbf') and b'\r\n' in raw and balanced(raw.decode('utf-8-sig')),'Windows encoding and balanced script '+p.name)
for p in R.glob('*.cmd'):
 s=p.read_text();check('DisableDelayedExpansion' in s and 'Sysnative' in s and '"%~dp0' in s,'quoted 64-bit launcher '+p.name)
pwsh=os.environ.get('R16_PWSH') or shutil.which('pwsh') or shutil.which('powershell')
if not pwsh:raise RuntimeError('Set R16_PWSH to PowerShell for the installer regression gate')
subprocess.run([pwsh,'-NoLogo','-NoProfile','-File',str(R/'tests/test_r16_installer.ps1'),'-SourceRoot',str(R)],check=True,stdout=subprocess.DEVNULL)
inst=json.loads((R/'audit/r16_installer_tests.json').read_text())
check(inst['passed']>=34 and inst['dll_sha256']==files['SAN14_MeritTraits.dll'],'actual PowerShell install/upgrade/uninstall/rollback and interruption fixtures passed with final DLL')
names=('r6_tests.json','r7_tests.json','r9_tests.json','r11_compat_tests.json','cp03_stack_tests.json','r11_tests.json','r12_tests.json','r13_tests.json','r14_tests.json','r15_domestic_tests.json','r16_trait_tests.json','r16_ui_tests.json','r16_compat_tests.json','r16_performance_tests.json')
for name in names:
 d=json.loads((R/'audit'/name).read_text());check(d['passed']>0 and len(d['tests'])==d['passed'],'executed regression report '+name)
b=json.loads((R/'audit/r16_ai_benchmark_r16.json').read_text());check(b['dll_sha256']==files['SAN14_MeritTraits.dll'],'AI timing refers to final DLL')
check(meta['ai_turn_r16']['rerolls_enabled'] and not meta['trait_rules_r16']['lower_row_rerolls'],'AI rerolls retained; lower row does not use tickets')
doc=(R/'검증결과.md').read_text();check(all(x in doc for x in ('139','1,088','3,264','재추첨권','실제 게임 FPS','미확인','ProxyCompat1')),'Korean report covers requirements and outstanding live checks')
(R/'audit/r16_package_tests.json').write_text(json.dumps({'passed':len(OUT),'tests':OUT,'dll_sha256':before,'scope':__doc__},ensure_ascii=False,indent=2)+'\n')
