#!/usr/bin/env python3
"""Reproduce the reviewed field submission path, without invoking native code.

This build-specific report distinguishes the confirmed callback handle from an
Array implementation candidate. A shared interface is not an instance identity.
Data/vtables are from disk; instructions are from the verified runtime capture.
"""
import hashlib
import json
import re
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler
from motion_analysis import TimingTables, hx, write
from factory_action_analysis import require, cstring, rip_lea, typeinfo
from lifecycle_analysis import validate_edge
from link_analysis import Graph


GUARDS = [
    (0x44a439, '48 8b 05 c8 6d 38 02', 'load original resource context global'),
    (0x44a440, '48 8b 98 98 00 00 00', 'callback resource handle is context+0x98'),
    (0x44a447, '48 85 db 74 55', 'null HANDLE skips this entire update/draw block'),
    (0x44a44c, '48 8b 5b 20', 'callback payload is handle+0x20'),
    (0x44a47d, '4c 8b c3 48 8b d6', 'same callback payload and engine context to manager draw'),
    (0x44a48f, '4c 8b c3 48 8b d6', 'same callback payload and engine context to model-pool draw'),
    (0x1c695d, 'f6 00 04 0f 84 9a 01 00 00', 'bit 0x04 absent skips all manager drawing'),
    (0x1c6966, '83 bd 98 04 00 00 00', 'manager flag +0x498 controls ordinary lists'),
    (0x1c6972, '0f 85 53 01 00 00', 'nonzero flag branches to duel list, not manager exit'),
    (0x1c699c, 'ff 50 28', 'ordinary army draw virtual +0x28'),
    (0x1c6acb, '48 8b 9d 78 03 00 00', 'duel active head at manager+0x378'),
    (0x1c6ae3, '4c 8b c7 48 8b d6 48 8b cb', 'forward callback to each duel actor'),
    (0x1c6aec, 'ff 50 28 48 8b 9b 90 01 00 00', 'duel draw then next node+0x190'),
    (0x3786b6, '83 79 18 00', 'draw wrapper requires ready field'),
    (0x3786bf, '48 8b 41 10 48 8b 48 20', 'wrapper handle to MotorCharacterSetting payload'),
    (0xc07380, '48 8b 41 30 c3', 'MotorCharacterSetting child MODEL HANDLE getter'),
    (0x3786d6, '4c 8b 03 48 8b d0 48 8b cb', 'callback is receiver; child handle is RDX'),
    (0x3786e4, '49 ff 60 08', 'tail dispatch to callback virtual +0x08'),
    (0x1c6681, '41 ff 50 08', 'ordinary noninstanced model uses the same virtual slot'),
    (0x1c6574, 'be 07 00 00 00', 'model pool checks seven output groups'),
    (0x1c6593, 'ff 50 08', 'nonempty model-pool group uses same callback slot'),
    (0x3fc370, '41 b8 ae 70 5d e6', 'resolve list resource hash e65d70ae'),
    (0x3fc37e, '48 89 85 98 00 00 00', 'store acquired list handle in context+0x98'),
    (0x14afc2, 'f0 44 0f b1 33', 'handle acquisition uses compare/exchange'),
    (0x14afd4, '25 00 00 ff 7f 81 e1 ff 7f 00 00', 'preserve two packed reference-count fields'),
    (0x14afdf, '05 00 00 01 00 ff c1 0b c8', 'acquisition increments both reference-count fields'),
    (0x3fd0e9, '41 be 1b 00 00 00', 'resource-context release loop has 27 slots'),
    (0x3fd16d, '48 8d 81 98 00 00 00', 'callback handle included in release-slot table'),
    (0x3fd1fc, '48 8b 41 38', 'release dispatch uses handle+0x38'),
    (0x3fd219, '4c 89 27', 'zero released resource-context slot'),
    (0x1cb2bf, '83 bb 48 07 00 00 02', 'army field actor cleanup when +0x748 equals 2'),
    (0x1cb2cb, '48 8b 9b e0 08 00 00', 'preserve next army actor before detach/cleanup'),
    (0x1cb2e9, 'ff 50 40', 'detach original army actor'),
    (0x1cb2f2, 'ff 50 18', 'cleanup original army actor before pool return'),
    (0x1c948c, 'ff 50 40', 'manager cleanup detaches each duel actor'),
    (0x1c9495, 'ff 50 18', 'manager cleanup releases each duel actor state'),
    (0xe4a5a, '48 8d 15 9b 3e 2a 01', 'register abstract List type with supplied name'),
    (0xeabdc, 'ba 88 34 68 3a', 'abstract List type ID'),
    (0xb48a4b, '3d 88 34 68 3a', 'script helper accepts List or derived type'),
    (0xb44db8, '85 ff b8 88 34 68 3a 0f 44 d8', 'Array exposes List as first base type'),
    (0xe613d0, 'b8 01 00 00 00 c3', 'Array declares one base type'),
    (0xb446ab, '48 8d 05 46 ce b4 00', 'Array constructor payload vtable'),
    (0xb446b6, '48 89 03 4c 89 63 08 48 89 7b 10', 'Array vtable, items, allocator'),
    (0xb446c1, '89 4b 18', 'Array capacity field'),
    (0xb446c8, '89 73 1c 89 53 20', 'Array count and growth fields'),
    (0xb446d8, '49 89 5d 20', 'constructed Array goes into resource handle+0x20'),
    (0xb43ec9, '41 b8 01 00 00 00', 'single append forwards count one'),
    (0xb440a0, 'e9 0b 00 00 00', 'Array bulk append forwards to the same implementation'),
    (0xb44148, '89 6f 1c', 'append publishes increased count after successful allocation'),
    (0xb4416b, '4c 89 04 c8', 'append stores supplied model HANDLE'),
    (0xb44184, '81 e2 ff 7f 00 00 81 e1 00 00 ff 7f ff c2 0b d1', 'Array append increments low ref field only'),
    (0xb44194, 'f0 41 0f b1 10', 'Array append reference compare/exchange'),
    (0xb442c4, '81 e1 ff 7f 00 00 81 e2 00 00 ff 7f ff c9 0b ca', 'Array clear decrements low ref field only'),
    (0xb442da, '85 c9 75 1b', 'release child object only when combined count word is zero'),
    (0xb442fd, '4c 89 2c 07', 'clear zeroes item pointers'),
    (0xb4431e, '44 89 6e 1c', 'clear sets Array count to zero'),
]

