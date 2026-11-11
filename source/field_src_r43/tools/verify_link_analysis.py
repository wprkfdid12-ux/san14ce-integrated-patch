#!/usr/bin/env python3
"""Check cross-file joins, full/prefix boundaries and non-repeating next requests."""
import copy
import hashlib
import json
from pathlib import Path
from link_analysis import Graph, signature
from resource_graph import prop
from rdb_payload import model_chunks


def main():
    root=Path(__file__).resolve().parents[1]
    g=Graph(root); result=g.build(); tests=[]
    def check(name,condition):
        if not condition:raise AssertionError(name)
        tests.append(dict(name=name,status='PASS'))
    saved=json.loads((root/'analysis/link_graph.json').read_text())
    check('Graph reproducible from original captured bytes',saved==result)
    check('Every registry setting resolves through wrapper and descriptor',
          all(s['setting_sources'] and s['wrapper_sources'] and s['descriptor_sources'] and s['model']['indexes'] for s in result['settings']))
    check('Mirrored action objects are byte-for-byte identical in type/property content',
          result['action_summary']['identical_mirrored_actions']==result['action_summary']['unique_actions'])
    check('Every referenced animation has an indexed original payload',not result['action_summary']['unindexed_clips'])
    check('Every captured character texture object has an indexed original texture',not result['texture_summary']['unindexed_resources'])
    check('Three unresolved texture objects remain explicit',len(result['texture_summary']['unresolved_objects'])==3)
    check('All nonzero color material references are original material objects',
          all(e['sources'][0]['type_hex']=='0xa8f14404' for e in result['materials'].values()))
    check('All original rig bytecodes exist',all(v['bytecode_bytes']>0 for v in result['rigs'].values()))
    cmp=result['comparison']
    check('Shared body action IDs resolve to identical clip references',
          cmp['shared_action_ids']==cmp['shared_action_ids_with_identical_clip_refs'])
    z=next(s for s in result['settings'] if s['setting_hash_hex']==cmp['zhao_setting'])
    m=next(s for s in result['settings'] if s['setting_hash_hex']==cmp['generic_setting'])
    check('Shared clips do not collapse character-specific events',
          next(a for a in z['actions'] if a['action_id']==27)['event_object_hashes']!=next(a for a in m['actions'] if a['action_id']==27)['event_object_hashes'])
    check('Generic base materials match color-table base row',m['wrapper_base_material_objects']==m['color_material_matrix'][0])
    check('Zhao and generic remain partial models with complete skeleton chunks',
          all(not s['g1m']['entire_model_present'] and any(c['tag']=='SM1G' and c['complete'] for c in s['g1m']['chunks']) for s in [z,m]))
    next_request=json.loads((root/'collect/state_request.json').read_text())
    prior=[json.loads((root/'collect/resource_request.json').read_text())['ranges'],
           json.loads((root/'collect/link_request.json').read_text())['entries']]
    old_ids={r.get('resource_id_hex') for rows in prior for r in rows}
    check('New assets were not present in either previous capture request',
          all(e.get('resource_id_hex') not in old_ids for e in next_request['entries'] if e['kind']=='rdb_record'))
    old_paths={r['relative_path'] for rows in prior for r in rows}
    check('New asset ranges use different source files from prior payload requests',
          not ({e['relative_path'] for e in next_request['entries']} & old_paths))
    check('New file collection remains bounded',sum(e['length'] for e in next_request['entries'])==next_request['requested_bytes']<=8*1024*1024)
    # Ensure conflicting mirrored objects cannot be silently joined by hash.
    ident=int(next(iter(result['actions'])),16);rows=g.objects[ident]
    altered=copy.deepcopy(rows[1][1]);altered['type']='ffffffff'
    g.objects[ident]=[rows[0],(rows[1][0],altered)]
    rejected=False
    try:g.object(ident)
    except ValueError:rejected=True
    check('Conflicting same-hash object rejected',rejected)
    check('Original EXE is unchanged',hashlib.sha256((root/'inputs/SAN14CE_KO.exe').read_bytes()).hexdigest()==next_request['expected_exe_sha256'])
    state_report=root/'analysis/state_capture_verification.json'
    state_verified=state_report.exists() and json.loads(state_report.read_text()).get('status')=='PASS'
    report=dict(schema='fieldleader3d.link_analysis_verification.v1',status='PASS',checks=tests,
                windows_state_collector='USER_CAPTURE_VERIFIED_20260920_154531' if state_verified else 'SOURCE_REVIEW_ONLY_NOT_EXECUTED',mod_in_game_test='NOT_RUN')
    (root/'analysis/link_analysis_verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',checks=len(tests))))


if __name__=='__main__':main()
