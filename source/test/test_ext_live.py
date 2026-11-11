import os
# R39 ExtBridge 게임 중 켬/끔: 꺼 둔 것도 불러 두고 SetEnabled(0), 설정 창에서 켜면 바로 SetEnabled(1)
import sys,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
F='/home/claude/r39/test/fake/'
fs={'SAN14_ExtBridge.dll':open('/home/claude/r39/ext_bridge/SAN14_ExtBridge.dll','rb').read(),
    'SAN14Mods/modules/ExtBridge.ini':b'[Ext]\r\nNoExecute=0\r\nRoninEscape=1\r\nAISearch=0\r\nFastMenu=0\r\nPopulation=0\r\n[Population]\r\nBonus=2\r\nWarLoss=1\r\n'}
for n in ('NoExecute','RoninEscape','AISearch','FastMenu','Population'):fs['SAN14Mods/Ext/SAN14_%s.dll'%n]=open(F+'SAN14_%s.dll'%n,'rb').read()
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
w.api._loadlib('SAN14_ExtBridge.dll');m=w.modules['san14_extbridge.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3');logf=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,ret1,ret1,logf))
r=w.run(ex['SAN14_ModInitV1'],(core,));print('init',r);assert r==1
def var(n,v='last'):
    mm=w.modules['san14_%s.dll'%n.lower()];return struct.unpack('<i',w.r(mm['exports'][v],4))[0]
st={n:var(n) for n in ('NoExecute','RoninEscape','AISearch','FastMenu')}
print('시작 때 SetEnabled 값:',st,'인구 bonus/loss',var('Population','bonus'),var('Population','loss'))
assert st=={'NoExecute':0,'RoninEscape':1,'AISearch':0,'FastMenu':0} and var('Population','bonus')==0 and var('Population','loss')==0
ws=lambda s:(lambda a:(w.w(a,s.encode('utf-16le')+b'\0\0'),a)[1])(w.alloc(64))
ES=ex['SAN14_ExtSetEnabled'];STATE=ex['SAN14_ExtState']
for n in ('NoExecute','AISearch','FastMenu'):
    r=w.run(ES,(ws(n),1));print(n,'켬 ->',r,'DLL 값',var(n),'상태',w.run(STATE,(ws(n),)));assert r==1 and var(n)==1 and w.run(STATE,(ws(n),))==1
r=w.run(ES,(ws('Population'),1));print('인구 켬 ->',r,var('Population','bonus'),var('Population','loss'));assert r==1 and var('Population','bonus')==2 and var('Population','loss')==1
r=w.run(ES,(ws('FastMenu'),0));assert r==1 and var('FastMenu')==0
print('ini:',w.api._ini_for('SAN14Mods/modules/ExtBridge.ini')['Ext'])
print('ALL OK')
