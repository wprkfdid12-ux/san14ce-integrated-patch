#!/usr/bin/env python3
"""Reproduce reviewed factory/ActionID findings from the supplied PE, text and DBs.

This is an offline, build-specific analysis tool. Instruction interpretations are
manual reverse-engineering findings, not PDB symbols or a native-call adapter.
No original game code is executed. On-disk data is not a live data observation.
"""
import bisect
import hashlib
import json
import re
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from motion_analysis import TimingTables, hx, write
from lifecycle_analysis import validate_edge
from link_analysis import Graph
from resource_graph import prop


# Reviewed complete instructions. In particular, +0x20 and +0x24 must not be
# interchanged just because Flag happens to equal ActionID in supplied records.
GUARDS = [
    (0x104127, 'ba eb ce 86 d1', 'MotorCharacterSetting registration type'),
    (0x104134, 'e9 87 67 a3 00', 'tail dispatch to factory registry'),
    (0xb30590, '4c 8b 11 49 ff 62 58', 'factory +0x90 forwards to own +0x58'),
    (0xb3ce87, 'ff 50 20', 'property provider receives current property ordinal'),
    (0xb3ce92, '49 8b 3c dc', 'destination pointer uses that same ordinal'),
    (0xd8d6d1, '48 8d 44 24 60', 'Flag temporary address'),
    (0xd8d6dc, '48 89 45 70', 'Flag destination slot 8, array starts at rbp+0x30'),
    (0xd8d6e4, '48 8d 44 24 5c', 'ActionID temporary address'),
    (0xd8d6ed, '48 89 45 78', 'ActionID destination slot 9'),
    (0xd8d6f1, '48 8d 55 30', 'destination pointer array base'),
    (0xd8de17, '44 8b 74 24 5c 8b 74 24 60', 'read ActionID and Flag separately'),
    (0xd8de5c, '89 71 20 44 89 71 24', 'Flag at payload+0x20; ActionID at +0x24'),
    (0xbc86ad, '8b 48 24 89 0c 32 46 89 04 3a', 'map stores ActionID and original index'),
    (0xbc85af, '4c 89 20 4c 89 48 08 44 89 70 10', 'map values, keys, count'),
    (0xbcc5ce, '8b 08 48 8b 43 38 8b 04 88', 'lookup uses original index in hash array'),
    (0xbcc5dd, '33 c0', 'missing lookup result is zero'),
    (0xbc8bde, 'ff 90 e0 00 00 00', 'action reference bridge can rewrite input hash'),
    (0xbc8bf5, '45 89 04 24', 'store hash after the bridge'),
    (0x371a93, '83 7b 18 00 75 e8', 'already-ready wrapper returns zero'),
    (0x371ae3, '45 33 c9 4c 8b c6 33 d2 48 8b cf', 'duel map initialization disables duplicate check'),
    (0x371af3, '83 7b 28 00 c7 43 18 01 00 00 00', 'map result is not checked before ready write'),
    (0x371b17, '89 4b 18', 'color stage clears ready for a subsequent poll'),
    (0x1c32df, '48 8b 84 24 c0 00 00 00 44 8b cb 48 89 44 24 30', 'field preloader forwards third argument as seventh'),
    (0xc07380, '48 8b 41 30 c3', 'child model handle getter'),
    (0x3786e4, '49 ff 60 08', 'submit child handle to render callback +0x08'),
    (0x37d015, '0f 85 95 08 00 00', 'release nonempty model through 0x37d8b0'),
]

FUNCTIONS = [
    (0x1040b0,None),(0xbc7d80,14),(0xbcd9b0,14),(0xbcd910,6),
    (0xb3a8c0,None),(0xb30590,7),(0xbc8740,None),(0xbcc920,None),
    (0xbcc020,None),(0xbd0c60,None),(0xbc8600,None),(0xbc8450,None),
    (0xbcc5b0,None),(0xbcc690,22),(0xd6c700,None),(0xf591d0,None),
    (0xd93580,14),(0xd93490,6),(0xd92b20,None),(0xd8d670,None),
    (0xb3ce20,None),(0x371a60,None),(0xb27000,4),(0xc07380,5),
    (0x1c3130,None),(0x1c33d0,None),(0x49c810,None),
    (0x1c14e0,None),(0x1d0100,None),(0x1cf4f0,None),
    (0x1c67f0,None),(0x3786b0,None),(0x1c7580,None),(0x37d010,0x78),(0x37d8b0,None),
]


