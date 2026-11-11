#!/usr/bin/env python3
"""Verify address mapping against objdump and runtime input guards with synthetic bytes.
This is NOT a Windows/in-game test and never executes the supplied EXE.
"""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path
from pe_probe import PE, probe


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('exe',type=Path)
    ap.add_argument('--output',required=True,type=Path)
    args = ap.parse_args()
    checks=[]
    before=hashlib.sha256(args.exe.read_bytes()).hexdigest()
    pe=PE(args.exe)
    output=subprocess.check_output(['objdump','-h',str(args.exe)],text=True)
    mapped=[]
    for line in output.splitlines():
        m=re.match(r'\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)',line)
        if not m: continue
        name,size,vma,lma,raw=m.groups()
        section=next(s for s in pe.sections if s['name']==name)
        assert int(vma,16)==pe.image_base+section['rva']
        assert int(raw,16)==section['raw']
        assert int(size,16)==min(section['virtual_size'],section['size'])
        for delta in (0,section['size']-1):
            assert pe.rva_to_offset(section['rva']+delta)==section['raw']+delta
            assert pe.offset_to_rva(section['raw']+delta)==section['rva']+delta
        mapped.append(name)
    assert len(mapped)==len(pe.sections)
    checks.append(dict(name='section_mapping_matches_independent_objdump',status='PASS',sections=mapped))
    text=next(s for s in pe.sections if s['name']=='.text')
    text_end=text['rva']+text['size']
    assert pe.rva_to_offset(text_end) is None
    checks.append(dict(name='unmapped_virtual_gap_rejected',status='PASS'))
    report=probe(args.exe)
    assert report['rip_candidates']==[]
    checks.append(dict(name='on_disk_text_does_not_produce_callable_xref_claim',status='PASS'))
    types={x['name']:x for x in report['rtti']}
    for name in ('.?AVCUnitArmy@@','.?AVCUnitDuel@@'):
        bases=types[name]['locators'][0]['hierarchy']
        assert bases[-1]['name']=='.?AVIUnitBase@@'
    checks.append(dict(name='army_duel_shared_base_from_rtti',status='PASS'))
    with tempfile.TemporaryDirectory(prefix='fieldleader3d_verify_') as tmp:
        temp=Path(tmp)
        target=next(r['rva'] for r in report['strings'] if r['text']=='CharacterEditor.rdb')
        fake=bytearray(text['size'])
        fake[:7]=bytes.fromhex('48 8d 0d')+struct.pack('<i',target-text['rva']-7)
        binary=temp/'SYNTHETIC_ONLY.runtime_text.bin'
        binary.write_bytes(fake)
        manifest=temp/'SYNTHETIC_ONLY.json'
        meta=dict(schema='fieldleader3d.runtime_text.v1',module_name='SAN14CE_KO.exe',
                  exe_sha256=before,exe_size=len(pe.disk_data),image_size=pe.image_size,
                  pe_timestamp=pe.timestamp,section_name='.text',section_rva=text['rva'],
                  section_raw_offset=text['raw'],section_virtual_size=text['virtual_size'],
                  capture_size=text['size'],runtime_text_sha256=hashlib.sha256(fake).hexdigest(),
                  consecutive_reads_identical=True,module_base_hex='0x7ff600000000')
        manifest.write_text(json.dumps(meta))
        overlay=PE(args.exe)
        overlay.attach_runtime_text(binary,manifest)
        candidates=overlay.rip_candidates({target:['synthetic_reference']})
        assert len(candidates)==1 and candidates[0]['rva']==text['rva']
        assert candidates[0]['target_rva']==target
        checks.append(dict(name='synthetic_runtime_rva_mapping_at_different_load_base',status='PASS'))
        for key,value in [('exe_sha256','0'*64),('section_rva',text['rva']+1),
                          ('runtime_text_sha256','f'*64),('capture_size',text['size']-1),
                          ('consecutive_reads_identical',False),('module_base_hex','0x1')]:
            bad={**meta,key:value}
            manifest.write_text(json.dumps(bad))
            try: PE(args.exe).attach_runtime_text(binary,manifest)
            except ValueError: pass
            else: raise AssertionError('Accepted mismatched '+key)
            checks.append(dict(name='reject_mismatch_'+key,status='PASS'))
    after=hashlib.sha256(args.exe.read_bytes()).hexdigest()
    assert after==before
    checks.append(dict(name='source_executable_unchanged',status='PASS',sha256=after))
    result=dict(schema='fieldleader3d.verification.v1',checks=checks,
                windows_collector_execution='NOT_RUN_no_Windows_runtime',
                in_game_verification='NOT_RUN',dll_build='NOT_ATTEMPTED_no_validated_hook_path')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(checks_passed=len(checks),output=str(args.output),in_game='NOT_RUN')))


if __name__=='__main__':main()
