# 모듈 안 4바이트 값 중 '옛 게임 자리 A - (옛 자리 B + 5)' (call/jmp rel32) 이면서 새 판에서 안 바뀐 것 = 두 자리 사이 상대값 미번역 의심
import os,sys,struct,re
CUR,NEW=sys.argv[1:3]
for root,ds,fs in os.walk(NEW):
    for f in fs:
        pn=os.path.join(root,f);pc=os.path.join(CUR,os.path.relpath(pn,NEW))
        if not f.lower().endswith(('.dll','.exe','.v2')) or not os.path.exists(pc):continue
        a=open(pc,'rb').read();b=open(pn,'rb').read()
        if a==b or len(a)!=len(b):continue
        # 옛 게임 rva 후보: 바뀐 4바이트 자리의 옛 값(0x1000~0x2a00000)
        R=set()
        for i in range(0,len(a)-3):
            if a[i:i+4]!=b[i:i+4]:
                v=struct.unpack_from('<I',a,i)[0]
                if 0x1000<=v<0x2a00000:R.add(v)
        R=sorted(R)
        if len(R)>4000:R=R[:4000]
        want={}
        for A in R:
            for B in R:
                if A!=B:want[(A-(B+5))&0xffffffff]=(A,B)
        hits=[]
        for i in range(0,len(a)-3):
            v=struct.unpack_from('<I',a,i)[0]
            if v in want and a[i:i+4]==b[i:i+4] and (0xff000000<v<=0xffff0000 or 0x10000<=v<0x1000000):
                hits.append((i,hex(v),[hex(x) for x in want[v]]))
        if hits:print(os.path.relpath(pn,NEW),len(R),hits[:12])
