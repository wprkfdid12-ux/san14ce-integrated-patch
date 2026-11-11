# 29번 항장·부상 색 — unicorn 단위 시험 (브리지 5개 + 판정 규칙)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_out/SAN14_StatusColor.dll'
fs={'SAN14_StatusColor.dll':open(DLL,'rb').read(),'SAN14Mods/modules/StatusColor.ini':b'[StatusColor]\r\nEnabled=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
base=w.api._loadlib('SAN14_StatusColor.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_statuscolor.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3')
free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r,w.stop_reason or '');assert r==1
tab=lambda i:w.run(ex['SAN14_StatusColorTestTable'],(i,))
B=[tab(i) for i in range(5)];RET=[None,None]+[tab(5),tab(6),tab(7),tab(8)];EXIT5=tab(9);LABEL=tab(10);SEEN=tab(11);LASTP=tab(12);STATIDX=tab(13)
print('ret 슬롯',[hex(w.r64(x)-GAME_BASE) for x in RET[2:]],hex(w.r64(EXIT5)-GAME_BASE))
assert [w.r64(x)-GAME_BASE for x in RET[2:]]==[0x930d40,0x85c42e,0x22fce7,0x153edb] and w.r64(EXIT5)-GAME_BASE==0x154138
# 가짜 DB
db=w.alloc(0x90000);w.w64(GAME_BASE+0x277df18,db)
def person(pid,ill=0,merit=0,stats=(70,71,72,73,74)):
    p=w.alloc(0x200);w.w(p,bytes(0x200));w.w(p+0x10,struct.pack('<H',pid));w.w(p+0x161,bytes([ill]));w.w(p+0x1be,struct.pack('<h',merit));w.w(p+0xec,bytes(stats))
    w.w64(db+0x158+pid*8,p);return p
def wstr(s):
    a=w.alloc(2*len(s)+16);w.w(a,s.encode('utf-16le')+b'\0\0');return a
def rstr(a):
    b=b''
    while True:
        c=w.r(a+len(b),2)
        if c==b'\0\0':return b.decode('utf-16le')
        b+=c
SP=STACK_BASE+STACK_SIZE-0x20000
def go(entry,regs,rsp,stop):
    w.mu.reg_write(UC_X86_REG_RSP,rsp)
    for k,v in regs.items():w.mu.reg_write(k,v)
    w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    try:w.mu.emu_start(entry,stop,timeout=0,count=5_000_000)
    except UcError as e:print('UcError',e,hex(w.mu.reg_read(UC_X86_REG_RIP)));return False
    return w.mu.reg_read(UC_X86_REG_RIP)==stop
P_ok=person(10);P_ex=person(11,merit=-1000);P_ill=person(12,ill=1);P_both=person(13,ill=2,merit=-5)
# 1) 판정: ScNameSeen 은 항장/부상만 기록
for p,n in ((P_ok,'조조'),(P_ex,'관우'),(P_ill,'장비'),(P_both,'유비')):w.run(SEEN,(p,wstr(n)))
def layout(vt_rva):
    o=w.alloc(0x200);w.w(o,bytes(0x200));w.w64(o,GAME_BASE+vt_rva);return o+0x130
