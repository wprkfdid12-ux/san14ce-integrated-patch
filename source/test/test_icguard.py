# R40 통합편의 장기 안정(ICGuard) — unicorn 단위 시험: 창 고착 해제, 처단불가 동굴, 문전박대 본거지, 자동 논공 무장별 제외
import sys,struct,os
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
import os
DLL=os.environ.get('ICGUARD_DLL','/home/claude/r41/build_out/SAN14_ICGuard.dll')
fs={'SAN14_ICGuard.dll':open(DLL,'rb').read()}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
base=w.api._loadlib('SAN14_ICGuard.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_icguard.dll'];ex=m['exports']
T=lambda op,a=0,b=0:w.run(ex['SAN14_ICGuardTest'],(op,a,b))
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',0,ret1,ret1,0,0,0))  # Readable=1, Log=0
# 가짜 IC 모듈 영역(필요한 자리만 씀)
IC=0x300000000;w.mu.mem_map(IC,0x220000,UC_PROT_ALL);w.w(IC,b'\xcc'*0x220000)
T(1,IC,core)
b8=lambda a:w.r(a,1)[0]; u32=lambda a:struct.unpack('<I',w.r(a,4))[0]
ok=True
def check(c,msg):
    global ok
    print(('PASS ' if c else 'FAIL ')+msg);ok=ok and c
# 1) 창 고착: 가짜 원래 함수 = 표시 켜고 포인터는 안 씀(실패) / 포인터도 씀(성공)
DLG=[(0x19e99e,1,0x1a2c48),(0x1c07fc,1,0x1c0fd0),(0x105058,4,0x105048),(0x1c076d,1,0x1a4ef0),(0x1c07fe,1,0x106a50)]
for k,(flag,size,ptr) in enumerate(DLG):
    setf=(b'\xc6\x05' if size==1 else b'\xc7\x05')
    def stub(write_ptr):
        code=b'\x48\xb8'+struct.pack('<Q',IC+flag)+(b'\xc6\x00\x01' if size==1 else b'\xc7\x00\x01\x00\x00\x00')
        if write_ptr:code+=b'\x48\xb8'+struct.pack('<Q',IC+ptr)+b'\x48\xc7\x00\x34\x12\x00\x00'
        return w._tramp(code+b'\x31\xc0\xc3')
    wr=T(4,k)
    w.w(IC+flag,bytes(size));w.w(IC+ptr,bytes(8))
    T(3,k,stub(False));w.run(wr,(0,0,0,0))
    check((b8(IC+flag) if size==1 else u32(IC+flag))==0,f'창 {k}: 창 못 만들면 "열고 있음" 표시를 되돌림')
    w.w(IC+flag,bytes(size));w.w(IC+ptr,bytes(8))
    T(3,k,stub(True));w.run(wr,(0,0,0,0))
    check((b8(IC+flag) if size==1 else u32(IC+flag))==1,f'창 {k}: 창을 만들면 표시 그대로(원래 소멸자가 끔)')
    w.w(IC+ptr,bytes(8))  # 이미 열려 있는 중(표시 1)에 다시 눌러도 건드리지 않음
    T(3,k,w._tramp(b'\x31\xc0\xc3'));w.run(wr,(0,0,0,0))
    check((b8(IC+flag) if size==1 else u32(IC+flag))==1,f'창 {k}: 이미 열려 있으면 표시를 건드리지 않음(중복 열기 막기 유지)')
# 2) 처단불가 동굴: 0x23f04 je 를 동굴로, 켜짐이면 '표시 있음' 길, 꺼짐이면 원래 처단 길
w.w(IC+0x23f04,bytes([0x0f,0x84,0xc6,0,0,0]));w.w(IC+0x23fd0,b'\xf4');w.w(IC+0x23f0a,b'\xf4')
check(T(5)==1,'처단불가: 0x23f04 원래 바이트 확인 후 동굴 연결')
flag=T(6)
def land(on):
    w.w(flag,bytes([on]));w.mu.reg_write(UC_X86_REG_EFLAGS,0x246)  # ZF=1 → je 탐
    rsp=STACK_BASE+STACK_SIZE-0x20000;w.mu.reg_write(UC_X86_REG_RSP,rsp)
    try:w.mu.emu_start(IC+0x23f04,0,count=100)
    except UcError:pass
    return w.mu.reg_read(UC_X86_REG_RIP)-IC
