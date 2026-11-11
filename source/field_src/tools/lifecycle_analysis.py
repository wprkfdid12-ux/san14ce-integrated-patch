#!/usr/bin/env python3
"""Record reviewed original lifecycle paths with reproducible byte evidence.

Labels and field interpretations are manual reverse-engineering conclusions,
not symbols recovered from a PDB and not automated proof of game semantics.
"""
import hashlib
import json
import re
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from motion_analysis import TimingTables, hx, write


CALL_EDGES = [
    (0x3ae093, 0x258200, 'S14Event::SExecAnnihilateArmyUnit'),
    (0x39ce17, 0x258200, 'S14Event::SExecAnnihilateArmyUnitByUnit'),
    (0x2701c5, 0x31dab0, 'zero-byte path queues SAnnihilate'),
    (0x270208, 0x258200, 'zero-byte path direct/fallback handling'),
    (0x44e155, 0x258200, 'queued record processor'),
    (0x25b020, 0x31dcc0, 'accepted handler reaches pending-army list insertion'),
]


def validate_edge(tool, site, target, disassembly_cache):
    raw = TimingTables(tool.pe).read(site,5,True)
    if raw[0] != 0xe8 or site+5+struct.unpack_from('<i',raw,1)[0] != target:
        raise ValueError('Reviewed call edge changed: '+hx(site))
    fn = tool.pe.function(site)
    if fn is None:
        raise ValueError('Call lacks a verified caller extent')
    root = tool.unwind_root((fn['start_rva'],fn['end_rva'],fn['unwind_rva']))[0]
    if root not in disassembly_cache:
        disassembly_cache[root] = tool.disassemble(root)
    pattern = r'^\s*'+format(tool.pe.image_base+site,'x')+r':\s+(?:[0-9a-f]{2}\s+)+call\s+0x'+format(tool.pe.image_base+target,'x')+r'\s*$'
    if not re.search(pattern,disassembly_cache[root],re.M):
        raise ValueError('Call is not on a decoded instruction boundary')
    return dict(site_rva=hx(site),target_rva=hx(target),caller_rva=hx(root),bytes_hex=raw.hex())


