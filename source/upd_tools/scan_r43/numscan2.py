import sys,os,re,subprocess,pickle,struct,collections
CUR,NEW=sys.argv[1:3]
NV='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver/'
m=pickle.load(open(NV+'대응/함수대응.pkl','rb'))['m']
def secs(p):
    b=open(p,'rb').read();pe=struct.unpack_from('<I',b,0x3c)[0];ns=struct.unpack_from('<H',b,pe+6)[0];so=pe+0x18+struct.unpack_from('<H',b,pe+0x14)[0]
    return [(struct.unpack_from('<8s',b,so+i*40)[0].rstrip(b'\0').decode(),)+struct.unpack_from('<II',b,so+i*40+8) for i in range(ns)]
OS=secs(NV+'old.dec.exe');NS=secs(NV+'new.dec.exe')
def sec(S,v):
    for n,vs,va in S:
        if va<=v<va+vs:return n
# 자료 구간 옮김 폭 모음(옛 값→새 값 차이가 자주 나오는 것)
def dis(p):
    o=subprocess.run(['objdump','-d','--no-show-raw-insn',p],capture_output=True,text=True).stdout;r={}
    for l in o.splitlines():
        mm=re.match(r'\s*([0-9a-f]+):\t(.*)',l)
        if mm:r[int(mm.group(1),16)]=mm.group(2).strip()
    return r
rows=[]
for root,ds,fs in os.walk(NEW):
    for f in fs:
        pn=os.path.join(root,f);pc=os.path.join(CUR,os.path.relpath(pn,NEW))
        if not f.lower().endswith(('.dll','.exe','.v2')) or not os.path.exists(pc):continue
        a_=open(pc,'rb').read();b_=open(pn,'rb').read()
        if a_==b_ or len(a_)!=len(b_):continue
        a=dis(pc);b=dis(pn);ks=sorted(b)
        for i,k in enumerate(ks):
            if k not in a or a[k]==b[k]:continue
            xo=[int(x,16) for x in re.findall(r'0x([0-9a-f]+)',a[k])];xn=[int(x,16) for x in re.findall(r'0x([0-9a-f]+)',b[k])]
            for o,n in zip(xo,xn):
                if o==n or o>=0x80000000:continue
                rows.append((os.path.relpath(pn,NEW),k,o,n,a[k],b[k],b[ks[i-1]] if i else ''))
dd=collections.Counter(n-o for _,_,o,n,_,_,_ in rows if sec(OS,o) and sec(OS,o)!='.text')
common=set(d for d,c in dd.items() if c>=3)
for r in rows:
    f,k,o,n,ta,tb,prev=r;s=sec(OS,o)
    arith=bool(re.match(r'(add|sub|cmp|imul|and|or)\s+\$0x[0-9a-f]+,%e',tb)) or 'imul' in prev
    if s=='.text':
        ok=m.get(o)==n
    elif s:
        ok=(n-o) in common
    else: ok=False
    if arith or not ok:
        print(f,hex(k),hex(o),'->',hex(n),'sec',s,'map',hex(m.get(o,0)),'ARITH' if arith else '','|',tb,'| prev:',prev[:40])
