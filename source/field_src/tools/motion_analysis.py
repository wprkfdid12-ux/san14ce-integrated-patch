#!/usr/bin/env python3
"""Read original timing descriptors; model their clock without calling the game.

The fixed locations/field meanings are build-specific, manually reviewed code
findings. Table rows, request values and resource joins are read from the inputs.
This is an offline research tool, not an animation player or a hook installer.
"""
import collections
import hashlib
import json
import math
import re
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from link_analysis import Graph
from resource_graph import prop


def hx(n):
    return f'0x{n:08x}'


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


# Reviewed RTTI vtables, execute methods, request tags and object sizes.
REQUEST_TYPES = {
    0x13b5b70: ('Voice', 0x372740, 0, 32),
    0x13b5b88: ('PersonMotion', 0x3726d0, 1, 32),
    0x13b5ba0: ('HorseMotion', 0x372680, 4, 32),
    0x13b5bb8: ('HorseMotionFrame', 0x3726a0, 5, 32),
    0x13b5bd0: ('Hide', 0x372660, 0xffffffff, 24),
    0x13b5be8: ('UnitEffect', 0x3726f0, 8, 32),
}
REQUEST_BEGIN, REQUEST_END = 0x1a3ec40, 0x1a3f2d8
POINTER_BEGIN, POINTER_END = 0x1a3e938, REQUEST_BEGIN


