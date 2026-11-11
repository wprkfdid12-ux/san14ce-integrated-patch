import sys,struct
sys.path.insert(0,'/home/claude/unicorn-engine/unicorn/bindings/python')
from unicorn import *; from unicorn.x86_const import *
P=sys.argv[1] if len(sys.argv)>1 else '/home/claude/port/th/SAN14_TraitHover_coexist.dll'
d=open(P,'rb').read()
nt=struct.unpack_from('<I',d,0x3c)[0]; so=struct.unpack_from('<H',d,nt+20)[0]; sh=nt+24+so; ns=struct.unpack_from('<H',d,nt+6)[0]
B=0x180000000; size=struct.unpack_from('<I',d,nt+24+56)[0]
mu=Uc(UC_ARCH_X86,UC_MODE_64); mu.mem_map(B,(size+0xfff)&~0xfff)
for i in range(ns):
    vs,va,rs,rp=struct.unpack_from('<IIII',d,sh+i*40+8); mu.mem_write(B+va,d[rp:rp+rs])
G=0x140000000; mu.mem_map(G,0x2a00000)
T9=0x1e0000000; mu.mem_map(T9,0x40000); mu.mem_write(T9+0x3c,struct.pack('<I',0x80)); mu.mem_write(T9+0x80+0x50,struct.pack('<I',0x33000))
mu.mem_map(0x7000000,0x10000); mu.mem_write(0x7000100,b'\xf4')
def stub(addr,code): mu.mem_write(addr,code); return addr
# GetModuleHandleW stub -> T9 (or 0)
ents=[struct.unpack_from('<III',d,0x0+0) for _ in range(0)]
def r2o(r):
    for i in range(ns):
        vs,va,rs,rp=struct.unpack_from('<IIII',d,sh+i*40+8)
        if va<=r<va+rs: return r-va+rp
ents=[struct.unpack_from('<III',d,r2o(0x62a0)+12*i) for i in range(152)]
T9H={0:0x6820,1:0x60b0,2:0x6020,3:0x6850,4:0x6240,5:0x69e0,6:0x6150,7:0x6100,8:0x5ba0,9:0x61a0,10:0x61f0,11:0x5ee0,12:0x5e70}
def run(owner_t9,gmh_ret,conflict=False):
    for s,e,k in ents: mu.mem_write(G+s,struct.pack('<Q',(T9+T9H[k]) if owner_t9 else (G+e)))
    if conflict: s,e,k=ents[5]; mu.mem_write(G+s,struct.pack('<Q',T9+0x1111))
    sa=0x7000200+(0x40 if gmh_ret else 0)+(0x80 if conflict else 0)+(0x100 if owner_t9 else 0)
    mu.mem_write(sa,b'\x48\xb8'+struct.pack('<Q',gmh_ret)+b'\xc3')
    mu.mem_write(B+0x8568,struct.pack('<Q',sa))
    mu.mem_write(B+0xa7250,b'\0'*104)
    mu.reg_write(UC_X86_REG_RAX,G); mu.reg_write(UC_X86_REG_RSP,0x7008000); mu.mem_write(0x7008000,struct.pack('<Q',0x7000100))
    for r in (UC_X86_REG_RBX,UC_X86_REG_RSI,UC_X86_REG_RDI,UC_X86_REG_R12): mu.reg_write(r,0x5555)
    mu.emu_start(B+0x5070,0x7000100,count=200000)
    Pv=struct.unpack('<13Q',mu.mem_read(B+0xa7250,104))
    base=struct.unpack('<Q',mu.mem_read(B+0x9000,8))[0]
    regs=[mu.reg_read(r) for r in (UC_X86_REG_RBX,UC_X86_REG_RSI,UC_X86_REG_RDI,UC_X86_REG_R12)]
    assert base==G and mu.reg_read(UC_X86_REG_RAX)==G and regs==[0x5555]*4 and mu.reg_read(UC_X86_REG_RSP)==0x7008008
    return Pv
nat={k:G+e for s,e,k in ents if k<=12}
Pv=run(True,T9); assert all(Pv[k]==T9+T9H[k] for k in range(13)),[hex(x) for x in Pv]; print('INIT: Traits9 owns all → P = Traits9 handlers per kind OK')
Pv=run(False,T9); assert all(Pv[k]==nat[k] for k in range(13)); print('INIT: native slots → P = game functions OK')
Pv=run(True,0); assert all(Pv[k]==nat[k] for k in range(13)); print('INIT: Traits9 not loaded → P = game functions OK')
Pv=run(True,T9,conflict=True); k5=ents[5][2]; assert Pv[k5]==nat[k5] and all(Pv[k]==T9+T9H[k] for k in range(13) if k!=k5); print('INIT: mixed owner in one kind → that kind falls back to game function (owner check will refuse) OK')
# PK cave
Pv=run(True,T9)
for idx in (0,5,40,150):
    r8=B+0x62a0; rcx=3*idx
    mu.reg_write(UC_X86_REG_R8,r8); mu.reg_write(UC_X86_REG_RCX,rcx); mu.reg_write(UC_X86_REG_RSP,0x7008000); mu.mem_write(0x7008000,struct.pack('<Q',0x7000100))
    mu.emu_start(B+0x5150,0x7000100,count=100)
    k=ents[idx][2]; assert mu.reg_read(UC_X86_REG_R9)==Pv[k] and mu.reg_read(UC_X86_REG_RAX)&0xffffffff==1 and mu.reg_read(UC_X86_REG_RCX)==rcx
print('PK cave OK')
for k in (0,3,8):
    mu.reg_write(UC_X86_REG_RAX,k); mu.reg_write(UC_X86_REG_RBX,8); mu.reg_write(UC_X86_REG_RDI,B+0x62a0); mu.reg_write(UC_X86_REG_RSP,0x7008000); mu.mem_write(0x7008000,struct.pack('<Q',0x7000100))
    mu.emu_start(B+0x5180,0x7000100,count=100); assert mu.reg_read(UC_X86_REG_R13)==Pv[k]
print('P3 cave OK')
# filter: jumps
for key,exp in ((0x150,0x2281),(0x200,0x220b),(0x2c8,0x220b),(0x2c9,0x2281),(0x10000+5,0x2281),(0,0x2281)):
    mu.reg_write(UC_X86_REG_RDX,key); mu.reg_write(UC_X86_REG_RSP,0x7008000)
    hit=[]
    h=mu.hook_add(UC_HOOK_CODE,lambda uc,a,s,u: (hit.append(a),uc.emu_stop()) if a in (B+0x2281,B+0x220b) else None)
    mu.emu_start(B+0x21b4,0,count=50); mu.hook_del(h)
    assert hit and hit[0]==B+exp,(hex(key),hit)
print('tooltip filter OK (traits/objects → close own tooltip, tactics 0x200..0x2c8 → tactic path)')
