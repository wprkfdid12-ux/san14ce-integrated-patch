# 새 판 주소 대응 공용 모듈 (R43). gen_merit_resolver_r43.py 의 규칙을 다른 이식 도구(field_port_r43.py 등)에서도 쓰게 뺀 것.
#  map_code(옛 RVA) / map_data(옛 RVA) → (새 RVA 또는 None, 근거 글)
#  규칙: 함수 시작 = 함수대응(m) → 자리_대응(정확/정렬) → 이웃 이동량+머리 바이트(마스크) 일치 ; 함수 안 = 새 시작 + 오프셋(바뀐 함수면 그 자리 바이트 확인)
#        자료 = 손 지정 → 자리_대응 → 이웃 자료 이동량 + 내용 일치 → vtable 항목 함수대응 일치
import struct,pickle,bisect,collections,os
NEWVER=os.environ.get('NEWVER','/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver')
MAP=os.environ.get('NEWMAP','/home/claude/r41/upd_tools/자리_대응_검증.tsv')
IB=0x140000000
def _load(p):
    d=open(p,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    secs=[struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)];names=[d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0') for i in range(ns)]
    pd=[s for s,n in zip(secs,names) if n==b'.pdata'][0];funcs=[]
    for k in range(0,pd[0],12):
        b,e,u=struct.unpack_from('<III',d,pd[3]+k)
        if b==0:break
        funcs.append((b,e))
    funcs.sort()
    def rd(rva,n):
        for vs,va,rs,ra in secs:
            if va<=rva<va+max(vs,rs):
                if rva-va+n<=rs:return d[ra+rva-va:ra+rva-va+n]
                return b''
        return b''
    return rd,funcs,d,secs
ord_,ofn,OLD_D,OLD_SECS=_load(NEWVER+'/old.dec.exe');nrd,nfn,NEW_D,NEW_SECS=_load(NEWVER+'/new.dec.exe')
ostart=[f[0] for f in ofn];oend={f[0]:f[1] for f in ofn};nstart=set(f[0] for f in nfn);nend={f[0]:f[1] for f in nfn}
_fm=pickle.load(open(NEWVER+'/대응/함수대응.pkl','rb'))['m'];_ch=pickle.load(open(NEWVER+'/대응/바뀐함수.pkl','rb'));SAME=set(_ch['same']);DIFF=set(_ch['diff'])
_rows=[l.split('\t') for l in open(MAP,encoding='utf-8').read().splitlines()[1:]]
def _good(v):return (v.startswith('정확') and not v.startswith('정확+')) or v.startswith('정렬')
SITE={};_dpts=[]
for r in _rows:
    if len(r)<13:continue
    try:o=int(r[3],16)
    except:continue
    if r[11].startswith('0x') and _good(r[12]):
        n=int(r[11],16);SITE.setdefault(o,n)
        if r[2]=='자료':_dpts.append((o,n-o))
_dpts.sort();_dx=[p[0] for p in _dpts]
MANUAL={0x44a3e0:0x60a280,0x42b570:0x4f90c0,0x92ad0:0x8ca1c0,0x773bd0:0x89ab90,0x6bea30:0x6c1130,0x8a5bc0:0x8a7da0,0x3363d0:0x338630,0x84ec80:0x5c1550,0x20ffa0:0x32dc20,0x20e990:0x35d1a0,0x2b0290:0x2b2550,0x7e9890:0x7ebe90,0x839ca0:0x71fa80,0x7208c0:0x722c40,0x403810:0x405b40,0xb1aa0:0xb2af0,0x401c0:0x58d90,0x415aa2:0x415bc2,0x91c140:0x91e670,0x7feb90:0x7ab6a0,0x924fd0:0x87abc0,0x450f80:0x876580}
DATA_MANUAL={0x14769e8:0x1479a78,0x16c9f80:0x16ccfc8,0x14784a0:0x147b530,0x1447ce0:0x144ad68,0x1447e80:0x144af08,0x1448240:0x144b2c8,0x13a4220:0x13a7260,0x1426f90:0x142a020,0x1411678:0x1414708,0x2783c60:0x2786cb0,0x1a70000:0x1a73000}
def _rip_form(b,i):
    """b[i:] 가 rel32/rip 변위를 가진 명령이면 (변위 시작, 명령 길이), 아니면 None. 접두사 66/f2/f3 + REX 까지 봄.
    (R43 보강: 83 3d / c7 05 / ff 05 / 0f xx 05 / ALU op modrm 05 — 0x115a0 머리의 'cmp [rip+x],imm8' 때문에)"""
    j=i;n=len(b)
    while j<n and b[j] in(0x66,0xf2,0xf3):j+=1
    if j<n and 0x40<=b[j]<=0x4f:j+=1
    if j>=n:return None
    op=b[j]
    if op in(0xe8,0xe9):return j+1,j+5
    if op==0x0f:
        if j+1>=n:return None
        op2=b[j+1]
        if 0x80<=op2<=0x8f:return j+2,j+6
        if op2 in(0x38,0x3a):return None
        if j+2<n and (b[j+2]&0xc7)==0x05:return j+3,j+7
        return None
    if j+1>=n or (b[j+1]&0xc7)!=0x05:return None
    if op==0xff and (b[j+1]&0x38) in(0x00,0x08,0x10,0x20,0x30):return j+2,j+6
    if op in(0x83,0x80,0xc6,0xf6) or (op==0xf7 and (b[j+1]&0x38)!=0):return j+2,(j+6+1) if op!=0xf7 else j+6
    if op in(0xc7,0x81) or (op==0xf7 and (b[j+1]&0x38)==0):return j+2,j+10
    if op<0x40 and (op&7)<4:return j+2,j+6
    if op in(0x63,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8d,0x69,0x6b):return j+2,j+6+(4 if op==0x69 else 1 if op==0x6b else 0)
    return None
def mask(b):
    b=bytearray(b);i=0
    while i<len(b):
        f=_rip_form(b,i)
        if f:
            s,e=f
            if e>len(b):return bytes(b[:i])
            b[s:s+4]=b'\0\0\0\0';i=e
        else:i+=1
    return bytes(b)
# ---- 명령 경계를 아는 마스크(R43): 역어셈블 pkl(주소→(니모닉,피연산자,대상)) 로 rel32/rip 변위 4바이트만 지운다.
#  바이트만 보는 mask 는 '74 0f 8b 53 ..' 처럼 앞 명령 꼬리를 jcc 로 잘못 읽어 뒤의 [rip+x] 를 못 지우는 일이 있었다(0x406a90 범위).
_asm={}
def asm(side):
    if side not in _asm:_asm[side]=pickle.load(open(NEWVER+'/대응/%s.dec.exe.asm.pkl'%side,'rb'))
    return _asm[side]
def imask(side,a,n):
    """side('old'/'new') 의 a 부터 n 바이트를 명령 단위로 마스크. a 가 명령 경계가 아니면 byte mask 로. 끝에 걸친 명령은 잘라낸다."""
    A=asm(side);rd=ord_ if side=='old' else nrd
    if a not in A:return mask(rd(a,n))
    out=bytearray();p=a;keys=None
    while len(out)<n:
        ins=A.get(p)
        if ins is None:break
        # 다음 명령 시작 = 길이
        if keys is None:keys=_akeys(side)
        i=bisect.bisect_right(keys,p);L=(keys[i]-p) if i<len(keys) else 15
        if L<=0 or L>15:break
        b=bytearray(rd(p,L));mn,ops,tgt=ins
        if len(out)+L>n:break
        t=None
        if tgt.startswith('0x') and 'rip' in ops:t=int(tgt,16)-IB
        elif mn.startswith(('call','jmp','j','loop')) and ops.startswith('0x'):t=int(ops,16)-IB
        elif tgt.startswith('0x') and mn.startswith(('call','jmp','j','loop')):t=int(tgt,16)-IB
        if t is not None:
            try:
                if 'rip' in ops:
                    d=(t-(p+L))&0xffffffff;enc=struct.pack('<I',d)
                    k=bytes(b).find(enc)
                    if k<0:  # 변위 뒤 즉시값이 있는 경우(imm8/imm32) 다음 ip 가 아닌 변위 자리 기준이 아님 → 전체에서 못 찾으면 그대로
                        pass
                    else:b[k:k+4]=b'\0\0\0\0'
                else:
                    d=(t-(p+L))&0xffffffff;enc=struct.pack('<I',d)
                    if L>=5 and bytes(b[-4:])==enc:b[-4:]=b'\0\0\0\0'
                    elif L>=5:
                        k=bytes(b).rfind(enc)
                        if k>=0:b[k:k+4]=b'\0\0\0\0'
            except ValueError:pass
        # R43: 피연산자 글에 '게임 주소처럼 보이는 상수'(RVA 0x1000~0x2a00000, 절대 0x140000000+)가 있으면 그 4/8바이트 인코딩도 지운다
        #  (점프표 [rsi+rax*4+0x37a278], lea rcx,[rdi+0xffa2c8], add rax,0x19b020, mov rax,0x14xxxxxxxx 등 — 같은 함수인데 주소만 밀린 경우)
        for hx in _HEXRE.findall(ops):
            v=int(hx,16)
            if IB+0x1000<=v<IB+0x2a00000:
                enc=struct.pack('<Q',v);k=bytes(b).find(enc)
                if k>=0:b[k:k+8]=b'\0'*8
                v-=IB
            if 0x1000<=v<0x2a00000:
                enc=struct.pack('<I',v);k=bytes(b).rfind(enc)
                if k>=0:b[k:k+4]=b'\0\0\0\0'
        out+=b;p+=L
    return bytes(out)
import re as _re_
_HEXRE=_re_.compile(r'0x([0-9a-f]{4,10})\b')
def _align(o,n,ln):
    """o/n 이 명령 중간이면 (앞 명령 시작 옛/새, 경계까지 거리) 를 돌려줌. 경계면 None."""
    A=asm('old');B=asm('new');ko=_akeys('old');kn=_akeys('new')
    i=bisect.bisect_right(ko,o)-1;j=bisect.bisect_right(kn,n)-1
    if i<0 or j<0:return None
    po,pn=ko[i],kn[j]
    if po==o and pn==n:return None
    if o-po!=n-pn or o-po>15:return 'bad'
    return (po,pn,o-po)
def same_code(o,n,ln):
    """옛 o / 새 n 의 ln 바이트가 같은 명령열인지. 명령 중간에서 시작하면 그 명령부터 맞춰 본다(덩어리 경계)."""
    al=_align(o,n,ln)
    if al is None:
        if o in asm('old') and n in asm('new'):return imask('old',o,ln)==imask('new',n,ln)
        return mask(ord_(o,ln))==mask(nrd(n,ln))
    if al=='bad':return mask(ord_(o,ln))==mask(nrd(n,ln))
    po,pn,dl=al
    return imask('old',po,ln+dl)==imask('new',pn,ln+dl)
_ak={}
def _akeys(side):
    if side not in _ak:_ak[side]=sorted(asm(side))
    return _ak[side]
def head_len(of,n=24):
    """옛 함수 of 의 머리 비교 길이: 첫 ret(명령 단위) 까지, 없으면 n"""
    A=asm('old');ks=_akeys('old');i=bisect.bisect_left(ks,of);acc=0
    while i<len(ks) and ks[i]-of<n:
        j=i+1;L=(ks[j]-ks[i]) if j<len(ks) else 1
        if A[ks[i]][0]=='ret':return ks[i]-of+L
        i+=1
    return n
def func_of(r):
    i=bisect.bisect_right(ostart,r)-1
    return ofn[i] if i>=0 and r<ofn[i][1] else None
def head_search(o,guess,span=0x4000):
    for sh in range(0,span,16):
        for c in(guess+sh,guess-sh):
            if c in nstart and same_code(o,c,24):return c
    return None
def map_code(o):
    if o in MANUAL:return MANUAL[o],'손으로 확인'
    f=func_of(o)
    if not f:
        if o in SITE:return SITE[o],'자리_대응(pdata 없음)'
        i=bisect.bisect_left(ostart,o);c=collections.Counter(_fm[ostart[j]]-ostart[j] for j in range(max(0,i-4),min(len(ostart),i+5)) if ostart[j] in _fm)
        for sh,_ in c.most_common():
            if same_code(o,o+sh,24):return o+sh,f'pdata 없음·이웃 이동량 +0x{sh:x}+바이트 일치'
        if o%16==0 and ord_(o-1,1) in(b'\xcc',b'\xc3'):
            # 잎 함수 길이: 첫 ret 뒤 cc 채움까지(그 뒤는 다음 함수라 비교에서 뺌)
            ln=head_len(o,48);oh=imask('old',o,ln)
            # (가) 다음 pdata 함수의 대응에서 거꾸로: 새 다음 함수 - 같은 간격
            j=bisect.bisect_right(ostart,o)
            if j<len(ostart) and ostart[j] in _fm and ostart[j]-o<=0x200:
                k=_fm[ostart[j]]-(ostart[j]-o)
                if imask('new',k,ln)==oh and nrd(k-1,1) in(b'\xcc',b'\xc3'):return k,f'pdata 없음·다음 함수 0x{ostart[j]:x} 대응에서 같은 간격'
            for sh,_ in c.most_common():
                cands=[k for k in range(o+sh-0x800,o+sh+0x800,16) if nrd(k-1,1) in(b'\xcc',b'\xc3') and imask('new',k,ln)==oh]
                if len(cands)==1:return cands[0],f'pdata 없음·이웃 이동량 +0x{sh:x} 근처 16정렬 탐색 0x{cands[0]:x}'
        return None,'함수 밖(pdata 없음)'
    st,off=f[0],o-f[0];ns=_fm.get(st) or SITE.get(st);how='함수대응' if st in _fm else ('자리_대응' if st in SITE else None)
    if ns is None:
        i=bisect.bisect_left(ostart,st);c=collections.Counter(_fm[ostart[j]]-ostart[j] for j in range(max(0,i-4),min(len(ostart),i+5)) if ostart[j] in _fm)
        for sh,_ in c.most_common():
            if st+sh in nstart and same_code(st,st+sh,24):ns=st+sh;how=f'이웃 이동량 +0x{sh:x}+바이트 일치';break
        if ns is None:return None,'새 함수 못 찾음'
    if not same_code(st,ns,24):
        alt=head_search(st,ns)
        if alt:ns=alt;how+='→머리 바이트로 바로잡음'
        else:return None,f'{how} 0x{ns:x} 머리 바이트 다름'
    tag='바뀐 함수' if st in DIFF else '같은 함수' if st in SAME else '대응만'
    if off:
        if same_code(o,ns+off,12):return ns+off,f'{how}+오프셋 0x{off:x} ({tag})'
        # 같은 길이 함수의 '명령이 아닌 자리'(코드 뒤 점프표 등)는 같은 오프셋으로
        if (oend.get(st,0)-st)==(nend.get(ns,0)-ns):
            ov=[struct.unpack_from('<I',ord_(o,12),k)[0] for k in range(0,9,4)];nv=[struct.unpack_from('<I',nrd(ns+off,12),k)[0] for k in range(0,9,4)]
            if len(ord_(o,12))==12 and all(st<=x<oend[st] for x in ov) and all(ns<=y<nend[ns] for y in nv):return ns+off,f'{how}+오프셋 0x{off:x} (같은 길이 함수의 점프표 자리)'
        # 함수가 늘어났거나 줄었으면 같은 바이트 열을 함수 안에서 찾음
        key=imask('old',o,12);ne=nend.get(ns,ns+0x1000);ks=_akeys('new');i0=bisect.bisect_left(ks,ns);i1=bisect.bisect_left(ks,ne)
        hits=[ks[j]-ns for j in range(i0,i1) if imask('new',ks[j],12)==key]
        if len(hits)==1:return ns+hits[0],f'{how}+함수 안 탐색 오프셋 0x{off:x}→0x{hits[0]:x} ({tag})'
        return None,f'{how} 함수 시작 0x{ns:x} 는 찾았으나 오프셋 0x{off:x} 자리 바이트 다름(후보 {len(hits)}개)'
    return ns,f'{how} ({tag})'
def map_data(o):
    if o in DATA_MANUAL:return DATA_MANUAL[o],'손으로 확인'
    if o in SITE:return SITE[o],'자리_대응'
    i=bisect.bisect_left(_dx,o);c=collections.Counter(_dpts[j][1] for j in range(max(0,i-3),min(len(_dpts),i+3)))
    for sh,_ in c.most_common():
        if ord_(o,16)==nrd(o+sh,16) and len(ord_(o,16))==16:return o+sh,f'이웃 이동량 +0x{sh:x}+내용 일치'
    for sh,_ in c.most_common():
        ok=True
        for k in range(0,16,8):
            a=ord_(o+k,8);b=nrd(o+sh+k,8)
            if len(a)<8 or len(b)<8:ok=False;break
            ov=struct.unpack('<Q',a)[0]-IB;nv=struct.unpack('<Q',b)[0]-IB
            if not(0<ov<0x1341000 and (_fm.get(ov)==nv or MANUAL.get(ov)==nv)):ok=False;break
        if ok:return o+sh,f'이웃 이동량 +0x{sh:x}+vtable 항목 함수대응 일치'
    # 32바이트 창(앞 8 + 뒤 24)이 이웃 이동량 ±0x800 안에서 유일하게 맞으면
    def pmask(b):
        b=bytearray(b)
        for k in range(0,len(b)-7):
            q=struct.unpack_from('<Q',b,k)[0]
            if IB+0x1000<=q<IB+0x2a00000:b[k:k+8]=b'\0'*8
        return bytes(b)
    win=ord_(o-8,32)
    if len(win)==32:
        mw=pmask(win)
        if len(set(mw))>3:
            for sh,_ in c.most_common(3):
                lo=o+sh-0x800;blob_=nrd(lo-8,0x1000+32);hits=[k for k in range(0,0x1000,8) if pmask(blob_[k:k+32])==mw]
                if len(hits)==1:return lo+hits[0],f'이웃 이동량 +0x{sh:x} 근처 32바이트 창(절대 포인터 가림) 유일 일치(0x{lo+hits[0]:x})'
    # 레코드 표 {절대 포인터 P, 플래그…}: 창이 단조로우면 P 가 가리키는 곳(P 의 내용 = 포인터들)을 대응시켜 P' 를 찾고, P' 를 담은 레코드를 o+이동량 근처에서 찾는다
    for back in(8,0,16):
        pb=ord_(o-back,8)
        if len(pb)<8:continue
        P=struct.unpack('<Q',pb)[0]-IB
        if not(0x1340000<=P<0x2a00000):continue
        pc=ord_(P,32)
        if len(pc)<32:continue
        ptrs=[struct.unpack_from('<Q',pc,k)[0]-IB for k in range(0,32,8)]
        if not all(0x1000<=x<0x2a00000 for x in ptrs):continue
        mp=[]
        for x in ptrs:
            y=(MANUAL.get(x) or _fm.get(x)) if x<0x1340000 else (vt_map().get(x) or _map_data_plain(x)[0])
            if y is None:break
            mp.append(y)
        if len(mp)<4:continue
        key=b''.join(struct.pack('<Q',IB+y) for y in mp)
        d=NEW_D;hits=[]
        for vs,va,rs,ra in NEW_SECS:
            if va<0x1340000:continue
            i=d.find(key,ra,ra+rs)
            while i!=-1:hits.append(va+i-ra);i=d.find(key,i+1,ra+rs)
        if len(hits)!=1:continue
        Pn=hits[0];keyr=struct.pack('<Q',IB+Pn);recs=[]
        for sh,_ in c.most_common(3):
            lo=o+sh-0x1000;blob_=nrd(lo-back,0x2000)
            k=blob_.find(keyr)
            while k!=-1:recs.append(lo+k);k=blob_.find(keyr,k+1)
        recs=sorted(set(recs))
        if len(recs)==1:return recs[0],f'레코드 표: 포인터 내용 대응(0x{P:x}→0x{Pn:x}) 으로 찾음'
    return None,'자료 못 찾음'

# ---- vtable 칸 대응(R43): 옛 .rdata 8정렬 칸이 코드(함수)를 가리키면, 새 .rdata 에서 대응 함수를 가리키는 칸을 찾는다.
#  한 번에 전부 만들어 둔다. 후보가 여럿(같은 함수가 여러 vtable 에)이면 가까운 확정 칸의 이동량과 같은 것을 고른다.
_vtc={}
def _vt_mc(q):
    if q not in _vtc:_vtc[q]=map_code(q)[0]
    return _vtc[q]
_vt=None
def vt_map():
    global _vt
    if _vt is not None:return _vt
    def rsecs(secs,d):
        out=[]
        for vs,va,rs,ra in secs:
            if 0x1340000<=va<0x2700000:out.append((va,rs,ra))
        return out
    newpos=collections.defaultdict(list)
    for va,rs,ra in rsecs(NEW_SECS,NEW_D):
        for k in range(0,rs-7,8):
            q=struct.unpack_from('<Q',NEW_D,ra+k)[0]-IB
            if 0x1000<=q<0x1340000:newpos[q].append(va+k)
    anchors={};amb={}
    for va,rs,ra in rsecs(OLD_SECS,OLD_D):
        for k in range(0,rs-7,8):
            q=struct.unpack_from('<Q',OLD_D,ra+k)[0]-IB
            if not(0x1000<=q<0x1340000):continue
            nf=MANUAL.get(q) or _fm.get(q)
            if nf is None:
                nf=_vt_mc(q)
                if nf is None:continue
            c=newpos.get(nf,[])
            if len(c)==1:anchors[va+k]=c[0]
            elif len(c)>1:amb[va+k]=c
    ak=sorted(anchors);out=dict(anchors)
    for s_,c in amb.items():
        i=bisect.bisect_left(ak,s_);near=[ak[j] for j in range(max(0,i-3),min(len(ak),i+3))]
        shifts=collections.Counter(anchors[a]-a for a in near if abs(a-s_)<0x1000)
        for sh,_ in shifts.most_common():
            if s_+sh in c:out[s_]=s_+sh;break
    _vt=out;return out
def map_slot(o):
    """옛 .rdata 칸(코드 포인터) → 새 칸. (새 칸, 근거) / (None, 이유)"""
    m=vt_map()
    if o in m:return m[o],'vtable 칸 내용(함수 대응)으로 찾음'
    # 이웃 칸 대응 + 같은 오프셋. 옛 내용이 코드 포인터면 새 내용의 함수 머리가 옛 함수 머리와 같아야 함(잎은 ret 까지)
    oq=struct.unpack('<Q',ord_(o,8))[0] if len(ord_(o,8))==8 else 0
    def head_same(of,nf):
        ln=head_len(of);return imask('old',of,ln)==imask('new',nf,ln)
    for dlt in(8,16,24,32,-8,-16,-24,-32,40,48,-40,-48):
        if o+dlt in m:
            n=m[o+dlt]-dlt;nq=struct.unpack('<Q',nrd(n,8))[0] if len(nrd(n,8))==8 else 0
            if IB+0x1000<=oq<IB+0x1340000:
                if IB+0x1000<=nq<IB+0x1340000 and head_same(oq-IB,nq-IB):return n,f'이웃 칸(0x{o+dlt:x}) 대응 {-dlt:+d}, 새 칸 내용 함수 머리 일치'
            elif ord_(o,8)==nrd(n,8) or oq<IB:return n,f'이웃 칸(0x{o+dlt:x}) 대응 {-dlt:+d}'
    return None,'vtable 칸 못 찾음'
_map_data_plain=map_data
def map_data(o):
    n,how=_map_data_plain(o)
    if n is not None:return n,how
    return map_slot(o)

# ---- 바뀐 함수 안의 작은 자리(2~10바이트) 대응(R43): 앞뒤 문맥(명령 단위 마스크)으로 새 함수 안에서 가장 비슷한 창을 찾는다.
def map_site_fuzzy(o,ln,ctx=24,min_score=0.85,margin=0.08):
    """(새 자리, 근거) / (None, 이유). 근거 글에 점수 포함. 함수 대응이 있는 자리만."""
    ko=_akeys('old');kn=_akeys('new')
    f=func_of(o);ns=None
    if f:
        ns,how=map_code(f[0])
        if ns is None and f[0] in _fm:ns=_fm[f[0]];how='함수대응(머리 다름)'
    if ns is not None:
        lo,hi=f[0],oend[f[0]];nlo,nhi=ns,nend.get(ns,ns+0x2000)
    else:
        i=bisect.bisect_left(ostart,o);c=collections.Counter(_fm[ostart[j]]-ostart[j] for j in range(max(0,i-4),min(len(ostart),i+4)) if ostart[j] in _fm)
        if not c:return None,'함수 밖·이웃 없음'
        sh=c.most_common(1)[0][0];lo,hi=o-0x200,o+ln+0x200;nlo,nhi=o+sh-0x800,o+ln+sh+0x800;how=f'함수 밖·이웃 이동량 +0x{sh:x}'
    ws=ko[max(0,bisect.bisect_right(ko,max(lo,o-ctx))-1)];i=bisect.bisect_left(ko,min(hi,o+ln+ctx));we=ko[i] if i<len(ko) else o+ln+ctx
    key=imask('old',ws,we-ws)
    if len(key)<ln+8:return None,'문맥 창이 너무 짧음'
    L=len(key);best=[]
    for j in range(bisect.bisect_left(kn,nlo),bisect.bisect_left(kn,nhi)):
        k=kn[j];w=imask('new',k,L)
        if len(w)<L*0.8:continue
        sc=sum(1 for a,b in zip(key,w) if a==b)/L;best.append((sc,k))
    if not best:return None,'후보 없음'
    best.sort(reverse=True);sc,k=best[0];sc2=best[1][0] if len(best)>1 else 0
    if sc<min_score:return None,f'최고 점수 {sc:.2f} 낮음(창 0x{k:x})'
    if sc2>sc-margin and best[1][1]!=k:return None,f'후보가 둘({sc:.2f}/{sc2:.2f}) 0x{k:x} 0x{best[1][1]:x}'
    # 창 안에서 자리 맞추기: 옛 자리 바이트(마스크)를 k+(o-ws) 근처 ±8 에서 찾음
    sb=imask('old',o,ln) if o in asm('old') else mask(ord_(o,ln))
    guess=k+(o-ws);cands=[]
    for dlt in range(-8,9):
        q=guess+dlt
        w=imask('new',q,ln) if q in asm('new') else mask(nrd(q,ln))
        if w==sb:cands.append(q)
    if not cands:return None,f'창은 맞음({sc:.2f}, 0x{k:x})이나 자리 바이트 못 맞춤'
    q=min(cands,key=lambda x:abs(x-guess))
    return q,f'문맥 창 점수 {sc:.2f}(2위 {sc2:.2f}) {how} 창 0x{k:x} 자리 이동 {q-guess:+d}'

def equiv_block(o,n,ln):
    """긴 기대 바이트 블록 비교: 명령 단위 마스크가 같으면 OK. 다르면 '차이가 모두 코드 뒤 점프표(4바이트 RVA 열)'일 때만 OK.
    반환 (ok, 이유)"""
    al=_align(o,n,ln);dl=0
    if al is None and o in asm('old') and n in asm('new'):a=imask('old',o,ln);b=imask('new',n,ln)
    elif isinstance(al,tuple):po,pn,dl=al;a=imask('old',po,ln+dl)[dl:];b=imask('new',pn,ln+dl)[dl:]
    else:a=mask(ord_(o,ln));b=mask(nrd(n,ln))
    if a==b and len(a)==ln:return True,'같음'
    L=min(len(a),len(b));d=[i for i in range(L) if a[i]!=b[i]]
    if not d and len(a)!=len(b):d=[L]
    if not d:return True,'같음'
    f=d[0]-((o+d[0])%4)
    if f<0:f=0
    ob=ord_(o+f,ln-f);nb=nrd(n+f,ln-f)
    if len(ob)!=len(nb) or len(ob)<4:return False,f'차이 {len(d)}곳 @+{d[0]:#x}'
    ov=[struct.unpack_from('<I',ob,k)[0] for k in range(0,len(ob)-3,4)];nv=[struct.unpack_from('<I',nb,k)[0] for k in range(0,len(nb)-3,4)]
    dif=[(x,y) for x,y in zip(ov,nv) if x!=y]
    okc=sum(1 for x,y in dif if 0x1000<=x<0x1340000 and 0x1000<=y<0x1341000 and abs((y-x)-(n-o))<0x4000)
    if dif and okc>=max(1,int(len(dif)*0.9)) and (len(ob)-len(dif)*4)>=0:
        # 차이가 점프표 칸(RVA) 뿐인지: 차이 바이트가 모두 그 dword 들 안에 있어야
        return True,f'코드 같음 + 뒤 점프표 {len(dif)}칸(RVA) 이동 @+{f:#x}'
    return False,f'차이 {len(d)}곳 @+{d[0]:#x}'