r0=land(0);r1=land(1)
check(r0 in (0x23fd0,0x23fd1),'처단불가 꺼짐: 원래대로 처단 길(0x23fd0) %x'%r0)
check(r1 in (0x23f0a,0x23f0b),'처단불가 켜짐: 처단하지 않는 길(0x23f0a) %x'%r1)
# 3) 문전박대 본거지: 원래 판정 실패 → force+0x160 도시
G=0x340000000;w.mu.mem_map(G,0x1500000,UC_PROT_ALL);w.w(G,b'\xcc'*0x100)
T(2,G)
city=w.alloc(0x100);w.w64(city,G+0x13a96d0)
w.w(G+0x32e3b0,b'\x48\xb8'+struct.pack('<Q',city)+b'\xc3')   # 0x32e3b0(db,id) → city
w.w(G+0x331780,b'\xb8\x01\x00\x00\x00\xc3')                  # 유효
force=w.alloc(0x200);w.w64(force,G+0x13a9880);w.w(force+0x160,bytes([7]))
ctx=w.alloc(0x40);db=w.alloc(0x40);w.w64(ctx+8,db);out=w.alloc(8);w.w64(out,0)
wr=T(7,w._tramp(b'\x31\xc0\xc3'))
r=w.run(wr,(ctx,force,out,0))
check((r&0xff)==1 and struct.unpack('<Q',w.r(out,8))[0]==city,'문전박대: 군주가 도시 아닌 곳이면 본거지 도시로 대체')
w.w64(force,G+0x1234);w.w64(out,0);r=w.run(wr,(ctx,force,out,0))
check((r&0xff)==0,'문전박대: 세력 객체가 아니면 원래대로 실패(추측 안 함)')
# 4) 자동 논공: 결과가 다른 무장만 제외, 제외된 무장은 후보 검사에서 0
p1=w.alloc(0x200);p2=w.alloc(0x200)
for p,i in ((p1,101),(p2,102)):w.w(p+0x10,struct.pack('<H',i))
w.w(p1+0x1bc,bytes([3]));w.w(p1+0x1c2,bytes([1,0]));w.w(p1+0x15e,struct.pack('<H',0x8000))   # 예측대로
w.w(p2+0x1bc,bytes([2]));w.w(p2+0x1c2,bytes([0,0]));w.w(p2+0x15e,struct.pack('<H',0))        # 예측과 다름
vec=w.alloc(0x50+0x18);arr=vec+0x18
for j,(p,rank,up,lo) in enumerate(((p1,3,1,0),(p2,3,1,0))):
    e=arr+j*0x28;w.w(e,bytes(0x28));w.w64(e,p);w.w(e+0x10,struct.pack('<I',rank));w.w(e+0x1c,struct.pack('<I',up));w.w(e+0x20,struct.pack('<I',lo))
w.w64(vec,arr);w.w64(vec+8,arr+0x50)
wr=T(9,w._tramp(b'\x31\xc0\xc3'))
r=w.run(wr,(0,vec,0,0))
check((r&0xff)==0 and T(10,102)==1 and T(10,101)==0,'자동 논공 실패: 예측과 다른 무장(102)만 제외, 맞는 무장(101)은 그대로')
called=w.alloc(8);w.w64(called,0)
orig=w._tramp(b'\x48\xb8'+struct.pack('<Q',called)+b'\x48\xc7\x00\x01\x00\x00\x00\xb8\x01\x00\x00\x00\xc3')
wc=T(8,orig)
check((w.run(wc,(0,p2,0,0))&0xff)==0 and struct.unpack('<Q',w.r(called,8))[0]==0,'제외된 무장은 자동 논공 후보 아님(원래 검사도 안 부름)')
check((w.run(wc,(0,p1,0,0))&0xff)==1 and struct.unpack('<Q',w.r(called,8))[0]==1,'다른 무장은 원래 검사 그대로')
for _ in range(6):T(12)
w.w64(called,0)
check((w.run(wc,(0,p2,0,0))&0xff)==1,'6턴 뒤에는 다시 후보(영구 제외 아님)')
print('ALL OK' if ok else 'SOME FAILED')

