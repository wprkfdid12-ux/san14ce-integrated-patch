#!/usr/bin/env python3
"""Record only already-verified R18 sources and evidence; never simulate a game result."""
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    build=json.loads((ROOT/'analysis/poc_native_build.json').read_text())
    verify=json.loads((ROOT/'analysis/poc_native_verification.json').read_text())
    evidence=json.loads((ROOT/'analysis/batch1_r18.json').read_text())
    assert build['revision']=='R18' and verify['status']=='PASS'
    assert len(verify['checks'])==129 and verify['mock_engine_scenarios']==103
    assert verify['build_report_sha256']==sha(ROOT/'analysis/poc_native_build.json')
    assert verify['batch1_report_sha256']==sha(ROOT/'analysis/batch1_r18.json')
    for mapping in (build['source_files'],verify['tested_source_hashes'],evidence['source_sha256']):
        for rel,value in mapping.items():assert sha(ROOT/rel)==value,rel
    for row in build['artifacts']:assert sha(ROOT/row['path'])==row['sha256']
    assert sha(ROOT/'MASTER_SPEC_CURRENT.txt')=='a1e834427346d9b3d151492c728a3b0775aa3627a177c9994939b176bf8f5a8e'
    paths=set(build['source_files'])|set(verify['tested_source_hashes'])
    paths.update(row['path'] for row in build['artifacts'])
    paths.update(('CURRENT_STATE.txt','PART_03_HANDOFF.txt','PART_03_RESULT.txt','BATCH_PLAN_CURRENT.md',
        'MASTER_SPEC_CURRENT.txt','FieldLeader3D_TODO_HorseScale_20260923.md',
        'native/READ_FIRST_KO.txt','native/ENGINE_CONTRACT.md',
        'analysis/poc_native_build.json','analysis/poc_native_contract.json','analysis/poc_native_verification.json',
        'analysis/batch1_r18.json','analysis/mounted_motion_r17.json','analysis/appearance_validation_r16/report.json',
        'analysis/poc_r17_user_result.json','analysis/hoof_audio_requirements.json','analysis/HoofAudio_NEXT.md',
        'tools/build_field_poc.py','tools/verify_field_poc.py','tools/verify_batch1.py',
        'tools/package_first_field_test.py','tools/package_checkpoint.py','tools/record_r18_change.py'))
    report={
        'schema':'fieldleader3d.change.r18','date':'2026-09-23',
        'status':'BATCH1_BUILT_OFFLINE_VERIFIED_AWAITING_INTEGRATED_GAME_TEST',
        'user_amendment':'Hit reaction only when receiving damaging tactics; NO normal-attack hit reaction.',
        'changes':[
            'Native field attack state selects original duel body9/horse1 normal attack',
            'Actual native tactic effect root selects body19/horse1 strong; shared cutscene phase4/no-cutscene timer0 event',
            'Only positive native tactic damage bucket0/style34 selects body39/horse1 hit; normal attack, recovery, buff and zero damage excluded',
            'Native clip-frame completion, strong priority, bounded event age and repeated-hit suppression',
            'Per-unit original land movement queue omission followed by original unmatched-voice cleanup acknowledgement',
            'Only original finite gait events admitted for visible actual movement, with distance/count normalization and bounded sources/voices/gain',
            'One paired DLL embedded in one GUI executable, one automatic result ZIP'
        ],
        'preserved':['R17 original move/idle and source tracking','R16 native appearance/body/horse colors and weapons',
                     'Original gameplay/save/duel/resources','Ownership and coherent status guards',
                     '500 proxies, 2 creates/frame, 8 pending, original water boats/audio',
                     'Deferred group horse scaling and infantry expansion'],
        'native_guards':build['runtime_guards'],'native_hook_count':21,
        'checks':len(verify['checks']),'mock_scenarios':103,'appearance_integration_combinations':1464,
        'body_attack_strong_hit_bindings':75,'original_byte_checks':9,'native_view_cases':6,
        'hoof_limits':{'candidate_sources':32,'audible_sources':8,'voice_reservations':16,'aggregate_linear_gain':1.5,
                       'duration_extra_ms':150,'programs':['0xe3c22636','0xc5540415','0xa6e5e1f4','0x8877bfd3']},
        'actual_windows_game_execution':False,'actual_gpu_motion_verified':False,
        'actual_audio_device_executed':False,'actual_listening_verified':False,'actual_fps_measured':False,
        'cutscene_effect_path_verified':True,'cutscene_visual_strike_alignment_verified':False,
        'limitations':evidence['limitations']+[
            'Screen-edge source selection is discrete; already emitted finite samples can finish their short tails.',
            'Native tactic damage reaction uses the direct damage bucket. Each live tactic still needs visual confirmation.',
            'Whole-roster per-person pixel/color verification remains unfinished; 118 R17 unresolved selections belong to batch2.'
        ],
        'next':'Receive one integrated R18 result, then batch2 as recorded in BATCH_PLAN_CURRENT.md.',
        'source_sha256':{rel:sha(ROOT/rel) for rel in sorted(paths)}
    }
    (ROOT/'analysis/poc_r18_change.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','native_guards','native_hook_count','checks','mock_scenarios')}))

if __name__=='__main__':main()
