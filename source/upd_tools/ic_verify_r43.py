# R43 IC 이식 결과를 '게임 없이' 검증: IC 가 시작 때 하는 검사(0x92a 표·전역 호환표·포인터표·훅표·기능 서술자)를 새 exe 바이트로 흉내 낸다.
#  사용: python3 ic_verify_r43.py <새 IC dll>
import sys,os,struct;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
d=open(sys.argv[1],'rb').read();IB=X.IB;ICB=0x180000000
def secs(b):
    pe=struct.unpack_from('<I',b,0x3c)[0];ns=struct.unpack_from('<H',b,pe+6)[0];opt=struct.unpack_from('<H',b,pe+20)[0]
    return [(b[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',b,pe+24+opt+40*i+8) for i in range(ns)]
S=secs(d)
def off(r):
    for n,vs,va,rs,ra in S:
        if va<=r<va+max(vs,rs):return ra+r-va
bad=[];ok=0
# 1) 0x92a
for i in range(0x92a):
    rva,ln,bo=struct.unpack_from('<QII',d,off(0xb7310)+16*i);b=d[off(0xb5710)+bo:off(0xb5710)+bo+ln]
    if X.nrd(rva,ln)!=b:bad.append(('0x92a',i,hex(rva),ln,b.hex(),X.nrd(rva,ln).hex()))
    else:ok+=1
# 2) 0xaefc0 ↔ game+0xffcbe8
if X.nrd(0xffcbe8,32)!=d[off(0xaefc0):off(0xaefc0)+32]:bad.append(('aefc0',))
else:ok+=1
# 3) 전역 호환표 0xdd240..0xde800 (rva,len,buf1,buf2)
r=0xdd240
while r<0xde800:
    rva,ln,p1,p2=struct.unpack_from('<QQQQ',d,off(r));g=X.nrd(rva,ln);b1=d[off(p1-ICB):off(p1-ICB)+ln];b2=d[off(p2-ICB):off(p2-ICB)+ln] if ICB<=p2<ICB+0x300000 else None
    if g==b1 or (b2 is not None and g==b2):ok+=1
    else:bad.append(('전역',hex(r),hex(rva),ln,'첫 차이 @',next((i for i in range(min(len(g),ln)) if g[i]!=b1[i]),None)))
    r+=32
# 4) 포인터표 0xde800..0xdedd0
r=0xde800
while r<0xdedd0:
    a,b=struct.unpack_from('<QQ',d,off(r));c=X.nrd(a,8)
    if len(c)==8 and struct.unpack('<Q',c)[0]==IB+b:ok+=1
    else:bad.append(('포인터',hex(r),hex(a),hex(b),c.hex()))
    r+=16
# 5) 훅표 0xdedd0.. 51 항목
r=0xdedd0
for i in range(51):
    a,t,det,l,op=struct.unpack_from('<5Q',d,off(r))
    if l==8:
        c=struct.unpack('<Q',X.nrd(a,8))[0]-IB;(bad.append(('훅vt',hex(a),hex(t),hex(c))) if c!=t else None)
    else:
        b=X.nrd(a,5);tg=(a+5+struct.unpack('<i',b[1:5])[0])&0xffffffff
        if b[0]!=op or tg!=t:bad.append(('훅call',hex(a),hex(t),b.hex()))
    ok+=1;r+=0x28
# 6) 기능 서술자(24바이트 {rva,len,ptr}) 전부: 새 exe 바이트와 비교
feat_bad=[]
for n,vs,va,rs,ra in S:
    if n not in('.rdata','.data'):continue
    for k in range(0,rs-23,8):
        rva,ln,p=struct.unpack_from('<QQQ',d,ra+k)
        if not(0x1000<=rva<0x1340000 and 1<=ln<=0x2000 and ICB+0x9a000<=p<ICB+0x210000):continue
        po=off(p-ICB)
        if po is None:continue
        b=d[po:po+ln];g=X.nrd(rva,ln)
        if len(g)!=ln:continue
        if g==b:ok+=1
        else:
            # 옛 exe 와 같은(=안 바꾼) 것만 보고: 우연 일치 표 제외
            if X.ord_(rva,ln)==b or X.imask('old',rva,ln)==X.imask('new',rva,ln) or sum(1 for i in range(ln) if g[i]!=b[i])<=ln//3:feat_bad.append(('기능서술자',hex(va+k),hex(rva),ln,'차이',sum(1 for i in range(ln) if g[i]!=b[i])))
print('통과',ok,'실패',len(bad),'기능 서술자 불일치',len(feat_bad))
for x in bad[:40]:print(' ',x)
for x in feat_bad[:40]:print(' ',x)
