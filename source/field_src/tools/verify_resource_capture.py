#!/usr/bin/env python3
"""Verify every requested range and safely normalize a Windows-created ZIP."""
import argparse
import hashlib
import json
import zipfile
from pathlib import Path, PurePosixPath


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def verify(archive_path, request_path, destination=None):
    expected_bytes = Path(request_path).read_bytes()
    request = json.loads(expected_bytes)
    with zipfile.ZipFile(archive_path) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP CRC failure')
        members = {}
        for entry in archive.infolist():
            name = entry.filename.replace('\\', '/')
            path = PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or ':' in name or name in members or entry.is_dir():
                raise ValueError('Unsafe or duplicate archive path: ' + name)
            members[name] = entry
        expected_names = {'READ_ME.txt', 'resource_capture.json', 'resource_request.json'}
        expected_names.update('payloads/' + row['output_name'] for row in request['ranges'])
        if set(members) != expected_names:
            raise ValueError('Unexpected or missing ZIP members')
        get = lambda name: archive.read(members[name])
        if get('resource_request.json') != expected_bytes:
            raise ValueError('Embedded request differs from issued request')
        capture = json.loads(get('resource_capture.json').decode('utf-8-sig'))
        if capture['schema'] != 'fieldleader3d.resource_capture.v1':
            raise ValueError('Unsupported capture schema')
        if capture['request_sha256'] != sha256(expected_bytes):
            raise ValueError('Request hash mismatch')
        if capture['exe_sha256'] != request['expected_exe_sha256']:
            raise ValueError('EXE hash mismatch')
        if capture['game_memory_accessed'] is not False or capture['game_files_modified'] is not False:
            raise ValueError('Unexpected collector activity')
        captured = {row['output_name']: row for row in capture['ranges']}
        if len(captured) != len(capture['ranges']) or set(captured) != {row['output_name'] for row in request['ranges']}:
            raise ValueError('Capture range set differs')
        validated = []
        for wanted in request['ranges']:
            row = captured[wanted['output_name']]
            for key in ('relative_path', 'offset', 'indexed_length', 'resource_id_hex', 'complete_external_record'):
                if row[key] != wanted[key]:
                    raise ValueError('Range metadata differs: ' + key)
            raw = get('payloads/' + row['output_name'])
            if len(raw) != row['captured_length'] or len(raw) != wanted['capture_length']:
                raise ValueError('Range length mismatch')
            if sha256(raw) != row['sha256'] or row['consecutive_reads_identical'] is not True:
                raise ValueError('Range hash or stability failure')
            validated.append(dict(output_name=row['output_name'], bytes=len(raw), sha256=sha256(raw),
                                  complete_external_record=row['complete_external_record']))
        total = sum(row['bytes'] for row in validated)
        if total != capture['total_captured_bytes'] or total != request['requested_bytes']:
            raise ValueError('Capture total differs')
        if destination:
            destination = Path(destination)
            for name in members:
                target = destination / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(get(name))
    return dict(schema='fieldleader3d.resource_verification.v1',
                archive_name=Path(archive_path).name, archive_sha256=sha256(Path(archive_path).read_bytes()),
                captured_utc=capture['captured_utc'], status='PASS', ranges=len(validated), bytes=total,
                complete_records=sum(row['complete_external_record'] for row in validated),
                partial_records=sum(not row['complete_external_record'] for row in validated),
                request_sha256=sha256(expected_bytes), exe_sha256=capture['exe_sha256'], files=validated)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--request', type=Path, required=True)
    parser.add_argument('--extract-to', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = verify(args.archive, args.request, args.extract_to)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({key: value for key, value in result.items() if key != 'files'}))


if __name__ == '__main__':
    main()
