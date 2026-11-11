#!/usr/bin/env python3
"""Package the current partial checkpoint with original inputs and reproducible tools."""
import argparse
import datetime
import hashlib
import json
import shutil
import subprocess
import zipfile
from pathlib import Path


def digest(path):
    sha=hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda:source.read(1024*1024),b''):sha.update(block)
    return sha.hexdigest()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for flag in ('spec','scale-reference','defeat-reference','duel-reference','exe','output'):
        ap.add_argument('--'+flag,required=True,type=Path)
    args=ap.parse_args()
    root=Path(__file__).resolve().parents[1]
    source_map={
        'MASTER_SPEC_CURRENT.txt':args.spec,
        'inputs/reference_scale.png':args.scale_reference,
        'inputs/reference_defeat.png':args.defeat_reference,
        'inputs/reference_duel.mp4':args.duel_reference,
        'inputs/SAN14CE_KO.exe':args.exe,
    }
    provenance=[]
    for rel,source in source_map.items():
        dest=root/rel
        dest.parent.mkdir(parents=True,exist_ok=True)
        if source.resolve()!=dest.resolve():shutil.copyfile(source,dest)
        sha=digest(dest)
        assert sha==digest(source)
        provenance.append(dict(path=rel,source_filename=source.name,size=dest.stat().st_size,sha256=sha))
    (root/'analysis/input_provenance.json').write_text(json.dumps(provenance,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    for name in ('collect/Collect_FieldLeader3D.ps1','collect/Collect_Resources.ps1','collect/Collect_Linked_Resources.ps1','collect/Collect_Field_State.ps1'):
        path=root/name
        content=path.read_text(encoding='utf-8-sig')
        path.write_text(content,encoding='utf-8-sig',newline='\r\n')
    for name in ('collect/Run_Collector.cmd','Run_Resource_Collector.cmd','Run_Link_Collector.cmd','Run_Field_State.cmd'):
        cmd=root/name
        content=cmd.read_text(encoding='ascii')
        cmd.write_text(content,encoding='ascii',newline='\r\n')
    probe=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_format','-show_streams','-of','json',str(root/'inputs/reference_duel.mp4')],text=True))
    probe['format']['filename']='inputs/reference_duel.mp4'
    (root/'analysis/video_metadata.json').write_text(json.dumps(probe,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    rawlog=root/'analysis/pe_objdump.txt'
    if rawlog.exists():
        trimmed=rawlog.read_text().split('The Function Table')[0]
        (root/'analysis/pe_headers_and_imports.txt').write_text(trimmed,encoding='utf-8')
    motion=json.loads((root/'analysis/motion_timing_analysis.json').read_text())
    lifecycle=json.loads((root/'analysis/lifecycle_analysis.json').read_text())
    verification=json.loads((root/'analysis/motion_analysis_verification.json').read_text())
    if verification['status']!='PASS' or any(c['status']!='PASS' for c in verification['checks']):
        raise ValueError('Motion analysis verification has not passed')
    factory=json.loads((root/'analysis/factory_action_analysis.json').read_text())
    factory_verification=json.loads((root/'analysis/factory_action_verification.json').read_text())
    if factory_verification['status']!='PASS' or any(c['status']!='PASS' for c in factory_verification['checks']):
        raise ValueError('Factory/action analysis verification has not passed')
    render=json.loads((root/'analysis/render_context_analysis.json').read_text())
    render_verification=json.loads((root/'analysis/render_context_verification.json').read_text())
    if render_verification['status']!='PASS' or not render_verification['checks'] or any(c['status']!='PASS' for c in render_verification['checks']):
        raise ValueError('Render-context analysis verification has not passed')
    for expected,path in (
        (render['analysis_tool_sha256'],'tools/render_context_analysis.py'),
        (render['source_exe_sha256'],'inputs/SAN14CE_KO.exe'),
        (render['runtime_text_sha256'],'inputs/SAN14CE_KO.runtime_text.bin'),
        (render_verification['analysis_report_sha256'],'analysis/render_context_analysis.json'),
        (render_verification['verifier_tool_sha256'],'tools/verify_render_context_analysis.py')):
        if digest(root/path)!=expected:
            raise ValueError('Stale R8 proof for '+path)
    runtime=(root/'inputs/SAN14CE_KO.runtime_text.bin').read_bytes()
    for fn in render['code_evidence']:
        for extent in fn['ranges']:
            a,b=int(extent['start_rva'],16)-0x1000,int(extent['end_rva'],16)-0x1000
            if not 0<=a<b<=len(runtime) or hashlib.sha256(runtime[a:b]).hexdigest()!=extent['sha256']:
                raise ValueError('Stale R8 code evidence for '+fn['rva'])
    tables=motion['tables']
    native_build=json.loads((root/'analysis/poc_native_build.json').read_text())
    native_verify=json.loads((root/'analysis/poc_native_verification.json').read_text())
    if native_verify['status']!='PASS' or not native_verify['checks'] or any(c['status']!='PASS' for c in native_verify['checks']):
        raise ValueError('Native PoC verification has not passed')
    if digest(root/'MASTER_SPEC_CURRENT.txt')!='a1e834427346d9b3d151492c728a3b0775aa3627a177c9994939b176bf8f5a8e':
        raise ValueError('Original master specification changed')
    for row in native_build['artifacts']:
        if digest(root/row['path'])!=row['sha256']: raise ValueError('Stale native artifact: '+row['path'])
    native_bound=dict(native_build['source_files'])
    native_bound.update(native_verify['tested_source_hashes'])
    native_bound['analysis/poc_native_build.json']=native_verify['build_report_sha256']
    native_bound['tools/verify_field_poc.py']=native_verify['verifier_sha256']
    for rel,value in native_bound.items():
        if digest(root/rel)!=value: raise ValueError('Stale native source/verification: '+rel)
    user_result=json.loads((root/'analysis/poc_r9_user_result.json').read_text())
    for name,row in user_result['evidence'].items():
        if digest(root/'inputs/first_field_r9_result'/name)!=row['sha256']:
            raise ValueError('Stale R9 user result: '+name)
    r11_result=json.loads((root/'analysis/poc_r11_user_result.json').read_text())
    for rel,row in r11_result['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Stale R11 user result: '+rel)
    r11_history=root/'history/first_field_r11_checkpoint'
    for rel,row in json.loads((r11_history/'preserved_manifest.json').read_text()).items():
        if digest(r11_history/rel)!=row['sha256']: raise ValueError('Changed R11 history: '+rel)
    r12_result=json.loads((root/'analysis/poc_r12_user_result.json').read_text())
    assert r12_result['actual_visible_pixels_verified']
    for rel,row in r12_result['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Stale R12 visual evidence: '+rel)
    r12_history=root/'history/first_field_r12_checkpoint'
    for rel,row in json.loads((r12_history/'preserved_manifest.json').read_text()).items():
        if digest(r12_history/rel)!=row['sha256']: raise ValueError('Changed R12 history: '+rel)
    assert native_build['revision']=='R20' and len(native_verify['checks'])>=155
    r13_result=json.loads((root/'analysis/poc_r13_user_result.json').read_text())
    for rel,row in r13_result['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Stale R13 user evidence: '+rel)
    r13_history=root/'history/first_field_r13_checkpoint'
    for rel,row in json.loads((r13_history/'preserved_manifest.json').read_text())['files'].items():
        if digest(r13_history/rel)!=row['sha256']: raise ValueError('Changed R13 history: '+rel)
    r14_change=json.loads((root/'analysis/poc_r14_change.json').read_text())
    for rel,row in r14_change['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Changed R14 source evidence: '+rel)
    r14_result=json.loads((root/'analysis/poc_r14_user_result.json').read_text())
    assert r14_result['multiple_units_visible_user_confirmed'] and r14_result['replacement_count']==42
    for rel,row in r14_result['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Changed R14 user evidence: '+rel)
    r14_history=root/'history/first_field_r14_checkpoint'
    for rel,row in json.loads((r14_history/'preserved_manifest.json').read_text())['files'].items():
        if digest(r14_history/rel)!=row['sha256']: raise ValueError('Changed R14 history: '+rel)
    r15_change=json.loads((root/'analysis/poc_r15_change.json').read_text())
    for rel,row in r15_change['proof_inputs'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Changed R15 proof input: '+rel)
    r15_history=root/'history/first_field_r15_checkpoint'
    for rel,row in json.loads((r15_history/'preserved_manifest.json').read_text())['files'].items():
        if digest(r15_history/rel)!=row['sha256']: raise ValueError('Changed R15 history: '+rel)
    r16_change=json.loads((root/'analysis/poc_r16_change.json').read_text())
    r16_history=root/'history/first_field_r16_checkpoint'
    for rel,row in json.loads((r16_history/'preserved_manifest.json').read_text())['files'].items():
        if digest(r16_history/rel)!=row['sha256']: raise ValueError('Changed R16 history: '+rel)
    for rel,value in r16_change['source_sha256'].items():
        if digest(r16_history/rel)!=value: raise ValueError('Stale historical R16 evidence: '+rel)
    r17_change=json.loads((root/'analysis/poc_r17_change.json').read_text())
    r17_history=root/'history/first_field_r17_checkpoint'
    for rel,value in json.loads((r17_history/'snapshot_manifest.json').read_text()).items():
        if digest(r17_history/rel)!=value: raise ValueError('Changed R17 history: '+rel)
    for rel,value in r17_change['source_sha256'].items():
        historical=r17_history/rel
        if not historical.is_file(): historical=root/rel
        if digest(historical)!=value: raise ValueError('Stale historical R17 evidence: '+rel)
    r17_result=json.loads((root/'analysis/poc_r17_user_result.json').read_text())
    for rel,row in r17_result['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Stale R17 user result: '+rel)
    r18_change=json.loads((root/'analysis/poc_r18_change.json').read_text())
    r18_history=root/'history/first_field_r18_checkpoint'
    for rel,value in json.loads((r18_history/'snapshot_manifest.json').read_text()).items():
        if digest(r18_history/rel)!=value: raise ValueError('Changed R18 history: '+rel)
    for rel,value in r18_change['source_sha256'].items():
        if digest(r18_history/rel)!=value: raise ValueError('Stale historical R18 evidence: '+rel)
    r18_result=json.loads((root/'analysis/poc_r18_user_result.json').read_text())
    for rel,row in r18_result['evidence'].items():
        if digest(root/rel)!=row['sha256']: raise ValueError('Stale R18 user result: '+rel)
    r19_change=json.loads((root/'analysis/poc_r19_change.json').read_text())
    r19_history=root/'history/first_field_r19_checkpoint'
    for rel,value in json.loads((r19_history/'snapshot_manifest.json').read_text()).items():
        if digest(r19_history/rel)!=value: raise ValueError('Changed R19 history: '+rel)
    for rel,value in r19_change['source_sha256'].items():
        if digest(r19_history/rel)!=value: raise ValueError('Stale R19 historical source/evidence: '+rel)
    r20_change=json.loads((root/'analysis/poc_r20_change.json').read_text())
    for rel,value in r20_change['source_sha256'].items():
        if digest(root/rel)!=value: raise ValueError('Stale R20 source/evidence: '+rel)
    batch=json.loads((root/'analysis/batch1_r18.json').read_text())
    assert batch['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED'
    assert digest(root/'analysis/batch1_r18.json')==native_verify['batch1_report_sha256']
    for rel,value in batch['source_sha256'].items():
        if digest(root/rel)!=value: raise ValueError('Stale original batch1 evidence: '+rel)
    mounted=json.loads((root/'analysis/mounted_motion_r17.json').read_text())
    assert mounted['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED'
    assert digest(root/'analysis/mounted_motion_r17.json')==native_verify['motion_report_sha256']
    for rel,value in mounted['source_sha256'].items():
        if digest(root/rel)!=value: raise ValueError('Stale original mounted-motion evidence: '+rel)
    appearance=json.loads((root/'analysis/appearance_validation_r16/report.json').read_text())
    assert appearance['status']=='PASS_OFFLINE_NOT_PIXEL_VERIFIED'
    assert digest(root/'analysis/appearance_validation_r16/report.json')==native_verify['appearance_report_sha256']
    color_audit_path='analysis/appearance_color_audit_20260920/audit.json'
    color_audit=json.loads((root/color_audit_path).read_text())
    assert color_audit['status']=='R15_COLOR_REQUEST_OMISSION_CONFIRMED_FIX_PENDING'
    for rel,row in color_audit['proof_inputs'].items():
        if digest(r15_history/rel)!=row['sha256']: raise ValueError('Changed historical appearance color audit: '+rel)
    hoof_requirements_path='analysis/hoof_audio_requirements.json'
    hoof_requirements=json.loads((root/hoof_requirements_path).read_text())
    assert hoof_requirements['status']=='R20_NATIVE_SOUND_LAYOUT_FIXED_LIVE_TEST_PENDING'
    assert hoof_requirements['runtime_audio_implemented']
    manifest=dict(schema='fieldleader3d.checkpoint.v20',status='R20_COMBAT_GUARD_AUDIO_OFFLINE_VERIFIED_GAME_TEST_PENDING',
                  generated_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  completed_parts=[],current_part=3,installable=False,dll='native/bin/FieldLeader3D_PoC.dll',
                  test_launcher='native/bin/FieldLeader3D_Test.exe',
                  current_revision='R20',native_windows_build='PASS',native_windows_execution='R20_NOT_RUN; prior user evidence preserved',
                  first_field_test_ready=True,native_verification_checks=len(native_verify['checks']),
                  mock_engine_lifecycle_scenarios=128,original_engine_calls_observed=True,
                  original_engine_observation_basis='R11 received log proves two loaded and animated mounted actors at source coordinates, three Array insertions each, and two orderly releases after 1417 submission frames. R12 separate user screenshot proves simultaneous mounted Zhao/common field pixels.',
                  known_gameplay_hooks=['native normal damage observed unchanged','native tactic effects observed unchanged'],in_game_tests='R20_NOT_RUN; exhaustive all-person pixel comparison pending',
                  prior_user_loader_observed=True,prior_user_loader_result='analysis/poc_r9_user_result.json',
                  user_reported_earlier_trial_connected_without_visible_change=True,
                  runtime_guard_failure='0x44a478; actual bytes absent in R9 result',
                  r10_existing_session_mode='read-only status and first-session log recovery; no reinjection or stop request',
                  r10_engine_dll_unchanged_from_r9=True,
                  r11_integrated_executable=True,r11_runtime_log_analysis='analysis/poc_r11_investigation.json',
                  r11_received_result='analysis/poc_r11_user_result.json',
                  r12_changes='analysis/poc_r12_change.json',r12_uniform_field_multiplier=3.0,
                  r12_user_visual_result='analysis/poc_r12_user_result.json',r12_actual_visible_pixels_verified=True,
                  r13_user_result='analysis/poc_r13_user_result.json',r13_submission_frames_observed=9713,
                  r13_clean_release_observed=False,r13_portrait_occlusion_followup=True,
                  r13_stop_context='User deployed additional units; user requested no separate investigation now',
                  r15_changes='analysis/poc_r15_change.json',r15_status_protocol='0x464c0003',
                  r15_coherent_status_checks=5,r15_true_host_thread_overlap_test=True,
                  r15_lifetime_TID_rule='replaced by whole native callback exclusion and current owner checks',
                  appearance_color_audit='analysis/appearance_validation_r16/report.json',
                  appearance_color_prior_diagnosis=color_audit_path,
                  appearance_color_status='R16_REQUESTS_IMPLEMENTED_OFFLINE_VERIFIED_GAME_PIXELS_PENDING',
                  appearance_color_scope='all 25 native body settings, 61 body appearances, all horse colors, unique AND common officers',
                  horse_color_scope='all unique and common officer appearances; preserve current original duel horse variant',
                  horse_color_original_variants=color_audit['horse_color_requirement']['variant_count'],
                  horse_color_fix_implemented=True,
                  appearance_color_fix_implemented=True,appearance_color_live_comparison_verified=False,
                  appearance_color_completion_required=True,
                  appearance_color_order='User priority: R16 full original appearance field test first; run/hoof audio afterward',
                  hoof_audio_requirements=hoof_requirements_path,
                  hoof_audio_implemented=True,hoof_audio_game_listening_verified=False,
                  hoof_audio_native_movement_replacement_required=True,
                  hoof_audio_mix_basis='currently visible actually moving mounted units; distance attenuation, bounded voices and aggregate gain',
                  hoof_audio_requirements_sha256=digest(root/hoof_requirements_path),
                  r14_user_result='analysis/poc_r14_user_result.json',r14_user_multi_unit_pixels_confirmed=True,
                  r14_replacement_events=42,r14_water_units_excluded=7,r14_submission_frames=5881,
                  r14_stop_context='user tried moving Zhao Yun; direct recorded stop is lifetime TID guard at frame5891',
                  r14_clean_release_observed=False,
                  r14_changes='analysis/poc_r14_change.json',r14_full_part_3_complete=False,
                  r14_candidate_modes=[0,2,3],r14_native_water_mode=1,r14_proxy_capacity=500,
                  r14_create_per_frame=2,r14_max_pending_native_loads=8,r14_status_protocol='0x464c0002',
                  r14_runtime_guards=61,r14_patch_calls=13,
                  r13_changes='analysis/poc_r13_change.json',r13_native_presentation_filter_scope='two land mode 0 units',
                  r13_capture_failure_blocks_result_zip=False,r13_capture_bounds_scenarios=5,
                  approved_reference_scale_match_verified=False,
                  windows_collector_test='USER_CAPTURE_VERIFIED_20260920_134719',
                  windows_resource_collector_test='USER_CAPTURE_VERIFIED_20260920_143326',
                  windows_link_collector_test='USER_CAPTURE_VERIFIED_20260920_150858',
                  windows_state_collector_test='USER_CAPTURE_VERIFIED_20260920_154531',spec_path='MASTER_SPEC_CURRENT.txt',
                  source_exe_sha256=digest(root/'inputs/SAN14CE_KO.exe'),
                  runtime_text_sha256=digest(root/'inputs/SAN14CE_KO.runtime_text.bin'),
                  verified_index_records=38212,verified_captured_ranges=493,
                  fully_decoded_external_records=434,decoded_external_prefixes=56,
                  decoded_database_objects=36190,body_settings_in_original_registry=25,weapon_settings_in_original_registry=6,
                  fully_linked_character_settings=55,original_actions=867,indexed_action_clips=125,
                  verified_field_officer_bindings_per_snapshot=31,field_snapshots=2,
                  field_mode_counts=dict(land_troops=22,water_ship=4,land_siege=5),
                  offline_body_branch_results=6,unresolved_stat_dependent_bodies=25,
                  body_results_are_observed_duel_instances=False,observed_active_duel_units=0,
                  fully_captured_g2a_clips=125,verified_g2a_tracks=14920,
                  original_audio_programs=13,original_audio_sounds=45,
                  horse_sound_event_links=10,decoded_horse_sound_previews=5,
                  sound_listening_classification='NOT_PERFORMED',
                  original_request_timelines=tables['descriptor_count'],
                  original_request_objects=tables['request_inventory_count'],
                  referenced_request_objects=tables['request_inventory_count']-len(tables['unreferenced_request_rvas']),
                  original_request_data_source='ON_DISK_PE_DATA_NOT_LIVE_DATA',
                  decoded_state_dispatch_entries=len(tables['state_dispatch']),
                  verified_direct_annihilation_calls=lifecycle['verified_direct_annihilation_calls'],
                  motion_analysis_checks=len(verification['checks']),
                  factory_action_analysis_checks=len(factory_verification['checks']),
                  factory_action_instruction_guards=len(factory['instruction_guards']),
                  factory_action_verified_call_edges=len(factory['verified_call_edges']),
                  confirmed_character_factory=factory['factories']['motor']['name'],
                  confirmed_action_factory=factory['factories']['action']['name'],
                  native_action_lookup_key=factory['action_lookup']['key'],
                  original_action_maps_verified=factory['database_summary']['settings'],
                  original_action_hash_lookups_verified=factory['database_summary']['action_references'],
                  original_maps_with_nonidentity_sort=factory['database_summary']['settings_with_nonidentity_sort'],
                  runtime_action_hash_bridge_observed=False,
                  render_context_analysis_checks=len(render_verification['checks']),
                  render_context_instruction_guards=len(render['instruction_guards']),
                  render_context_verified_call_edges=len(render['verified_call_edges']),
                  render_context_reviewed_functions=len(render['code_evidence']),
                  field_callback_resource_hash=render['contract']['resource_hash'],
                  field_callback_context_global_rva=render['contract']['resource_context_global_rva'],
                  field_callback_context_handle_offset=render['contract']['handle_offset'],
                  field_callback_concrete_type_verified=True,
                  field_callback_live_vtable_observed=True,
                  field_render_frame_reset_verified=False,
                  motion_action_semantics_verified=False,
                  motion_analysis_file='analysis/motion_timing_analysis.json',
                  lifecycle_analysis_file='analysis/lifecycle_analysis.json',
                  factory_action_analysis_file='analysis/factory_action_analysis.json',
                  render_context_analysis_file='analysis/render_context_analysis.json',
                  field_hook_candidates_file='analysis/field_hook_candidates.json',
                  blockers=['R16_actual_all_person_selection_and_field_color_evidence_pending',
                            'original_duel_vs_field_per_part_appearance_comparison',
                            'field_unit_portrait_troop_count_status_group_occludes_mounted_model',
                            'live_return_redeploy_save_load_and_map_transition_lifecycle',
                            'actual_R20_attack_guard_audio_cutscene_alignment_validation',
                            'actual_siege_water_transition_validation',
                            'R20_corrected_hoof_program_layout_requires_live_audio_confirmation',
                            'remaining_defeat_disappearance_info_placement_appearance_refresh_lifecycle_fps'],
                  next_input='R20 automatic result ZIP after one integrated movement/combat/tactic test; one executable only',
                  next_launcher='native/bin/FieldLeader3D_Test.exe',
                  next_requested_bytes=0,next_requested_entries=0,
                  next_action='Inspect integrated R20 combat/guard/audio and cost samples, then complete batch2 defeat/disappearance, unit-info occlusion and full appearance refresh/color audit together. Batch3 lifecycle/performance. Horse scaling and infantry remain deferred.',
                  r16_changes='analysis/poc_r16_change.json',
                  r16_appearance_validation='analysis/appearance_validation_r16/report.json',
                  r16_native_id_domain_cases=appearance['native_id_domain_cases'],
                  r16_body_settings=25,r16_body_appearances=61,r16_horse_variants=3,
                  r16_native_color_request_rva='0x1c8000',r16_guard_count=65,
                  r16_all_person_audit_implemented=True,r16_all_person_live_scan_verified=False,
                  r16_all_person_pixels_verified=False,r16_audio_implemented=False,
                  r17_changes='analysis/poc_r17_change.json',r17_motion_evidence='analysis/mounted_motion_r17.json',
                  r17_runtime_guards=80,r17_patch_calls=15,
                  r17_mounted_motion_implemented=True,r17_game_motion_verified=True,
                  r17_game_motion_verification_basis='USER_REPORT; preserved result ZIP; not exhaustive GPU comparison',
                  r17_user_result='analysis/poc_r17_user_result.json',
                  r17_additional_sound_events_gated=True,r17_original_field_audio_retained=True,
                  r17_hoof_replacement_implemented=False,r17_horse_group_scale_implemented=False,
                  r18_changes='analysis/poc_r18_change.json',r18_batch1_evidence='analysis/batch1_r18.json',
                  r18_runtime_guards=111,r18_patch_calls=21,
                  r18_attack_implemented=True,r18_tactic_strong_implemented=True,
                  r18_hit_only_damaging_tactics=True,r18_normal_attack_hit_reaction=False,
                  r18_hoof_replacement_implemented=True,r18_game_motion_verified=False,
                  r18_cutscene_visual_sync_verified=False,r18_actual_audio_device_executed=False,
                  r18_body_action_bindings=75,r18_native_byte_checks=9,r18_frustum_cases=6,
                  r18_user_result='analysis/poc_r18_user_result.json',
                  r18_normal_attack_gallop_defect_confirmed=True,r18_hoof_played_observed=0,
                  r19_changes='analysis/poc_r19_change.json',
                  r19_normal_attack_standing_horse=True,r19_periodic_engaged_attack=True,
                  r19_tactic_forward_strong_and_brief_gallop=True,r19_side_facing_live_verified=False,
                  r19_original_visible_update_rate_preserved=True,r19_game_fps_measured=False,
                  r19_runtime_private_cost_instrumentation=True,r19_hoof_event_probe=True,
                  user_batch_plan='BATCH_PLAN_CURRENT.md',
                  r20_changes='analysis/poc_r20_change.json',
                  r20_evidence='analysis/combat_r20_evidence.json',
                  r20_patch_calls=24,r20_runtime_guards=native_build['runtime_guards'],
                  r20_body_action_bindings=100,r20_normal_motion=13,r20_guard_motion=35,
                  r20_cosmetic_rng_independent=True,r20_game_damage_unchanged=True,
                  r20_paused_contact_trials=True,r20_tactic_hit_excludes_clash=True,
                  r20_audio_device_test=False,r20_gpu_frame_rate_test=False,
                  previous_checkpoint='SAN14CE_FieldLeader3D_BATCH1_R19_CHECKPOINT.zip',
                  previous_checkpoint_documents='history/first_field_r19_checkpoint')
    (root/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    excluded={'analysis/pe_objdump.txt','analysis/artifact_checksums.json'}
    files=[p for p in sorted(root.rglob('*')) if p.is_file()
           and '__pycache__' not in p.parts and '.pyc'!=p.suffix
           and 'video_frames' not in p.parts and p.relative_to(root).as_posix() not in excluded
           and p.resolve()!=args.output.resolve()]
    checksums=[dict(path=p.relative_to(root).as_posix(),size=p.stat().st_size,sha256=digest(p)) for p in files]
    checksum_file=root/'analysis/artifact_checksums.json'
    checksum_file.write_text(json.dumps(checksums,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    files.append(checksum_file)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(args.output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for path in files:archive.write(path,'FieldLeader3D_PART01/'+path.relative_to(root).as_posix())
    with zipfile.ZipFile(args.output) as archive:
        assert archive.testzip() is None
        assert all(not n.startswith('/') and '..' not in Path(n).parts for n in archive.namelist())
        for row in checksums:
            assert hashlib.sha256(archive.read('FieldLeader3D_PART01/'+row['path'])).hexdigest()==row['sha256']
    print(json.dumps(dict(file=str(args.output.resolve()),files=len(files),bytes=args.output.stat().st_size,
                          sha256=digest(args.output),archive_crc_and_hashes='PASS')))


if __name__=='__main__':main()
