# 모든 모듈 자료 구역의 (u32 slot, u32 target) 이웃 쌍 검사: 새 exe [slot](qword) 이 게임 .text 를 가리키는데 target 과 다르면 보고
#  (TraitHover {kind,slot,target} 0x92ad0 처럼 target 만 옛값으로 남은 경우)
import sys,os,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
G=0x140000000;TEND=0x1342120
for rel in open(sys.argv[2]).read().split():
    if 'runtime' in rel:continue
    p=os.path.join(sys.argv[1],rel)
    if not os.path.exists(p):continue
    d=open(p,'rb').read()
    try:S=P.secs(d)
    except Exception:continue
    ok=0;bad=[]
    for nm,vs,va0,rs,ra,ch in S:
        if nm=='.text' or ch&0x20000000:continue
        for k in range(ra,ra+rs-8,4):
            s,t=struct.unpack_from('<II',d,k)
            if not(0x1343000<=s<0x2a02000 and 0x1000<=t<TEND):continue
            q=X.nrd(s,8)
            if len(q)<8:continue
            v=struct.unpack('<Q',q)[0]-G
            if not(0x1000<=v<TEND):continue
            if v==t:ok+=1
            else:bad.append((hex(va0+k-ra),hex(s),hex(t),hex(v)))
    if bad or ok:print('==',rel,'OK',ok,'불일치',len(bad))
    for b in bad[:12]:print('   ',*b)
