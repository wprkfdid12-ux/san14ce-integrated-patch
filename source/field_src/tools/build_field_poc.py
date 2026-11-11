#!/usr/bin/env python3
"""Build the native x64 first-field experiment; never run or patch the game."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
from runtime_disasm import RuntimeDisassembler
from pe_probe import PE

ENTRY_POINTS = [
    0xd9e8c0,
    0x19b020,0xdc0550,0x32f500,0x92cc80,0x92cf90,0x9303b0,0x930000,0x37d150,0x37e9b0,0x179e40,0x31ecc0,0x1c7940,0x19e500,0x19ec30,0x86f250,
    0x1bf5a0,0x1c0560,0x1c5500,0x1c82b0,0x1cb520,0x1c7010,0x1c99d0,
    0x373040,0x3730c0,0x372ed0,0x373390,0x372b40,0x372c80,0x3728a0,0x3736e0,
    0x1cbaa0,0x1c7160,0x1c9ba0,0x3fe200,0x1cdd90,0x1c2090,0x1c2220,
    0x1c8660,0x37b820,0xbc9390,0xbccc70,0xbd18a0,0x11170,0xdb0000,
    0xf0b8a0,0xf0b9c0,0xe50a10,
    0x16a1d0,0xe536f0,0x2a8d40,0x16ab50,0x16a990,0x16b790,0x16d510,
    0x172250,0x171ec0,0x17a6d0,0x1a3fe0,0x404b40,0x405c50,0x4067d0,0x401240,
    0xeb20,0x1c5eb0,0x115a0,0x403e20,0x405840,0x1ca600,
    0xdb30c0,0xdacde0,0xbd8370,0x1c6ec0,0x1c70e0,0x1c6c60,0x1c6de0,0x1c8c20,0x1c91a0,
]
PATCHES = [(0x44b428,0x1cbaa0),(0x44b43a,0x1c7160),(0x1c9d5e,0x1c9ba0),(0x14bb61,0x3fe200),
           (0x1c6f3f,0x1c70e0),(0x1c6f51,0x1c70e0),(0x1c6f7f,0x1c70e0),
           (0x1c6f9a,0x1c70e0),(0x1c6fb5,0x1c6c60),
           (0x1c6eea,0x1c70e0),(0x1c6efc,0x1c70e0),(0x1c6f0e,0x1c70e0),(0x1c6f20,0x1c70e0),
           (0xe512ac,0xf0b8a0),(0xe515ed,0xf0b9c0),
           (0x171f44,0x172250),(0x172065,0x1a3fe0),
           (0x401414,0x404b40),(0x401440,0x404b40),(0x401524,0x404b40),(0x406b0f,0x405c50),
           (0x16a8bc,0x16a1d0),(0xe4df82,0xe536f0),(0x171f5f,0x172250),(0x406b80,0x403e20),(0x3a3b53,0x405840),(0x13a03c8,0x1ca600),(0x1c6375,0x37d150),(0x1c1b9c,0x37d150),(0x25bd00,0x31ecc0),(0x13a03e8,0x1c7940),(0x1398ce8,0x19e500),(0x1398a58,0x19b020),(0x37b995,0x9352c0),(0x37ba8a,0x9352c0),(0x37b9ff,0xd9e8c0),(0x37baa1,0xbd14b0)]
LEAVES = [(0xbd14b0,6),(0x9352c0,23),(0x241c30,29),(0x373a80,6),(0x373aa0,8),(0x373840,9),(0x3745d0,11),(0xdb02f0,81),(0x37a650,15),(0xbcd3d0,6),(0x56fe50,63)]
VTABLES = [(0x13a05c0+o,t) for o,t in [
    (0,0x1c1030),(8,0x1c2a70),(0x18,0x1c99d0),(0x20,0x1cb520),
    (0x28,0x1c7010),(0x30,0x1cdd90),(0x38,0x1c82b0),(0x40,0x1c7da0),
    (0x48,0x1c5500),(0x50,0x92ad0),(0x60,0x1c2090),(0x68,0x1c2220),(0x70,0x1c8660)]]
VTABLES += [(0x16c9f80+0x68,0xdc0550),(0x16c9f80+0x80,0xdb0000),(0x16c9f80+0x90,0xdb02f0),(0x1691578+8,0xb44a80)]
VTABLES += [(0x16c9f80+0x78,0xdc0a20),(0x16c9f80+0xf0,0xdacde0),(0x16c9f80+0x418,0xdb30c0)]

VTABLES += [(0x13a05d0,0x1c57e0),(0x13a0618,0x1c5e60),(0x13a03e8,0x1c7940),(0x1398ce8,0x19e500),(0x1398a58,0x19b020)]
VTABLES += [(0x13a03a8+0x28,0x1c6ec0),(0x13a03c8,0x1ca600)]

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def array(b): return ','.join('0x%02x'%x for x in b)

def exports(path):
    """Parse this build's own PE export directory without executing the DLL."""
    b=Path(path).read_bytes(); nt=struct.unpack_from('<I',b,0x3c)[0]
    assert b[:2]==b'MZ' and b[nt:nt+4]==b'PE\0\0'
    assert struct.unpack_from('<H',b,nt+4)[0]==0x8664
    n=struct.unpack_from('<H',b,nt+6)[0]; opt=nt+24
    assert struct.unpack_from('<H',b,opt)[0]==0x20b
    sec=opt+struct.unpack_from('<H',b,nt+20)[0]
    sections=[]
    for i in range(n):
        o=sec+i*40; vs,rva,size,raw=struct.unpack_from('<IIII',b,o+8)
        sections.append((rva,max(vs,size),raw,size))
    def offset(rva):
        for start,length,raw,size in sections:
            if start<=rva<start+size: return raw+rva-start
        raise ValueError('RVA outside file-backed sections: %x'%rva)
    er,esize=struct.unpack_from('<II',b,opt+112); e=offset(er)
    fields=struct.unpack_from('<IIHHIIIIIII',b,e)
    count=fields[7]; funcs,names,ordinals=fields[8:11]
    out={}
    for i in range(count):
        name_rva=struct.unpack_from('<I',b,offset(names)+4*i)[0]
        start=offset(name_rva); name=b[start:b.index(0,start)].decode('ascii')
        ordinal=struct.unpack_from('<H',b,offset(ordinals)+2*i)[0]
        value=struct.unpack_from('<I',b,offset(funcs)+4*ordinal)[0]
        assert not er<=value<er+esize, 'Unexpected forwarded export'
        out[name]=value
    return out

