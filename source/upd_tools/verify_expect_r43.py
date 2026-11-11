# R43 시험3: 바이너리 모듈의 '기대 바이트' 오프라인 검증(시작 검사 흉내).
#  ① payload_cur 자료 구역에서 옛 exe 와 같은 바이트 조각(≥8바이트, .text 안 유일하지 않아도 됨)을 찾고 → payload_new 같은 자리의 조각이 새 exe 에 있는지
#  ② 조각 ±64바이트 안의 u32 칸이 옛 exe 의 그 조각 자리(RVA)였으면 → payload_new 의 그 칸 값이 새 exe 에서 새 조각 자리와 같은지
#  ③ 코드: cmp [base+V],imm (base_flow 와 같은 추적 없이) — payload_new 의 cmp BYTE/WORD/DWORD [R+V],imm 중 V 가 새 exe .text 범위면 새 exe 바이트와 imm 비교(보고만)
#  사용: python3 verify_expect_r43.py <payload_cur> <payload_new> <목록>
import sys,os,re,struct,collections
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
IB=P.IB;OT=P.OT;NT=P.NT;ova=P.ova;nva=P.nva
MINLEN=8
import numpy as np
class Index:
    def __init__(self,buf):
        n=len(buf)-7;self.buf=buf
        w=np.ndarray(shape=(n,),dtype='<u8',buffer=buf,strides=(1,)).copy()
        self.order=np.argsort(w,kind='stable');self.sorted=w[self.order]
    def find(self,needle8):
        k=np.frombuffer(needle8,dtype='<u8')[0];i=np.searchsorted(self.sorted,k)
        if i<len(self.sorted) and self.sorted[i]==k:return int(self.order[i])
        return -1
OTI=Index(OT);NTI=Index(NT)
def find_all(hay,needle):
    i=hay.find(needle);out=[]
    while i>=0 and len(out)<8:out.append(i);i=hay.find(needle,i+1)
    return out
def run(CUR,NEW):
    c=open(CUR,'rb').read();d=open(NEW,'rb').read()
    if len(c)!=len(d):print('==',os.path.basename(NEW),'크기 다름');return
    S=P.secs(c);name=os.path.basename(NEW)
    ok=0;bad=[];rvabad=[];rvaok=0
    for n,vs,va,rs,ra,ch in S:
        if n=='.text' or ch&0x20000000:continue
        i=0
        while i<rs-MINLEN:
            # 옛 exe .text 에서 cur 조각 찾기: 8바이트 앵커
            anchor=c[ra+i:ra+i+MINLEN]
            if anchor.count(0)>=5 or anchor==b'\xcc'*8:i+=1;continue
            pos=OTI.find(anchor)
            if pos<0:i+=1;continue
            # 늘리기
            L=MINLEN
            while i+L<rs and pos+L<len(OT) and c[ra+i+L]==OT[pos+L]:L+=1
            if L<MINLEN:i+=1;continue
            orva=ova+pos
            # 유일성(옛 exe 안): 두 번째 위치가 있으면 그 중 아무거나였을 수 있음 — 기록
            nb=d[ra+i:ra+i+L]
            if nb==c[ra+i:ra+i+L]:
                # 안 바뀜: 새 exe 에 같은 바이트가 (같은 자리에서 옮긴 자리에) 있나
                m,how=X.map_code(orva)
                if m is not None and NT[m-nva:m-nva+L]==nb:ok+=1;found=m
                elif NTI.find(nb[:8])>=0 and NT[NTI.find(nb[:8]):NTI.find(nb[:8])+L]==nb:ok+=1;found=nva+NTI.find(nb[:8])
                else:bad.append((hex(va+i),L,hex(orva),'안 바뀌었고 새 exe 에 없음',(how or '')[:40]));found=None
            else:
                p2=NTI.find(nb[:8])
                if p2>=0 and NT[p2:p2+L]!=nb:p2=NT.find(nb)
                if p2>=0:ok+=1;found=nva+p2
                else:bad.append((hex(va+i),L,hex(orva),'바뀐 조각이 새 exe 에 없음'));found=None
            # 주변 u32 칸 = 옛 조각 RVA?
            if found is not None:
                for off in range(-64,65,1):
                    k=ra+i+off
                    if k<ra or k+4>ra+rs or (i<=off<i+L and False):continue
                    if off>=0 and off<L:continue
                    ov=struct.unpack_from('<I',c,k)[0]
                    if ov==orva or (orva<ov<orva+L and L>=16):
                        nv=struct.unpack_from('<I',d,k)[0];exp=found+(ov-orva)
                        if nv==exp:rvaok+=1
                        else:rvabad.append((hex(va+i+off),hex(ov),'→',hex(nv),'기대',hex(exp),'조각',hex(va+i),L))
            i+=L
    # ③ 코드 cmp 검사(새 모듈 기준)
    ins=P.disasm(NEW);cmpbad=[];cmpok=0
    for addr,bs,mn,ops in ins:
        if mn!='cmp' or 'rip' in ops:continue
        m=re.match(r'^(BYTE|WORD|DWORD) PTR \[(r[a-z0-9]+)(?:\+r[a-z0-9]+\*\d)?\+0x([0-9a-f]+)\],0x([0-9a-f]+)$',ops)
        if not m or m.group(2)=='rsp':continue
        v=int(m.group(3),16);imm=int(m.group(4),16);w={'BYTE':1,'WORD':2,'DWORD':4}[m.group(1)]
        if not(0x10000<=v<nva+len(NT)) or v<nva:continue
        nb=NT[v-nva:v-nva+w]
        if len(nb)<w:continue
        if int.from_bytes(nb,'little')==imm:cmpok+=1
        else:
            # 옛 모듈의 같은 명령이 옛 exe 와 맞았는지(맞았으면 진짜 불일치)
            cmpbad.append((hex(addr-IB),ops,'새 exe',nb.hex()))
    print('==',name,'조각 OK',ok,'불일치',len(bad),'| RVA칸 OK',rvaok,'불일치',len(rvabad),'| cmp OK',cmpok,'불일치',len(cmpbad))
    for b in bad[:30]:print('  조각:',*b)
    for b in rvabad[:30]:print('  RVA칸:',*b)
    for b in cmpbad[:30]:print('  cmp:',*b)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:run(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:
            import traceback;traceback.print_exc()
