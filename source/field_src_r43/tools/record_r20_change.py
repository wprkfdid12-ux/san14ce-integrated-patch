#!/usr/bin/env python3
"""Bind R20 source, compiled executable and offline checks without claiming a game test."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    b=json.loads((ROOT/'analysis/poc_native_build.json').read_text());v=json.loads((ROOT/'analysis/poc_native_verification.json').read_text())
    e=json.loads((ROOT/'analysis/combat_r20_evidence.json').read_text())
    assert b['revision']=='R20' and v['status']=='PASS' and len(v['checks'])==155 and v['mock_engine_scenarios']==128
    assert v['build_report_sha256']==sha(ROOT/'analysis/poc_native_build.json')
    assert v['combat_r20_report_sha256']==sha(ROOT/'analysis/combat_r20_evidence.json')
    for mapping in (b['source_files'],v['tested_source_hashes'],e['source_sha256']):
        for name,digest in mapping.items():assert sha(ROOT/name)==digest,name
    for row in b['artifacts']:assert sha(ROOT/row['path'])==row['sha256']
    hist=ROOT/'history/first_field_r19_checkpoint'
    for name,digest in json.loads((hist/'snapshot_manifest.json').read_text()).items():assert sha(hist/name)==digest,name
    paths=set(b['source_files'])|set(v['tested_source_hashes'])|{r['path'] for r in b['artifacts']}
    paths.update(('CURRENT_STATE.txt','PART_03_HANDOFF.txt','PART_03_RESULT.txt','BATCH_PLAN_CURRENT.md','README_KO.txt',
        'MASTER_SPEC_CURRENT.txt','FieldLeader3D_TODO_HorseScale_20260923.md','native/READ_FIRST_KO.txt','native/ENGINE_CONTRACT.md',
        'analysis/poc_native_build.json','analysis/poc_native_contract.json','analysis/poc_native_verification.json',
        'analysis/combat_r20_evidence.json','analysis/r20_audio_sample_matches.json',
        'inputs/r20_reference_duel.mp4','inputs/r19_received_result.zip',
        'analysis/hoof_audio_requirements.json','analysis/HoofAudio_NEXT.md',
        'history/first_field_r19_checkpoint/snapshot_manifest.json',
        'tools/build_field_poc.py','tools/verify_field_poc.py','tools/verify_combat_r20.py',
        'tools/package_first_field_test.py','tools/package_checkpoint.py','tools/record_r20_change.py'))
    report=dict(schema='fieldleader3d.change.r20',date='2026-09-23',status='INTEGRATED_TEST_BUILD_OFFLINE_VERIFIED_GAME_TEST_PENDING',
        changes=[
            'Short original sweep13, standing horse0 and side yaw; suppress normal private motion VFX only',
            'Original spin-attack19, brief horse1; native per-body contact event drives serial-bound victim reaction',
            'Independent cosmetic guard35 based on effective A+D with bounded defense and native tactic-factor corrections',
            'Native strategy pause repeats visual-contact guard trials with cached last real opponent/scores; no frozen outcome or damage call',
            'Normal/guard share quiet original collision program273f6ffd; tactic guard is louder',
            'Unblocked tactics emit no additional collision; only tactic victims enter hit39 with louder original hit6fd54d22',
            'Correct native sound ProgramIdx+0/Pitch+4, retaining gain+8 and ownership/cleanup rules',
            'Bounded visible combat sources16, normal voices12, all combat voices24, distance/density and gain ceilings',
            'One GUI EXE embeds the exact paired DLL and writes one automatic result ZIP'],
        native_guards=b['runtime_guards'],native_hook_count=24,checks=len(v['checks']),mock_scenarios=128,
        native_byte_checks=len(e['checks']),body_action_bindings=100,appearance_integration_combinations=1464,
        original_damage_ai_save_rng_unchanged=True,guard_probability_is_new_cosmetic_rule=True,
        preserved_visual_quality=True,preserved_unit_capacity=500,preserved_full_actor_update_rate=True,
        audio_evidence=dict(bank='11de6328',clash_program='273f6ffd',matched_sample='5ebb7e22',
            matched_duel_time_seconds=5.428390022675737,normalized_sample_correlation=0.32580969314875063,
            role='Inferred from original waveform match and inspected guard-contact scene; shared collision fallback, no distinct guard-only audio confirmed',
            hit_program='6fd54d22',hit_role='Original body39 SoundDopeSheet'),
        actual_windows_game_execution=False,actual_gpu_motion_verified=False,actual_audio_device_executed=False,
        actual_listening_verified=False,actual_fps_measured=False,cutscene_visual_strike_alignment_verified=False,
        limitations=[
            'Native rendering/audio devices and final sound balance require the supplied game test',
            'Strong starts at native effect stage; victim reactions follow original strong contact, but cutscene frame-perfect sync is unverified',
            'Paused guards require at least one real observed contact to cache opponent scores; no fabricated nearby opponent',
            'One current pending cosmetic tactic reaction per victim; newer tactics supersede stale display outcomes while each actual event is independently rolled',
            'Strong/hit/guard priorities and replay cooldowns may suppress an overlapping cosmetic pose; native damage remains unchanged',
            'All-person live appearance comparison and prior unresolved selector observations remain batch2'],
        next='One integrated R20 test, then authorized batch2 defeat/fade, unit-info occlusion and appearance refresh audit; batch3 stability afterward',
        source_sha256={name:sha(ROOT/name) for name in sorted(paths)})
    (ROOT/'analysis/poc_r20_change.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','native_guards','native_hook_count','checks','mock_scenarios')}))
if __name__=='__main__':main()
