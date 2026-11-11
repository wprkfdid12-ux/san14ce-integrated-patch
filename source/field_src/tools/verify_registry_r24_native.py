#!/usr/bin/env python3
"""Run the real R2 registry instructions and the compiled read-only adapter."""
import ctypes as C,hashlib,json,re,struct,subprocess,sys,tempfile
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from verify_all_appearances import NativeBytes
from pe_probe import PE
R=Path(__file__).resolve().parents[1]
DLL=R/'inputs/compat_r2/SAN14_IntegratedConvenience.dll'
assert hashlib.sha256(DLL.read_bytes()).hexdigest()=='d3b0886a6c89afb53f0cf283deee93ac7885dd9cc7364ebb99d0982d613f0a08'
e=NativeBytes();u=e.uc;p=PE(DLL);m=p.image_base;t=e.base+0x3000000
u.mem_map(m,(p.image_size+4095)&~4095);u.mem_map(t,0x3000)
for s in p.sections:u.mem_write(m+s['rva'],p.data[s['raw']:s['raw']+s['size']])
u.mem_write(t,bytes(u.mem_read(e.base+0x1a3e290,25*68)))
for i in range(25,126):u.mem_write(t+i*68,f'Synthetic/extension_{i}.g1m\0'.encode())
e.q(m+0x1ec360,e.base);e.q(m+0x1eca38,t);e.q(m+0x1c1360,e.base)
api={0x9a120:0x700000,0x9a1c0:0x701000,0x9a1a8:0x702000,0x9a170:0x703000}
for iat,target in api.items():e.q(m+iat,target)
def stub(u,a,n,user):
 regs=[u.reg_read(x) for x in [UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9]]
 if a==0x700000:e.ret(0xffffffffffffffff)
 elif a==0x701000:
  _,source,dest,size=regs;u.mem_write(dest,bytes(u.mem_read(source,size)));written=e.getq(u.reg_read(UC_X86_REG_RSP)+0x28)
  if written:e.q(written,size)
  e.ret(1)
 elif a==0x702000:e.i(regs[3],0x20);e.ret(1)
 else:e.ret(1)
for target in list(api.values())+[m+0x77120]:u.hook_add(UC_HOOK_CODE,stub,begin=target,end=target)
def run(r,rcx=0,rdx=0):
 e.q(e.STACK,e.STOP);u.reg_write(UC_X86_REG_RSP,e.STACK);u.reg_write(UC_X86_REG_RCX,rcx);u.reg_write(UC_X86_REG_RDX,rdx)
 u.emu_start(m+r,e.STOP,count=100000);assert u.reg_read(UC_X86_REG_RIP)==e.STOP
 return u.reg_read(UC_X86_REG_RAX)
