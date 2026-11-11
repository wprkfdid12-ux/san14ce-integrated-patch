# 자료 구역의 (slot, target) 쌍 전수 검사 — 크기 u64/u32, 범위 무관(코드·자료·BSS 모두)
#  옛 모듈(payload_cur)에서 옛 exe [slot]==game+target 이 성립하던 쌍이, 새 모듈에서 새 exe [slot']==game+target' 로 성립하는지.
#  새 exe 파일에서 0 인 BSS 칸은 '런타임' 으로 보고(정적 확인 불가)만.
import sys,os,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
G=0x140000000
def q(R,a):
    b=R(a,8);return struct.unpack('<Q',b)[0] if len(b)==8 else None
for rel in open(sys.argv[3]).read().split():
    if 'runtime' in rel:continue
    pc=os.path.join(sys.argv[1],rel);pn=os.path.join(sys.argv[2],rel)
    if not(os.path.exists(pc) and os.path.exists(pn)):continue
    c=open(pc,'rb').read();d=open(pn,'rb').read()
    if len(c)!=len(d):continue
    try:S=P.secs(d)
    except Exception:continue
    ok=0;bad=[];rt=0
    for nm,vs,va0,rs,ra,ch in S:
        if nm=='.text' or ch&0x20000000:continue
        for W,fmt in ((8,'<QQ'),(4,'<II')):
            for k in range(ra,ra+rs-2*W+1,W):
                s,t=struct.unpack_from(fmt,c,k)
                if not(0x1000<=s<0x2a02000 and 0x1000<=t<0x2a02000):continue
                ov=q(X.ord_,s)
                if ov!=G+t:continue
                ns,nt=struct.unpack_from(fmt,d,k);nv=q(X.nrd,ns)
                if nv==G+nt:ok+=1;continue
                if nv==0 or nv is None:rt+=1;continue
                bad.append((hex(va0+k-ra),W,hex(s),hex(t),'→',hex(ns),hex(nt),'새exe값',hex(nv-G) if nv>=G else hex(nv)))
    if bad or ok:print('==',rel,'OK',ok,'불일치',len(bad),'런타임',rt)
    for b in bad[:20]:print('   ',*b)
