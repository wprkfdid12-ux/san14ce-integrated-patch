# 코드가 rip 상대로 가리키는 자료 조각(5~7바이트처럼 verify_expect 의 8바이트 앵커보다 짧은 것 포함)이 옛 exe .text 에는 있고 새 exe 에는 없으면 보고
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
OT=P.OT;NT=P.NT;ova=P.ova;nva=P.nva;IB=P.IB
MOD=sys.argv[1];d=open(MOD,'rb').read();S=P.secs(d)
def off(va):
    for nm,vs,va0,rs,ra,ch in S:
        if nm!='.text' and va0<=va-IB<va0+rs:return ra+(va-IB-va0)
ins=P.disasm(MOD);seen=set()
for a,b,m,o in ins:
    mm=re.search(r'# 0x([0-9a-f]+)',o)
    if not mm or m not in('lea','mov','movdqa','movups','movdqu','movzx','cmp'):continue
    t=int(mm.group(1),16)
    if t in seen:continue
    seen.add(t);fo=off(t)
    if fo is None:continue
    blob=d[fo:fo+32]
    # 옛 exe 에 있는 가장 긴 접두
    L=0
    for n in range(5,33):
        if OT.find(blob[:n])>=0:L=n
        else:break
    if L<5:continue
    if NT.find(blob[:L])>=0:continue
    # 옛 자리
    oi=OT.find(blob[:L]);cnt=OT.count(blob[:L])
    print('%x: 자료 %x 길이 %d 옛exe %x(%d곳) 새exe 없음  %s'%(a-IB,t-IB,L,ova+oi,cnt,blob[:L].hex()))
