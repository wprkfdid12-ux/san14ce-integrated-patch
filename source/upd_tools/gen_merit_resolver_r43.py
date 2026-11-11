# 13번 MeritResolver 소스(merit_resolver.c)를 새 판 주소로 번역 (R43, 2026-11-05)
#  입력: 옛 소스, 옛/새 해제 exe, 함수대응.pkl(find_new), 자리_대응_검증.tsv
#  규칙: 함수 주소 = 함수대응(m) → 없으면 자리_대응(정확/정렬) → 없으면 이웃 함수 이동량 + 바이트 확인. 함수 안 주소 = 새 함수 시작 + 같은 오프셋(함수가 '같음' 목록일 때만 자동).
#        자료 주소 = 자리_대응(정확/검증) → 없으면 이웃 자료 이동량 + 내용(16바이트) 확인. 기대 바이트(프롤로그·훅 바이트)는 새 exe 에서 다시 읽음.
#        판 지문(0x6a983c25,0x29ff000)은 새 값으로. 못 정한 것은 그대로 두고 보고서에 적음(빌드 전에 손으로).
#  사용: python3 gen_merit_resolver_r43.py <옛 merit_resolver.c> <옛 exe> <새 exe> <함수대응.pkl> <바뀐함수.pkl> <자리_대응_검증.tsv> <출력 .c> <보고 .txt>
import sys,re,struct,pickle,bisect,collections
SRC,OLDEXE,NEWEXE,FMAP,CHG,MAP,OUT,REP=sys.argv[1:9]
NEW_STAMP,NEW_SIZE=0x6ad06f53,0x2a02000
def load(p):
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
    return rd,funcs
ord_,ofn=load(OLDEXE);nrd,nfn=load(NEWEXE)
ostart=[f[0] for f in ofn];nstart=set(f[0] for f in nfn)
def func_of(r):
    i=bisect.bisect_right(ostart,r)-1
    return ofn[i] if i>=0 and r<ofn[i][1] else None
m=pickle.load(open(FMAP,'rb'))['m'];chg=pickle.load(open(CHG,'rb'));same=set(chg['same']);diff=set(chg['diff'])
rows=[l.split('\t') for l in open(MAP,encoding='utf-8').read().splitlines()[1:]]
good=lambda v:(v.startswith('정확') and not v.startswith('정확+')) or v.startswith('정렬')
site={};dpts=[]
for r in rows:
    if len(r)<13:continue
    try:o=int(r[3],16)
    except:continue
    if r[11].startswith('0x') and good(r[12]):
        n=int(r[11],16);site.setdefault(o,n)
        if r[2]=='자료':dpts.append((o,n-o))
dpts.sort();dx=[p[0] for p in dpts]
def mask(b):
    """rel32(e8/e9/0f 8x)·rip 상대(48 8b/8d 05) 변위는 0 으로. 끝에서 잘린 그런 명령은 거기서 자름(두 쪽 다 같은 길이로 비교됨)"""
    b=bytearray(b);i=0
    while i<len(b):
        if b[i] in(0xe8,0xe9):
            if i+5>len(b):return bytes(b[:i])
            b[i+1:i+5]=b'\0\0\0\0';i+=5
        elif b[i]==0x0f and i+1<len(b) and 0x80<=b[i+1]<=0x8f:
            if i+6>len(b):return bytes(b[:i])
            b[i+2:i+6]=b'\0\0\0\0';i+=6
        elif b[i] in(0x48,0x4c) and i+2<len(b) and b[i+1] in(0x8d,0x8b,0x89) and (b[i+2]&0xc7)==0x05:
            if i+7>len(b):return bytes(b[:i])
            b[i+3:i+7]=b'\0\0\0\0';i+=7   # rip 상대
        else:i+=1
    return bytes(b)
notes=[]
MANUAL={0x92ad0:0x8ca1c0}   # 옛→새 (함수 시작). 0x92ad0: 'ret 0' 디딤 함수(pdata 없음) — 새 vtable 칸 내용으로 확인(2026-11-05). 호출 쪽에서 더 채움
DATA_MANUAL={0x14784a0:0x147b530}   # 칸 자리: 이웃 칸 0x1478468→0x147b4f8(+0x3090) 과 같은 이동, 칸 내용 0x8ca1c0 확인
def head_search(o,guess,span=0x4000):
    """옛 함수 머리 16바이트(마스크)와 같은 자리를 새 exe 에서 guess 주변에서 찾음"""
    oh=mask(ord_(o,24))
    for sh in range(0,span,16):
        for c in(guess+sh,guess-sh):
            if c in nstart and mask(nrd(c,24))==oh:return c
    return None
