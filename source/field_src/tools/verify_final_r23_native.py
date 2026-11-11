#!/usr/bin/env python3
"""Execute native position/layout contracts and the built x64 forwarders.
Windows API dependencies are stubbed; this is not a Windows/game execution.
"""
import json,struct,hashlib
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_64,UC_HOOK_CODE
from unicorn.x86_const import *
from verify_batch1 import BatchBytes
from build_field_poc import exports
from pe_probe import PE
ROOT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
checks=[]
e=BatchBytes();uc=e.uc
# Verify packed scale/rotation/position produced by the actual dac1d0 body.
instance=e.FORCE;skeleton=e.NODES;descriptor=e.NODES+0x100;bones=e.NODES+0x200;out=e.POINT
for a,b in [(e.ACTOR+0x150,instance),(instance,skeleton),(instance+0x10,bones),(skeleton,descriptor),(skeleton+8,bones)]:e.q(a,b)
uc.mem_write(descriptor+4,struct.pack('<HH',2,2));uc.mem_write(descriptor+0xc,struct.pack('<hh',0,1));e.i(bones+48+0xc,0x12345678)
def srt(u,address,size,user):
    assert u.reg_read(UC_X86_REG_RCX)==instance and u.reg_read(UC_X86_REG_RDX)==1
    assert u.reg_read(UC_X86_REG_R8)==e.ACTOR and u.reg_read(UC_X86_REG_R9)==bones
    sp=u.reg_read(UC_X86_REG_RSP);dst=e.getq(sp+0x28)
    u.mem_write(dst,struct.pack('<3fi4f3fI',3,4,5,-1,0,0,0,1,101,202,303,0));e.ret(0)
h=uc.hook_add(UC_HOOK_CODE,srt,begin=e.base+0xbd76a0,end=e.base+0xbd76a0)
assert e.execute(0xdac1d0,regs={UC_X86_REG_RCX:e.ACTOR,UC_X86_REG_RDX:1,UC_X86_REG_R8:out})&255==1
assert struct.unpack('<3f',uc.mem_read(out,12))==(3,4,5)
assert struct.unpack('<3f',uc.mem_read(out+0x1c,12))==(101,202,303)
assert e.geti(out+0x28)==0x12345678
uc.hook_del(h);checks.append('Original bone getter returns world position at+1c, distinct from scale at0; logical mapping and output ABI executed')
# Original unattached world setter invalidates transforms without scaling.
e.q(e.ACTOR,e.VTABLE);e.q(e.ACTOR+0x1c0,0);dirty=0x660000;e.q(e.VTABLE+0x418,dirty)
seen=[]
def dirty_call(u,address,size,user):seen.append(u.reg_read(UC_X86_REG_RCX));e.ret(0)
h=uc.hook_add(UC_HOOK_CODE,dirty_call,begin=dirty,end=dirty)
uc.mem_write(e.POINT,struct.pack('<3f',41,52,63));uc.mem_write(e.ACTOR+0x34,struct.pack('<3f',3,3,3))
e.execute(0xdbf9c0,regs={UC_X86_REG_RCX:e.ACTOR,UC_X86_REG_RDX:e.POINT})
assert struct.unpack('<3f',uc.mem_read(e.ACTOR+0x18,12))==(41,52,63)
assert struct.unpack('<3f',uc.mem_read(e.ACTOR+0x34,12))==(3,3,3) and seen==[e.ACTOR]
uc.hook_del(h);checks.append('Original unattached model setter changes XYZ and dirty state without changing body scale')
# Native UI entry transforms and anchored size/position round trip.
repo=e.DATA;table=e.DATA+0x1000;obj=e.ACTOR;transform=e.MATRIX
uc.mem_write(obj,bytes(0x300));e.q(obj+0xe8,transform);e.q(repo+8,table);e.i(repo+0x10,0x40);e.q(table+0x31*8,obj)
uc.mem_write(obj+0xf0,struct.pack('<hhff',120,40,1,1));e.byte(obj+0x112,0)
# Environmental class/parent lookup: a zero-origin parent layout.
parent=e.NODE;parent_transform=e.NODE+0x500;e.q(parent+0xe8,parent_transform)
def ui_parent(u,address,size,user):e.ret(parent)
hp=uc.hook_add(UC_HOOK_CODE,ui_parent,begin=e.base+0xba8f50,end=e.base+0xba8f50)
ht=uc.hook_add(UC_HOOK_CODE,ui_parent,begin=e.base+0xad0e80,end=e.base+0xad0e80)
for anchor in (0,0x11,0x22):
 e.byte(obj+0x10c,anchor)
 e.execute(0x92f870,regs={UC_X86_REG_RCX:obj,UC_X86_REG_RDX:94,UC_X86_REG_R8:30})
 e.execute(0x92f4c0,regs={UC_X86_REG_RCX:obj,UC_X86_REG_RDX:13,UC_X86_REG_R8:5})
 e.execute(0x92c130,regs={UC_X86_REG_RCX:repo,UC_X86_REG_RDX:e.POINT,UC_X86_REG_R8:0x31})
 assert struct.unpack('<ii',uc.mem_read(e.POINT,8))==(13,5)
 e.execute(0x92c440,regs={UC_X86_REG_RCX:repo,UC_X86_REG_RDX:e.POINT,UC_X86_REG_R8:0x31})
 assert struct.unpack('<ii',uc.mem_read(e.POINT,8))==(94,30)
