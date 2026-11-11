# 34번 R41d 지역담당관 공석 경고(빨간 이름 글자색) — unicorn 단위 시험 (매 프레임 브리지 2개 + 판정 + 스타일 바꾸기/되돌리기)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_out/SAN14_RegionOfficer.dll'
fs={'SAN14_RegionOfficer.dll':open(DLL,'rb').read(),'SAN14Mods/modules/RegionOfficer.ini':b'[RegionOfficer]\r\nWarning=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
base=w.api._loadlib('SAN14_RegionOfficer.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_regionofficer.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3')
free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r);assert r==1
tab=lambda i:w.run(ex['SAN14_RegionOfficerTestTable'],(i,))
BC,BF,FRAME,JUDGE,WSETS,RESTS,FRAMES=[tab(i) for i in range(7)]
G=lambda f,*a:w.run(ex[f],a)
assert G('SAN14_RegionOfficerGet',1)==1
def recorder(rva):
    rec=w.alloc(0x40);w.w(rec,bytes(0x40))
    code=b'\x48\xb8'+struct.pack('<Q',rec)+b'\x48\x89\x08\x48\x89\x50\x08\x4c\x89\x40\x10\x4c\x89\x48\x18\x48\xff\x40\x20\xc3'
    w.w(GAME_BASE+rva,code);return rec
STY=recorder(0x92ef10);TXT=recorder(0x930d20)
cnt=lambda rec:w.r64(rec+0x20)
last=lambda:(w.r64(STY),w.r64(STY+8)&0xffffffff,w.r64(STY+0x10)&0xffffffff)
db=w.alloc(0x90000);w.w(db,bytes(0x90000));w.w64(GAME_BASE+0x277df18,db)
areas=w.alloc(0x98*8);w.w(areas,bytes(0x98*8));w.w64(db+0x6caf0,areas)
for i in range(8):w.w64(db+0x6caf0+i*8,areas+i*0x98)
F=[w.alloc(0x40) for i in range(4)]
for i,f in enumerate(F):w.w64(db+0xdf30+i*8,f)
scen=w.alloc(0x100);w.w(scen,bytes(0x100));w.w(scen+0x3a,bytes([1]));w.w64(db+0x871e8,scen)
for t in range(4):
    tp=w.alloc(0x40);w.w(tp,bytes(0x40));w.w(tp+0x14,bytes([t]));w.w64(db+0xe270+t*8,tp)
vt_ok=w.alloc(0x40);w.w64(vt_ok+0x18,ret1);ret0=w._tramp(b'\x31\xc0\xc3');vt_bad=w.alloc(0x40);w.w64(vt_bad+0x18,ret0)
def person(pid,ok=True):
    p=w.alloc(0x200);w.w(p,bytes(0x200));w.w64(p,vt_ok if ok else vt_bad);w.w(p+0x10,struct.pack('<H',pid));w.w64(db+0x158+pid*8,p);return p
person(77);person(78,ok=False)
def area(i,owner_tile,officer,name):
    a=areas+i*0x98;w.w(a+0x3a,struct.pack('<H',owner_tile));w.w(a+0x36,struct.pack('<H',officer));w.w(a+0x10,name.encode('utf-16le')+b'\0\0');return a
A1=area(1,1,0,'성고');A2=area(2,2,0,'완');A3=area(3,1,77,'신야');A4=area(4,1,78,'번')
J=lambda a:struct.unpack("<i",struct.pack("<I",w.run(JUDGE,(a,))&0xffffffff))[0]
assert J(A1)==3 and J(A2)==1 and J(A3)==2 and J(A4)==3 and J(areas)==-1
def layout(kind,data,subvt=None):
    L=w.alloc(0x300);w.w(L,bytes(0x300));w.w64(L,GAME_BASE+(0x139a328 if kind==0 else 0x13999b8));w.w64(L+0x1f0,data)
    sub=w.alloc(0x300);w.w(sub,bytes(0x300));w.w64(sub,GAME_BASE+(subvt if subvt else (0x1399e78 if kind==0 else 0x1399108)))
    arr=w.alloc(0x100);w.w(arr,bytes(0x100))
    for c in range(0x18):e=w.alloc(0x140);w.w(e,bytes(0x140));w.w64(arr+c*8,e)
    w.w64(sub+0x130+8,arr);w.w(sub+0x130+0x10,struct.pack('<i',0x18))
    w.w64(L+(0x208 if kind==0 else 0x218),sub);return L,sub