def require(image, rva, expected):
    expected = bytes.fromhex(expected)
    if image.read(rva,len(expected),True) != expected:
        raise ValueError('Reviewed instruction changed at '+hx(rva))


def cstring(image, rva):
    raw = image.read(rva,256)
    end = raw.find(b'\0')
    if end < 0:
        raise ValueError('Unterminated type/property name')
    return raw[:end].decode('ascii')


def rip_lea(image, rva, prefix):
    raw = image.read(rva,7,True)
    if raw[:3] != bytes.fromhex(prefix):
        raise ValueError('Reviewed RIP-relative LEA changed')
    return rva+7+struct.unpack_from('<i',raw,3)[0]


def typeinfo(image, vtable, expected_name, expected_type, methods):
    found = {hx(slot):hx(image.unpack('<Q',vtable+slot)[0]-image.pe.image_base)
             for slot in range(0,0xa0,8)}
    for slot,target in methods.items():
        if int(found[hx(slot)],16) != target:
            raise ValueError('Factory vtable changed at '+hx(vtable+slot))
    name_getter = int(found[hx(0x28)],16)
    require(image,name_getter+7,'48 89 02 48 8b c2 c3')
    name_rva = rip_lea(image,name_getter,'48 8d 05')
    name = cstring(image,name_rva)
    type_getter = int(found[hx(0x30)],16)
    raw = image.read(type_getter,6,True)
    if raw[0] != 0xb8 or raw[5] != 0xc3:
        raise ValueError('Unexpected type ID getter')
    type_id = struct.unpack_from('<I',raw,1)[0]
    if name != expected_name or type_id != expected_type:
        raise ValueError('Factory name/type identity mismatch')
    return dict(name=name,type_id=hx(type_id),name_rva=hx(name_rva),
                vtable_rva=hx(vtable),methods=found)


def decode_factories(image):
    for rva,raw,_ in GUARDS:
        require(image,rva,raw)
    motor = typeinfo(image,0x1697508,'MotorCharacterSetting',0xd186ceeb,
        {0x20:0xbcc920,0x28:0xbcd9b0,0x30:0xbcd910,0x58:0xbc8740,
         0x60:0xbcc020,0x78:0xbd0c60,0x90:0xb30590})
    action = typeinfo(image,0x16c8800,'SequentialPlayer',0x11a85121,
        {0x20:0xd92b20,0x28:0xd93580,0x30:0xd93490,0x58:0xd8d670,0x60:0xd8f020})
    if rip_lea(image,0xbc7d80,'48 8d 05') != 0x1697508:
        raise ValueError('Motor factory constructor vtable mismatch')
    require(image,0xbc7d87,'48 89 01 48 8b c1 c3')
    # Both registration paths reference the same once-initialized factory object.
    static_object = rip_lea(image,0x1040fb,'48 8d 0d')
    if static_object != rip_lea(image,0x104120,'4c 8d 05'):
        raise ValueError('Factory registration object mismatch')
    motor.update(registration_rva=hx(0x1040b0),registry_rva=hx(0xb3a8c0),
                 static_factory_object_rva=hx(static_object),payload_vtable_rva=hx(0x1697500),
                 alternate_create='factory +0x90 tail-jumps to the same factory +0x58')
    fields = []
    for slot,expected_case,name,expected_hash,offset in (
        (8,0xd92c54,'Flag',0x05d92acc,0x20),
        (9,0xd92c6b,'ActionID',0x7fb88531,0x24)):
        case = image.unpack('<I',0xd92cd8+slot*4,code=True)[0]
        if case != expected_case:
            raise ValueError('Action property ordinal changed')
        require(image,case,'c7 42 f8 01 00 00 05')
        require(image,case+14,'c7 42 fc')
        hash_value = image.unpack('<I',case+17,code=True)[0]
        name_rva = rip_lea(image,case+7,'48 8d 05')
        if hash_value != expected_hash or cstring(image,name_rva) != name:
            raise ValueError('Action property name/hash mismatch')
        fields.append(dict(slot=slot,name=name,property_hash=hx(hash_value),
                           case_rva=hx(case),name_rva=hx(name_rva),payload_offset=hx(offset)))
    action['reviewed_scalar_fields'] = fields
    names = [(0xbcc951,'48 8d 3d'),(0xbcc95f,'48 8d 35'),(0xbcc966,'48 8d 2d'),
             (0xbcc96d,'4c 8d 35'),(0xbcc974,'4c 8d 3d'),(0xbcc97b,'4c 8d 25')]
    properties = []
    for slot,(lea,prefix) in enumerate(names):
        case = image.unpack('<I',0xbcca48+4*slot,code=True)[0]
        require(image,case,'c7 42 f8')
        require(image,case+7,'c7 42 fc')
        properties.append(dict(slot=slot,name=cstring(image,rip_lea(image,lea,prefix)),
            property_type=hx(image.unpack('<I',case+3,code=True)[0]),
            property_hash=hx(image.unpack('<I',case+10,code=True)[0]),case_rva=hx(case)))
    motor['properties'] = properties
    return dict(motor=motor,action=action)


