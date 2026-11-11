#!/usr/bin/env python3
"""Request new clip/audio payloads plus a bounded live field/officer snapshot."""
import hashlib
import json
from pathlib import Path
from pe_probe import PE


def build(root):
    graph=json.loads((root/'analysis/link_graph.json').read_text())
    inventory=json.loads((root/'inputs/resource_inventory.json').read_text(encoding='utf-8-sig'))
    sizes={r['relative_path'].replace('\\','/'):r['size'] for r in inventory['inventory']}
    resources={r['resource_hash_hex']:r for a in graph['actions'].values() for v in a['animations'] for r in v['files']}
    for bank in graph['audio_banks'].values():
        for r in bank['asset_packages']:resources[r['resource_hash_hex']]=r
    entries=[]
    for ident,r in sorted(resources.items()):
        if r['captured']:
            raise ValueError('Do not request previously captured resource')
        if len(r['indexes'])!=1:
            raise ValueError('Ambiguous resource index')
        row=r['indexes'][0];source=row['external']
        entries.append(dict(kind='rdb_record',output_name='asset_'+ident[2:]+'.bin',
                            resource_id_hex=ident,type_hash_hex=row['type_hash_hex'],
                            relative_path=source['relative_path'],offset=source['offset'],
                            length=source['length'],expected_file_size=source['inventory_size']))
    entries.append(dict(kind='index',output_name='system.rdb',relative_path='0002/system.rdb',
                        offset=0,length=sizes['0002/system.rdb'],expected_file_size=sizes['0002/system.rdb']))
    old=json.loads((root/'analysis/rdb_indexes.json').read_text())['indexes']
    new=json.loads((root/'analysis/link_indexes.json').read_text())
    indexes=[dict(relative_path='0002/'+r['file'],size=r['bytes'],sha256=r['sha256'])
             for r in old+new if r['file'] in ('RRPreview.rdb','CharacterEditor.rdb')]
    runtime=(root/'inputs/SAN14CE_KO.runtime_text.bin').read_bytes()
    # Small version guards at the actual read-path implementations; no executable code changes.
    guards=[]
    for rva,size in [(0x1c7840,32),(0x22b030,50),(0x233040,40),(0x1c4ea0,36),(0x371f60,32)]:
        raw=runtime[rva-0x1000:rva-0x1000+size]
        guards.append(dict(rva=rva,length=size,sha256=hashlib.sha256(raw).hexdigest()))
    return dict(schema='fieldleader3d.state_request.v1',
                expected_exe_sha256=hashlib.sha256((root/'inputs/SAN14CE_KO.exe').read_bytes()).hexdigest(),
                indexes=indexes,entries=entries,requested_bytes=sum(e['length'] for e in entries),
                runtime_guards=guards,
                capture_intent='Read active field units and their single officer, original appearance inputs, original duel objects; do not execute selector/factory functions.',
                source_evidence=['RVA 0x1c7840: CUnitArmy+0x40 -> CArmyUnitData',
                                 'RVA 0x22b030/0x22aed0: CArmyUnitData+0x12 uint16 -> GameDataManager+0x158+ID*8 -> CPersonData',
                                 'RVA 0x1c4ea0/0x1c27b0: 500 CUnitArmy nodes, stride0x8f0, pool at manager+8',
                                 'RVA 0x371b60/0x379780: duel manager, two embedded units at +0x120/+0x2e0',
                                 'RVA 0x22bf60/0x371f60: officer+0xe0 -> faction tables -> appearance variant'],
                live_limits=dict(snapshots=2,maximum_field_units=500,maximum_army_data=501,
                                 maximum_memory_bytes=8388608,maximum_individual_read=65536),
                limitations=['Non-atomic read-only snapshots; changing entries are marked.',
                             'No DLL installation, memory writes, suspension, injection or game function calls.',
                             'A single map capture is not proof of duel, transition or lifetime behavior.'])


if __name__=='__main__':
    root=Path(__file__).resolve().parents[1]
    report=build(root)
    (root/'collect/state_request.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(entries=len(report['entries']),bytes=report['requested_bytes'],
                          source_files=sorted({r['relative_path'] for r in report['entries']}))))
