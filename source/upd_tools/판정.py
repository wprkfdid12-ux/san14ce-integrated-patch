# 데모(실제 게임 실행) 기록으로 통합패치 34개 항목 + 제보 회귀 규칙을 한 번에 판정.
#  설치기의 '게임 적용' 판정(Engine.cs ParseApplyLog·Judge·T9Screen)을 그대로 옮기고, 그 위에 회귀 규칙(회귀규칙.tsv)·튕김·프레임을 더함.
#  읽기만 함(아무 파일도 안 씀). 결과는 화면에 출력. 실패가 하나라도 있으면 종료 코드 1.
#
#  사용: python3 판정.py <게임 폴더> [runtime.log 경로] [--since "2026-11-02 21:58"] [--rules 회귀규칙.tsv]
#   · 게임 폴더: SAN14CE_KO.exe 가 있는 곳(SAN14_기록\적용확인.log, SAN14CE_11Patch.log, SAN14_CrashLog.log 등을 읽음)
#   · runtime.log: %LOCALAPPDATA%\SAN14CE_ModCore\runtime.log (안 주면 게임 폴더 옆 SAN14CE_ModCore\runtime.log 를 찾아 봄)
#   · --since: 이 시각 뒤 기록만(안 주면 마지막 실행의 시작 줄 시각 1분 전부터)
import os,re,sys,datetime,collections
A=sys.argv[1:]
def opt(k,d=None):
    if k in A:i=A.index(k);v=A[i+1];del A[i:i+2];return v
    return d
SINCE=opt('--since');RULES=opt('--rules',os.path.join(os.path.dirname(os.path.abspath(__file__)),'회귀규칙.tsv'))
ROOT=A[0];RT=A[1] if len(A)>1 else None
if not RT:
    for c in(os.path.join(ROOT,'..','SAN14CE_ModCore','runtime.log'),os.path.join(os.environ.get('LOCALAPPDATA',''),'SAN14CE_ModCore','runtime.log')):
        if os.path.exists(c):RT=c;break
def rd(p,utf16=False,tail=3*1024*1024):
    try:
        with open(p,'rb') as f:
            f.seek(0,2);n=f.tell();f.seek(max(0,n-tail));b=f.read()
        if utf16:
            if len(b)&1:b=b[1:]
            return b.decode('utf-16-le','replace')
        return b.decode('utf-8','replace')
    except OSError:return ''
J=lambda *p:os.path.join(ROOT,*p)
NAMES=["모드 설정 창","턴 종료 자동(포상·명승·논공·자동인사)","문전박대","연출 생략(버튼 8개)","함께 이동·수행","진행 배속","인재·포로 편의","BGM","개성 9개·콜라보","고유 무장 등록·3D 모델",
 "필드 3D 조형","필드 안개","논공 개성 가챠","개성·전법 설명","연계 탭","출진 프리셋","능력치 상한 200","경험치 능력치","120프레임","글씨체",
 "포로 관직 유지·귀환","내정 자동화(스위치 5개)","도시 상한 확장","수여 능력치 미적용","타 세력 재야","기능확장 v1.5","독립 미니맵","인구 보너스","항장·부상 색","편의성 v11",
 "주메뉴 그래픽 v30","고용무장 얼굴","사거리 설정","지역담당관 공석 경고"]
# ---- 기록 읽기
ap=rd(J('SAN14_기록','적용확인.log'))
lines=[l.rstrip('\r') for l in ap.split('\n') if l.strip()]
TS=re.compile(r'(\d{4}-\d{1,2}-\d{1,2} \d{1,2}:\d{2}(?::\d{2})?)')
def t_of(s):
    m=TS.search(s)
    if not m:return None
    for f in('%Y-%m-%d %H:%M:%S','%Y-%m-%d %H:%M'):
        try:return datetime.datetime.strptime(m.group(1),f)
        except ValueError:pass
if SINCE:since=t_of(SINCE)
else:
    st=[t_of(l) for l in lines if l.startswith(('R39\t','R42D\t')) and ('\tstart\t' in l or l.startswith('R39\t'))]
    st=[x for x in st if x];since=None
    if st:
        last=max(st);since=last-datetime.timedelta(minutes=1)
        # 같은 실행의 시작 줄 묶음 맨 앞으로
        for x in sorted(st,reverse=True):
            if last-x<datetime.timedelta(minutes=3):since=x-datetime.timedelta(minutes=1)
# 시작 줄 뒤 기록만(시각 없는 줄은 앞 줄 시각을 따름)
cur=None;sess=[]
for l in lines:
    t=t_of(l)
    if t:cur=t
    if since and cur and cur>=since:sess.append(l)
