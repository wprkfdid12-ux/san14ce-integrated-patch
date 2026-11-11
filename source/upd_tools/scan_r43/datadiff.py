import os,struct,pickle,sys
NV='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver/'
m=pickle.load(open(NV+'대응/함수대응.pkl','rb'))['m']
CUR,NEW=sys.argv[1:3]
def secs(b):
    pe=struct.unpack_from('<I',b,0x3c)[0];ns=struct.unpack_from('<H',b,pe+6)[0];so=pe+0x18+struct.unpack_from('<H',b,pe+0x14)[0]
    return [(b[so+i*40:so+i*40+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',b,so+i*40+8) for i in range(ns)]
for root,ds,fs in os.walk(NEW):
    for f in fs:
        pn=os.path.join(root,f);pc=os.path.join(CUR,os.path.relpath(pn,NEW))
        if not os.path.exists(pc) or not f.lower().endswith(('.dll','.exe','.v2')):continue
        a=open(pc,'rb').read();b=open(pn,'rb').read()
        if a==b or len(a)!=len(b):continue
        S=secs(b)
        for nm,vs,va,rs,ro in S:
            if nm=='.text':continue
            i=ro;out=[]
            while i<ro+rs:
                if a[i:i+4]!=b[i:i+4]:
                    j=i
                    while j<ro+rs and a[j]!=b[j]:j+=1
                    out.append((i,a[i:max(j,i+4)].hex(),b[i:max(j,i+4)].hex()))
                    i=j+1
                else:i+=1
            if out:print(os.path.relpath(pn,NEW),nm,len(out),out[:6])
