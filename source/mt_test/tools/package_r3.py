"""Reproducible R3 metadata/staging/ZIP helper; never edits a user's game folder.
Run --stage after build, run gates, then --zip /absolute/output/folder.
Optional --baseline supplies an extracted R2 source for an incremental diff.
"""
from pathlib import Path
import argparse,difflib,hashlib,json,shutil,zipfile
R=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def dump(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def files():
 return sorted(p for p in R.rglob('*') if p.is_file() and not p.is_symlink()
  and not any(x.startswith('.') or x=='__pycache__' for x in p.relative_to(R).parts)
  and p.suffix not in ('.so','.obj','.lib','.map','.pdb','.pyc')
  and (not p.is_relative_to(R/'source/build') or p.name in ('SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll'))
  and p.name not in ('HANDOFF_MANIFEST.json','SHA256.json','core'))
def stage():
 for p in list(R.glob('*.ps1'))+list((R/'payload/SAN14Mods/modules').glob('*.ini')):
  # Mechanical Windows encoding/newline normalization, not semantic rewriting.
  text=p.read_text(encoding='utf-8-sig');p.write_bytes(b'\xef\xbb\xbf'+text.replace('\r\n','\n').replace('\n','\r\n').encode())
 for name,rel in [('SAN14_MeritTraits.dll','SAN14_MeritTraits.dll'),('SAN14_MeritResolver_CEKO.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll')]:shutil.copyfile(R/'source/build'/name,R/'payload'/rel)
 pkg=json.loads((R/'package.manifest.json').read_text());meta=json.loads((R/'payload/SAN14Mods/manifests/MeritTraits.json').read_text());base=json.loads((R/'audit/r2_release_baseline/package.manifest.json').read_text())
 pkg['Version']='RC4-R3';pkg['ReleaseChannel']='Installable R3 incremental candidate; live game unverified';pkg['LiveGameTested']=False
 pkg['KnownLimitations']=meta['known_limitations'];pkg['KnownLimitations'].append(meta['storage']['downgrade'])
 for name in ['SAN14_MeritTraits.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini']:
  values=pkg['RecognizedPrevious'].setdefault(name,[])
  if base['Files'][name] not in values:values.append(base['Files'][name])
 pkg['Files']={p.relative_to(R/'payload').as_posix():sha(p) for p in sorted((R/'payload').rglob('*')) if p.is_file()}
 assert len(pkg['Files'])==8;dump(R/'package.manifest.json',pkg)
def makezip(out,baseline):
 suite=json.loads((R/'audit/r3_suite_run.json').read_text());assert all(r['exit']==0 for r in suite['results'])
 package=json.loads((R/'audit/r3_package_tests.json').read_text());manifest=json.loads((R/'package.manifest.json').read_text())
 assert all(sha(R/'payload'/name)==h for name,h in manifest['Files'].items())
 for name,h in suite['dll_sha256'].items():assert sha(R/'source/build'/name)==h
 if baseline:
  chunks=[]
  for p in files():
   rel=p.relative_to(R)
   if rel.parts[0] not in ('source','tests') or p.suffix not in ('.c','.h','.inc','.s','.py','.sh','.txt'):continue
   old=baseline/rel;a=old.read_text().splitlines(keepends=True) if old.exists() else [];b=p.read_text().splitlines(keepends=True)
   chunks.extend(difflib.unified_diff(a,b,fromfile='R2/'+rel.as_posix(),tofile='R3/'+rel.as_posix()))
  (R/'audit/R2_to_R3.diff').write_text(''.join(chunks))
 paths=files();hashes={p.relative_to(R).as_posix():sha(p) for p in paths};dump(R/'SHA256.json',hashes)
 dump(R/'HANDOFF_MANIFEST.json',{'checkpoint':'RC4_R3_Incremental_Installable_Candidate','base_code':'supplied RC4-R2 handoff; not restarted','live_game_tested':False,'dll_assertions':package['executed_dll_assertions'],'package_static_assertions':package['passed'],'files':[{'path':p.relative_to(R).as_posix(),'size':p.stat().st_size,'sha256':hashes[p.relative_to(R).as_posix()]} for p in paths]})
 paths+= [R/'SHA256.json',R/'HANDOFF_MANIFEST.json'];out.mkdir(parents=True,exist_ok=True);results=[]
 for kind in ['Install','Source']:
  target=out/f'SAN14CE_MeritTraits_RC4_R3_{kind}.zip'
  if target.exists():raise FileExistsError('Refusing to overwrite an existing deliverable: '+str(target))
  selected=[p for p in paths if kind=='Source' or p.relative_to(R).parts[0]=='payload' or (p.parent==R and p.suffix in ('.txt','.md','.ps1','.cmd')) or p.name=='package.manifest.json']
  prefix=f'SAN14CE_MeritTraits_RC4_R3_{kind}/'
  with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
   for p in sorted(selected):
    zi=zipfile.ZipInfo(prefix+p.relative_to(R).as_posix(),(2026,9,19,0,0,0));zi.compress_type=zipfile.ZIP_DEFLATED;zi.external_attr=(0o100755 if p.suffix=='.sh' else 0o100644)<<16;z.writestr(zi,p.read_bytes())
  with zipfile.ZipFile(target) as z:assert z.testzip() is None
  results.append({'path':str(target),'bytes':target.stat().st_size,'sha256':sha(target),'files':len(selected)})
 print(json.dumps(results,ensure_ascii=False,indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--stage',action='store_true');p.add_argument('--zip',type=Path);p.add_argument('--baseline',type=Path);a=p.parse_args()
 if a.stage:stage()
 if a.zip:makezip(a.zip.resolve(),a.baseline.resolve() if a.baseline else None)
