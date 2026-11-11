# R42 C 바탕화면 실행기 입구 SAN14_SceneTestAttach — 게임 이미지 크기·ModCore 확인, 한 번만 시작
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
DLL='/home/claude/r41/build_c/SAN14_SceneTest.dll'
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),{'SAN14_SceneTest.dll':open(DLL,'rb').read()},ini_text='',log=lambda s:None)
w.api._loadlib('SAN14_SceneTest.dll');ex=w.modules['san14_scenetest.dll']['exports']
r=w.run(ex['SAN14_SceneTestAttach'],(0,));print('ModCore 없음 ->',r);assert r==4
# 가짜 ModCore 모듈 등록 후
w.modules['san14_modcore.dll']=dict(base=0x7ff70000_0000,size=0x1000,pe=None,path='SAN14Mods/Core/SAN14_ModCore.dll',exports={},name='SAN14_ModCore.dll')
r=w.run(ex['SAN14_SceneTestAttach'],(0,));print('ModCore 있음 ->',r,w.stop_reason);
r2=w.run(ex['SAN14_SceneTestAttach'],(0,));print('두 번째 ->',r2)
print([m[2] for m in w.msgs if 'attached' in str(m)][:2])
assert r==1 and r2==3
print('ALL OK')
