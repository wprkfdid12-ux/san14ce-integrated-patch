#!/usr/bin/env python3
"""Decode captured IDRK records, retaining the distinction between full and prefix data.

The framing is derived from supplied records. Each little-endian compressed length
precedes an independent zlib stream of at most 16 KiB; a zero length ends the record.
Incomplete final compressed chunks are never treated as complete data.
"""
import argparse
import collections
import hashlib
import json
import struct
import zlib
from pathlib import Path


def decode_record(raw, expected=None):
    if len(raw) < 56 or raw[:8] != b'IDRK0000':
        raise ValueError('Unsupported external record header')
    total, packed, declared = struct.unpack_from('<3Q', raw, 8)
    unknown, ident, kind, flags, metadata_hash, metadata_count = struct.unpack_from('<6I', raw, 32)
    start = total - packed
    if not 56 <= start <= len(raw) <= total or not 0 < declared <= 128 * 1024 * 1024:
        raise ValueError('Invalid record bounds')
    if flags != 0x100000:
        raise ValueError('Unsupported external record flags')
    if expected:
        if total != expected['indexed_length'] or ident != int(expected['resource_id_hex'], 16) or kind != int(expected['type_hash_hex'], 16):
            raise ValueError('External record does not match index')
        if len(raw) != expected['capture_length']:
            raise ValueError('Capture size mismatch')
    pos = start
    output = bytearray()
    blocks = []
    terminated = False
    pending = None
    while pos + 4 <= len(raw):
        chunk_offset = pos
        length, = struct.unpack_from('<I', raw, pos)
        pos += 4
        if length == 0:
            terminated = True
            break
        if length > 1024 * 1024:
            raise ValueError('Unsupported compressed block size')
        if pos + length > len(raw):
            pending = dict(offset=chunk_offset, compressed_bytes=length, available_bytes=len(raw)-pos)
            break
        stream = zlib.decompressobj()
        decoded = stream.decompress(raw[pos:pos+length], 16385)
        if len(decoded) > 16384 or not stream.eof or stream.unused_data or stream.unconsumed_tail:
            raise ValueError('Invalid, oversized, or incomplete zlib block')
        blocks.append(dict(offset=chunk_offset, compressed_bytes=length, decoded_bytes=len(decoded)))
        output.extend(decoded)
        if len(output) > declared:
            raise ValueError('Decoded bytes exceed declared size')
        pos += length
    complete = len(raw) == total
    if complete and (not terminated or pos != total or len(output) != declared):
        raise ValueError('Complete external record does not fully decode')
    if not complete and terminated:
        raise ValueError('Unexpected early end marker')
    report = dict(resource_id_hex=f'0x{ident:08x}', type_hash_hex=f'0x{kind:08x}',
                  external_bytes=total, captured_bytes=len(raw), packed_bytes=packed,
                  declared_decoded_bytes=declared, decoded_bytes=len(output), payload_offset=start,
                  complete_external_record=complete, decoded_entire_payload=complete,
                  flags_hex=f'0x{flags:08x}', unknown_u32_20=unknown,
                  metadata_hash_hex=f'0x{metadata_hash:08x}', metadata_count=metadata_count,
                  metadata_hex=raw[56:start].hex(), blocks=blocks, incomplete_final_block=pending,
                  decoded_sha256=hashlib.sha256(output).hexdigest(), magic_hex=output[:12].hex())
    return bytes(output), report


def model_chunks(data):
    """List only complete G1M chunks present in the decoded prefix; no mesh edits."""
    if data[:8] != b'_M1G7300' or len(data) < 24:
        raise ValueError('Unsupported G1M header')
    total, start, unknown, count = struct.unpack_from('<4I', data, 8)
    pos = start
    chunks = []
    for _ in range(count):
        if pos + 12 > len(data):
            break
        tag, version, size = struct.unpack_from('<4s4sI', data, pos)
        if size < 12 or pos + size > total:
            raise ValueError('Invalid G1M chunk')
        complete = pos + size <= len(data)
        chunks.append(dict(tag=tag.decode('ascii'), version=version.decode('ascii'), offset=pos,
                           size=size, complete=complete,
                           sha256=hashlib.sha256(data[pos:pos+size]).hexdigest() if complete else None))
        if not complete:
            break
        pos += size
    return dict(declared_bytes=total, declared_chunks=count, chunks=chunks,
                entire_model_present=len(data)==total and len(chunks)==count and pos==total)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--decoded-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    request = json.loads((args.capture/'resource_request.json').read_text(encoding='utf-8-sig'))
    args.decoded_dir.mkdir(parents=True, exist_ok=True)
    records = []
    for row in request['ranges']:
        data, report = decode_record((args.capture/'payloads'/row['output_name']).read_bytes(), row)
        (args.decoded_dir/row['output_name']).write_bytes(data)
        report.update(output_name=row['output_name'], purpose=row['purpose'], source_file=row['relative_path'],
                      source_offset=row['offset'])
        if data.startswith(b'_M1G'):
            report['g1m'] = model_chunks(data)
        records.append(report)
    result = dict(schema='fieldleader3d.rdb_payload.v1', records=len(records),
                  fully_decoded=sum(r['decoded_entire_payload'] for r in records),
                  decoded_prefixes=sum(not r['decoded_entire_payload'] for r in records),
                  decoded_bytes=sum(r['decoded_bytes'] for r in records),
                  type_counts=dict(collections.Counter(r['type_hash_hex'] for r in records)), entries=records)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='entries'}))


if __name__ == '__main__':
    main()