class TimingTables:
    def __init__(self, pe, disk_data=None):
        self.pe = pe
        self.disk = pe.disk_data if disk_data is None else disk_data

    def read(self, rva, size, code=False):
        section = next((s for s in self.pe.sections
                        if s['rva'] <= rva and rva+size <= s['rva']+s['size']), None)
        if size < 0 or section is None:
            raise ValueError('Range outside one backed PE section')
        data = self.pe.data if code else self.disk
        off = section['raw']+rva-section['rva']
        if off+size > len(data):
            raise ValueError('Truncated source range')
        return data[off:off+size]

    def unpack(self, fmt, rva, code=False):
        return struct.unpack(fmt, self.read(rva, struct.calcsize(fmt), code))

    def pointer(self, va):
        rva = va-self.pe.image_base
        self.read(rva, 1)
        return rva

    def parse(self):
        # Read the actual LEA displacement and count immediate in Unit initialization.
        code = self.read(0x3730b2, 24, True)
        if code[:3] != bytes.fromhex('48 8d 05') or code[7:20] != bytes.fromhex(
                '48 89 83 30 01 00 00 c7 83 2c 01 00 00'):
            raise ValueError('Timing-table initializer does not match reviewed code')
        table_rva = 0x3730b9+struct.unpack_from('<i', code, 3)[0]
        count, = struct.unpack_from('<I', code, 20)
        if not 0 < count <= 128:
            raise ValueError('Unreasonable descriptor count')
        self.read(table_rva, count*24)
        requests = {}
        at = REQUEST_BEGIN
        while at < REQUEST_END:
            vt, = self.unpack('<Q', at)
            vt_rva = self.pointer(vt)
            if vt_rva not in REQUEST_TYPES:
                raise ValueError('Unknown request vtable')
            kind, execute, tag, size = REQUEST_TYPES[vt_rva]
            if at+size > REQUEST_END:
                raise ValueError('Request straddles inventory boundary')
            method, = self.unpack('<Q', vt_rva+8)
            if method != self.pe.image_base+execute:
                raise ValueError('Request execute method mismatch')
            observed_tag, frame, repeat, interval = self.unpack('<4I', at+8)
            if observed_tag != tag or repeat not in (0, 1) or (repeat and not interval):
                raise ValueError('Invalid request tag or repeat interval')
            row = dict(request_rva=hx(at), kind=kind, vtable_rva=hx(vt_rva),
                       execute_rva=hx(execute), tag_hex=hx(tag), bytes=size,
                       start_frame=frame, repeat=repeat, interval=interval)
            if size == 32:
                value, extra = self.unpack('<If', at+24)
                if not math.isfinite(extra):
                    raise ValueError('Non-finite request parameter')
                row.update(value=value, extra_float=extra)
            requests[hx(at)] = row
            at += size
        timelines = []
        ids = set()
        used = set()
        for index in range(count):
            rva = table_rva+index*24
            ident, pad1, ptr, n, pad2 = self.unpack('<IIQII', rva)
            if ident in ids or pad1 or pad2:
                raise ValueError('Duplicate ID or nonzero descriptor padding')
            ids.add(ident)
            array_rva = self.pointer(ptr)
            if n == 0 or n > 64 or array_rva % 8 or not (
                    POINTER_BEGIN <= array_rva < array_rva+n*8 <= POINTER_END):
                raise ValueError('Invalid request pointer-array extent')
            refs = []
            for va in self.unpack('<'+'Q'*n, array_rva):
                request_rva = hx(self.pointer(va))
                if request_rva not in requests:
                    raise ValueError('Pointer is not an inventoried request start')
                refs.append(request_rva)
                used.add(request_rva)
            timelines.append(dict(index=index, timeline_id=ident, descriptor_rva=hx(rva),
                                  pointer_array_rva=hx(array_rva), requests_in_original_order=refs))
        targets = self.unpack('<29I', 0x379178, True)
        if any(not 0x378ca6 <= v <= 0x379160 for v in targets):
            raise ValueError('State dispatch target outside reviewed function')
        clock_rate, = self.unpack('<f', 0x13ae8a8)
        if clock_rate != 60.0:
            raise ValueError('Original clock base rate differs from the reviewed model')
        return dict(descriptor_table_rva=hx(table_rva), descriptor_count=count,
                    clock_base_rate=clock_rate,
                    descriptor_stride=24, request_inventory_count=len(requests),
                    request_kind_counts=dict(collections.Counter(r['kind'] for r in requests.values())),
                    requests=requests, timelines=timelines,
                    unreferenced_request_rvas=sorted(set(requests)-used),
                    missing_ids_in_range=sorted(set(range(max(ids)+1))-ids),
                    state_dispatch=[dict(state=i, entry_rva=hx(v)) for i, v in enumerate(targets)])


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def select_original(tables, requested_id, previous_id):
    """379740 resets clock even if it finds no ID; missing IDs retain old pointer."""
    found = any(t['timeline_id'] == requested_id for t in tables['timelines'])
    return dict(clock=0.0, selected_id=requested_id if found else previous_id, matched=found)


def due_requests(tables, timeline_id, clock, fps):
    """Float32 nonrepeat branch of 37a3c0; preserve array order, not frame sort.

    Every captured request is nonrepeat. Repeating requests are deliberately
    unsupported until integer overflow/conversion behavior has been specified.
    """
    if not math.isfinite(fps) or fps <= 0 or not math.isfinite(clock) or clock < 0:
        raise ValueError('Invalid clock or FPS')
    timeline = next((t for t in tables['timelines'] if t['timeline_id'] == timeline_id), None)
    if timeline is None:
        raise ValueError('Missing timeline: an adapter must not reuse the previous selection silently')
    clock = f32(clock)
    step = f32(60.0/f32(fps))
    due = []
    for ref in timeline['requests_in_original_order']:
        req = tables['requests'][ref]
        if req['repeat']:
            raise ValueError('Repeat requests are not supported by this offline clock model')
        start = f32(req['start_frame'])
        if start <= clock < f32(start+step):
            due.append(ref)
    return due, f32(clock+step)


def simulate(tables, timeline_id, fps, until_frame=242):
    clock = 0.0
    fired = []
    for tick in range(math.ceil(until_frame*fps/60)+2):
        if clock > until_frame:
            break
        refs, nxt = due_requests(tables, timeline_id, clock, fps)
        for ref in refs:
            fired.append(dict(tick=tick, clock=clock, request_rva=ref))
        clock = nxt
    return fired


