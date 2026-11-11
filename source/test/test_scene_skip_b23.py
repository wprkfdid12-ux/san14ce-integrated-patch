# R42 B2(등용 대사 장면)·B3(탐색 명품 발견 창) 생략 — unicorn 단위 시험
#  시험 DLL = scene_skip_b23.cpp + 브리지 + test/b23_testmain.cpp(가짜 SsAddHistory·가짜 게임 메시지 함수)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_b23/SAN14_SceneSkipB23Test.dll'
fs={'SAN14_SceneSkipB23Test.dll':open(DLL,'rb').read()}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
ENTRY=bytes.fromhex('4057 4883ec70'.replace(' ',''))
assert w.r(GAME_BASE+0x6c43b0,6)==ENTRY and w.r(GAME_BASE+0x6c4670,6)==ENTRY,'게임 입구 바이트'
# 게임 원본에서 두 자리가 정말 등용 장면/발견 창을 쌓는지(쌓는 함수·상태 vtable) 한 번 더 확인
d=w.r(GAME_BASE+0x6c43b0,0x92);assert b'\xe8' in d
def call_target(at):
    rel=struct.unpack('<i',w.r(GAME_BASE+at+1,4))[0];return at+5+rel
assert call_target(0x6c4427)==0x6873c0 and call_target(0x6c46e7)==0x6863d0
assert struct.unpack('<i',w.r(GAME_BASE+0x68743a+3,4))[0]+0x68743a+7==0x1440ff8   # CStrategyEmployTalkState
assert struct.unpack('<i',w.r(GAME_BASE+0x6a4185+3,4))[0]+0x6a4185+7==0x14471e8   # CSearchDiscoveryState
for at in (0x1db340,0x1db6ef,0x1dbbd0,0x1dbf6a):assert call_target(at)==0x6c43b0
for at in (0x1dc6b5,0x1dc97d,0x1dcfcf,0x26f646,0x26f9ea,0x42276d):assert call_target(at)==0x6c4670
base=w.api._loadlib('SAN14_SceneSkipB23Test.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_sceneskipb23test.dll'];ex=m['exports']
# 가짜 게임 메시지 함수 연결(movabs rax,주소; jmp rax)
stubs={0x30a2f0:'S_Reset',0x30dbb0:'S_Person',0x30dc20:'S_Item',0x30d9d0:'S_Area',0x30d8c0:'S_City',0x195de0:'S_FmtObj',0x196b50:'S_Fmt',
       0x22ca00:'S_Force',0x22e970:'S_Lord',0x22d9e0:'S_AreaId',0x3f5a0:'S_CityOf'}
for rva,name in stubs.items():w.w(GAME_BASE+rva,b'\x48\xb8'+struct.pack('<Q',ex[name])+b'\xff\xe0')
fvt=w.alloc(0x40);w.w64(fvt+8,ex['S_Tmpl']);w.w64(GAME_BASE+0x1a23440,fvt)
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3');free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r);assert r==1
T=lambda i:w.run(ex['T_Table'],(i,))
EB,TB,EF,TF,ES,TS,ER,TR,TXF=[T(i) for i in range(9)]
assert w.r64(ER)==GAME_BASE+0x6c43b0+6 and w.r64(TR)==GAME_BASE+0x6c4670+6
rd32=lambda a:struct.unpack('<I',w.r(a,4))[0]
hist_n=lambda:rd32(w.run(ex['T_Hist'],(-1,)))
def hist(i):
    a=w.run(ex['T_Hist'],(i,));s=w.r(a,1024);return s.decode('utf-16le').split('\0')[0]
def person(pid):
    p=w.alloc(0x200);w.w(p,bytes(0x200));w.w(p+0x10,struct.pack('<H',pid));return p
actor,target=person(100),person(200)
def emp(a,b,ok):
    p=w.alloc(0x40);w.w(p,struct.pack('<QQI',a,b,ok)+bytes(0x14));return p
# B2 성공
n0=hist_n();assert w.run(EF,(emp(actor,target,1),))==1;assert hist_n()==n0+1
h=hist(n0);print('B2 성공 →',h);assert h=='TC3F1 {P100@0,P200@1,}'
assert w.run(EF,(emp(actor,target,0),))==1;h=hist(n0+1);print('B2 실패 →',h);assert h=='TC3F2 {P100@0,P200@1,}'
assert rd32(ES)==2
# 끄면 원래대로(장면 그대로), 기록 없음
w.run(ex['T_SetEn'],(0,0));n1=hist_n();assert w.run(EF,(emp(actor,target,1),))==0 and hist_n()==n1;print('B2 끔 → 원래 장면')
w.run(ex['T_SetEn'],(0,1))
# 모양 이상(실행자 없음) → 원래대로
assert w.run(EF,(emp(0,target,1),))==0 and w.run(EF,(0,))==0;print('B2 인자 이상 → 원래대로')
# 문장 틀이 없으면: 장면은 빼되(결과·게임 자체 기록은 원래대로) 우리 줄은 안 넣음
w.run(ex['T_TmplNull'],(1,));n2=hist_n();assert w.run(EF,(emp(actor,target,1),))==1 and hist_n()==n2 and rd32(TXF)==1;w.run(ex['T_TmplNull'],(0,))
print('B2 문장 틀 없음 → 장면만 빼고 줄 생략')
# B3 명품(종류 0)
item=w.alloc(0x100);w.w(item,bytes(0x100));w.w(item+0x10,struct.pack('<H',0x1234));w.w(item+0x4a,struct.pack('<H',100))
place=w.alloc(0x40);w.w(place,bytes(0x40))
def disc(kind,a,pl,it):
    p=w.alloc(0x40);w.w(p,struct.pack('<IIQQQ',kind,0,a,pl,it)+bytes(0x1c));return p
