# R42 C 시험 도구 SAN14_SceneTest.dll — unicorn 단위 시험 (버튼 요청 큐·게임 스레드 실행·빈 칸 고르기·자동 저장 요청·달 검사 바꿈 원복)
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_c/SAN14_SceneTest.dll'
fs={'SAN14_SceneTest.dll':open(DLL,'rb').read(),'SAN14Mods/modules/SceneTest.ini':b'[SceneTest]\r\nWindow=0\r\n'}
w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
G=GAME_BASE
base=w.api._loadlib('SAN14_SceneTest.dll')
if isinstance(base,str):
    rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
m=w.modules['san14_scenetest.dll'];ex=m['exports']
ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3');free=w._tramp(b'\xc3')
resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',G)+bytes(0x18))
core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,ret1,free,0))
r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');print('ModInit ->',r);assert r==1
tab=lambda i:w.run(ex['SAN14_SceneTestTable'],(i,))
TICK,NEXT,REQ,STAGE,SLOT,TM,STATUS,MBOX,T22=[tab(i) for i in range(9)]
assert w.r32(TM)==1,'Window=0 → 창 안 띄움'
rd32=lambda a:struct.unpack('<i',w.r(a,4))[0]
# ---- 게임 함수 가짜 ----
calls=[]
headers={s:1 for s in range(17)}          # 0~16 사용 중(사용자 PC 와 같은 모양)
steam_files=set()
namebuf=w.alloc(0x40)
def f_name():
    s=w.arg(0);n=('svdexceKO%02d.s14'%s).encode()+b'\0';w.w(namebuf,n);return namebuf
w.override(G+0x32d090,f_name)
w.override(G+0x935ac0,lambda:calls.append(('enum',w.arg(1))) or 0)
w.override(G+0x9352e0,lambda:0x1000 if headers.get(w.arg(1)) else 0)
w.override(G+0x1d37b0,lambda:0)
mgr=w.alloc(0x400);w.w64(G+0x27dba68,mgr)
# Steam: SteamInternal_ContextInit → ctx[0]=rs, rs vt[13]=FileExists
rs=w.alloc(0x40);vt=w.alloc(0x100);w.w64(rs,vt);ctx=w.alloc(0x10);w.w64(ctx,rs)
fe=w._tramp(b'\xc3'*8);w.w64(vt+0x68,fe)
def f_fe():
    nm=w.rstr(w.arg(1));calls.append(('exists',nm));return 1 if nm in steam_files else 0
w.override(fe,f_fe)
ci=w._tramp(b'\xc3'*8);w.override(ci,lambda:ctx);w.w64(G+0x1340b28,ci)
saved_rq=[]
def f_req():
    p=w.arg(0);sl=rd32(p);ln=w.r64(p+8+0x10);cap=w.r64(p+8+0x18);s=(w.r(w.r64(p+8),ln) if cap>=16 else w.r(p+8,ln)).decode();saved_rq.append((sl,s,ln,cap,w.r64(p+0x28+0x10)));return 0
