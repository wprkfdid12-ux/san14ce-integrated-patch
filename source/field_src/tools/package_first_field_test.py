#!/usr/bin/env python3
"""Small, standalone user test kit; no copyrighted game payloads included."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--output',type=Path,required=True); args=ap.parse_args()
    root=Path(__file__).resolve().parents[1]; native=root/'native'
    build=json.loads((root/'analysis/poc_native_build.json').read_text())
    verify=json.loads((root/'analysis/poc_native_verification.json').read_text())
    assert build['revision']=='R22'
    assert verify['mock_engine_scenarios']==158
    assert verify['status']=='PASS' and len(verify['checks'])>=187 and all(c['status']=='PASS' for c in verify['checks'])
    assert sha(root/'analysis/poc_native_build.json')==verify['build_report_sha256']
    for row in build['artifacts']: assert sha(root/row['path'])==row['sha256']
    for rel,value in verify['tested_source_hashes'].items(): assert sha(root/rel)==value
    change=json.loads((root/'analysis/poc_r22_change.json').read_text())
    for rel,value in change['source_sha256'].items(): assert sha(root/rel)==value,rel
    files={
        'FieldLeader3D.exe':native/'bin/FieldLeader3D_Test.exe',
        'READ_FIRST_KO.txt':native/'READ_FIRST_KO.txt',
        'technical/ENGINE_CONTRACT.md':native/'ENGINE_CONTRACT.md',
        'technical/build.json':root/'analysis/poc_native_build.json',
        'technical/verification.json':root/'analysis/poc_native_verification.json',
        'technical/r22_changes.json':root/'analysis/poc_r22_change.json',
        'technical/r22_native_evidence.json':root/'analysis/batch2_r22_evidence.json',
        'technical/r22_roster_review.json':root/'analysis/r22_received_roster_review.json',
        'technical/r21_native_evidence.json':root/'analysis/combat_r21_evidence.json',
        'technical/r20_received_review.json':root/'analysis/poc_r20_received_review.json',
        'technical/r20_native_evidence.json':root/'analysis/combat_r20_evidence.json',
        'technical/r20_audio_matches.json':root/'analysis/r20_audio_sample_matches.json',
        'technical/r18_received_result.json':root/'analysis/poc_r18_user_result.json',
        'technical/batch1_evidence.json':root/'analysis/batch1_r18.json',
        'technical/BATCH_PLAN_CURRENT.md':root/'BATCH_PLAN_CURRENT.md',
        'technical/mounted_motion.json':root/'analysis/mounted_motion_r17.json',
        'technical/all_appearances.json':root/'analysis/appearance_validation_r16/report.json',
    }
    for p in sorted(native.glob('*.cpp'))+sorted(native.glob('*.h')):
        files['source/native/'+p.name]=p
    for p in sorted((native/'tests').glob('*')):
        if p.is_file(): files['source/native/tests/'+p.name]=p
    for name in ('build_field_poc.py','verify_field_poc.py','verify_batch1.py','verify_batch2_r22.py','verify_all_appearances.py',
                 'state_assets_analysis.py','runtime_disasm.py','pe_probe.py','record_r22_change.py','verify_combat_r20.py','verify_combat_r21.py','package_checkpoint_r22.py'):
        files['source/tools/'+name]=root/'tools'/name
    index={name:{'bytes':p.stat().st_size,'sha256':sha(p)} for name,p in files.items()}
    root_name='FieldLeader3D_R22/'
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,p in files.items(): z.write(p,root_name+name)
        z.writestr(root_name+'technical/checksums.json',json.dumps(index,indent=2)+'\n')
        z.writestr(root_name+'source/BUILD_NOTE.txt',
            'Sources and generated headers for the distributed DLL/EXE are included.\n'
            'Rebuilding needs Zig 0.14.1 and the preserved original EXE/runtime inputs from the full R22 checkpoint.\n'
            'R22 rebuilds the DLL and embeds its exact bytes into one GUI executable.\n'
            'The source directory retains project/tools and project/native layout; supply the original inputs directory.\n'
            'Rebuild the DLL and regenerate dll_build.h before rebuilding the loader; export RVAs/hashes are paired.\n'
            'Users can run the prebuilt EXE; no compiler, PowerShell or separate resource collection is required.\n')
    with zipfile.ZipFile(args.output) as z:
        assert z.testzip() is None
        for name,row in index.items(): assert hashlib.sha256(z.read(root_name+name)).hexdigest()==row['sha256']
        root_files=[n[len(root_name):] for n in z.namelist() if '/' not in n[len(root_name):]]
        assert sorted(root_files)==['FieldLeader3D.exe','READ_FIRST_KO.txt'],root_files
    report={'path':str(args.output.resolve()),'bytes':args.output.stat().st_size,
            'sha256':sha(args.output),'crc_and_per_file_hashes':'PASS',
            'game_inputs_included':False,'windows_game_test':'NOT_RUN'}
    (root/'analysis/poc_user_kit.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