rt=rd(RT) if RT else '';k=rt.rfind('SAN14 ModCore 1.0');rt=rt[k:] if k>=0 else rt
ic=rd(J('SAN14CE_11Patch.log'),utf16=True);k=ic.rfind('지원 실행 파일 확인 완료');ic=ic[k:] if k>=0 else ic
R=lambda x:x in rt;I=lambda x:x in ic
# 설치한 항목(설치기 기록 .san14ce_compat_r3/state.xml). 없으면 전부 설치한 것으로 봄
ON=set(range(1,35))
_st=rd(J('.san14ce_compat_r3','state.xml'))
if _st:ON={int(a) for a,b in re.findall(r'<Feature id="(\d+)" enabled="(true)"',_st)}
# ---- 1) 항목 판정
rec=collections.defaultdict(list);cnt=collections.defaultdict(dict)
for l in sess:
    c=l.split('\t')
    if c[0]=='R39' and len(c)>=8 and c[1].isdigit():rec[int(c[1])].append(dict(mod=c[2],time=c[3],state=c[4],sites=c[5],set=c[6],why=c[7]))
    elif c[0]=='R42D' and len(c)>=8 and c[3]=='start':rec[int(c[1])].append(dict(mod=c[2],time=c[4],state=c[5],sites=c[6],set=c[7],why=c[8] if len(c)>8 else '-'))
    elif c[0]=='R39C' and len(c)>=5 and c[1].isdigit():cnt[int(c[1])][c[3]]=c[4]
def judge_old(i):
    ok='적용됨';no='적용 안 됨 – 시작 기록 없음'
    if i==1:
        if R('settings page 6 not attached') or R('page 6 write rejected'):return '적용 안 됨 – 설정 창 5·6쪽 연결 실패'
        return ok if I('정보 메뉴 모드 설정 설치') and R('settings pages attached') else ('일부 적용 – 설정 창 1~4쪽만' if I('정보 메뉴 모드 설정 설치') else no)
    if i==2:return ok if I('자동인사 설치') else no
    if i==3:return '적용 안 됨 – 외교 메뉴 코드가 달라 설치 실패' if I('문전박대 설치 실패') else (ok if I('문전박대 설치:') else no)
    if i==4:
        if not(I('이벤트 자동 포커스') and I('행동보고·수지보고 화면 생략') and I('도시 증축 암전 제거 설치')):return no
        if not R('SceneSkip R42:'):return '일부 적용 – 재해·등용 결과·보물 발견 생략 시작 기록 없음'
        return '일부 적용 – 재해·등용·보물 일부 자리가 달라 원래대로' if 'NOT hooked' in rt[rt.rfind('SceneSkip R42:'):].split('\n')[0] else ok
    if i==5:return ok if I('대열 이동') and I('수행 연결') else no
    if i==6:return ok if I('진행속도 확장') else no
    if i==7:return ok if I('인재 메뉴') else no
    if i==8:return ok if I('BGM 플레이어 연결') else no
    if i==9:return ok if R('Collab entry invoked') else no
    if i==10:return '적용 안 됨 – 모델 ID 확장 확인 실패' if I('CAPACITY100') else (ok+' (모델 0~173)' if I('확장 ID 25~173 예약') else no)
    if i==11:return '적용 안 됨 – 3D 런타임 시작 실패' if R('automatic runtime launch FAILED') else (ok if R('FieldLeader3D automatic runtime started') else no)
    if i==13:return (ok if R('SAN14CE.MeritTraits.HoverX RVA=') else '적용 안 됨 – 13번 시작 실패') if (R('MeritTraits:') or R('MeritTraits RC4')) else no
    if i==14:return ok if R('TraitHover entry invoked') else no
    if i==15:return ok if R('RelatedOfficersTab RC2.4: HOOKS_INSTALLED') else no
    if i==16:return ok if R('SortiePresets RVA=') else no
    if i==17:return ok if R('reader cap 100 -> 200 applied') and R('Stats200 entry invoked') else ('일부 적용 – 편집 무장 읽기 상한 미적용' if R('Stats200 entry invoked') else no)
    if i==19:return ok if R('FPS120 R8 ACTIVE') else no
    if i==21:return ok if R('PrisonerReturn R4 loaded') else no
    if i==22:return '적용 안 됨 – 읽기 전용 모드' if R('READ-ONLY') else (ok if R('DomesticCE: ACTIVE') else no)
    if i==23:return ok if R('CityCaps: city limits extended') else no
    if i==25:return ok if R('Patch27: checked feature groups') else no
    if i in(18,20,24,31):return '파일만 바꾸는 항목(설치기 표시로 확인)'
    return ''
