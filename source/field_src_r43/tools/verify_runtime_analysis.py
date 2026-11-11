#!/usr/bin/env python3
"""Meaningful read-only checks against actual captured code and indexes. No game tests."""
import argparse
import copy
import hashlib
import json
import struct
import tempfile
from pathlib import Path
from appearance_tables import extract
from runtime_disasm import RuntimeDisassembler
from rdb_index import parse_index,name_hash


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('inputs',type=Path)
    ap.add_argument('--output',required=True,type=Path);args=ap.parse_args()
    results=[]
    def check(name,condition):
        if not condition:raise AssertionError(name)
        results.append(dict(name=name,status='PASS'))
    d=RuntimeDisassembler(args.inputs)
    check('real_capture_manifest_build_hash_and_stability_validated',bool(d.pe.runtime_info))
    check('joined_pdata_chains_do_not_stop_mid_function',d.ranges(0x37a720)[0]==[(0x37a720,0x37a9fd)])
    check('interior_pdata_resolves_to_original_function',d.ranges(0x37a7fd)[0]==d.ranges(0x37a720)[0])
    checks=[(0x3726d0,0x14,'[r8+0x28]'),(0x372680,0x14,'[r8+0x2c]'),
            (0x1c4ce0,None,'0x140371ba0'),(0x372e80,None,'[rbx+0xa8]')]
    for rva,size,pattern in checks:check(f'objdump_actual_instruction_{rva:08x}',pattern in d.disassemble(rva,size))
    inv=json.loads((args.inputs/'resource_inventory.json').read_text(encoding='utf-8-sig'))
    sizes={r['relative_path'].replace('\\','/'):r['size'] for r in inv['inventory']}
    character=parse_index(args.inputs/'resources/0002/CharacterEditor.rdb',sizes)
    kids=parse_index(args.inputs/'resources/0002/KIDSSystemResource.rdb',sizes)
    check('both_real_indexes_consumed_with_unique_ids',character['parsed_record_count']==319 and kids['parsed_record_count']==9041
          and character['unique_resource_ids']==319 and kids['unique_resource_ids']==9041)
    check('all_4838_external_ranges_fit_actual_inventory',sum('external' in r for idx in [character,kids] for r in idx['records'])==4838)
    ident=name_hash('CharacterEditor.kidssingletondb');row=next(r for r in kids['records'] if r['resource_id']==ident)
    check('computed_internal_db_id_matches_actual_record_and_location',ident==0xb290631c and row['location']=='39d85@436a#e')
    tables=extract(args.inputs/'SAN14CE_KO.exe')
    check('zhao_yun_table_name_scale_and_both_original_tables',tables['characters'][3]['name']=='CE1MotorCharacterSetting［m004_chouun_a］'
          and tables['characters'][3]['original_scale']==1.0)
    with tempfile.TemporaryDirectory() as temp:
        path=Path(temp)/'CharacterEditor.rdb';original=(args.inputs/'resources/0002/CharacterEditor.rdb').read_bytes()
        bad=bytearray(original);struct.pack_into('<Q',bad,32+8,len(original)+1);path.write_bytes(bad)
        try:parse_index(path,sizes)
        except ValueError:results.append(dict(name='malformed_record_length_rejected',status='PASS'))
        else:raise AssertionError('Out-of-file record accepted')
        path.write_bytes(original);bad_sizes=copy.deepcopy(sizes);bad_sizes['0002/CharacterEditor.rdb.bin']=1
        try:parse_index(path,bad_sizes)
        except ValueError:results.append(dict(name='out_of_container_read_rejected',status='PASS'))
        else:raise AssertionError('Out-of-container range accepted')
    check('original_exe_unchanged',hashlib.sha256((args.inputs/'SAN14CE_KO.exe').read_bytes()).hexdigest()==tables['exe_sha256'])
    report=dict(schema='fieldleader3d.runtime_verification.v1',scope='Offline verification of actual user inputs; no in-game mod execution',
                passed=len(results),failed=0,tests=results)
    args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report))


if __name__=='__main__':main()
