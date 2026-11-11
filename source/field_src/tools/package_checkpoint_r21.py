#!/usr/bin/env python3
"""Freeze the already-built and verified R21 project; never overwrite inputs or rebuild historical stages."""
import argparse,hashlib,json,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):
 h=hashlib.sha256()
 with Path(p).open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 change=json.loads((ROOT/'analysis/poc_r21_change.json').read_text())
 assert change['checks']==172 and change['mock_scenarios']==144
 for name,digest in change['source_sha256'].items():assert sha(ROOT/name)==digest,name
 old=ROOT/'manifest.json'; previous=ROOT/'analysis/previous_r20_manifest.json'
 if old.exists() and not previous.exists():previous.write_bytes(old.read_bytes())
 manifest=dict(schema='fieldleader3d.checkpoint.r21',revision='R21',date='2026-09-23',
  runnable='native/bin/FieldLeader3D_Test.exe',embedded_runtime=True,small_user_kit='FieldLeader3D_R21_Test.zip',
  status=change['status'],verification='analysis/poc_native_verification.json',change='analysis/poc_r21_change.json',
  current_instructions='BATCH_PLAN_CURRENT.md',handoff='CURRENT_STATE.txt',previous='history/first_field_r20_checkpoint',
  windows_game_test=False,visual_audio_fps_verification_pending=True,
  next='One R21 live user test, then authorized batch2 defeat/fade, nearby info placement, full appearance audit; batch3 stability')
 old.write_text(json.dumps(manifest,indent=2)+'\n')
 excluded={'analysis/pe_objdump.txt','analysis/artifact_checksums.json'}
 files=[p for p in sorted(ROOT.rglob('*')) if p.is_file() and '__pycache__' not in p.parts and p.suffix!='.pyc'
  and 'video_frames' not in p.parts and p.relative_to(ROOT).as_posix() not in excluded and p.resolve()!=a.output.resolve()]
 rows=[dict(path=p.relative_to(ROOT).as_posix(),bytes=p.stat().st_size,sha256=sha(p)) for p in files]
 index=ROOT/'analysis/artifact_checksums.json';index.write_text(json.dumps(rows,indent=2)+'\n');files.append(index)
 a.output.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(a.output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in files:z.write(p,'FieldLeader3D_PART01/'+p.relative_to(ROOT).as_posix())
 with zipfile.ZipFile(a.output) as z:
  assert z.testzip() is None
  for row in rows:
   assert hashlib.sha256(z.read('FieldLeader3D_PART01/'+row['path'])).hexdigest()==row['sha256'],row['path']
 print(json.dumps(dict(path=str(a.output.resolve()),bytes=a.output.stat().st_size,files=len(files),sha256=sha(a.output),archive_crc_and_hashes='PASS')))
if __name__=='__main__':main()