def map_code(o):
    if o in MANUAL:return MANUAL[o],'손으로 확인'
    f=func_of(o)
    if not f:
        # .pdata 없는 작은 함수: 자리_대응 → 이웃 이동량 + 머리 바이트
        if o in site:return site[o],'자리_대응(pdata 없음)'
        i=bisect.bisect_left(ostart,o);cands=collections.Counter(m[ostart[j]]-ostart[j] for j in range(max(0,i-4),min(len(ostart),i+5)) if ostart[j] in m)
        for sh,_ in cands.most_common():
            if mask(ord_(o,24))==mask(nrd(o+sh,24)):return o+sh,f'pdata 없음·이웃 이동량 +0x{sh:x}+바이트 일치'
        return None,'함수 밖(pdata 없음)'
    st,off=f[0],o-f[0];ns=m.get(st) or site.get(st)
    how='함수대응' if st in m else ('자리_대응' if st in site else None)
    if ns is None:
        # 이웃 함수 이동량
        i=bisect.bisect_left(ostart,st);cands=collections.Counter()
        for j in range(max(0,i-4),min(len(ostart),i+5)):
            if ostart[j] in m:cands[m[ostart[j]]-ostart[j]]+=1
        for sh,_ in cands.most_common():
            if st+sh in nstart and mask(ord_(st,24))==mask(nrd(st+sh,24)):ns=st+sh;how=f'이웃 이동량 +0x{sh:x}+바이트 일치';break
        if ns is None:return None,'새 함수 못 찾음'
    if off==0 and mask(ord_(st,24))!=mask(nrd(ns,24)):
        alt=head_search(st,ns)
        if alt:notes.append(f'함수 0x{st:x}: 대응 0x{ns:x} 머리 바이트 다름 → 근처 0x{alt:x} 가 같은 머리(바로잡음)');ns=alt;how+='→머리 바이트로 바로잡음'
        else:notes.append(f'함수 0x{st:x}: 대응 0x{ns:x} 머리 바이트 다름, 근처에서도 못 찾음 — 손으로')
    tag=('바뀐 함수' if st in diff else '같은 함수' if st in same else '대응만')
    if off and st in diff:
        # 바뀐 함수 안: 오프셋 자리 바이트가 같으면 받아들임
        if mask(ord_(o,12))==mask(nrd(ns+off,12)):return ns+off,f'{how}+오프셋 0x{off:x} (바뀐 함수지만 그 자리 바이트 같음)'
        return None,f'{how} 함수 시작 0x{ns:x} 는 찾았으나 바뀐 함수 안 오프셋 0x{off:x} 자리 바이트 다름 — 손으로'
    return ns+off,f'{how}'+(f'+오프셋 0x{off:x}' if off else '')+f' ({tag})'
def map_data(o):
    if o in DATA_MANUAL:return DATA_MANUAL[o],'손으로 확인'
    if o in site:return site[o],'자리_대응'
    i=bisect.bisect_left(dx,o);c=collections.Counter(dpts[j][1] for j in range(max(0,i-3),min(len(dpts),i+3)))
    for sh,_ in c.most_common():
        if ord_(o,16)==nrd(o+sh,16):return o+sh,f'이웃 이동량 +0x{sh:x}+내용 일치'
    # vtable: 함수 포인터 배열이면 각 항목이 함수대응으로 맞는지
    for sh,_ in c.most_common():
        ok=True
        for k in range(0,16,8):
            ov=struct.unpack('<Q',ord_(o+k,8))[0]-0x140000000;nv=struct.unpack('<Q',nrd(o+sh+k,8))[0]-0x140000000
            if not(0<ov<0x1341000 and m.get(ov)==nv):ok=False;break
        if ok:return o+sh,f'이웃 이동량 +0x{sh:x}+vtable 항목 함수대응 일치'
    return None,'자료 못 찾음'
