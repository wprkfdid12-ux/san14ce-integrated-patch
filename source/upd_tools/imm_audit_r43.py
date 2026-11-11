# payload_new 코드에서 아직 옛 exe 범위 값 그대로인 즉시값/변위(명령 피연산자 텍스트 기준) 감사 — 보고만
import sys,os,re,struct,collections
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
CUR,NEW=sys.argv[1],sys.argv[2];IB=P.IB;MIN=int(sys.argv[3],16) if len(sys.argv)>3 else 0x10000
c=open(CUR,'rb').read();d=open(NEW,'rb').read();S=P.secs(d)
def foff(rva):
    for nm,vs,va0,rs,ra,ch in S:
        if va0<=rva<va0+rs:return ra+rva-va0
ins=P.disasm(NEW);rep=collections.defaultdict(list)
for a,b,m,o in ins:
    if len(b)<5 or m in('call','jmp') or m.startswith('j'):continue
    rva=a-IB;fo=foff(rva)
    for tok in re.findall(r'(?<![#\w])0x([0-9a-f]{5,8})\b',o.split('#')[0]):
        v=int(tok,16)
        if not(MIN<=v<P.OIMG_END):continue
        le=struct.pack('<I',v);p=b.rfind(le)
        if p<1:continue
        if c[fo+p:fo+p+4]!=le:continue
        mp,how=X.map_code(v) if v<P.OTEXT_END else X.map_data(v)
        if mp is None:mp,how=None,how or '번역 못함'
        if mp==v:continue
        rep[(v,mp,how)].append('%x %s %s'%(rva,m,o))
print('==',os.path.relpath(NEW),len(rep))
for (v,mp,how),L in sorted(rep.items(),key=lambda kv:-len(kv[1])):
    print('  %3d x %#x %s %s 예: %s'%(len(L),v,hex(mp) if mp else None,(how or '')[:40],L[0]))
    for l in L[1:6]:print('        ',l)