# ---------- R42 22번 C: DomesticCE 3초 틱 지역별 건너뛰기 ----------
ok22=True
def chk(c,msg):
    global ok22
    print(('PASS ' if c else 'FAIL ')+'[22] '+msg);ok22=ok22 and c
from pe import PE as _PE
DCE_PATH='/home/claude/r41/pkg/support/payload/f22/SAN14_DomesticCE.dll'
raw=open(DCE_PATH,'rb').read()
DCE=0x390000000;w.mu.mem_map(DCE,0x40000,UC_PROT_ALL)
e=struct.unpack_from('<I',raw,0x3c)[0];ns=struct.unpack_from('<H',raw,e+6)[0];so=struct.unpack_from('<H',raw,e+20)[0]
for i in range(ns):
    o=e+24+so+i*40;vs,va,rs,ra=struct.unpack_from('<IIII',raw,o+8)
    if va+max(vs,rs)<=0x40000:w.w(DCE+va,raw[ra:ra+rs])
orig={a:w.r(DCE+a,n) for a,n in ((0x15f4,4),(0x160f,8),(0x1617,7),(0x163b,8))}
chk(T(15,DCE)==1,'배포본 DomesticCE(a1cd31a7) 바이트 확인 후 4곳 설치')
chk(w.r(DCE+0x15f6,2)==b'\x90\x90' and w.r(DCE+0x1611,6)==b'\x66\x0f\x1f\x44\x00\x00','R40 두 자리(첫 고리 끝까지·틱 중단 안 함) 그대로')
chk(w.r(DCE+0x1617,1)==b'\xe9' and w.r(DCE+0x163b,1)==b'\xe9','둘째 고리 앞(0x1617)·머리(0x163b) 우회 연결')
# 가짜 게임: db, 지역 표 501칸(0x98 간격), P = 지역 j0 + 0x51510
G2=0x3a0000000;w.mu.mem_map(G2,0x2a00000,UC_PROT_ALL)
T(2,G2)
DB=w.alloc(0x88000,0x1000);w.w(DB,bytes(0x88000));w.w64(G2+0x277df18,DB)
AR=w.alloc(8+501*0x98,0x1000);w.w(AR,bytes(8+501*0x98))
area=lambda k:AR+8+k*0x98
for k in range(501):w.w64(DB+0x6caf0+k*8,area(k))
J0=1;P=area(J0)+0x51510
def setv(k,f=300,c=400,s=500,foc=1,f78=0x0005):
    a=area(k);w.w(a+0x3c,struct.pack('<HHH',f,c,s));w.w8(a+0x72,foc);w.w16(a+0x78,f78)
