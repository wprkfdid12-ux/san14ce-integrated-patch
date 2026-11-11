"""R17 packaging after all gates pass. Never installs into the user's game."""
from pathlib import Path
import argparse,difflib,hashlib,json,zipfile,shutil
R=Path(__file__).resolve().parents[1]
VERSION='RC4-R17-TurnTransitionPerf'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def dump(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n')
def files():
 return sorted(p for p in R.rglob('*') if p.is_file() and not p.is_symlink()
  and not any(x.startswith('.') or x=='__pycache__' for x in p.relative_to(R).parts)
  and p.suffix not in ('.so','.obj','.lib','.map','.pdb','.pyc')
  and (not p.is_relative_to(R/'source/build') or p.name in ('SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll'))
  and not (p.name=='core' or p.name.startswith('core.'))
  and not (p.parent==R and p.name in ('HANDOFF_MANIFEST.json','SHA256.json')))
def makezip(out,baseline):
 suite=json.loads((R/'audit/r17_suite_run.json').read_text())
 assert all(x['exit']==0 for x in suite['results']) and any(x['test']=='test_r17_package.py' for x in suite['results'])
 pkg=json.loads((R/'package.manifest.json').read_text());assert pkg['ReleaseReady'] and pkg['Version']==VERSION
 expected={**pkg['Files'],**{x['Source']:x['SHA256'] for x in pkg['Loaders'].values()}}
 assert all(sha(R/'payload'/n)==h for n,h in expected.items())
 for n,h in suite['dll_sha256'].items():assert sha(R/'source/build'/n)==h
 for n in ('r17_installer_tests.json','r17_ai_benchmark_r17.json'):
  proof=json.loads((R/'audit'/n).read_text());assert proof['dll_sha256']==pkg['Files']['SAN14_MeritTraits.dll']
 if baseline:
  chunks=[]
  for p in files():
   rel=p.relative_to(R)
   if not (rel.parts[0] in ('source','tests','tools') or p.parent==R):continue
   if p.suffix not in ('.c','.h','.inc','.s','.py','.sh','.ps1','.txt'):continue
   old=baseline/rel;a=old.read_text(encoding='utf-8-sig').splitlines(keepends=True) if old.exists() else []
   chunks.extend(difflib.unified_diff(a,p.read_text(encoding='utf-8-sig').splitlines(keepends=True),fromfile='R16/'+rel.as_posix(),tofile='R17/'+rel.as_posix()))
  (R/'audit/R16_to_R17.diff').write_text(''.join(chunks))
 out.mkdir(parents=True,exist_ok=True);results=[];paths=files()
 for kind in ('Install','Source'):
  chosen=[p for p in paths if kind=='Source' or p.relative_to(R).parts[0]=='payload' or
    (p.parent==R and (p.suffix in ('.ps1','.cmd') or p.name in ('먼저읽기.txt','변경내역.txt','검증결과.md','실게임확인.txt','package.manifest.json')))]
  hashes={p.relative_to(R).as_posix():sha(p) for p in chosen}
  handoff={'checkpoint':VERSION,'baseline':'R16 Compatibility-TraitRules','live_game_tested':False,'dll_sha256':suite['dll_sha256'],'suite':suite,'installer_assertions':json.loads((R/'audit/r17_installer_tests.json').read_text())['passed'],'scope':'Compiled Win64 code and PowerShell filesystem fixtures; native Windows loading/game graphics untested.'}
  extras={'SHA256.json':json.dumps(hashes,ensure_ascii=False,indent=2)+'\n','HANDOFF_MANIFEST.json':json.dumps(handoff,ensure_ascii=False,indent=2)+'\n'}
  if kind=='Source':
   for n,data in extras.items():(R/n).write_text(data)
  target=out/f'SAN14CE_MeritTraits_R17_{kind}.zip';prefix=f'SAN14CE_MeritTraits_R17_{kind}/'
  with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
   for name,data in [(p.relative_to(R).as_posix(),p.read_bytes()) for p in chosen]+[(n,s.encode()) for n,s in extras.items()]:
    zi=zipfile.ZipInfo(prefix+name,(2026,9,23,0,0,0));zi.compress_type=zipfile.ZIP_DEFLATED;zi.external_attr=(0o100755 if name.endswith('.sh') else 0o100644)<<16
    z.writestr(zi,data)
  with zipfile.ZipFile(target) as z:
   assert z.testzip() is None
   for name,h in hashes.items():assert hashlib.sha256(z.read(prefix+name)).hexdigest()==h
  results.append({'path':str(target),'bytes':target.stat().st_size,'sha256':sha(target),'files':len(chosen)+2})
 report=out/'SAN14CE_MeritTraits_R17_검증결과.md';shutil.copy2(R/'검증결과.md',report)
 results.append({'path':str(report),'bytes':report.stat().st_size,'sha256':sha(report)})
 dump(out/'SAN14CE_MeritTraits_R17_Artifacts.json',results);print(json.dumps(results,ensure_ascii=False,indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--baseline',type=Path);a=p.parse_args()
 makezip(a.output.resolve(),a.baseline.resolve() if a.baseline else None)
