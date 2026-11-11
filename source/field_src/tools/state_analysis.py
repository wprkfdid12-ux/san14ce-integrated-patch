#!/usr/bin/env python3
"""Join the captured field pointers and decode new assets; never access a process.

Branch results below are offline code-derived predictions, not observed duel
loader outputs. The general stat-dependent selector is intentionally unresolved.
"""
import argparse
import base64
import collections
import hashlib
import json
import math
import struct
from pathlib import Path
from pe_probe import PE
from rdb_index import name_hash, parse_index
from rdb_payload import decode_record


def u16(data, off=0):
    return struct.unpack_from('<H', data, off)[0]


def u32(data, off=0):
    return struct.unpack_from('<I', data, off)[0]


def u64(data, off=0):
    return struct.unpack_from('<Q', data, off)[0]


def hx(value):
    return f'0x{value:08x}'


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


class OriginalSelector:
    """Interpret only the original ID-dispatch instructions, with strict bounds.

    There is no hand-written per-officer model table and no x86 execution.
    Reaching the generic branch returns None rather than guessing a body.
    """
    def __init__(self, root):
        self.root = root
        self.pe = PE(root/'inputs/SAN14CE_KO.exe')
        self.code = (root/'inputs/SAN14CE_KO.runtime_text.bin').read_bytes()

    def fixed(self, officer_id):
        pc, edx, eax, comparison = 0x371fef, officer_id, None, None
        def signed(n):
            return n - 0x100000000 if n & 0x80000000 else n
        for _ in range(80):
            if pc == 0x3720eb:
                return None
            if not 0x371fef <= pc < 0x3722a9:
                raise ValueError('ID dispatch left its known extent')
            b = self.code[pc-0x1000:pc-0x1000+8]
            size = None
            if b[:2] in (b'\x81\xfa', b'\x83\xfa', b'\x81\xea', b'\x83\xea'):
                size = 6 if b[0] == 0x81 else 3
                imm = struct.unpack_from('<i' if size == 6 else '<b', b, 2)[0]
                if b[1] == 0xea:
                    edx = (edx-imm) & 0xffffffff
                    comparison = signed(edx)
                else:
                    comparison = signed(edx)-imm
            elif b[0] in (0x74, 0x75, 0x7f) or b[:2] in (b'\x0f\x84', b'\x0f\x85', b'\x0f\x8f'):
                if comparison is None:
                    raise ValueError('Branch without comparison')
                long = b[0] == 0x0f
                op = b[1] if long else b[0]
                size = 6 if long else 2
                displacement = struct.unpack_from('<i' if long else '<b', b, 2 if long else 1)[0]
                take = comparison == 0 if op in (0x74, 0x84) else comparison != 0 if op in (0x75, 0x85) else comparison > 0
                pc += size + (displacement if take else 0)
                continue
            elif b[0] == 0xb8:
                eax, size = u32(b, 1), 5
            elif b[:2] == b'\x33\xc0':
                eax, size = 0, 2
            elif b[:2] == b'\x8d\x42':
                eax, size = (edx+struct.unpack_from('<b', b, 2)[0]) & 0xffffffff, 3
            elif b[:4] == b'\x48\x83\xc4\x20':
                size = 4
            elif b[0] == 0x5b:
                size = 1
            elif b[0] == 0xc3:
                if eax is None or not 0 <= eax < 25:
                    raise ValueError('Invalid original body index')
                return eax
            else:
                raise ValueError(f'Unsupported ID-dispatch opcode at {pc:#x}: {b.hex()}')
            pc += size
        raise ValueError('ID-dispatch instruction limit')

    def body(self, person, country):
        index = self.fixed(u16(person, 0x10))
        route = 'original_fixed_id_branch'
        if index is None:
            route = 'original_generic_branch'
            if person[0xa9] == 1:
                index = 16
            elif person[0xe6] == 1:
                index = 10 if country[0x47] == 1 else 11
            else:
                return dict(status='UNRESOLVED_STAT_DEPENDENCIES', route=route,
                            reason='RVA 22f210 uses items, roles, policies and current limits outside this capture. Base stats alone are insufficient.')
        offset = self.pe.rva_to_offset(0x1a3e290+index*0x44)
        row = self.pe.data[offset:offset+0x44]
        name = row[:64].split(b'\0', 1)[0].decode('utf-8')
        variant = None
        if 11 <= index <= 16 or 22 <= index <= 24:
            variant = u32(self.pe.data, self.pe.rva_to_offset(0x1a3e190+country[0x49]*4))
        return dict(status='CODE_DERIVED_BODY_BRANCH', route=route, selector=index,
                    setting_name=name, setting_hash_hex=hx(name_hash(name)),
                    original_scale=struct.unpack_from('<f', row, 64)[0],
                    color_variant=variant,
                    caveat='Assumes original object validity checks pass. No live selector call, weapon/item resolution, or rendered appearance was observed.')


