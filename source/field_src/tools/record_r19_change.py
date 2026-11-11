#!/usr/bin/env python3
"""Bind the actual R19 build and offline results, separately from received R18 evidence."""
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    build=json.loads((ROOT/'analysis/poc_native_build.json').read_text())
    verify=json.loads((ROOT/'analysis/poc_native_verification.json').read_text())
    native=json.loads((ROOT/'analysis/batch1_r18.json').read_text())
    received=json.loads((ROOT/'analysis/poc_r18_user_result.json').read_text())
    assert build['revision']=='R19' and verify['status']=='PASS'
    assert len(verify['checks'])==141 and verify['mock_engine_scenarios']==115
    assert verify['build_report_sha256']==sha(ROOT/'analysis/poc_native_build.json')
    assert verify['batch1_report_sha256']==sha(ROOT/'analysis/batch1_r18.json')
    for mapping in (build['source_files'],verify['tested_source_hashes'],native['source_sha256']):
        for rel,value in mapping.items(): assert sha(ROOT/rel)==value,rel
    for row in build['artifacts']: assert sha(ROOT/row['path'])==row['sha256']
    for rel,row in received['evidence'].items(): assert sha(ROOT/rel)==row['sha256']
    assert sha(ROOT/'MASTER_SPEC_CURRENT.txt')=='a1e834427346d9b3d151492c728a3b0775aa3627a177c9994939b176bf8f5a8e'
    historical=ROOT/'history/first_field_r18_checkpoint'
    for rel,value in json.loads((historical/'snapshot_manifest.json').read_text()).items():
        assert sha(historical/rel)==value,rel
    paths=set(build['source_files'])|set(verify['tested_source_hashes'])
    paths.update(row['path'] for row in build['artifacts'])
    paths.update(('CURRENT_STATE.txt','PART_03_HANDOFF.txt','PART_03_RESULT.txt','BATCH_PLAN_CURRENT.md',
        'MASTER_SPEC_CURRENT.txt','FieldLeader3D_TODO_HorseScale_20260923.md',
        'native/READ_FIRST_KO.txt','native/ENGINE_CONTRACT.md',
        'analysis/poc_native_build.json','analysis/poc_native_contract.json','analysis/poc_native_verification.json',
        'analysis/batch1_r18.json','analysis/mounted_motion_r17.json','analysis/appearance_validation_r16/report.json',
        'analysis/poc_r18_user_result.json','analysis/hoof_audio_requirements.json','analysis/HoofAudio_NEXT.md',
        'history/first_field_r18_checkpoint/snapshot_manifest.json',
        'tools/build_field_poc.py','tools/verify_field_poc.py','tools/verify_batch1.py',
        'tools/package_first_field_test.py','tools/package_checkpoint.py','tools/record_r19_change.py'))
    report={
        'schema':'fieldleader3d.change.r19','date':'2026-09-23',
        'status':'COMBAT_CORRECTION_BUILT_OFFLINE_VERIFIED_AWAITING_ONE_GAME_TEST',
        'received_baseline':'analysis/poc_r18_user_result.json',
        'user_amendment':'Standing horse with sideways periodic normal attack during engagement; forward strong attack with brief gallop for tactics; hit only on damaging tactics; preserve visible quality while reducing redundant work.',
        'changes':[
            'Normal attack body9/horse0, native152-frame attack and45-frame original body55 battle pause; horse clock not restarted by rider-only changes',
            'Current native field yaw minus pi/2 only for private normal combat/engaged idle/hit; gameplay actor orientation and position untouched',
            'Native tactic effect-stage body19/horse1 with forward field yaw, original180-frame completion, return to current engagement/idle/movement',
            'Positive native tactic damage only body39/horse0; normal losses/recovery/buffs never cause hit reaction',
            'Indexed current-generation army lookup retains exact ID/person/active checks; frame timestamp reused',
            'Every log record retained; routine disk flush grouped to1second, critical lifetime boundaries immediately flushed',
            'Private update/draw QPC cost samples automatically included in result log; no GPU/FPS improvement claim',
            'Bounded native audio argument probes added because R18 recorded zero successful hoof plays',
            'One paired DLL embedded in one GUI EXE with automatic result ZIP'
        ],
        'preserved':['Original model/body/horse color/weapon selection and original animation assets',
                     'Full native animation and attachment update per unit per frame',
                     '500 supported units, three submitted models per ready unit, two creates/frame, eight pending',
                     'Gameplay/save/duel/AI/damage and native field movement untouched',
                     'Water visuals/audio and audio ownership/no-overlap rules',
                     'Batch2/3 and deferred horse scale/infantry scope'],
        'native_guards':build['runtime_guards'],'native_hook_count':21,
        'checks':141,'mock_scenarios':115,'new_combat_performance_scenarios':12,
        'appearance_integration_combinations':1464,'body_attack_strong_hit_bindings':75,
        'original_byte_checks':9,'native_view_cases':6,
        'actual_windows_game_execution':False,'actual_gpu_motion_verified':False,
        'side_yaw_policy':{'relative_radians':-1.5707963267948966,'native_field_yaw_source':'actor+0x744',
                           'gameplay_actor_written':False,'live_enemy_swing_alignment_verified':False},
        'actual_audio_device_executed':False,'actual_listening_verified':False,'actual_fps_measured':False,
        'r18_audio_played':received['audio_observations']['played'],
        'cutscene_effect_path_verified':True,'cutscene_visual_strike_alignment_verified':False,
        'limitations':[
            'Offline tests validate state transitions and contracts, not final GPU animation or live opponent swing alignment.',
            'Private display uses existing field facing; no new enemy target pointer has been decoded.',
            'The original strong clip runs on the native animation clock; wall-clock duration depends on game progression.',
            'R18 hoof audio acceptance failed (played=0); R19 adds diagnosis without claiming a sound fix.',
            'Performance counters measure private CPU wall time, not whole-game CPU/GPU time or FPS.',
            'Whole-roster live pixel/color comparison and118 prior unresolved selections remain batch2.'
        ],
        'next':'One R19 integrated combat/tactic result with automatic cost and native event logs, then batch2.',
        'source_sha256':{rel:sha(ROOT/rel) for rel in sorted(paths)}
    }
    (ROOT/'analysis/poc_r19_change.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','native_guards','native_hook_count','checks','mock_scenarios')}))

if __name__=='__main__': main()
