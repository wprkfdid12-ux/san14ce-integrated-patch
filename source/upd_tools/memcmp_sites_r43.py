# IC(및 다른 모듈)의 memcmp(call 0x180094d70) 호출 자리: rcx=게임기준+V, rdx=rip 자료, r8=길이 → 새 exe 바이트와 자료 비교
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
MOD=sys.argv[1];MEMCMP=int(sys.argv[2],16) if len(sys.argv)>2 else None
NT=P.NT;nva=P.nva;OT=P.OT;ova=P.ova
d=open(MOD,'rb').read();S=P.secs(d);IB=P.IB
def rd(va,n):
    for nm,vs,va0,rs,ra,ch in S:
        if va0<=va-IB<va0+rs:
            off=ra+(va-IB-va0);return d[off:off+n]
    return b''
ins=P.disasm(MOD)
calls=[i for i,(a,b,m,o) in enumerate(ins) if m=='call' and o==('0x%x'%MEMCMP)]
print('memcmp 호출',len(calls))
for i in calls:
    rcx=rdx=r8=None;rcxs=rdxs=''
    for j in range(i-1,max(i-25,0),-1):
        a,b,m,o=ins[j]
        if m in('call','ret','jmp'):break
        if rdx is None and re.match(r'^rdx,\[rip\+0x[0-9a-f]+\]',o) and m=='lea':
            rdx=int(re.search(r'# 0x([0-9a-f]+)',o).group(1),16)
        if rcx is None and m=='lea':
            mm=re.match(r'^rcx,\[(r[a-z0-9]+)\+0x([0-9a-f]+)\]$',o)
            if mm:rcx=int(mm.group(2),16);rcxs=mm.group(1)
        if r8 is None and m=='mov' and re.match(r'^r8d?,0x[0-9a-f]+$',o):r8=int(o.split(',')[1],16)
    a=ins[i][0]
    if rcx is None or rdx is None or r8 is None or r8>0x200:
        print('  %x: 인자 못 찾음 rcx=%s rdx=%s r8=%s'%(a-IB,rcx,rdx,r8));continue
    blob=rd(rdx,r8)
    if rcx>=nva and rcx-nva+r8<=len(NT):
        nb=NT[rcx-nva:rcx-nva+r8]
        ok=(nb==blob)
        # 옛 exe 에서 blob 위치
        oi=OT.find(blob)
        print('  %x: [%s+%x] len %d %s'%(a-IB,rcxs,rcx,r8,'OK' if ok else '불일치'),('옛exe에서 %x'%(ova+oi) if oi>=0 else '옛exe없음'), ('새exe %x'%(nva+NT.find(blob)) if NT.find(blob)>=0 else '새exe없음') if not ok else '')
    else:
        print('  %x: [%s+%x] len %d 범위 밖'%(a-IB,rcxs,rcx,r8))
