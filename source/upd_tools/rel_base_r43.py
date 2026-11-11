# 회귀 (xi): 'base 를 trampoline 기준으로 바꾼 뒤' 쓰는 상대 주소 상수 — sub rdx,r8 뒤 lea rax,[rdx+K] (K=게임 rva) / 범위 검사 mov eax,0x80000000+K
#  같은 모듈 옛/새 명령이 똑같이 남아 있고(번역 안 됨) map_code(K) 가 K 와 다르면 보고. 근처에 번역된 lea r,[rdx+K'] 가 있으면 같은 이동량인지도 봄.
#  사용: python3 rel_base_r43.py <옛 payload> <새 payload> <목록>
import sys,os,re,subprocess
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
ALL='--all' in sys.argv;A,B,LST=[a for a in sys.argv[1:] if not a.startswith('--')][:3];sys.argv=['x','a','b','c']
import newmap as X
def dis(p):
    out=subprocess.run(['objdump','-d','-M','intel',p],capture_output=True,text=True).stdout;L=[]
    for ln in out.split('\n'):
        m=re.match(r'\s+([0-9a-f]+):\t[0-9a-f ]+\t(.*)',ln)
        if m:L.append((int(m.group(1),16),m.group(2).strip()))
    return L
for rel in open(LST).read().split():
    pa,pb=os.path.join(A,rel),os.path.join(B,rel)
    if not(os.path.exists(pa) and os.path.exists(pb)):continue
    la,lb=dis(pa),dis(pb)
    if len(la)!=len(lb):continue
    for i,((ad,ia),(bd,ib)) in enumerate(zip(la,lb)):
        if ia!=ib:continue
        m=re.match(r'lea\s+\w+,\[(r\w+|e\w+)\+(0x[0-9a-f]+)\]$',ib) or re.match(r'(?:mov|add|sub|cmp)\s+\w+,(0x[0-9a-f]{8})$',ib)
        if not m:continue
        v=int(m.groups()[-1],16)
        if v>=0x80000000 and v<0x81400000:K=v-0x80000000;kind='range'
        elif m.re.pattern.startswith('lea') and m.group(1) not in('rip','rsp','rbp') and 0x10000<v<0x1341000:K=v;kind='lea'
        else:continue
        n,how=X.map_code(K)
        if n is None or n==K:continue
        # 근처 40명령 안에 번역된 lea [rdx+..] 가 있나(같은 기능의 다른 자리) — 있으면 '진짜 게임 주소 묶음'일 가능성 높음
        near=[]
        for j in range(max(0,i-40),min(len(la),i+40)):
            if la[j][1]!=lb[j][1]:
                ma=re.search(r'\+(0x[0-9a-f]+)\]',la[j][1]);mb=re.search(r'\+(0x[0-9a-f]+)\]',lb[j][1])
                if ma and mb:near.append('%s→%s'%(ma.group(1),mb.group(1)))
        if near or kind=='range' or ALL:print(rel,hex(bd),ib,'→',hex(n+(v-K)),'('+(how or '')[:24]+')','주변번역:',' '.join(near[:4]))
