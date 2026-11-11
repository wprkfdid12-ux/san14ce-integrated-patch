# 회귀 전수 조사(R43 시험10): 이식 모듈의 '바뀌지 않은' 모든 즉시값·변위(코드)와 정렬된 4/8바이트 값(자료) 가운데
#  옛 exe 주소의 여러 꼴(그대로, +1 경계, 0x80000000+, 음수, 음수-k, 0x140000000+)과 맞는 것을 빠짐없이 뽑는다.
#  사용: python3 census_r43.py <옛 payload> <새 payload> <목록> <출력.tsv>
import sys,os,re,subprocess,pickle,struct,collections
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
A,B,LST,OUT=sys.argv[1:5];sys.argv=['x','a','b','c']
import newmap as X, rtti_r43 as R
S='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver'
oi=pickle.load(open('/tmp/census/old_idx.pkl','rb'));ni=pickle.load(open('/tmp/census/new_idx.pkl','rb'))
FN=set(X.ostart);BR=oi['BR'];DREF=oi['DREF'];INS=oi['INS']
VT=set()
for v in R.vtables(S+'/old.dec.exe'):
    for k in range(0,0x400,8):VT.add(v+k)
TEXT_END=0x1341000;IMG=0x29ff000
def ev(c):
    if c in FN:return 'FN'
    if c in BR:return 'BR'
    if c in VT:return 'VT'
    if c in DREF:return 'DREF'
    return None
def forms(x,neg):
    out=[]
    u=x&0xffffffff
    if neg:u=(-x)&0xffffffff
    out.append(('raw',u))
    out.append(('+1',u+1))            # 경계 = 주소-1
    if 0x80000000<=u<0x80000000+IMG:out.append(('0x8',u-0x80000000))
    if 0x140000000<=x<0x140000000+IMG:out.append(('abs',x-0x140000000))
    n=(-u)&0xffffffff
    if n<IMG:
        for k in range(0,16):out.append(('neg-%d'%k,n-k))
    return out
def mapped(c):
    if c<TEXT_END:return X.map_code(c)
    return X.map_data(c)
def secs_of(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    return [(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode('latin1'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
def dis(p):
    out=subprocess.run(['objdump','-d','-M','intel',p],capture_output=True,text=True).stdout;L=[]   # 실행 구역 전부(.text·.p1fix·.compat·.s14sw 등)
    for ln in out.split('\n'):
        m=re.match(r'\s+([0-9a-f]+):\t([0-9a-f ]+)\t(.*)',ln)
        if m:L.append((int(m.group(1),16),m.group(2).strip(),m.group(3).strip()))
    return L
rows=[]
for rel in open(LST).read().split():
    pa,pb=os.path.join(A,rel),os.path.join(B,rel)
    if not(os.path.exists(pa) and os.path.exists(pb)):continue
    da,db=open(pa,'rb').read(),open(pb,'rb').read()
    if len(da)!=len(db):print('길이 다름',rel);continue
    # ---- 코드
    la,lb=dis(pa),dis(pb)
    if len(la)==len(lb):
        changed=[ia!=ib for (_,_,ia),(_,_,ib) in zip(la,lb)]
        for i,((ad,_,ia),(bd,_,ib)) in enumerate(zip(la,lb)):
            if changed[i]:continue
            ops=ib.split(None,1)[1] if ' ' in ib else ''
            mn=ib.split()[0] if ib else ''
            if mn.startswith('j') or mn=='call':continue
            ops2=re.sub(r'#.*','',ops)
            for m in re.finditer(r'(rip)?([+-]?)(0x[0-9a-f]+)',ops2):
                if m.group(1):continue
                pre=ops2[max(0,m.start()-4):m.start()]
                if 'rip' in pre:continue
                x=int(m.group(3),16);neg=m.group(2)=='-'
                if x<0x10000 and not neg:continue
                for f,c in forms(x,neg):
                    if not(0x10000<=c<IMG):continue
                    e=ev(c)
                    if not e:continue
                    n,how=mapped(c)
                    if n is None or n==c:continue
                    near=sum(1 for j in range(max(0,i-30),min(len(la),i+30)) if changed[j])
                    rows.append((rel,'code',hex(bd),ib,hex(x),f,hex(c),e,hex(n),(how or '')[:30],oi['DISP'].get(x,0),ni['DISP'].get(x,0),oi['IMM'].get(x,0),ni['IMM'].get(x,0),near))
    else:print('명령 수 다름',rel)
    # ---- 자료(.rdata/.data, 정렬 4/8)
    for nm,vs,va,rs,ra in secs_of(db):
        if nm in('.text','.pdata','.reloc','.rsrc','.xdata','.buildid','.p1fix','.compat','.s14sw','.hooks') or nm.startswith('/'):continue   # 자료 구역 전부(.rodata·.s14sync·.fptable 포함)
        for off in range(ra,ra+rs-3,4):
            if da[off:off+4]!=db[off:off+4]:continue
            u=struct.unpack_from('<I',db,off)[0]
            cands=[('raw',u)]
            if off%8==0 and off+8<=len(db):
                q=struct.unpack_from('<Q',db,off)[0]
                if 0x140000000<=q<0x140000000+IMG and da[off:off+8]==db[off:off+8]:cands.append(('abs',q-0x140000000))
            for f,c in cands:
                if not(0x10000<=c<IMG):continue
                e=ev(c)
                if not e or e=='INS':continue
                n,how=mapped(c)
                if n is None or n==c:continue
                # 이웃 16칸 안 번역된 칸 수
                near=sum(1 for k in range(-64,64,4) if 0<=off+k<len(db)-4 and da[off+k:off+k+4]!=db[off+k:off+k+4])
                rows.append((rel,nm,hex(va+off-ra),'',hex(u),f,hex(c),e,hex(n),(how or '')[:30],oi['DISP'].get(u,0),ni['DISP'].get(u,0),oi['IMM'].get(u,0),ni['IMM'].get(u,0),near))
with open(OUT,'w',encoding='utf-8') as f:
    f.write('모듈\t구역\t자리\t명령\t값\t꼴\t옛주소\t증거\t새주소\t근거\t옛disp\t새disp\t옛imm\t새imm\t주변번역\n')
    for r in rows:f.write('\t'.join(str(x) for x in r)+'\n')
print('후보',len(rows))
c=collections.Counter((r[0],r[1]) for r in rows)
for k,v in sorted(c.items()):print(' ',k,v)
