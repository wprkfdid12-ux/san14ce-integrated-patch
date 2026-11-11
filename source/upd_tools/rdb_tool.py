# 0002 색인(.rdb) 다루기 — 설치기 Engine.cs ParseRdb/WriteRdb 와 같은 규칙. (R43 2-2 도구)
import struct,hashlib,sys
def parse(b):
    if len(b)<32 or b[:8]!=b'_DRK0000' or struct.unpack_from('<i',b,8)[0]!=32:return None
    n=struct.unpack_from('<I',b,16)[0];head=bytes(b[:32]);recs=[];pos=32
    for i in range(n):
        if pos+56>len(b) or b[pos:pos+8]!=b'IDRK0000':return None
        size=struct.unpack_from('<q',b,pos+8)[0]
        if size<56 or pos+size>len(b):return None
        recs.append((struct.unpack_from('<I',b,pos+36)[0],bytes(b[pos:pos+size])));pos=(pos+size+3)&~3
    if pos!=len(b) or len(set(k for k,_ in recs))!=len(recs):return None
    return head,recs
def write(head,recs):
    h=bytearray(head);struct.pack_into('<I',h,16,len(recs));out=bytearray(h)
    for k,v in recs:
        out+=v;out+=bytes((4-len(v)%4)%4)
    return bytes(out)
def ours(U,O):
    """합본 U 에서 원본 O 에 없거나 다른 레코드 = 우리 것; replaced = 원본에도 있는 것의 원본 레코드"""
    om=dict(O[1]);ours=[];rep={}
    for k,v in U[1]:
        if k not in om:ours.append((k,v))
        elif om[k]!=v:ours.append((k,v));rep[k]=om[k]
    return ours,rep
def apply(C,ours,rep):
    """현재 색인 C 에 우리 레코드 넣기(설치기 ModelPatch 와 같음). 충돌이면 (None, 이유)"""
    head,recs=C;recs=list(recs);idx={k:i for i,(k,_) in enumerate(recs)}
    for k,v in ours:
        i=idx.get(k)
        if k in rep:
            if i is None:return None,'교체 대상 레코드 %d 없음'%k
            if recs[i][1]==v:continue
            if recs[i][1]!=rep[k]:return None,'레코드 %d 가 원본과 다름(게임이 바꿈?)'%k
            recs[i]=(k,v)
        else:
            if i is not None:
                if recs[i][1]!=v:return None,'레코드 %d 를 다른 내용으로 씀'%k
                continue
            recs.append((k,v));idx[k]=len(recs)-1
    return write(head,recs),'ok'
if __name__=='__main__':
    for p in sys.argv[1:]:
        r=parse(open(p,'rb').read());print(p,'records',len(r[1]) if r else None)
