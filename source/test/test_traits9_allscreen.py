# R42c Traits9Check: 9칸 화면 부품(Traits9_UI 49fb15b4)의 요약 화면 칸 늘리기(+0x2280) 대신 — unicorn 시험
#  ① 원래 함수와 우리 함수를 같은 순정 칸 모양으로 돌려, 원래가 되는 모양에서는 만든 칸 9개·기억 칸이 똑같은지
#  ② 원래가 실패하는 모양(높이 조금 다름 / 줄 겹침 / 세로 한 줄)에서 우리 함수는 만들고 이유를 기록하는지
#  ③ 부모가 다르면 원래처럼 안 만듦 ④ 실제 연결(입구 14바이트 ff25 → 우리 함수), 다른 판이면 안 바꿈 ⑤ AI 안내는 설치돼 있을 때만
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL=sys.argv[1] if len(sys.argv)>1 else '/home/claude/r41/build_b1/SAN14_Traits9Check.dll'
UIP='/home/claude/r41/pkg/support/payload/f09/SAN14_Traits9_UI.dll'
G=GAME_BASE
def boot(ai_json=None):
    fs={'SAN14_Traits9Check.dll':open(DLL,'rb').read(),'SAN14Mods/modules/Traits9Check.ini':b'[Traits9Check]\r\nNotice=1\r\n','SAN14_Traits9_UI.dll':open(UIP,'rb').read()}
    if ai_json is not None:fs['AI_TacticsMod/backup/input_state.json']=ai_json
    w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
    base=w.api._loadlib('SAN14_Traits9Check.dll')
    if isinstance(base,str):
        rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
    ex=w.modules['san14_traits9check.dll']['exports']
    ui=w.load(UIP,name='SAN14_Traits9_UI.dll');w.modules['san14_traits9_ui.dll']['path']='SAN14_Traits9_UI.dll'
    w.w64(ui+0x32d68,G)                       # 부품 안 '게임 시작 주소' 칸(DllMain 대신)
    ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3');free=w._tramp(b'\xc3')
    rdb=w._tramp(b'\xb8\x01\x00\x00\x00\xc3')
    resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',G)+bytes(0x18))
    core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,rdb,ret1,ret1,free,0))
    w.api.CreateThread=lambda:0x1234
    assert w.run(ex['SAN14_ModInitV1'],(core,),label='init')==1
    T=lambda i:w.run(ex['SAN14_Traits9CheckTestTable'],(i,))
    return w,ex,ui,T
w,ex,ui,T=boot()
PATCH,CREATE,PATCHED,DIAG,CALLS,RELAX,FAIL,UIV,AIP=range(12,21)
# 게임 쪽 가짜: 할당·표지 칸 만들기·위치·보이기·말풍선 연결
made=[];links=[]
heap=[w.alloc(0x200) for _ in range(400)];hp=[0]
fvt=w.alloc(0x400);hide=w._tramp(b'\xc3')
for i in range(0x80):w.w64(fvt+8*i,hide)
def alloc():
    o=heap[hp[0]];hp[0]+=1;w.w(o,bytes(0x200));return o
w.override(G+0x3f6bc0,alloc)
w.override(G+0x84b150,lambda:w.w64(w.arg(0),fvt) or 0)
fail_create=[False]
def place():
    o=w.arg(0);made.append((w.arg(1)&0xffffffff,w.arg(2)&0xffffffff,w.arg(3)&0xffffffff,w.r32(w.rsp()+0x28),w.r64(w.rsp()+0x30)))
    w.w64(o+0x178,0 if fail_create[0] else 0x777);return 0
w.override(G+0x851160,place)
w.override(G+0x89af80,lambda:links.append(w.arg(1)) or 0)
logs=[];w.override(ui+0x1060,lambda:logs.append(w.rstr(w.arg(0))) or 0)
def setup(boxes,par_diff=False):
    tbl=w.alloc(0x500);w.w(tbl,bytes(0x500));par=w.alloc(0x40);w.w64(par,G+0x14b8ce0);w.w64(tbl,par)
    nat=w.alloc(0x40);ns=[]
    for i,(x,y,ww,h) in enumerate(boxes):
        n=w.alloc(0x40);w.w(n,bytes(0x40));w.w64(n+8,par if not(par_diff and i==3) else par+8);w.w(n+0x20,struct.pack('<iiii',x,y,ww,h));ns.append(n);w.w64(nat+8*i,n)
    return tbl,par,nat,ns
def run(fn,boxes,kind=2,arg4=0,**k):
    made.clear();links.clear();logs.clear()
    tbl,par,nat,ns=setup(boxes,**k)
    r=w.run(fn,(tbl,kind,nat,arg4))
    if r is None:print("STOP",w.stop_reason);r=0xdead
    rec=tbl+kind*0x80
    labels=[w.r64(rec+0x18+8*i) for i in range(9)];kept=[w.r64(rec+0x60+8*i) for i in range(5)]
    return r&0xffffffff,list(made),labels,kept,w.r64(rec+0x88),list(links),list(logs),par,ns
