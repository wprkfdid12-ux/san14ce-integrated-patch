#!/usr/bin/env python3
"""Join observed KIDS references, character registry and captured RDB resources."""
import argparse
import collections
import json
import re
import struct
from pathlib import Path
from kids_database import parse_database
from rdb_index import name_hash
from pe_probe import PE


def prop(obj, name):
    key=f'{name_hash(name):08x}'
    for p in obj['properties']:
        if p['hash']==key:
            return p['values']
    return None


def build(root):
    root=Path(root)
    directory=root/'analysis/resource_decoded'
    dbs={p.name:parse_database(p.read_bytes()) for p in sorted(directory.glob('db_*.bin'))}
    modeldb=dbs['db_b290631c.bin']
    localobjects={o['hash']:o for o in modeldb['objects']}
    indexes=json.loads((root/'analysis/rdb_indexes.json').read_text())
    resources={r['resource_id_hex'][2:]:{**r,'index_file':i['file']} for i in indexes['indexes'] for r in i['records']}
    registry_obj=next(o for o in dbs['db_6d011726.bin']['objects'] if prop(o,'CharacterKeyHashArray') is not None)
    keys=prop(registry_obj,'CharacterKeyHashArray')
    settings=prop(registry_obj,'CharacterSettingObjectNameHashArray')
    if len(keys)!=len(settings):
        raise ValueError('Registry arrays differ in length')
    pe=PE(root/'inputs/SAN14CE_KO.exe')
    tables=json.loads((root/'analysis/appearance_tables.json').read_text())
    knownsettings={int(r['setting_hash_hex'],16):r['name'] for r in tables['characters']+tables['weapons']}
    names={name_hash(m.group()):m.group().decode() for m in re.finditer(rb'm[a-z]?\d{3}_[a-z0-9_]+',pe.data)}
    registry=[dict(index=i,character_key_hex=f'0x{k:08x}',character_name=names.get(k),
                   setting_hash_hex=f'0x{s:08x}',setting_name=knownsettings.get(s)) for i,(k,s) in enumerate(zip(keys,settings))]
    dependency_obj=next(o for o in dbs['db_6d011726.bin']['objects'] if 0x2082ad97 in (prop(o,'ResourceNameHashArray') or []))
    db_dependencies=[]
    for ident in prop(dependency_obj,'ResourceNameHashArray'):
        row=resources.get(f'{ident:08x}')
        if row is None or 'external' not in row:
            raise ValueError('Original DB dependency is absent from supplied index')
        db_dependencies.append(dict(resource_id_hex=row['resource_id_hex'],external=row['external'],
                                     captured=(directory/f'db_{ident:08x}.bin').exists()))
    models=[]
    texture_stats=collections.Counter()
    unresolved_texture_resources=set()
    for obj in modeldb['objects']:
        mesh=prop(obj,'KTGLModelDataResourceHash')
        if mesh is None:
            continue
        links={}
        for name in ('KTGLModelDataResourceHash','KTGLShaderBindTableResourceHash',
                     'KTGLPartsModelGroupBindTableResourceHash','KTGLMaterialGroupBindTableResourceHash',
                     'KTGLSwingDataResourceHash','RealtimeRigScriptCodeObjectHash'):
            values=prop(obj,name) or []
            links[name]=[dict(hash_hex=f'0x{value:08x}',rdb=resources.get(f'{value:08x}')) for value in values if value]
        texture_state_hash=prop(obj,'TexturesRenderStateObjectHash')[0]
        state=localobjects[f'{texture_state_hash:08x}']
        binding=prop(state,'TextureBindTableCSVFileResourceHash')[0]
        raw=(directory/f'character_{binding:08x}.bin').read_bytes()
        pairs=list(struct.iter_unpack('<II',raw))
        if [i for i,_ in pairs]!=list(range(len(pairs))):
            raise ValueError('Unsupported texture binding table indices')
        textures=[]
        for slot,texhash in pairs:
            texobj=localobjects.get(f'{texhash:08x}')
            row=dict(slot=slot,texture_object_hex=f'0x{texhash:08x}',texture_object_in_character_db=texobj is not None)
            texture_stats['bindings']+=1
            if texobj:
                resource=prop(texobj,'KTGLTexContextResourceHash')[0]
                tid=prop(texobj,'TID')[0]
                source=resources.get(f'{resource:08x}')
                row.update(texture_resource_hex=f'0x{resource:08x}',tid=tid,resource_in_supplied_indexes=source is not None)
                texture_stats['objects_in_character_db']+=1
                if source:
                    texture_stats['resources_in_supplied_indexes']+=1
                else:
                    unresolved_texture_resources.add(resource)
            else:
                texture_stats['objects_outside_character_db']+=1
            textures.append(row)
        models.append(dict(descriptor_hash_hex='0x'+obj['hash'],descriptor_offset=obj['offset'],links=links,
                           texture_binding_resource_hex=f'0x{binding:08x}',textures=textures,
                           motion_sound_container=prop(obj,'MotionSoundContainerObjectNameHash')))
    pool=[]
    for i in range(97):
        rva=0x1a21000+i*0x4c
        off=pe.rva_to_offset(rva)
        row=pe.data[off:off+0x4c]
        name=row[:64].split(b'\0',1)[0].decode('utf-8')
        capacity,unknown,kind=struct.unpack_from('<3I',row,64)
        pool.append(dict(index=i,rva_hex=hex(rva),name=name,capacity=capacity,
                         unknown_u32_44=unknown,kind=kind,
                         registry_key_match=any(r['character_key_hex']==f'0x{name_hash(name):08x}' for r in registry)))
    return dict(schema='fieldleader3d.resource_graph.v1',
                registry_source=dict(db='db_6d011726.bin',object_hash_hex='0x'+registry_obj['hash'],offset=registry_obj['offset']),
                registry=registry,matched_exe_body_settings=sum(r['setting_hash_hex'] in {x['setting_hash_hex'] for x in tables['characters']} for r in registry),
                matched_exe_weapon_settings=sum(r['setting_hash_hex'] in {x['setting_hash_hex'] for x in tables['weapons']} for r in registry),
                dependency_source=dict(db='db_6d011726.bin',object_hash_hex='0x'+dependency_obj['hash'],offset=dependency_obj['offset']),
                character_database_dependencies=db_dependencies,models=models,
                texture_summary={**texture_stats,'unresolved_distinct_resource_hashes':len(unresolved_texture_resources)},
                field_pool_table=pool,
                shared_factory_evidence=dict(duel='0x371ba0',pooled_model='0x1c33d0',factory='0xb389c0',
                                            database_id_hex='0x2082ad97',field_table_rva='0x1a21000'),
                limitations=['Registry setting objects are in uncaptured database 0x2082ad97.',
                             'No specific live generic officer has been compared with Zhao Yun.',
                             'Pool/factory reuse does not prove scene ownership, lifetime, or an in-game proxy.'])


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('project',type=Path)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    report=build(args.project)
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps(dict(registry_entries=len(report['registry']),models=len(report['models']),
                          body_settings=report['matched_exe_body_settings'],weapon_settings=report['matched_exe_weapon_settings'],
                          required_databases=len(report['character_database_dependencies']),
                          texture_summary=report['texture_summary'],
                          original_character_pool_entries=sum(r['kind']==1 for r in report['field_pool_table']))))


if __name__=='__main__':
    main()