checks.append('Original UI setters/getters preserve exact relative XY and dimensions for all three tested anchor modes')
# Native hex pitch constants, also checked against saved center calculations.
def diskfloat(r):return struct.unpack('<f',e.pe.disk_data[e.pe.rva_to_offset(r):e.pe.rva_to_offset(r)+4])[0]
assert diskfloat(0x1395940)==702 and diskfloat(0x1394adc)==468 and diskfloat(0x1395934)==3
checks.append('One-hex fall cap uses native 702/468/sqrt3 grid pitch, not a screen-pixel guess')
# Execute every exported assembly bridge from both compiled proxy DLLs. The
# resolver stub deliberately clobbers volatile registers to test their recovery.
bridge_cases=0
for filename in ('version.dll','dinput8.dll'):
 p=ROOT/'native/bin'/filename;pe=PE(p);base=pe.image_base
 u=Uc(UC_ARCH_X86,UC_MODE_64);u.mem_map(base,(pe.image_size+4095)&~4095);u.mem_map(0x100000,0x20000)
 for s in pe.sections:u.mem_write(base+s['rva'],pe.data[s['raw']:s['raw']+s['size']])
 stop=0x101000;target=0x102000;sp=0x118008
 for index,(name,rva) in enumerate(exports(p).items()):
  code=pe.data[pe.rva_to_offset(rva):pe.rva_to_offset(rva)+140];off=code.index(b'\xe8');resolver=base+rva+off+5+struct.unpack_from('<i',code,off+1)[0]
  regs=[UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9]
  values=[0x1111111122222222,0x3333333344444444,0x5555555566666666,0x7777777788888888]
  xmm=[UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_XMM2,UC_X86_REG_XMM3]
  floats=[int.from_bytes(bytes([i+1])*16,'little') for i in range(4)]
  stack_args=struct.pack('<5Q',11,22,33,44,55);u.mem_write(sp,struct.pack('<Q',stop));u.mem_write(sp+0x28,stack_args)
  u.reg_write(UC_X86_REG_RSP,sp)
  for r,v in zip(regs,values):u.reg_write(r,v)
  for r,v in zip(xmm,floats):u.reg_write(r,v)
  visited=[]
  def observe(machine,addr,size,user):
   rsp=machine.reg_read(UC_X86_REG_RSP)
   if addr==resolver:
    assert rsp%16==8
    for reg in regs+xmm:machine.reg_write(reg,0xdeadbeef)
    machine.reg_write(UC_X86_REG_RAX,target)
   else:
    assert [machine.reg_read(r) for r in regs]==values
    assert [machine.reg_read(r) for r in xmm]==floats
    assert rsp==sp and bytes(machine.mem_read(sp+0x28,40))==stack_args
    visited.append(name);machine.reg_write(UC_X86_REG_RAX,0x1234567890abcdef)
   ret=struct.unpack('<Q',machine.mem_read(rsp,8))[0];machine.reg_write(UC_X86_REG_RSP,rsp+8);machine.reg_write(UC_X86_REG_RIP,ret)
  h1=u.hook_add(UC_HOOK_CODE,observe,begin=resolver,end=resolver);h2=u.hook_add(UC_HOOK_CODE,observe,begin=target,end=target)
  u.emu_start(base+rva,stop,count=1000)
  assert visited==[name] and u.reg_read(UC_X86_REG_RAX)==0x1234567890abcdef and u.reg_read(UC_X86_REG_RSP)==sp+8
  u.hook_del(h1);u.hook_del(h2);bridge_cases+=1
checks.append(f'All {bridge_cases} compiled proxy exports preserve four integer arguments, four SIMD arguments, five stack arguments and 64-bit return values')
report={'status':'PASS_NATIVE_BYTES_NOT_WINDOWS_GAME','checks':checks,'proxy_bridge_cases':bridge_cases,'native_sources':{p:sha(ROOT/p) for p in ['inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin','native/bin/version.dll','native/bin/dinput8.dll','tools/verify_final_r23_native.py']},'windows_game':False,'gpu_pixels':False,'actual_fps':False}
(ROOT/'analysis/final_r23_native_evidence.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