FUNCTIONS = [
    (0x44a020,None),(0x1c6940,None),(0x1cb280,None),
    (0x1c67f0,None),(0x3786b0,None),(0xc07380,5),
    (0x1c66a0,None),(0x1c68c0,None),(0x1c65c0,None),(0x1c6550,None),
    (0x3fb150,None),(0x3fbcc0,None),(0x3fc130,None),(0x14af80,None),
    (0x157e80,None),(0x14ae10,None),(0x3fd0a0,None),(0x3fbd10,None),
    (0x1c9380,None),(0x1c9530,None),(0x1c7580,None),(0x37d8b0,None),
    (0xe4290,None),(0xeab90,None),(0x14dc20,6),
    (0xb48a30,None),(0xb488b0,None),(0xb44da0,None),
    (0xc0adc0,14),(0xb44e90,6),(0xe613d0,6),(0xb44430,None),(0xb44a60,None),
    (0xb43ec0,None),(0xb440a0,5),(0xb440b0,None),(0xb44260,None),(0xb443f0,None),
]

CALL_EDGES = [
    (0x44a478,0x1cb280),(0x44a48a,0x1c6940),(0x44a49c,0x1c6550),
    (0x1c6838,0x3786b0),(0x3786cc,0xc07380),(0x1c6669,0xc07380),
    (0x3fc379,0x14af80),(0x14af9e,0xb39c10),(0x14afb3,0xb32b50),
    (0x14affb,0xb332b0),(0x1584b9,0x3fbcc0),(0x1584c8,0x3fc130),
    (0x3fbce4,0x3fb150),(0x14af21,0x3fd0a0),(0x14af26,0x3fbd10),
    (0x3fd20d,0xb33610),(0x3fd214,0x13210),
    (0x1cb2de,0x231720),(0x1cb2fc,0x1cf160),
    (0xe4a61,0xeab90),(0xb43ed4,0xb440b0),(0xb442e7,0xb2ff50),
    (0xb440dc,0xb3c530),(0xb441ab,0xb3c7b0),
]


