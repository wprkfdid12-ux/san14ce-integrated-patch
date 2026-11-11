#!/usr/bin/env python3
"""Verify cross-source timing joins and original-clock edge cases offline."""
import collections
import copy
import hashlib
import json
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from motion_analysis import TimingTables, due_requests, select_original, simulate, resource_joins, write
from lifecycle_analysis import build as build_lifecycle


def main():
    root = Path(__file__).resolve().parents[1]
    tool = RuntimeDisassembler(root/'inputs')
    saved = json.loads((root/'analysis/motion_timing_analysis.json').read_text())
    tables = TimingTables(tool.pe).parse()
    checks = []
    def check(name, result):
        if not result:
            raise AssertionError(name)
        checks.append(dict(name=name,status='PASS'))
    def rejects(name, fn):
        try:
            fn()
        except (ValueError,struct.error):
            check(name,True)
        else:
            check(name,False)
    def alter(rva,fmt,*values):
        disk = bytearray(tool.pe.disk_data)
        struct.pack_into(fmt,disk,tool.pe.rva_to_offset(rva),*values)
        return TimingTables(tool.pe,disk).parse()
    check('Original PE timing tables reproduce the saved interpretation',tables==saved['tables'])
    check('39 descriptors have ID 39 but no ID 25; IDs are not array indices',
          len(tables['timelines'])==39 and tables['timelines'][-1]['timeline_id']==39 and tables['missing_ids_in_range']==[25])
    check('All 53 typed request objects are accounted for, with one unreferenced effect',
          len(tables['requests'])==53 and tables['unreferenced_request_rvas']==['0x01a3ec40']
          and tables['requests']['0x01a3ec40']['kind']=='UnitEffect')
    check('Body, horse, voice, frame and hide have distinct execute methods',
          len({r['execute_rva'] for r in tables['requests'].values()})==6)
    one=next(t for t in tables['timelines'] if t['timeline_id']==1)
    two=next(t for t in tables['timelines'] if t['timeline_id']==2)
    check('Timelines 1 and 2 share pointers rather than copied guessed actions',
          one['requests_in_original_order']==two['requests_in_original_order'])
    check('Original array order is deliberately not chronological',
          [tables['requests'][v]['start_frame'] for v in one['requests_in_original_order']]==[0,240,210,240,0])
    for fps in (30,60,120):
        fired=simulate(tables,1,fps)
        check(f'{fps} FPS run fires each opening, horse, hide and voice request once',
              collections.Counter(x['request_rva'] for x in fired)==collections.Counter(one['requests_in_original_order'])
              and all(0 <= x['clock']-tables['requests'][x['request_rva']]['start_frame'] < 60/fps for x in fired))
    check('At frame 240 the body reset precedes hiding all model flags',
          [tables['requests'][r]['kind'] for r in due_requests(tables,1,240,60)[0]]==['PersonMotion','Hide'])
    check('Upper clock boundary is exclusive and no event fires early',
          due_requests(tables,1,239.5,120)[0]==[] and due_requests(tables,1,240.5,120)[0]==[])
    check('Missing ID resets clock and retains selection in the original routine',
          select_original(tables,25,36)==dict(clock=0.0,selected_id=36,matched=False))
    rejects('Offline adapter refuses missing ID instead of animating an old selection',lambda:due_requests(tables,25,0,60))
    rejects('Offline clock refuses invalid FPS',lambda:due_requests(tables,1,0,0))
    rejects('Unmapped descriptor pointer is rejected',lambda:alter(0x1a3f2e8,'Q',0))
    rejects('Pointer-array extent crossing into objects is rejected',lambda:alter(0x1a3f2f0,'I',0xffffffff))
    rejects('Pointer into the middle of a valid object is rejected',
            lambda:alter(0x1a3e938,'Q',tool.pe.image_base+0x1a3ec68))
    rejects('Unknown request vtable is rejected',lambda:alter(0x1a3ec60,'Q',tool.pe.image_base+0x13b5b58))
    rejects('Duplicated timeline IDs cannot silently overwrite a row',lambda:alter(0x1a3f2f8,'I',0))
    rejects('Repeating request with a zero interval is rejected',lambda:alter(0x1a3ec70,'II',1,0))
    rejects('Non-finite frame parameter is rejected',lambda:alter(0x1a3ecdc,'f',float('nan')))
    modified=copy.deepcopy(tables)
    modified['requests']['0x01a3ece0']['repeat']=1
    rejects('Unsupported repeat semantics cannot be claimed by the clock model',lambda:due_requests(modified,1,0,60))
    joins=resource_joins(root,tables)
    check('Resource joins reproduce from original ordered DB references and G2A headers',joins==saved['sample_resource_joins'])
    zhao=next(s for s in joins if s['setting_hash_hex']=='0x7c6a126d')
    common=next(s for s in joins if s['setting_hash_hex']=='0x20d9cc18')
    horse=next(s for s in joins if s['role']=='horse')
    check('Body requests absent from Zhao remain explicitly unresolved',zhao['missing_requested_values']==[6,7,57,59])
    check('Named common body and Zhao retain the same original requested clip references',
          {a['action_id']:a['clips'] for a in zhao['matching_actions']}=={a['action_id']:a['clips'] for a in common['matching_actions']})
    check('Horse request values 0, 1 and 2 join to different captured clips',
          [(a['action_id'],a['clips'][0]['frames']) for a in horse['matching_actions']]==[(0,121),(1,152),(2,33)])
    factory=json.loads((root/'analysis/factory_action_analysis.json').read_text())
    check('Native ActionID proof is separate from coincident Flag values in source DB joins',
          all(a['flag']==[a['action_id']] for s in joins for a in s['matching_actions'])
          and all(s['join_status']=='CODE_CONFIRMED_ACTION_ID_KEY_SOURCE_DB_JOIN' for s in joins)
          and factory['action_lookup']['key']=='ActionID' and factory['action_lookup']['flag_is_lookup_key'] is False)
    # Review-sensitive fallthrough: state 3 can never hold timeline 3 on this path.
    branch=TimingTables(tool.pe).read(0x378cd6,23,True)
    check('State 3 immediately falls through to timeline 4, with no intervening branch',
          branch==bytes.fromhex('ba03000000e8600a0000ba04000000488bcbe8530a0000'))
    lifecycle=build_lifecycle(root)
    check('Annihilation edges and factory evidence reproduce against original bytes',
          lifecycle==json.loads((root/'analysis/lifecycle_analysis.json').read_text())
          and lifecycle['verified_direct_annihilation_calls']==22)
    check('Pending-list check is not mislabeled as health or logical deallocation',
          lifecycle['annihilation']['pending_check_is_health_test'] is False
          and lifecycle['annihilation']['accepted_list_is_deallocation'] is False)
    check('Morale naming and troop-zero cause remain separate unverified claims',
          lifecycle['annihilation']['zero_value_path']['exact_ui_field_name_verified'] is False
          and lifecycle['annihilation']['zero_value_path']['troop_zero_path_confirmed'] is False)
    check('Original executable still matches the supplied capture manifest',
          hashlib.sha256((root/'inputs/SAN14CE_KO.exe').read_bytes()).hexdigest()==tool.pe.runtime_info['exe_sha256'])
    report=dict(schema='fieldleader3d.motion_verification.v1',status='PASS',checks=checks,
                native_calls=False,hook_installation=False,windows_build='NOT_RUN',in_game_test='NOT_RUN')
    write(root/'analysis/motion_analysis_verification.json',report)
    print(json.dumps(dict(status='PASS',checks=len(checks))))


if __name__ == '__main__':
    main()
