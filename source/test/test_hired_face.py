# 32번 고용무장 원본 얼굴 — unicorn 단위 시험 (브리지 4개 + 배정 규칙)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_out/SAN14_HiredFace.dll'
fs={'SAN14_HiredFace.dll':open(DLL,'rb').read(),'SAN14Mods/modules/HiredFace.ini':b'[HiredFace]\r\nEnabled=1\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
base=w.api._loadlib('SAN14_HiredFace.dll')
if isinstance(base,str):  # DllMain 트램펄린
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain');base=w.modules['san14_hiredface.dll']['base']
m=w.modules['san14_hiredface.dll'];ex=m['exports']
# 가짜 Core API (Readable/InstallHooks = 1, MakeRelay = 게임 근처 주소)
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000   # 게임 이미지 바로 뒤(±2GB 안) — 매핑
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3')
free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r,w.stop_reason or '')
assert r==1,'ModInit failed'
tab=lambda i:w.run(ex['SAN14_HiredFaceTestTable'],(i,))
B1,B2,B3,B4=[tab(i) for i in range(4)];ASSIGN,RESCAN=tab(8),tab(9)
# 가짜 DB: 데이터 루트 → db, 무장 포인터 표 db+0x158, 고용 풀 db+0x861a0
db=w.alloc(0x90000);w.w64(GAME_BASE+0x277df18,db)
persons=[]
def person(pid,gender,face,new=False,status=4):
    p=w.alloc(0x200);w.w(p,bytes(0x200));w.w(p+0x10,struct.pack('<H',pid));w.w(p+0xa9,bytes([gender]));w.w(p+0xa6,struct.pack('<H',face));w.w(p+0xe6,bytes([status]))
    if pid<0x1771:w.w64(db+0x158+pid*8,p)
    persons.append(p);return p
face=lambda p:struct.unpack('<H',w.r(p+0xa6,2))[0]
# 기존 무장: 남 2000,2001 사용 중, 여 2050 사용 중
person(1,0,2000);person(2,0,2001);person(3,1,2050)
# 1) FaceAssign 직접: 남 범용 7800 → 2000 은 사용 중 → 다음 빈 것(시작은 표에서 7800→2000 찾음 → 2001 사용중 → 2002)
p=person(500,0,7800);w.run(ASSIGN,(p,7800));print('남 7800 ->',face(p));assert face(p)==2002
# 2) 여 범용 7900 → 2050 사용중 → 2051
q=person(501,1,7900);w.run(ASSIGN,(q,7900));print('여 7900 ->',face(q));assert face(q)==2051
# 3) 범용 아님(얼굴 300) → 해시 시작: (id*31)%101 ... 결과는 표 안 값이어야
r3=person(502,0,300);w.run(ASSIGN,(r3,300));f3=face(r3);print('남 300 ->',f3);assert 2000<=f3<=2163 and f3 not in (2000,2001,2002)
# 4) 성별 2(없음) → 그대로
r4=person(503,2,7800);w.run(ASSIGN,(r4,7800));assert face(r4)==7800
# 5) 브리지 1: 0x32f196 상황 재현 — rdi=person, ax=id, rsp 정렬, 복귀 주소 FaceRet1 을 우리 정지점으로
def run_bridge(entry,ret_slot,regs,count=50_000_000):
    stop=w._tramp(b'\xf4')  # hlt
    w.w64(ret_slot,stop)
    rsp=STACK_BASE+STACK_SIZE-0x20000;rsp&=~0xf  # 훅 지점 rsp%16==0 (함수 0x32f14e 프롤로그 뒤 상태)
    w.mu.reg_write(UC_X86_REG_RSP,rsp)
    for k,v in regs.items():w.mu.reg_write(k,v)
    w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    try:w.mu.emu_start(entry,stop,timeout=0,count=count)
    except UcError as e:print('UcError',e,hex(w.mu.reg_read(UC_X86_REG_RIP)));return False
    return w.mu.reg_read(UC_X86_REG_RIP)==stop and w.mu.reg_read(UC_X86_REG_RSP)==rsp
