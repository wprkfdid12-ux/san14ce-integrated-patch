# 전수 조사 2: '두 주소의 차이' 상수 — 바뀌지 않은 즉시값/변위 D 근처(±16명령)에 번역된 주소 A(옛 Ao→새 An)가 있고
#  Ao±D 가 옛 exe 함수 시작/분기 대상인데 map(Ao±D) - An ≠ ±D 이면 보고.
import sys,os,re,subprocess,pickle
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
A,B,LST=sys.argv[1:4];sys.argv=['x','a','b','c']
import newmap as X
oi=pickle.load(open('/tmp/census/old_idx.pkl','rb'));FN=set(X.ostart);BR=oi['BR']
def dis(p):
    out=subprocess.run(['objdump','-d','-M','intel',p],capture_output=True,text=True).stdout;L=[]
    for ln in out.split('\n'):
        m=re.match(r'\s+([0-9a-f]+):\t[0-9a-f ]+\t(.*)',ln)
        if m:L.append((int(m.group(1),16),m.group(2).strip()))
    return L
nums=lambda s:[(m.group(1)=='-',int(m.group(2),16)) for m in re.finditer(r'([+-]?)(0x[0-9a-f]+)',re.sub(r'#.*','',s)) if 'rip' not in s[max(0,m.start()-4):m.start()]]
for rel in open(LST).read().split():
    pa,pb=os.path.join(A,rel),os.path.join(B,rel)
    if not(os.path.exists(pa) and os.path.exists(pb)):continue
    la,lb=dis(pa),dis(pb)
    if len(la)!=len(lb):continue
    trans=[]
    for i,((_,ia),(_,ib)) in enumerate(zip(la,lb)):
        if ia!=ib:
            na=[v for _,v in nums(ia)];nb=[v for _,v in nums(ib)]
            for x,y in zip(na,nb):
                if x!=y and 0x1000<=x<0x1341000:trans.append((i,x,y))
    import bisect;ti=[t[0] for t in trans]
    for i,((ad,ia),(bd,ib)) in enumerate(zip(la,lb)):
        if ia!=ib:continue
        mn=ib.split()[0] if ib else ''
        if mn.startswith('j') or mn=='call':continue
        for neg,D in nums(ib):
            if not(0x100<=D<0x2000000):continue
            lo=bisect.bisect_left(ti,i-16);hi=bisect.bisect_right(ti,i+16)
            for _,Ao,An in trans[lo:hi]:
                for s in(1,-1):
                    c=Ao+s*D
                    if not(c in FN or c in BR):continue
                    n,how=X.map_code(c)
                    if n is None:continue
                    if n-An!=s*D:print(rel,hex(bd),ib,'| 근처',hex(Ao),'→',hex(An),'| Ao%+d·D=%x→%x 새 차이 %x'%(s,c,n,abs(n-An)),(how or '')[:20])
