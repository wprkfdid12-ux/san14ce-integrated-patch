#!/usr/bin/env python3
import hashlib,json,zipfile
from pathlib import Path
R=Path(__file__).resolve().parents[1];D=R.parent/'deliverables';D.mkdir(exist_ok=True)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
build=json.loads((R/'analysis/install_fix_r23_1_build.json').read_text());verification=json.loads((R/'analysis/install_fix_r23_1_verification.json').read_text())
assert verification['status']=='PASS_OFFLINE_NOT_WINDOWS_INSTALLED'
assert verification['build_sha256']==sha(R/'analysis/install_fix_r23_1_build.json')
assert verification['compiled_forwarders_executed']==99 and verification['embedded_payloads']==5
for row in build['files']:assert sha(R/row['path'])==row['sha256']
for path,h in build['sources'].items():assert sha(R/path)==h,path
for path,h in verification['source_sha256'].items():assert sha(R/path)==h,path
kit=D/'FieldLeader3D_Final_R23_1_InstallFix.zip';setup=R/'native/bin/FieldLeader3D_Setup.exe'
readme=(R/'native/READ_FIRST_KO.txt').read_text().replace('\r\n','\n').replace('\n','\r\n').encode('utf-8-sig')
with zipfile.ZipFile(kit,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 z.write(setup,'FieldLeader3D_Setup.exe');z.writestr('READ_FIRST_KO.txt',readme)
with zipfile.ZipFile(kit) as z:
 assert z.testzip() is None and hashlib.sha256(z.read('FieldLeader3D_Setup.exe')).hexdigest()==sha(setup)
 assert z.read('READ_FIRST_KO.txt')==readme
manifest={'schema':'fieldleader3d.final.r23_1','revision':'R23.1','date':'2026-09-24','runnable':'native/bin/FieldLeader3D_Setup.exe','user_zip':kit.name,'user_zip_sha256':sha(kit),'setup_sha256':sha(setup),'one_time_install':True,'installer_fix_only':True,'runtime_byte_identical_to_R23':True,'vacant_only_slots':['version.dll','dinput8.dll','dxgi.dll','d3d9.dll'],'private_directory':'auto-selected FieldLeader3D_Auto or _2.._8; existing owned installs preferred','foreign_files_preserved':True,'compiled_proxy_abi_cases':99,'installer_transaction_groups':7,'windows_game_executed':False,'all_other_patches_live_verified':False,'current_state':'CURRENT_STATE.txt','exact_build':'analysis/install_fix_r23_1_build.json','verification':'analysis/install_fix_r23_1_verification.json'}
(R/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'user_zip':str(kit),'bytes':kit.stat().st_size,'sha256':sha(kit)}),flush=True)
checkpoint=D/'SAN14CE_FieldLeader3D_FINAL_R23_1_CHECKPOINT.zip'
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