def make_lookup(rows, active_indices=None):
    """Offline model of complete payloads; active_indices models null handles only.

    Duplicate keys are refused by this analysis adapter. The original duel call
    disables its duplicate-check option, so this is not its duplicate behavior.
    Nonnull handles with missing payloads are NOT safe inputs to native bc8600.
    """
    if [r['original_array_index'] for r in rows] != list(range(len(rows))):
        raise ValueError('Source rows must retain original reference-array order')
    active = set(range(len(rows))) if active_indices is None else set(active_indices)
    if not active <= set(range(len(rows))):
        raise ValueError('Unknown original array index')
    pairs = []
    for r in rows:
        if r['original_array_index'] not in active:
            continue
        key = r['action_id']
        if not isinstance(key,int) or not 0 <= key <= 0xffffffff:
            raise ValueError('ActionID must be uint32')
        pairs.append((key,r['original_array_index']))
    if len({key for key,_ in pairs}) != len(pairs):
        raise ValueError('Duplicate ActionID: original unchecked behavior not modeled')
    pairs.sort()
    return dict(keys=[p[0] for p in pairs],original_indices=[p[1] for p in pairs])


def lookup_hash(rows, lookup, action_id):
    """Model bcc5b0 only. Negative current-action requests use another function."""
    if not 0 <= action_id <= 0xffffffff:
        raise ValueError('Negative/current-action request is outside this lookup model')
    pos = bisect.bisect_left(lookup['keys'],action_id)
    if pos == len(lookup['keys']) or lookup['keys'][pos] != action_id:
        return 0
    return int(rows[lookup['original_indices'][pos]]['action_object_hash_hex'],16)


def database_maps(root, factories):
    graph = Graph(root)
    settings = json.loads((root/'analysis/link_graph.json').read_text())['settings']
    reports = []
    for s in settings:
        ident = int(s['setting_hash_hex'],16)
        obj = graph.object(ident)
        if int(obj['type'],16) != int(factories['motor']['type_id'],16):
            raise ValueError('DB setting is not the confirmed MotorCharacterSetting type')
        refs = prop(obj,'CharacterActionObjectNameHashArray')
        groups = prop(obj,'CharacterActionGroupIDArray')
        if len(refs) != len(groups):
            raise ValueError('Original source action/group arrays differ in length')
        rows = []
        for index,(ref,group) in enumerate(zip(refs,groups)):
            action_obj = graph.object(ref)
            if int(action_obj['type'],16) != int(factories['action']['type_id'],16):
                raise ValueError('Referenced action is not SequentialPlayer')
            key, = prop(action_obj,'ActionID')
            rows.append(dict(original_array_index=index,action_id=key,flag=prop(action_obj,'Flag'),
                group_id=group,action_object_hash_hex=hx(ref),sources=graph.source(ref)))
        reports.append(dict(setting_hash_hex=hx(ident),setting_name=s['setting_name'],
            character_name=s['character_name'],sources=graph.source(ident),
            original_action_rows=rows,lookup=make_lookup(rows)))
    return reports


