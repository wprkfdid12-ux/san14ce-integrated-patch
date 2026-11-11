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
    0xda0be0,
    0x19c2f0,0xdc2ae0,0x331780,0x92f130,0x92f440,0x932940,0x932590,0x37f520,0x380d80,0x17adf0,0x320d30,0x1c8cc0,0x19f800,0x19ff30,0x871600,
    0x1c0940,0x1c18f0,0x1c6880,0x1c9600,0x1cc740,0x1c8390,0x1cad10,
    0x3753b0,0x375430,0x375240,0x375700,0x374eb0,0x374ff0,0x374c10,0x375a50,
    0x1cccc0,0x1c84e0,0x1caee0,0x400530,0x1cefb0,0x1c3420,0x1c35b0,
    0x1c99b0,0x37db70,0xbcbad0,0xbcf3b0,0xbd3f70,0x110d0,0xdb24e0,
    0xf0dd30,0xf0de50,0xe52df0,
    0x16b180,0xe55ad0,0x2aaf30,0x16bb00,0x16b940,0x16c740,0x16e4c0,
    0x173200,0x172e70,0x17b680,0x1a52e0,0x406e70,0x407f80,0x408b00,0x403580,
    0xea80,0x1c7230,0x11500,0x406150,0x407b70,0x1cb940,
    0xdb55b0,0xdaf2c0,0xbdab10,0x1c8240,0x1c8460,0x1c7fe0,0x1c8160,0x1c9f70,0x1ca4f0,
]
PATCHES = [(0x44d668,0x1cccc0),(0x44d67a,0x1c84e0),(0x1cb09e,0x1caee0),(0x14cb41,0x400530),
           (0x1c82bf,0x1c8460),(0x1c82d1,0x1c8460),(0x1c82ff,0x1c8460),
           (0x1c831a,0x1c8460),(0x1c8335,0x1c7fe0),
           (0x1c826a,0x1c8460),(0x1c827c,0x1c8460),(0x1c828e,0x1c8460),(0x1c82a0,0x1c8460),
           (0xe5368c,0xf0dd30),(0xe539cd,0xf0de50),
           (0x172ef4,0x173200),(0x173015,0x1a52e0),
           (0x403754,0x406e70),(0x403780,0x406e70),(0x403864,0x406e70),(0x408e3f,0x407f80),
           (0x16b86c,0x16b180),(0xe50362,0xe55ad0),(0x172f0f,0x173200),(0x408eb0,0x406150),(0x3a6ef3,0x407b70),(0x13a3408,0x1cb940),(0x1c76f5,0x37f520),(0x1c2f2c,0x37f520),(0x25d6a0,0x320d30),(0x13a3428,0x1c8cc0),(0x139bd28,0x19f800),(0x139ba98,0x19c2f0),(0x37dce5,0x937a00),(0x37ddda,0x937a00),(0x37dd4f,0xda0be0),(0x37ddf1,0xbd3b80)]
LEAVES = [(0xbd3b80,6),(0x937a00,23),(0x2435d0,29),(0x375de0,6),(0x375e00,8),(0x375bb0,9),(0x376930,11),(0xdb27e0,81),(0x37c9a0,15),(0xbcfb10,6),(0x5726b0,63)]
VTABLES = [(0x13a3600+o,t) for o,t in [
    (0,0x1c23c0),(8,0x1c3df0),(0x18,0x1cad10),(0x20,0x1cc740),
    (0x28,0x1c8390),(0x30,0x1cefb0),(0x38,0x1c9600),(0x40,0x1c9120),
    (0x48,0x1c6880),(0x50,0x8ca1c0),(0x60,0x1c3420),(0x68,0x1c35b0),(0x70,0x1c99b0)]]
VTABLES += [(0x16ccfc8+0x68,0xdc2ae0),(0x16ccfc8+0x80,0xdb24e0),(0x16ccfc8+0x90,0xdb27e0),(0x1694578+8,0xb471e0)]
VTABLES += [(0x16ccfc8+0x78,0xdc2fb0),(0x16ccfc8+0xf0,0xdaf2c0),(0x16ccfc8+0x418,0xdb55b0)]

VTABLES += [(0x13a3610,0x1c6b60),(0x13a3658,0x1c71e0),(0x13a3428,0x1c8cc0),(0x139bd28,0x19f800),(0x139ba98,0x19c2f0)]
VTABLES += [(0x13a33e8+0x28,0x1c8240),(0x13a3408,0x1cb940)]

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
    if sha(root/'inputs/SAN14CE_KO.exe')!='782cf229a56cdde2b95399e0e7536740b3f5e96bea702679365e4dad061abdce':
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
    rva,size=0xdc2fb0,109
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
        if rva==0xdb27e0: code=code[:32]
        guards.append((rva,code))
    # Entire 88-byte color request leaf, including its tail-jump to 379550.
    # Two guards end at reviewed instruction boundaries, below the 64B cap.
    rva,size=0x1c9b70,0x58
    (evidence/f'rva_{rva:08x}.asm.txt').write_text(tool.disassemble(rva,size))
    code=pe.data[pe.rva_to_offset(rva):pe.rva_to_offset(rva)+size]
    assert code[-1]==0xc3 and code[0x52:0x57]==bytes.fromhex('e9 f9 14 1b 00')
    guards.extend([(rva,code[:0x3d]),(rva+0x3d,code[0x3d:])])
    proof.append({'rva':hex(rva),'extent':'reviewed complete color-request leaf',
                  'ranges':[{'start':hex(rva),'end':hex(rva+size),'sha256':hashlib.sha256(code).hexdigest()}]})
    # Every branch/argument in the mode-dispatched draw is reviewed and guarded,
    # including the shared call whose argument is a ship/siege in other modes.
    dis=tool.disassemble(0x1c8240)
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
    assert chunk_start+len(chunk)==0x1c834a
    # Entire reviewed sound-call argument setup and failure branch, plus the
    # field interpolation action hint. Split only at disassembled boundaries.
    for start,end in [(0xe5361c,0xe536a7),(0xe53924,0xe539e8),(0x1cb1a0,0x1cb21e),
                      (0x172ee7,0x172ef9),(0x173004,0x17301a),
                      (0x403745,0x403761),(0x403771,0x40378d),(0x40385b,0x403869),(0x408dc0,0x408e54),
                      (0x17b80b,0x17b82a),(0x3700d0,0x3700d9),(0x11515,0x11597),
                      (0x16b837,0x16b871),(0xe502ac,0xe50370),(0x172f02,0x172f14),(0x408e54,0x408ebd),(0x3a6ee0,0x3a6ef8),(0x1cccf0,0x1cccff),(0x1c363c,0x1c3667),(0x407b70,0x407be5),(0x3804e0,0x380558),(0x37c18d,0x37c1be),(0x1c76db,0x1c7706),(0x1c2f12,0x1c2f41),(0x25d696,0x25d6a5),(0x1abf24,0x1abf8f),(0x1cc87b,0x1cc8d9),(0x2e36b8,0x2e36c8)]:
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
        if site in (0x13a3408,0x13a3428,0x139bd28,0x139ba98): continue  # relocated pointer is checked by VTABLES
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
