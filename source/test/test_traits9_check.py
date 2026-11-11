# R42 A10 개성 9칸 공용 부품 확인창 — unicorn 단위 시험
import sys,os,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL=sys.argv[1] if len(sys.argv)>1 else '/home/claude/r41/build_out/SAN14_Traits9Check.dll'
UI_SKIP=b'SAN14 Traits9 UI: DISPLAY_SKIPPED - unexpected layout or native label creation failed; original view retained.\r\n'
def boot(extra_fs=None,notice=1):
    fs={'SAN14_Traits9Check.dll':open(DLL,'rb').read(),'SAN14Mods/modules/Traits9Check.ini':('[Traits9Check]\r\nNotice=%d\r\n'%notice).encode()}
    fs.update(extra_fs or {})
    w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
    base=w.api._loadlib('SAN14_Traits9Check.dll')
    if isinstance(base,str):
        rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
    m=w.modules['san14_traits9check.dll'];ex=m['exports']
    ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3');free=w._tramp(b'\xc3')
    resolved=w.alloc(0x40);w.w(resolved,struct.pack('<IIIIII',0x40,0x10000,1,0,0,0x29ff000)+struct.pack('<Q',GAME_BASE)+bytes(0x18))
    core=w.alloc(0x40);w.w(core,struct.pack('<II',0x38,0x10000)+struct.pack('<QQQQQQ',resolved,ret1,ret1,ret1,free,0))
    tab=lambda i:w.run(ex['SAN14_Traits9CheckTestTable'],(i,))
    T=[tab(i) for i in range(11)]
    # ModInit 은 감시 스레드를 만든다: 스레드 전환 없이 쓰려고 CreateThread 를 '만든 척'으로 바꿔 둠
    w.api.CreateThread=lambda:0x1234
    r=w.run(ex['SAN14_ModInitV1'],(core,),label='init');assert r==1,r
    return w,T
def boxes(w):return [m for m in w.msgs if m[0]=='msgbox']
def lines(w):
    d=w.fs_written.get('SAN14_기록/적용확인.log') or b''
    return [l for l in d.decode('utf-8','replace').split('\n') if l]
NINE,UIH,FIRST,HAS,SHOWN,FA,FB,FC,NOTICE,ROOT,REPORT=range(11)
w,T=boot()
SITES=[(0x4a6cf3,'48c745dc05000000','48c745dc09000000'),(0x4a7313,'48c745dc05000000','48c745dc09000000'),(0x22558f,'4183fc05','4183fc09'),(0x23064a,'83fb05','83fb09'),(0x232d97,'4883fb05','4883fb09')]
for a,b,_ in SITES: assert w.r(GAME_BASE+a,len(bytes.fromhex(b)))==bytes.fromhex(b),hex(a)
assert w.run(T[NINE])==0; assert w.run(T[UIH])&0xff==0; print('원본 게임: 9칸 0/5, 화면 부품 없음 OK')
for i,(a,_,af) in enumerate(SITES):
    w.w(GAME_BASE+a,bytes.fromhex(af));assert w.run(T[NINE])==i+1