res={}
for i in range(1,35):
    if i not in ON:res[i]=('OFF','설치 안 함');continue
    old=judge_old(i)
    if rec.get(i):
        bad=[r for r in rec[i] if r['state'] not in('OK','OFF')];off=[r for r in rec[i] if r['state']=='OFF']
        st='FAIL' if any(r['state']=='FAIL' for r in bad) else 'PART' if bad else ('OFF' if off and len(off)==len(rec[i]) else 'OK')
        why='; '.join(f"{r['mod']} {r['state']} {r['sites']} {r['why']}" for r in bad) or ', '.join(f"{r['mod']} {r['sites']}" for r in rec[i])
        res[i]=(st,why)
    elif old:res[i]=('OK' if old.startswith('적용됨') else 'FILE' if old.startswith('파일') else 'PART' if old.startswith('일부') else 'FAIL',old)
    else:res[i]=('NONE','기록 없음(설치 안 했거나 이 판단에 쓰는 기록이 없음)')
# 9칸 화면
t9=[l for l in sess if l.startswith('R42T9\t')]
t9ui=rd(J('SAN14_Traits9_UI.log')).strip().split('\n')[-1:] or ['']
t9bad=[l for l in t9 if l.startswith('R42T9\tC') or ('relaxed=fail' in l) or ('label create failed' in l)]
if res[9][0]=='OK' and (t9bad or (('SKIPPED' in t9ui[0] and 'HOVER_SKIPPED' not in t9ui[0]) or 'FAILED' in t9ui[0])):res[9]=('PART','개성 9칸 화면 실패: '+(t9bad[0][:120] if t9bad else t9ui[0][:120]))
# ---- 2) 튕김·멈춤
crash=rd(J('SAN14_CrashLog.log'));crashes=[]
for m in re.finditer(r'=== (\d{4}-\d{1,2}-\d{1,2} \d{1,2}:\d{2}:\d{2})[^\n]*\n위치: ([^\n]*)',crash):
    t=t_of(m.group(1))
    if since and t and t>=since:crashes.append(f'{m.group(1)} {m.group(2).strip()}')
notice=[l for l in sess if l.startswith('R40E\tMC')]
# ---- 3) 회귀 규칙
def num(pat,src,agg=max):
    v=[int(x) for x in re.findall(pat,src)];return agg(v) if v else None
S='\n'.join(sess)
ctx=dict(ON=ON,S=S,rt=rt,ic=ic,R=R,I=I,res=res,cnt=cnt,num=num,crashes=crashes,root=ROOT,rd=rd,J=J,since=since,re=re,os=os)
rules=[]
if os.path.exists(RULES):
    for l in open(RULES,encoding='utf-8').read().split('\n')[1:]:
        c=l.split('\t')
        if len(c)>=5 and c[0].strip():rules.append(c)
# ---- 출력
print(f'판정 기준: {since} 이후 기록 (적용확인.log {len(sess)}줄, runtime.log {len(rt)}자, IC 기록 {len(ic)}자)')
print('\n[항목]')
mark={'OK':'✔','FILE':'✔','OFF':'－','PART':'⚠','FAIL':'✖','NONE':'·'}
for i in range(1,35):
    st,why=res[i];c=cnt.get(i,{})
    extra=(' | 동작: '+', '.join(f'{k} {v}' for k,v in list(c.items())[:4])) if c else ''
    print(f' {mark[st]} {i:2d} {NAMES[i-1]:<22} {st:<4} {why[:110]}{extra[:120]}')
print('\n[튕김]',('없음' if not crashes else f'{len(crashes)}건'))
for c in crashes[:10]:print('  ',c)
if notice:print('[통합패치 전체 멈춤 안내창]',notice[-1])
fails=sum(1 for i in res if res[i][0]=='FAIL')+len(crashes)
print('\n[회귀 규칙]')
for c in rules:
    rid,item,what,kind,expr=c[:5];note=c[5] if len(c)>5 else ''
    if kind=='직접':print(f' ○ {rid} [{item}] {what} — 데모에서 직접 확인: {note}');continue
    try:ok=eval(expr,ctx)
    except Exception as e:ok=None;note=f'판정식 오류 {e}'
    if ok is None:print(f' · {rid} [{item}] {what} — 기록 없음(그 상황이 데모에서 안 나옴){(" "+note) if note else ""}')
    elif ok:print(f' ✔ {rid} [{item}] {what}')
    else:print(f' ✖ {rid} [{item}] {what} {note}');fails+=1
print(f'\n결과: 실패 {fails}건')
sys.exit(1 if fails else 0)
