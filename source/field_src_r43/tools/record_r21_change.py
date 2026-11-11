#!/usr/bin/env python3
"""Bind the R21 test executable, exact source and offline evidence."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    b=json.loads((ROOT/'analysis/poc_native_build.json').read_text());v=json.loads((ROOT/'analysis/poc_native_verification.json').read_text())
    e=json.loads((ROOT/'analysis/combat_r21_evidence.json').read_text())
    assert b['revision']=='R21' and v['status']=='PASS' and len(v['checks'])==172 and v['mock_engine_scenarios']==144
    assert v['build_report_sha256']==sha(ROOT/'analysis/poc_native_build.json')
    assert v['combat_r21_report_sha256']==sha(ROOT/'analysis/combat_r21_evidence.json')
    for mapping in (b['source_files'],v['tested_source_hashes'],e['source_sha256']):
        for name,digest in mapping.items():assert sha(ROOT/name)==digest,name
    for row in b['artifacts']:assert sha(ROOT/row['path'])==row['sha256']
    hist=ROOT/'history/first_field_r20_checkpoint'
    for name,digest in json.loads((hist/'snapshot_manifest.json').read_text()).items():assert sha(hist/name)==digest,name
    paths=set(b['source_files'])|set(v['tested_source_hashes'])|{row['path'] for row in b['artifacts']}
    paths.update(('CURRENT_STATE.txt','PART_03_HANDOFF.txt','PART_03_RESULT.txt','BATCH_PLAN_CURRENT.md','README_KO.txt',
        'MASTER_SPEC_CURRENT.txt','FieldLeader3D_TODO_HorseScale_20260923.md','native/READ_FIRST_KO.txt','native/ENGINE_CONTRACT.md',
        'analysis/poc_native_build.json','analysis/poc_native_contract.json','analysis/poc_native_verification.json',
        'analysis/combat_r21_evidence.json','analysis/combat_r20_evidence.json','analysis/poc_r20_received_review.json',
        'inputs/r20_received_result.zip','inputs/r20_received_video_a.mp4','inputs/r20_received_video_b.mp4','inputs/r20_received_video_c.mp4',
        'history/first_field_r20_checkpoint/snapshot_manifest.json',
        'tools/build_field_poc.py','tools/verify_field_poc.py','tools/verify_combat_r21.py',
        'tools/package_first_field_test.py','tools/package_checkpoint_r21.py','tools/record_r21_change.py'))
    report=dict(schema='fieldleader3d.change.r21',date='2026-09-23',status='INTEGRATED_TEST_BUILD_OFFLINE_VERIFIED_GAME_TEST_PENDING',
        changes=['Per-motion native weapon hand for all25 bodies',
            'Original horse dust creation requires actual motion; private normal attack VFX suppressed',
            'Current replacement army graphical-update scope also gates stationary hidden-source motion effects',
            'Own strong tactic always first; remaining overlapping poses follow actual opponent A+D order',
            'Immutable per-attack guard outcome; independent different-source reactions; no repeated reaction restart',
            'Native tactic hit/block audio at original contact timing independent of victim motion; no duplicate hit audio',
            'Original combat troop ambience40%, independent of added combat/hoof audio; safe current-voice restore on Stop',
            'One GUI executable with updated plain Korean instructions and automatic result ZIP'],
        native_guards=b['runtime_guards'],e8_hooks=26,vtable_hooks=1,native_hook_count=27,
        checks=len(v['checks']),mock_scenarios=144,native_r21_evidence_checks=len(e['checks']),body_action_hand_bindings=150,
        appearance_integration_combinations=1464,original_damage_ai_save_rng_unchanged=True,
        original_guard_probability_from_r20_preserved=True,preserved_visual_quality=True,preserved_unit_capacity=500,
        preserved_full_actor_update_rate=True,actual_windows_game_execution=False,actual_gpu_motion_verified=False,
        actual_audio_device_executed=False,actual_listening_verified=False,actual_fps_measured=False,
        limitations=['Windows rendering/audio and actual FPS require the supplied integrated user test',
            'Motion and native per-voice volume paths are verified offline; residual particle tails and perceived ambience need live confirmation',
            'Fresh original audio callback spatial parameters are preserved; target reaction is decoupled from pose, not a new spatial audio engine',
            'One current pending cosmetic tactic per victim; a later tactic may supersede an older pending visual reaction',
            'Frame-perfect tactic cutscene alignment not certified'],
        deferred_with_user_authorization=['Defeat dismount/fade without original troop flash in batch2','Portrait/info occlusion near current info position in batch2',
            'Full appearance refresh/colors audit in batch2','Lifecycle/performance batch3','Group horse scaling later','Formation infantry later'],
        next='Inspect one R21 automatic result ZIP; then proceed with batch2 as authorized',
        source_sha256={name:sha(ROOT/name) for name in sorted(paths)})
    (ROOT/'analysis/poc_r21_change.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','native_guards','native_hook_count','checks','mock_scenarios')}))
if __name__=='__main__':main()
