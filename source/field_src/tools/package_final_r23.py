#!/usr/bin/env python3
"""Package the already verified final installable release and resumable source."""
import json,hashlib,zipfile
from pathlib import Path
R=Path(__file__).resolve().parents[1];D=R.parent/'deliverables';D.mkdir(exist_ok=True)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()
build=json.loads((R/'analysis/final_r23_build.json').read_text());v=json.loads((R/'analysis/poc_native_verification.json').read_text());n=json.loads((R/'analysis/final_r23_native_evidence.json').read_text());d=json.loads((R/'analysis/final_r23_distribution_verification.json').read_text())
assert v['status']=='PASS' and len(v['checks'])==194 and v['mock_engine_scenarios']==165
assert n['proxy_bridge_cases']==23 and d['embedded_payloads']==3
for row in build['files']:assert sha(R/row['path'])==row['sha256']
for path,h in build['sources'].items():assert sha(R/path)==h,path
kit=D/'FieldLeader3D_Final_R23.zip';setup=R/'native/bin/FieldLeader3D_Setup.exe'
readme=(R/'native/READ_FIRST_KO.txt').read_text().replace('\r\n','\n').replace('\n','\r\n').encode('utf-8-sig')
with zipfile.ZipFile(kit,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 z.write(setup,'FieldLeader3D_Setup.exe');z.writestr('READ_FIRST_KO.txt',readme)
with zipfile.ZipFile(kit) as z:
 assert z.testzip() is None and hashlib.sha256(z.read('FieldLeader3D_Setup.exe')).hexdigest()==sha(setup)
 assert z.read('READ_FIRST_KO.txt')==readme
print(json.dumps({'user_zip':str(kit),'bytes':kit.stat().st_size,'sha256':sha(kit)}),flush=True)
old=R/'manifest.json';previous=R/'analysis/previous_r22_manifest.json'
if old.exists() and not previous.exists():previous.write_bytes(old.read_bytes())
manifest={'schema':'fieldleader3d.final.r23','revision':'R23','date':'2026-09-24','runnable':'native/bin/FieldLeader3D_Setup.exe','user_zip':kit.name,'user_zip_sha256':sha(kit),'setup_sha256':sha(setup),'one_time_install':True,'siege':'all native except info UI','checks':194,'mock_scenarios':165,'proxy_abi_cases':23,'installer_transaction_groups':4,'windows_game_executed':False,'actual_fps_measured':False,'source_previous':'history/final_r22_base','current_requirements':'BATCH_PLAN_CURRENT.md','continue_from':'CURRENT_STATE.txt','deferred':['group horse scaling','formation infantry']}
old.write_text(json.dumps(manifest,indent=2)+'\n')
(R/'analysis/poc_r23_change.json').write_text(json.dumps({**manifest,'verification_reports':['analysis/poc_native_verification.json','analysis/final_r23_native_evidence.json','analysis/final_r23_distribution_verification.json'],'exact_build':'analysis/final_r23_build.json'},indent=2)+'\n')
checkpoint=D/'SAN14CE_FieldLeader3D_FINAL_R23_CHECKPOINT.zip'
excluded={'analysis/pe_objdump.txt','analysis/artifact_checksums.json'}
files=[p for p in sorted(R.rglob('*')) if p.is_file() and '__pycache__' not in p.parts and p.suffix!='.pyc' and 'video_frames' not in p.parts and p.relative_to(R).as_posix() not in excluded]
rows=[{'path':p.relative_to(R).as_posix(),'bytes':p.stat().st_size,'sha256':sha(p)} for p in files]
index=R/'analysis/artifact_checksums.json';index.write_text(json.dumps(rows,indent=2)+'\n');files.append(index)
with zipfile.ZipFile(checkpoint,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in files:z.write(p,'FieldLeader3D_PART01/'+p.relative_to(R).as_posix())
with zipfile.ZipFile(checkpoint) as z:
 assert z.testzip() is None
 for row in rows:assert hashlib.sha256(z.read('FieldLeader3D_PART01/'+row['path'])).hexdigest()==row['sha256'],row['path']
print(json.dumps({'checkpoint':str(checkpoint),'bytes':checkpoint.stat().st_size,'files':len(files),'sha256':sha(checkpoint),'crc_and_hashes':'PASS'}),flush=True)
