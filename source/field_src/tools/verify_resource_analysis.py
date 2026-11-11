#!/usr/bin/env python3
"""Evidence checks on actual captured resources; these are not in-game MOD tests."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from kids_database import parse_database
from rdb_payload import decode_record


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('project',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();root=args.project;checks=[]
    def check(name,ok):
        if not ok:
            raise AssertionError(name)
        checks.append(dict(name=name,status='PASS'))
    request=json.loads((root/'inputs/resource_capture/resource_request.json').read_text())
    capture=json.loads((root/'inputs/resource_capture/resource_capture.json').read_text(encoding='utf-8-sig'))
    captured={r['output_name']:r for r in capture['ranges']}
    check('all_333_captured_hashes_and_lengths',len(captured)==333 and all(
        len((root/'inputs/resource_capture/payloads'/name).read_bytes())==row['captured_length'] and
        hashlib.sha256((root/'inputs/resource_capture/payloads'/name).read_bytes()).hexdigest()==row['sha256']
        for name,row in captured.items()))
    decoded=[]
    for row in request['ranges']:
        data,info=decode_record((root/'inputs/resource_capture/payloads'/row['output_name']).read_bytes(),row)
        if data!=(root/'analysis/resource_decoded'/row['output_name']).read_bytes():
            raise AssertionError('Decoded artifact differs')
        decoded.append(info)
    check('all_records_redecode_identically',len(decoded)==333)
    check('277_full_and_56_partial_records_remain_distinct',sum(r['decoded_entire_payload'] for r in decoded)==277 and sum(not r['complete_external_record'] for r in decoded)==56)
    dbs=[parse_database(p.read_bytes()) for p in (root/'analysis/resource_decoded').glob('db_*.bin')]
    check('15_databases_25371_objects_exact_property_consumption',len(dbs)==15 and sum(d['count'] for d in dbs)==25371)
    g=json.loads((root/'analysis/resource_graph.json').read_text())
    check('all_25_body_and_6_weapon_settings_in_original_registry',g['matched_exe_body_settings']==25 and g['matched_exe_weapon_settings']==6)
    check('all_55_model_descriptors_point_to_character_mesh_records',len(g['models'])==55 and all(
        m['links']['KTGLModelDataResourceHash'][0]['rdb']['type_hash_hex']=='0x563bdef1' for m in g['models']))
    check('original_11_field_character_pool_names_match_registry',all(r['registry_key_match'] for r in g['field_pool_table'] if r['kind']==1) and sum(r['kind']==1 for r in g['field_pool_table'])==11)
    check('missing_character_database_has_exact_indexed_range',next(r for r in g['character_database_dependencies'] if r['resource_id_hex']=='0x2082ad97')['external']['offset']==0 and next(r for r in g['character_database_dependencies'] if r['resource_id_hex']=='0x2082ad97')['external']['length']==14761)
    next_request=json.loads((root/'collect/link_request.json').read_text())
    check('next_request_31_complete_records_and_2_indexes',len(next_request['entries'])==33 and sum(r['kind']=='rdb_record' for r in next_request['entries'])==31 and next_request['requested_bytes']==1104882)
    check('next_request_does_not_repeat_prior_payloads',not ({r['output_name'] for r in next_request['entries']} & set(captured)))
    raw=(root/'analysis/resource_decoded/db_b290631c.bin').read_bytes()
    rejected=False
    try:parse_database(raw[:-1])
    except ValueError:rejected=True
    check('truncated_database_rejected',rejected)
    row=next(r for r in request['ranges'] if r['output_name']=='character_0137aae8.bin')
    raw=bytearray((root/'inputs/resource_capture/payloads'/row['output_name']).read_bytes())
    raw[36]^=1
    rejected=False
    try:decode_record(bytes(raw),row)
    except ValueError:rejected=True
    check('payload_index_identity_mismatch_rejected',rejected)
    check('original_exe_unchanged',hashlib.sha256((root/'inputs/SAN14CE_KO.exe').read_bytes()).hexdigest()=='07a047826cfc316dc9e3db6bfddc72b85e8d7de99743cc73a2d7199a1ad3aa40')
    link_status='NOT_EXECUTED'
    link_proof=root/'analysis/link_capture_verification.json'
    if link_proof.exists() and json.loads(link_proof.read_text())['status']=='PASS':
        link_status='USER_LINKS_CAPTURE_VERIFIED'
    result=dict(schema='fieldleader3d.resource_tests.v1',status='PASS',checks=checks,
                windows_new_collector=link_status,in_game_mod_tests='NOT_RUN')
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status='PASS',checks=len(checks),windows_new_collector=link_status,in_game_mod_tests='NOT_RUN')))


if __name__=='__main__':
    main()
