# 옛/새 모듈 같은 자리 명령 비교: 바뀐 작은 즉시값(0x100~0x100000) 중 '새 exe 가 옛 값을 구조체 변위로 계속 쓰는' 것 = 오역 의심
import sys,os,re,subprocess,pickle,collections
CUR,NEW=sys.argv[1:3]
N='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver/대응/new.dec.exe.asm.pkl'
cache='/tmp/claude-0/pt/dispcount.pkl'
if os.path.exists(cache):cnt=pickle.load(open(cache,'rb'))
else:
    d=pickle.load(open(N,'rb'));cnt=collections.Counter()
    for a,(m,op,_) in d.items():
        for x in re.findall(r'[+-]0x([0-9a-f]+)\]',op):cnt[int(x,16)]+=1
    pickle.dump(cnt,open(cache,'wb'))
def dis(p):
    o=subprocess.run(['objdump','-d','--no-show-raw-insn',p],capture_output=True,text=True).stdout;r={}
    for l in o.splitlines():
        m=re.match(r'\s*([0-9a-f]+):\t(.*)',l)
        if m:r[int(m.group(1),16)]=m.group(2).strip()
    return r
for root,ds,fs in os.walk(NEW):
    for f in fs:
        if not f.lower().endswith(('.dll','.exe','.asi','.v2')):continue
        pn=os.path.join(root,f);pc=os.path.join(CUR,os.path.relpath(pn,NEW))
        if not os.path.exists(pc) or open(pc,'rb').read()==open(pn,'rb').read():continue
        if os.path.getsize(pc)!=os.path.getsize(pn):continue
        a=dis(pc);b=dis(pn)
        for k in sorted(b):
            if k not in a or a[k]==b[k]:continue
            xo=[int(x,16) for x in re.findall(r'0x([0-9a-f]+)',a[k])];xn=[int(x,16) for x in re.findall(r'0x([0-9a-f]+)',b[k])]
            for o,n in zip(xo,xn):
                if o!=n and 0x100<=o<0x100000 and cnt[o]>=5 and cnt[n]*10<cnt[o]:
                    print(os.path.relpath(pn,NEW),hex(k),hex(o),'->',hex(n),'old_uses',cnt[o],'new_uses',cnt[n],'|',b[k])