ORIG=ui+0x2280
# ① 원래가 되는 모양 3가지: 가로 한 줄 / 3+2 두 줄 / 세로 한 줄(명령 화면 대상무장 칸 모양)
cases={'가로 한 줄':[(100+i*90,50,80,30) for i in range(5)],
       '두 줄(3+2)':[(100,50,80,30),(190,50,80,30),(280,50,80,30),(100,90,80,30),(190,90,80,30)],
       '세로 한 줄':[(1118,90,200,44),(1118,138,200,44),(1118,186,200,44),(1118,234,200,44),(1118,282,200,44)]}
for nm,b in cases.items():
    a=run(ORIG,b,arg4=0x4242);c=run(T(CREATE),b,arg4=0x4242)
    print(nm,'원래',a[0],a[1][:3],'… / 우리',c[0])
    assert a[0]==1 and c[0]==1,(a[0],c[0])
    pa,pc=a[7],c[7]
    assert [m[:4] for m in a[1]]==[m[:4] for m in c[1]] and all(m[4]==pa for m in a[1]) and all(m[4]==pc for m in c[1]),(a[1],c[1])
    assert a[4]==c[4]==0x4242 and len(a[5])==len(c[5])==9 and a[6]==c[6]==['SAN14 Traits9 UI: ALLSCREEN_GRID_CREATED - nine native labels created with original label height and row positions.\r\n']
    assert a[3]==a[8] and c[3]==c[8]
assert w.r(T(DIAG),1)==b'\0','원래가 되는 모양에선 이유 기록 없음'
print('① 원래가 되는 모양: 만든 칸 9개 위치·크기·부모, 기억 칸, 말풍선 연결, 완료 기록이 원래와 똑같음')
# ② 원래가 실패하는 모양
bad={'높이 조금 다름(E)':[(1118,90,200,44),(1118,138,200,44),(1118,186,200,44),(1118,234,200,44),(1118,282,200,46)],
     '줄 겹침(G)':[(1118,90,200,44),(1118,130,200,44),(1118,170,200,44),(1118,210,200,44),(1118,250,200,44)],
     '같은 줄 y 1~2 차이':[(100,50,80,30),(190,51,80,30),(280,50,80,30),(100,90,80,30),(190,91,80,30)]}
for nm,b in bad.items():
    a=run(ORIG,b);c=run(T(CREATE),b)
    d=w.rstr(T(DIAG));print(nm,'원래',a[0],'/ 우리',c[0],'|',d)
    assert a[0]==0 and c[0]==1 and 'relaxed=ok' in d and len(c[1])==9
    ys=sorted(set(m[1] for m in c[1]));assert all(y in [x[1] for x in b] for y in ys)
print('② 원래 실패 모양 → 너그럽게 다시 재서 9칸 만듦 + 이유 기록')
# ③ 부모 다름 / 만들기 실패
a=run(ORIG,cases['세로 한 줄'],par_diff=True);c=run(T(CREATE),cases['세로 한 줄'],par_diff=True);d=w.rstr(T(DIAG))
assert a[0]==0 and c[0]==0 and 'why=P' in d and 'relaxed=fail' in d and c[1]==[];print('③ 부모 다름 → 원래처럼 안 만듦:',d[:60])
fail_create[0]=True;c=run(T(CREATE),cases['세로 한 줄']);fail_create[0]=False;assert c[0]==0;print('   칸 만들기 실패 → 0(원래 실패 길)')
lines=[l for l in (w.fs_written.get('SAN14_기록/적용확인.log') or b'').decode('utf-8','replace').split('\n') if 'R42T9\tD' in l]
print('기록 줄 예:',lines[0][:150]);assert len(lines)>=4
# ④ 실제 연결
w.run(T(PATCH));print('연결 결과',w.r32(T(PATCHED)),w.r(ORIG,6).hex())
assert w.r32(T(PATCHED))==1 and w.r(ORIG,6)==bytes.fromhex('ff2500000000') and w.r64(ORIG+6)==T(CREATE)
c=run(ORIG,bad['줄 겹침(G)']);assert c[0]==1 and len(c[1])==9;print('④ 부품 입구가 우리 함수로: 줄 겹침 모양도 9칸')
w2,ex2,ui2,T2=boot();w2.w8(ui2+0x2285,0x90);w2.run(T2(PATCH));assert w2.r32(T2(PATCHED))==3 and w2.r8(ui2+0x2280)==0x41;print('   입구가 이미 다르면 안 바꿈(3)')
# ⑤ AI 안내
assert w.run(T(AIP))&0xff==0
w3,_,_,T3=boot(b'{\r\n    "installed":  false,\r\n    "release_version": "v13.3"\r\n}');assert w3.run(T3(AIP))&0xff==0
w4,_,_,T4=boot(b'{\r\n    "installed":  true\r\n}');assert w4.run(T4(AIP))&0xff==1
print('⑤ AI 안내: 해제 상태(installed false)·폴더 없음 → 안 붙임 / 설치(true) → 붙임')
print('ALL OK')