# Native registry disabled; the log's still-active independent UI cleanup.
e.q(e.base+0x1398a08,m+0x4940)
with tempfile.TemporaryDirectory() as d:
 so=Path(d)/'bridge.so';subprocess.run(['g++','-shared','-fPIC','-O2','-std=c++17',str(R/'native/tests/registry_bridge_probe.cpp'),'-o',str(so)],check=True)
 lib=C.CDLL(str(so));Reader=C.CFUNCTYPE(C.c_bool,C.c_size_t,C.c_void_p,C.c_size_t)
 def read(a,out,size):
  try:C.memmove(out,bytes(u.mem_read(a,size)),size);return True
  except Exception:return False
 reader=Reader(read)
 lib.registry_installed.argtypes=[Reader,C.c_size_t,C.c_size_t]
 lib.registry_info.argtypes=[Reader,C.c_size_t,C.c_size_t];lib.registry_info.restype=C.c_size_t
 lib.registry_guard.argtypes=[C.c_size_t,C.c_size_t,C.c_uint,C.c_void_p,C.c_void_p,C.c_uint]
 lib.registry_resolve.argtypes=[Reader,C.c_size_t,C.c_size_t,C.c_size_t,C.c_uint,C.POINTER(C.c_int),C.POINTER(C.c_int)]
 assert lib.registry_installed(reader,e.base,m)==0
 assert lib.registry_info(reader,e.base,m)==m+0x4940
 # The exact wrapper observed in the supplied log preserves the destructor ABI.
 calls=[]
 def cleanup(u,a,n,user):
  calls.append((a,u.reg_read(UC_X86_REG_RCX),u.reg_read(UC_X86_REG_EDX)))
  e.ret(0xdeadbeef if a==m+0x37d30 else 0x123456789abcdef)
 for target in [m+0x37d30,e.base+0x19a640]:u.hook_add(UC_HOOK_CODE,cleanup,begin=target,end=target)
 assert run(0x4940,0x12345000,0x87654321)==0x123456789abcdef
 assert [v[0] for v in calls]==[m+0x37d30,e.base+0x19a640] and calls[-1][1:]==(0x12345000,0x87654321)
 original_byte=bytes(u.mem_read(m+0x4940,1));u.mem_write(m+0x4940,b'\x90');assert lib.registry_info(reader,e.base,m)==0;u.mem_write(m+0x4940,original_byte)
 assert run(0x3dee0)&255==1
 assert lib.registry_installed(reader,e.base,m)==1
 text=(R/'native/game_guards.h').read_text();code=text.split('static const FLGuard fl_guards[]={',1)[1].split('};',1)[0]
 guards=0
 for a,size,values in re.findall(r'\{(0x[0-9a-f]+),(\d+),\{([^}]+)\}\}',code):
  expected=bytes(int(x,16) for x in values.split(','));actual=bytes(u.mem_read(e.base+int(a,16),int(size)))
  assert lib.registry_guard(e.base,t,int(a,16),expected,actual,int(size)),a
  # No blanket masking: each byte outside the three relocated displacements rejects corruption.
  for i in range(len(actual)):
   corrupt=bytearray(actual);corrupt[i]^=1
   assert not lib.registry_guard(e.base,t,int(a,16),expected,bytes(corrupt),int(size))
  guards+=1
 # Real create wrapper versus our adapter for all IDs, with matching officer-name identity.
 controller=0x250000;binding=0x260000;catalog=0x270000;officer=723
 u.mem_write(e.PERSON,bytes(0x1f0));e.q(e.PERSON,e.base+0x14680a0);u.mem_write(e.PERSON+0x10,struct.pack('<H',officer));u.mem_write(e.PERSON+0x12,'test\0'.encode('utf-16le'))
 e.q(e.DATA+0x158+officer*8,e.PERSON);e.q(e.base+0x2783730,controller)
 e.q(m+0x1ec348,binding);e.q(m+0x1ec350,binding+40)
 e.i(binding,officer);u.mem_write(binding+8,'test\0'.encode('utf-16le'));e.q(binding+0x18,4);e.q(binding+0x20,7)
 e.q(m+0x1ec330,catalog);e.q(m+0x1ec338,catalog+101*40)
 for i in range(101):e.i(catalog+i*40,i+25)
 created=[]
 def wrappers(u,a,n,user):
  if a==m+0x4c500:e.ret(e.PERSON)
  elif a==m+0x45fc0:
   dest=u.reg_read(UC_X86_REG_RCX);u.mem_write(dest,bytes(32));u.mem_write(dest,'test\0'.encode('utf-16le'));e.q(dest+0x10,4);e.q(dest+0x18,7);e.ret(dest)
  elif a==e.base+0x1c4ce0:
   d=u.reg_read(UC_X86_REG_RDX);created.append((e.geti(d+0x14),e.geti(d+0x20)));e.ret(1)
  else:e.ret(0)
 for target in [m+0x4c500,m+0x45fc0,m+0x1d660,m+0x238c0,e.base+0x1c4ce0]:u.hook_add(UC_HOOK_CODE,wrappers,begin=target,end=target)
 cases=0
 for model in range(126):
  for color in [-1,0,2,4]:
   e.i(binding+4,model);descriptor=controller+0x120;u.mem_write(descriptor,bytes(0x1c0));e.q(descriptor+0xb0,e.PERSON);e.i(descriptor+0x14,0);e.i(descriptor+0x20,color)
   run(0x3b910,e.ACTOR,descriptor)
   b=C.c_int(0);c=C.c_int(color)
   assert lib.registry_resolve(reader,e.base,m,t,officer,C.byref(b),C.byref(c))==1
   assert (b.value,c.value)==created[-1],(model,color,b.value,c.value,created[-1]);cases+=1
 # Clear mappings, missing resources, mismatched name, unknown ID, torn vector.
 def resolve():
  b=C.c_int(0);c=C.c_int(2);ok=lib.registry_resolve(reader,e.base,m,t,officer,C.byref(b),C.byref(c));return ok,b.value,c.value
 e.i(binding+4,126);assert resolve()==(1,0,2)
 e.i(binding+4,125);e.q(m+0x1ec338,catalog);assert resolve()==(1,0,2);e.q(m+0x1ec338,catalog+101*40)
 e.i(binding+4,-1);assert resolve()==(1,0,2)
 e.i(binding+4,2);u.mem_write(e.PERSON+0x12,'else\0'.encode('utf-16le'));assert resolve()==(1,0,2)
 e.q(m+0x1ec350,binding+41);assert resolve()[0]==-1
 e.q(m+0x1ec350,binding);assert resolve()==(1,0,2)
 # Shared cleanup chains remain valid with registry both enabled and disabled.
 assert lib.registry_info(reader,e.base,m)==m+0x4940
 e.byte(m+0x1c1f8d,0);assert not lib.registry_installed(reader,e.base,m)
 assert lib.registry_info(reader,e.base,m)==m+0x4940
 report={'status':'PASS_OFFLINE_NOT_GAME_RENDERED','actual_registry_model_color_cases':cases,'code_guards_tested':guards,'all_guard_byte_corruptions_rejected':True,'user_log_ui_cleanup_conflict_reproduced':True,'actual_ui_wrapper_abi_and_order':True,'registry_on_and_off':True,'clear_unavailable_identity_and_torn_vector_cases':True,'original_bodies':list(range(25)),'extension_catalog_range':[25,125],'extension_resources':'synthetic resource names in emulation; no custom GPU assets verified','source_sha256':{str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [DLL,R/'native/registry_bridge.h',R/'native/tests/registry_bridge_probe.cpp',Path(__file__)]},'windows_game_executed':False}
 (R/'analysis/registry_r24_native_verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