def fr(L,k,dt=0):
    w.tick+=dt;w.run(FRAME,(L,k))
# 1) 도시형: 내 공석 지역 → 처음엔 기다림(0.3초), 0.4초 뒤 스타일 0x14(빨강) 를 (글자판+0x130, 0x16) 에
L1,S1=layout(0,A1);fr(L1,0);assert cnt(STY)==0;print('처음 본 공석: 기다림')
fr(L1,0,100);assert cnt(STY)==0;print('0.15초 안: 판정 안 함')
fr(L1,0,300);print('도시형 빨강:',[hex(x) for x in last()]);assert cnt(STY)==1 and last()==(S1+0x130,0x16,0x14)
assert cnt(TXT)==0;print('이름 글자는 안 건드림')
# 2) 빨강 유지 중: 1초마다 다시 칠함, 그 사이엔 안 칠함
fr(L1,0,200);assert cnt(STY)==1;fr(L1,0,900);assert cnt(STY)==2 and last()[2]==0x14;print('빨강 유지 1초마다 다시 칠함')
# 3) 담당 임명 → 다음 판정 때 바로 원래 색 0x13
w.w(A1+0x36,struct.pack('<H',77));fr(L1,0,200);print('임명 후:',[hex(x) for x in last()]);assert cnt(STY)==3 and last()==(S1+0x130,0x16,0x13)
fr(L1,0,2000);assert cnt(STY)==3;print('임명 상태: 더 안 칠함')
# 4) 잠깐 0(다른 스레드가 0 썼다가 다시 채움) → 안 칠함
w.w(A1+0x36,b'\0\0');fr(L1,0,200);w.w(A1+0x36,struct.pack('<H',77));fr(L1,0,200);fr(L1,0,200);assert cnt(STY)==3;print('잠깐 0: 무시')
# 5) 남의 지역 공석 / 내 지역 담당 있음 → 안 칠함, 담당 무장 무효 → 빨강
L2,_=layout(0,A2);L3,_=layout(0,A3);L4,S4=layout(0,A4)
for L in(L2,L3,L4):fr(L,0)
w.tick+=400
for L in(L2,L3):fr(L,0)
assert cnt(STY)==3;fr(L4,0);assert cnt(STY)==4 and last()==(S4+0x130,0x16,0x14);print('남의 지역·담당 있음: 그대로 / 무효 무장: 빨강')
# 6) 사용자 글자 모드 → 칠했던 판이면 원래 색, 아니면 손대지 않음
w.w(L4+0x220,struct.pack('<I',1));fr(L4,0,200);assert cnt(STY)==5 and last()[2]==0x13;fr(L4,0,2000);assert cnt(STY)==5;print('사용자 글자 모드: 원래 색으로만')
w.w(L4+0x220,struct.pack('<I',0))
# 7) 거점형: 데이터 → 0x229e40(가짜) → 지역, 글자판 [판+0x218], 칸 0xe, 빨강 0x10 / 원래 0
w.w(A1+0x36,b'\0\0')
ret_area=w._tramp(b'\x48\xb8'+struct.pack('<Q',A1)+b'\xc3');w.w(GAME_BASE+0x229e40,b'\x48\xb8'+struct.pack('<Q',ret_area)+b'\xff\xe0')
fd=w.alloc(0x100);w.w(fd,bytes(0x100))
L6,S6=layout(1,fd);fr(L6,1,400);fr(L6,1,400);print('거점형 빨강:',[hex(x) for x in last()]);assert last()==(S6+0x130,0xe,0x10)
w.w(A1+0x36,struct.pack('<H',77));fr(L6,1,200);assert last()==(S6+0x130,0xe,0);print('거점형 임명 → 원래 색 0')
# 8) 끄기 → 빨강인 판 원래 색 / 켜기 → 다시 빨강
w.w(A1+0x36,b'\0\0');fr(L6,1,200);fr(L6,1,400);assert last()[2]==0x10
G('SAN14_RegionOfficerSet',1,0);fr(L6,1,200);assert last()[2]==0 and G('SAN14_RegionOfficerGet',1)==0;print('끄기: 원래 색')
G('SAN14_RegionOfficerSet',1,1);fr(L6,1,200);fr(L6,1,400);assert last()[2]==0x10;print('켜기: 다시 빨강')
# 9) 판 돌려쓰기: 같은 판이 담당 있는 지역 데이터로 바뀜 → 기다리지 않고 바로 원래 색
c0=cnt(STY);w.w(L1+0x1f0,struct.pack('<Q',A1));w.w(A1+0x36,b'\0\0');fr(L1,0,200);fr(L1,0,400);assert last()==(S1+0x130,0x16,0x14)
w.w64(L1+0x1f0,A3);c0=cnt(STY);fr(L1,0,10);print('돌려쓰기:',[hex(x) for x in last()]);assert cnt(STY)==c0+1 and last()==(S1+0x130,0x16,0x13)
# 10) 다른 vtable 의 판 / 글자판 종류가 다르면 손대지 않음
c0=cnt(STY);LX,_=layout(0,A1);w.w64(LX,GAME_BASE+0x1399708);fr(LX,0,400);fr(LX,0,400);assert cnt(STY)==c0
LY,_=layout(0,A1,subvt=0x139a7d8);fr(LY,0,400);fr(LY,0,400);assert cnt(STY)==c0;print('다른 판·다른 글자판: 안 건드림')
# 11) 칸이 없는 글자판(자식 수 부족) → 안 칠함(게임 함수도 안 부름)
LZ,SZ=layout(0,A1);w.w(SZ+0x130+0x10,struct.pack('<i',4));fr(LZ,0,400);fr(LZ,0,400);assert cnt(STY)==c0;print('칸 없음: 안 부름')
# 12) 브리지(R41f 에필로그): rbx=판, [rsp+0x30]=저장 rbx, add 0x20 뒤 pop rdi, ret. rax 등 보존
def go(entry,regs,rsp,stop):
    w.mu.reg_write(UC_X86_REG_RSP,rsp)
    for k,v in regs.items():w.mu.reg_write(k,v)
    w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    try:w.mu.emu_start(entry,stop,timeout=0,count=5_000_000)
    except UcError as e:print('UcError',e,hex(w.mu.reg_read(UC_X86_REG_RIP)));return False
    return w.mu.reg_read(UC_X86_REG_RIP)==stop