w.override(G+0x33aee0,f_req)
statemgr=w.alloc(0x40);w.override(G+0x110e0,lambda:statemgr)
pushed=[]
w.override(G+0x466530,lambda:pushed.append((w.arg(0),w.rstr(w.arg(1)),w.arg(2))) or 0)
w.override(G+0x56fe10,lambda:1)
histl=[]
w.override(G+0x16efc0,lambda:0x5000)
w.override(G+0x934400,lambda:histl.append(w.rstr(w.r64(w.rsp()+0x38),wide=True)) or 0)
saving=[0]
w.override(G+0x56fc30,lambda:0x1000 if (saving[0] and w.rstr(w.arg(1))=='CSaveState') else 0)
w.override(G+0x332e90,lambda:0)
w.override(G+0x44f310,lambda:calls.append(('menu',w.arg(0),w.arg(1))) or 0)
nat=[];natstub=w._tramp(b'\xc3'*8);w.override(natstub,lambda:nat.append(w.arg(0)) or 0);w.w64(NEXT,natstub)
state=w.alloc(0x800);w.w32(state+0x470,1)
def tick():w.run(TICK,(state,0,0,0))
# ---- 1. 빈 칸 고르기: 머리 없는 칸 중 Steam 파일도 없는 첫 칸 ----
steam_files.add('svdexceKO17.s14')            # 17 은 칸 정보엔 없지만 파일이 남아 있음 → 건너뜀
s=w.run(ex['SAN14_SceneTestPickEmpty'],());print('빈 칸 =',s);assert s==18
# ---- 2. 버튼 요청 큐: 진행 중이면 두 번째 요청 안 받음 ----
assert w.run(ex['SAN14_SceneTestPress'],(6,))==6
assert w.run(ex['SAN14_SceneTestPress'],(1,))==6;print('요청 큐: 진행 중 두 번째 요청 무시')
# ---- 3. 플레이어 턴 칸: 원래 처리 먼저, 그다음 자동 저장 시작 ----
w.w32(state+0x470,3);tick();assert nat==[state] and w.r32(STAGE)==0 and not pushed;print('명령 단계 아님(3) → 원래 처리만')
w.w32(state+0x470,1);tick()
print('자동 저장 요청:',saved_rq,pushed)
assert w.r32(STAGE)==1 and rd32(SLOT)==18 and saved_rq==[(18,'svdexceKO18.s14',15,15,0)] and pushed==[(statemgr,'CSaveState',0)]
# 저장 확인 전: 칸 정보에 아직 없음 → 기다림(시험 안 함)
saving[0]=1;steam_files.add('svdexceKO18.s14');w.tick+=2000;tick();assert w.r32(STAGE)==1 and not [c for c in calls if c[0]=='menu'];print('저장 중(CSaveState 있음) → 시험 안 함(칸 정보 다시 읽기 안 함)')
saving[0]=0;steam_files.discard('svdexceKO18.s14');w.tick+=2000;tick();assert w.r32(STAGE)==1;print('저장 끝났어도 파일 없으면 기다림')
# 저장 끝(칸 머리 생김) → 시험 실행(6 명품 창 = 메뉴 명령 0x15)
steam_files.add('svdexceKO18.s14');w.tick+=2000;tick()
menu=[c for c in calls if c[0]=='menu'];print('시험 실행:',menu)
assert w.r32(STAGE)==0 and w.r32(REQ)==0 and menu==[('menu',state,0x15)]
# ---- 4. 빈 칸 없음 → 시험 안 함 ----
for k in range(0x32):headers[k]=1
calls.clear();pushed.clear();assert w.run(ex['SAN14_SceneTestPress'],(6,))==6;tick()
assert not pushed and w.r32(REQ)==0 and not [c for c in calls if c[0]=='menu'];print('빈 칸 없음 → 저장·시험 안 함, 요청 비움')
# ---- 5. 저장 60초 안에 확인 안 되면 시험 안 함 ----
headers={s:1 for s in range(19)};pushed.clear();calls.clear()
w.run(ex['SAN14_SceneTestPress'],(6,));tick();assert w.r32(STAGE)==1 and rd32(SLOT)==19
w.tick+=61000;tick();assert w.r32(STAGE)==0 and not [c for c in calls if c[0]=='menu'];print('저장 확인 실패 → 시험 안 함')
# ---- 6. 재해 시험용 달 검사 바꿈: 4곳 nop → 원복 ----
sites=[0x299dcd,0x299dde,0x29a513,0x29a521];orig=[w.r(G+a,6) for a in sites]
assert w.run(ex['SAN14_SceneTestMonth'],(1,))==1 and all(w.r(G+a,6)==bytes.fromhex('660f1f440000') for a in sites)
assert w.run(ex['SAN14_SceneTestMonth'],(0,))==1 and [w.r(G+a,6) for a in sites]==orig;print('달 검사 4곳 바꿈 → 원래 바이트로 복원')
w.w(G+0x29a513,b'\xe9'+orig[2][1:])          # 다른 모드가 바꾼 자리 흉내 → 손대지 않음
assert w.run(ex['SAN14_SceneTestMonth'],(1,))==0 and w.r(G+0x299dcd,6)==orig[0];w.w(G+0x29a513,orig[2]);print('바이트 다르면 아무것도 안 바꿈')
# ---- 7. 긴 파일 이름(도전 모드 17자)은 게임 할당기로 ----
big=[]
heap=w.alloc(0x40);w.override(G+0x3f6bc0,lambda:big.append(w.arg(0)) or heap)
w._overrides[G+0x1d37b0]=lambda:0x1234;w.override(G+0x1d2a00,lambda:0)
def f_name2():
    s=w.arg(0);n=('svdexceccKO%02d.s14'%(s-0x3c)).encode()+b'\0';w.w(namebuf,n);return namebuf
