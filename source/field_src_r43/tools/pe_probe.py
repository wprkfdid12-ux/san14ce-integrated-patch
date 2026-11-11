#!/usr/bin/env python3
"""Read-only PE64/RTTI evidence collector. No game calls, hooks or binary writes.

RIP-relative matches are byte-pattern candidates until disassembly is checked.
All addresses are RVAs or preferred-image VAs, never live process addresses.
"""
from __future__ import annotations
import argparse
import bisect
import hashlib
import json
import re
import struct
from pathlib import Path


class PE:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.disk_data = self.data
        self.runtime_info = None
        if self.data[:2] != b'MZ':
            raise ValueError('Not MZ')
        pe = self.u32(0x3c)
        if self.data[pe:pe+4] != b'PE\0\0':
            raise ValueError('Not PE')
        self.machine, nsec, self.timestamp = self.unpack('<HHI', pe+4)
        optsize = self.u16(pe+20)
        opt = pe+24
        if self.machine != 0x8664 or self.u16(opt) != 0x20b or optsize < 112:
            raise ValueError('Expected x64 PE32+')
        self.image_base = self.u64(opt+24)
        self.entry_rva = self.u32(opt+16)
        self.image_size = self.u32(opt+56)
        self.header_size = self.u32(opt+60)
        ndirs = min(self.u32(opt+108), 16, (optsize-112)//8)
        self.directories = [self.unpack('<II', opt+112+i*8) for i in range(ndirs)]
        self.sections = []
        for i in range(nsec):
            off = opt+optsize+i*40
            name, vs, rva, size, raw = self.unpack('<8sIIII', off)
            self.need(raw, size)
            self.sections.append(dict(name=name.split(b'\0')[0].decode('ascii'),
                                      rva=rva, virtual_size=vs, size=size, raw=raw,
                                      flags=self.u32(off+36)))
        self.functions = []
        if len(self.directories) > 3:
            rva, size = self.directories[3]
            if rva and size:
                off = self.rva_to_offset(rva)
                self.need(off, size)
                for pos in range(off, off+size-11, 12):
                    start, end, unwind = self.unpack('<III', pos)
                    if start < end <= self.image_size and self.is_code(start):
                        self.functions.append((start, end, unwind))
        self.functions.sort()
        self.starts = [f[0] for f in self.functions]

    def need(self, offset, length):
        if offset is None or offset < 0 or length < 0 or offset+length > len(self.data):
            raise ValueError(f'Out-of-file read: {offset}, {length}')

    def unpack(self, fmt, offset):
        self.need(offset, struct.calcsize(fmt))
        return struct.unpack_from(fmt, self.data, offset)

    def u16(self, off): return self.unpack('<H', off)[0]
    def u32(self, off): return self.unpack('<I', off)[0]
    def u64(self, off): return self.unpack('<Q', off)[0]

    def rva_to_offset(self, rva):
        for s in self.sections:
            if s['rva'] <= rva < s['rva']+s['size']:
                return s['raw']+rva-s['rva']
        if 0 <= rva < self.header_size and rva < len(self.data):
            return rva
        return None

    def offset_to_rva(self, off):
        for s in self.sections:
            if s['raw'] <= off < s['raw']+s['size']:
                return s['rva']+off-s['raw']
        if 0 <= off < self.header_size:
            return off
        return None

    def is_code(self, rva):
        return any(s['flags'] & 0x20000000 and s['rva'] <= rva < s['rva']+s['size']
                   for s in self.sections)

    def function(self, rva):
        i = bisect.bisect_right(self.starts, rva)-1
        if i >= 0 and self.functions[i][0] <= rva < self.functions[i][1]:
            start, end, unwind = self.functions[i]
            return dict(start_rva=start, end_rva=end, unwind_rva=unwind)
        return None

    def location(self, offset):
        rva = self.offset_to_rva(offset)
        return dict(file_offset=offset, rva=rva,
                    preferred_va=None if rva is None else self.image_base+rva)

    def attach_runtime_text(self, binary_path, manifest_path):
        meta = json.loads(Path(manifest_path).read_text(encoding='utf-8-sig'))
        payload = Path(binary_path).read_bytes()
        section = next(s for s in self.sections if s['name'] == '.text')
        checks = {
            'schema': 'fieldleader3d.runtime_text.v1',
            'module_name': 'SAN14CE_KO.exe',
            'exe_sha256': hashlib.sha256(self.disk_data).hexdigest(),
            'exe_size': len(self.disk_data),
            'image_size': self.image_size,
            'pe_timestamp': self.timestamp,
            'section_name': '.text',
            'section_rva': section['rva'],
            'section_raw_offset': section['raw'],
            'section_virtual_size': section['virtual_size'],
            'capture_size': section['size'],
            'runtime_text_sha256': hashlib.sha256(payload).hexdigest(),
            'consecutive_reads_identical': True,
        }
        for key, value in checks.items():
            if meta.get(key) != value:
                raise ValueError('Runtime capture mismatch: '+key)
        if len(payload) != section['size']:
            raise ValueError('Runtime payload length mismatch')
        base = int(meta['module_base_hex'], 16)
        if base <= 0 or base % 4096:
            raise ValueError('Invalid runtime module base')
        start = section['raw']
        self.data = self.disk_data[:start]+payload+self.disk_data[start+len(payload):]
        self.runtime_info = meta

    def ascii_strings(self):
        for match in re.finditer(rb'[\x20-\x7e]{4,}\x00', self.data):
            yield dict(text=match.group()[:-1].decode('ascii'), **self.location(match.start()))

    def fixed_version_candidates(self):
        result = []
        for match in re.finditer(re.escape(struct.pack('<I', 0xfeef04bd)), self.data):
            off = match.start()
            if off+52 <= len(self.data) and self.u32(off+4) == 0x10000:
                nums = self.unpack('<IIII', off+8)
                versions = ['.'.join(map(str, (a>>16, a&65535, b>>16, b&65535)))
                            for a, b in (nums[:2], nums[2:])]
                result.append(dict(**self.location(off), file_version=versions[0],
                                   product_version=versions[1], status='fixed_info_signature_candidate'))
        return result

    def version_resources(self):
        if len(self.directories) < 3 or not self.directories[2][0]:
            return []
        root = self.rva_to_offset(self.directories[2][0])
        if root is None:
            return []

        def entries(relative):
            at = root+relative
            named, ids = self.unpack('<HH', at+12)
            if named+ids > 1024:
                raise ValueError('Unreasonable resource directory')
            return [self.unpack('<II', at+16+i*8) for i in range(named+ids)]

        def node(at, limit):
            length, value_length, kind = self.unpack('<HHH', at)
            if length < 6 or at+length > limit:
                raise ValueError('Truncated version resource')
            pos = at+6
            end = pos
            while end+2 <= at+length and self.u16(end):
                end += 2
            if end+2 > at+length:
                raise ValueError('Missing version resource key terminator')
            key = self.data[pos:end].decode('utf-16le')
            value_at = (end+2+3)&~3
            nbytes = value_length*2 if kind == 1 else value_length
            if value_at+nbytes > at+length:
                raise ValueError('Invalid version value length')
            value = self.data[value_at:value_at+nbytes]
            children = []
            pos = (value_at+nbytes+3)&~3
            while pos+6 <= at+length and self.u16(pos):
                child = node(pos, at+length)
                children.append(child)
                pos = (pos+self.u16(pos)+3)&~3
            return dict(key=key, value=value.decode('utf-16le').rstrip('\0') if kind==1 else value.hex(),
                        value_kind='string' if kind==1 else 'binary', children=children)

        found = []
        def walk(relative, path):
            if len(path)>3:
                raise ValueError('Version resource directory depth')
            for name, address in entries(relative):
                if not path and name != 16:
                    continue
                if address & 0x80000000:
                    walk(address&0x7fffffff, path+[name])
                else:
                    data_rva, size, codepage, _ = self.unpack('<IIII', root+address)
                    offset = self.rva_to_offset(data_rva)
                    self.need(offset,size)
                    found.append(dict(path=path+[name], codepage=codepage, rva=data_rva,
                                      tree=node(offset,offset+size)))
        walk(0, [])
        return found

    def hierarchy(self, rva):
        off = self.rva_to_offset(rva)
        if off is None:
            return []
        signature, attributes, count, array_rva = self.unpack('<IIII', off)
        if count > 128:
            return []
        array = self.rva_to_offset(array_rva)
        if array is None:
            return []
        result = []
        for i in range(count):
            descriptor_rva = self.u32(array+i*4)
            at = self.rva_to_offset(descriptor_rva)
            if at is None:
                continue
            td, contained, mdisp, pdisp, vdisp, attrs = self.unpack('<IIiiiI', at)
            name_at = self.rva_to_offset(td+16)
            if name_at is None:
                continue
            end = self.data.find(b'\0', name_at, name_at+512)
            if end < 0:
                continue
            name = self.data[name_at:end].decode('ascii', errors='replace')
            result.append(dict(name=name, type_descriptor_rva=td, contained_bases=contained,
                               member_displacement=mdisp, vbtable_displacement=pdisp,
                               displacement_inside_vbtable=vdisp, attributes=attrs))
        return result

    def rtti(self, strings):
        result = []
        ro = [s for s in self.sections if s['name'] in ('.rdata', '_RDATA')]
        for row in strings:
            td = row['rva']-16
            locators = []
            for section in ro:
                raw, size = section['raw'], section['size']
                region = self.data[raw:raw+size]
                for match in re.finditer(re.escape(struct.pack('<I', td)), region):
                    off = raw+match.start()-12
                    if off < raw or off+24 > raw+size:
                        continue
                    sig, displacement, cd, typ, hierarchy, self_rva = self.unpack('<IIIIII', off)
                    col_rva = self.offset_to_rva(off)
                    if sig != 1 or self_rva != col_rva or typ != td:
                        continue
                    if self.rva_to_offset(hierarchy) is None:
                        continue
                    tables = []
                    pointer = struct.pack('<Q', self.image_base+col_rva)
                    for s2 in ro:
                        region2 = self.data[s2['raw']:s2['raw']+s2['size']]
                        for m2 in re.finditer(re.escape(pointer), region2):
                            voff = s2['raw']+m2.start()+8
                            if voff % 8:
                                continue
                            methods = []
                            for i in range(256):
                                at = voff+i*8
                                if at+8 > s2['raw']+s2['size']:
                                    break
                                ptr = self.u64(at)
                                if not self.is_code(ptr-self.image_base):
                                    break
                                methods.append(dict(index=i, rva=ptr-self.image_base,
                                                    function=self.function(ptr-self.image_base)))
                            if methods:
                                tables.append(dict(**self.location(voff), methods=methods))
                    locators.append(dict(**self.location(off), subobject_offset=displacement,
                                         cd_offset=cd, hierarchy_rva=hierarchy,
                                         hierarchy=self.hierarchy(hierarchy), vtables=tables))
            result.append(dict(name=row['text'], type_descriptor_rva=td, locators=locators))
        return result

    def rip_candidates(self, targets):
        """Fast LEA/MOV RIP-relative prefilter; NOT an instruction decoder."""
        targets = {int(k): v for k, v in targets.items()}
        result = []
        for section in self.sections:
            if section['name'] != '.text':
                continue
            raw, size = section['raw'], section['size']
            region = self.data[raw:raw+size]
            for match in re.finditer(rb'[\x40-\x4f][\x8b\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]', region):
                off = raw+match.start()
                if off+7 > raw+size:
                    continue
                rva = self.offset_to_rva(off)
                dest = rva+7+self.unpack('<i', off+3)[0]
                if dest in targets:
                    result.append(dict(**self.location(off), target_rva=dest,
                                       target_names=targets[dest], bytes=self.data[off:off+7].hex(' '),
                                       function=self.function(rva),
                                       status='raw_pattern_candidate_requires_disassembly'))
        return result


RTTI_FILTER = re.compile(r'^\.\?AV(?:CUnitArmy@@|CUnitDuel@@|CDuelModel@@|CUnitManager@@|'
                         r'CUnitList@@|CInstancingModel@@|CArmyUnitData@@|CPersonData@@|'
                         r'CUnit@duel@@|CDuelManager@duel@@|CRequest[^@]+@timing_data@duel@@|'
                         r'CAnnihilateArmyUnitScene@@|CUniqueTacticsScene@@|IUnit[^@]*@@)')
ENGINE_FILTER = re.compile(r'^(?:kids::.*(?:Resource|Model|Motion|Animation|Sound|Voice|Scene|Rig)|'
                           r'(?:CharacterEditor|KIDSSystemResource|FieldEditor4|MaterialEditor|system|RRPreview)[.A-Za-z0-9]*)$')


def probe(path, runtime_text=None, runtime_manifest=None):
    pe = PE(path)
    if (runtime_text is None) != (runtime_manifest is None):
        raise ValueError('Both runtime text and manifest are required')
    if runtime_text is not None:
        pe.attach_runtime_text(runtime_text, runtime_manifest)
    all_strings = list(pe.ascii_strings())
    selected_types = [r for r in all_strings if RTTI_FILTER.search(r['text'])]
    selected_strings = [r for r in all_strings if ENGINE_FILTER.search(r['text'])
                        or 'kidsscndb' in r['text'] or 'kidssingletondb' in r['text']
                        or r['text'].lower().endswith('.rdb')]
    types = pe.rtti(selected_types)
    targets = {}
    def add(rva, name):
        if rva is not None: targets.setdefault(rva, []).append(name)
    for row in selected_strings:
        add(row['rva'], 'string:'+row['text'])
    for row in types:
        add(row['type_descriptor_rva'], 'type:'+row['name'])
        for col in row['locators']:
            for tab in col['vtables']:
                add(tab['rva'], 'vtable:'+row['name'])
    return dict(schema='fieldleader3d.pe_probe.v1',
                file=dict(name=pe.path.name, size=len(pe.disk_data), sha256=hashlib.sha256(pe.disk_data).hexdigest()),
                code_source='on_disk' if pe.runtime_info is None else 'runtime_text_in_memory_overlay',
                runtime_capture=pe.runtime_info,
                pe=dict(machine=pe.machine, timestamp=pe.timestamp, image_base=pe.image_base,
                        entry_rva=pe.entry_rva, size_of_image=pe.image_size,
                        sections=pe.sections, unwind_function_count=len(pe.functions),
                        version_candidates=pe.fixed_version_candidates(),
                        version_resources=pe.version_resources()),
                strings=selected_strings, rtti=types, rip_candidates=pe.rip_candidates(targets),
                constraints=['Static evidence only; no runtime verification.',
                             'RVAs apply only to this file hash until re-resolved.',
                             'Class names and vtables do not establish callable prototypes.',
                             'RIP byte matches must be verified at decoded instruction boundaries.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--runtime-text', type=Path)
    parser.add_argument('--runtime-manifest', type=Path)
    args = parser.parse_args()
    if args.output.resolve() in {p.resolve() for p in (args.exe, args.runtime_text, args.runtime_manifest) if p}:
        parser.error('Output must not overwrite input')
    report = probe(args.exe, args.runtime_text, args.runtime_manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(output=str(args.output), sha256=report['file']['sha256'],
                          rtti=len(report['rtti']), strings=len(report['strings']),
                          rip_candidates=len(report['rip_candidates'])), ensure_ascii=False))


if __name__ == '__main__':
    main()
