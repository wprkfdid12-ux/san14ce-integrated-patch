# 음수로 들어간 주소 상수 감사: imm32 v 에서 -v(=2^32-v) 가 옛 exe 주소(±16) 이고 newmap 이 다른 값으로 옮기면 보고 (예: IC 0x3e00f add eax,-(0x1a3e290+7))
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
CUR,NEW=sys.argv[1],sys.argv[2];IB=P.IB
c=open(CUR,'rb').read();d=open(NEW,'rb').read();S=P.secs(d)
def foff(rva):
    for nm,vs,va0,rs,ra,ch in S:
        if va0<=rva<va0+rs:return ra+rva-va0
ins=P.disasm(NEW);n=0
for a,b,m,o in ins:
    if m not in('add','sub','lea','mov','cmp','imul'):continue
    for tok in re.findall(r'0x([0-9a-f]{8})\b',o.split('#')[0]) + re.findall(r'-0x([0-9a-f]{5,8})\b',o.split('#')[0]):
        v=int(tok,16)
        neg=(0x100000000-v) if v>=0x80000000 else v
        if '-0x'+tok in o:neg=v
        if not(0x10000<=neg<P.OIMG_END+0x100):continue
        rva=a-IB;fo=foff(rva);le=struct.pack('<I',(0x100000000-neg)&0xffffffff);p=b.rfind(le)
        if p<1 or c[fo+p:fo+p+4]!=le:continue
        for k in range(0,17):
            base=neg-k
            mp,how=X.map_code(base) if base<P.OTEXT_END else X.map_data(base)
            if mp and mp!=base and (k==0 or base in X.ostart or True):
                print('%s %x %s %s  → -(%#x+%d) 옛 %#x 새 %#x (%s)'%(os.path.basename(NEW),rva,m,o,base,k,base,mp,(how or '')[:30]));n+=1;break
print('==',os.path.basename(NEW),'후보',n)
