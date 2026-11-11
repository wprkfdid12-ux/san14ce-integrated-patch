#!/usr/bin/env python3
"""Record the R15 color-application gap from existing code/resources/logs only.

This does not run, rebuild, inject into, or change the game or the trial DLL.
It is a diagnosis and a required follow-up, not a completed color fix.
"""
import collections
import hashlib
import json
from pathlib import Path
import re

from runtime_disasm import RuntimeDisassembler


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[1]
    out = root / 'analysis/appearance_color_audit_20260920'
    out.mkdir(exist_ok=True)
    dis = RuntimeDisassembler(root / 'inputs')
    raw = dis.binary.read_bytes()
    checks = []
    # Exact, bounded instructions distinguish selection, queuing and application.
    for rva, expected, meaning in [
        (0x371fbc, '89 43 20', 'resolver stores selected body color at duel+20'),
        (0x371ec8, 'c7 47 24 01 00 00 00', 'equipment traversal can select horse variant1 at duel+24'),
        (0x371ed1, 'c7 47 24 02 00 00 00', 'equipment traversal can select horse variant2 at duel+24'),
        (0x379a22, '44 8b 83 40 01 00 00', 'duel controller reads first duel+20'),
        (0x379a2b, 'e8 d0 e5 e4 ff', 'controller calls actor color request 1c8000'),
        (0x379a3c, '44 8b 83 44 01 00 00', 'controller reads first duel+24 for horse'),
        (0x379a48, 'e8 b3 e5 e4 ff', 'controller queues horse color separately'),
        (0x379550, '89 51 24 44 89 41 2c c7 41 28 01 00 00 00 c3',
         'wrapper stores variant+24, secondary material slot+2c and pending+28'),
        (0x371beb, '4c 89 61 20 4c 89 61 28', 'wrapper loader initially clears color and pending fields'),
        (0x371af3, '83 7b 28 00', 'poller tests pending color request'),
        (0x371b09, 'e8 b2 fc ff ff', 'poller invokes material application 3717c0'),
        (0x371b17, '89 4b 18', 'color stage clears ready for another poll'),
        (0x585e0, '8b 41 28 c3', '585e0 reads action+28, not color+20'),
    ]:
        value = bytes.fromhex(expected)
        assert raw[rva - 0x1000:rva - 0x1000 + len(value)] == value, hex(rva)
        checks.append(dict(rva=hex(rva), bytes=expected, meaning=meaning))
    for rva, size in [(0x1c8000, 0x58), (0x379550, 0xf), (0x3717c0, None)]:
        (out / ('rva_%08x.asm.txt' % rva)).write_text(dis.disassemble(rva, size))
    xrefs = json.loads((out / 'xrefs/xrefs.json').read_text())
    calls = [x for x in xrefs['xrefs'] if x['validated'] and x['target_rva'] == 0x1c8000]
    assert len(calls) == 4 and {x['unwind_root_rva'] for x in calls} == {0x379780}
    source = (root / 'native/field_poc.cpp').read_text()
    assert 'fn<Call1>(FL_CURRENT_APPEARANCE)(p.duel)' in source
    assert 'p.color = at<int>(p.duel, 0x20)' in source
    assert len(re.findall(r'\bp\.color\b', source)) == 2  # assignment and logging only
    contract = (root / 'native/poc_contract.h').read_text()
    assert '1c8000' not in (source + contract).lower()
    settings = {s['setting_hash_hex']: s for s in json.loads((root / 'analysis/link_graph.json').read_text())['settings']}
    registry = json.loads((root / 'analysis/appearance_tables.json').read_text())['characters']
    rows = []
    for body in registry:
        i = body['selector']
        if i not in (*range(11, 17), *range(22, 25)):
            continue
        setting = settings[body['setting_hash_hex']]
        matrix = setting['color_material_matrix']
        rows.append(dict(body=i, name=body['name'], variants=setting['color_variation_count'],
                         material_slots=setting['color_material_slots'],
                         distinct_rows=len({tuple(r) for r in matrix}),
                         original_secondary_slot=5 if i == 14 else 4 if (0x180b000 >> i) & 1 else 0))
    assert len(rows) == 9 and all(r['variants'] == r['distinct_rows'] == 5 for r in rows)
    horse = settings['0xe1866005']
    assert horse['color_variation_count'] == 3 and horse['color_material_slots'] == 2
    assert len({tuple(r) for r in horse['color_material_matrix']}) == 3
    selected = collections.Counter()
    for line in (root / 'inputs/first_field_r14_result/FieldLeader3D_Runtime.log').read_text().splitlines():
        m = re.search(r'CREATE_BEGIN slot=(\d+) officer=(\d+) body=(\d+) horse=(-?\d+) weapon=(-?\d+) colour=(-?\d+) equipment=(-?\d+)', line)
        if m:
            selected[(int(m[3]), int(m[6]))] += 1
    assert sum(selected.values()) == 42
    required = [
        'native/field_poc.cpp', 'native/poc_contract.h', 'tools/audit_appearance_colors.py',
        'analysis/appearance_tables.json', 'analysis/link_graph.json',
        'inputs/SAN14CE_KO.exe', 'inputs/SAN14CE_KO.runtime_text.bin',
        'inputs/first_field_r14_result/FieldLeader3D_Runtime.log',
        'analysis/poc_native_evidence/rva_001c4ce0.asm.txt',
        'analysis/poc_native_evidence/rva_001cad00.asm.txt',
        'analysis/poc_native_evidence/rva_00371f60.asm.txt',
        'analysis/poc_native_evidence/rva_00371df0.asm.txt',
        'analysis/poc_native_evidence/rva_00371a60.asm.txt',
        'analysis/poc_native_evidence/rva_00371ba0.asm.txt',
    ]
    required += [p.relative_to(root).as_posix() for p in sorted(out.rglob('*')) if p.is_file() and p.suffix in ('.txt', '.json') and p.name != 'audit.json']
    report = dict(
        schema='fieldleader3d.appearance_color_audit.v1', date='2026-09-20',
        status='R15_COLOR_REQUEST_OMISSION_CONFIRMED_FIX_PENDING',
        user_report='Female officers all looked the same in part colors. User requires male common officers to be checked too; implementation timing can follow project order.',
        user_test_revision='UNCONFIRMED', male_pixels_reported=False,
        original_runtime_sha256=dis.pe.runtime_info['runtime_text_sha256'],
        finding='The private path selects duel+20 and logs it, but omits the controller\'s separate 1c8000 request. The wrapper\'s pending color flag stays at its initial zero; 371a60 therefore does not enter the 3717c0 material application stage for that selected color.',
        scope='Shared actor creation/update path; not female-specific. Male common bodies and monarch variants are included. Original horse color variation must be preserved for BOTH unique and common officer appearances.',
        original_call_chain=['371f60 selects duel+20', '379780 calls 1c8000 for body color and horse appearance after actor creation', '1c8000 chooses native material slots and tail-calls 379550', '379550 queues wrapper+24/+2c/+28', '371a60 polls readiness then 3717c0 applies original material references'],
        important_correction='585e0/39b470 is action setup; it is not evidence of color application. The equipment-labelled duel+24 field is also passed to horse color application by the original controller.',
        instruction_evidence=checks, verified_controller_calls=calls,
        common_resource_variants=rows,
        horse_color_requirement=dict(
            user_requested=True, scope='all unique and common officer appearances',
            setting_hash=horse['setting_hash_hex'], variant_count=horse['color_variation_count'],
            material_slots=horse['color_material_slots'], names=horse['color_names'],
            original_material_matrix=horse['color_material_matrix'],
            selection='Original 371df0 equipment traversal resolves duel+24; 379780 passes it to 1c8000(actor,1,variant) independently of body color.',
            implementation='Pending alongside the existing body/part color fix, before move/hoof animation.',
            rule='Use the same current native selection as the original duel, including the unchanged/base case. Do not assign a horse color solely from officer identity or unique/common body class.',
            verification='Compare original selected horse variant and applied horse material for base and both alternates, for unique and common bodies. Include multiple simultaneous instances and no color leakage.',
            limitations='Three original resource variants and the selection/apply code path are verified offline. Specific named horse/color correspondences and final rendered colors have not been visually verified; no such correspondence is asserted.'),
        r14_selection_log_counts=[dict(body=b, color=c, units=n) for (b, c), n in sorted(selected.items())],
        implementation_status='NOT_CHANGED: shipped R15 EXE/DLL stay identical',
        scheduled_order='After R15 movement continuity/stability check, before adding run/hoof animations; must be resolved and compared with original appearance before appearance/PART3 completion.',
        acceptance=[
            'Use the native resolved body/horse variants and native per-body material-slot routing; no invented per-officer/random colors.',
            'Preserve all three original horse resource variants for unique AND common body appearances, according to current native equipment selection.',
            'Check common female body16, common male bodies12-15, and applicable monarch/additional bodies11/22-24.',
            'Compare same-base officers with distinct original variants; also preserve matching colors when the original selects the same variant.',
            'Apply only to private visual actors through verified owner callbacks; never call the real duel controller to recolor a field proxy.',
            'Wait for color material application and subsequent readiness polling before replacing original troops; avoid cross-instance material leakage.',
            'Retain normal cleanup, creation budget, water exclusion, original equipment appearance and existing movement protection.',
            'Verify code-selected and actually applied material identities, then representative in-game duel/field appearance. The latter is still pending.',
        ],
        limitations=['This is an offline code/resource audit, not a live GPU material capture or a completed fix.',
                     'Identical female colors alone do not prove a bug: the original common variants are force-dependent, so same-base/same-force officers can legitimately match.',
                     'Existing R14 logs prove selected numbers, not the resulting pixel colors. No new game collection is requested.'],
        proof_inputs={rel: dict(sha256=sha(root / rel), bytes=(root / rel).stat().st_size) for rel in sorted(set(required))},
    )
    (out / 'audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps(dict(status=report['status'], common_body_settings=len(rows), distinct_original_variants_per_setting=5,
                          native_bytes_and_inputs='CHECKED', live_color_fix_verified=False), ensure_ascii=False))


if __name__ == '__main__':
    main()
