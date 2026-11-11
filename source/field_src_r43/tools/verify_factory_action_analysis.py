#!/usr/bin/env python3
"""Cross-check native field provenance, original DB order and unsafe lookup cases."""
import copy
import json
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from motion_analysis import TimingTables, write
from factory_action_analysis import build, decode_factories, make_lookup, lookup_hash


def main():
    root = Path(__file__).resolve().parents[1]
    report = build(root)
    saved = json.loads((root/'analysis/factory_action_analysis.json').read_text())
    checks = []
    def check(name, condition):
        if not condition:
            raise AssertionError(name)
        checks.append(dict(name=name,status='PASS'))
    def rejects(name, fn):
        try:
            fn()
        except (ValueError,struct.error):
            check(name,True)
        else:
            check(name,False)
    check('Factory/code evidence and all original DB maps reproduce the saved report',report==saved)
    check('All 55 settings and 867 unique actions match the two confirmed factory types',
          report['database_summary']['settings']==55 and report['database_summary']['unique_actions']==867
          and report['factories']['motor']['name']=='MotorCharacterSetting'
          and report['factories']['action']['name']=='SequentialPlayer')
    # Independent saved R4 links were sorted by ActionID. Only join by the ID,
    # never zip those sorted rows with original action/group array positions.
    old = {s['setting_hash_hex']:s for s in json.loads((root/'analysis/link_graph.json').read_text())['settings']}
    matches = 0
    for s in report['database_maps']:
        for a in old[s['setting_hash_hex']]['actions']:
            expected = int(a['object_hash_hex'],16)
            if lookup_hash(s['original_action_rows'],s['lookup'],a['action_id']) != expected:
                raise AssertionError('Original hash/index permutation differs from independent R4 link')
            matches += 1
    check('867 lookups agree with independent R4 action links by ActionID',matches==867)
    check('34 settings require nonidentity permutations; sorted position is not source index',
          report['database_summary']['settings_with_nonidentity_sort']==34)
    zhao = next(s for s in report['database_maps'] if s['setting_hash_hex']=='0x7c6a126d')
    check('Absent Zhao requests 6/7/57/59 return zero at the lookup helper',
          all(lookup_hash(zhao['original_action_rows'],zhao['lookup'],v)==0 for v in (6,7,57,59)))
    tool = RuntimeDisassembler(root/'inputs')
    class AlteredCode(TimingTables):
        def __init__(self,changes):
            super().__init__(tool.pe)
            self.changes = changes
        def read(self,rva,size,code=False):
            raw = bytearray(super().read(rva,size,code))
            if code:
                for address,value in self.changes.items():
                    if rva <= address < rva+size:
                        raw[address-rva] = value
            return bytes(raw)
    rejects('Changing native map read from ActionID +0x24 to Flag +0x20 is rejected',
            lambda:decode_factories(AlteredCode({0xbc86af:0x20})))
    replacement = struct.pack('<I',0xd92c6b)
    rejects('Changing property ordinal 8 to the ActionID case is rejected',
            lambda:decode_factories(AlteredCode({0xd92cd8+8*4+i:b for i,b in enumerate(replacement)})))
    disk = bytearray(tool.pe.disk_data)
    struct.pack_into('<Q',disk,tool.pe.rva_to_offset(0x1697508+0x58),tool.pe.image_base+0xd8d670)
    rejects('Wrong concrete factory create method is rejected',lambda:decode_factories(TimingTables(tool.pe,disk)))
    # Deliberately separate Flag, ActionID and source index, unlike the real set
    # where all 867 Flag scalars equal ActionID. No native code is run here.
    rows = [dict(original_array_index=i,action_id=k,flag=[f],group_id=g,action_object_hash_hex=h)
            for i,(k,f,g,h) in enumerate([(57,3,6,'0x1001'),(0,57,9,'0x1002'),(3,0,2,'0x1003'),
                                          (0xfffffffe,1,4,'0x1004')])]
    lookup = make_lookup(rows)
    check('Unsigned key sort preserves original indices with distinct Flag and group values',
          lookup==dict(keys=[0,3,57,0xfffffffe],original_indices=[1,2,0,3])
          and [lookup_hash(rows,lookup,k) for k in (57,0,3,0xfffffffe)]==[0x1001,0x1002,0x1003,0x1004])
    changed = copy.deepcopy(rows)
    for row in changed:
        row['flag'] = [1234]
    check('Changing every Flag leaves the ActionID search result unchanged',make_lookup(changed)==lookup)
    partial = make_lookup(rows,{0,2})
    check('Skipping null handles keeps surviving original indices and returns zero for missing entries',
          partial==dict(keys=[3,57],original_indices=[2,0])
          and lookup_hash(rows,partial,3)==0x1003 and lookup_hash(rows,partial,0)==0)
    check('An empty map returns zero instead of retaining a previous selection',lookup_hash(rows,make_lookup(rows,set()),57)==0)
    ambiguous = copy.deepcopy(rows)
    ambiguous[1]['action_id'] = 57
    rejects('Analysis adapter rejects duplicate keys; unchecked original behavior is not invented',lambda:make_lookup(ambiguous))
    rejects('Negative/current-action request cannot silently enter the nonnegative helper model',lambda:lookup_hash(rows,lookup,-1))
    result = dict(schema='fieldleader3d.factory_action_verification.v1',status='PASS',checks=checks,
        native_calls=False,hook_installation=False,windows_build='NOT_RUN',in_game_test='NOT_RUN',
        scope='Offline provenance and model edge cases; no native function was emulated or invoked')
    write(root/'analysis/factory_action_verification.json',result)
    print(json.dumps(dict(status='PASS',checks=len(checks))))


if __name__ == '__main__':
    main()
