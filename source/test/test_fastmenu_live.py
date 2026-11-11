import os
# R39 FastMenu 게임 중 켬/끔: 실제 DLL 을 1.0.1 EXE 위에서 돌려 자리 6곳 거는 것 확인 → 끔 → 들어오기 대기 자리 원래 바이트 → 켬 → 다시 xor
import sys,struct,time
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
D='/home/claude/r39/fastmenu/out/SAN14_FastMenu.dll'
fs={'SAN14Mods/Ext/SAN14_FastMenu.dll':open(D,'rb').read(),'SAN14Mods/Ext/SAN14_FastMenu.ini':open('/home/claude/r39/payload/f30/SAN14Mods/Ext/SAN14_FastMenu.ini','rb').read()}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
orig=bytes(w.r(GAME_BASE+0x56FFA9,5))
t0=time.time();r=w.api._loadlib('SAN14Mods/Ext/SAN14_FastMenu.dll');print('load',r,w.stop_reason)
if isinstance(r,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);r2=w.run(rip,max_insns=4_000_000_000,label='fm');print('first run',r2,w.stop_reason,'%.0fs'%(time.time()-t0))
m=w.modules['san14_fastmenu.dll'];ex=m['exports']
res=w.continue_threads(max_insns=4_000_000_000);print('threads',res,'%.0fs'%(time.time()-t0))
log=''.join(str(x[2]) for x in w.msgs if 'FastMenu.log' in str(x[0]))
print(log[-1500:])
cur=bytes(w.r(GAME_BASE+0x56FFA9,5));print('들어오기 자리 원래',orig.hex(),'지금',cur.hex())
assert cur==bytes.fromhex('31c00f1f00')
print('state',w.run(ex['SAN14_FastMenuState'],()))
w.run(ex['SAN14_FastMenuSetEnabled'],(0,));c0=bytes(w.r(GAME_BASE+0x56FFA9,5));print('끔 ->',c0.hex());assert c0==orig and w.run(ex['SAN14_FastMenuState'],())==0
w.run(ex['SAN14_FastMenuSetEnabled'],(1,));c1=bytes(w.r(GAME_BASE+0x56FFA9,5));print('켬 ->',c1.hex());assert c1==bytes.fromhex('31c00f1f00')
for a,n in ((0x56FFC7,5),(0x5709B0,5),(0x86EF64,6),(0x86F06D,6),(0x86C74A,5),(0x86C763,5)):print(hex(a),bytes(w.r(GAME_BASE+a,n)).hex())
print('ALL OK')
