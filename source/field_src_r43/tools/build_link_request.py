#!/usr/bin/env python3
"""Request only missing resources referenced by the parsed character database graph."""
import argparse
import json
from pathlib import Path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('project',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();root=args.project
    graph=json.loads((root/'analysis/resource_graph.json').read_text())
    report=json.loads((root/'analysis/rdb_indexes.json').read_text())
    inventory=json.loads((root/'inputs/resource_inventory.json').read_text(encoding='utf-8-sig'))
    sizes={r['relative_path'].replace('\\','/'):r['size'] for r in inventory['inventory']}
    wanted={r['resource_id_hex']:'Original character ResourceNameHashArray dependency; ProjectCommonResource object 0xfd0a61f4'
            for r in graph['character_database_dependencies'] if not r['captured']}
    for model in graph['models']:
        for link in model['links']['RealtimeRigScriptCodeObjectHash']:
            if link['rdb']:
                wanted[link['hash_hex']]='RealtimeRigScriptCodeObjectHash referenced by captured CharacterEditor model descriptor'
    index=next(i for i in report['indexes'] if i['file']=='KIDSSystemResource.rdb')
    rows=[]
    for record in index['records']:
        if record['resource_id_hex'] not in wanted:
            continue
        external=record['external']
        rows.append(dict(kind='rdb_record',relative_path=external['relative_path'],expected_file_size=external['inventory_size'],
                         offset=external['offset'],length=external['length'],output_name='db_'+record['resource_id_hex'][2:]+'.bin',
                         resource_id_hex=record['resource_id_hex'],type_hash_hex=record['type_hash_hex'],
                         index_file=index['file'],index_record=record['index'],purpose=wanted[record['resource_id_hex']]))
    if len(rows)!=len(wanted):
        raise ValueError('A dependency could not be resolved to an indexed external range')
    for name,purpose in [('MaterialEditor.rdb','Locate captured model shader bindings and 897 unresolved texture resource hashes'),
                         ('RRPreview.rdb','Locate G1A/G1S animation resource hashes referenced by captured animation databases')]:
        rel='0002/'+name
        rows.append(dict(kind='index',relative_path=rel,expected_file_size=sizes[rel],offset=0,length=sizes[rel],
                         output_name=name,purpose=purpose))
    result=dict(schema='fieldleader3d.link_request.v1',
                expected_exe_sha256='07a047826cfc316dc9e3db6bfddc72b85e8d7de99743cc73a2d7199a1ad3aa40',
                source_capture='20260920_143326',
                indexes=[dict(relative_path='0002/'+index['file'],size=index['bytes'],sha256=index['sha256'])],
                game_files_modified=False,game_memory_accessed=False,
                requested_bytes=sum(r['length'] for r in rows),entries=rows)
    if result['requested_bytes']>4*1024*1024 or len({r['output_name'] for r in rows})!=len(rows):
        raise ValueError('Unexpected request size or duplicate output names')
    args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(dict(entries=len(rows),bytes=result['requested_bytes'],
                         source_files=sorted({r['relative_path'] for r in rows}))))


if __name__=='__main__':
    main()