def decode_contract(image):
    """Decode ABI fields only after checking the reviewed instruction sequence."""
    for rva,raw,_ in GUARDS:
        require(image,rva,raw)
    array = typeinfo(image,0x1691550,'Array',0x2eb79141,
        {0x28:0xc0adc0,0x30:0xb44e90,0x38:0xe613d0,0x40:0xb44da0,0x58:0xb44430,0x60:0xb44a60})
    payload_vtable = rip_lea(image,0xb446ab,'48 8d 05')
    if payload_vtable != 0x16914f8:
        raise ValueError('Array payload vtable changed')
    methods = {slot:image.unpack('<Q',payload_vtable+slot)[0]-image.pe.image_base
               for slot in (0x08,0x10,0x38,0x40)}
    if methods != {0x08:0xb43ec0,0x10:0xb440a0,0x38:0xb443f0,0x40:0xb44260}:
        raise ValueError('Array payload interface changed')
    if cstring(image,rip_lea(image,0xe4a5a,'48 8d 15')) != 'List':
        raise ValueError('Abstract List registration name changed')
    require(image,0x14dc20,'b8 88 34 68 3a c3')
    if image.unpack('<Q',0x1389940+0x30)[0]-image.pe.image_base != 0x14dc20:
        raise ValueError('Abstract List type getter changed')
    # Match real actor vtables rather than converting an indirect call to a
    # direct call without receiver provenance.
    actor_methods = []
    for name,vt,draw,update in [('CUnitArmy',0x13a0358,0x1c66a0,0x1c9de0),
                               ('CUnitDuel',0x13a0570,0x1c67f0,0x1cad00)]:
        if [image.unpack('<Q',vt+slot)[0]-image.pe.image_base for slot in (0x28,0x20)] != [draw,update]:
            raise ValueError('Actor vtable does not match reviewed draw/update path')
        actor_methods.append(dict(name=name,vtable_rva=hx(vt),draw_rva=hx(draw),update_rva=hx(update)))
    raw=image.read(0x44a439,7,True)
    global_rva=0x44a439+7+struct.unpack_from('<i',raw,3)[0]
    handle_offset=image.unpack('<I',0x44a443,code=True)[0]
    payload_offset=image.read(0x44a44f,1,True)[0]
    initialized_offset=image.unpack('<I',0x3fc381,code=True)[0]
    released_offset=image.unpack('<I',0x3fd170,code=True)[0]
    if not handle_offset==initialized_offset==released_offset==0x98:
        raise ValueError('Acquisition, draw and release do not refer to the same handle slot')
    return dict(resource_context_global_rva=hx(global_rva),handle_offset=handle_offset,
        payload_offset=payload_offset,resource_hash=hx(image.unpack('<I',0x3fc372,code=True)[0]),
        actor_methods=actor_methods,
        list_interface_type=hx(0x3a683488),
        array_candidate=dict(factory=array,payload_vtable_rva=hx(payload_vtable),
            methods={hx(k):hx(v) for k,v in methods.items()},
            candidate_for_field_callback=True,confirmed_field_instance=False))


def instruction_map(disassembly,base):
    rows={}
    for line in disassembly.splitlines():
        m=re.match(r'^\s*([0-9a-f]+):[ \t]+((?:[0-9a-f]{2}[ \t]+)+)(.*)$',line)
        if m:
            rva=int(m[1],16)-base
            raw=bytes.fromhex(m[2])
            if rva in rows and rows[rva]!=raw:
                raise ValueError('Overlapping incompatible instruction decodes')
            rows[rva]=raw
    return rows


