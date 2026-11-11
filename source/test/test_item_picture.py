# R41e A8 명품 목록 그림 — unicorn 단위 시험 (0x6839e0 입구 브리지 + 그림 방식 정하기)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_out/SAN14_ItemPicture.dll'
fs={'SAN14_ItemPicture.dll':open(DLL,'rb').read(),'SAN14Mods/modules/ItemPicture.ini':b'[ItemPicture]\r\nEnabled=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
# 실제 게임 0x6839e0 입구 바이트 확인
assert w.r(GAME_BASE+0x6839e0,9)==bytes.fromhex('405748 81ec80040000'.replace(' ',''))
base=w.api._loadlib('SAN14_ItemPicture.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_itempicture.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3');free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r);assert r==1
tab=lambda i:w.run(ex['SAN14_ItemPictureTestTable'],(i,))
BR,FIX,RET,USES,SETS,EN=[tab(i) for i in range(6)]
assert w.r64(RET)==GAME_BASE+0x6839e0+9
rd=lambda a,o:struct.unpack('<I',w.r(a+o,4))[0]
def cfg(v114=0,v118=0,v11c=0):
    c=w.alloc(0x400);w.w(c,bytes(0x400));w.w(c+0x114,struct.pack('<III',v114,v118,v11c));return c
c=cfg();w.run(FIX,(c,));print('정한 것 없음 → +0x114 =',rd(c,0x114));assert rd(c,0x114)==1 and rd(c,0x118)==0 and rd(c,0x11c)==0
c=cfg(v118=1);w.run(FIX,(c,));assert rd(c,0x114)==0 and rd(c,0x118)==1;print('부른 쪽이 +0x118 정함 → 그대로')
c=cfg(v11c=1);w.run(FIX,(c,));assert rd(c,0x114)==0;print('부른 쪽이 +0x11c 정함 → 그대로')
c=cfg(v114=1);w.run(FIX,(c,));assert rd(c,0x114)==1;print('이미 +0x114 → 그대로')
w.run(ex['SAN14_ItemPictureSetEnabled'],(0,));c=cfg();w.run(FIX,(c,));assert rd(c,0x114)==0;print('끄면 손대지 않음')
w.run(ex['SAN14_ItemPictureSetEnabled'],(1,));w.run(FIX,(0,));print('설정 없음(0) → 안전')
# 브리지: 원래 9바이트(push rdi; sub rsp,0x480) 실행 후 +9 로, rcx·rdx 등 보존
stop=w._tramp(b'\xf4');w.w64(RET,stop)
SP=(STACK_BASE+STACK_SIZE-0x20000)&~0xf;rsp=SP-8
c=cfg()
w.mu.reg_write(UC_X86_REG_RSP,rsp)
for k,v in {UC_X86_REG_RCX:c,UC_X86_REG_RDX:0x5555,UC_X86_REG_RDI:0xB3,UC_X86_REG_RBX:0xB1,UC_X86_REG_R8:0x88,UC_X86_REG_RAX:0x77}.items():w.mu.reg_write(k,v)
w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
w.mu.emu_start(BR,stop,timeout=0,count=2_000_000)
g=w.mu.reg_read
print('브리지:',hex(g(UC_X86_REG_RIP)),hex(g(UC_X86_REG_RSP)),hex(w.r64(rsp-8)),'+0x114=',rd(c,0x114))
assert g(UC_X86_REG_RIP)==stop and g(UC_X86_REG_RSP)==rsp-8-0x480 and w.r64(rsp-8)==0xB3 and g(UC_X86_REG_RCX)==c and g(UC_X86_REG_RDX)==0x5555 and g(UC_X86_REG_R8)==0x88 and g(UC_X86_REG_RAX)==0x77 and g(UC_X86_REG_RBX)==0xB1 and rd(c,0x114)==1
print('기록:',[x[2] for x in w.msgs if 'ItemPicture' in str(x)][:3])
print('ALL OK')