def evidence(tool, image, root):
    folder = root/'analysis/factory_action_code_refs'
    folder.mkdir(exist_ok=True)
    records,cache = [],{}
    for rva,size in FUNCTIONS:
        ranges,extent = tool.ranges(rva,size)
        if 'leaf candidate' in extent:
            raise ValueError('Unreviewed leaf extent: '+hx(rva))
        source = tool.disassemble(rva,size)
        path = folder/f'rva_{rva:08x}.asm.txt'
        path.write_text(source,encoding='utf-8')
        cache[rva] = source
        records.append(dict(rva=hx(rva),path=path.relative_to(root).as_posix(),extent=extent,
            ranges=[dict(start_rva=hx(a),end_rva=hx(b),sha256=hashlib.sha256(
                image.read(a,b-a,True)).hexdigest()) for a,b in ranges]))
    decoded = '\n'.join(cache.values())
    guards = []
    for site,raw,label in GUARDS:
        require(image,site,raw)
        if not re.search(r'^\s*'+format(tool.pe.image_base+site,'x')+':',decoded,re.M):
            raise ValueError('Guard is not at a decoded instruction boundary')
        guards.append(dict(rva=hx(site),bytes_hex=bytes.fromhex(raw).hex(),interpretation=label))
    edges = [validate_edge(tool,a,b,cache) for a,b in (
        (0x104102,0xbc7d80),(0xd8d761,0xb3ce20),(0xbc8719,0xbc8450),
        (0xbc8567,0xf591d0),(0xbcc5c4,0xd6c700),(0xbc8bf9,0xb39c10),
        (0x371aee,0xbc8600),(0x49c836,0x1c3130),(0x1c32fd,0x1c33d0),
        (0x1c1531,0x1cf4f0),(0x1c1542,0x1d0100),(0x1c6838,0x3786b0),
        (0x37d8ee,0xb33610),(0x37d8f5,0x13210))]
    return records,guards,edges


