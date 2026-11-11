import pickle,collections,re,subprocess,os,sys
P='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver/대응/'
cache='/tmp/claude-0/pt/drefs.pkl'
if os.path.exists(cache):O,N=pickle.load(open(cache,'rb'))
else:
    def refs(tag):
        d=pickle.load(open(P+tag+'.dec.exe.asm.pkl','rb'));s=collections.Counter()
        for a,(m,ops,tgt) in d.items():
            if tgt:
                v=int(tgt,16)-0x140000000
                if 0x1340000<=v<0x2a00000:s[v]+=1
        return s
    O=refs('old');N=refs('new');pickle.dump((O,N),open(cache,'wb'))
OS=sorted(O)
import bisect
def best_shift(ov,nv):
    lo=bisect.bisect_left(OS,ov-0x200);hi=bisect.bisect_right(OS,ov+0x200)
    ks=OS[lo:hi]
    if len(ks)<3:return None
    res=[]
    for dl in range(-0x100,0x101,4):
        sc=sum(1 for k in ks if N.get(k+(nv-ov)+dl,0)==O[k])
        res.append((sc,-abs(dl),dl))
    res.sort(reverse=True);sc,_,dl=res[0];sc0=[r for r in res if r[2]==0][0][0]
    return dl,sc,sc0,len(ks)
def imms(p):
    out=subprocess.run(['objdump','-d','--no-show-raw-insn',p],capture_output=True,text=True).stdout
    r={}
    for l in out.splitlines():
        m=re.match(r'\s*(18[0-9a-f]+):\s+(\S+)\s+(.*)',l)
        if not m:continue
        for x in re.findall(r'\$0x([0-9a-f]+)',m.group(3)):
            v=int(x,16)
            if 0x1340000<=v<=0x2a00000:r[int(m.group(1),16)]=v
    return r
CUR='/home/claude/r41/newpkg/payload_cur';NEW='/home/claude/r41/newpkg/payload_new'
seen=set()
for root,_,fs in os.walk(NEW):
    for f in fs:
        if not (f.endswith('.dll') or f.endswith('.v2')):continue
        pn=os.path.join(root,f);pc=pn.replace(NEW,CUR)
        if not os.path.exists(pc):continue
        o=imms(pc);n=imms(pn)
        pairs=collections.Counter((o[a],n[a]) for a in n if a in o and o[a]!=n[a])
        for (ov,nv),c in sorted(pairs.items()):
            if (f,ov,nv) in seen:continue
            seen.add((f,ov,nv))
            b=best_shift(ov,nv)
            if b and b[0]!=0 and b[1]>b[2]+1:
                print(f"{os.path.relpath(pn,NEW)}\t{hex(ov)}->{hex(nv)}\tshift {b[0]:+#x}\tscore {b[1]} vs {b[2]} (refs {b[3]})\tuses {c}")
