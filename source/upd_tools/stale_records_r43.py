# {u32 rva,u32 len,bytes} 기록 중 rva 가 옛값 그대로(번역 안 됨)인데 새 exe 그 자리 바이트가 다르면 보고 + map_code 후보(그 후보 자리 바이트 일치 여부)
import sys,os,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
for rel in open(sys.argv[3]).read().split():
    if 'runtime' in rel:continue
    pc=os.path.join(sys.argv[1],rel);pn=os.path.join(sys.argv[2],rel)
    if not(os.path.exists(pc) and os.path.exists(pn)):continue
    c=open(pc,'rb').read();d=open(pn,'rb').read()
    if len(c)!=len(d):continue
    try:S=P.secs(d)
    except Exception:continue
    for nm,vs,va,rs,ra,ch in S:
        if nm=='.text':continue
        for k in range(ra,ra+rs-12,4):
            rva,ln=struct.unpack_from('<II',d,k)
            if not(0x1000<=rva<P.OIMG_END and 4<=ln<=64) or k+8+ln>ra+rs:continue
            if struct.unpack_from('<I',c,k)[0]!=rva:continue
            b=d[k+8:k+8+ln]
            if b.count(0)>ln-3:continue
            if X.ord_(rva,ln)!=b:continue          # 옛 exe 의 그 자리 바이트였던 기록만
            if X.nrd(rva,ln)==b:continue           # 새 exe 에서도 그대로면 OK
            m,how=X.map_code(rva) if rva<P.OTEXT_END else X.map_data(rva)
            ok=m is not None and X.nrd(m,ln)==b
            print(rel,'rva칸 %x: %#x len %d 옛값 그대로·새 exe 다름 → 후보 %s %s'%(va+k-ra,rva,ln,hex(m) if m else None,'바이트 일치' if ok else '불일치'),(how or '')[:30])
