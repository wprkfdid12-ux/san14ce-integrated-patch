#!/usr/bin/env python3
"""Validate a future STATE capture. This does not claim a Windows test has occurred."""
import argparse
import base64
import hashlib
import json
import zipfile
from pathlib import Path, PurePosixPath
from rdb_payload import decode_record


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


BLOCK_LENGTHS={'data_manager_root':8,'unit_manager_root':8,'duel_manager_root':8,
    'army_pointer_table':4008,'army':512,'officer_pointer':8,'officer':496,
    'officer_faction_pointer':8,'officer_faction':80,'officer_country_pointer':8,'officer_country':80,
    'tile_pointer':8,'tile':32,'terrain_pointer':8,'terrain':48,'unit_manager':1176,
    'field_header':80,'field_visual_state':432,'duel_manager':288,'duel_unit':448,'duel_model_slots':416}


def verify(archive_path,request_path,destination=None):
    request_bytes=Path(request_path).read_bytes(); request=json.loads(request_bytes)
    with zipfile.ZipFile(archive_path) as z:
        if sum(i.file_size for i in z.infolist())>32*1024*1024 or z.testzip() is not None:
            raise ValueError('ZIP bounds/CRC failure')
        members={}
        for info in z.infolist():
            name=info.filename.replace('\\','/'); path=PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or ':' in name or name in members or info.is_dir():
                raise ValueError('Unsafe/duplicate archive member')
            members[name]=info
        expected={'READ_ME.txt','state_request.json','state_capture.json','live_state.json'}
        expected.update('payloads/'+e['output_name'] for e in request['entries'])
        if set(members)!=expected:raise ValueError('Archive entries differ')
        read=lambda n:z.read(members[n])
        if read('state_request.json')!=request_bytes:raise ValueError('Issued request differs')
        capture=json.loads(read('state_capture.json').decode('utf-8-sig'))
        if capture['schema']!='fieldleader3d.state_capture.v1' or capture['request_sha256']!=digest(request_bytes):
            raise ValueError('Capture schema/request hash differs')
        if capture['exe_sha256']!=request['expected_exe_sha256']:raise ValueError('EXE differs')
        for key in ('game_files_modified','game_memory_written','game_functions_called'):
            if capture[key] is not False:raise ValueError('Unexpected write/call mode')
        rows={e['output_name']:e for e in capture['entries']}
        if len(rows)!=len(capture['entries']) or set(rows)!={e['output_name'] for e in request['entries']}:
            raise ValueError('Captured entry metadata differs')
        for e in request['entries']:
            raw=read('payloads/'+e['output_name']); row=rows[e['output_name']]
            if any(row[k]!=e[k] for k in ('kind','relative_path','offset')):
                raise ValueError('Resource source differs')
            if len(raw)!=e['length'] or row['captured_length']!=len(raw) or row['sha256']!=digest(raw) or row['consecutive_reads_identical'] is not True:
                raise ValueError('Resource length/hash/repeat failure')
            if e['kind']=='rdb_record':
                _,decoded=decode_record(raw,dict(indexed_length=e['length'],capture_length=e['length'],
                    resource_id_hex=e['resource_id_hex'],type_hash_hex=e['type_hash_hex']))
                if not decoded['decoded_entire_payload']:raise ValueError('Partial new asset')
            elif raw[:8]!=b'_DRK0000':raise ValueError('Bad new index header')
        if capture['total_captured_bytes']!=request['requested_bytes']:raise ValueError('Wrong byte total')
        live=json.loads(read('live_state.json').decode('utf-8-sig'))
        if live['schema']!='fieldleader3d.live_state.v1' or not 1<=len(live['snapshots'])<=2:
            raise ValueError('Bad live state schema/count')
        for key in ('game_memory_written','process_suspended','game_functions_called','atomic_snapshot'):
            if live[key] is not False:raise ValueError('Unexpected live mode')
        if not 0<live['read_bytes']<=16*1024*1024:raise ValueError('Read budget exceeded')
        summaries=[]; total=0
        for snap in live['snapshots']:
            keys=set(); unstable=0
            for b in snap['blocks']:
                key=(b['kind'],b['index'])
                if key in keys or BLOCK_LENGTHS.get(b['kind'])!=b['length']:
                    raise ValueError('Duplicate block or undocumented length')
                keys.add(key)
                raw=base64.b64decode(b['bytes_base64'],validate=True)
                if len(raw)!=b['length'] or digest(raw)!=b['sha256']:raise ValueError('Memory block hash/length differs')
                address=int(b['address_hex'],16)
                if not 0x10000<=address<=0x7fffffffffff-len(raw):raise ValueError('Invalid memory address')
                if type(b['consecutive_reads_identical']) is not bool:raise ValueError('Missing repeat flag')
                unstable+=not b['consecutive_reads_identical']; total+=len(raw)
            summaries.append(dict(active_army_data=snap['active_army_data'],field_visuals=snap['field_visuals'],
                                  officers=snap['officers'],duel_units=snap['duel_units'],
                                  global_roots_unchanged=snap['global_roots_unchanged'],
                                  unstable_blocks=unstable,skipped=snap['skipped'],blocks=len(keys)))
        if total>8*1024*1024:raise ValueError('Captured memory budget exceeded')
        if destination:
            for name in members:
                p=Path(destination)/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(read(name))
    return dict(schema='fieldleader3d.state_verification.v1',status='PASS',
                archive_sha256=digest(Path(archive_path).read_bytes()),captured_utc=capture['captured_utc'],
                resource_entries=len(rows),resource_bytes=request['requested_bytes'],memory_bytes=total,snapshots=summaries,
                caveat='Integrity PASS is not semantic field/duel/lifetime validation. Changing blocks remain explicitly marked.')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path)
    p.add_argument('--request',required=True,type=Path);p.add_argument('--output',required=True,type=Path)
    p.add_argument('--extract-to',type=Path);a=p.parse_args()
    r=verify(a.archive,a.request,a.extract_to)
    a.output.write_text(json.dumps(r,ensure_ascii=False,indent=2)+'\n');print(json.dumps(r))