w.w(GAME_BASE+0x491f01,bytes.fromhex('ff2500000000')+struct.pack('<Q',0x1e0001234))
assert w.run(T[UIH])&0xff==1; print('9칸 5/5, 화면 부품(0x491f01 ff25) 판정 OK')
w.w(GAME_BASE+0x491f01,bytes.fromhex('e900000000')); assert w.run(T[UIH])&0xff==0; print('다른 모양(e9)은 화면 부품 아님 OK')
# 기록 첫 줄 읽기
buf=w.alloc(400)
def wstr(s):a=w.alloc(len(s)*2+8);w.wstr(a,s,True);return a
w.fs['SAN14_Traits9_UI.log']=UI_SKIP
assert w.run(T[FIRST],(wstr('SAN14_Traits9_UI.log'),buf,320))&0xff==1
got=w.r(buf,200).split(b'\0')[0];assert got==UI_SKIP.rstrip(b'\r\n'),got;print('첫 줄 읽기 OK:',got[:40])
assert w.run(T[FIRST],(wstr('없는파일.log'),buf,320))&0xff==0;print('없는 파일 → false OK')
def cstr(s):a=w.alloc(len(s)+8);w.w(a,s+b'\0');return a
assert w.run(T[HAS],(cstr(UI_SKIP),cstr(b'SKIPPED')))&0xff==1 and w.run(T[HAS],(cstr(b'APPLIED - x'),cstr(b'SKIPPED')))&0xff==0;print('has OK')
# 알림: 기록 줄 + 창, 같은 글은 두 실행까지
def rep(code,detail,what):return w.run(T[REPORT],(cstr(code),cstr(detail),wstr(what)))
rep(b'C',UI_SKIP.rstrip(),'화면 하나 실패')
assert len(boxes(w))==1 and 'R42 T9 C' in boxes(w)[0][2] and 'DISPLAY_SKIPPED' in boxes(w)[0][2] and '댓글' in boxes(w)[0][2],boxes(w)
assert any(l.startswith('R42T9\tC\t') and 'DISPLAY_SKIPPED' in l for l in lines(w)),lines(w);print('창 1번 + 적용확인.log R42T9 줄 OK')
rep(b'C',UI_SKIP.rstrip(),'화면 하나 실패');assert len(boxes(w))==2;print('같은 글 두 번째(다음 실행 가정) 창 OK')
rep(b'C',UI_SKIP.rstrip(),'화면 하나 실패');assert len(boxes(w))==2 and sum(1 for l in lines(w) if l.startswith('R42T9'))==3;print('같은 글 세 번째 → 창 없이 기록만 OK')
rep(b'A',b'slots=0/5','다른 글');assert len(boxes(w))==3
rep(b'B',b'ui=nofile','또 다른 글');assert len(boxes(w))==3,'한 실행 최대 3창';print('한 실행 창 최대 3개 OK')
# AI 모드 흔적 → 안내 한 줄
w2,T2=boot({'SAN14_CombatAI.dll':b'MZ'})
w2.run(T2[REPORT],(0,0,0)) if False else None
a=w2.alloc(64);w2.w(a,b'C\0');d=w2.alloc(64);w2.w(d,b'ALLSCREEN_SKIPPED\0');t=w2.alloc(64);w2.wstr(t,'x',True)
w2.run(T2[REPORT],(a,d,t));assert len(boxes(w2))==1 and 'AI 모드' in boxes(w2)[0][2];print('AI 모드 흔적 → 안내 줄 OK')
# Notice=0 → 창 없음, 기록만
w3,T3=boot(notice=0)
a=w3.alloc(64);w3.w(a,b'C\0');d=w3.alloc(64);w3.w(d,b'X_SKIPPED\0');t=w3.alloc(64);w3.wstr(t,'x',True)
w3.run(T3[REPORT],(a,d,t));assert not boxes(w3) and any(l.startswith('R42T9') for l in lines(w3));print('Notice=0 → 기록만 OK')
assert not [p for p in w.patches if p[0]=='SAN14_Traits9Check.dll'],'게임 코드 안 바꿈';print('게임 코드 쓰기 0 OK')
print('ALL OK')
# 감시 스레드 흐름(Sleep 은 바로 돌아오고, 4번째 Sleep 에서 멈춤)
def watch_run(fs_extra,patch):
    w,T=boot(fs_extra);WATCH=w.run(w.modules['san14_traits9check.dll']['exports']['SAN14_Traits9CheckTestTable'],(11,))
    patch(w);cnt=[0]
    def sl():
        if w.arg(0) in (4000,500):return 0   # R42c: 시작 4초·화면 부품 연결 기다림은 세지 않음(16000 = 옛 20000 자리)
        cnt[0]+=1
        if cnt[0]==2:w.fs['SAN14_Traits9_UI.log']=UI_SKIP  # 화면 열 때 실패 줄이 생김
        if cnt[0]>=4:raise StopEmu('stop')
        return 0
    w.api.Sleep=sl
    try:w.run(WATCH,(0,),label='watch')
    except Exception as e:pass
    return w
def full(w):
    for a,_,af in SITES:w.w(GAME_BASE+a,bytes.fromhex(af))
    w.w(GAME_BASE+0x491f01,bytes.fromhex('ff2500000000')+struct.pack('<Q',0x1e0001234))
w=watch_run({'SAN14_Traits9_UI.log':b'SAN14 Traits9 UI v2.6: APPLIED - nine-trait UI\r\n'},full)
b=boxes(w);print('정상 시작 → 나중에 화면 실패:',[x[2][:30] for x in b])
assert len(b)==1 and 'R42 T9 C' in b[0][2] and any(l.startswith('R39\t9\tSAN14_Traits9Check.dll') and '\tOK\t6/6' in l for l in lines(w)),lines(w)
w=watch_run({},lambda w:None);b=boxes(w);print('9칸 안 켜짐:',[x[2][:40] for x in b]);assert 'R42 T9 A' in b[0][2] and 'slots=0/5' in b[0][2]
def nine_only(w):
    for a,_,af in SITES:w.w(GAME_BASE+a,bytes.fromhex(af))
w=watch_run({'SAN14_Traits9_UI.log':b'SAN14 Traits9 UI: SKIPPED - editor lifecycle hook conflict; no UI code changed.\r\n','SAN14_Traits9_UI.dll':b'MZ'},nine_only)
b=boxes(w);print('화면 부품 시작 실패:',[x[2][:40] for x in b]);assert len(b)==2 and 'R42 T9 B' in b[0][2] and 'editor lifecycle' in b[0][2] and 'R42 T9 C' in b[1][2]
print('WATCH ALL OK')
