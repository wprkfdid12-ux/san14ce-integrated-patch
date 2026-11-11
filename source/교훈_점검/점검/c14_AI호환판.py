# 교훈 L142: AI 모드(행동교정) 호환판 대조. 11-11 에 v15.14 원본이 통합패치 5번·6번 시작 검사를 깼다(실행 기록 18:16).
#  자료/AI모드자리.tsv(AI 소스에서 뽑은 '게임 코드를 고치는 자리' + 호환판 상태)를 우리 쪽 자리와 대조한다.
#  ① 상태 '켜짐'(호환판에서도 도는) AI 자리가 통합패치 자리·통합편의 확인 구간/후크·기색·계절 자리와 겹치면 실패
#     → 통합패치가 새 자리를 넣었거나 AI 표가 낡음. 호환판을 다시 대조(점검표 4)하고, 문제없다고 확인했으면 자료/다른모드_확인됨.tsv 에 모드 'AI' 로 적는다.
#  ② 상태 '늦게걸기'(통합편의 확인 뒤에 거는) 자리는 통합편의 확인 구간 안이어야 하고(아니면 늦출 까닭이 없음), 실제로 고치는 자리(통합·통합편의 후크·기색·계절)와는 겹치면 안 됨.
#  ③ 호환판 결과물이 있으면 표의 강제끔 기능 = 호환판 설치 관리자 표(relabel1514.tsv)의 강제끔 28개.
import re,struct,os
from _공통 import *
o=Out('c14 AI 모드 호환판 자리 × 우리 자리 (L142)')
f=LESS+'/자료/AI모드자리.tsv'
if not os.path.exists(f):o.bad('자료/AI모드자리.tsv 없음 — ai_compat/v1514/tools/ai_table.py');o.done()
ai=[]
for l in open(f,encoding='utf-8'):
    if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
    c=l.rstrip('\n').split('\t');ai.append((int(c[0],16),int(c[1]),c[2],c[3],c[4]))
fix=[];chk=[]
for nm in ('우리자리.tsv','우리자리_추가.tsv'):
    for l in open(LESS+'/자료/'+nm,encoding='utf-8'):
        if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
        c=l.rstrip('\n').split('\t');fix.append((int(c[0],16),int(c[1]),'통합 %s'%c[2]))
for nm in ('다른모드자리.tsv','계절자리.tsv'):
    p=LESS+'/자료/'+nm
    if not os.path.exists(p):o.bad('자료/%s 없음'%nm);continue
    for l in open(p,encoding='utf-8'):
        if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
        c=l.rstrip('\n').split('\t');fix.append((int(c[0],16),int(c[1]),'%s %s'%(c[2],c[4])))
ic=PE(IC_DLL);d=ic.d
r=0xdd240;i=0
while r<0xde800:
    a,l,p1,p2=struct.unpack_from('<QQQQ',d,ic.off(r));chk.append((a,l,'통합편의 전역 확인 %d번'%i));r+=32;i+=1
r=0xde800
while r<0xdedd0:
    a,b=struct.unpack_from('<QQ',d,ic.off(r));chk.append((a,8,'통합편의 포인터 확인'));r+=16
for k in range(51):
    a,tg,det,l,op=struct.unpack_from('<5Q',d,ic.off(0xdedd0+0x28*k));fix.append((a,8 if l==8 else 5,'통합편의 후크 %d번'%k))
for k in range(28):
    a,l,p1,p2=struct.unpack_from('<QQQQ',d,ic.off(0xa3150+32*k))
    if 0<a<0x2a02000 and 0<l<0x10000:chk.append((a,l,'통합편의 6번 준비 %d번'%k))
head,rows=tsv('자료/다른모드_확인됨.tsv');ACK={(int(c[0],16),c[1]):c for n,c in rows}
tests=open(R+'/test/run_offline_all.sh',encoding='utf-8').read();names=set(re.findall(r'^(?:run|nat)\s+(\S+)',tests,re.M))
rep={c[0] for n,c in tsv('04_제보장부.tsv')[1]}
def hits(a,l,lst):return [x for s,ll,x in lst if a<s+ll and s<a+l]
from collections import Counter
st=Counter(x[3] for x in ai);o.note('AI 자리 %d곳: %s'%(len(ai),', '.join('%s %d'%kv for kv in st.most_common())))
bad=0
for a,l,key,s,ev in ai:
    if s=='켜짐':
        h=hits(a,l,fix)+hits(a,l,chk)
        if not h:continue
        k=ACK.get((a,'AI'))
        if not k:bad+=1;o.bad('켜진 AI 자리 0x%x(%d) %s [%s] — %s'%(a,l,key,ev,'; '.join(sorted(set(h)))));continue
        e=k[4].strip() if len(k)>4 else ''
        good=(e.startswith('시험:') and e[3:] in names) or e in rep
        (o.ok if good else o.bad)('켜진 AI 자리 0x%x %s — 확인됨(%s)'%(a,key,e))
    elif s=='늦게걸기':
        h1=hits(a,l,chk);h2=hits(a,l,fix)
        if h2:bad+=1;o.bad('늦게 거는 AI 자리 0x%x %s 가 고치는 자리와 겹침: %s'%(a,key,'; '.join(h2)))
        elif not h1:o.bad('늦게 거는 AI 자리 0x%x %s 가 통합편의 확인 구간 밖 — 늦출 까닭 다시 볼 것'%(a,key))
        else:o.ok('늦게 거는 AI 자리 0x%x %s — %s 안, 고치는 자리와 안 겹침'%(a,key,h1[0]))
if not bad:o.ok('켜진 AI 자리 %d곳이 통합·통합편의·기색·계절 자리와 안 겹침'%st['켜짐'])
lab=R+'/ai_compat/v1514/relabel1514.tsv'
if os.path.exists(lab):
    forced=set(c[0].split('.')[1] for c in (l.rstrip('\n').split('\t') for l in open(lab,encoding='utf-8') if l.strip() and not l.startswith('#')) if '꺼져 동작 안 함' not in c[1])
    tabf=set(k for a,l,key,s,ev in ai if s=='강제끔' for k in key.split('·'))
    if not tabf<=forced:o.bad('표의 강제끔 기능 중 호환판 강제끔 목록에 없는 것: %s'%sorted(tabf-forced))
    else:o.ok('표의 강제끔 기능 %d개가 호환판 강제끔 %d개 안에 있음'%(len(tabf),len(forced)))
o.done()
