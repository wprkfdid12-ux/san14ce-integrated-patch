#!/usr/bin/env python3
"""Verify and extract the next LINKS capture against the exact issued request."""
import argparse
import hashlib
import json
import zipfile
from pathlib import Path, PurePosixPath
from rdb_payload import decode_record


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def verify(archive_path, request_path, destination=None):
    request_bytes=Path(request_path).read_bytes()
    request=json.loads(request_bytes)
    with zipfile.ZipFile(archive_path) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP CRC failure')
        members={}
        for info in archive.infolist():
            name=info.filename.replace('\\','/')
            path=PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or ':' in name or name in members or info.is_dir():
                raise ValueError('Unsafe or duplicate archive name')
            members[name]=info
        expected={'READ_ME.txt','link_capture.json','link_request.json'}
        expected.update('payloads/'+row['output_name'] for row in request['entries'])
        if set(members)!=expected:
            raise ValueError('Unexpected or missing capture files')
        get=lambda name:archive.read(members[name])
        if get('link_request.json')!=request_bytes:
            raise ValueError('Issued and captured requests differ')
        capture=json.loads(get('link_capture.json').decode('utf-8-sig'))
        if capture['schema']!='fieldleader3d.link_capture.v1' or capture['request_sha256']!=digest(request_bytes):
            raise ValueError('Capture schema or request hash differs')
        if capture['exe_sha256']!=request['expected_exe_sha256'] or capture['game_files_modified'] is not False or capture['game_memory_accessed'] is not False:
            raise ValueError('EXE or capture mode differs')
        captured={row['output_name']:row for row in capture['entries']}
        if len(captured)!=len(capture['entries']) or set(captured)!={row['output_name'] for row in request['entries']}:
            raise ValueError('Capture entries differ')
        validated=[]
        for wanted in request['entries']:
            row=captured[wanted['output_name']]
            for key in ('kind','relative_path','offset'):
                if row[key]!=wanted[key]:
                    raise ValueError('Capture metadata differs')
            raw=get('payloads/'+row['output_name'])
            if len(raw)!=wanted['length'] or len(raw)!=row['captured_length'] or digest(raw)!=row['sha256'] or row['consecutive_reads_identical'] is not True:
                raise ValueError('Length, hash or consecutive read mismatch')
            if wanted['kind']=='rdb_record':
                _,decoded=decode_record(raw,dict(indexed_length=wanted['length'],capture_length=wanted['length'],
                                                 resource_id_hex=wanted['resource_id_hex'],type_hash_hex=wanted['type_hash_hex']))
                if not decoded['decoded_entire_payload']:
                    raise ValueError('Linked record is incomplete')
            elif raw[:8]!=b'_DRK0000':
                raise ValueError('Unexpected RDB index header')
            validated.append(dict(output_name=row['output_name'],kind=row['kind'],bytes=len(raw),sha256=digest(raw)))
        total=sum(row['bytes'] for row in validated)
        if total!=request['requested_bytes'] or total!=capture['total_captured_bytes']:
            raise ValueError('Capture byte total differs')
        if destination:
            for name in members:
                path=Path(destination)/name
                path.parent.mkdir(parents=True,exist_ok=True)
                path.write_bytes(get(name))
    return dict(schema='fieldleader3d.link_verification.v1',status='PASS',archive_name=Path(archive_path).name,
                archive_sha256=digest(Path(archive_path).read_bytes()),captured_utc=capture['captured_utc'],
                entries=len(validated),bytes=total,files=validated)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive',type=Path)
    parser.add_argument('--request',type=Path,required=True)
    parser.add_argument('--extract-to',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    report=verify(args.archive,args.request,args.extract_to)
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='files'}))


if __name__=='__main__':
    main()