def generate_guards(root):
    tool=RuntimeDisassembler(root/'inputs'); pe=tool.pe
    if sha(root/'inputs/SAN14CE_KO.exe')!='87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a':
        raise ValueError('Wrong original executable')
    evidence=root/'analysis/poc_native_evidence'; evidence.mkdir(exist_ok=True)
    guards=[]; proof=[]
    for rva in ENTRY_POINTS:
        ranges,reason=tool.ranges(rva)
        if reason.startswith('leaf') or ranges[0][0]!=rva: raise ValueError('Unreviewed entry extent')
        dis=tool.disassemble(rva); (evidence/f'rva_{rva:08x}.asm.txt').write_text(dis)
        byte_rows=[]
        for line in dis.splitlines():
            m=re.match(r'\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s)+)\s*\S',line)
            if m:
                pc=int(m[1],16)-pe.image_base
                if pc>=rva and sum(map(len,byte_rows))<20:
                    expected=rva+sum(map(len,byte_rows))
                    if pc!=expected: raise ValueError('Noncontiguous guard')
                    byte_rows.append(bytes.fromhex(m[2]))
        code=b''.join(byte_rows)
        if not 20<=len(code)<=64: raise ValueError('Incomplete entry guard')
        assert code==pe.data[pe.rva_to_offset(rva):pe.rva_to_offset(rva)+len(code)]
        guards.append((rva,code))
        proof.append({'rva':hex(rva),'extent':reason,'ranges':[{'start':hex(a),'end':hex(z),
            'sha256':hashlib.sha256(pe.data[pe.rva_to_offset(a):pe.rva_to_offset(a)+z-a]).hexdigest()} for a,z in ranges]})
    # Reviewed 109-byte world-scale setter ends in virtual dirty-notification
    # tailcalls, not RET. Its first 32 bytes end exactly at dbfeb0.
    rva,size=0xdc0a20,109
    dis=tool.disassemble(rva,size); (evidence/f'rva_{rva:08x}.asm.txt').write_text(dis)
    code=pe.data[pe.rva_to_offset(rva):pe.rva_to_offset(rva)+size]
    assert code[-7:]==bytes.fromhex('48 ff a0 18 04 00 00')
    guards.append((rva,code[:32]))
    proof.append({'rva':hex(rva),'extent':'reviewed complete tailcall leaf',
                  'ranges':[{'start':hex(rva),'end':hex(rva+size),'sha256':hashlib.sha256(code).hexdigest()}]})
    for rva,size in LEAVES:
        dis=tool.disassemble(rva,size); (evidence/f'rva_{rva:08x}.asm.txt').write_text(dis)
        code=pe.data[pe.rva_to_offset(rva):pe.rva_to_offset(rva)+size]
        if code[-1]!=0xc3: raise ValueError('Leaf extent is not the reviewed RET boundary')
        # Scale getter: all 81 bytes reviewed; first 32 end at an instruction
        # boundary (daf6cc movss [rdx],xmm0), within the fixed guard capacity.
        if rva==0xdb02f0: code=code[:32]
        guards.append((rva,code))
    # Entire 88-byte color request leaf, including its tail-jump to 379550.
    # Two guards end at reviewed instruction boundaries, below the 64B cap.
    rva,size=0x1c8820,0x58
    (evidence/f'rva_{rva:08x}.asm.txt').write_text(tool.disassemble(rva,size))
    code=pe.data[pe.rva_to_offset(rva):pe.rva_to_offset(rva)+size]
    assert code[-1]==0xc3 and code[0x52:0x57]==bytes.fromhex('e9 f9 14 1b 00')
    guards.extend([(rva,code[:0x3d]),(rva+0x3d,code[0x3d:])])
    proof.append({'rva':hex(rva),'extent':'reviewed complete color-request leaf',
                  'ranges':[{'start':hex(rva),'end':hex(rva+size),'sha256':hashlib.sha256(code).hexdigest()}]})
    # Every branch/argument in the mode-dispatched draw is reviewed and guarded,
    # including the shared call whose argument is a ship/siege in other modes.
    dis=tool.disassemble(0x1c6ec0)
    chunk=b''; chunk_start=0
    for line in dis.splitlines():
        m=re.match(r'\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s)+)\s*\S',line)
        if not m: continue
        pc=int(m[1],16)-pe.image_base; ins=bytes.fromhex(m[2])
        if chunk and len(chunk)+len(ins)>64:
            guards.append((chunk_start,chunk)); chunk=b''
        if not chunk: chunk_start=pc
        assert pc==chunk_start+len(chunk)
        chunk+=ins
    if chunk: guards.append((chunk_start,chunk))
    assert chunk_start+len(chunk)==0x1c6fca
    # Entire reviewed sound-call argument setup and failure branch, plus the
    # field interpolation action hint. Split only at disassembled boundaries.
    for start,end in [(0xe5123c,0xe512c7),(0xe51544,0xe51608),(0x1c9e60,0x1c9ede),
                      (0x171f37,0x171f49),(0x172054,0x17206a),
                      (0x401405,0x401421),(0x401431,0x40144d),(0x40151b,0x401529),(0x406a90,0x406b24),
                      (0x17a85b,0x17a87a),(0x36dcc0,0x36dcc9),(0x115b5,0x11637),
                      (0x16a887,0x16a8c1),(0xe4decc,0xe4df90),(0x171f52,0x171f64),(0x406b24,0x406b8d),(0x3a3b40,0x3a3b58),(0x1cbad0,0x1cbadf),(0x1c22ac,0x1c22d7),(0x405840,0x4058b5),(0x37e110,0x37e188),(0x379e3d,0x379e6e),(0x1c635b,0x1c6386),(0x1c1b82,0x1c1bb1),(0x25bcf6,0x25bd05),(0x1aac24,0x1aac8f),(0x1cb65b,0x1cb6b9),(0x2e12fd,0x2e130d)]:
        dis=tool.disassemble(start,end-start); chunk=b''; chunk_start=0
        for line in dis.splitlines():
            m=re.match(r'\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s)+)\s*\S',line)
            if not m: continue
            pc=int(m[1],16)-pe.image_base; ins=bytes.fromhex(m[2])
            if chunk and len(chunk)+len(ins)>64:
                guards.append((chunk_start,chunk)); chunk=b''
            if not chunk:chunk_start=pc
            assert pc==chunk_start+len(chunk)
            chunk+=ins
        if chunk:guards.append((chunk_start,chunk))
        assert chunk_start+len(chunk)==end
    for site,target in PATCHES:
        if site in (0x13a03c8,0x13a03e8,0x1398ce8,0x1398a58): continue  # relocated pointer is checked by VTABLES
        code=pe.data[pe.rva_to_offset(site):pe.rva_to_offset(site)+5]
        assert code[0]==0xe8 and site+5+struct.unpack_from('<i',code,1)[0]==target
        guards.append((site,code))
    for rva,target in VTABLES:
        assert struct.unpack_from('<Q',pe.disk_data,pe.rva_to_offset(rva))[0]==pe.image_base+target
    header=['#pragma once','#include <stdint.h>',
        'typedef struct FLGuard { uint32_t rva; unsigned length; unsigned char bytes[64]; } FLGuard;',
        'static const unsigned char fl_expected_exe_sha256[32]={'+array(bytes.fromhex(sha(root/'inputs/SAN14CE_KO.exe')))+'};',
        'static const FLGuard fl_guards[]={']
    header += ['{0x%x,%d,{%s}},'%(rva,len(code),array(code)) for rva,code in guards]
    header += ['};','#define FL_GUARD_COUNT (sizeof(fl_guards)/sizeof(fl_guards[0]))',
        'typedef struct FLVtableGuard { uint32_t rva,target; } FLVtableGuard;',
        'static const FLVtableGuard fl_vtable_guards[]={']
    header += ['{0x%x,0x%x},'%(rva,t) for rva,t in VTABLES]
    header += ['};','#define FL_VTABLE_GUARD_COUNT (sizeof(fl_vtable_guards)/sizeof(fl_vtable_guards[0]))']
    (root/'native/game_guards.h').write_text('\n'.join(header)+'\n')
    report={'schema':'fieldleader3d.native_contract.v1','status':'STATIC_CALL_CONTRACT_ONLY',
            'source_exe_sha256':sha(root/'inputs/SAN14CE_KO.exe'),
            'runtime_text_sha256':sha(root/'inputs/SAN14CE_KO.runtime_text.bin'),
            'functions':proof,'entry_guards':[{'rva':hex(r),'bytes_hex':b.hex()} for r,b in guards],
            'patches':[{'site':hex(s),'target':hex(t)} for s,t in PATCHES],
            'vtable_guards':[{'rva':hex(r),'target':hex(t)} for r,t in VTABLES],
            'native_game_execution':False,'in_game_rendering_verified':False}
    (root/'analysis/poc_native_contract.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--zig',required=True)
    args=ap.parse_args()
    root=Path(__file__).resolve().parents[1]; native=root/'native'; out=native/'bin'; out.mkdir(exist_ok=True)
    contract=generate_guards(root)
    common=[args.zig,'c++','-target','x86_64-windows-gnu','-O2','-g','-Wall','-Wextra','-Werror',
            '-fno-exceptions','-fno-rtti','-std=c++17']
    dll=out/'FieldLeader3D_PoC.dll'; exe=out/'FieldLeader3D_Runtime.exe'
    subprocess.run(common+['-shared',str(native/'field_poc.cpp'),'-o',str(dll)],check=True)
    ex=exports(dll)
    mapping={'REGISTRY':'FL_Registry','CONFIGURE':'FL_Configure','STATUS':'FL_Status','STATUS_SEQUENCE':'FL_StatusSequence','ROSTER_AUDIT':'FL_RosterAudit','STOP':'FL_RequestStop','UPDATE':'FL_HookUpdate',
             'DRAW':'FL_HookDraw','MANAGER_RELEASE':'FL_HookManagerRelease','RESOURCE_RELEASE':'FL_HookResourceRelease',
             'FIELD_GROUP_DRAW':'FL_HookFieldGroupDraw','FIELD_ACTION_DRAW':'FL_HookFieldActionDraw',
             'EVENT_SOUND_2D':'FL_HookEventSound2D','EVENT_SOUND_3D':'FL_HookEventSound3D',
             'TACTIC_EFFECT':'FL_HookTacticEffect','TACTIC_DAMAGE':'FL_HookTacticDamage',
             'AMBIENT_ENQUEUE':'FL_HookAmbientEnqueue','AMBIENT_CLEANUP':'FL_HookAmbientCleanup',
             'NORMAL_DAMAGE':'FL_HookNormalDamage','MOTION_EFFECT':'FL_HookMotionEffect','TACTIC_CHILD':'FL_HookTacticChild',
             'AMBIENT_START':'FL_HookAmbientStart','AMBIENT_VOLUME':'FL_HookAmbientVolume','ARMY_UPDATE':'FL_HookArmyUpdate',
             'GROUND_EFFECT':'FL_HookGroundEffect','NORMAL_FIELD_EFFECT':'FL_HookNormalFieldEffect',
             'DEFEAT_COMMIT':'FL_HookDefeatCommit','ARMY_DETACH':'FL_HookArmyDetach','INFO_ANCHOR':'FL_HookInfoAnchor','INFO_DESTROY':'FL_HookInfoDestroy','ANIMATION_RATE':'FL_HookAnimationRate','MOTION_EVALUATE':'FL_HookMotionEvaluate','MOTION_COMMIT':'FL_HookMotionCommit'}
    header=['#pragma once','static const unsigned char fl_dll_sha256[32]={'+array(bytes.fromhex(sha(dll)))+'};']
    header += ['#define FL_EXPORT_%s 0x%x'%(key,ex[value]) for key,value in mapping.items()]
    (native/'dll_build.h').write_text('\n'.join(header)+'\n')
    (native/'embedded_runtime.h').write_text('#pragma once\nstatic const unsigned char fl_embedded_dll[]={\n'+
        ',\n'.join(array(dll.read_bytes()[i:i+32]) for i in range(0,dll.stat().st_size,32))+'\n};\n')
    subprocess.run(common+[str(native/'poc_loader.cpp'),'-Wl,--subsystem,windows','-municode',
        '-lbcrypt','-luser32','-lgdi32','-lshell32','-o',str(exe)],check=True)
    build={'schema':'fieldleader3d.native_build.v1','status':'WINDOWS_X64_BUILT_NOT_GAME_TESTED',
           'revision':'R24','loader_only_revision':False,
           'compiler':subprocess.check_output([args.zig,'version'],text=True).strip(),
           'target':'x86_64-windows-gnu','commands_without_local_compiler_path':[c for c in common[1:]],
           'artifacts':[{'path':str(p.relative_to(root)),'bytes':p.stat().st_size,'sha256':sha(p)} for p in (dll,exe)],
           'source_files':{str(p.relative_to(root)):sha(p) for p in sorted(native.glob('*.cpp'))+sorted(native.glob('*.h')) if p.name!='installer_payload.h'},
           'dll_exports':ex,'runtime_guards':len(contract['entry_guards']),
           'native_game_execution':False,'in_game_rendering_verified':False}
    (root/'analysis/poc_native_build.json').write_text(json.dumps(build,indent=2)+'\n')
    print(json.dumps(build,indent=2))

if __name__=='__main__': main()