ret_slots=[tab(4+i) for i in range(4)]
p5=person(600,0,7805);w.w(p5+0x10,b'\0\0')
ok=run_bridge(B1,ret_slots[0],{UC_X86_REG_RDI:p5,UC_X86_REG_RAX:600,UC_X86_REG_RBX:600,UC_X86_REG_R15:db,UC_X86_REG_R12:0x1111,UC_X86_REG_R13:0x2222})
pid=struct.unpack('<H',w.r(p5+0x10,2))[0];print('브리지1: 복귀',ok,'id 저장',pid,'얼굴',face(p5),'rcx=rsp+0x20?',w.mu.reg_read(UC_X86_REG_RCX)==w.mu.reg_read(UC_X86_REG_RSP)+0x20,'r12/r13 보존',w.mu.reg_read(UC_X86_REG_R12)==0x1111 and w.mu.reg_read(UC_X86_REG_R13)==0x2222)
assert ok and pid==600 and face(p5)==2005
# 6) 브리지 2: dec [r15+8] + mov rax,rdi 재현 + 전체 훑기 (얼굴 4201 인 고용무장은 재배정)
cnt=w.alloc(0x10);w.w(cnt+8,struct.pack('<I',7))
p6=person(601,1,4201)
ok=run_bridge(B2,ret_slots[1],{UC_X86_REG_RDI:0x7777,UC_X86_REG_R15:cnt})
print('브리지2: 복귀',ok,'카운터',struct.unpack('<I',w.r(cnt+8,4))[0],'rax=rdi',w.mu.reg_read(UC_X86_REG_RAX)==0x7777,'4201 →',face(p6))
assert ok and struct.unpack('<I',w.r(cnt+8,4))[0]==6 and w.mu.reg_read(UC_X86_REG_RAX)==0x7777 and 2050<=face(p6)<=2166
# 7) 브리지 3: mov [r15+0x165],al 재현 + 배정(원래 0)
p7=person(602,0,0);w.w(p7+0x165,b'\0')
ok=run_bridge(B3,ret_slots[2],{UC_X86_REG_R15:p7,UC_X86_REG_RAX:0x33})
print('브리지3: 복귀',ok,'+0x165',w.r(p7+0x165,1)[0],'얼굴',face(p7));assert ok and w.r(p7+0x165,1)[0]==0x33 and 2000<=face(p7)<=2163
# 8) 브리지 4: [rdi+0x50]==0 && [rdi+0x20]==1 → 훑기; eax=1 유지
mgr=w.alloc(0x100);w.w(mgr,bytes(0x100));w.w(mgr+0x20,struct.pack('<I',1))
p8=person(603,0,4300)
ok=run_bridge(B4,ret_slots[3],{UC_X86_REG_RDI:mgr})
print('브리지4: 복귀',ok,'eax',w.mu.reg_read(UC_X86_REG_RAX),'4300 →',face(p8));assert ok and w.mu.reg_read(UC_X86_REG_RAX)==1 and 2000<=face(p8)<=2163
w.w(mgr+0x50,struct.pack('<I',5));p9=person(604,0,4301);ok=run_bridge(B4,ret_slots[3],{UC_X86_REG_RDI:mgr});print('브리지4(오류 상태): eax',w.mu.reg_read(UC_X86_REG_RAX),'4301 그대로',face(p9));assert ok and w.mu.reg_read(UC_X86_REG_RAX)==0 and face(p9)==4301
# 9) 끔 → 배정 안 함
w.run(ex['SAN14_HiredFaceSetEnabled'],(0,));p10=person(605,0,7800);w.run(ASSIGN,(p10,7800));assert face(p10)==7800;print('끔 상태 그대로',face(p10))
# 10) 전부 사용 중이면 2500/2501
w.run(ex['SAN14_HiredFaceSetEnabled'],(1,))
for i,f in enumerate(range(2050,2167)):
    if f in (2050,2051): continue
    person(700+i,1,f)
# 여 표 66장 모두 사용: 2050(기존) 2051(q) + 나머지
p11=person(900,1,7900);w.run(ASSIGN,(p11,7900));print('여 전부 사용 →',face(p11));assert face(p11)==2501
# R40 A16: 죽은(신분 7) 무장의 얼굴은 비어 있는 것으로 셈 + 대체 얼굴(2501)에 묶인 고용무장은 빈 얼굴이 생기면 재배정
w.run(ex['SAN14_HiredFaceSetEnabled'],(1,))
hired=person(4250,1,0x9c5);w.w64(db+0x158+4250*8,hired)
w.run(RESCAN,());print('R40 빈 얼굴 없음: 고용무장 4250 그대로',face(hired));assert face(hired)==0x9c5
holder=[p for p in persons if struct.unpack('<H',w.r(p+0xa6,2))[0]==2066 and p!=hired][0];w.w(holder+0xe6,bytes([7]))  # 2066 쓰던 여무장 사망
w.run(RESCAN,());print('R40 사망으로 빈 얼굴 → 고용무장 4250',face(hired));assert face(hired)==2066
p12=person(901,1,7900,status=4);w.run(ASSIGN,(p12,7900));print('R40 다시 전부 사용 →',face(p12));assert face(p12)==0x9c5
print('기록 줄:',[m for m in w.msgs if 'R39' in str(m)][:3])
print('ALL OK')
