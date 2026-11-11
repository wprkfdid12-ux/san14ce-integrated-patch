import struct, pathlib, bisect, subprocess, re
U16=lambda b,o:struct.unpack_from('<H',b,o)[0]
U32=lambda b,o:struct.unpack_from('<I',b,o)[0]
U64=lambda b,o:struct.unpack_from('<Q',b,o)[0]
class PE:
 def __init__(self,path):
  self.path=pathlib.Path(path);self.b=self.path.read_bytes(); b=self.b;n=U32(b,60);self.nt=n;self.opt=n+24;self.ts=U32(b,n+8);o=self.opt;self.base=U64(b,o+24);self.entry=U32(b,o+16);self.size=U32(b,o+56);self.dirs=[struct.unpack_from('<II',b,o+112+8*i) for i in range(16)];self.sections=[]
  self.m=bytearray(self.size);self.m[:U32(b,o+60)]=b[:U32(b,o+60)]
  for j in range(U16(b,n+6)):
   s=o+U16(b,n+20)+40*j;vs,va,rs,fp=struct.unpack_from('<4I',b,s+8);name=b[s:s+8].rstrip(b'\0').decode();self.sections.append((name,va,vs,rs,fp,U32(b,s+36)));self.m[va:va+rs]=b[fp:fp+rs]
  r,l=self.dirs[3];self.funcs=[]
  for p in range(r,r+l,12):
   a,z,u=struct.unpack_from('<3I',self.m,p)
   if a<z:self.funcs.append((a,z,u))
  self.funcs.sort();self.starts=[f[0] for f in self.funcs]
  self.root_ranges={}
  for f in self.funcs:
   root=self.chainroot(f)
   old=self.root_ranges.get(root[0],root)
   self.root_ranges[root[0]]=(root[0],max(old[1],f[1]),root[2])
  self.exports={};r,l=self.dirs[0]
  if r:
   _,_,_,_,_,ordinal,nf,nn,fun,names,ords=struct.unpack_from('<IIHHIIIIIII',self.m,r)
   for i in range(nn):self.exports[self.cs(U32(self.m,names+i*4))]=U32(self.m,fun+U16(self.m,ords+2*i)*4)
  self.imports={};r,l=self.dirs[1]
  while r and any(self.m[r:r+20]):
   look,_,_,name,iat=struct.unpack_from('<5I',self.m,r);look=look or iat;i=0
   while (v:=U64(self.m,look+8*i)):
    self.imports[iat+8*i]=(self.cs(name),('#'+str(v&65535)) if v>>63 else self.cs(v+2));i+=1
   r+=20
 def fileoff(self,rva):
  for name,va,vs,rs,fp,fl in self.sections:
   if va<=rva<va+rs:return fp+rva-va
  raise ValueError(hex(rva))
 def cs(self,r):return bytes(self.m[r:self.m.index(0,r)]).decode('utf8',errors='replace')
 def u16(self,r):return U16(self.m,r)
 def u32(self,r):return U32(self.m,r)
 def u64(self,r):return U64(self.m,r)
 def chainroot(self,f):
  seen=set()
  while f[2] and f[2] not in seen:
   u=f[2];seen.add(u)
   if not (self.m[u]>>3)&4:break
   x=u+4+((self.m[u+2]+1)&~1)*2
   f=struct.unpack_from('<3I',self.m,x)
  return f
 def func(self,r):
  i=bisect.bisect_right(self.starts,r)-1
  if i>=0 and self.funcs[i][0]<=r<self.funcs[i][1]:return self.root_ranges[self.chainroot(self.funcs[i])[0]]
  return (r,r+0x100,0)
 def dis(self,a,z=None):
  if z is None:a,z,_=self.func(a)
  return subprocess.check_output(['objdump','-d','-M','intel',f'--start-address={a+self.base:#x}',f'--stop-address={z+self.base:#x}',str(self.path)],text=True)
 def find(self,s):
  if isinstance(s,str):s=s.encode()
  out=[];o=0
  while (o:=self.m.find(s,o))>=0:out.append(o);o+=1
  return out
 def refs64(self,r):return self.find(struct.pack('<Q',self.base+r))
 def refs32(self,r):return self.find(struct.pack('<I',r))
 def vtables(self,name):
  out=[]
  for s in self.find(('.?AV'+name+'@@\0').encode()):
   td=s-16
   for q in self.refs32(td):
    col=q-12
    if col>=0 and self.u32(col)==1 and self.u32(col+20)==col:
     for ref in self.refs64(col):out.append(ref+8)
  return out
if __name__=='__main__':
 import sys
 p=PE(sys.argv[1])
 if len(sys.argv)>2:
  a=int(sys.argv[2],16);z=int(sys.argv[3],16) if len(sys.argv)>3 else None;print(p.dis(a,z))
 else:
  print(p.sections);print(p.exports)