HOLD=layout(0x14132b0)   # CInfoDetailPersonPerson (허용)
HEDIT=layout(0x140bd18)  # CEditParamDialog (막음)
HINP=layout(0x1470e38)   # CInputStringLayout (막음)
L=lambda s,ret=0x12345,h=None:rstr(w.run(LABEL,(wstr(s),ret,h or HOLD)))
print('편집창',L('관우',h=HEDIT),'입력창',L('관우',h=HINP),'이미 색 기호',L('^03관우'))
assert L('관우',h=HEDIT)=='관우' and L('관우',h=HINP)=='관우' and L('^03관우')=='^03관우'
print('보통',L('조조'),'항장',L('관우'),'부상',L('장비'),'둘다',L('유비'),'다른글',L('관우 군'))
assert L('조조')=='조조' and L('관우')=='^0A관우' and L('장비')=='^03장비' and L('유비')=='^03유비' and L('관우 군')=='관우 군'
# 항장 풀리면(공로 0 이상) 원래대로
w.w(P_ex+0x1be,struct.pack('<h',3));assert L('관우')=='관우';w.w(P_ex+0x1be,struct.pack('<h',-1000))
# 1b) 동명이인: 건강한 '관우'(id 30) 이름을 방금 꺼냈으면 칠하지 않음, 항장 관우를 다시 꺼내면 칠함
P_dup=person(30);w.run(SEEN,(P_dup,wstr('관우')));d1=L('관우');w.run(SEEN,(P_ex,wstr('관우')));d2=L('관우')
print('동명이인 건강',d1,'항장 본인',d2);assert d1=='관우' and d2=='^0A관우'
# 2) H1 브리지: 이름 함수 끝 재현 — [rsp+0x20]=저장 rdi, [rsp+0x28]=복귀, [rsp+0x30]=저장 rbx
P5=person(20,ill=1);nb=wstr('여포')
stop=w._tramp(b'\xf4');rsp=SP&~0xf;rsp-=8
w.w64(rsp+0x20,0xAAAA);w.w64(rsp+0x28,stop);w.w64(rsp+0x30,0xBBBB)
ok=go(B[0],{UC_X86_REG_RDI:P5,UC_X86_REG_RBX:nb,UC_X86_REG_R12:0x1212},rsp,stop)
g=lambda r:w.mu.reg_read(r);r12ok=g(UC_X86_REG_R12)==0x1212;h1=(g(UC_X86_REG_RAX),g(UC_X86_REG_RBX),g(UC_X86_REG_RDI),g(UC_X86_REG_RSP))
print('H1: 복귀',ok,'rax/rbx/rdi/rsp',h1==(nb,0xBBBB,0xAAAA,rsp+0x30),'r12 보존',r12ok,'기록',L('여포'))
assert ok and h1==(nb,0xBBBB,0xAAAA,rsp+0x30) and L('여포')=='^03여포' and r12ok
# 3) H4 브리지: rcx=무장 edx=번호 → 기억 + mov [rsp+0x10],edx / push rbp,rsi,rdi
rsp=(SP&~0xf)-8;w.w64(RET[4],stop)
ok=go(B[3],{UC_X86_REG_RCX:P_ex,UC_X86_REG_RDX:2,UC_X86_REG_RBP:0x5151,UC_X86_REG_RSI:0x5252,UC_X86_REG_RDI:0x5353},rsp,stop)
st=struct.unpack('<QQQ',w.r(g(UC_X86_REG_RSP),24))
print('H4: 복귀',ok,'기억',w.r64(LASTP)==P_ex,'[rsp+0x10]',w.r(rsp+0x10,4)==b'\2\0\0\0','push',st==(0x5353,0x5252,0x5151),g(UC_X86_REG_RSP)==rsp-24)
assert ok and w.r64(LASTP)==P_ex and w.r(rsp+0x10,4)==b'\2\0\0\0' and st==(0x5353,0x5252,0x5151) and g(UC_X86_REG_RSP)==rsp-24
# 4) H3 브리지: edx=번호 r8d=값 r9d=기본값 → 기억 + push 5개 + sub 0x840
rsp=(SP&~0xf)-8;w.w64(RET[3],stop)
ok=go(B[2],{UC_X86_REG_RDX:2,UC_X86_REG_R8:65,UC_X86_REG_R9:72,UC_X86_REG_RBX:1,UC_X86_REG_RBP:2,UC_X86_REG_RSI:3,UC_X86_REG_RDI:4,UC_X86_REG_R14:5},rsp,stop)
st=struct.unpack('<5Q',w.r(g(UC_X86_REG_RSP)+0x840,40))
print('H3: 복귀',ok,'기억',struct.unpack('<III',w.r(STATIDX,12)),'push',st,g(UC_X86_REG_RSP)==rsp-40-0x840)
assert ok and struct.unpack('<I',w.r(STATIDX,4))[0]==2 and st==(5,4,3,2,1) and g(UC_X86_REG_RSP)==rsp-40-0x840
# 5) H2 브리지: 이름 글자칸 (r8 바뀜, 나머지 보존, 원래 mov 3개)
rsp=(SP&~0xf)-0x30;w.w64(RET[2],stop);w.w64(rsp+0x28,0x140001234)   # R39b: 0x930d35(프롤로그 뒤) — 반환 주소 [rsp+0x28]
txt=wstr('관우')
ok=go(B[1],{UC_X86_REG_RCX:HOLD,UC_X86_REG_RDX:9,UC_X86_REG_R8:txt,UC_X86_REG_R9:1,UC_X86_REG_RBX:0x11,UC_X86_REG_RSI:0x22,UC_X86_REG_RDI:0x33,UC_X86_REG_R12:0x44},rsp,stop)
r8=g(UC_X86_REG_R8)
print('H2 이름: 복귀',ok,'글자',rstr(r8),'rcx/rdx/r9',g(UC_X86_REG_RCX)==HOLD,g(UC_X86_REG_RDX)==9,g(UC_X86_REG_R9)==1,'shadow',struct.unpack('<QQQ',w.r(rsp+8,24)))
assert ok and rstr(r8)=='^0A관우' and g(UC_X86_REG_RCX)==HOLD and g(UC_X86_REG_RDX)==9 and g(UC_X86_REG_R9)==1 and g(UC_X86_REG_RSI)==9 and g(UC_X86_REG_RDI)==r8 and g(UC_X86_REG_R14)==HOLD and g(UC_X86_REG_RBX)==0x11 and g(UC_X86_REG_R12)==0x44 and g(UC_X86_REG_RSP)==rsp
txt2=wstr('조조');ok=go(B[1],{UC_X86_REG_R8:txt2,UC_X86_REG_RCX:HOLD},rsp,stop);assert ok and g(UC_X86_REG_R8)==txt2;print('H2 보통 무장: 그대로')
# 6) H2 능력치 칸: 복귀 주소 0x85c5dc, 마지막 계산 무장 P_ex 번호 2, 기본값 72 → 회색, 안쪽 ^02 도 회색
w.w64(rsp+0x28,GAME_BASE+0x85c5dc);s=wstr('80^02+8^00')
ok=go(B[1],{UC_X86_REG_R8:s,UC_X86_REG_RCX:HOLD},rsp,stop);print('H2 능력치 항장:',rstr(g(UC_X86_REG_R8)));assert ok and rstr(g(UC_X86_REG_R8))=='^0A80^0A+8^00'
# 기본값 다르면(다른 무장 칸) 그대로
w.w(STATIDX+8,struct.pack('<I',50));ok=go(B[1],{UC_X86_REG_R8:s,UC_X86_REG_RCX:HOLD},rsp,stop);assert g(UC_X86_REG_R8)==s;print('H2 능력치 기본값 불일치: 그대로')
# 부상 무장 → 빨강
w.w64(LASTP,P_ill);w.w(STATIDX,struct.pack('<III',2,60,72));s=wstr('60^03-12^00');ok=go(B[1],{UC_X86_REG_R8:s,UC_X86_REG_RCX:HOLD},rsp,stop);print('H2 능력치 부상:',rstr(g(UC_X86_REG_R8)));assert rstr(g(UC_X86_REG_R8))=='^0360^03-12^00'
# 다른 스레드 기록이면 그대로
w.w(TEB_BASE+0x48,struct.pack('<I',1));w.cur_tid=7;ok=go(B[1],{UC_X86_REG_R8:s,UC_X86_REG_RCX:HOLD},rsp,stop);assert g(UC_X86_REG_R8)==s;w.cur_tid=1;print('H2 다른 스레드: 그대로')
w.w64(LASTP,P_ex);w.w(STATIDX,struct.pack('<III',2,80,72));s=wstr('80');ok=go(B[1],{UC_X86_REG_R8:s,UC_X86_REG_RCX:HEDIT},rsp,stop);assert g(UC_X86_REG_R8)==s;print('H2 능력치 편집창(CEditParamDialog): 그대로')
# 버퍼 링 16칸: 17번째에 첫 칸 재사용
bufs=[w.run(LABEL,(wstr('관우'),0x12345,HOLD)) for i in range(17)];assert len(set(bufs[:16]))==16 and bufs[16]==bufs[0];print('버퍼 링 16칸 확인')
# 7) H5 브리지: 기호 0x0A → 회색(감마표 → vfunc+0x48), 함수 끝으로
for i in range(256):w.w(GAME_BASE+0x277df20+i,bytes([i]))
rec=w.alloc(0x40);setc=w._tramp(b'\x48\xb8'+struct.pack('<Q',rec)+b'\x48\x89\x08\x89\x50\x08\xc3')  # [rec]=rcx [rec+8]=edx
vt=w.alloc(0x100);w.w64(vt+0x48,setc);obj=w.alloc(0x40);w.w64(obj,vt);ctx=w.alloc(0x40);w.w64(ctx,obj);w.w(ctx+8,b'\x0a')
w.w64(EXIT5,stop);rsp=SP&~0xf
ok=go(B[4],{UC_X86_REG_R8:ctx,UC_X86_REG_RBX:0x99},rsp,stop)
print('H5 회색: 끝으로',ok,'obj',w.r64(rec)==obj,'색',hex(struct.unpack('<I',w.r(rec+8,4))[0]),'rdi=ctx',g(UC_X86_REG_RDI)==ctx,'rbx',g(UC_X86_REG_RBX)==0x99)
assert ok and w.r64(rec)==obj and struct.unpack('<I',w.r(rec+8,4))[0]==0xff929696 and g(UC_X86_REG_RDI)==ctx and g(UC_X86_REG_RBX)==0x99
stop2=w._tramp(b'\xf4');w.w64(RET[5],stop2);w.w(ctx+8,b'\x03')
ok=go(B[4],{UC_X86_REG_R8:ctx},rsp,stop2);fl=g(UC_X86_REG_EFLAGS)
print('H5 순정 3: 원래 길',ok,'eax',g(UC_X86_REG_RAX),'ja 안 탐',not(((fl&1)==0) and ((fl&0x40)==0)))
assert ok and g(UC_X86_REG_RAX)==3 and not(((fl&1)==0) and ((fl&0x40)==0))
w.w(ctx+8,b'\x0c');ok=go(B[4],{UC_X86_REG_R8:ctx},rsp,stop2);fl=g(UC_X86_REG_EFLAGS);assert ok and ((fl&1)==0 and (fl&0x40)==0);print('H5 범위 밖 12: ja 탐(순정처럼 무시)')
# 8) 끔
w.run(ex['SAN14_StatusColorSetEnabled'],(0,));assert L('관우')=='관우';print('끔: 그대로')
print('기록 줄:',[x for x in w.msgs if 'R39' in str(x)][:3])

