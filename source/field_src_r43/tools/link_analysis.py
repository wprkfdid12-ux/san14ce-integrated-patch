#!/usr/bin/env python3
"""Reproduce LINKS decoding and join actual KIDS references without array-order guesses."""
import argparse
import collections
import hashlib
import json
from pathlib import Path
from kids_database import parse_database, property_names
from rdb_index import parse_index
from rdb_payload import decode_record
from resource_graph import prop


def hx(value):
    return f'0x{value:08x}'


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


def signature(obj):
    return obj['type'], [(p['hash'], p['type'], p['count'], p['raw_hex']) for p in obj['properties']]


class Graph:
    def __init__(self, root):
        self.root = Path(root)
        self.dbs = {}
        self.objects = collections.defaultdict(list)
        for folder in ('resource_decoded', 'link_decoded'):
            for p in sorted((self.root/'analysis'/folder).glob('db_*.bin')):
                db = parse_database(p.read_bytes())
                if p.name in self.dbs:
                    raise ValueError('Duplicate database source')
                self.dbs[p.name] = db
                for o in db['objects']:
                    self.objects[int(o['hash'], 16)].append((p.name, o))
        self.indexes = json.loads((self.root/'analysis/rdb_indexes.json').read_text())['indexes']
        self.indexes += json.loads((self.root/'analysis/link_indexes.json').read_text())
        self.resources = collections.defaultdict(list)
        for index in self.indexes:
            for row in index['records']:
                self.resources[row['resource_id']].append(dict(index_file=index['file'], **row))
        self.payloads = {r['resource_id_hex']: r for r in
                         json.loads((self.root/'analysis/rdb_payloads.json').read_text())['entries']}
        self.names = property_names((self.root/'inputs/SAN14CE_KO.exe').read_bytes(), self.dbs)

    def object(self, ident):
        rows = self.objects.get(ident, [])
        if not rows:
            raise ValueError('Missing referenced object '+hx(ident))
        if any(signature(o) != signature(rows[0][1]) for _, o in rows[1:]):
            raise ValueError('Different objects share hash '+hx(ident))
        return rows[0][1]

    def source(self, ident):
        self.object(ident)
        return [dict(db=fn, offset=o['offset'], type_hex='0x'+o['type'])
                for fn, o in self.objects[ident]]

    def resource(self, ident):
        captured = self.payloads.get(hx(ident))
        return dict(resource_hash_hex=hx(ident), indexes=self.resources.get(ident, []),
                    captured=captured is not None,
                    complete_payload=bool(captured and captured['decoded_entire_payload']))

    def event(self, ident):
        obj = self.object(ident)
        values = {}
        for p in obj['properties']:
            labels = self.names.get(p['hash'], [])
            name = labels[0] if len(labels) == 1 else 'hash_'+p['hash']
            if name in values:
                raise ValueError('Ambiguous event property')
            values[name] = p['values'] if p['values'] is not None else {'raw_hex': p['raw_hex']}
        return dict(object_hash_hex=hx(ident), sources=self.source(ident), properties=values)

    def action(self, ident):
        obj = self.object(ident)
        animations = []
        for value in prop(obj, 'AnimationDataObjectNameHashArray') or []:
            a = self.object(value)
            animations.append(dict(object_hash_hex=hx(value), sources=self.source(value),
                                   files=[self.resource(n) for n in prop(a, 'G1AFileResourceHash') or []]))
        return dict(object_hash_hex=hx(ident), sources=self.source(ident),
                    action_id=prop(obj, 'ActionID')[0], flag=prop(obj, 'Flag'),
                    start_frame=prop(obj, 'StartFrame'), frame_length=prop(obj, 'FrameLength'),
                    loop_start_frame=prop(obj, 'LoopStartFrame'),
                    animations=animations,
                    event_object_hashes=[hx(n) for n in prop(obj, 'DopeSheetObjectNameHashArray') or []])

    def setting(self, registry):
        ident = int(registry['setting_hash_hex'], 16)
        obj = self.object(ident)
        wrapper_id, = prop(obj, 'CharacterModelNameHash')
        wrapper = self.object(wrapper_id)
        descriptor_id, = prop(wrapper, 'DisplaysetObjectNameHash')
        descriptor = self.object(descriptor_id)
        model_id, = prop(descriptor, 'KTGLModelDataResourceHash')
        model = self.payloads[hx(model_id)]
        count, = prop(obj, 'ColorVariationNum')
        flat = prop(obj, 'ColorVariationMaterialObjectNameHashArray')
        if count:
            if len(flat) % count:
                raise ValueError('Unequal color variation rows')
            width = len(flat)//count
            matrix = [[hx(n) for n in flat[i*width:(i+1)*width]] for i in range(count)]
        else:
            if flat:
                raise ValueError('Color entries without declared rows')
            width, matrix = 0, []
        color_name_raw = bytes(prop(obj, 'ColorVariationNameArray'))
        color_names = color_name_raw.rstrip(b'\0').decode('utf-8').split('\0') if color_name_raw else []
        if len(color_names) != count:
            raise ValueError('Variation names/count mismatch')
        action_refs = prop(obj, 'CharacterActionObjectNameHashArray')
        groups = prop(obj, 'CharacterActionGroupIDArray')
        if len(action_refs) != len(groups):
            raise ValueError('Action group array length mismatch')
        actions = [self.action(n) for n in action_refs]
        if len({a['action_id'] for a in actions}) != len(actions):
            raise ValueError('Repeated ActionID needs explicit handling')
        return dict(**registry, setting_sources=self.source(ident),
                    wrapper_hash_hex=hx(wrapper_id), wrapper_sources=self.source(wrapper_id),
                    descriptor_hash_hex=hx(descriptor_id), descriptor_sources=self.source(descriptor_id),
                    model=self.resource(model_id), g1m=model['g1m'],
                    descriptor_resources={name: [self.resource(n) for n in prop(descriptor, name) or [] if n]
                        for name in ('KTGLShaderBindTableResourceHash', 'KTGLPartsModelGroupBindTableResourceHash',
                                     'KTGLMaterialGroupBindTableResourceHash', 'KTGLSwingDataResourceHash')},
                    rig_object_hashes=[hx(n) for n in prop(descriptor, 'RealtimeRigScriptCodeObjectHash') or [] if n],
                    color_variation_count=count, color_material_slots=width,
                    color_names=color_names, color_material_matrix=matrix,
                    wrapper_base_material_names=[hx(n) for n in prop(wrapper, 'MaterialReplacementNameHashes') or []],
                    wrapper_base_material_objects=[hx(n) for n in prop(wrapper, 'MaterialReplacementObjectNames') or []],
                    action_groups=groups, actions=sorted(actions, key=lambda a:a['action_id']))

    def build(self):
        old = json.loads((self.root/'analysis/resource_graph.json').read_text())
        settings = [self.setting(row) for row in old['registry']]
        zhao = next(s for s in settings if s['setting_hash_hex'] == '0x7c6a126d')
        generic = next(s for s in settings if s['setting_hash_hex'] == '0x4e761942')
        horse = next(s for s in settings if s['setting_hash_hex'] == '0xe1866005')
        def clips(s):
            return {a['action_id']: [r['resource_hash_hex'] for v in a['animations'] for r in v['files']]
                    for a in s['actions']}
        za, ga = clips(zhao), clips(generic)
        shared_ids = sorted(za.keys() & ga.keys())
        events = {}
        actions = {}
        for s in settings:
            for a in s['actions']:
                actions[a['object_hash_hex']] = a
                for event_id in a['event_object_hashes']:
                    if event_id not in events:
                        events[event_id] = self.event(int(event_id, 16))
        textures, unresolved_objects = [], []
        for model in old['models']:
            for t in model['textures']:
                ident = int(t['texture_object_hex'], 16)
                if ident not in self.objects:
                    unresolved_objects.append(hx(ident))
                    continue
                obj = self.object(ident)
                resource_id, = prop(obj, 'KTGLTexContextResourceHash')
                textures.append(dict(descriptor_hash_hex=model['descriptor_hash_hex'], slot=t['slot'],
                                     object_hash_hex=hx(ident), resource_hash_hex=hx(resource_id),
                                     tid=prop(obj, 'TID')[0], indexed=bool(self.resources.get(resource_id))))
        material_ids = sorted({int(h, 16) for s in settings for row in s['color_material_matrix'] for h in row if int(h, 16)})
        materials = {hx(n): self.event(n) for n in material_ids}
        rig_ids = sorted({int(h, 16) for s in settings for h in s['rig_object_hashes']})
        rigs = {}
        for n in rig_ids:
            o = self.object(n)
            data = bytes(prop(o, 'ByteCode'))
            rigs[hx(n)] = dict(sources=self.source(n), bytecode_bytes=len(data),
                               bytecode_sha256=hashlib.sha256(data).hexdigest(),
                               referred_host_functions=[hx(v) for v in prop(o, 'ReferredHostFunction') or []],
                               interpretation='Captured original VM bytecode; not decompiled or executed.')
        audio_banks = {}
        for e in events.values():
            for n in e['properties'].get('BankObjectNameHash', []):
                if hx(n) in audio_banks:
                    continue
                bank = self.event(n)
                bank['asset_packages'] = [self.resource(v) for v in
                                          prop(self.object(n), 'KTGLSound2AssetPackageResourceHash') or [] if v]
                audio_banks[hx(n)] = bank
        unique_clips = {r['resource_hash_hex']: r for a in actions.values()
                        for v in a['animations'] for r in v['files']}
        mirrored = [a for a in actions.values() if len(a['sources']) > 1]
        return dict(schema='fieldleader3d.link_graph.v1',
                    database_count=len(self.dbs), database_objects=sum(db['count'] for db in self.dbs.values()),
                    database_summary=[dict(file=fn, objects=db['count'], bytes=db['bytes'],
                                           type_counts=dict(collections.Counter(o['type'] for o in db['objects'])))
                                      for fn, db in self.dbs.items()],
                    index_records=sum(i['parsed_record_count'] for i in self.indexes),
                    settings=settings, actions=actions, events=events, materials=materials,
                    rigs=rigs, audio_banks=audio_banks, texture_bindings=textures,
                    texture_summary=dict(bindings=len(textures), unresolved_objects=sorted(set(unresolved_objects)),
                                         unindexed_resources=sorted({t['resource_hash_hex'] for t in textures if not t['indexed']})),
                    action_summary=dict(unique_actions=len(actions), identical_mirrored_actions=len(mirrored),
                                        unique_clips=len(unique_clips),
                                        unindexed_clips=sorted(h for h, r in unique_clips.items() if not r['indexes'])),
                    comparison=dict(zhao_setting=zhao['setting_hash_hex'], generic_setting=generic['setting_hash_hex'],
                                    shared_action_ids=shared_ids,
                                    shared_action_ids_with_identical_clip_refs=[n for n in shared_ids if za[n] == ga[n]],
                                    generic_additional_action_ids=sorted(ga.keys()-za.keys()),
                                    distinct_model_resources=zhao['model']['resource_hash_hex'] != generic['model']['resource_hash_hex'],
                                    horse_setting=horse['setting_hash_hex'],
                                    caveats=['m024_officerm_a is a generic body category, not a named live officer.',
                                             'Matching clip references do not imply identical per-character event tracks.',
                                             'G1MS byte comparison alone does not establish semantic bone equivalence.']),
                    limitations=['No game functions were called and no game files were changed.',
                                 'Animation and audio payload locations are indexed; their content is not captured.',
                                 'Shared shader table 0x5f150754 is not in the four supplied indexes.',
                                 'Field commander, scene/lifetime and live appearance validation remain open.'])


