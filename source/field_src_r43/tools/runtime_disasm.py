#!/usr/bin/env python3
"""Disassemble verified runtime .text without rebuilding or modifying an EXE."""
import argparse
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path
from pe_probe import PE


class RuntimeDisassembler:
    def __init__(self, inputs):
        self.inputs=Path(inputs)
        self.pe=PE(self.inputs/'SAN14CE_KO.exe')
        self.binary=self.inputs/'SAN14CE_KO.runtime_text.bin'
        self.manifest=self.inputs/'runtime_manifest.json'
        self.pe.attach_runtime_text(self.binary,self.manifest)
        self.text=next(s for s in self.pe.sections if s['name']=='.text')
        self.groups=defaultdict(list)
        for entry in self.pe.functions:
            self.groups[self.unwind_root(entry)].append(entry)

    def unwind_root(self,entry):
        """Follow documented UNW_FLAG_CHAININFO; a pdata entry can be only a fragment."""
        seen=set()
        while True:
            if entry in seen:raise ValueError('Cyclic unwind chain')
            seen.add(entry)
            off=self.pe.rva_to_offset(entry[2])
            version_flags,prolog,count,frame=self.pe.unpack('<4B',off)
            if version_flags&7 not in (1,2):raise ValueError('Unsupported unwind version')
            flags=version_flags>>3
            if not flags&4:return entry
            if flags&3:raise ValueError('Incompatible chained unwind flags')
            entry=self.pe.unpack('<III',off+4+2*((count+1)&~1))

    def ranges(self,rva,size=None):
        if size is not None:return [(rva,rva+size)],'explicit extent'
        fn=self.pe.function(rva)
        if not fn:return [(rva,rva+256)],'leaf candidate: 256-byte window may include adjacent code'
        root=self.unwind_root((fn['start_rva'],fn['end_rva'],fn['unwind_rva']))
        ranges=[]
        for a,b,_ in sorted(self.groups[root]):
            if ranges and a==ranges[-1][1]:ranges[-1]=(ranges[-1][0],b)
            else:ranges.append((a,b))
        return ranges,'pdata fragments joined only through validated unwind chains'

    def disassemble(self,rva,size=None):
        ranges,reason=self.ranges(rva,size)
        return '\n'.join(self.disassemble_range(start,end,reason) for start,end in ranges)

    def disassemble_range(self,start,end,reason):
        if not (self.text['rva']<=start<end<=self.text['rva']+self.text['size']):
            raise ValueError('Disassembly request outside captured .text')
        # Preferred VA makes comparisons reproducible; relative control flow remains valid.
        # Absolute immediates may still contain live ASLR addresses and require separate interpretation.
        command=['objdump','-D','-w','-b','binary','-m','i386:x86-64','-M','intel',
                 '--adjust-vma='+hex(self.pe.image_base+self.text['rva']),
                 '--start-address='+hex(self.pe.image_base+start),
                 '--stop-address='+hex(self.pe.image_base+end),str(self.binary)]
        raw=subprocess.check_output(command,text=True)
        lines=['RVA range '+hex(start)+'..'+hex(end),
               'Extent: '+reason,
               'Code source SHA256 '+self.pe.runtime_info['runtime_text_sha256'],
               'Preferred VAs shown; no live process is accessed.']
        lines.extend(raw.splitlines())
        return '\n'.join(lines)+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs',type=Path)
    parser.add_argument('rvas',nargs='+',type=lambda s:int(s,0))
    parser.add_argument('--size',type=lambda s:int(s,0))
    parser.add_argument('--out-dir',required=True,type=Path)
    args=parser.parse_args(); tool=RuntimeDisassembler(args.inputs)
    args.out_dir.mkdir(parents=True,exist_ok=True)
    paths=[]
    for rva in args.rvas:
        path=args.out_dir/('rva_%08x.asm.txt'%rva)
        path.write_text(tool.disassemble(rva,args.size),encoding='utf-8');paths.append(str(path))
    print(json.dumps({'written':paths}))


if __name__=='__main__':main()
