#!/usr/bin/env python3
"""Read supplied _DOK/IDOK databases and match property hashes to EXE strings.

Type widths are validated by exact record consumption across all supplied DBs.
The semantic meanings of types 0/2/10/12 remain opaque. Labels are hash matches,
not source symbols. Unknown property hashes are retained.
"""
import argparse
import collections
import json
import re
import struct
from pathlib import Path
from rdb_index import name_hash

WIDTHS = {0:1, 1:1, 2:2, 4:4, 5:4, 8:4, 10:16, 12:8}


def parse_database(raw):
    if len(raw) < 28 or raw[:8] != b'_DOK0000':
        raise ValueError('Unsupported KIDS database header')
    start, version, count, database_hash, total = struct.unpack_from('<5I', raw, 8)
    if start != 28 or version != 10 or total != len(raw):
        raise ValueError('Unexpected KIDS header values')
    pos = start
    objects = []
    for _ in range(count):
        if pos+24 > len(raw) or raw[pos:pos+8] != b'IDOK0000':
            raise ValueError('Invalid object header')
        size, ident, kind, fields = struct.unpack_from('<4I', raw, pos+8)
        if size < 24+12*fields or pos+size > len(raw):
            raise ValueError('Invalid object bounds')
        properties = []
        value_offset = pos+24+12*fields
        for i in range(fields):
            data_type, elements, key = struct.unpack_from('<3I', raw, pos+24+12*i)
            if data_type not in WIDTHS:
                raise ValueError('Unknown property width')
            length = elements * WIDTHS[data_type]
            if value_offset+length > pos+size:
                raise ValueError('Property data extends beyond object')
            data = raw[value_offset:value_offset+length]
            if data_type in (1,4,5,8):
                fmt = {1:'B',4:'i',5:'I',8:'f'}[data_type]
                values = list(struct.unpack('<'+str(elements)+fmt, data))
            else:
                values = None
            properties.append(dict(hash=f'{key:08x}', type=data_type, count=elements,
                                   offset=value_offset, raw_hex=data.hex(), values=values))
            value_offset += length
        if value_offset != pos+size:
            raise ValueError('Unconsumed object data')
        objects.append(dict(hash=f'{ident:08x}', type=f'{kind:08x}', offset=pos, size=size, properties=properties))
        pos = (pos+size+3)&~3
    if pos != len(raw) or len({o['hash'] for o in objects}) != len(objects):
        raise ValueError('Unconsumed DB data or duplicate object hashes')
    return dict(database_hash=f'{database_hash:08x}', bytes=len(raw), count=count, objects=objects)


def property_names(exe, databases):
    wanted={p['hash'] for db in databases.values() for o in db['objects'] for p in o['properties']}
    names=collections.defaultdict(set)
    for match in re.finditer(rb'[\x20-\x7e]{3,}\x00', exe):
        value=match.group()[:-1]
        key=f'{name_hash(value):08x}'
        if key in wanted:
            names[key].add(value.decode('ascii'))
    return {k:sorted(v) for k,v in names.items()}


def named_properties(obj, names):
    return [{**p,'candidate_names':names.get(p['hash'],[])} for p in obj['properties']]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--decoded-dir',type=Path,required=True)
    parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    databases={p.name:parse_database(p.read_bytes()) for p in sorted(args.decoded_dir.glob('db_*.bin'))}
    names=property_names(args.exe.read_bytes(),databases)
    summary=[]
    for filename, db in databases.items():
        summary.append(dict(file=filename, database_hash=db['database_hash'], bytes=db['bytes'], objects=db['count'],
                            types=dict(collections.Counter(o['type'] for o in db['objects']))))
    models=[]
    for obj in databases['db_b290631c.bin']['objects']:
        if any(p['hash']==f'{name_hash("KTGLModelDataResourceHash"):08x}' for p in obj['properties']):
            models.append({**obj,'properties':named_properties(obj,names)})
    result=dict(schema='fieldleader3d.kids_database.v1', databases=summary,
                total_objects=sum(db['count'] for db in databases.values()),
                all_record_and_property_lengths_match=True, property_names=names, character_model_descriptors=models,
                caveat='Model descriptor hashes have not yet been linked to CE1MotorCharacterSetting names.')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(dict(databases=len(databases),objects=result['total_objects'],
                          matched_property_names=len(names),character_models=len(models))))


if __name__=='__main__':
    main()
