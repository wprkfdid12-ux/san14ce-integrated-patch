# 우리 소스 숫자 대조(시험10): 옛 소스(src42d/r41/src)와 새 소스를 숫자 단위로 맞춰
#  (가) 안 바뀐 숫자 중 옛 exe 주소 증거(함수 시작·분기 대상·vtable·rip 참조·썽크)가 있는데 새 판에서 달라야 하는 것
#  (나) 바뀐 숫자 중 새 값이 map(옛 값)/RTTI 대응과 다른 것  을 보고.
import sys,os,re,difflib,pickle,bisect
_a=sys.argv[1:];sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));sys.argv=['x','a','b','c']
import newmap as X, rtti_r43 as R
S='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver'
oi=pickle.load(open('/tmp/census/old_idx.pkl','rb'));TH=pickle.load(open('/tmp/census/old_thunks.pkl','rb'))
FN=set(X.ostart);BR=oi['BR'];DREF=oi['DREF']
ov=R.vtables(S+'/old.dec.exe');nv=R.vtables(S+'/new.dec.exe');ok=sorted(ov);nk=sorted(nv)
def encl(tab,keys,v):
    i=bisect.bisect_right(keys,v)-1
    if i<0:return None
    b=keys[i];return (tab[b][0],v-b) if v-b<=0x800 else None
def ev(c):
    if c in FN:return 'FN'
    if c in TH:return 'THUNK'
    if c in BR:return 'BR'
    if encl(ov,ok,c):return 'VT'
    if c in DREF:return 'DREF'
    return None
def toks(path):
    t=open(path,encoding='utf-8',errors='replace').read()
    t=re.sub(r'//[^\n]*','',t);t=re.sub(r'/\*.*?\*/','',t,flags=re.S)
    return [int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{5,8})\b',t)]

OLD,NEW=_a[0],_a[1]
IMG=0x29ff000
for f in sorted(os.listdir(NEW)):
    if not f.endswith(('.cpp','.h','.S','.c','.inc')):continue
    po=os.path.join(OLD,f)
    if not os.path.exists(po):continue
    a=toks(po);b=toks(os.path.join(NEW,f))
    sm=difflib.SequenceMatcher(None,a,b,autojunk=False)
    for tag,i1,i2,j1,j2 in sm.get_opcodes():
        if tag=='equal':
            for v in a[i1:i2]:
                if not(0x10000<=v<IMG):continue
                e=ev(v)
                if not e:continue
                n,how=(X.map_code(v) if v<0x1341000 else X.map_data(v))
                if e=='VT':
                    eo=encl(ov,ok,v);en=encl(nv,nk,v)
                    if eo==en:continue
                if n is not None and n==v:continue
                print('그대로?',f,hex(v),e,'→',hex(n) if n else n,(how or '')[:24])
        elif tag=='replace' and i2-i1==j2-j1:
            for v,w in zip(a[i1:i2],b[j1:j2]):
                if not(0x10000<=v<IMG):continue
                eo=encl(ov,ok,v)
                if eo:
                    en=encl(nv,nk,w)
                    if en!=eo:print('vtable?',f,hex(v),'→',hex(w),eo,'새',en)
                    continue
                n,how=(X.map_code(v) if v<0x1341000 else X.map_data(v))
                if n is not None and n!=w:print('대응?',f,hex(v),'→',hex(w),'map',hex(n),(how or '')[:24])
