# 사용자 목표(11-10): 통합패치의 모든 모드를 원소스가 있는 상태로. "원소스가 없어서 못 했다"가 어떤 모드에도 나오지 않게.
# 규칙: ① 꾸러미의 DLL 은 전부 05_소스장부.tsv 에 있어야 함 ② '바이너리' 줄 수는 자료/바이너리_허용수.txt 를 넘을 수 없음(줄면 그 숫자를 내릴 것)
#       ③ '소스' 줄은 소스 파일이 실제로 있고, 빌드 결과(build_out·mt/obj_new)가 꾸러미 파일과 바이트까지 같아야 함.
import glob,os
from _공통 import *
o=Out('c04 소스 장부(자립 진행)')
head,rows=tsv('05_소스장부.tsv');L={c[0]:c for n,c in rows}
pk={}
for t,s,h,w,i in manifest_files():
    if t.lower().endswith('.dll'):pk.setdefault(t.split('/')[-1],set()).add(h)
for n in sorted(pk):
    if n not in L:o.bad('%s 가 장부에 없음 — 05_소스장부.tsv 에 줄을 넣을 것'%n)
for n in sorted(L):
    if n not in pk:o.bad('%s 는 장부에 있는데 꾸러미에 없음 — 장부에서 지우거나 이름을 고칠 것'%n)
nb=0
for n,c in sorted(L.items()):
    st=c[2]
    if st=='바이너리':nb+=1
    elif st=='소스':
        files=[f for pat in c[3].split(';') for f in glob.glob(R+'/'+pat.strip())]
        if not files:o.bad('%s: 소스 칸(%s)에 맞는 파일 없음'%(n,c[3]));continue
        built=[p for p in (R+'/build_out/'+n,R+'/mt/obj_new/'+n,R+'/build_out/'+n.replace('.dll','_domestic.dll'))+tuple(glob.glob(R+'/kits/*/out/'+n)) if os.path.exists(p)]
        hs={sha(p) for p in built}
        if n in pk and pk[n]<=hs:o.ok('%s 소스 있음 · 빌드 결과 = 꾸러미'%n)
        elif n in pk:o.bad('%s: 빌드 결과(%s)와 꾸러미(%s) 해시가 다름 — 다시 빌드해 맞출 것'%(n,','.join(sorted(x[:8] for x in hs)) or '없음',','.join(sorted(x[:8] for x in pk[n]))))
    elif st!='남의런타임':o.bad('%s: 상태 칸이 이상함(%s)'%(n,st))
lim=int(open(LESS+'/자료/바이너리_허용수.txt').read().split()[0])
if nb>lim:o.bad('소스 없는 DLL 이 늘었음(%d > 허용 %d) — 새 모듈은 반드시 소스로'%(nb,lim))
elif nb<lim:o.bad('소스 없는 DLL 이 줄었음(%d < %d) — 자료/바이너리_허용수.txt 를 %d 로 내릴 것(다시 못 늘게)'%(nb,lim,nb))
else:o.ok('소스 없는 DLL %d개(허용 %d) — 남은 자립 대상'%(nb,lim))
# L119·L120: 직전 판에 바이너리였다가 이번에 소스가 된 DLL — 자립기록에 줄 + 자리 대조 시험 등록 + 한 판 4개까지
prevf=LESS+'/자료/직전판_소스장부.tsv'
if os.path.exists(prevf):
    prev={}
    for l in open(prevf,encoding='utf-8-sig'):
        c=l.rstrip('\r\n').split('\t')
        if len(c)>2 and not l.startswith('#'):prev[c[0]]=c[2]
    rec={c[0]:c for n,c in tsv('자료/자립기록.tsv')[1]}
    tests=open(R+'/test/run_offline_all.sh',encoding='utf-8').read()
    import re as _re
    names=set(_re.findall(r'^(?:run|nat)\s+(\S+)',tests,_re.M))
    conv=[n for n,c in sorted(L.items()) if c[2]=='소스' and prev.get(n)=='바이너리']
    for n in conv:
        r_=rec.get(n)
        if not r_:o.bad('%s: 이번 판에서 소스가 됐는데 자료/자립기록.tsv 에 줄이 없음(자리 대조 시험 이름을 적을 것, L120)'%n);continue
        t=r_[2].strip() if len(r_)>2 else ''
        if t=='해시일치' or t in names:o.ok('%s 자립 — 자리 대조: %s'%(n,t))
        else:o.bad('%s: 자리 대조 시험 "%s" 가 test/run_offline_all.sh 에 없음(L120)'%(n,t))
    if len(conv)>4:o.bad('이번 판에서 소스로 바꾼 모듈이 %d개 — 한 판에 4개까지(L119): %s'%(len(conv),', '.join(conv)))
    else:o.ok('이번 판에서 소스로 바꾼 모듈 %d개(한 판 4개까지)'%len(conv))
    for n,r_ in rec.items():
        if n not in L or L[n][2]!='소스':o.bad('자립기록의 %s 가 소스 장부에서 소스가 아님'%n)
else:o.bad('자료/직전판_소스장부.tsv 가 없음 — 관문.py --넘김 이 만들어야 함')
o.done()
