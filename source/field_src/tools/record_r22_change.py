#!/usr/bin/env python3
"""Bind the R22 executable, unchanged native inputs and completed offline checks."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    b = json.loads((ROOT / 'analysis/poc_native_build.json').read_text())
    v = json.loads((ROOT / 'analysis/poc_native_verification.json').read_text())
    e = json.loads((ROOT / 'analysis/batch2_r22_evidence.json').read_text())
    assert b['revision'] == 'R22' and v['status'] == 'PASS'
    assert len(v['checks']) == 187 and v['mock_engine_scenarios'] == 158
    assert all(row['status'] == 'PASS' for row in v['checks'])
    assert v['build_report_sha256'] == sha(ROOT / 'analysis/poc_native_build.json')
    assert v['batch2_r22_report_sha256'] == sha(ROOT / 'analysis/batch2_r22_evidence.json')
    for mapping in (b['source_files'], v['tested_source_hashes'], e['source_sha256']):
        for name, digest in mapping.items():
            assert sha(ROOT / name) == digest, name
    for row in b['artifacts']:
        assert sha(ROOT / row['path']) == row['sha256']
    hist = ROOT / 'history/first_field_r21_checkpoint'
    old = json.loads((ROOT / 'analysis/poc_r21_change.json').read_text())
    historical_checked = 0
    for name, digest in old['source_sha256'].items():
        if (hist / name).is_file():
            assert sha(hist / name) == digest, 'R21 changed: ' + name
            historical_checked += 1
    assert historical_checked > 20
    paths = set(b['source_files']) | set(v['tested_source_hashes']) | {row['path'] for row in b['artifacts']}
    paths.update((
        'CURRENT_STATE.txt', 'PART_03_HANDOFF.txt', 'PART_03_RESULT.txt', 'BATCH_PLAN_CURRENT.md',
        'README_KO.txt', 'MASTER_SPEC_CURRENT.txt', 'native/READ_FIRST_KO.txt', 'native/ENGINE_CONTRACT.md',
        'analysis/poc_native_build.json', 'analysis/poc_native_contract.json', 'analysis/poc_native_verification.json',
        'analysis/batch2_r22_evidence.json', 'analysis/r22_received_roster_review.json',
        'analysis/r21_received_officer_appearance_audit.csv', 'inputs/r21_received_result.zip',
        'analysis/appearance_validation_r16/report.json', 'analysis/combat_r21_evidence.json',
        'tools/build_field_poc.py', 'tools/verify_field_poc.py', 'tools/verify_batch2_r22.py',
        'tools/package_first_field_test.py', 'tools/package_checkpoint_r22.py', 'tools/record_r22_change.py'))
    report = dict(
        schema='fieldleader3d.change.r22', date='2026-09-24', revision='R22',
        status='INTEGRATED_BATCH2_OFFLINE_VERIFIED_GAME_TEST_PENDING',
        changes=[
            'Whole native portrait/count/icons among the same horse legs, original horizontal position and native layout',
            'Native normal field arc creation blocked and previous arcs retired; strong-tactic VFX retained',
            'Hidden troop dust cleared; private original horse dust only with actual movement',
            'Native capture outcome controls original fall; uncaptured defeated unit disappears mounted',
            'Independent terminal visual lifetime; accepted defeat never briefly draws original soldier geometry',
            'Stable two-bank staged refresh of original body/weapon/parts/horse colors with old model retained until ready',
            'One redundant native weapon attachment per unit/frame removed, full animation and render cadence retained',
            'Bounded buffered logging removes synchronous field-thread disk flushes',
            'Existing combat priorities, original sounds and40% native troop ambience retained',
            'One GUI executable, Korean instructions, automatic result ZIP'],
        native_guards=b['runtime_guards'], e8_hooks=29, vtable_hooks=3, native_hook_count=32,
        checks=len(v['checks']), mock_scenarios=v['mock_engine_scenarios'],
        native_r22_evidence_checks=len(e['checks']), original_fall_bindings=e['fall_binding_count'],
        original_damage_ai_save_rng_capture_unchanged=True,
        preserved_visual_quality=True, preserved_unit_capacity=500, preserved_full_actor_update_rate=True,
        historical_r21_hashes_checked=historical_checked,
        actual_windows_game_execution=False, actual_gpu_motion_verified=False,
        actual_audio_device_executed=False, actual_fps_measured=False,
        roster_original_resolved=1337, roster_original_unresolved=119, all_person_pixels_verified=False,
        limitations=[
            'Real-game info placement at different zooms/dense units, effect disappearance, capture timing and FPS remain unverified',
            '119 received roster entries have original selector-1; captured data does not establish the cause',
            'No claim of all-officer pixel validation or measured FPS improvement',
            'Existing frame-perfect cutscene alignment and audio balance still require live observation'],
        deferred_with_user_authorization=['Batch3 lifecycle/performance/compatibility', 'Group horse size', 'Formation infantry'],
        next='Inspect one integrated R22 game result; complete any observed batch2 corrections, then batch3',
        source_sha256={name: sha(ROOT / name) for name in sorted(paths)})
    (ROOT / 'analysis/poc_r22_change.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('status', 'native_guards', 'native_hook_count', 'checks', 'mock_scenarios')}))

if __name__ == '__main__':
    main()
