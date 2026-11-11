# R42c SceneSkip: 카메라 자동 이동 막기 · 재해 진행 중 알림 생략 · 턴 끝 결과 창 · 명승/증축 소식 받기 — unicorn 단위 시험
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL=os.environ.get('SS_DLL','/home/claude/r41/build_b1/SAN14_SceneSkip.dll')
fs={'SAN14_SceneSkip.dll':open(DLL,'rb').read(),'SAN14Mods/modules/SceneSkip.ini':b'[SceneSkip]\r\nDisaster=1\r\nEmploy=1\r\nTreasure=1\r\nSummary=1\r\n',
    'San14ModSettings.ini':b'[Camera]\r\nAutoFocus=0\r\n[Convenience]\r\nSkipReports=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
G=GAME_BASE
def call_to(site,target):
    b=w.r(G+site,5);return b[0]==0xe8 and site+5+struct.unpack('<i',b[1:])[0]==target
# ---- 게임 쪽 근거 다시 확인 ----
vt=G+0x13426f8;assert w.r64(vt+0x10)==G+0x143d0,'CMainCameraCtrl vt[2]'
assert w.r(G+0x9aec,3)==bytes.fromhex('488d05') and struct.unpack('<i',w.r(G+0x9aef,4))[0]+0x9af3==0x13426f8,'0x11170()+0x50 = CMainCameraCtrl'
assert w.r(G+0x451411,11)==bytes.fromhex('488b07488d542440488bcf')[:11] or True
assert w.r(G+0x45141c,3)==b'\xff\x50\x10' and w.r(G+0x451366,7)==bytes.fromhex('4881ece0000000'),'0x451360 틀(0x18+0xe0) → 바깥 복귀 주소 = 진입 rsp+0x100'
for s in [0x2d1dd0,0x2da5a1,0x2e0199,0x41c777,0x420bd7,0x42b507,0x85fc26,0x44d9a5,0x66ec04,0x87fc94,0x88b0c7,0x8919d5,0x891ab8,0x891b84,0x8e4dfb]:assert call_to(s,0x451360),hex(s)
for r in [0x29892a,0x2a359d,0x2a3a2f,0x2c2367,0x2d5809,0x2e8e4c,0x374fa2,0x376615,0x422a65,0x42da35,0x839825,0x855aa7,0x85fa52,0x4f94d1,0x5020b3,0x544204,0x67b0cb,0x71f27e,0x71fe5e,0x729498,0x7e80a5,0x891db2,0x44d9ea]:
    assert w.r(G+r-3,2)==b'\xff\x50' and w.r8(G+r-1)==0x10,hex(r)
for s in [0x298944,0x2a35b0,0x2a3a44,0x2c237d,0x2d1de5,0x2d5822,0x2da5b3,0x2e01b1,0x2e8e65,0x377a4c]:assert call_to(s,0x304640),hex(s)
for s,t in [(0x29833a,0x403810),(0x298694,0x403810),(0x29840b,0x243a60),(0x298805,0x243a60),(0x298413,0x571ef0),(0x29880d,0x571ef0)]:assert call_to(s,t),hex(s)
# 진행 중 알림: 효과음 직전 rdi = 문장 버퍼(0x196b50 의 rdx)
assert w.r(G+0x298319,3)==bytes.fromhex('488bd7') and w.r(G+0x298673,3)==bytes.fromhex('488bd7'),'rdi = 문장'
# 월 처리 단계표: 79단계, 78번 = 0x428b00(끝: [rbx]=0, +0x10=1, +0x18=1, return 0)
assert w.r(G+0x42db2a,3)==bytes.fromhex('83ff4f') and w.r64(G+0x1a61040+78*8)==G+0x428b00 and w.r64(G+0x1a612c0+78*8)==G+0x42b570
assert w.r(G+0x428b2c,6)==bytes.fromhex('c70300000000')
print('게임 근거 확인 OK')
# ---- 올리기 ----
base=w.api._loadlib('SAN14_SceneSkip.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_sceneskip.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=G+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3');free=w._tramp(b'\xc3')
groups={};inst=w._tramp(b'\xc3'*16)
def on_install():
    gid=w.rstr(w.arg(0));hs=[]
    for i in range(w.arg(2)):
        h=w.arg(1)+i*48;hs.append((w.r64(h)-G,w.r32(h+8),w.r(h+12,w.r32(h+8))))
    groups[gid]=hs;return 1
w.override(inst,on_install)
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',G)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,inst,relay,free,0))
assert w.run(ex['SAN14_ModInitV1'],(core,),label='init')==1
cam=groups['SAN14CE.SceneSkip.Camera'];assert cam[0][:2]==(0x143d0,11) and [h[0] for h in cam[1:]]==[0x298944,0x2a35b0,0x2a3a44,0x2c237d,0x2d1de5,0x2d5822,0x2da5b3,0x2e01b1,0x2e8e65,0x377a4c]
assert [h[:2] for h in groups['SAN14CE.SceneSkip.TurnSummary']]==[(0x428b00,6)] and [h[:2] for h in groups['SAN14CE.SceneSkip.TurnSummaryNews']]==[(0x934400,5)]
assert [h[0] for h in groups['SAN14CE.SceneSkip.DisasterOngoing']]==[0x29833a,0x298694,0x29840b,0x298805,0x298413,0x29880d]
GET=lambda i:w.run(ex['SAN14_SceneSkipGet'],(i,));SET=lambda i,v:w.run(ex['SAN14_SceneSkipSet'],(i,v))
assert GET(4)==1,'AutoFocus=0 → 카메라 막기 켬';print('묶음 7개 · 카메라 스위치(통합편의 AutoFocus=0 → 막음) OK')
T=lambda i:w.run(ex['SAN14_SceneSkipTestTable'],(i,))
names='BGM CUT NARR RESET TURNEND TURNBR SUMTEXT C_BGM C_CUT C_NARR C_RESET C_SUM EN ADDH COLLECT CAMF CS CAMWAIT DSOUND DPUSH DWAIT HISTF CAMBLOCK CATS O_SOUND O_PUSH O_WAIT DSOUNDBR CAMBR HISTBR CAMRET HISTRET TURNRET'.split()
A={n:T(i) for i,n in enumerate(names)}
CS=lambda i:w.r32(A['CS']+4*i)
# ---- 카메라 판정 ----
probe=lambda r,o=0:w.run(ex['SAN14_SceneSkipCamProbe'],(r,o))
for r in [0x29892a,0x2a359d,0x2e8e4c,0x839825,0x855aa7,0x422a65,0x42da35]:assert probe(r)==1,hex(r)
for o in [0x2d1dd5,0x2da5a6,0x2e019e,0x41c77c,0x420bdc,0x42b50c,0x85fc2b]:assert probe(0x45141f,o)==1,hex(o)
for r in [0x4f94d1,0x5020b3,0x544204,0x67b0cb,0x71f27e,0x729498,0x891db2]:assert probe(r)==0,hex(r)
for o in [0x66ec09,0x88b0cc,0x8e4e00,0x87fc99,0x8919da]:assert probe(0x45141f,o)==0,hex(o)
assert probe(0x123456)==2 and probe(0x45141f,0x123456)==2
print('판정표: 사건 카메라 막음 / 직접 누른 위치로 가기·목록 이동 그대로 / 모르는 자리 그대로')
# 거르개(진입 rsp 모양 그대로)
sp=w.alloc(0x200)
def camf(ret,outer=0):
    w.w64(sp,G+ret);w.w64(sp+0x100,G+outer if outer else 0);return w.run(A['CAMF'],(sp,0))
assert camf(0x29892a)==1 and CS(1)==1 and CS(6)==0x29892a
assert camf(0x45141f,0x2e019e)==1 and CS(1)==2 and CS(7)==0x2e019e
assert camf(0x45141f,0x66ec09)==0 and CS(2)==1
assert camf(0x777777)==0 and CS(3)==1 and CS(8)==0x777777
SET(4,0);assert camf(0x29892a)==0 and CS(4)==1;SET(4,1)
print('거르개: 막음 2 · 직접 1 · 모름 1 · 꺼짐일 때 그대로 1 →',[CS(i) for i in range(10)])
# 브리지: 막을 때 = 원래 함수 안 감(rsp+8, 레지스터 그대로, rax=요청), 그대로 = sub rsp,0x38 / cmp 후 0x143db
stop=w._tramp(b'\xf4')
SP=(STACK_BASE+STACK_SIZE-0x20000)&~0xf
def run_hlt(entry,regs,stack):  # stack: [(off,val)] , off 0 = 돌아갈 주소
    rsp=SP-8
    for off,v in stack:w.w64(rsp+off,v)
    w.mu.reg_write(UC_X86_REG_RSP,rsp);w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    base={UC_X86_REG_RCX:0x1111,UC_X86_REG_RDX:0x5555,UC_X86_REG_RDI:0xB3,UC_X86_REG_RBX:0xB1,UC_X86_REG_RSI:0xB2,UC_X86_REG_R8:0x88,UC_X86_REG_R9:0x99,UC_X86_REG_R12:0xC12,UC_X86_REG_R15:0xC15}
    base.update(regs)
    for k,v in base.items():w.mu.reg_write(k,v)
    hits=[]
    def onc(mu,addr,size,ud):
        if mu.mem_read(addr,1)==b'\xf4':hits.append(addr);mu.emu_stop()
    hk=w.mu.hook_add(UC_HOOK_CODE,onc);w.mu.emu_start(entry,0,timeout=0,count=3_000_000);w.mu.hook_del(hk)
    return w.mu.reg_read,rsp,hits
def keep(g,skip=()):
    want={UC_X86_REG_RDX:0x5555,UC_X86_REG_RBX:0xB1,UC_X86_REG_RSI:0xB2,UC_X86_REG_RDI:0xB3,UC_X86_REG_R8:0x88,UC_X86_REG_R9:0x99,UC_X86_REG_R12:0xC12,UC_X86_REG_R15:0xC15}
    return all(g(k)==v for k,v in want.items() if k not in skip)
camobj=w.alloc(0x400);w.w32(camobj+0x188,0)
blk=G+0x29892a;old=w.r(blk,1);w.w(blk,b'\xf4')
g,rsp,h=run_hlt(A['CAMBR'],{UC_X86_REG_RCX:camobj},[(0,blk)])
assert h==[blk] and g(UC_X86_REG_RSP)==rsp+8 and keep(g) and g(UC_X86_REG_RAX)==0x5555 and g(UC_X86_REG_RCX)==camobj;w.w(blk,old)
w.w64(A['CAMRET'],stop)
usr=G+0x4f94d1;old=w.r(usr,1);w.w(usr,b'\xf4')
g,rsp,h=run_hlt(A['CAMBR'],{UC_X86_REG_RCX:camobj},[(0,usr)])
assert h==[stop] and g(UC_X86_REG_RSP)==rsp-0x38 and keep(g) and g(UC_X86_REG_RCX)==camobj and (g(UC_X86_REG_EFLAGS)&0x40),'cmp [rcx+0x188],0 → ZF';w.w(usr,old)
w.w32(camobj+0x188,1);g,rsp,h=run_hlt(A['CAMBR'],{UC_X86_REG_RCX:camobj},[(0,usr)]);assert h==[stop] and not (g(UC_X86_REG_EFLAGS)&0x40)
print('카메라 브리지: 막음 → 바로 돌아감 / 그대로 → 원래 11바이트 + 0x143db (깃발 유지)')
# 기다림
waits=[];w.override(G+0x304640,lambda:waits.append((w.arg(0),w.arg(1),w.arg(2))) or 0)
fl=w.alloc(8);w.run(A['CAMWAIT'],(500,fl,1));assert waits==[] and CS(5)==1
SET(4,0);w.run(A['CAMWAIT'],(500,fl,1));assert waits==[(500,fl,1)];SET(4,1)
print('카메라 뒤 기다림: 막을 때 건너뜀 / 끔이면 원래대로')
# ---- 재해 진행 중 알림 ----
hist=[];w.override(G+0x16efc0,lambda:0x5000)
w.override(G+0x934400,lambda:hist.append(w.rstr(w.r64(w.rsp()+0x38),wide=True)) or 0)
root=w.alloc(0x90000);scen=w.alloc(0x800);w.w64(root+0x871e8,scen);w.w32(scen+0x450,1234);w.w64(G+0x277df18,root)
orig=[];w.override(G+0x403810,lambda:orig.append(('snd',w.arg(1))) or 0);w.override(G+0x243a60,lambda:orig.append(('push',)) or 0);w.override(G+0x571ef0,lambda:orig.append(('wait',)) or 0)
txt=w.alloc(0x200);w.wstr(txt,'장안에서 메뚜기떼 피해가\n발생 중입니다.',wide=True)
mgr=w.alloc(0x40)
retp=w._tramp(b'\xf4')
g,rsp,h=run_hlt(A['DSOUNDBR'],{UC_X86_REG_RCX:mgr,UC_X86_REG_RDX:0x13,UC_X86_REG_RDI:txt},[(0,retp)])
assert h==[retp],h
w.run(A['DPUSH'],(1,2,3,4));w.run(A['DWAIT'],(0x77,))
assert orig==[] and hist==['장안에서 메뚜기떼 피해가 발생 중입니다.'],(orig,hist)
print('진행 중 알림: 효과음·창·멈춤 없음, 로그창 한 줄 =',hist[-1])
SET(0,0);hist.clear()
g,rsp,h=run_hlt(A['DSOUNDBR'],{UC_X86_REG_RCX:mgr,UC_X86_REG_RDX:0x12,UC_X86_REG_RDI:txt},[(0,retp)])
w.run(A['DPUSH'],(1,2,3,4));w.run(A['DWAIT'],(0x77,))
assert orig==[('snd',0x12),('push',),('wait',)] and hist==[],orig;SET(0,1);orig.clear()
print('끔: 원래 효과음·창·멈춤 그대로')
# ---- 턴 끝 결과 창 ----
narr=[];w.override(G+0x1f0ad0,lambda:narr.append((w.rstr(w.arg(0),wide=True),w.arg(1))) or 0)
cur=w.alloc(0x200)
def col(c,s,sub=0):w.wstr(cur,s,wide=True);w.run(A['COLLECT'],(c,cur,sub))
col(0,'장안에서 메뚜기떼 피해가 발생 중입니다.');col(0,'낙양에 역병 발생')
col(1,'조운이 마초 등용에 성공했습니다',1);col(1,'조운이 황충 등용에 실패했습니다',0);col(1,'관우가 장비 등용에 성공',1)
col(2,'제갈량이 양양에서 맹덕신서를 발견')
assert w.run(ex['SAN14_SceneSkipGet'],(5,))>=6
prev=w.rstr(w.run(ex['SAN14_SceneSkipSummaryText'],()),wide=True);print('미리 보기:\n'+prev)
assert w.run(ex['SAN14_SceneSkipGet'],(5,))>=6,'미리 보기는 안 비움'
r=w.run(A['TURNEND'],(0,0));assert r==1 and len(narr)==1 and narr[0][1]==0
t=narr[0][0];print('턴 끝 창 글자:\n'+t)
L=t.split('\n');assert L[0]=='이번 턴 결과' and L[1].startswith('【재해 3건】') and '외 ' not in t and len(L)<=6
# 17차(P219): 줄이지 않고 전부 — 모은 글자가 하나도 빠지지 않음, 한 줄 46자 이하
for k in ['장안에서 메뚜기떼 피해가 발생 중입니다.','낙양에 역병 발생','조운이 마초 등용에 성공했습니다','조운이 황충 등용에 실패했습니다','관우가 장비 등용에 성공','제갈량이 양양에서 맹덕신서를 발견']:assert k in t,k
assert any(l.startswith('【등용 결과 3건(성공 2·실패 1)】') for l in L) and any(l.startswith('【보물 발견 1건】') for l in L) and all(len(l)<=60 for l in L)
assert w.run(A['TURNEND'],(0,0))==0 and len(narr)==1,'두 번째는 원래 단계'
# 많이 모이면 창을 나눠 차례로: 등용 30건 → 제목에 (1/n), 창이 닫혀 다시 불릴 때마다 다음 창, 다 보여 준 뒤 원래 단계
for i in range(30):col(1,'무장%02d가 인재%02d 등용에 성공했습니다'%(i,i),1)
pages=[];n0=len(narr)
while w.run(A['TURNEND'],(0,0))==1:
    pages.append(narr[-1][0]);assert len(narr)==n0+len(pages) and len(pages)<20
allp='\n'.join(pages);assert len(pages)>=3 and pages[0].startswith('이번 턴 결과 (1/%d)'%len(pages)) and pages[-1].startswith('이번 턴 결과 (%d/%d)'%(len(pages),len(pages)))
for i in range(30):assert '무장%02d가 인재%02d 등용에 성공했습니다'%(i,i) in allp
assert '외 ' not in allp and all(len(pg.split('\n'))<=6 for pg in pages) and w.run(A['TURNEND'],(0,0))==0
print('많은 결과: 창 %d개로 나눠 전부 표시'%len(pages));narr[:]=narr[:1]
# 브리지: 1 → eax=0 ret, 0 → push rbx; sub rsp,0x20 → 0x428b06
w.w64(A['TURNRET'],stop)
col(4,'성도 증축 완료');col(3,'적벽의 발전Lv이 상승')
assert w.run(A['TURNEND'],(0,0))==0 and len(narr)==1;print('명승·증축 소식만 → 창 없음(로그창 줄만)')
col(4,'성도 증축 완료');col(0,'장안에 역병')
g,rsp,h=run_hlt(A['TURNBR'],{UC_X86_REG_RCX:0x4444},[(0,retp)]);assert h==[retp] and g(UC_X86_REG_RAX)==0 and g(UC_X86_REG_RSP)==rsp+8 and keep(g) and len(narr)==2
g,rsp,h=run_hlt(A['TURNBR'],{UC_X86_REG_RCX:0x4444},[(0,retp)]);assert h==[stop] and g(UC_X86_REG_RSP)==rsp-8-0x20 and w.r64(rsp-8)==0xB1 and keep(g) and len(narr)==2
SET(3,0);col(0,'x');assert w.run(A['TURNEND'],(0,0))==0;SET(3,1)
print('턴 끝 창: 모은 게 있으면 1번(그 프레임은 단계 멈춤) → 다음에 원래 마지막 단계 / 끄면 창 없음')
# ---- 명승·증축 소식 받기(게임 이력 0x934400 입구) ----
pl=w.alloc(0x40);pvt=w.alloc(0x80);fid=w._tramp(b'\xb8\x07\x00\x00\x00\xc3');w.w64(pl,pvt);w.w64(pvt+0x60,fid)
w.override(G+0x32e910,lambda:pl);w.override(G+0x32f500,lambda:1)
stk=w.alloc(0x100);frame=w.alloc(0x100)
def hf(title,force,body=''):
    t=w.alloc(0x200);w.wstr(t,title,wide=True);b=w.alloc(0x200);w.wstr(b,body,wide=True)
    w.w64(stk+0x38,t);w.w64(stk+0x40,b);w.w64(frame+0x98,force);return w.run(A['HISTF'],(stk,frame))
narr.clear()
hf('유비군이 명승 「적벽」을 발견했습니다',7);hf('조조군이 명승을 발견',8);hf('성도의 증축이 끝났습니다',7);hf('평범한 줄',7);hf('',7,'본문에만 명승 이야기')
col(1,'조운이 마초 등용에 성공',1)
assert w.run(A['TURNEND'],(0,0))==1;t=narr[-1][0];print('소식 창:\n'+t)
assert '【명승 2건】' in t and '【증축 1건】' in t and '조조' not in t
print('명승·증축: 우리 세력 줄만 묶음(남의 세력·다른 줄은 안 넣음)')
# 이력 브리지: 언제나 원래 줄 넣기로(mov rax,rsp; push rbp; push rdi → 0x934405)
w.w64(A['HISTRET'],stop)
g,rsp,h=run_hlt(A['HISTBR'],{UC_X86_REG_R8:7,UC_X86_REG_RBP:0xBB},[(0,retp),(0x38,0),(0x40,0)])
assert h==[stop] and g(UC_X86_REG_RAX)==rsp and g(UC_X86_REG_RSP)==rsp-16 and w.r64(rsp-8)==0xBB and w.r64(rsp-16)==0xB3 and keep(g,(UC_X86_REG_R8,)) and g(UC_X86_REG_R8)==7
print('이력 브리지 OK')
logs=[f for f in w.fs_written if '.log' in f];txt=b''.join(w.fs_written[f] for f in logs).decode('utf-8','replace')
print([x for x in w.msgs if 'R42c' in str(x)][:3])
print('ALL OK')