for i in range(338):setv(J0+i)
chk(T(13,P)==1 and (T(16)&0xff)==1 and ((T(16)>>8)&0xffffff)==J0,'대상 확인: 338곳이 게임 지역 표와 같음 → 지역별 모드, j0=%d'%J0)
rsi=lambda i:(-0x514d4+i*0x98)&0xffffffffffffffff
chk(T(14,P,rsi(0))==1 and T(14,P,rsi(337))==1,'정상 지역(첫·끝)은 처리')
setv(J0+10,f=1500);setv(J0+11,foc=7);setv(J0+12,f78=0x7f05)
chk(T(14,P,rsi(10))==0,'개발값 1000 넘는 지역은 그 지역만 건너뜀')
chk(T(14,P,rsi(11))==0,'중점 칸 4 이상인 지역은 그 지역만 건너뜀')
chk(T(14,P,rsi(12))==1,'+0x79 빈칸 쓰레기값만 있는 지역은 처리(처리에 안 쓰는 칸)')
chk(T(14,P,rsi(13))==1,'옆 지역은 계속 처리')
chk(T(13,P)==1,'범위 밖 지역이 있어도 틱 전체는 진행(지역별 모드)')
# 대상 불일치 → 원래 규칙
w.w64(DB+0x6caf0+(J0+100)*8,area(J0+100)+0x10)
chk(T(13,P)==0 and (T(16)&0xff)==2,'지역 표와 안 맞으면 원래 모드: 범위 밖 하나라도 있으면 틱 건너뜀')
setv(J0+10);setv(J0+11);setv(J0+12)
chk(T(13,P)==1,'원래 모드: 전부 범위 안이면 진행')
chk(T(14,P,rsi(10))==1,'원래 모드에선 지역 머리 검사 안 함(원래와 같음)')
w.w64(DB+0x6caf0+(J0+100)*8,area(J0+100))
# 고리 머리 흐름: 실제 바뀐 바이트에서 시작해 어디로 가는지
FR=w.alloc(0x200);w.w64(FR+0x68,P)
def flow(start,rsi_v):
    stops={DCE+0x1627:'next',DCE+0x1643:'body',DCE+0x161e:'loop2',DCE+0x1bcc:'abort'}
    res=['?']
    def hk(mu,addr,size,ud):
        if addr in stops:res[0]=stops[addr];mu.emu_stop()
    h=w.mu.hook_add(UC_HOOK_CODE,hk)
    rsp=STACK_BASE+STACK_SIZE-0x20000;w.mu.reg_write(UC_X86_REG_RSP,rsp);w.mu.reg_write(UC_X86_REG_RBP,FR);w.mu.reg_write(UC_X86_REG_RSI,rsi_v)
    w.mu.reg_write(UC_X86_REG_RBX,0x1234);w.mu.reg_write(UC_X86_REG_R14,0x5555);w.mu.reg_write(UC_X86_REG_RDI,0x7777)
    try:w.mu.emu_start(DCE+start,0,count=20000)
    except UcError as ex:res[0]='err %s'%ex
    w.mu.hook_del(h)
    return res[0],w.mu.reg_read(UC_X86_REG_RBX),w.mu.reg_read(UC_X86_REG_R13),w.mu.reg_read(UC_X86_REG_RSP)==rsp
T(13,P)
r=flow(0x163b,rsi(5));chk(r[0]=='body' and r[1]==P and r[2]==(P+rsi(5))&0xffffffffffffffff and r[3],'머리: 정상 지역 → 원래 두 명령 실행 후 0x1643 (rbx=P, r13=P+rsi, 스택 그대로) %s'%(r[0],))
setv(J0+5,s=2000);r=flow(0x163b,rsi(5));chk(r[0]=='next' and r[3],'머리: 범위 밖 지역 → 다음 지역(0x1627) %s'%(r[0],));setv(J0+5)
w.w32(FR+0x74,0xdead);r=flow(0x1617,0);chk(r[0]=='loop2' and w.r32(FR+0x74)==0 and r[3],'앞: 대상 맞음 → [rbp+0x74]=0 후 둘째 고리(0x161e)')
w.w64(DB+0x6caf0+(J0+100)*8,0);setv(J0+20,c=5000);w.w32(FR+0x74,0xdead)
r=flow(0x1617,0);chk(r[0]=='abort' and w.r32(FR+0x74)==0xdead,'앞: 대상 불일치+범위 밖 → 틱 건너뜀(0x1bcc)')
w.w64(DB+0x6caf0+(J0+100)*8,area(J0+100));setv(J0+20)
# 다른 판(바이트 다름)이면 아무것도 안 바꿈
DCE2=0x3d0000000;w.mu.mem_map(DCE2,0x40000,UC_PROT_ALL);w.w(DCE2,w.r(DCE,0x40000))
for a,b in orig.items():w.w(DCE2+a,b)
w.w(DCE2+0x163b,b'\x90')
before=w.r(DCE2,0x2000)
chk(T(15,DCE2)==0 and w.r(DCE2,0x2000)==before,'바이트 다른 DomesticCE: 하나도 안 바꿈')
print('22 ALL OK' if ok22 else '22 SOME FAILED')
