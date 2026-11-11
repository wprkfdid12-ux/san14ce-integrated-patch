"""R14 artifact gate: exact changed DLL, guards, upgrade and observer contract.
PowerShell checks here are static only. No Windows installation/FPS certification.
"""
from pathlib import Path
import hashlib,json,struct,subprocess,sys
from package_syntax import balanced
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'analysis'))
from peutil import PE
OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,text):
 if not ok:raise AssertionError(text)
 OUT.append({'name':text,'pass':True});print('PASS',text,flush=True)
pkg=json.loads((R/'package.manifest.json').read_text());old=json.loads((R/'audit/r13_release_baseline/package.manifest.json').read_text())
meta=json.loads((R/'payload/SAN14Mods/manifests/MeritTraits.json').read_text())
check(pkg['Version']==meta['version']=='RC4-R14-MainMenuQueryPerf','R14 package and module version agree')
check(pkg['ReleaseReady'] and not pkg['LiveGameTested'] and not meta['live_game_tested'] and not meta['main_menu_query_r14']['live_fps_recovery_verified'],'installable build explicitly distinguishes unverified live FPS')
files={p.relative_to(R/'payload').as_posix():sha(p) for p in (R/'payload').rglob('*') if p.is_file()}
check(files==pkg['Files'] and len(files)==8,'exact eight managed payload files match manifest SHA256')
changed={n for n in files if files[n]!=old['Files'][n]}
check(changed=={'SAN14_MeritTraits.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini'},'only main DLL and two metadata files changed from supplied R13')
for n in changed:check(old['Files'][n] in pkg['RecognizedPrevious'][n],'R13 upgrade hash recognized: '+n)
sourcebase=json.loads((R/'audit/r13_release_baseline/source_sha256.json').read_text())
changedsource={n for n,h in sourcebase.items() if sha(R/'source'/n)!=h}
check(changedsource=={'merit_runtime.c','native_rules.inc','merit_history.inc','runtime_flow.inc','hover_x_adapter.inc','event_log.inc','build.sh'},'production diff limited to native query contract, counters/version and main-only build option')
for n in ('gacha.c','gacha.h','merit_resolver.c','merit_core.c','slot_store.inc','native_ui.inc','native_controls.inc','native_selection.inc','native_notice.inc','display_patch.inc','presets_x_adapter.inc'):
 check(sha(R/'source'/n)==sourcebase[n],'R13 gameplay/storage/renderer preserved: '+n)
for n,h in json.loads((R/'audit/r13_release_baseline/script_sha256.json').read_text()).items():
 check(sha(R/n)==h,'R13 existing installer/backup/rollback/collector bytes preserved: '+n)
before={p.name:sha(p) for p in (R/'source/build').glob('SAN14_*.dll')}
subprocess.run(['bash',str(R/'source/build.sh'),'--main-only'],check=True,stdout=subprocess.DEVNULL)
for n,h in before.items():
 check(sha(R/'source/build'/n)==h,'deterministic main-only rebuild: '+n)
 rel='SAN14Mods/MeritTraits/'+n if 'Resolver' in n else n
 check(files[rel]==h,'tested build equals payload: '+n)
for p in (R/'payload').rglob('*.dll'):
 q=PE(p)
 check(struct.unpack_from('<H',q.b,q.nt+4)[0]==0x8664 and struct.unpack_from('<H',q.b,q.opt)[0]==0x20b,'Win64 PE: '+p.name)
 check(all(not(flags&0x80000000 and flags&0x20000000) for _,_,_,_,_,flags in q.sections),'no writable executable section: '+p.name)
q=PE(R/'payload/SAN14_MeritTraits.dll');b=PE(R/'audit/r13_release_baseline/SAN14_MeritTraits.dll')
check(set(q.imports.values())==set(b.imports.values()),'counter observation adds no imported OS service')
check(set(q.exports)==set(b.exports)|{'SAN14_MeritPerfV1'},'existing exports preserved; one read-only-observer data export added')
rv=q.exports['SAN14_MeritPerfV1'];block=bytes(q.m[rv:rv+128])
check(block[:8]==b'MTPERF14' and struct.unpack_from('<II',block,8)==(1,128) and not any(block[16:]),'observer data ABI is 128 bytes with zero initial counters')
check(any(va<=rv and rv+128<=va+max(vs,rs) and flags&0x80000000 and not flags&0x20000000 for _,va,vs,rs,_,flags in q.sections),'observer export is data, never executable code')
for p in R.glob('*.ps1'):
 raw=p.read_bytes();check(raw.startswith(b'\xef\xbb\xbf') and b'\r\n' in raw,'Windows script encoding: '+p.name)
 check(balanced(raw.decode('utf-8-sig')),'static PowerShell delimiter check: '+p.name)
for p in R.glob('*.cmd'):
 s=p.read_text();check('DisableDelayedExpansion' in s and 'Sysnative' in s and '"%~dp0' in s,'quoted 64-bit launcher: '+p.name)
s=(R/'CaptureUIPerformance.ps1').read_text(encoding='utf-8-sig')
check('OpenProcess(0x410,false,id)' in s and 'ReadProcessMemory' in s,'observer requests query/read-only process access')
check(all(x not in s for x in ('WriteProcessMemory','VirtualAllocEx','CreateRemoteThread','SuspendThread','SetThreadContext')),'observer has no target write, injection or suspension APIs')
check('Start-Sleep -Milliseconds 250' in s and '[int]$Seconds=30' in s and 'SAN14_MeritPerfV1' in s,'bounded 4Hz observer resolves named data export')
for report in ('r6_tests.json','r7_tests.json','r9_tests.json','r11_compat_tests.json','cp03_stack_tests.json','r11_tests.json','r12_tests.json','r13_tests.json','r14_tests.json','r14_performance_tests.json'):
 d=json.loads((R/'audit'/report).read_text());check(d['passed']>0 and len(d['tests'])==d['passed'],'executed regression report: '+report)
bench=json.loads((R/'audit/r14_performance_tests.json').read_text())
check(bench['current_dll_sha256']==files['SAN14_MeritTraits.dll'] and bench['baseline_dll_sha256']==old['Files']['SAN14_MeritTraits.dll'],'performance comparison identifies exact shipped R13 and R14 DLLs')
doc=(R/'검증결과.md').read_text();check('아직' in doc and '실제 게임 FPS' in doc and '3,000' in doc,'documentation separates observation, operation counts and unverified live FPS')
d={'version':pkg['Version'],'scope':__doc__,'passed':len(OUT),'checks':OUT,'dll_sha256':before}
(R/'audit/r14_package_tests.json').write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n');print('PASS R14 PACKAGE',len(OUT))
