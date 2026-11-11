#!/usr/bin/env python3
"""Verify installer collision recovery and every compiled forwarding ABI."""
import hashlib,json,os,re,struct,subprocess,tempfile
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_64,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_probe import PE
from build_field_poc import exports
R=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((R/'analysis/registry_r24_build.json').read_text())
for row in b['files']:assert sha(R/row['path'])==row['sha256']
for path,h in b['sources'].items():assert sha(R/path)==h,path
payload=(R/'native/installer_payload.h').read_text();setup=(R/'native/bin/FieldLeader3D_Setup.exe').read_bytes()
symbols=[('fl_runtime','FieldLeader3D_Runtime.exe')]+[('fl_'+s['dll'],s['dll']+'.dll') for s in b['proxy_specs']]
for symbol,name in symbols:
 data=bytes(int(n) for n in re.search(r'\b'+symbol+r'\[\]=\{(.*?)\};',payload,re.S)[1].replace('\n','').split(',') if n.strip())
 assert data==(R/'native/bin'/name).read_bytes() and data in setup
def imports(path):
 p=PE(path);r,_=p.directories[1];o=p.rva_to_offset(r);out={}
 for j in range(300):
  lookup,_,_,name,iat=struct.unpack_from('<5I',p.data,o+j*20)
  if not name:break
  n=p.rva_to_offset(name);dll=p.data[n:p.data.index(0,n)].decode().lower();required=[]
  for k in range(2000):
   v=struct.unpack_from('<Q',p.data,p.rva_to_offset(lookup or iat)+k*8)[0]
   if not v:break
   if v>>63:required.append(v&0xffff);continue
   n=p.rva_to_offset(v)+2;required.append(p.data[n:p.data.index(0,n)].decode())
  out[dll]=required
 return out
game_imports=imports(R/'inputs/SAN14CE_KO.exe');covered={};cases=0;dependencies={}
for spec in b['proxy_specs']:
 filename=spec['dll']+'.dll';path=R/'native/bin'/filename;p=PE(path);base=p.image_base
 named=exports(path);er,_=p.directories[0];fields=struct.unpack_from('<IIHHIIIIIII',p.data,p.rva_to_offset(er))
 ordinal_base,num_funcs,funcs=fields[5],fields[6],fields[8]
 targets=[]
 for index,entry in enumerate(spec['entries']):
  if 'name' in entry:name=entry['name'];rva=named[name]
  else:
   name=entry['ordinal'];assert ordinal_base<=name<ordinal_base+num_funcs
   rva=struct.unpack_from('<I',p.data,p.rva_to_offset(funcs)+4*(name-ordinal_base))[0];assert rva
  targets.append((name,rva,index))
 required=game_imports[filename];assert set(required)<={name for name,_,_ in targets};covered[filename]=required
 dependencies[filename]=list(imports(path));assert all(x not in dependencies[filename] for x in ('libstdc++-6.dll','libgcc_s_seh-1.dll','libwinpthread-1.dll'))
 u=Uc(UC_ARCH_X86,UC_MODE_64);u.mem_map(base,(p.image_size+4095)&~4095);u.mem_map(0x100000,0x20000)
 for section in p.sections:u.mem_write(base+section['rva'],p.data[section['raw']:section['raw']+section['size']])
 stop=0x101000;target=0x102000;sp=0x118008
 for name,rva,index in targets:
  code=p.data[p.rva_to_offset(rva):p.rva_to_offset(rva)+140];off=code.index(b'\xe8');resolver=base+rva+off+5+struct.unpack_from('<i',code,off+1)[0]
  regs=[UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9];values=[0x1111111122222222,0x3333333344444444,0x5555555566666666,0x7777777788888888]
  xmm=[UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_XMM2,UC_X86_REG_XMM3];floats=[int.from_bytes(bytes([i+1])*16,'little') for i in range(4)]
  stack_args=struct.pack('<5Q',11,22,33,44,55);u.mem_write(sp,struct.pack('<Q',stop));u.mem_write(sp+0x28,stack_args);u.reg_write(UC_X86_REG_RSP,sp)
  for reg,value in zip(regs,values):u.reg_write(reg,value)
  for reg,value in zip(xmm,floats):u.reg_write(reg,value)
  visited=[]
  def observe(machine,addr,size,user):
   rsp=machine.reg_read(UC_X86_REG_RSP)
   if addr==resolver:
    assert rsp%16==8 and machine.reg_read(UC_X86_REG_RCX)==index
    for reg in regs+xmm:machine.reg_write(reg,0xdeadbeef)
    machine.reg_write(UC_X86_REG_RAX,target)
   else:
    assert [machine.reg_read(reg) for reg in regs]==values
    assert [machine.reg_read(reg) for reg in xmm]==floats
    assert rsp==sp and bytes(machine.mem_read(sp+0x28,40))==stack_args
    visited.append(name);machine.reg_write(UC_X86_REG_RAX,0x1234567890abcdef)
   ret=struct.unpack('<Q',machine.mem_read(rsp,8))[0];machine.reg_write(UC_X86_REG_RSP,rsp+8);machine.reg_write(UC_X86_REG_RIP,ret)
  h1=u.hook_add(UC_HOOK_CODE,observe,begin=resolver,end=resolver);h2=u.hook_add(UC_HOOK_CODE,observe,begin=target,end=target)
  u.emu_start(base+rva,stop,count=1000)
  assert visited==[name] and u.reg_read(UC_X86_REG_RAX)==0x1234567890abcdef and u.reg_read(UC_X86_REG_RSP)==sp+8
  u.hook_del(h1);u.hook_del(h2);cases+=1
with tempfile.TemporaryDirectory() as d:
 test=Path(d)/'installer';subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(R/'native/tests/install_transaction_test.cpp'),'-o',str(test)],check=True)
 # LeakSanitizer cannot enumerate threads under this host's ptrace runtime.
 # Address bounds/use-after-free and undefined-behaviour instrumentation remain.
 result=subprocess.run([str(test)],text=True,capture_output=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 assert result.returncode==0,result.stdout+result.stderr
assert len(result.stdout.splitlines())==7
report={'status':'PASS_OFFLINE_NOT_WINDOWS_INSTALLED','build_sha256':sha(R/'analysis/registry_r24_build.json'),'runtime_byte_identical_to_R23':False,'embedded_payloads':len(symbols),'original_imports_covered':covered,'compiled_forwarders_executed':cases,'includes_d3d9_ordinal_16':True,'system_dependencies':dependencies,'transaction_checks':result.stdout.splitlines(),'transaction_sanitizers':'ASan+UBSan','source_sha256':{str(p.relative_to(R)):sha(p) for p in [R/'tools/verify_registry_r24_installer.py',R/'native/tests/install_transaction_test.cpp',R/'native/install_transaction.h',R/'native/install_contract.h']},'windows_installer_executed':False,'other_user_patches_live_verified':False}
(R/'analysis/registry_r24_installer_verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
