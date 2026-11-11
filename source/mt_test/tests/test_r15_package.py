"""R15 artifact gate. Static installer checks; no Windows installation claim."""
from pathlib import Path
import hashlib,json,struct,subprocess,sys
from package_syntax import balanced
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'analysis'))
from peutil import PE
OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,name):
 if not ok:raise AssertionError(name)
 OUT.append({'name':name,'pass':True});print('PASS',name,flush=True)
pkg=json.loads((R/'package.manifest.json').read_text());meta=json.loads((R/'payload/SAN14Mods/manifests/MeritTraits.json').read_text())
check(pkg['Version']==meta['version']=='RC4-R15-RulerDomestic-AITurnPerf','R15 version agreement')
check(pkg['ReleaseReady'] and not pkg['LiveGameTested'] and not meta['live_game_tested'],'installable build does not claim live-game verification')
files={p.relative_to(R/'payload').as_posix():sha(p) for p in (R/'payload').rglob('*') if p.is_file()}
check(files==pkg['Files'] and len(files)==8,'exact eight payload files match SHA256 manifest')
for version in (13,14):
 old=json.loads((R/f'audit/r{version}_release_baseline/package.manifest.json').read_text())
 changed={n for n in files if files[n]!=old['Files'][n]}
 check(changed=={'SAN14_MeritTraits.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini'},f'R{version} baseline: only main DLL and two metadata files change')
 for n in changed:check(old['Files'][n] in pkg['RecognizedPrevious'][n],f'R{version} upgrade recognizes '+n)
sourcebase=json.loads((R/'audit/r14_release_baseline/source_sha256.json').read_text())
changed={n for n,h in sourcebase.items() if sha(R/'source'/n)!=h}
check(changed=={'runtime_flow.inc','gacha.c','gacha.h','native_rules.inc','event_log.inc','merit_runtime.c'},'source changes limited to domestic bonuses, cached calculations, guard and version')
for n in ('merit_resolver.c','merit_core.c','slot_store.inc','native_ui.inc','native_controls.inc','native_selection.inc','native_notice.inc','display_patch.inc','presets_x_adapter.inc','hover_x_adapter.inc','official_db.h','collab_db.h'):
 check(sha(R/'source'/n)==sourcebase[n],'unchanged original dependency/DB/state/UI: '+n)
for n,h in json.loads((R/'audit/r13_release_baseline/script_sha256.json').read_text()).items():check(sha(R/n)==h,'existing installer/backup/rollback/collector preserved: '+n)
before={p.name:sha(p) for p in (R/'source/build').glob('SAN14_*.dll')}
subprocess.run(['bash',str(R/'source/build.sh'),'--main-only'],check=True,stdout=subprocess.DEVNULL)
for n,h in before.items():
 check(sha(R/'source/build'/n)==h,'deterministic main-only rebuild: '+n)
 rel='SAN14Mods/MeritTraits/'+n if 'Resolver' in n else n
 check(files[rel]==h,'tested binary equals payload: '+n)
for p in (R/'payload').rglob('*.dll'):
 q=PE(p);check(struct.unpack_from('<H',q.b,q.nt+4)[0]==0x8664 and struct.unpack_from('<H',q.b,q.opt)[0]==0x20b,'Win64 PE: '+p.name)
 check(all(not(flags&0x80000000 and flags&0x20000000) for _,_,_,_,_,flags in q.sections),'no writable executable section: '+p.name)
q=PE(R/'payload/SAN14_MeritTraits.dll');old=PE(R/'audit/r14_release_baseline/SAN14_MeritTraits.dll')
check(set(q.imports.values())==set(old.imports.values()),'optimizations add no imported OS functions')
check(set(q.exports)==set(old.exports),'public DLL export ABI unchanged')
rv=q.exports['SAN14_MeritPerfV1'];check(bytes(q.m[rv:rv+16])==b'MTPERF14'+struct.pack('<II',1,128),'R14 read-only observer ABI remains compatible')
check(meta['ai_turn_r15']['rerolls_enabled'] and meta['ai_turn_r15']['reroll_rules_unchanged'] and not meta['ruler_domestic_r15']['bottom_row_uses_rng_or_reroll_tickets'],'release explicitly preserves AI rerolls and excludes bottom row tickets')
for p in R.glob('*.ps1'):
 raw=p.read_bytes();check(raw.startswith(b'\xef\xbb\xbf') and b'\r\n' in raw and balanced(raw.decode('utf-8-sig')),'Windows script encoding and static syntax: '+p.name)
for p in R.glob('*.cmd'):
 s=p.read_text();check('DisableDelayedExpansion' in s and 'Sysnative' in s and '"%~dp0' in s,'quoted 64-bit launcher: '+p.name)
for n in ('r6_tests.json','r7_tests.json','r9_tests.json','r11_compat_tests.json','cp03_stack_tests.json','r11_tests.json','r12_tests.json','r13_tests.json','r14_tests.json','r14_performance_tests.json','r15_domestic_tests.json','r15_gacha_tests.json','r15_performance_tests.json'):
 d=json.loads((R/'audit'/n).read_text());check(d['passed']>0 and len(d['tests'])==d['passed'],'executed regression report: '+n)
d=json.loads((R/'audit/r15_ai_benchmark_r15.json').read_text());check(d['dll_sha256']==files['SAN14_MeritTraits.dll'],'AI benchmark identifies exact final payload binary')
doc=(R/'검증결과.md').read_text();check(all(s in doc for s in ('1,088','3,264','재추첨권','실제 게임 FPS','마제','127.1','42.3')),'documentation reports all requested scopes and measurement limits')
report={'version':pkg['Version'],'scope':__doc__,'passed':len(OUT),'tests':OUT,'dll_sha256':before}
(R/'audit/r15_package_tests.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print('PASS R15 PACKAGE',len(OUT))