w._overrides[G+0x32d090]=f_name2
headers={};saved_rq.clear();pushed.clear();w.run(ex['SAN14_SceneTestPress'],(6,));tick()
print("도전 모드:",saved_rq,big);assert saved_rq[0][0]==0x3c and saved_rq[0][2]==17 and big==[18]

# ---- 8. 재해 시험(R42c): 버튼 = 조건만 걸기(역병 대기 표시 + 달 검사 끔), 턴이 넘어가 게임이 처리한 뒤 내 턴에서 되돌림·결과 ----
steam_files.add('svdexceccKO00.s14');w.tick+=2000;tick();assert w.r32(STAGE)==0 and w.r32(REQ)==0
root=w.alloc(0x90000);w.w64(G+0x277df18,root)
areas={}
for i in range(1,9):
    a=w.alloc(0x100);w.w(a,bytes(0x100));w.w64(a,G+0x13a6980);areas[i]=a;w.w64(root+0x6caf0+8*i,a);w.w8(a+0x35,i//2)
junk=w.alloc(0x100);w.w(junk,bytes(0x100));w.w64(root+0x6caf0+8*9,junk)   # 표 뒤쪽 다른 자료(지역 아님) → 안 읽음
w.w8(areas[1]+0x38,0x01)            # 1번은 이미 역병 → 제외
PF=0x7777;OTHER=0x8888
owner={a:(PF if i!=8 else OTHER) for i,a in areas.items()}
scen=w.alloc(0x100);w.w64(root+0x871e8,scen);w.w32(scen+0x34,(255)|(2<<16)|(21<<24))
w.override(G+0x32e910,lambda:PF)
w.override(G+0x32f500,lambda:1 if w.arg(0) else 0)
asked=[]
def f_force():asked.append(w.arg(0));return owner.get(w.arg(0),0)
w.override(G+0x22c990,f_force)
w.override(G+0x2337b0,lambda:1 if w.arg(0)==PF else 0)
called=[];w.override(G+0x427ff0,lambda:called.append(1) or 1);w.override(G+0x297e10,lambda:called.append(2) or 0)
headers={};w._overrides[G+0x1d37b0]=lambda:0;w._overrides[G+0x32d090]=f_name
w.run(ex['SAN14_SceneTestPress'],(1,));tick();steam_files.add('svdexceKO00.s14');w.tick+=2000;tick()
assert called==[],'재해 함수 직접 안 부름'
assert [w.r(G+a,6).hex() for a in sites]==['660f1f440000']*4 and w.r8(areas[2]+0x38)==2 and junk not in asked
print('재해 시험 준비: 내 지역 2번 대기 표시, 달 검사 끔, 지역 아닌 칸은 안 읽음')
tick();assert [w.r(G+a,6).hex() for a in sites]==['660f1f440000']*4,'턴 안 넘어가면 그대로'
w.w32(scen+0x34,(255)|(3<<16)|(1<<24));w.w8(areas[2]+0x38,0x01)          # 턴이 넘어가고 게임이 역병을 냄
tick()
assert [w.r(G+a,6) for a in sites]==orig;print('턴 넘어간 뒤 내 턴: 달 검사 되돌림 + 결과 기록')
print([x[2][:200] for x in w.msgs if 'R42T' in str(x) and 'result' in str(x)][-1:])

# ---- 9. 8·9(상태 기록)는 저장 없이 바로, 2·3 은 SceneSkip 없으면 장면 자리 안 부름 ----
pushed.clear();w.run(ex['SAN14_SceneTestPress'],(8,));tick();assert not pushed and w.r32(REQ)==0
w.run(ex['SAN14_SceneTestPress'],(9,));tick();assert not pushed and w.r32(REQ)==0
called6=[];w.override(G+0x6c43b0,lambda:called6.append(1) or 0);w.override(G+0x6c4670,lambda:called6.append(2) or 0)
headers={};w.run(ex['SAN14_SceneTestPress'],(2,));tick();headers[0]=1;w.tick+=2000;tick()
headers={};w.run(ex['SAN14_SceneTestPress'],(3,));tick();headers[0]=1;w.tick+=2000;tick()
assert called6==[];print('8·9 저장 없이 기록만, 2·3 SceneSkip 없으면 장면 자리 안 부름')
print('기록:',[x[2] for x in w.msgs if 'R42T' in str(x)][:6])
print('로그창 줄:',histl[:6]);assert any('재해 시험' in x for x in histl) and all(x.startswith('[통합패치 시험] ') for x in histl) and not any('자동 저장 중' in x for x in histl)
print('ALL OK')
