#!/usr/bin/env python3
"""Verify capture provenance and reject unsafe render-ABI substitutions."""
import json
import hashlib
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from motion_analysis import TimingTables,write
from render_context_analysis import build,decode_contract,instruction_map,validate_guard_boundaries


def main():
    root=Path(__file__).resolve().parents[1]
    report=build(root)
    saved=json.loads((root/'analysis/render_context_analysis.json').read_text())
    checks=[]
    def check(name,result):
        if not result:
            raise AssertionError(name)
        checks.append(dict(name=name,status='PASS'))
    def rejects(name,fn):
        try:
            fn()
        except ValueError:
            check(name,True)
        else:
            check(name,False)
    check('Reviewed field path, code evidence, original vtables and DB coverage reproduce the report',report==saved)
    c=report['contract']
    check('The same +0x98 resource slot links acquisition, draw and release; payload is +0x20',
        c['resource_context_global_rva']=='0x027d1208' and c['handle_offset']==0x98
        and c['payload_offset']==0x20 and c['resource_hash']=='0xe65d70ae')
    check('Update then manager draw then model-pool submission is backed by three decoded calls',
        [v['target_rva'] for v in report['field_frame']['ordered_calls']]==['0x001cb280','0x001c6940','0x001c6550'])
    check('No concrete field-list identity is inferred from the standalone Array implementation',
        report['database_coverage']['captured_databases']==46
        and report['database_coverage']['captured_objects']==36190
        and report['database_coverage']['field_list_object_sources']==[]
        and report['database_coverage']['array_object_count']==0
        and c['array_candidate']['confirmed_field_instance'] is False
        and report['database_coverage']['live_callback_vtable_observed'] is False)
    tool=RuntimeDisassembler(root/'inputs')
    class AlteredCode(TimingTables):
        def __init__(self,changes):
            super().__init__(tool.pe);self.changes=changes
        def read(self,rva,size,code=False):
            raw=bytearray(super().read(rva,size,code))
            if code:
                for address,value in self.changes.items():
                    if rva<=address<rva+size:
                        raw[address-rva]=value
            return bytes(raw)
    rejects('Changing callback handle offset +0x98 to +0x90 is rejected',
        lambda:decode_contract(AlteredCode({0x44a443:0x90})))
    rejects('Confusing handle payload +0x20 with type metadata +0x18 is rejected',
        lambda:decode_contract(AlteredCode({0x44a44f:0x18})))
    rejects('Substituting MotorCharacterSetting payload for its child model handle is rejected',
        lambda:decode_contract(AlteredCode({0xc07383:0x20})))
    rejects('Changing the manager flag branch to bypass the duel list is rejected',
        lambda:decode_contract(AlteredCode({0x1c6974:0x88})))
    rejects('Removing the Array item retain increment is rejected',
        lambda:decode_contract(AlteredCode({0xb44190:0x90,0xb44191:0x90})))
    def changed_pointer(rva,target):
        disk=bytearray(tool.pe.disk_data)
        struct.pack_into('<Q',disk,tool.pe.rva_to_offset(rva),tool.pe.image_base+target)
        return TimingTables(tool.pe,disk)
    rejects('Using the Array clear method as its append method is rejected',
        lambda:decode_contract(changed_pointer(0x16914f8+8,0xb44260)))
    rejects('Changing the actor draw vtable target is rejected',
        lambda:decode_contract(changed_pointer(0x13a0570+0x28,0x1c6940)))
    rejects('An altered Array type ID cannot pass on name alone',
        lambda:decode_contract(AlteredCode({0xb44e91:0x42})))
    decoded=instruction_map(tool.disassemble(0x3786b0),tool.pe.image_base)
    rejects('A guard truncated inside the indirect tailcall is rejected',
        lambda:validate_guard_boundaries([(0x3786e4,'49 ff 60','truncated instruction')],decoded))
    rejects('An interior byte pattern cannot be used as an instruction boundary',
        lambda:validate_guard_boundaries([(0x3786e5,'ff 60 08','inside instruction')],decoded))
    result=dict(schema='fieldleader3d.render_context_verification.v1',status='PASS',checks=checks,
        analysis_report_sha256=hashlib.sha256((root/'analysis/render_context_analysis.json').read_bytes()).hexdigest(),
        verifier_tool_sha256=hashlib.sha256((root/'tools/verify_render_context_analysis.py').read_bytes()).hexdigest(),
        scope='Offline evidence and ABI-substitution rejection; no native function execution or emulation',
        native_calls=False,hook_installation=False,windows_build='NOT_RUN',in_game_tests='NOT_RUN')
    write(root/'analysis/render_context_verification.json',result)
    print(json.dumps(dict(status='PASS',checks=len(checks))))


if __name__=='__main__':
    main()