n3=hist_n();assert w.run(TF,(disc(0,actor,place,item),))==1;h=hist(n3);print('B3 명품 →',h)
assert h=='T51BD {P100@0,P77@1,A55@0,I4660@0,C33@0,}' and rd32(TS)==1
# 곳 없으면 제목 문장(누가·무슨 명품)으로
assert w.run(TF,(disc(0,actor,0,item),))==1;h=hist(n3+1);print('B3 곳 없음 →',h);assert h=='TC39D {P100@0,I4660@0,}'
# 다른 종류(1 탐색 인재, 3·4, 5 명승 영상 뒤)는 원래대로
for k in (1,3,4,5):n4=hist_n();assert w.run(TF,(disc(k,actor,place,item),))==0 and hist_n()==n4
print('B3 종류 1·3·4·5 → 원래대로')
w.run(ex['T_SetEn'],(1,0));assert w.run(TF,(disc(0,actor,place,item),))==0;w.run(ex['T_SetEn'],(1,1));print('B3 끔 → 원래 창')
assert w.run(TF,(disc(0,actor,place,0),))==0;print('B3 명품 없음 → 원래대로')
# 브리지: 건너뛸 때는 바로 ret(스택·보존 레지스터 그대로), 아닐 때는 원래 6바이트(push rdi; sub rsp,0x70) 실행 후 +6
stop=w._tramp(b'\xf4');back=w._tramp(b'\xf4')
w.w64(ER,stop);w.w64(TR,stop)
SP=(STACK_BASE+STACK_SIZE-0x20000)&~0xf
def bridge(br,arg):
    rsp=SP-8;w.w64(rsp,back)
    w.mu.reg_write(UC_X86_REG_RSP,rsp)
    for k,v in {UC_X86_REG_RCX:arg,UC_X86_REG_RDX:0x5555,UC_X86_REG_RDI:0xB3,UC_X86_REG_RBX:0xB1,UC_X86_REG_RSI:0xB2,UC_X86_REG_R8:0x88,UC_X86_REG_R9:0x99,UC_X86_REG_R12:0xC12,UC_X86_REG_R15:0xC15}.items():w.mu.reg_write(k,v)
    w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    end=w.mu.reg_read  #
    try:w.mu.emu_start(br,0,timeout=0,count=3_000_000)
    except Exception as e:pass
    g=w.mu.reg_read;return g,rsp
def run_to_hlt(br,arg):
    rsp=SP-8;w.w64(rsp,back)
    w.mu.reg_write(UC_X86_REG_RSP,rsp)
    for k,v in {UC_X86_REG_RCX:arg,UC_X86_REG_RDX:0x5555,UC_X86_REG_RDI:0xB3,UC_X86_REG_RBX:0xB1,UC_X86_REG_RSI:0xB2,UC_X86_REG_R8:0x88,UC_X86_REG_R9:0x99,UC_X86_REG_R12:0xC12,UC_X86_REG_R15:0xC15}.items():w.mu.reg_write(k,v)
    w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    hits=[]
    def onc(mu,addr,size,ud):
        if addr in (stop,back):hits.append(addr);mu.emu_stop()
    hk=w.mu.hook_add(UC_HOOK_CODE,onc)
    w.mu.emu_start(br,0,timeout=0,count=3_000_000);w.mu.hook_del(hk)
    return w.mu.reg_read,rsp,hits
def keep(g):
    return g(UC_X86_REG_RDX)==0x5555 and g(UC_X86_REG_RBX)==0xB1 and g(UC_X86_REG_RSI)==0xB2 and g(UC_X86_REG_RDI)==0xB3 and g(UC_X86_REG_R8)==0x88 and g(UC_X86_REG_R9)==0x99 and g(UC_X86_REG_R12)==0xC12 and g(UC_X86_REG_R15)==0xC15
for br,arg,name in ((EB,emp(actor,target,1),'B2'),(TB,disc(0,actor,place,item),'B3')):
    g,rsp,hits=run_to_hlt(br,arg)
    print(name,'브리지 건너뜀:',[hex(x) for x in hits],hex(g(UC_X86_REG_RSP)))
    assert hits==[back] and g(UC_X86_REG_RSP)==rsp+8 and keep(g) and g(UC_X86_REG_RCX)==arg
w.run(ex['T_SetEn'],(0,0));w.run(ex['T_SetEn'],(1,0))
for br,arg,name in ((EB,emp(actor,target,1),'B2'),(TB,disc(0,actor,place,item),'B3')):
    g,rsp,hits=run_to_hlt(br,arg)
    print(name,'브리지 원래대로:',[hex(x) for x in hits],hex(g(UC_X86_REG_RSP)),hex(w.r64(rsp-8)))
    assert hits==[stop] and g(UC_X86_REG_RSP)==rsp-8-0x70 and w.r64(rsp-8)==0xB3 and keep(g) and g(UC_X86_REG_RCX)==arg
w.run(ex['T_SetEn'],(0,1));w.run(ex['T_SetEn'],(1,1))
# 종류 1(인재)은 켜져 있어도 브리지가 원래대로
g,rsp,hits=run_to_hlt(TB,disc(1,actor,place,item));assert hits==[stop];print('B3 브리지 종류1 → 원래대로')
logs=[f for f in w.fs_written if '.log' in f]
txt=b''.join(w.fs_written[f] for f in logs).decode('utf-8','replace')
print('기록 줄:',[l for l in txt.split('\n') if 'SceneSkip' in l][:4])
assert 'employ_skip_ok' in txt and 'treasure_skip' in txt and 'employ_scene_skipped' in txt
print('ALL OK')
