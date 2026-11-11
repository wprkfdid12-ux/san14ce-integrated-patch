# '두 주소 중 고르기' 차이 상수 감사: and/imul/shl 로 만든 D 를 lea [R+X*1+A] 에 더하는 꼴 → 옛(A+D) 의 새 자리 − 새 A 가 D 와 같은지
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
IB=P.IB
for rel in open(sys.argv[3]).read().split():
    if 'runtime' in rel:continue
    pc=os.path.join(sys.argv[1],rel);pn=os.path.join(sys.argv[2],rel)
    if not(os.path.exists(pc) and os.path.exists(pn)):continue
    try:ic=P.disasm(pc);inn=P.disasm(pn)
    except Exception:continue
    if len(ic)!=len(inn):continue
    for i,(a,b,m,o) in enumerate(inn):
        mm=re.match(r'^r[a-z0-9]+,\[r[a-z0-9]+\+(r[a-z0-9]+)\*1\+0x([0-9a-f]+)\]$',o) if m=='lea' else None
        if not mm:continue
        reg=mm.group(1);A=int(mm.group(2),16)
        oo=re.match(r'^r[a-z0-9]+,\[r[a-z0-9]+\+r[a-z0-9]+\*1\+0x([0-9a-f]+)\]$',ic[i][3]);Ao=int(oo.group(1),16) if oo else None
        if not(0x1000<=A<0x2a02000) or Ao is None:continue
        for j in range(i-1,max(i-5,0),-1):
            a2,b2,m2,o2=inn[j]
            r32=reg.replace('r','e',1) if reg in('rax','rbx','rcx','rdx','rsi','rdi','rbp') else reg+'d'
            m3=re.match(r'^(%s|%s),0x([0-9a-f]+)$'%(reg,r32),o2) if m2 in('and','mov','add') else None
            if m3:
                D=int(m3.group(2),16)
                if D<0x100:break
                mo=X.map_code(Ao+D)[0] if Ao+D<P.OTEXT_END else X.map_data(Ao+D)[0]
                ok=(mo is not None and mo-A==D)
                print(rel,'%x'%(a-IB),'A %#x→%#x D %#x 옛 A+D %#x → 새 %s'%(Ao,A,D,Ao+D,hex(mo) if mo else None),'OK' if ok else '불일치 새 D=%s 자리 %x'%(hex(mo-A) if mo else '?',a2-IB))
                break
