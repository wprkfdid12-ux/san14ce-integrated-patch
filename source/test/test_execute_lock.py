# R42 B4 26번 처단불가 → 포로 처우 창 처단 버튼 회색 — unicorn 단위 시험
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL=os.environ.get('EXECLOCK_DLL','/home/claude/r41/build_out/SAN14_ExecuteLock.dll')
fs={'SAN14_ExecuteLock.dll':open(DLL,'rb').read(),'SAN14Mods/modules/ExecuteLock.ini':b'[ExecuteLock]\r\nEnabled=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
ok=True
def check(c,msg):
    global ok
    print(('PASS ' if c else 'FAIL ')+msg);ok=ok and c
G=GAME_BASE
check(w.r(G+0x66ff4e,5)==bytes.fromhex('488b5c2460') and w.r(G+0x66ff53,5)==bytes.fromhex('488b6c2468'),'게임 0x66ff4e(0x66fd80 끝) 원래 바이트')
check(w.r(G+0x66fec0,3)==bytes.fromhex('488b0b') and w.r(G+0x66feb6,7)==bytes.fromhex('498d9d80030000'),'게임 버튼 고리: 창+0x380 부터 4개(0x66feb6)')
base=w.api._loadlib('SAN14_ExecuteLock.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_executelock.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3');free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');check(r==1,'ModInit 설치(게임 0x66ff4e 한 곳)')
T=lambda op,a=0,b=0:w.run(ex['SAN14_ExecuteLockTest'],(op,a,b))
# 가짜 버튼: vt+0xd0 = 받은 edx 를 기록
rec=w.alloc(16)
def mkbtn():
    b=w.alloc(0x100);vt=w.alloc(0x100);w.w(b,bytes(0x100))
    code=b'\x48\xb8'+struct.pack('<Q',rec)+b'\x89\x10'+b'\x48\x89\x48\x08'+b'\xc3'   # mov rax,rec; mov [rax],edx; mov [rax+8],rcx; ret
    w.w64(vt+0xd0,w._tramp(code));w.w64(b,vt);return b
def mklayout(vt):
    L=w.alloc(0x400);w.w(L,bytes(0x400));w.w64(L,vt)
    btns=[mkbtn() for _ in range(4)]
    for i,bb in enumerate(btns):w.w64(L+0x380+i*8,bb)
    return L,btns
reset=lambda:(w.w32(rec,0x77),w.w64(rec+8,0))
L,btns=mklayout(G+0x142def0)
T(3,1);reset();T(4,L)
check(w.r32(rec)==0 and w.r64(rec+8)==btns[1],'처단불가 켜짐: 게임 포로 처우 창 처단 버튼(창+0x388)만 끔(vt+0xd0, 0)')
T(3,0);reset();T(4,L)
check(w.r32(rec)==0x77,'처단불가 꺼짐: 손대지 않음(게임 원래 규칙)')
T(3,1);L2,_=mklayout(G+0x13999b8);reset();T(4,L2)
check(w.r32(rec)==0x77,'다른 창(vtable 다름)은 손대지 않음')
reset();T(4,0);check(w.r32(rec)==0x77,'창 없음(0) → 안전')
w.run(ex['SAN14_ExecuteLockSetEnabled'],(0,));reset();T(4,L);check(w.r32(rec)==0x77,'모듈 끄면 손대지 않음')
w.run(ex['SAN14_ExecuteLockSetEnabled'],(1,))
# 브리지: r13=창, 원래 mov rbx,[rsp+0x60] 실행 후 0x66ff53 로, 다른 레지스터 보존
stop=w._tramp(b'\xf4');T(10,stop);BR=T(9)
SP=(STACK_BASE+STACK_SIZE-0x20000)&~0xf;rsp=SP-0x40;w.w64(rsp+0x60,0xB1B1)
w.mu.reg_write(UC_X86_REG_RSP,rsp)
regs={UC_X86_REG_R13:L,UC_X86_REG_RCX:0x1111,UC_X86_REG_RDX:0x2222,UC_X86_REG_RAX:0x3333,UC_X86_REG_R8:0x88,UC_X86_REG_RSI:0x55,UC_X86_REG_RDI:0x66,UC_X86_REG_R14:0x1414}
for k,v in regs.items():w.mu.reg_write(k,v)
w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE);reset()
w.mu.emu_start(BR,stop,timeout=0,count=2_000_000)
g=w.mu.reg_read
check(g(UC_X86_REG_RIP)==stop and g(UC_X86_REG_RSP)==rsp and g(UC_X86_REG_RBX)==0xB1B1 and all(g(k)==v for k,v in regs.items()),'브리지: 원래 명령 실행 후 0x66ff53, rsp·레지스터 보존')
check(w.r32(rec)==0 and w.r64(rec+8)==btns[1],'브리지 경유로도 처단 버튼 끔')
# IC(7번 포로 회유 창): 실제 배포 IC 바이트에서 설치, 감싼 함수 동작
ICP='/home/claude/r41/pkg/support/payload/base/SAN14_IntegratedConvenience.dll'
raw=open(ICP,'rb').read();IC=0x3a0000000;w.mu.mem_map(IC,0x220000,UC_PROT_ALL)
e=struct.unpack_from('<I',raw,0x3c)[0];ns=struct.unpack_from('<H',raw,e+6)[0];so=struct.unpack_from('<H',raw,e+20)[0]
for i in range(ns):
    o=e+24+so+i*40;vs,va,rs,ra=struct.unpack_from('<IIII',raw,o+8)
    if va+min(rs,max(vs,rs))<=0x220000:w.w(IC+va,raw[ra:ra+min(rs,0x220000-va)])
T(2,IC)
check(T(5,IC)==1 and w.r(IC+0x6acd0,1)==b'\xe9','IC(ae365eab) 0x18006acd0 입구 감싸기 설치')
calls=w.alloc(8);w.w64(calls,0)
orig=w._tramp(b'\x48\xb8'+struct.pack('<Q',calls)+b'\x48\xff\x00\xb8\x05\x00\x00\x00\xc3')  # inc [calls]; return 5
T(7,orig);WR=T(6)
Li,bi=mklayout(IC+0x106130);w.w64(IC+0x105050,Li);w.w32(IC+0x105058,2)
T(3,1);reset();r=w.run(WR,(0,0,0,0))
check(r==5 and w.r64(calls)==1 and w.r32(rec)==0 and w.r64(rec+8)==bi[1],'IC 창 활성(2): 원래 IC 처리 뒤 처단 버튼 끔, 반환값 그대로')
w.w32(IC+0x105058,1);reset();w.run(WR,(0,0,0,0))
check(w.r64(calls)==2 and w.r32(rec)==0x77,'IC 창 아님: 원래만 실행')
w.w32(IC+0x105058,2);T(3,0);reset();w.run(WR,(0,0,0,0))
check(w.r32(rec)==0x77,'처단불가 꺼짐: IC 창도 손대지 않음')
IC2=0x3d0000000;w.mu.mem_map(IC2,0x220000,UC_PROT_ALL);w.w(IC2,w.r(IC,0x220000));w.w(IC2+0x6acd0,raw[0x6acd0-0x1000+0x400:0x6acd0-0x1000+0x400+16]);w.w(IC2+0x67903,b'\x90')
before=w.r(IC2+0x6acd0,16)
check(T(5,IC2)==0 and w.r(IC2+0x6acd0,16)==before,'IC 바이트 다르면 안 바꿈')
print('ALL OK' if ok else 'SOME FAILED')
