# R42c SceneSkip 카메라 막기 — 게임 코드 그대로 돌려 보는 시험(훅을 실제로 게임 메모리에 씀)
#  ① 사건 카메라 함수 0x2e8e30(명령 실행 사건) → CMainCameraCtrl vt[2] 0x143d0 → 막힘 · 뒤 기다림 0x304640 건너뜀
#  ② 공용 0x451360(칸으로 카메라)을 재해 진행 중 자리(0x2e0199)에서 부른 것처럼 → 0x451360 틀 안 rsp+0x100 바깥 주소로 판정 → 막힘
#     같은 0x451360 을 기록 창 '위치로 가기'(0x66ec04)에서 부른 것처럼 → 그대로 움직임
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL=os.environ.get('SS_DLL','/home/claude/r41/build_b1/SAN14_SceneSkip.dll')
fs={'SAN14_SceneSkip.dll':open(DLL,'rb').read(),'SAN14Mods/modules/SceneSkip.ini':b'[SceneSkip]\r\nDisaster=1\r\nEmploy=1\r\nTreasure=1\r\nSummary=1\r\n',
    'San14ModSettings.ini':b'[Camera]\r\nAutoFocus=0\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
G=GAME_BASE
base=w.api._loadlib('SAN14_SceneSkip.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
ex=w.modules['san14_sceneskip.dll']['exports']
near=G+0x29ff000+0x100000;w.mu.mem_map(near,0x20000,UC_PROT_ALL);nxt=[near]
def mk_relay():
    dest=w.arg(1);a=nxt[0];nxt[0]+=16;w.w(a,b'\x48\xb8'+struct.pack('<Q',dest)+b'\xff\xe0');return a
relay=w._tramp(b'\xc3');w.override(relay,mk_relay)
written={}
def install():
    gid=w.rstr(w.arg(0));n=w.arg(2)
    for i in range(n):
        h=w.arg(1)+i*48;t=w.r64(h);ln=w.r32(h+8);exp=w.r(h+12,ln);rep=w.r(h+28,ln)
        assert w.r(t,ln)==exp,(gid,hex(t-G));w.w(t,rep);written[t-G]=gid
    return 1
inst=w._tramp(b'\xc3');w.override(inst,install)
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3');free=w._tramp(b'\xc3');rdb=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',G)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,rdb,inst,relay,free,0))
assert w.run(ex['SAN14_ModInitV1'],(core,),label='init')==1
assert 0x143d0 in written and 0x2e8e65 in written and 0x934400 in written and 0x428b00 in written;print('훅',len(written),'곳 게임 메모리에 씀')
# 카메라 관리자 가짜: 0x11170() → mgr, mgr+0x50 = CMainCameraCtrl(vt 0x13426f8 그대로), +0x188 = 0(안 잠김)
mgr=w.alloc(0x1000);w.w64(mgr+0x50,G+0x13426f8);w.w32(mgr+0x50+0x188,0)
w.override(G+0x11170,lambda:mgr)
moved=[];waits=[]
w.override(G+0x13f30,lambda:moved.append(('smooth',w.r32(w.arg(1)))) or 0)   # 0x143d0 이 시간 >0 일 때 jmp 하는 곳
w.override(G+0xaca0,lambda:moved.append(('now',)) or 0)
w.override(G+0x304640,lambda:waits.append((w.arg(0),w.arg(2))) or 0)
req=w.alloc(0x80);w.w(req,bytes(0x80));w.w32(req,500)
CS=lambda i:w.run(ex['SAN14_SceneSkipCamStats'],(i,))
# ① 0x2e8e30
w.run(G+0x2e8e30,(req,),label='2e8e30');print('사건 카메라 0x2e8e30 막기 켬:',moved,waits,'막음',CS(1))
assert moved==[] and waits==[] and CS(1)==1 and CS(6)==0x2e8e4c and CS(5)==1
w.run(ex['SAN14_SceneSkipSet'],(4,0))
w.run(G+0x2e8e30,(req,),label='2e8e30 off');print('막기 끔:',moved,waits)
assert moved==[('smooth',500)] and waits==[(500,1)] and CS(4)==1
w.run(ex['SAN14_SceneSkipSet'],(4,1));moved.clear();waits.clear()
# ② 0x451360 을 게임 자리에서 부른 것처럼: 돌아갈 주소 = 그 자리 call 다음(멈춤 표시)
w.override(G+0x1886b0,lambda:(w.w(w.arg(2),struct.pack('<4f',100.0,5.0,200.0,0.0)),0)[1])
def via(site_ret):
    hit=[]
    def stop():
        hit.append(1);raise StopEmu('back')
    w.override(G+site_ret,stop)
    # 실제 call 모양: [rsp]=돌아갈 주소, [rsp+0x28]=다섯째(double), [rsp+0x30]=여섯째(int), 진입 rsp%16==8
    rsp=((STACK_BASE+STACK_SIZE-0x30000)&~0xf)-0x48;w.w64(rsp+0x28,struct.unpack('<Q',struct.pack('<d',0.5))[0]);w.w64(rsp+0x30,1)
    w.w64(rsp,G+site_ret)
    w.mu.reg_write(UC_X86_REG_RSP,rsp);w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    for r,v in ((UC_X86_REG_RCX,1234),(UC_X86_REG_RDX,0),(UC_X86_REG_R8,500)):w.mu.reg_write(r,v)
    w.mu.reg_write(UC_X86_REG_XMM3,struct.unpack('<Q',struct.pack('<d',1.25))[0])
    w.stop_reason=None
    try:w.mu.emu_start(G+0x451360,0,timeout=0,count=2_000_000)
    except UcError as e:print('emu',e,hex(w.mu.reg_read(UC_X86_REG_RIP)))
    return hit