def validate_guard_boundaries(guards,decoded):
    for site,raw,_ in guards:
        expected=bytes.fromhex(raw);cursor=site;found=b''
        while len(found)<len(expected):
            if cursor not in decoded:
                raise ValueError('Guard starts or continues inside an instruction: '+hx(cursor))
            part=decoded[cursor];found+=part;cursor+=len(part)
        if found!=expected:
            raise ValueError('Guard ends inside an instruction or differs from decode: '+hx(site))


def evidence(tool,image,root):
    folder=root/'analysis/render_context_evidence';folder.mkdir(exist_ok=True)
    records=[];cache={}
    for rva,size in FUNCTIONS:
        ranges,extent=tool.ranges(rva,size)
        if 'leaf candidate' in extent:
            raise ValueError('Unreviewed leaf extent')
        source=tool.disassemble(rva,size);cache[rva]=source
        path=folder/f'rva_{rva:08x}.asm.txt';path.write_text(source)
        records.append(dict(rva=hx(rva),path=path.relative_to(root).as_posix(),extent=extent,
            ranges=[dict(start_rva=hx(a),end_rva=hx(b),sha256=hashlib.sha256(image.read(a,b-a,True)).hexdigest())
                    for a,b in ranges]))
    decoded=instruction_map('\n'.join(cache.values()),tool.pe.image_base)
    validate_guard_boundaries(GUARDS,decoded)
    edges=[validate_edge(tool,a,b,cache) for a,b in CALL_EDGES]
    return records,[dict(rva=hx(a),bytes_hex=bytes.fromhex(b).hex(),interpretation=c) for a,b,c in GUARDS],edges


def build(root):
    root=Path(root);tool=RuntimeDisassembler(root/'inputs');image=TimingTables(tool.pe)
    contract=decode_contract(image)
    records,guards,edges=evidence(tool,image,root)
    graph=Graph(root)
    # Absence here means absence in this capture, not absence in the game.
    target=int(contract['resource_hash'],16)
    coverage=dict(captured_databases=len(graph.dbs),captured_objects=sum(d['count'] for d in graph.dbs.values()),
        field_list_object_sources=graph.source(target) if target in graph.objects else [],
        array_object_count=sum(v[0][1]['type']=='2eb79141' for v in graph.objects.values()),
        field_list_identity='UNRESOLVED_IN_CAPTURE',live_callback_vtable_observed=False)
    if coverage['field_list_object_sources']:
        # Newly added material requires review; never silently keep a stale
        # negative statement or guess the callback type from the Array candidate.
        raise ValueError('Field-list DB object is now available: review and extend this report')
    return dict(schema='fieldleader3d.render_context_analysis.v1',status='PARTIAL',
        analysis_tool_sha256=hashlib.sha256((root/'tools/render_context_analysis.py').read_bytes()).hexdigest(),
        source_exe_sha256=hashlib.sha256(tool.pe.disk_data).hexdigest(),
        runtime_text_sha256=tool.pe.runtime_info['runtime_text_sha256'],
        scope='Reviewed instructions and on-disk vtables; no game calls, emulation, patching, or live callback observations',
        contract=contract,database_coverage=coverage,code_evidence=records,instruction_guards=guards,verified_call_edges=edges,
        field_frame=dict(driver_rva=hx(0x44a020),
            ordered_calls=[dict(site_rva=hx(a),target_rva=hx(b)) for a,b in CALL_EDGES[:3]],
            ordering='unit update, unit draw, seven model-pool output groups',
            null_handle='skips the entire local update/draw block at 44a447',
            null_payload='not tested by driver at this load; receiving draw routines have their own guards',
            global_draw_bit=4,ordinary_list_disable_offset=hx(0x498),
            nonzero_manager_flag='ordinary lists skipped; duel head +0x378 still drawn if global bit is set',
            submission='RDX is child model HANDLE, not MotorCharacterSetting payload or model payload',
            proof_boundary='CPU-side callback submission is confirmed; downstream scene/renderer consumption and thread identity are unresolved'),
        resource_lifetime=dict(
            acquire='157e80 -> 3fbcc0 allocates context; 3fc130 -> 14af80 resolves e65d70ae and stores handle+0x98',
            retain='14af80 resolves through b39c10/b32b50, increments low/high packed count fields, then releases temporary resolver handle via b332b0',
            release='14ae10 -> 3fd0a0 includes +0x98 among 27 slots; routes via handle+0x38 to b33610 or direct 013210, then zeroes slot',
            context_free='14ae10 then 3fbd10 frees original resource context and clears global 27d1208',
            boundary='Application resource teardown is confirmed. Individual scenario/load transitions and frame-local list reset are not identified.'),
        actor_cleanup=dict(update_rva=hx(0x1cb280),manager_cleanup_rva=hx(0x1c9380),
            state_two='Army actor+0x748==2: save next+0x8e0; optional 231720; virtual+0x40 detach, +0x18 cleanup; 1cf160 returns actor to pool',
            duel_shutdown='Manager walks head+0x378, virtual+0x40/+0x18, next+0x190, then returns to pool cleanup 1c6de0',
            boundary='Visual actor cleanup and pool reuse do not establish final logical ArmyUnitData deletion or defeat cause'),
        array_semantics=dict(
            identity='Array type 2eb79141 derives from abstract List 3a683488. It is an implementation candidate, not the observed field-list instance.',
            fields={'items':hx(8),'allocator':hx(0x10),'capacity':hx(0x18),'count':hx(0x1c),'growth':hx(0x20),'lock':hx(0x24)},
            append='payload virtual+0x08 -> b43ec0 -> b440b0(count=1); lock, grow if needed, append HANDLE and increment low packed ref field for each nonnull item',
            duplicates='No equality search in append; repeated handles are retained and appended repeatedly. Null items occupy a slot without a retain.',
            allocation_failure='Growth failure leaves count/items untouched and returns false; original draw path does not inspect append result',
            clear='payload virtual+0x40 -> b44260; lock, decrement each nonnull low ref field; combined count word zero invokes b2ff50 and allocator; zero item and count; unlock',
            caution='A raw-pointer memcpy is not equivalent to append; clearing the global list from a mod would also remove original submissions.',
            limitation='Exact ownership behavior above is proven for Array only; no evidence yet binds global+0x98 to this vtable.'),
        next_steps=[
            'Resolve e65d70ae definition or observe its handle type/payload vtable, then connect the actual frame clear and renderer consumption path.',
            'Trace model/action hash bridge +d8/+e0 and the remaining current-equipment and generic body-selection dependencies.',
            'Separate actor pool reuse from accepted annihilation/final logical deletion and scenario/load transitions before any runtime integration.'],
        native_calls=False,mod_dll=None,installable=False,in_game_tests='NOT_RUN',next_input=None)