def run_once():
    global notes
    notes=[]
    return translate()

def translate():
 t=open(SRC,encoding='utf-8').read()
 # 1) 판 지문
 t=t.replace('0x6a983c25','0x%x'%NEW_STAMP).replace('0x29ff000','0x%x'%NEW_SIZE);notes.append(f'지문 → 0x{NEW_STAMP:x}/0x{NEW_SIZE:x}')
 lines=t.split('\n')
 # 2) 첫 중괄호 목록(함수 21개) / 둘째(오프셋) / 셋째(프롤로그 16바이트×21) / 훅표 줄
 def fmt(v):return '0x%x'%v
 out=[];state=0;fn_new=[]
 for ln in lines:
     s=ln
     if state==0 and ln.startswith('{0x') and ln.rstrip().endswith('},'):   # 함수 목록
         vals=[int(x,16) for x in re.findall(r'0x[0-9a-fA-F]+',ln)];nv=[]
         for v in vals:
             n,how=map_code(v);notes.append(f'함수 0x{v:x} → {fmt(n) if n else "?"}  {how}');nv.append(n if n else v);fn_new.append(n)
         s='{'+','.join(fmt(x) for x in nv)+'},';state=1
     elif state==1 and ln.startswith('{0x'):   # 오프셋 목록: 0x277df18(db), 0x491f01(hover 지문 자리), 0x1478440(자료)만
         vals=[int(x,16) for x in re.findall(r'0x[0-9a-fA-F]+',ln)];nv=[]
         for v in vals:
             if v>=0x1341000:n,how=map_data(v)
             elif v>=0x10000 and v in site:n,how=site[v],'자리_대응(코드 자리)'   # 둘째 목록은 구조 오프셋이 대부분(db+0x7dd18 등) — 자리_대응에 코드 자리로 있는 값(0x491f01)만
             else:n,how=v,'구조 오프셋(그대로)'
             if n!=v:notes.append(f'오프셋 목록 0x{v:x} → {fmt(n) if n else "?"}  {how}')
             nv.append(n if n else v)
         s='{'+','.join(fmt(x) for x in nv)+'},';state=2
     elif state==2 and ln.startswith('{{0x'):   # 프롤로그 16바이트 × 함수 수: 새 exe 에서 다시 읽음
         groups=re.findall(r'\{([^{}]*)\}',ln);nb=[]
         for i,g in enumerate(groups):
             old=bytes(int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+',g))
             nf=fn_new[i] if i<len(fn_new) else None
             if nf is None:nb.append(old);notes.append(f'프롤로그[{i}] 함수 못 정함 → 옛 바이트 유지');continue
             b=nrd(nf,16);nb.append(b)
             if b!=old:notes.append(f'프롤로그[{i}] 0x{nf:x}: {"rel32/rip 만 다름" if mask(b)==mask(old) else "바이트 다름(손으로 볼 것)"}  옛 {old.hex()} 새 {b.hex()}')
         s='{'+','.join('{'+','.join(fmt(x) for x in b)+'}' for b in nb)+'},';state=3
     elif re.match(r'\s*\{0x[0-9a-fA-F]+,\d+,H_',ln):   # 훅표 줄: {rva,len,id,kind,flags,{bytes},target}
         mm=re.match(r'(\s*\{)(0x[0-9a-fA-F]+),(\d+),(\w+),(\d+),(\d+),\{([^}]*)\},(0x[0-9a-fA-F]+)(\}.*)',ln)
         if mm:
             rva=int(mm.group(2),16);ln_=int(mm.group(3));kind=int(mm.group(5));tgt=int(mm.group(8),16)
             if kind==1 or rva>=0x1341000:nr,how=map_data(rva)
             else:nr,how=map_code(rva)
             nt,how2=(map_code(tgt) if tgt<0x1341000 else map_data(tgt)) if tgt else (0,'-')
             if kind==1 and tgt and (nr is None or nt is None):
                 # 칸 이웃 이동량 후보마다: 새 칸 내용 = 함수 X, X 의 머리 바이트가 옛 대상과 같으면 채택
                 i=bisect.bisect_left(dx,rva);c=collections.Counter(dpts[j][1] for j in range(max(0,i-4),min(len(dpts),i+4)))
                 for sh,_ in c.most_common():
                     cand=rva+sh;b=nrd(cand,8)
                     if len(b)!=8:continue
                     x=struct.unpack('<Q',b)[0]-0x140000000
                     if 0<x<0x1341000 and mask(nrd(x,24))==mask(ord_(tgt,24)):nr,how=cand,f'이웃 칸 이동량 +0x{sh:x}';nt,how2=x,'새 칸 내용(머리 바이트 일치)';MANUAL[tgt]=x;break
             oldb=bytes(int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+',mm.group(7)))
             if kind==0 and oldb and oldb[0] in(0xe8,0xe9) and nt:
                 def rel_target(rd,at):b=rd(at,5);return (at+5+struct.unpack('<i',b[1:5])[0]) if len(b)==5 and b[0]==oldb[0] else None
                 if nr is None or rel_target(nrd,nr)!=nt:
                     guess=nr if nr is not None else rva+(nt-tgt);found=None
                     for sh in range(0,0x4000):
                         for c in(guess+sh,guess-sh):
                             if rel_target(nrd,c)==nt:found=c;break
                         if found is not None:break
                     if found is not None:notes.append(f'훅표 {mm.group(4)} 0x{rva:x}: 대응 자리 {fmt(nr) if nr else "?"} 가 새 대상 0x{nt:x} 을 안 부름 → 근처 0x{found:x} 가 부름(바로잡음)');nr=found;how='rel 대상으로 찾음'
                     else:notes.append(f'훅표 {mm.group(4)} 0x{rva:x}: 새 대상 0x{nt:x} 을 부르는 자리를 근처에서 못 찾음 — 손으로')
             if nr is not None:
                 nb=nrd(nr,ln_)
                 if nb!=oldb and not(kind==1 and nt is not None and len(nb)==8 and struct.unpack('<Q',nb)[0]-0x140000000==nt):notes.append(f'훅표 {mm.group(4)} 0x{rva:x}→0x{nr:x} 바이트 {"rel32/rip 만 다름" if mask(nb)==mask(oldb) else "다름(손으로 볼 것)"}: 옛 {oldb.hex()} 새 {nb.hex()}')
                 if kind==1 and nt is not None and struct.unpack('<Q',nb)[0]-0x140000000!=nt:notes.append(f'훅표 {mm.group(4)} 칸 0x{nr:x} 내용 0x{struct.unpack("<Q",nb)[0]-0x140000000:x} ≠ 대상 대응 0x{nt:x} — 손으로')
             else:nb=oldb
             notes.append(f'훅표 {mm.group(4)} 자리 0x{rva:x} → {fmt(nr) if nr else "?"} ({how}); 대상 0x{tgt:x} → {fmt(nt) if nt is not None else "?"} ({how2})')
             s=f'{mm.group(1)}{fmt(nr if nr else rva)},{ln_},{mm.group(4)},{kind},{mm.group(6)},{{{",".join(fmt(x) for x in nb)}}},{fmt(nt if nt is not None else tgt)}{mm.group(9)}'
     elif 'base+0x' in ln:   # R43 시험3: verify_row_identity 의 `base+0x89abe0u` 식 함수 상수(표 밖) — 첫 PC 실행에서 13번이 소리 없이 꺼진 원인
         def _rep(m):
             v=int(m.group(1),16);nv,how=map_code(v) if v<0x1341000 else map_data(v)
             notes.append(f'코드 상수 base+0x{v:x} → {fmt(nv) if nv else "?"} ({how})');return f'base+0x{nv:x}u' if nv else m.group(0)
         s=re.sub(r'base\+0x([0-9a-fA-F]+)u',_rep,ln)
     out.append(s)
 return out

out=run_once();out=run_once()   # 2차: 1차에서 칸 내용으로 찾은 함수(MANUAL)를 함수 목록에도 반영
open(OUT,'w',encoding='utf-8').write('\n'.join(out))
unres=[n for n in notes if '?' in n or '손으로' in n]
open(REP,'w',encoding='utf-8').write(f'미정/손으로 {len(unres)}건\n'+'\n'.join(unres)+'\n\n[전체]\n'+'\n'.join(notes)+'\n')
print(f'번역 완료 → {OUT}; 미정/손으로 {len(unres)}건 → {REP}')
for n in unres:print(' ',n[:200])
