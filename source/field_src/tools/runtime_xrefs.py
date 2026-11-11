#!/usr/bin/env python3
"""Find direct relative calls/jumps and validate them using objdump instruction boundaries."""
import argparse
import json
import re
import struct
from pathlib import Path
from runtime_disasm import RuntimeDisassembler


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('inputs',type=Path);ap.add_argument('targets',nargs='+',type=lambda v:int(v,0))
    ap.add_argument('--out-dir',type=Path,required=True)
    args=ap.parse_args();d=RuntimeDisassembler(args.inputs)
    raw=d.binary.read_bytes();targets=set(args.targets);cache={};results=[]
    args.out_dir.mkdir(parents=True,exist_ok=True)
    for m in re.finditer(b'[\xe8\xe9]',raw):
        off=m.start()
        if off+5>len(raw):continue
        src=d.text['rva']+off;target=src+5+struct.unpack_from('<i',raw,off+1)[0]
        if target not in targets:continue
        fn=d.pe.function(src);entry=fn['start_rva'] if fn else None
        row=dict(instruction_rva=src,target_rva=target,opcode=hex(raw[off]),pdata_fragment_start=entry,validated=False)
        if entry is not None:
            root=d.unwind_root((fn['start_rva'],fn['end_rva'],fn['unwind_rva']))[0]
            if root not in cache:
                cache[root]=d.disassemble(root)
                (args.out_dir/f'caller_{root:08x}.asm.txt').write_text(cache[root],encoding='utf-8')
            pattern=r'^\s*'+format(d.pe.image_base+src,'x')+r':\s+(?:[0-9a-f]{2}\s+)+\s*(?:call|jmp)\s+0x'+format(d.pe.image_base+target,'x')+r'\s*$'
            row['validated']=bool(re.search(pattern,cache[root],re.M))
            row['unwind_root_rva']=root
        else:row['note']='No pdata: caller start must be established separately'
        results.append(row)
    report=dict(schema='fieldleader3d.direct_xrefs.v1',runtime_sha256=d.pe.runtime_info['runtime_text_sha256'],targets=args.targets,xrefs=results)
    (args.out_dir/'xrefs.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(results))


if __name__=='__main__':main()