def resource_joins(root, tables):
    graph = Graph(root)
    settings = json.loads((root/'analysis/link_graph.json').read_text())['settings']
    clips = json.loads((root/'analysis/g2a_analysis.json').read_text())['clips']
    # Re-read the original ordered reference/group arrays. R4's actions are sorted.
    joined = []
    for s in settings:
        if s['setting_hash_hex'] not in {'0x7c6a126d','0x20d9cc18','0x4e761942','0xe1866005'}:
            continue
        original = graph.object(int(s['setting_hash_hex'], 16))
        refs = prop(original, 'CharacterActionObjectNameHashArray')
        groups = prop(original, 'CharacterActionGroupIDArray')
        if len(refs) != len(groups):
            raise ValueError('Action/group array mismatch')
        by_id = {}
        for index, (ref, group) in enumerate(zip(refs, groups)):
            action = graph.action(ref)
            if action['action_id'] in by_id:
                raise ValueError('Ambiguous ActionID')
            files = [c['resource_hash_hex'] for a in action['animations'] for c in a['files']]
            if any(c not in clips for c in files):
                raise ValueError('Referenced clip is not captured')
            by_id[action['action_id']] = dict(action_id=action['action_id'], flag=action['flag'],
                    original_array_index=index, group_id=group, action_object_hash_hex=hx(ref),
                    clips=[dict(resource_hash_hex=c, fps=clips[c]['fps'], frames=clips[c]['frames']) for c in files])
        horse = s['setting_hash_hex'] == '0xe1866005'
        requested = sorted({r['value'] for r in tables['requests'].values()
                            if r['kind'] in (('HorseMotion','HorseMotionFrame') if horse else ('PersonMotion',))})
        joined.append(dict(setting_hash_hex=s['setting_hash_hex'], setting_name=s['setting_name'],
                           character_name=s['character_name'], role='horse' if horse else 'body',
                           matching_actions=[by_id[v] for v in requested if v in by_id],
                           missing_requested_values=[v for v in requested if v not in by_id],
                           join_status='CODE_CONFIRMED_ACTION_ID_KEY_SOURCE_DB_JOIN',
                           key_evidence='analysis/factory_action_analysis.json',
                           key_limit='Native search uses SequentialPlayer+0x24 ActionID. These are source DB hashes; the factory action bridge may rewrite runtime hashes.'))
    return joined


def code_evidence(tool, outdir):
    outdir.mkdir(exist_ok=True)
    evidence = []
    # Leaf extents end at their reviewed RET, never at an arbitrary 256-byte window.
    functions = [(0x372660,0x1b),(0x372680,0x14),(0x3726a0,0x28),(0x3726d0,0x14),
                 (0x3726f0,None),(0x372740,None),(0x379740,0x3f),
                 (0x373000,None),(0x37a3c0,None),(0x37a4e0,None),(0x37a1b0,None),
                 (0x378c70,None),(0x375980,None),(0x37a720,None),(0xbcc5b0,None)]
    for rva, size in functions:
        ranges, extent = tool.ranges(rva,size)
        path = outdir/f'rva_{rva:08x}.asm.txt'
        path.write_text(tool.disassemble(rva,size),encoding='utf-8')
        evidence.append(dict(function_rva=hx(rva), disassembly=path.relative_to(outdir.parent.parent).as_posix(),
                             extent=extent, ranges=[dict(start_rva=hx(a),end_rva=hx(b),
                             sha256=hashlib.sha256(TimingTables(tool.pe).read(a,b-a,True)).hexdigest()) for a,b in ranges]))
    return evidence


