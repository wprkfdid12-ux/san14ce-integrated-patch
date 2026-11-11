#!/usr/bin/env python3
"""Read the two supplied SAN14CE RDB indexes. Never change or unpack game files.

Record framing is verified empirically against both complete supplied indexes.
Names use the signed-byte 31-power hash observed at runtime RVA 0x158450.
Type values and unknown metadata remain opaque unless separately established.
"""
import argparse
import collections
import hashlib
import json
import re
import struct
from pathlib import Path


def name_hash(value):
    value=value.encode('utf-8') if isinstance(value,str) else value
    factor=1;result=0
    for byte in value:
        if byte==0:break
        factor=(factor*31)&0xffffffff
        result=(result+(byte if byte<128 else byte-256)*factor)&0xffffffff
    return result


def parse_index(path,inventory=None):
    path=Path(path);data=path.read_bytes()
    if len(data)<32 or data[:8]!=b'_DRK0000':raise ValueError('Unsupported RDB header')
    header,unknown,count,index_hash=struct.unpack_from('<4I',data,8)
    if header!=32:raise ValueError('Unsupported header length')
    rows=[];pos=header
    for i in range(count):
        if pos+56>len(data) or data[pos:pos+8]!=b'IDRK0000':raise ValueError(f'Invalid record {i} at {pos:#x}')
        size,location_size,declared_size=struct.unpack_from('<3Q',data,pos+8)
        if size<56 or pos+size>len(data) or location_size>size-56:raise ValueError('Record length outside file')
        words=struct.unpack_from('<6I',data,pos+32)
        locbytes=data[pos+size-location_size:pos+size] if location_size else b''
        if locbytes and locbytes[-1]!=0:raise ValueError('Unterminated external location')
        loc=locbytes.rstrip(b'\0').decode('ascii') if locbytes else None
        row=dict(index=i,index_offset=pos,record_size=size,resource_id=words[1],
                 resource_id_hex=f'0x{words[1]:08x}',type_hash=words[2],type_hash_hex=f'0x{words[2]:08x}',
                 flags_hex=f'0x{words[3]:08x}',declared_payload_size=declared_size,
                 unknown_u32_20=words[0],unknown_u32_30=words[4],unknown_u32_34=words[5],
                 opaque_metadata_hex=data[pos+56:pos+size-location_size].hex(),location=loc)
        if loc:
            m=re.fullmatch(r'([0-9a-fA-F]+)@([0-9a-fA-F]+)(?:#([0-9a-fA-F]+)(?:&([0-9a-fA-F]+))?)?',loc)
            if not m:raise ValueError(f'Unknown external location grammar: {loc}')
            offset,length=int(m[1],16),int(m[2],16)
            filename=path.name+'.bin'
            if m[3] is not None:filename+=str(int(m[3],16))
            if m[4] is not None:filename+='_'+str(int(m[4],16))
            rel='0002/'+filename
            row['external']=dict(relative_path=rel,offset=offset,length=length,
                interpretation='hex offset@length; hex container suffix rendered as decimal filename')
            if inventory is not None:
                actual=inventory.get(rel)
                row['external']['inventory_size']=actual
                row['external']['within_inventory_size']=actual is not None and length>0 and offset+length<=actual
                if not row['external']['within_inventory_size']:raise ValueError(f'External range does not fit inventory: {loc}')
        rows.append(row);pos=(pos+size+3)&~3
    if pos!=len(data):raise ValueError(f'Index trailing bytes: {len(data)-pos}')
    ids=[r['resource_id'] for r in rows]
    return dict(file=path.name,bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),
                header_unknown_0c=unknown,header_id_hex=f'0x{index_hash:08x}',declared_record_count=count,
                parsed_record_count=len(rows),unique_resource_ids=len(set(ids)),
                consumed_entire_file=True,
                counts_by_type=dict(collections.Counter(r['type_hash_hex'] for r in rows)),
                counts_by_container=dict(collections.Counter(r['external']['relative_path'] for r in rows if 'external' in r)),
                records=rows)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('inputs',type=Path);ap.add_argument('--output',required=True,type=Path)
    args=ap.parse_args()
    inventory=json.loads((args.inputs/'resource_inventory.json').read_text(encoding='utf-8-sig'))
    sizes={r['relative_path'].replace('\\','/'):r['size'] for r in inventory['inventory']}
    indexes=[parse_index(p,sizes) for p in sorted((args.inputs/'resources/0002').glob('*.rdb'))]
    raw=(args.inputs/'SAN14CE_KO.exe').read_bytes()
    known=[]
    for m in re.finditer(rb'[\x20-\x7e]{4,}\x00',raw):
        name=m.group()[:-1]
        if b'kidsscndb' not in name and b'kidssingletondb' not in name:continue
        ident=name_hash(name);matches=[]
        for index in indexes:
            for row in index['records']:
                if row['resource_id']==ident:matches.append(dict(index_file=index['file'],record_index=row['index'],external=row.get('external')))
        known.append(dict(name=name.decode(),source_exe_file_offset=m.start(),resource_id_hex=f'0x{ident:08x}',matches=matches))
    capture_available=(args.inputs/'resource_capture/resource_capture.json').is_file()
    report=dict(schema='fieldleader3d.rdb_observation.v1',hash_evidence_rva='0x00158450',
                container_mapping_status='ALL_LOCATIONS_FIT_SUPPLIED_INVENTORY; see rdb_payloads.json for captured header validation' if capture_available else 'ALL_LOCATIONS_FIT_SUPPLIED_INVENTORY; payload headers not yet supplied',
                payload_capture_available=capture_available,
                indexes=indexes,exe_internal_db_names=known)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(indexes=[{k:v for k,v in r.items() if k!='records'} for r in indexes],matched_internal_db_names=sum(bool(r['matches']) for r in known)),ensure_ascii=False))


if __name__=='__main__':main()
