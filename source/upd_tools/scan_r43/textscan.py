# 바뀐 자료 바이트가 글자열(ASCII/UTF-16) 한가운데에 있는 경우 = 문자열이 주소로 오인돼 깨진 것
import os,sys,struct
CUR,NEW=sys.argv[1:3]
def pr(c):return c==0 or 0x20<=c<0x7f or c in(9,10,13)
for root,ds,fs in os.walk(NEW):
    for f in fs:
        pn=os.path.join(root,f);pc=os.path.join(CUR,os.path.relpath(pn,NEW))
        if not os.path.exists(pc) or not f.lower().endswith(('.dll','.exe','.v2')):continue
        a=open(pc,'rb').read();b=open(pn,'rb').read()
        if a==b or len(a)!=len(b):continue
        i=0
        while i<len(a):
            if a[i]!=b[i]:
                j=i
                while j<len(a) and a[j]!=b[j]:j+=1
                ctx=a[max(0,i-12):i]+a[j:j+12];old=a[i:j]
                score=sum(pr(c) for c in ctx)/max(1,len(ctx))
                letters=sum(1 for c in ctx+old if 0x41<=c<=0x7a)
                if score>=0.95 and all(pr(c) for c in old) and letters>=8:
                    print(os.path.relpath(pn,NEW),hex(i),repr(a[max(0,i-12):j+12]),'->',repr(b[max(0,i-12):j+12]))
                i=j
            i+=1