class Snapshot:
    def __init__(self, value, base):
        self.value, self.base = value, base
        self.blocks = {}
        for row in value['blocks']:
            key = row['kind'], row['index']
            if key in self.blocks:
                raise ValueError('Duplicate memory block')
            data = base64.b64decode(row['bytes_base64'], validate=True)
            if len(data) != row['length'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                raise ValueError('Memory block checksum mismatch')
            self.blocks[key] = dict(row, data=data, address=int(row['address_hex'], 16))

    def get(self, kind, index=0):
        return self.blocks[kind, index]

    def pointer(self, kind, index, expected_address, target_kind, target_index):
        pointer = self.get(kind, index)
        target = self.get(target_kind, target_index)
        if pointer['address'] != expected_address or u64(pointer['data']) != target['address']:
            raise ValueError(f'Broken captured pointer: {kind}/{index}')
        return target['data']

    def build(self, selector, graph):
        gdm = u64(self.get('data_manager_root')['data'])
        manager = self.pointer('unit_manager_root', 0, self.base+0x2143260, 'unit_manager', 0)
        if self.get('data_manager_root')['address'] != self.base+0x277df18:
            raise ValueError('Wrong data-manager global')
        table = self.get('army_pointer_table')
        if table['address'] != gdm+0x7e838:
            raise ValueError('Wrong army pointer table')
        pool, capacity = u64(manager, 0x10), u32(manager, 0x40)
        if not 1 <= capacity <= 500:
            raise ValueError('Invalid field pool capacity')
        fields = collections.defaultdict(list)
        for (kind, slot), header in self.blocks.items():
            if kind != 'field_header':
                continue
            if not 0 <= slot < capacity or header['address'] != pool+slot*0x8f0:
                raise ValueError('Field header is not in original pool slot')
            if u64(header['data']) != self.base+0x13a0358:
                raise ValueError('Wrong CUnitArmy vtable')
            fields[u64(header['data'], 0x40)].append(slot)
        rows = []
        settings = {s['setting_hash_hex']:s for s in graph['settings']}
        field_names = {r['index']:r['name'] for r in json.loads((selector.root/'analysis/resource_graph.json').read_text())['field_pool_table']}
        for (kind, army_index), army in sorted(self.blocks.items()):
            if kind != 'army':
                continue
            data = army['data']
            if u64(table['data'], army_index*8) != army['address'] or u64(data) != self.base+0x1342458:
                raise ValueError('Invalid CArmyUnitData pointer or vtable')
            if not data[0x10]:
                raise ValueError('Inactive army')
            officer_id = u16(data, 0x12)
            person = self.pointer('officer_pointer', officer_id, gdm+0x158+officer_id*8, 'officer', officer_id)
            if u64(person) != self.base+0x13a6b38 or u16(person, 0x10) != officer_id:
                raise ValueError('Army officer ID does not match actual person object')
            faction_index = person[0xe0] if person[0xe0] <= 51 else 0
            faction = self.pointer('officer_faction_pointer', officer_id, gdm+0xe0d0+faction_index*8, 'officer_faction', officer_id)
            country_index = faction[0x10] if faction[0x10] <= 51 else 0
            country = self.pointer('officer_country_pointer', officer_id, gdm+0xdf30+country_index*8, 'officer_country', officer_id)
            if u64(faction) != self.base+0x13a6860 or u64(country) != self.base+0x13a67f0:
                raise ValueError('Wrong district/force vtable')
            tile_id = u16(data, 0x2a)
            tile = self.pointer('tile_pointer', army_index, gdm+0xe270+tile_id*8, 'tile', army_index)
            terrain_id = tile[0x10]
            terrain = self.pointer('terrain_pointer', army_index, gdm+0x73860+terrain_id*8, 'terrain', army_index)
            if u64(tile) != self.base+0x13a5f60 or u64(terrain) != self.base+0x13a6c80:
                raise ValueError('Wrong hex/terrain vtable')
            slots = fields[army['address']]
            if len(slots) != 1:
                raise ValueError('Army does not have exactly one captured field visual')
            slot = slots[0]
            header = self.get('field_header', slot)
            visual = self.get('field_visual_state', slot)
            if visual['address'] != header['address']+0x740:
                raise ValueError('Visual state address mismatch')
            vd = visual['data']
            mode = u32(vd)
            water = bool(terrain[0x2c] & 2)
            siege = data[0x1b] in (8, 9, 10)
            if (mode == 1) != water or (mode != 3 and not water and (mode == 2) != siege):
                raise ValueError('Visual mode disagrees with original terrain/formation branch')
            model_slots = []
            for n in range(7):
                off = 0x750+n*0x28
                count, model_id, other = struct.unpack_from('<3I', vd, off-0x740+0x18)
                model_slots.append(dict(offset_hex=hex(off), count=count, model_index=model_id,
                                        model_name=field_names.get(model_id), unknown=other))
            if water:
                target = model_slots[5]
                if target['count'] != 1 or target['model_index'] not in (7, 8, 9):
                    raise ValueError('Water branch is not using a ship model slot')
            elif mode == 2:
                target = model_slots[2]
                if target['count'] != 3 or target['model_index'] != data[0x1b]-4:
                    raise ValueError('Siege model slot does not match original formation mapping')
            world = list(struct.unpack_from('<3f', header['data'], 0x10))
            yaw = struct.unpack_from('<f', vd, 4)[0]
            if not all(math.isfinite(n) for n in world+[yaw]):
                raise ValueError('Non-finite field transform')
            name = ''.join(person[a:b].decode('utf-16-le').split('\0', 1)[0] for a,b in ((0x12,0x20),(0x20,0x2e)))
            appearance = selector.body(person, country)
            if 'setting_hash_hex' in appearance:
                setting = settings[appearance['setting_hash_hex']]
                appearance.update(wrapper_hash_hex=setting['wrapper_hash_hex'], descriptor_hash_hex=setting['descriptor_hash_hex'],
                                  model_resource_hex=setting['model']['resource_hash_hex'],
                                  color_rows=setting['color_variation_count'])
                if appearance['color_variant'] is not None and not 0 <= appearance['color_variant'] < setting['color_variation_count']:
                    raise ValueError('Body color row outside original setting')
            rows.append(dict(army_index=army_index, army_address_hex=army['address_hex'],
                             field_pool_slot=slot, field_address_hex=header['address_hex'],
                             officer_id=officer_id, officer_name=name,
                             officer_address_hex=self.get('officer', officer_id)['address_hex'],
                             position_xyz=world, orientation_radians=yaw,
                             tile_id=tile_id, terrain_id=terrain_id, terrain_flags_hex=hx(u32(terrain, 0x2c)),
                             district_index=faction_index, force_index=country_index,
                             force_color_index=country[0x49],
                             land_formation=data[0x1b], water_formation=data[0x1c],
                             field_mode=mode, field_class={0:'land_troops',1:'water_ship',2:'land_siege',3:'elephant_branch'}[mode],
                             model_slots=model_slots, appearance=appearance))
        if len(rows) != self.value['active_army_data'] or len(rows) != self.value['field_visuals']:
            raise ValueError('Active counts do not match verified bindings')
        return dict(captured_utc=self.value['captured_utc'], finished_utc=self.value['finished_utc'],
                    field_pool_capacity=capacity, mode_counts=dict(collections.Counter(r['field_class'] for r in rows)),
                    matched_armies=len(rows), rows=rows)


def decode_assets(root):
    capture = root/'inputs/state_capture'
    out = root/'analysis/state_decoded'
    out.mkdir(exist_ok=True)
    request = json.loads((capture/'state_request.json').read_text(encoding='utf-8-sig'))
    entries = []
    for row in request['entries']:
        if row['kind'] != 'rdb_record':
            continue
        data, report = decode_record((capture/'payloads'/row['output_name']).read_bytes(),
            dict(indexed_length=row['length'], capture_length=row['length'],
                 resource_id_hex=row['resource_id_hex'], type_hash_hex=row['type_hash_hex']))
        (out/row['output_name']).write_bytes(data)
        report['output_name'] = row['output_name']
        entries.append(report)
    write(root/'analysis/state_payloads.json', dict(schema='fieldleader3d.state_payloads.v1', entries=entries))
    inventory = json.loads((root/'inputs/resource_inventory.json').read_text(encoding='utf-8-sig'))
    sizes = {r['relative_path'].replace('\\','/'):r['size'] for r in inventory['inventory']}
    index = parse_index(capture/'payloads/system.rdb', sizes)
    write(root/'analysis/state_index.json', index)
    return entries, index


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', nargs='?', type=Path, default=Path(__file__).resolve().parents[1])
    root = parser.parse_args().root.resolve()
    selector = OriginalSelector(root)
    graph = json.loads((root/'analysis/link_graph.json').read_text())
    live = json.loads((root/'inputs/state_capture/live_state.json').read_text())
    base = int(live['module_base_hex'], 16)
    snapshots = [Snapshot(s, base) for s in live['snapshots']]
    results = [s.build(selector, graph) for s in snapshots]
    changes = []
    if len(snapshots) == 2:
        a, b = snapshots
        for key in sorted(set(a.blocks) | set(b.blocks)):
            if key not in a.blocks or key not in b.blocks:
                changes.append(dict(kind=key[0], index=key[1], status='added_or_removed'))
            elif a.blocks[key]['data'] != b.blocks[key]['data']:
                changes.append(dict(kind=key[0], index=key[1], changed_byte_offsets=[i for i,(x,y) in enumerate(zip(a.blocks[key]['data'], b.blocks[key]['data'])) if x != y]))
    entries, index = decode_assets(root)
    report = dict(schema='fieldleader3d.field_state_analysis.v1', snapshots=results,
                  cross_snapshot_changed_blocks=changes,
                  original_fixed_branches=[dict(officer_id=i, body_selector=v) for i in range(6001) if (v:=selector.fixed(i)) is not None],
                  decoded_resource_count=len(entries), decoded_resource_bytes=sum(e['decoded_bytes'] for e in entries),
                  new_index_records=index['parsed_record_count'],
                  located_shared_shader=[r for r in index['records'] if r['resource_id_hex']=='0x5f150754'],
                  captured_type_names=dict(officer_faction='CDistrictData', officer_country='CForceData',
                                           tile='CHexData', terrain='CTerrainData'),
                  caveats=['31 bindings are actual field/officer pointer observations; body selection is an offline code-derived branch result.',
                           'Original validity virtual calls were not executed. General stat-dependent bodies and current equipped weapons remain unresolved.',
                           'Snapshots are not atomic. Consecutive reads were stable, but separate captures contain visual-state changes.',
                           'No active duel, field proxy, land/water transition, return, annihilation or save-load lifetime was observed.'])
    write(root/'analysis/field_state_analysis.json', report)
    print(json.dumps(dict(snapshots=len(results), matched_armies=results[0]['matched_armies'],
                          modes=results[0]['mode_counts'], changed_blocks=len(changes),
                          body_branches=collections.Counter(r['appearance']['status'] for r in results[0]['rows'])), ensure_ascii=False))


if __name__ == '__main__':
    main()