def decode_links(root):
    capture = root/'inputs/link_capture'
    request = json.loads((capture/'link_request.json').read_text(encoding='utf-8-sig'))
    directory = root/'analysis/link_decoded'
    directory.mkdir(exist_ok=True)
    reports = []
    for row in request['entries']:
        if row['kind'] != 'rdb_record':
            continue
        raw = (capture/'payloads'/row['output_name']).read_bytes()
        data, report = decode_record(raw, dict(indexed_length=row['length'], capture_length=row['length'],
                                              resource_id_hex=row['resource_id_hex'], type_hash_hex=row['type_hash_hex']))
        parse_database(data)
        (directory/row['output_name']).write_bytes(data)
        reports.append(dict(**report, output_name=row['output_name']))
    inventory = json.loads((root/'inputs/resource_inventory.json').read_text(encoding='utf-8-sig'))
    sizes = {r['relative_path'].replace('\\', '/'): r['size'] for r in inventory['inventory']}
    indexes = [parse_index(capture/'payloads'/name, sizes) for name in ('MaterialEditor.rdb', 'RRPreview.rdb')]
    write(root/'analysis/link_payloads.json', reports)
    write(root/'analysis/link_indexes.json', indexes)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('project', type=Path)
    args = p.parse_args()
    decode_links(args.project)
    graph = Graph(args.project)
    report = graph.build()
    write(args.project/'analysis/link_property_names.json', graph.names)
    write(args.project/'analysis/link_graph.json', report)
    print(json.dumps(dict(databases=report['database_count'], objects=report['database_objects'],
                          settings=len(report['settings']), actions=report['action_summary'],
                          textures=report['texture_summary'], materials=len(report['materials']),
                          rigs=len(report['rigs']))))


if __name__ == '__main__':
    main()