def build(root):
    tool = RuntimeDisassembler(root/'inputs')
    image = TimingTables(tool.pe)
    cache = {}
    edges = [dict(**validate_edge(tool,a,b,cache),interpretation=label) for a,b,label in CALL_EDGES]
    xrefs = json.loads((root/'analysis/annihilate_xrefs/xrefs.json').read_text())
    if xrefs['runtime_sha256'] != tool.pe.runtime_info['runtime_text_sha256']:
        raise ValueError('Xref source mismatch')
    direct = [x for x in xrefs['xrefs'] if x['validated']]
    for x in direct:
        validate_edge(tool,x['instruction_rva'],x['target_rva'],cache)
    # The nonzero branch bypasses queue/annihilation processing.
    code = image.read(0x26fa9c,8,True)
    if code[:4] != bytes.fromhex('85 db 0f 85') or 0x26faa4+struct.unpack_from('<i',code,4)[0] != 0x270214:
        raise ValueError('Zero-value gate differs from reviewed branch')
    functions = [0x371ba0,0x1c33d0,0xb389c0,0xb38bc0,0xb38d60,0x372600,0xb33610,
                 0x258200,0x26f9a0,0x240b40,0x22df00,0x268070,0x31dab0,0x31dcc0,
                 0x232960,0x44e0b0,0x3ae020,0x39cdc0]
    evidence = []
    out = root/'analysis/lifecycle_code_refs'
    out.mkdir(exist_ok=True)
    for rva in functions:
        source = cache[rva] if rva in cache else tool.disassemble(rva)
        path = out/f'rva_{rva:08x}.asm.txt'
        path.write_text(source,encoding='utf-8')
        ranges, extent = tool.ranges(rva)
        evidence.append(dict(rva=hx(rva),path=path.relative_to(root).as_posix(),extent=extent,
                             ranges=[dict(start_rva=hx(a),end_rva=hx(b),sha256=hashlib.sha256(
                                 image.read(a,b-a,True)).hexdigest()) for a,b in ranges]))
    return dict(schema='fieldleader3d.lifecycle_paths.v2', status='OFFLINE_CODE_CANDIDATES_NOT_HOOK_READY',
        runtime_text_sha256=tool.pe.runtime_info['runtime_text_sha256'],code_evidence=evidence,
        verified_call_edges=edges,verified_direct_annihilation_calls=len(direct),
        original_factory=dict(
            duel_entry_rva='0x00371ba0',field_entry_rva='0x001c33d0',
            shared_calls=['0x00b389c0','0x00b38bc0','0x00b38d60'],
            distinction='Duel passes null in argument 2; field wrapper forwards a loading context. This is not proof of a Scene pointer.',
            dispatch='MotorCharacterSetting type 0xd186ceeb, factory vtable 0x1697508, create bc8740; +0x90 forwards to the same +0x58.',
            ownership='Factory release bcc020 frees map and releases child/action handles; field Scene callback ownership and thread rules remain unresolved.',
            further_evidence='analysis/factory_action_analysis.json',
            release_rva='0x00372600',
            release='Model handle +0x38 selects 0xb33610 or 0x13210; wrapper fields are reset afterward.'),
        annihilation=dict(
            handler_rva='0x00258200',semantic_evidence='Named S14Event RTTI handlers call the same function.',
            entry_guard='Invalid units and units already in the pending list can return early.',
            accepted_list_rva='0x0031dcc0',accepted_list_field='CGameDataManager+0x70',
            accepted_list_is_deallocation=False,
            queued_records_rva='0x0031dab0',queued_records_field='CGameDataManager+0x140',record_bytes=48,
            queue_processor_rva='0x0044e0b0',pending_check_rva='0x00232960',
            pending_check_is_health_test=False,
            queue_fields={'0x00':'victim army','0x08':'additional context (not fully identified)',
                          '0x10':'handler argument 2','0x18':'handler argument 3',
                          '0x24':'handler argument 4','0x28':'handler argument 5 / reason candidate'},
            cause_scope='Includes script-forced annihilation and duel results; not all calls mean ordinary combat defeat.',
            zero_value_path=dict(update_rva='0x0026f9a0',setter_rva='0x00240b40',
                field='CArmyUnitData+0x1a',width_bytes=1,
                branch='Nonzero updated value returns; zero can reach queue insertion or the handler.',
                cap_function_rva='0x00268070',on_disk_base_cap=image.unpack('<I',0x1a4978c)[0],
                interpretation='MORALE_CANDIDATE; byte field, base cap 100 and force modifiers support the inference.',
                exact_ui_field_name_verified=False,troop_zero_path_confirmed=False),
            integration_constraint='Observe an accepted original event; do not delay or replace original gameplay handling.'),
        proposed_visual_lifetime=dict(status='DESIGN_ONLY_NO_IMPLEMENTATION',
            live_binding='Field slot + officer ID + generation/scene epoch; raw address alone is insufficient.',
            terminal_visual='Keep only an independently owned visual and last transform after accepted defeat.',
            cleanup='Return, water transition, scene exit and load clear/rebuild visuals without treating all detach calls as defeat.',
            invalidation='Never retain dereferenceable ArmyData/PersonData pointers after original lifetime ends.'),
        remaining=['Concrete field render callback owner, Scene registration/lifecycle and thread rules',
                   'Normal return and final logical deletion sites, generation/scene epoch invalidation',
                   'Exact morale UI/data label and troop-count-zero path',
                   'Accepted-event timing relative to render cleanup for each annihilation cause',
                   'Actual game movement, combat, return, water transition and save/load observations'])


def main():
    root = Path(__file__).resolve().parents[1]
    report = build(root)
    write(root/'analysis/lifecycle_analysis.json',report)
    print(json.dumps(dict(status='PASS',call_edges=len(report['verified_call_edges']),
                          validated_annihilation_calls=report['verified_direct_annihilation_calls'])))


if __name__ == '__main__':
    main()