def build(root):
    tool = RuntimeDisassembler(root/'inputs')
    tables = TimingTables(tool.pe).parse()
    rtti_strings = [s for s in tool.pe.ascii_strings() if re.fullmatch(
        r'\.\?AVCRequest\w+@timing_data@duel@@', s['text'])]
    rtti = tool.pe.rtti(rtti_strings)
    discovered = {v['rva'] for row in rtti for col in row['locators'] for v in col['vtables']}
    if not set(REQUEST_TYPES) <= discovered:
        raise ValueError('Timing request vtables do not match original RTTI')
    evidence = code_evidence(tool, root/'analysis/motion_code_refs')
    return dict(schema='fieldleader3d.motion_timing.v2', status='OFFLINE_ORIGINAL_CODE_ANALYSIS',
                source_exe_sha256=hashlib.sha256(tool.pe.disk_data).hexdigest(),
                runtime_text_sha256=tool.pe.runtime_info['runtime_text_sha256'],
                tables_source='On-disk PE data with preferred-base pointers; not a live data snapshot',
                tables=tables, request_rtti=rtti, code_evidence=evidence,
                sample_resource_joins=resource_joins(root,tables),
                clock=dict(base_rate=tables['clock_base_rate'], clock_offset='unit+0x128', descriptor_offset='unit+0x138',
                           step='float32(60 / runtime_fps)', trigger='start <= clock < float32(start + step)',
                           array_order_preserved=True, repeat_branch_simulated=False,
                           invalid_id_behavior='Clock resets but prior descriptor pointer remains'),
                effects=dict(PersonMotion='unit+0x28=request.value; unit+0x30=1.0',
                             HorseMotion='unit+0x2c=request.value; unit+0x34=1.0',
                             HorseMotionFrame='unit+0x2c=request.value; unit+0x34=max(extra_float,0)',
                             Hide='Clear all five flags at unit+0x108,+0x10c,+0x110,+0x114,+0x118',
                             Voice='Officer-specific voice request via 0x403650; distinct from horse sound events',
                             UnitEffect='Request effect through CUnitDuel / 0x1c5b80'),
                example_timeline_1_clock_runs={str(fps):simulate(tables,1,fps) for fps in (30,60,120)},
                limitations=[
                    'No native function was called, no hook installed, no animation rendered.',
                    'Timing schedule frames are not clip lengths and do not prove visual action semantics.',
                    'State 3 immediately selects timeline 3 and then timeline 4 by fallthrough.',
                    'DuelManager update also moves duel units and camera; it is not a field-proxy tick.',
                    'Separate duel position integration does not establish suppression of clip root motion.',
                    'ActionID key and zero-on-missing lookup are confirmed in factory_action_analysis.json; later playback effects and runtime hash rewrites are not observed.',
                    'The unreferenced effect request is inventoried but is not activated by these descriptors.',
                    'Only constant positive FPS and nonrepeat requests are modeled offline.'])