g=lambda r:w.mu.reg_read(r)
SP=(STACK_BASE+STACK_SIZE-0x20000)&~0xf
# 실제 게임 에필로그 바이트 확인
for rva in (0x1a7fc5,0x1a7f41):assert w.r(GAME_BASE+rva,11)==bytes.fromhex('488b5c24304883c4205fc3')
for B,k in((BC,0),(BF,1)):
    stop=w._tramp(b'\xf4');Lb,Sb=layout(k,A1 if k==0 else fd);f0=struct.unpack('<I',w.r(FRAMES,4))[0]
    rsp=SP-8;w.w64(rsp+0x30,0xB0B0);w.w64(rsp+0x20,0xD1D1);w.w64(rsp+0x28,stop)
    ok=go(B,{UC_X86_REG_RBX:Lb,UC_X86_REG_RAX:0x77,UC_X86_REG_RCX:0x99,UC_X86_REG_R12:0x55,UC_X86_REG_RDI:0x1234},rsp,stop)
    f1=struct.unpack('<I',w.r(FRAMES,4))[0]
    print('브리지',k,ok,'rbx',hex(g(UC_X86_REG_RBX)),'rdi',hex(g(UC_X86_REG_RDI)),'rax',hex(g(UC_X86_REG_RAX)),'frames',f1-f0)
    assert ok and g(UC_X86_REG_RBX)==0xB0B0 and g(UC_X86_REG_RDI)==0xD1D1 and g(UC_X86_REG_RAX)==0x77 and g(UC_X86_REG_R12)==0x55 and g(UC_X86_REG_RSP)==rsp+0x30 and f1==f0+1
d=[x[2] for x in w.msgs if 'R41D' in str(x) or 'frame_hook' in str(x)];print('기록',len(d),d[:3])
assert any('\tred\t' in x for x in d) and any('\tnormal\t' in x for x in d) and any('frame_hook_alive' in x for x in d)
print('warn_sets',struct.unpack('<I',w.r(WSETS,4))[0],'restores',struct.unpack('<I',w.r(RESTS,4))[0])
print('기록 줄:',[x for x in w.msgs if 'R39' in str(x)][:2])
print('ALL OK')