b0=CS(1)
h=via(0x2e019e);print('0x451360 ← 재해 진행 중 자리 0x2e0199:',h,moved,'막음 +',CS(1)-b0,'바깥',hex(CS(7)))
assert h==[1] and moved==[] and CS(1)==b0+1 and CS(7)==0x2e019e
h=via(0x66ec09);print('0x451360 ← 기록 창 위치로 가기 0x66ec04:',h,moved)
assert h==[1] and moved==[('smooth',500)] and CS(1)==b0+1

# ③ 월 처리 단계 실행기 0x42dab0 를 78번(마지막)에서: 모은 결과 있으면 창 1번 + 단계 그대로 78(멈춤) → 다음 호출에 원래 끝 단계
narr=[];w.override(G+0x1f0ad0,lambda:narr.append(w.rstr(w.arg(0),wide=True)) or 0)
fin=[];w.override(G+0x33b830,lambda:fin.append('33b830') or 0);w.override(G+0x325140,lambda:fin.append('325140') or 0)
w.override(G+0x933df0,lambda:0);w.override(G+0x332e40,lambda:0)
root=w.alloc(0x90000);scen=w.alloc(0x800);w.w64(root+0x871e8,scen);w.w64(G+0x277df18,root)
col=w.run(ex['SAN14_SceneSkipTestTable'],(14,));t=w.alloc(0x100);w.wstr(t,'장안에 역병 발생',wide=True);w.run(col,(0,t,0))
st=w.alloc(0x40);w.w(st,bytes(0x40));w.w32(st,78)
w.run(G+0x42dab0,(st,),label='runner1');print('실행기 1번째:',narr,fin,'단계',w.r32(st),w.r32(st+0x18))
assert len(narr)==1 and narr[0].startswith('이번 턴 결과\n【재해 1건】') and fin==[] and w.r32(st)==78 and w.r32(st+0x18)==0
w.run(G+0x42dab0,(st,),label='runner2');print('실행기 2번째:',fin,'단계',w.r32(st),w.r32(st+0x10),w.r32(st+0x18))
assert len(narr)==1 and fin==['33b830','325140'] and w.r32(st)==0 and w.r32(st+0x10)==1 and w.r32(st+0x18)==1
print('턴 끝 결과 창: 마지막 단계에서 1번 → 창 닫힌 뒤 원래 끝 단계')
print('ALL OK')