def render_tables(report):
    tables = report['tables']
    labels = {'PersonMotion':'몸체 요청','HorseMotion':'말 요청','HorseMotionFrame':'말 시작 프레임 요청',
              'Voice':'장수 음성','Hide':'표시 슬롯 5개 숨김','UnitEffect':'효과 요청'}
    lines = ['# 원본 일기토 요청표 — R7 동작 검색 근거 반영', '',
        '기존 실행파일의 데이터와 캡처된 코드에서 읽은 표다. 실제 게임에서 재생한 결과가 아니다.', '',
        '39개 묶음 안에서 요청 객체 52개를 참조한다. 전체 인접 객체 53개 중 효과 요청 1개는 이 표에서 참조하지 않는다.',
        '번호 25는 없고 39는 있다. 표의 행 번호를 요청 ID로 사용하면 안 된다.', '',
        '아래 @ 값은 원본 요청 시점이다. 영상이나 G2A 클립 길이가 아니며, 원본 포인터 배열 순서를 유지했다.', '',
        '| 요청표 ID | 배열 순서대로 읽은 요청 (@시점) |', '| --- | --- |']
    for t in tables['timelines']:
        parts=[]
        for ref in t['requests_in_original_order']:
            r=tables['requests'][ref]
            label=labels[r['kind']]
            if 'value' in r: label+=' '+str(r['value'])
            if r['kind']=='HorseMotionFrame': label+=f" (매개변수 {r['extra_float']:g})"
            parts.append(label+' @'+str(r['start_frame']))
        lines.append(f"| {t['timeline_id']} | {' → '.join(parts)} |")
    lines += ['', '## 원본 동작 리소스 연결 후보', '',
        'R7에서 원본 검색 키가 SequentialPlayer+0x24의 ActionID임을 확인했다. Flag는+0x20의 별도 필드다. 검색 실패는0을 반환한다. 아래 연결은 수집된 DB 기준이며, 생성 중 리소스 해시를 바꿀 수 있는 원본 bridge의 실행 결과는 관측하지 않았다.',
        'ActionGroup은 정렬된 Action 목록에 임의로 붙이지 않고 원본 참조 배열의 같은 위치에서 읽었다.', '',
        '| 표본 setting | 몸체/말 | 연결된 요청값 수 | 해당 표본에 없는 요청값 |', '| --- | --- | --- | --- |']
    for s in report['sample_resource_joins']:
        missing=', '.join(map(str,s['missing_requested_values'])) or '없음'
        name=s['setting_name'] or '원본 이름 미확인'
        role='말' if s['role']=='horse' else '몸체'
        lines.append(f"| {name} ({s['setting_hash_hex']}) | {role} | {len(s['matching_actions'])} | {missing} |")
    lines += ['', '## 제어 흐름에서 확인한 점', '',
        '- 379740은 요청 ID를 순회 검색한다. 없는 번호도 시계는 0으로 되돌리며 기존 선택 포인터는 남긴다.',
        '- 37a3c0은 60/FPS만큼 시계를 증가시킨다. 현재 시각이 [요청 시점, 요청 시점+증분)일 때 가상 Execute를 호출한다.',
        '- 현재 표의 repeat 값은 전부 0이다. 오프라인 모델은 양의 일정 FPS와 비반복 요청만 지원한다.',
        '- 상태 3은 요청표 3을 선택한 직후 4 선택으로 이어진다. 요청표 3을 지속 상태로 취급하면 원본과 달라진다.',
        '- 상태 25/26/27/28은 각각 요청표 36/37/38/39로 이어진다. 375980의 결과 분기에서 사용된다.',
        '- 36은 몸체 49→120시점 몸체 3과 음성13, 37은 몸체53/말0, 38은 몸체52다. 승리·낙마 등 화면상 의미는 아직 확정하지 않았다.',
        '- 37a1b0은 요청 시계 외에 37a4e0의 일기토 위치 적분과 카메라도 갱신한다. 전체 매니저 갱신을 필드용 프록시 함수로 사용하면 안 된다.',
        '- 클립 root motion 억제 여부, 실제 종료 판정과 Scene 소유권은 별도 조사 대상이다.', '',
        '전체 값·원본 RVA·코드 SHA256·30/60/120 FPS 예제는 motion_timing_analysis.json에 있다.',
        '29개 상태의 분기 진입 주소는 tables.state_dispatch에 있으며, 동작 의미의 확정표가 아니다.', '']
    return '\n'.join(lines)


def main():
    root = Path(__file__).resolve().parents[1]
    report = build(root)
    write(root/'analysis/motion_timing_analysis.json', report)
    (root/'analysis/MOTION_TIMELINES_KO.md').write_text(render_tables(report),encoding='utf-8')
    t = report['tables']
    print(json.dumps(dict(status='PASS',timelines=t['descriptor_count'], requests=t['request_inventory_count'],
                          kinds=t['request_kind_counts'], unreferenced=t['unreferenced_request_rvas'])))


if __name__ == '__main__':
    main()
