"""Executes compiled Win64 PE code under synthetic Win32/game boundaries on Linux.
This is NOT a Windows or live-game test. Original executable bytes are local only.
"""
import ctypes as C,struct,re,subprocess,sys,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'analysis'))
from peutil import PE
U=C.c_uint64
class Bridge:
 def __init__(self,names):
  self.names=dict(names);self.callbacks={};self.errors=[]
  lines=['#include <stdint.h>','typedef uint64_t U;', 'typedef U(*D)(U,U,U,U,U,U,U,U,U);','static D d;','void set_dispatch(D p){d=p;}']
  for i,(name,n) in enumerate(self.names.items()):
   args=','.join('U a'+str(j) for j in range(n)) or 'void';vals=','.join(['a'+str(j) for j in range(n)]+['0']*(8-n))
   lines.append(f'__attribute__((ms_abi)) U {name}({args}){{return d({i},{vals});}}')
  lines.append('U call8(U f,U a,U b,U c,U e,U g,U h,U i,U j){return ((U(__attribute__((ms_abi))*)(U,U,U,U,U,U,U,U))f)(a,b,c,e,g,h,i,j);}')
  source=ROOT/'tests/abi_shim.c';so=ROOT/'tests/abi_shim.so';source.write_text('\n'.join(lines))
  subprocess.run(['gcc','-shared','-fPIC','-O2','-Werror',str(source),'-o',str(so)],check=True)
  self.lib=C.CDLL(str(so));self.lib.call8.argtypes=[U]*9;self.lib.call8.restype=U
  self.dispatch_type=C.CFUNCTYPE(U,*([U]*9))
  self.dispatch=self.dispatch_type(self.route);self.lib.set_dispatch.argtypes=[self.dispatch_type];self.lib.set_dispatch(self.dispatch)
 def route(self,i,*args):
  name=list(self.names)[i]
  try:return (self.callbacks[name](*args[:self.names[name]]) or 0)&((1<<64)-1)
  except BaseException as e:
   self.errors.append((name,str(e)));print('STUB FAILURE',name,repr(e),flush=True);return 0
 def addr(self,name):return C.cast(getattr(self.lib,name),C.c_void_p).value
 def call(self,f,*args):
  out=self.lib.call8(f,*args,*([0]*(8-len(args))))
  if self.errors:raise RuntimeError(self.errors)
  return out
class Mapping:
 def __init__(self,size,data=b''):
  libc=C.CDLL(None);libc.mmap.argtypes=[C.c_void_p,C.c_size_t,C.c_int,C.c_int,C.c_int,C.c_long];libc.mmap.restype=C.c_void_p
  self.size=size;self.addr=libc.mmap(None,size,7,0x22,-1,0)
  if not self.addr or self.addr==C.c_void_p(-1).value:raise OSError('mmap')
  if data:C.memmove(self.addr,data,len(data))
 def put(self,o,data):assert 0<=o<=self.size-len(data);C.memmove(self.addr+o,data,len(data))
 def get(self,o,n):assert 0<=o<=self.size-n;return C.string_at(self.addr+o,n)
 def u64(self,o,v=None):
  if v is not None:self.put(o,struct.pack('<Q',v))
  return struct.unpack('<Q',self.get(o,8))[0]
 def u32(self,o,v=None):
  if v is not None:self.put(o,struct.pack('<I',v&0xffffffff))
  return struct.unpack('<I',self.get(o,4))[0]
 def close(self):C.CDLL(None).munmap(C.c_void_p(self.addr),C.c_size_t(self.size))
class Loaded(Mapping):
 def __init__(self,path,bridge=None,mapfile=None):
  self.pe=PE(path);super().__init__(self.pe.size)
  data=bytearray(self.pe.m);r,n=self.pe.dirs[5];end=r+n
  while r and r<end:
   page,size=struct.unpack_from('<II',data,r)
   if size<8:raise ValueError('bad relocation')
   for o in range(r+8,r+size,2):
    item=struct.unpack_from('<H',data,o)[0];kind=item>>12;x=page+(item&4095)
    if kind==10:struct.pack_into('<Q',data,x,(struct.unpack_from('<Q',data,x)[0]+self.addr-self.pe.base)&((1<<64)-1))
    elif kind:raise ValueError(kind)
   r+=size
  if bridge:
   for i,(_dll,name) in self.pe.imports.items():struct.pack_into('<Q',data,i,bridge.addr(name))
  self.put(0,bytes(data));self.symbols=dict(self.pe.exports)
  if bridge and 'SAN14_MeritDiagnosticsV1' in self.pe.exports:
   table=bridge.call(self.addr+self.pe.exports['SAN14_MeritDiagnosticsV1'])
   for i in range(512):
    name,addr=struct.unpack('<QQ',C.string_at(table+i*16,16))
    if not name:break
    self.symbols[C.string_at(name).decode()]=addr-self.addr
 def sym(self,name):return self.addr+self.symbols[name]
 def val(self,name,v=None,fmt='<I'):
  p=self.sym(name)
  if v is not None:C.memmove(p,struct.pack(fmt,v),struct.calcsize(fmt))
  return struct.unpack(fmt,C.string_at(p,struct.calcsize(fmt)))[0]
 def jump(self,rva,destination):self.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',destination))
def wide(p,max_chars=1024):
 b=bytearray()
 for i in range(max_chars):
  c=C.string_at(p+2*i,2)
  if c==b'\0\0':return b.decode('utf-16le')
  b+=c
 raise ValueError('unterminated UTF16')
def writewide(p,s,n):
 b=(s+'\0').encode('utf-16le')
 if len(b)>n*2:return len(b)//2
 C.memmove(p,b,len(b));return len(s)
