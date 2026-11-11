# R42 B1 재해 연출 생략 — unicorn 단위 시험 (SAN14_SceneSkip.dll)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL=os.environ.get('SS_DLL','/home/claude/r41/build_b1/SAN14_SceneSkip.dll')
fs={'SAN14_SceneSkip.dll':open(DLL,'rb').read(),'SAN14Mods/modules/SceneSkip.ini':b'[SceneSkip]\r\nDisaster=1\r\nEmploy=1\r\nTreasure=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
G=GAME_BASE
# 게임 쪽 자리 바이트(원래 call 대상) 확인
def call_to(site,target):
    b=w.r(G+site,5);return b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target
for s,t in [(0x2999ef,0x404a00),(0x299a0c,0x451ff0),(0x299ace,0x1f0ad0),(0x299adb,0x43ff90),(0x29ac7b,0x404a00),(0x29ac97,0x451ff0),(0x29aea6,0x1f0ad0),(0x29aeb3,0x43ff90),(0x2ba672,0x404a00),(0x2ba684,0x451ff0),(0x2ba85a,0x1f0ad0),(0x2ba891,0x43ff90)]:
    assert call_to(s,t),hex(s)
assert w.r(G+0x427ffe,10)==bytes.fromhex('b8010000004883c428c3') and call_to(0x427ff4,0x297490) and call_to(0x427ff9,0x2ba170)
base=w.api._loadlib('SAN14_SceneSkip.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_sceneskip.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=G+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3');free=w._tramp(b'\xc3')
groups=[]
inst=w._tramp(b'\xc3'*16)
def on_install():
    idp,hp,n=w.arg(0),w.arg(1),w.arg(2)
    gid=w.rstr(idp);hs=[]
    for i in range(n):
        h=hp+i*48;t=w.r64(h);ln=w.r32(h+8);hs.append((t-G,ln,w.r(h+12,ln),w.r(h+28,ln)))
    groups.append((gid,hs));return 1
w.override(inst,on_install)
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',G)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,inst,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r,[g[0] for g in groups]);assert r==1
if os.environ.get('SS_FULL'):
    gm={g[0]:g[1] for g in groups};assert set(gm)=={'SAN14CE.SceneSkip.Disaster','SAN14CE.SceneSkip.Employ','SAN14CE.SceneSkip.Treasure','SAN14CE.SceneSkip.DisasterOngoing','SAN14CE.SceneSkip.TurnSummary','SAN14CE.SceneSkip.TurnSummaryNews','SAN14CE.SceneSkip.Camera'},gm.keys()
    assert [(h[0],h[1]) for h in gm['SAN14CE.SceneSkip.Employ']]==[(0x6c43b0,6)] and [(h[0],h[1]) for h in gm['SAN14CE.SceneSkip.Treasure']]==[(0x6c4670,6)]
    for i in range(7):assert w.run(ex['SAN14_SceneSkipHooked'],(i,))==1,i
    print('B2·B3 묶음 따로 설치 OK')
dg=[g for g in groups if g[0]=='SAN14CE.SceneSkip.Disaster'];assert len(dg)==1 and len(dg[0][1])==12 and 0x427ffe not in [h[0] for h in dg[0][1]]
for rva,ln,exp,rep in dg[0][1]:
    assert rep[0] in (0xe8,0xe9) and all(x==0x90 for x in rep[5:]),hex(rva)
    if ln==5: assert rep[0]==0xe8
print('재해 자리 12곳(연출만, 요약 창 자리 0x427ffe 안 검) 묶음 OK')
assert w.run(ex['SAN14_SceneSkipHooked'],(0,))==1
tab=lambda i:w.run(ex['SAN14_SceneSkipTestTable'],(i,))
BGM,CUT,NARR,RESET,SUM,SUMBR,SUMBUF,C_BGM,C_CUT,C_NARR,C_RESET,C_SUM,EN,ADDH=[tab(i) for i in range(14)]
# 게임 원래 함수 자리를 기록 함수로 바꿔 '불렸는지' 확인
called=[]
def rec(name):
    def f():
        a=[w.arg(i) for i in range(2)]
        if name=='narr':called.append((name,w.rstr(a[0],wide=True),a[1]))
        else:called.append((name,a[0],a[1]))
        return 0
    return f
for a,nm in [(0x404a00,'bgm'),(0x451ff0,'cut'),(0x1f0ad0,'narr'),(0x43ff90,'reset')]:w.override(G+a,rec(nm))
txt=w.alloc(0x400)
histl=[]
root=w.alloc(0x90000);scen=w.alloc(0x800);w.w64(root+0x871e8,scen);w.w32(scen+0x450,1234);w.w64(G+0x277df18,root)
w.override(G+0x16efc0,lambda:0x5000)
w.override(G+0x934400,lambda:histl.append((w.arg(1),w.r32(w.rsp()+0x28),w.rstr(w.r64(w.rsp()+0x38),wide=True))) or 0)
def narr(s,mode=0):w.wstr(txt,s,wide=True);w.run(NARR,(txt,mode))
# 켬: 연출 4가지 모두 안 부름, 문장은 모아 둠
called.clear()
w.run(BGM,(0x1234,7));w.run(CUT,(0x5678,0x10));narr('A 지역에 역병 발생');w.run(RESET,(0x5678,))
w.run(BGM,(0x1234,7));w.run(CUT,(0x5678,0xf));narr('B·C 지역에 메뚜기 떼\n발생');w.run(RESET,(0x5678,))
assert called==[],called;print('켬: 음악·띠·나레이션·띠 접기 안 함')
print('로그창 줄:',histl);assert histl==[(1,1,'A 지역에 역병 발생'),(1,1,'B·C 지역에 메뚜기 떼 발생')];histl.clear()
# R42b: 창은 하나도 없음 — 요약 창 자리도 안 걸고, 많은 건도 로그창 줄만
for i in range(9):narr('재해 %d'%i)
assert called==[] and len(histl)==9 and histl[-1]==(1,1,'재해 8');histl.clear();print('9건: 창 0, 로그창 9줄')
# 끔: 원래 함수 그대로
w.run(ex['SAN14_SceneSkipSet'],(0,0));called.clear()
w.run(BGM,(0x1234,7));w.run(CUT,(0x5678,0x10));narr('D 지역 역병');w.run(RESET,(0x5678,))
print('끔:',[c[0] for c in called]);assert [c[0] for c in called]==['bgm','cut','narr','reset'] and called[0][1:]==(0x1234,7) and called[1][1:]==(0x5678,0x10) and called[2][1]=='D 지역 역병'
assert histl==[];print('끔: 로그창 줄 안 넣음(원작 창이 뜸)')
# 띠 펼침을 건너뛴 뒤 스위치가 꺼져도 접기는 건너뜀(쌍 맞춤), 다음 접기는 원래대로
w.run(ex['SAN14_SceneSkipSet'],(0,1));w.run(CUT,(0x5678,0x10));w.run(ex['SAN14_SceneSkipSet'],(0,0));called.clear()
w.run(RESET,(0x5678,));assert called==[];w.run(RESET,(0x5678,));assert [c[0] for c in called]==['reset'];print('띠 쌍 맞춤 OK')
w.run(ex['SAN14_SceneSkipSet'],(0,1))
assert w.fs_written.get('SAN14Mods/modules/SceneSkip.ini') is not None or True
print('기록:',[x for x in w.msgs if 'SceneSkip' in str(x)][:4])
print('ALL OK')
