import os
# R39 1번 새 모드 설정 창 — unicorn 단위 시험: 훅 A(get)/B(set) 다리 + D/E/IC 클릭 다리 + F(표 채우기)/C(바인드 보정) 몸통
import sys,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_out/SAN14_ModSettings.dll'
fs={'SAN14_ModSettings.dll':open(DLL,'rb').read(),
    'SAN14Mods/modules/ModSettings.ini':b'[ModSettings]\r\nUseNewWindow=1\r\n',
    'SAN14_WheelRegion.dll':b'MZ','SAN14_WheelRegion.ini':b'[WheelRegion]\r\nEnabled=1\r\n',
    'SAN14_DragGive.dll':b'MZ','SAN14_DragGive.ini':b'[DragGive]\r\nEnabled=0\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
base=w.api._loadlib('SAN14_ModSettings.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_modsettings.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
near=GAME_BASE+0x29ff000+0x100000
w.mu.mem_map(near,0x10000,UC_PROT_ALL);w.w(near,b'\xc3'*0x10000)
relay=w._tramp(b'\x48\xb8'+struct.pack('<Q',near)+b'\xc3');free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,relay,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r,w.stop_reason or '');assert r==1
st=w.run(ex['SAN14_ModSettingsState'],());print('state',st);assert st&3==3
tab=lambda i:w.run(ex['SAN14_ModSettingsTestTable'],(i,))
GETB,SETB,STDB,MSGB,CLKB,MODGET,MODSET=[tab(i) for i in range(7)]
S_GETJA,S_GETRET,S_SETJA,S_SETRET,S_STDRET,S_MSGORIG=[tab(i) for i in range(7,13)]
MODENTER,MODBIND=tab(14),tab(15);hdrobj=w.alloc(0x40);w.w(GAME_BASE+0x2af6a0,b'\x48\xb8'+struct.pack('<Q',hdrobj)+b'\xc3');w.w(GAME_BASE+0x8542e0,b'\x48\x89\x51\x08\xc3');P_ENTER,P_BIND,P_PENDING,P_CLKORIG=tab(16),tab(17),tab(18),tab(19)
q64=lambda a:struct.unpack('<Q',w.r(a,8))[0];r32=lambda a:struct.unpack('<i',w.r(a,4))[0]
# 원래 주소 확인
assert q64(S_GETJA)==GAME_BASE+0x3ea6bf and q64(S_GETRET)==GAME_BASE+0x3ea409 and q64(S_SETJA)==GAME_BASE+0x3effdb and q64(S_SETRET)==GAME_BASE+0x3efce9
assert q64(S_STDRET)==GAME_BASE+0x63625c and q64(S_MSGORIG)==GAME_BASE+0x308ef0
print('되돌아갈 주소 6개 OK')
STOPS={}
def stop(name):
    a=w._tramp(b'\xf4');STOPS[a]=name;return a
RET=stop('ret')
def call(entry,regs,slots={}):
    for slot,name in slots.items():w.w64(slot,stop(name))
    rsp=(STACK_BASE+STACK_SIZE-0x20000)&~0xf;rsp-=8;w.w64(rsp,RET)   # 함수 진입 상태(rsp≡8)
    w.mu.reg_write(UC_X86_REG_RSP,rsp);w.mu.reg_write(UC_X86_REG_GS_BASE,TEB_BASE)
    for k,v in regs.items():w.mu.reg_write(k,v)
    hit=[None]
    def hk(mu,addr,size,ud):
        if addr in STOPS:hit[0]=STOPS[addr];mu.emu_stop()
    h=w.mu.hook_add(UC_HOOK_CODE,hk)
    try:w.mu.emu_start(entry,0,count=20_000_000)
    except UcError as e:print('UcError',e,hex(w.mu.reg_read(UC_X86_REG_RIP)))
    w.mu.hook_del(h)
    return hit[0],w.mu.reg_read(UC_X86_REG_RAX),w.mu.reg_read(UC_X86_REG_RSP)-rsp
R=UC_X86_REG_RCX;D=UC_X86_REG_RDX;R8=UC_X86_REG_R8;R9=UC_X86_REG_R9
ID_WHEEL=0x100+23;ID_GIVE=0x100+25;ID_TROOPS=0x100+17;ID_CAMERA=0x100
# A: 우리 id → 우리 값 / 원래 id → 원래 코드 / 0x3c 넘는 원래 id → ja 대상
h,rax,d=call(GETB,{R:0x1234,D:ID_WHEEL,R8:0});print('A get 휠클릭(켬=0):',h,rax&0xffffffff,d);assert h=='ret' and rax&0xffffffff==0 and d==8
h,rax,d=call(GETB,{R:0x1234,D:ID_GIVE,R8:0});print('A get 명품 수여(끔=1):',h,rax&0xffffffff);assert h=='ret' and rax&0xffffffff==1
h,rax,d=call(GETB,{R:0x1234,D:ID_TROOPS,R8:0});print('A get IC 없는 줄 → 기본값:',rax&0xffffffff);assert rax&0xffffffff==7000
h,rax,d=call(GETB,{R:0x1234,D:0x10,R8:0},{S_GETRET:'get_native'});print('A get id 0x10:',h,'rdx',w.mu.reg_read(D),d);assert h=='get_native' and w.mu.reg_read(D)==0x10 and w.mu.reg_read(R)==0x1234 and d==0
h,rax,d=call(GETB,{R:0x1234,D:0x50,R8:0},{S_GETJA:'get_ja'});print('A get id 0x50:',h);assert h=='get_ja' and d==0
h,rax,d=call(GETB,{R:0x1234,D:0x3c,R8:0},{S_GETRET:'get_native'});assert h=='get_native'
h,rax,d=call(GETB,{R:0x1234,D:0x200,R8:0},{S_GETJA:'get_ja'});print('A get id 0x200 (우리 범위 밖):',h);assert h=='get_ja'
h,rax,d=call(GETB,{R:0x1234,D:0xffffffff,R8:0},{S_GETJA:'get_ja'});assert h=='get_ja'
# B: 우리 id 쓰기 → ini 반영, 원래 id → 원래 코드(r9 보존)
MS=lambda:w.api._ini_for('SAN14Mods/modules/ModSettings.ini')
# 시작 때 옮기기: 예전에 키트 ini 에 Enabled=0 이던 명품 수여 → [Kit] DragGive=0, 키트 ini 는 1(자리를 걸어 두게)
print('옮기기:',MS().get('Kit'),w.api._ini_for('SAN14_DragGive.ini'));assert MS()['Kit']['DragGive']=='0' and w.api._ini_for('SAN14_DragGive.ini')['DragGive']['Enabled']=='1'
h,rax,d=call(SETB,{R:0x1234,D:ID_WHEEL,R8:0,R9:1});print('B set 휠클릭 끔:',h,MS().get('Kit'),w.api._ini_for('SAN14_WheelRegion.ini'));assert h=='ret' and MS()['Kit']['WheelRegion']=='0' and w.api._ini_for('SAN14_WheelRegion.ini')['WheelRegion']['Enabled']=='1'
h,rax,d=call(GETB,{R:0x1234,D:ID_WHEEL,R8:0});assert rax&0xffffffff==1
h,rax,d=call(SETB,{R:0x1234,D:ID_GIVE,R8:0,R9:0});assert MS()['Kit']['DragGive']=='1' and w.api._ini_for('SAN14_DragGive.ini')['DragGive']['Enabled']=='1';print('B set 명품 수여 켬: OK')
h,rax,d=call(SETB,{R:0x1234,D:ID_TROOPS,R8:0,R9:5000});print('B set 잠긴/없는 줄 무시:',h,rax&0xffffffff);assert h=='ret' and rax&0xffffffff==0
h,rax,d=call(SETB,{R:0x1234,D:0x10,R8:0,R9:0x77},{S_SETRET:'set_native'});print('B set id 0x10:',h,'r9',hex(w.mu.reg_read(R9)));assert h=='set_native' and w.mu.reg_read(R9)==0x77 and w.mu.reg_read(D)==0x10 and d==0
h,rax,d=call(SETB,{R:0x1234,D:0x41,R8:0,R9:0x77},{S_SETJA:'set_ja'});assert h=='set_ja'
print('A/B 다리 OK')
# D: 원래 id → 밀린 4개 명령 재실행 후 원래+12 / 우리 id → 그냥 복귀
ctl=w.alloc(0x100);w.w(ctl,bytes(0x100));w.w(ctl+0x88,struct.pack('<i',0x10));w.w(ctl+0x80,struct.pack('<i',-1000))
h,rax,d=call(STDB,{R:0x5555,D:ctl,UC_X86_REG_RSI:0x51,UC_X86_REG_RDI:0x52,UC_X86_REG_R14:0x53},{S_STDRET:'std_native'})
sp=w.mu.reg_read(UC_X86_REG_RSP);print('D 원래 id:',h,'rsp 차',hex(d));assert h=='std_native' and d==-(0x90+24) and q64(sp+0x90)==0x53 and q64(sp+0x98)==0x52 and q64(sp+0xa0)==0x51
w.w(ctl+0x88,struct.pack('<i',0x100+5));h,rax,d=call(STDB,{R:0x5555,D:ctl},{S_STDRET:'std_native'});print('D 우리 id:',h);assert h=='ret' and d==8
# E: 우리 글 id → 설명 글, 아니면 원래로
h,rax,d=call(MSGB,{R:0x9999,D:0x7ff00000+23},{S_MSGORIG:'msg_native'});s=w.r(rax,200).decode('utf-16le').split('\0')[0];print('E 글:',h,s[:30]);assert h=='ret' and s.startswith('[30번]')
h,rax,d=call(MSGB,{R:0x9999,D:0x2849},{S_MSGORIG:'msg_native'});assert h=='msg_native' and w.mu.reg_read(D)==0x2849 and w.mu.reg_read(R)==0x9999
h,rax,d=call(MSGB,{R:0x9999,D:0x7ff00100},{S_MSGORIG:'msg_native'});assert h=='msg_native'
# E 제목: 우리 창을 여는 동안 원래 글이 '숙련 모드'면 '모드'로, 다른 글·여는 중 아닐 땐 그대로
P_ENTERING=tab(20);w.w64(tab(30),0x7fffffffffffffff);s1=w.alloc(64);w.w(s1,'숙련 모드'.encode('utf-16le')+b'\0\0');s2=w.alloc(64);w.w(s2,'숙련 설정 설명'.encode('utf-16le')+b'\0\0')
def orig_ret(ptr):return w._tramp(b'\x48\xb8'+struct.pack('<Q',ptr)+b'\xc3')
rd=lambda a:w.r(a,40).decode('utf-16le').split('\0')[0]
w.w(P_ENTERING,struct.pack('<i',1));w.w64(S_MSGORIG,orig_ret(s1));h,rax,d=call(MSGB,{R:0x9999,D:0x2849});t1=rd(rax)
w.w64(S_MSGORIG,orig_ret(s2));h,rax,d=call(MSGB,{R:0x9999,D:0x2849});t2=rd(rax)
w.w(P_ENTERING,struct.pack('<i',0));w.w64(S_MSGORIG,orig_ret(s1));h,rax,d=call(MSGB,{R:0x9999,D:0x2849});t3=rd(rax)
print('제목 바꾸기:',t1,'/',t2,'/',t3);assert t1=='모드' and t2=='숙련 설정 설명' and t3=='숙련 모드'
w.w64(S_MSGORIG,GAME_BASE+0x308ef0)
print('D/E 다리 OK')
# IC 클릭 다리: IC 없음 → 레지스터 그대로 원래로
h,rax,d=call(CLKB,{R:0x1111,D:0x2222,R8:0x3333,R9:0x4444,UC_X86_REG_R10:0x5555},{P_CLKORIG:'click_orig'})
print('클릭 다리(IC 없음):',h);assert h=='click_orig' and [w.mu.reg_read(x) for x in (R,D,R8,R9,UC_X86_REG_R10)]==[0x1111,0x2222,0x3333,0x4444,0x5555] and d==0
# F: Enter 원본을 빈 함수로 바꾸고, 게임 함수 3개(목록 재구성·탭 위치·제목)를 기록용으로 바꿔 표 채우기 확인
rec=w.alloc(0x100);w.w(rec,bytes(0x100))
def recorder(off):return w._tramp(b'\x48\xb8'+struct.pack('<Q',rec+off)+b'\x48\x89\x08\x48\x89\x50\x08\xc3')  # [rec+off]=rcx,[+8]=rdx
for rva,off in ((0x6566e0,0),(0x656d90,0x10),(0x8a1380,0x20)):
    t=recorder(off);w.w(GAME_BASE+rva,b'\x48\xb8'+struct.pack('<Q',t)+b'\xff\xe0')
w.w64(P_ENTER,ret1)
state=w.alloc(0x500);w.w(state,bytes(0x500));layout=w.alloc(0x2000);w.w(layout,bytes(0x2000));w.w64(state+0x470,layout);w.w64(state+0x490,2)
tp=w.alloc(0x200);w.w(tp,bytes(0x200));w.w64(layout+0x8b8,tp);w.w32(tp+0x140,6)
btns=w.alloc(0x40);w.w64(tp+0x130,btns);vt=w.alloc(0x200);w.w(vt,bytes(0x200))
lab=w.alloc(0x100);w.w(lab,bytes(0x100))
settxt=w._tramp(b'\x48\xb8'+struct.pack('<Q',lab)+b'\x48\x8b\x49\x08\x48\x89\x14\xc8\xc3')  # lab[obj+8]=rdx
w.w64(vt+0x1d8,settxt)
for i in range(6):
    b=w.alloc(0x20);w.w64(b,vt);w.w64(b+8,i);w.w64(btns+8*i,b)
descobj=w.alloc(0x100);w.w64(layout+0x8c8,descobj)
w.w32(P_PENDING,0);h,rax,d=call(MODENTER,{R:state});assert r32(layout+0x17c)==0;print('F: 우리가 안 연 창은 그대로')
w.w32(P_PENDING,1);w.w64(state+0x490,1);h,rax,d=call(MODENTER,{R:state});assert r32(layout+0x17c)==0;print('F: mode 1(게임 설정창 확인)은 그대로')
w.w64(state+0x490,2);h,rax,d=call(MODENTER,{R:state});print('F mode2:',h)
counts=[r32(layout+0x17c+4*i) for i in range(6)];print('탭별 줄 수',counts)
ids=[r32(layout+0x8d0+i*0x60) for i in range(sum(counts))]
print('줄 id',[hex(x) for x in ids])
assert counts==[2,1,1,1,1,1] and ids[:2]==[0x100+23,0x100+25] and ids[2]==0x1ff and ids[3]==0x1ff
e=layout+0x8d0;nm=lambda p:w.r(p,120).decode('utf-16le').split('\0')[0]
print('첫 줄:',nm(q64(e+0x18)),'/',nm(q64(e+0x20)),nm(q64(e+0x28)),'값',r32(e+4),'선택지',q64(e+8),'설명',hex(r32(e+0x48)))
assert q64(e+8)==2 and r32(e+4)==1 and r32(e+0x48)==0x7ff00000+23
print('탭 이름',[nm(q64(lab+8*i)) for i in range(6)])
assert nm(q64(lab))=='편의' and nm(q64(lab+40))=='사운드'
assert q64(rec)==layout and q64(rec+0x10)==layout and q64(rec+0x20)==descobj;print('목록 재구성·탭·제목 호출 OK, 제목:',nm(q64(rec+0x28)))
assert r32(P_PENDING)==0
# C: 바인드 원본 빈 함수, def/row 를 돌려주는 가짜, 버튼 활성 기록
w.w64(P_BIND,ret1)
row=w.alloc(0x300);w.w(row,bytes(0x300));en=w.alloc(0x40);w.w(en,b'\xff'*0x40)
setena=w._tramp(b'\x48\xb8'+struct.pack('<Q',en)+b'\x48\x8b\x49\x08\x88\x14\x08\xc3')  # en[obj+8]=dl
cvt=w.alloc(0x200);w.w(cvt,bytes(0x200));w.w64(cvt+0xd0,setena)
for k in range(4):
    b=w.alloc(0x20);w.w64(b,cvt);w.w64(b+8,k);w.w64(row+0x1a8+8*k,b)
w.w(GAME_BASE+0x86b270,b'\x48\xb8'+struct.pack('<Q',e)+b'\xc3');w.w(GAME_BASE+0x86b6f0,b'\x48\xb8'+struct.pack('<Q',row)+b'\xc3')
h,rax,d=call(MODBIND,{R:0x7777,D:0,R8:0});print('C 휠클릭 줄 버튼 활성',list(w.r(en,4)));assert list(w.r(en,4))==[1,1,1,1]
w.w(GAME_BASE+0x86b270,b'\x48\xb8'+struct.pack('<Q',e+2*0x60)+b'\xc3');w.w(en,b'\xff'*4)
h,rax,d=call(MODBIND,{R:0x7777,D:0,R8:3});print('C 빈 탭 안내 줄 버튼',list(w.r(en,4)));assert list(w.r(en,4))==[0,0,0,0]
# R39b 추가 ------------------------------------------------------------
RNGB,MODUPD,MODEXIT,P_RNGRET,P_UPD,P_EXIT,ROWMIN,P_CACHE,P_REEN=[tab(i) for i in range(21,30)]
print('RowMin 병력',r32(ROWMIN+4*17),'캐시',r32(P_CACHE))
assert r32(ROWMIN+4*17)==1000 and r32(P_CACHE)==1
# D: 우리 id 숫자 칸(값 != -1000) → 원래 입력창 경로로
w.w(ctl+0x88,struct.pack('<i',0x100+17));w.w(ctl+0x80,struct.pack('<i',5000));h,rax,d=call(STDB,{R:0x5555,D:ctl},{S_STDRET:'std_native'});print('D 우리 숫자 칸:',h);assert h=='std_native'
w.w(ctl+0x80,struct.pack('<i',-1000))
# H: 입력창 범위
h,rax,d=call(RNGB,{UC_X86_REG_RAX:0x111,R:0x111*0x1c,R8:GAME_BASE+0x14005c0},{P_RNGRET:'rng_ret'})
sp=w.mu.reg_read(UC_X86_REG_RSP);print('H 우리 id 범위',r32(sp+0x38),rax&0xffffffff,'rsp차',d);assert h=='rng_ret' and r32(sp+0x38)==1000 and rax&0xffffffff==20000 and d==0
tb=GAME_BASE+0x14005c0;exp0=r32(tb+0x10*0x1c);exp1=r32(tb+0x10*0x1c+4)
h,rax,d=call(RNGB,{UC_X86_REG_RAX:0x10,R:0x10*0x1c,R8:tb},{P_RNGRET:'rng_ret'})
sp=w.mu.reg_read(UC_X86_REG_RSP);print('H 원래 id',r32(sp+0x38),rax&0xffffffff,exp0,exp1);assert r32(sp+0x38)==exp0 and rax&0xffffffff==exp1 and d==0
# G: Update 때 꺼진 단추 다시 켬(비트 +0x40&4)
w.w64(P_UPD,ret1);lst=w.alloc(0x200);w.w(lst,bytes(0x200));w.w64(layout+0x8c0,lst);w.w32(lst+0x178,1)
w.w(GAME_BASE+0x86b270,b'\x48\xb8'+struct.pack('<Q',e)+b'\xc3')
row2=w.alloc(0x300);w.w(row2,bytes(0x300));w.w32(row2+0x84,1);en2=w.alloc(0x40);w.w(en2,b'\xff'*0x40)
setena2=w._tramp(b'\x48\xb8'+struct.pack('<Q',en2)+b'\x48\x8b\x49\x08\x88\x14\x08\xc3')
cvt2=w.alloc(0x200);w.w(cvt2,bytes(0x200));w.w64(cvt2+0xd0,setena2)
for k in range(2):
    b=w.alloc(0x60);w.w(b,bytes(0x60));w.w64(b,cvt2);w.w64(b+8,k);w.w32(b+0x40,4 if k==1 else 0);w.w64(row2+0x1a8+8*k,b)
w.w(GAME_BASE+0x86b6f0,b'\x48\xb8'+struct.pack('<Q',row2)+b'\xc3')
h,rax,d=call(MODUPD,{R:state});print('G Update 다시 켬',list(w.r(en2,2)),'횟수',r32(P_REEN));assert list(w.r(en2,2))==[1,0xff] and r32(P_REEN)==1
h,rax,d=call(MODUPD,{R:state+8});assert r32(P_REEN)==1;print('G 다른 창은 안 건드림')
w.w64(P_EXIT,ret1);h,rax,d=call(MODEXIT,{R:state});print('Exit 뒤 캐시',r32(P_CACHE));assert r32(P_CACHE)==0
# S: 스크롤바 보이기(우리 창: 줄 수 >= 칸 수, 다른 창: 탭>=4)
SB=tab(33);P_OL=tab(34);P_SR=tab(35);lay=w.alloc(0x900);w.w(lay,bytes(0x900));ls=w.alloc(0x200);w.w(ls,bytes(0x200));w.w64(lay+0x8c0,ls);w.w32(ls+0x178,9)
w.w64(P_OL,lay);res=[]
for r15,rb,dx in ((26,lay,0),(3,lay,5),(12,lay,0),(26,lay+8,2),(3,lay+8,5)):
    h,rax,d=call(SB,{UC_X86_REG_RBP:rb,UC_X86_REG_R15:r15,D:dx,UC_X86_REG_RAX:0x1234},{P_SR:'sb_ret'});res.append((h,w.mu.reg_read(D)&0xffffffff,rax))
print('S 스크롤바',res);assert [x[1] for x in res]==[1,0,1,0,1] and all(x[0]=='sb_ret' and x[2]==0x1234 for x in res)
w.w64(P_OL,0)
tt=w.r(q64(hdrobj+8),8).decode('utf-16le');print('화면 제목',tt);assert tt=='모드\0\0'[:4] or tt.startswith('모드')
# K: 0x627310 범위 읽기 — 우리 id 면 RowMin/RowMax, 아니면 표(r12)
KB=tab(36);P_KR=tab(37);tb=GAME_BASE+0x14005c0
h,rax,d=call(KB,{UC_X86_REG_RAX:0x124,R:0x124*0x1c,UC_X86_REG_R12:tb,D:0x77},{P_KR:'k_ret'});kb1=(h,w.mu.reg_read(UC_X86_REG_RBX)&0xffffffff,rax&0xffffffff,w.mu.reg_read(D),d)
h,rax,d=call(KB,{UC_X86_REG_RAX:0x10,R:0x10*0x1c,UC_X86_REG_R12:tb,D:0x77},{P_KR:'k_ret'});kb2=(h,w.mu.reg_read(UC_X86_REG_RBX)&0xffffffff,rax&0xffffffff,w.mu.reg_read(D),d)
print('K 범위',kb1,kb2,exp0,exp1);assert kb1==('k_ret',2,4,0x77,0) and kb2==('k_ret',exp0,exp1,0x77,0)
print('전부 통과')