def markdown(report):
    return '''# R8 필드 출력 목록과 수명

기존 자료에서 필드 장수의 CPU 출력 경로와 목록 핸들의 확보·해제를 연결했다. 실제 게임에서 모델을 띄우거나 새 프록시를 등록한 결과는 아니다. 모든 주소는 해당 빌드의 RVA다.

| 단계 | 확인한 경로 | 의미 |
|---|---|---|
| 목록 핸들 확보 | `3fc130 → 14af80`, 객체 해시 `e65d70ae` | 전역 `27d1208` 객체의 `+98`에 참조 관리 핸들 저장 |
| 매 프레임 전달 | `44a020`, `+98 → handle+20` | 같은 payload를 부대 draw와 모델 풀 draw에 전달 |
| 필드 갱신 → 출력 | `1cb280 → 1c6940 → 1c6550` | 원본 호출 순서. 마지막 함수는 모델 풀의 7개 출력 그룹 검사 |
| 일기토 장수 draw | `1c6940 → CUnitDuel.vt+28 → 1c67f0 → 3786b0` | ready인 표시 대상 모델을 전달 |
| 전달 인자 | `c07380 → callback.vt+08` | `RCX=callback`, `RDX=자식 모델 핸들`. payload 포인터가 아님 |
| 목록 핸들 해제 | `14ae10 → 3fd0a0 → 3fbd10` | 27개 관리 슬롯 중 `+98`을 해제·0 처리한 뒤 전역 context 정리 |

`1c6940`의 전역 비트 `04`가 꺼지면 전체 draw를 건너뛴다. manager `+498`이 0이 아니면 일반 부대 목록을 건너뛰지만 일기토 목록 `+378`은 계속 처리한다. `44a020`에서 `+98` 핸들이 없으면 해당 갱신·출력 블록 전체를 건너뛴다. 핸들과 payload의 NULL 조건을 혼동하면 안 된다.

원본 일반 모델의 비인스턴싱 출력과 모델 풀 출력도 같은 `callback.vt+08`을 사용한다. 이 연결은 원본 출력 진입점을 특정한 것이며, 그 뒤 Scene 소비·스레드·독립 프록시 수명까지 입증하지 않는다.

## List 계열의 구체 구현 후보

원본 타입 등록에서 `List(3a683488)`와 파생 `Array(2eb79141)`를 확인했다. Array factory vtable은 `1691550`, 생성 함수는 `b44430`, payload vtable은 `16914f8`이다.

| Array 연산 | 함수 | 확인된 동작 |
|---|---|---|
| 한 모델 추가, vt+08 | `b43ec0 → b440b0` | 핸들 저장, 필요한 확장, lock, 낮은 참조 카운트 증가 |
| 개수 조회, vt+38 | `b443f0` | lock 상태에서 `+1c` 조회 |
| 목록 비우기, vt+40 | `b44260` | 각 항목의 참조 감소, 필요 시 객체 해제, 항목·개수 0 처리 |

Array 추가에는 중복 검색이 없다. 같은 핸들을 두 번 추가하면 두 항목과 두 참조가 된다. NULL 항목은 슬롯만 차지한다. 목록 핸들 확보 함수는 낮은/높은 참조 카운트를 모두 증가시키지만 Array 항목 추가는 낮은 쪽만 증가시킨다. 원본 관리 함수를 포인터 복사로 대체할 수 없다.

**Array가 실제 필드 목록이라는 연결은 미확인이다.** 수집한 DB 46개·객체 36,190개에는 `e65d70ae`의 정의와 Array 인스턴스가 없고, 실행 중 callback의 vtable도 수집되지 않았다. 현재 자료에 없다는 뜻이며 게임 자체에 없다는 뜻이 아니다. Array의 비우기 함수는 찾았지만, 실제 필드가 언제 어떤 구현으로 목록을 비우는지는 남아 있다.

## 부대 시각 객체의 정리

`1cb280`은 army actor `+748==2`이면 다음 노드를 먼저 확보하고, 부대 링크 정리 후 virtual `+40` detach와 `+18` cleanup을 호출한 뒤 `1cf160`으로 풀에 반환한다. `1c9380`은 일기토 actor 목록도 detach/cleanup한다. 이는 시각 객체 정리 근거이며 논리 부대 삭제·전멸 사유를 확정하는 근거가 아니다.

## 재현과 남은 작업

`tools/render_context_analysis.py`와 `tools/verify_render_context_analysis.py`가 원본 바이트, 명령 경계, actor/Array vtable, 직접 호출 및 기존 DB의 범위를 대조한다. 상세 결과는 `render_context_analysis.json`과 `render_context_verification.json`에 있다. 검증 도구도 원본 게임 함수를 실행하지 않는다.

다음은 `e65d70ae`의 실제 타입/프레임 초기화/렌더 소비 연결, `bc8740`의 모델·동작 bridge, 장착 외형 및 시나리오·로드 수명이다. R7까지 확정한 생성 함수와 ActionID 검색표를 처음부터 재조사하지 않는다. 지금 사용자에게 요청한 추가 실행·수집은 없다.
'''


def main():
    root=Path(__file__).resolve().parents[1];report=build(root)
    write(root/'analysis/render_context_analysis.json',report)
    (root/'analysis/FIELD_RENDER_CONTEXT_KO.md').write_text(markdown(report),encoding='utf-8')
    print(json.dumps(dict(status=report['status'],guards=len(report['instruction_guards']),
        calls=len(report['verified_call_edges']),functions=len(report['code_evidence']),
        field_callback_concrete_type='UNRESOLVED',native_calls=False)))


if __name__=='__main__':
    main()
