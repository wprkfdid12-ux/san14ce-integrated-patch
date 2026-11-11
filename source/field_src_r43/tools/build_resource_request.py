#!/usr/bin/env python3
"""Build exact, bounded file-read requests from the verified user RDB indexes."""
import argparse
import json
from pathlib import Path


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--analysis',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();report=json.loads(args.analysis.read_text())
    rows=[];indexes=report['indexes']
    for idx in indexes:
        if idx['file']!='CharacterEditor.rdb':continue
        for row in idx['records']:
            if 'external' not in row:continue
            ext=row['external'];capture=min(ext['length'],65536)
            rows.append(dict(index_file=idx['file'],index_record=row['index'],resource_id_hex=row['resource_id_hex'],
                type_hash_hex=row['type_hash_hex'],relative_path=ext['relative_path'],expected_file_size=ext['inventory_size'],
                offset=ext['offset'],indexed_length=ext['length'],capture_length=capture,
                complete_external_record=capture==ext['length'],
                output_name='character_'+row['resource_id_hex'][2:]+'.bin',
                purpose='Small associated resource in full, or bounded prefix for model/texture format identification'))
    for name in report['exe_internal_db_names']:
        for match in name['matches']:
            ext=match['external']
            if not ext:continue
            idx=next(i for i in indexes if i['file']==match['index_file'])
            row=idx['records'][match['record_index']]
            rows.append(dict(index_file=idx['file'],index_record=row['index'],resource_id_hex=row['resource_id_hex'],
                type_hash_hex=row['type_hash_hex'],relative_path=ext['relative_path'],expected_file_size=ext['inventory_size'],
                offset=ext['offset'],indexed_length=ext['length'],capture_length=ext['length'],complete_external_record=True,
                output_name='db_'+row['resource_id_hex'][2:]+'.bin',purpose=name['name']))
    assert len(set(r['output_name'] for r in rows))==len(rows)
    assert all(0<r['capture_length']<=r['indexed_length'] and r['offset']+r['indexed_length']<=r['expected_file_size'] for r in rows)
    result=dict(schema='fieldleader3d.resource_request.v1',
                expected_exe_sha256='07a047826cfc316dc9e3db6bfddc72b85e8d7de99743cc73a2d7199a1ad3aa40',
                source_inventory='20260920_134719',
                indexes=[dict(relative_path='0002/'+i['file'],sha256=i['sha256'],size=i['bytes']) for i in indexes],
                game_files_modified=False,game_memory_accessed=False,
                requested_bytes=sum(r['capture_length'] for r in rows),ranges=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(ranges=len(rows),bytes=result['requested_bytes'],
        complete_records=sum(r['complete_external_record'] for r in rows),
        files=sorted(set(r['relative_path'] for r in rows)))))


if __name__=='__main__':main()