def build(root):
    tool = RuntimeDisassembler(root/'inputs')
    image = TimingTables(tool.pe)
    factories = decode_factories(image)
    maps = database_maps(root,factories)
    records,guards,edges = evidence(tool,image,root)
    rows = [r for s in maps for r in s['original_action_rows']]
    return dict(schema='fieldleader3d.factory_action.v1',status='OFFLINE_CODE_CONFIRMED_ACTION_ID_KEY',
        source_exe_sha256=hashlib.sha256(tool.pe.disk_data).hexdigest(),
        runtime_text_sha256=tool.pe.runtime_info['runtime_text_sha256'],
        data_source='Original on-disk PE and supplied DB payloads; not live runtime object contents',
        method='Manually reviewed instructions, exact byte guards, decoded boundaries, and DB type/property joins',
        factories=factories,code_evidence=records,instruction_guards=guards,verified_call_edges=edges,
        action_lookup=dict(key='ActionID',flag_is_lookup_key=False,payload_offset='0x24',
            builder_rva=hx(0xbc8600),sort_rva=hx(0xbc8450),lookup_rva=hx(0xbcc5b0),
            fields={'0x38':'action hashes AFTER bridge', '0x40':'action handle array in original order',
                    '0x58':'sorted map values = original array indices','0x60':'sorted uint32 ActionID keys','0x68':'map count'},
            result='Matching original-array hash; zero when map/key is missing',
            negative_request='37a720 uses bcc690/current index for a negative request; it is a separate path',
            duplicate_policy='Offline adapter rejects duplicates; original duel passes r9b=0, disabling the optional check',
            null_handle_policy='bc8600 skips null handles but dereferences nonnull handle+0x20 without a payload-null check',
            runtime_bridge='bc8740 passes hash/index/group to virtual +0xe0; it reads back and stores the possibly rewritten hash',
            db_maps_scope='Maps below describe supplied DB references before that bridge; live rewritten hashes were not captured'),
        database_summary=dict(settings=len(maps),action_references=len(rows),
            unique_actions=len({r['action_object_hash_hex'] for r in rows}),
            flag_equals_action_id_count=sum(r['flag']==[r['action_id']] for r in rows),
            settings_with_nonidentity_sort=sum(s['lookup']['original_indices']!=list(range(len(s['original_action_rows']))) for s in maps)),
        database_maps=maps,
        loading_and_ownership=dict(
            runtime_payload_fields={'0x0c':'action count','0x20':'pending action counter','0x28':'base FPS 60',
                '0x30':'child model handle','0x38':'action hash array','0x40':'action handle array'},
            create='bc8740 creates a 0x70-byte header plus arrays, recursively loads the child model and resolves action handles',
            phases='bd0c60 polls phase 0 child model, phase 1 action loading, phase 2 pending counter, then phase 3 completion',
            context='Non-null second argument selects worker-job loading; null selects synchronous handle polling',
            field_context_chain='49c810 -> 1c3130 forwards its third argument -> 1c33d0 seventh argument -> generic factory argument 2',
            context_is_proven_scene_pointer=False,
            duel_initialization='371a60 polls the handle, then calls bc8600 for non-kind-2 payloads; map return is ignored',
            readiness_caveat='Already-ready wrappers return zero; the color stage clears +0x18 and can require another poll. Check actual ready/map/payload conditions',
            release='bcc020 frees the lookup allocation, releases each action handle and child model handle, then frees owned arrays and the payload',
            reference_rule='A handle can route cleanup through +0x38/b33610 or 13210; copying a pointer does not establish an independent reference'),
        field_render_and_pool=dict(
            create_rva=hx(0x1c14e0),pool_field='CUnitManager+0x368',
            active_list_field='CUnitManager+0x378',node_links=['CUnitDuel+0x190','CUnitDuel+0x198'],
            bind='1c14e0 first detaches a matching unit, returns its node through 1cf4f0, then acquires a pool node with 1d0100 and invokes virtual bind methods',
            pool_release='1cf4f0 unlinks the active node and pushes it to the free list; it is not native model or army deallocation',
            render='1c67f0 visits five visible model slots -> 3786b0 -> child model handle -> caller render callback virtual +0x08',
            detach='1c7580 clears its unit link and five model pointers; 37d010 -> 37d8b0 releases nonempty model handles and resets state',
            scene_registration='The submission callback is identified, but its concrete field owner, registration/thread lifetime and independent proxy integration remain unverified',
            consequence='Do not retain pooled actor/army pointers across detach/reuse; source observation still needs generation and scene/load invalidation'),
        remaining=['Concrete field render callback owner, Scene lifecycle and thread rules',
            'Action bridge implementations and current equipped weapon/horse plus stat-dependent appearance',
            'Original motion visual semantics, root motion and actual field render verification',
            'Accepted annihilation versus final logical deletion, normal return and save/load transitions',
            'Missing shared shader payload and three texture objects'],
        native_calls=False,hook_installation=False,windows_build='NOT_RUN',in_game_test='NOT_RUN')


def main():
    root = Path(__file__).resolve().parents[1]
    report = build(root)
    write(root/'analysis/factory_action_analysis.json',report)
    print(json.dumps(dict(status='PASS',**report['database_summary'],
        instruction_guards=len(report['instruction_guards']),call_edges=len(report['verified_call_edges']))))


if __name__ == '__main__':
    main()
