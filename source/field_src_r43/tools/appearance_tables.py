#!/usr/bin/env python3
"""Extract original appearance names/scales as evidence, not a replacement selector."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from pe_probe import PE
from rdb_index import name_hash


def extract(exe):
    pe=PE(exe)
    expected='07a047826cfc316dc9e3db6bfddc72b85e8d7de99743cc73a2d7199a1ad3aa40'
    if hashlib.sha256(pe.data).hexdigest()!=expected:raise ValueError('Unverified build')
    rows=[]
    # Count 25 is bounded by the original selector returns 0..24 (RVA 0x371fe0).
    for i in range(25):
        rva=0x1a3e290+i*0x44;off=pe.rva_to_offset(rva)
        name=pe.data[off:off+64].split(b'\0')[0].decode('utf-8')
        alternate=pe.rva_to_offset(0x1a20840+i*0x40)
        assert name.encode()==pe.data[alternate:alternate+64].split(b'\0')[0]
        rows.append(dict(selector=i,name=name,setting_hash_hex=f'0x{name_hash(name):08x}',
            name_rva_hex=hex(rva),scale_rva_hex=hex(rva+64),original_scale=pe.unpack('<f',off+64)[0],
            alternate_name_table_rva_hex=hex(0x1a20840+i*0x40)))
    weapons=[]
    for i in range(6):
        rva=0x1a20e80+i*64;off=pe.rva_to_offset(rva)
        name=pe.data[off:off+64].split(b'\0')[0].decode('utf-8')
        weapons.append(dict(selector=i,name=name,setting_hash_hex=f'0x{name_hash(name):08x}',name_rva_hex=hex(rva)))
    return dict(schema='fieldleader3d.appearance_tables.v1',exe_sha256=expected,
        evidence='Original EXE initialized data; referenced by verified runtime function RVA 0x1c4ce0',
        caveat='Setting hash is not an RDB record ID. Current live data may be changed by other mods.',
        characters=rows,weapons=weapons,
        raw_model_keys=[dict(slot=i,rva_hex=hex(rva),raw_disk_value_hex=f'0x{pe.u32(pe.rva_to_offset(rva)):08x}')
            for i,rva in [(1,0x1a206cc),(3,0x1a207b4),(4,0x1a20834)]])


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path)
    ap.add_argument('--output',required=True,type=Path);args=ap.parse_args()
    report=extract(args.exe)
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(characters=len(report['characters']),weapons=len(report['weapons']),table_copies_match=True)))


if __name__=='__main__':main()