# R41 H6: 표·직접 글자 경로(0x930e30)
B6=tab(14);TXT=tab(15);RET6=tab(16);w.run(ex['SAN14_StatusColorSetEnabled'],(1,))
assert w.r64(RET6)-GAME_BASE==0x930e45,hex(w.r64(RET6)-GAME_BASE)
T=lambda s,ret:rstr(w.run(TXT,(0,wstr(s),GAME_BASE+ret)))
print('표 칸(0x86f9f2) 항장',T('관우',0x86f9f2),'부상',T('장비',0x86f9f2),'보통',T('조조',0x86f9f2))
assert T('관우',0x86f9f2)=='^0A관우' and T('장비',0x86f9f2)=='^03장비' and T('조조',0x86f9f2)=='조조'
print('0x930d20 경로',T('관우',0x930d6f),'표 밖 직접(최근 목록만)',T('관우',0x500000))
assert T('관우',0x930d6f)=='관우' and T('관우',0x500000)=='관우'
w.run(SEEN,(P_ex,wstr('관우')))   # 같은 스레드에서 방금 이 무장 이름을 꺼냄
print('방금 꺼낸 이름(표 밖 직접)',T('관우',0x500000));assert T('관우',0x500000)=='^0A관우'
w.run(SEEN,(P_ok,wstr('조조')))
assert T('관우',0x500000)=='관우' and T('^03관우',0x86f9f2)=='^03관우'
# 브리지 6: 원래 12바이트(mov r11,rsp; push rbx; push rsi; sub rsp,0x838) 뒤 0x930e45, rdx=바뀐 글자
w.w(GAME_BASE+0x930e45,b'\xf4');stop6=GAME_BASE+0x930e45
rsp=(SP&~0xf)-8;w.w64(rsp,GAME_BASE+0x86f9f2);txt=wstr('관우')
ok=go(B6,{UC_X86_REG_RCX:0x7777,UC_X86_REG_RDX:txt,UC_X86_REG_RBX:0xB1,UC_X86_REG_RSI:0xB2,UC_X86_REG_R8:0x88},rsp,stop6)
print('브리지6',ok,rstr(w.mu.reg_read(UC_X86_REG_RDX)),hex(w.mu.reg_read(UC_X86_REG_R11)),hex(w.mu.reg_read(UC_X86_REG_RSP)))
assert ok and rstr(w.mu.reg_read(UC_X86_REG_RDX))=='^0A관우' and w.mu.reg_read(UC_X86_REG_R11)==rsp and w.mu.reg_read(UC_X86_REG_RSP)==rsp-0x10-0x838 and w.r64(rsp-8)==0xB1 and w.r64(rsp-0x10)==0xB2 and w.mu.reg_read(UC_X86_REG_RCX)==0x7777 and w.mu.reg_read(UC_X86_REG_R8)==0x88
print('R41D',[x[2] for x in w.msgs if 'R41D' in str(x)][:4])
print('ALL OK')
