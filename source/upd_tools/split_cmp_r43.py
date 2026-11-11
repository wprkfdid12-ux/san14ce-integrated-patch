# 회귀 (x): 컴파일러 switch 이분 탐색의 '경계값' 상수(주소-1 등) — 주변 cmp 상수는 번역됐는데 경계(cmp K ; jg/ja/jl/jb/jle/jbe/jge/jae)만 그대로인 것.
#  사용: python3 split_cmp_r43.py <옛 payload> <새 payload> <목록>
import sys,os,re,subprocess
def dis(p):
    out=subprocess.run(['objdump','-d','-M','intel',p],capture_output=True,text=True).stdout
    L=[]
    for ln in out.split('\n'):
        m=re.match(r'\s+([0-9a-f]+):\t[0-9a-f ]+\t(.*)',ln)
        if m:L.append((int(m.group(1),16),m.group(2).strip()))
    return L
A,B,LST=sys.argv[1:4]
for rel in open(LST).read().split():
    pa,pb=os.path.join(A,rel),os.path.join(B,rel)
    if not(os.path.exists(pa) and os.path.exists(pb)):continue
    la,lb=dis(pa),dis(pb)
    if len(la)!=len(lb):print(rel,'명령 수 다름 — 건너뜀');continue
    for i,((ad,ia),(bd,ib)) in enumerate(zip(la,lb)):
        m=re.match(r'cmp\s+\w+,(0x[0-9a-f]+)$',ia)
        if not m or ia!=ib:continue
        K=int(m.group(1),16)
        if not(0x10000<K<0x2a00000):continue
        nxt=lb[i+1][1] if i+1<len(lb) else ''
        if not re.match(r'j(g|ge|l|le|a|ae|b|be)\s',nxt):continue
        # 주변 40 명령 안의 cmp 상수 중 옛/새가 다른 것
        diffs=[]
        for j in range(max(0,i-40),min(len(la),i+40)):
            ma=re.match(r'cmp\s+\w+,(0x[0-9a-f]+)$',la[j][1]);mb=re.match(r'cmp\s+\w+,(0x[0-9a-f]+)$',lb[j][1])
            if ma and mb and ma.group(1)!=mb.group(1):diffs.append((int(ma.group(1),16),int(mb.group(1),16)))
        if diffs:print(rel,hex(bd),ib,nxt,'주변 번역됨:',' '.join('%x→%x'%d for d in diffs[:8]))
