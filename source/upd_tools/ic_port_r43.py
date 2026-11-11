# R43 IC(SAN14_IntegratedConvenience.dll) 전용 자료 이식기 — 1.0.0→1.0.1 때의 src42d/port/ic_port2.py 를 새 대응(newmap)으로 다시 쓴 것.
#  apply_new.py 가 바꾼 것(코드 즉시값·지문·훅표 0xdd9d0)은 그대로 두고, 자료 구역(.rdata/.data)의 표들을 번역한다:
#   ① 0x92a 확인표(0xb7310, 16바이트 항목 {qword 자리, u32 길이, u32 블롭 오프셋}) + 블롭(0xb5710, 0x1bb0): 자리 번역 + 기대 바이트를 새 exe 에서 다시 채움
#   ② 포인터 표 {qword 자료 자리, qword 코드 자리}: 옛 exe 의 자료 자리 내용이 코드 자리를 가리키는 짝 → 둘 다 번역, 새 exe 에서도 짝이 맞는지 확인
#   ③ 호출 자리 표 24바이트 {qword 자리, 기대 5바이트, 0×11}
#   ④ 서술자: {u32/qword 자리 + (길이) + 기대 바이트(인라인 또는 IC 안 포인터)} — 기대 바이트가 옛 exe 그 자리와 같을 때만
#   ⑤ CAPACITY100 표 0xc5dd0 {u32 코드 자리, u32 자료 자리}×3 (1.0.1 때 손으로 고쳤던 것)
#   ⑥ 남은 8정렬 qword RVA(.rdata<0xe4000, .data<0x107000; 글자처럼 보이면·외톨이면 건너뜀) — ic_port2 와 같은 규칙
#   ⑦ 번역한 코드 자리의 옛 exe 바이트 열(≥8)이 자료 구역에 남아 있으면 새 바이트로(지문)
#  모든 기대 바이트 교체는 옛/새 명령 단위 마스크가 같을 때만(다르면 '손으로' 목록).
#  사용: python3 ic_port_r43.py <payload_cur IC> <payload_new IC(입력이자 출력)> [--dry]
import sys,os,struct,bisect,collections,hashlib;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
CURP,NEWP=sys.argv[1:3];DRY='--dry' in sys.argv
cur=open(CURP,'rb').read();d=bytearray(open(NEWP,'rb').read());assert len(cur)==len(d)
IB=X.IB;ICB=0x180000000
def secs(b):
    pe=struct.unpack_from('<I',b,0x3c)[0];ns=struct.unpack_from('<H',b,pe+6)[0];opt=struct.unpack_from('<H',b,pe+20)[0]
    return [(b[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',b,pe+24+opt+40*i+8) for i in range(ns)]
S=secs(cur)
def off(r):
    for n,vs,va,rs,ra in S:
        if va<=r<va+max(vs,rs):return ra+r-va
OINS=set(X._akeys('old'));NINS=set(X._akeys('new'))
rep=collections.Counter();log=[];hand=[];blob=[]
def inblob(r):return any(a<=r<b for a,b in blob)
def L(*a):log.append('\t'.join(str(x) for x in a))
def Hd(*a):hand.append('\t'.join(str(x) for x in a))
_mc={}
import re as _re
REPORT=os.path.join(os.path.dirname(NEWP),'..','적용_보고.txt')
if os.path.exists(REPORT):
    for l in open(REPORT,encoding='utf-8'):
        m=_re.match(r'SAN14_IntegratedConvenience\.dll\t파일 0x[0-9a-f]+\t주소\t0x([0-9a-f]+) → 0x([0-9a-f]+)',l)
        if m:_mc[int(m.group(1),16)]=(int(m.group(2),16),'apply_new 코드 즉시값')
def tr(v):
    if v not in _mc:_mc[v]=X.map_code(v) if v<0x1340000 else X.map_data(v)
    return _mc[v]
def equiv(o,n,ln):return X.equiv_block(o,n,ln)[0] if ln>=16 else X.same_code(o,n,ln)
def same_as_cur(fo,n):return bytes(d[fo:fo+n])==cur[fo:fo+n]
def put(fo,b):
    if not DRY:d[fo:fo+len(b)]=b
# ---------- ① 0x92a 표 + 블롭
TAB,BLOB,N=0xb7310,0xb5710,0x92a;blob.append((TAB,TAB+16*N));blob.append((BLOB,BLOB+0x1bb0));last_new=None
for i in range(N):
    o=off(TAB)+16*i;orva,ln,bo=struct.unpack_from('<QII',cur,o)
    if not same_as_cur(o,16):Hd('0x92a',i,'이미 바뀐 항목?',hex(orva));continue
    n,how=tr(orva)
    if n is None or not equiv(orva,n,ln):
        n2,how2=X.map_site_fuzzy(orva,ln)
        if n2 is not None and equiv(orva,n2,ln):
            L('0x92a-fuzzy',i,hex(orva),'→',hex(n2),ln,how2,'(1차: '+('번역 못함 '+how if n is None else f'0x{n:x} 명령 다름')+')');n=n2;rep['0x92a-fuzzy']+=1
        else:
            # 검증 전용 표(IC 는 2346곳 memcmp 만 함: 0x18004b5d0/0x18004b950 루프) → 못 찾은 자리는 새 exe 의 '그 함수 시작'(없으면 직전 항목 자리)으로 재표본
            f=X.func_of(orva);ns=None
            if f:ns=X.map_code(f[0])[0] or X._fm.get(f[0])
            if ns is None:ns=last_new
            if ns is None:Hd('0x92a',i,hex(orva),ln,'재표본 자리도 없음: '+how);continue
            L('0x92a-resample',i,hex(orva),'→',hex(ns),ln,'(1차: '+('번역 못함 '+how if n is None else f'0x{n:x} 명령 다름')+' | 문맥: '+how2+')');n=ns;rep['0x92a-resample']+=1
            put(o,struct.pack('<Q',n));put(off(BLOB)+bo,X.nrd(n,ln));last_new=n;continue
    put(o,struct.pack('<Q',n));nbb=X.nrd(n,ln);assert len(nbb)==ln;last_new=n
    assert cur[off(BLOB)+bo:off(BLOB)+bo+ln]==X.ord_(orva,ln),('블롭이 옛 exe 와 다름',i,hex(orva))
    put(off(BLOB)+bo,nbb);rep['0x92a']+=1
    if n!=orva:rep['0x92a-moved']+=1
    if nbb!=X.ord_(orva,ln):rep['0x92a-bytes']+=1
# ---------- ⑤ CAPACITY100 표 0xc5dd0
CAP=0xc5dd0;blob.append((CAP,CAP+24))
for k in range(3):
    o=off(CAP)+8*k;s,t=struct.unpack_from('<II',cur,o);ns_,h1=tr(s);nt,h2=tr(t)
    if ns_ is None or nt is None:Hd('CAP',hex(s),hex(t),h1,h2);continue
    b=X.nrd(ns_,7);disp=struct.unpack('<i',b[3:7])[0]
    if b[:2]!=b'\x48\x8d' or ns_+7+disp!=nt:Hd('CAP','새 lea 가 새 표를 안 가리킴',hex(ns_),b.hex(),hex(nt));continue
    put(o,struct.pack('<II',ns_,nt));rep['cap']+=1;L('CAP',hex(s),'→',hex(ns_),hex(t),'→',hex(nt))
# ---------- ② 포인터 표 {qword 자료, qword 코드} (옛 exe 에서 자료 자리 내용 == IB+코드)
for name,vs,va,rs,ra in S:
    if name not in('.rdata','.data'):continue
    for k in range(0,rs-15,8):
        r=va+k
        if inblob(r):continue
        a,b=struct.unpack_from('<QQ',cur,ra+k)
        if not(0x1340000<=a<0x2a00000 and 0x1000<=b<0x1340000):continue
        o8=X.ord_(a,8)
        if len(o8)<8 or struct.unpack('<Q',o8)[0]!=IB+b:continue
        if not same_as_cur(ra+k,16):Hd('ptr','이미 바뀜',hex(r));continue
        na,h1=tr(a);nb_,h2=tr(b)
        if na is None:Hd('ptr',hex(r),hex(a),hex(b),'자료 번역 못함 '+h1);continue
        n8=X.nrd(na,8);actual=struct.unpack('<Q',n8)[0]-IB if len(n8)==8 else None
        if actual is None or actual not in NINS:Hd('ptr',hex(r),hex(a),'→',hex(na),'새 자리 내용이 코드가 아님',hex(actual) if actual else None);continue
        if nb_!=actual:
            # 새 vtable 칸 내용이 진짜 — 옛 대응을 바로잡음(머리 바이트 확인)
            if nb_ is None or X.imask('old',b,24)==X.imask('new',actual,24):L('ptr-fix',hex(r),hex(b),'대응',hex(nb_) if nb_ else None,'→ 칸 내용',hex(actual));nb_=actual;_mc[b]=(actual,'vtable 칸 내용')
            else:Hd('ptr',hex(r),hex(a),hex(b),'대응',hex(nb_),'≠ 새 칸 내용',hex(actual),'머리도 다름');continue
        put(ra+k,struct.pack('<QQ',na,nb_));blob.append((r,r+16));rep['ptr']+=1;L('ptr',hex(r),hex(a),'→',hex(na),hex(b),'→',hex(nb_),h1)
        if (na,nb_)!=(a,b):rep['ptr-moved']+=1
# ---------- ③ 호출 자리 표 24바이트
for name,vs,va,rs,ra in S:
    if name not in('.rdata','.data'):continue
    for k in range(0,rs-24,8):
        r=va+k
        if inblob(r):continue
        orva=struct.unpack_from('<Q',cur,ra+k)[0]
        if not(0x1000<=orva<0x1340000):continue
        o5=X.ord_(orva,5)
        if len(o5)<5 or cur[ra+k+8:ra+k+13]!=o5 or cur[ra+k+13:ra+k+24]!=b'\0'*11:continue
        if not same_as_cur(ra+k,24):Hd('callsite','이미 바뀜',hex(r));continue
        n,how=tr(orva)
        if n is None or not equiv(orva,n,5):
            n2,how2=X.map_site_fuzzy(orva,5)
            if n2 is not None and equiv(orva,n2,5):L('callsite-fuzzy',hex(r),hex(orva),'→',hex(n2),how2);n=n2;rep['callsite-fuzzy']+=1
            elif n is None:Hd('callsite',hex(r),hex(orva),'번역 못함 '+how,'| 문맥: '+how2);continue
            else:Hd('callsite',hex(r),hex(orva),'→',hex(n),'옛/새 명령 다름',o5.hex(),X.nrd(n,5).hex(),'| 문맥: '+how2);continue
        put(ra+k,struct.pack('<Q',n));put(ra+k+8,X.nrd(n,5));blob.append((r,r+24));rep['callsite']+=1
        if n!=orva:rep['callsite-moved']+=1
# ---------- ④-0 서술자 D {u64 자리, u64 길이, u64 IC VA 포인터}(0xaee90 표 등) + alt 버퍼
for name,vs,va,rs,ra in S:
    if name not in('.rdata','.data'):continue
    for k in range(0,rs-23,8):
        r=va+k
        if inblob(r):continue
        rva,ln,ptr=struct.unpack_from('<QQQ',cur,ra+k)
        if not(0x1000<=rva<0x2a00000 and 1<=ln<=0x2000 and ICB+0x9a000<=ptr<ICB+0x210000):continue
        po=off(ptr-ICB)
        if po is None or cur[po:po+ln]!=X.ord_(rva,ln) or len(X.ord_(rva,ln))!=ln:continue   # 기대 바이트가 옛 exe 그 자리와 같을 때만(명령 경계가 아니어도 됨: 0x7e9890·4096바이트 덩어리)
        if not same_as_cur(ra+k,24) or not same_as_cur(po,ln):Hd('descD','이미 바뀜',hex(r));continue
        if rva>=0x1340000:
            # 자료 구역 기대값(전역 검사표의 4·12바이트 상수 등): 자료 대응 + 내용 같음
            n,how=tr(rva)
            if n is None or X.nrd(n,ln)!=cur[po:po+ln]:
                Hd('descD-자료',hex(r),hex(rva),ln,'자료 대응 못함/내용 다름 '+how,X.nrd(n,ln).hex() if n else '');continue
            put(ra+k,struct.pack('<Q',n));blob.append((r,r+24));blob.append((ptr-ICB,ptr-ICB+ln));rep['descD-data']+=1;L('descD-data',hex(r),hex(rva),'→',hex(n),ln,how)
            if k+32<=rs:
                p4=struct.unpack_from('<Q',cur,ra+k+24)[0]
                if ICB+0x9a000<=p4<ICB+0x210000 and off(p4-ICB) is not None and p4!=ptr:blob.append((r+24,r+32))
            continue
        n,how=tr(rva)
        # 손으로 정한 자리(IC 7번 포로 묶음, 게임이 포로 처리 함수 0x2e0d24→0x2e3054 를 고침):
        #  옛 0x2e110c(34): cmp [rsp+0xd8],r12d / je / mov rcx,r14 / call A(0x2337b0) / test / je / call B  →  새 함수에선 A 호출이 r15·r14·r14 세 번으로 늘어남.
        #  구조가 같은 세 번째(mov rcx,r14 / call A / test / je / call B = 0x2e3478) 를 대응 자리로 삼음. 0x2e11e6(12) 은 0x2e35a6 에 그대로 있음.
        MANUAL_DESC={0x2e110c:0x2e3478,0x2e11e6:0x2e35a6}
        if rva in MANUAL_DESC:n=MANUAL_DESC[rva];how='손 대응(7번 포로: 새 함수의 같은 구조 자리)';L('descD-manual',hex(r),hex(rva),'→',hex(n),ln,how)
        GLOBAL=0xdd240<=r<0xde800   # 전역 호환 검사표(0x18006e1eb 루프): 검사 전용(원본 또는 다른 모드 패치 뒤 모습) → 새 exe 로 재표본해도 됨
        ACCEPT_IMM={0x3d5cd0:'cmp edx,0x1c5→0x1c6 (게임 상수 1 증가) — 기대 바이트만 새 값으로'}
        if n is not None and not equiv(rva,n,ln) and rva in ACCEPT_IMM:L('descD-imm',hex(r),hex(rva),'→',hex(n),ln,ACCEPT_IMM[rva]);rep['descD-imm']+=1
        elif rva in MANUAL_DESC:pass
        elif n is None or not equiv(rva,n,ln):
            n2,how2=X.map_site_fuzzy(rva,ln)
            if n2 is not None and equiv(rva,n2,ln):n=n2;how=how2
            elif GLOBAL:
                f=X.func_of(rva);ns_=(X.map_code(f[0])[0] or X._fm.get(f[0])) if f else (n or None)
                if ns_ is None:Hd('descD-전역',hex(r),hex(rva),ln,'재표본 자리 없음 '+how);continue
                n=ns_;how='전역 검사표 재표본(새 함수 시작, 바뀐 함수)';rep['descD-global-resample']+=1;L('descD-global-resample',hex(r),hex(rva),'→',hex(n),ln,'(1차: '+how2+')')
            else:
                # 못 맞춘 기능 서술자: 자리는 새 함수 시작으로 옮기되 기대 바이트는 옛 것 그대로 → IC 가 검사에서 실패해 그 기능만 안전하게 건너뜀(엉뚱한 자리 패치 방지)
                f=X.func_of(rva);ns_=(X.map_code(f[0])[0] or X._fm.get(f[0])) if f else None
                if ns_ is not None and same_as_cur(ra+k,8):put(ra+k,struct.pack('<Q',ns_));blob.append((r,r+24));rep['descD-failsafe']+=1
                Hd('descD-기능꺼짐',hex(r),hex(rva),'→',hex(ns_) if ns_ else '(옛 자리 유지)',ln,'번역 못함/명령 다름 '+how,'| 문맥: '+how2);continue
        put(ra+k,struct.pack('<Q',n));nbb=X.nrd(n,ln);put(po,nbb);blob.append((r,r+24));blob.append((ptr-ICB,ptr-ICB+ln));rep['descD']+=1
        L('descD',hex(r),hex(rva),'→',hex(n),ln,'bytes@',hex(ptr-ICB),how)
        # 4번째 qword: 두 번째 버퍼(원본 사본 또는 다른 모드 패치 뒤 모습) — 원본과 같으면 새 바이트로, 다르면 차이 바이트만 유지
        if k+32<=rs:
            p4=struct.unpack_from('<Q',cur,ra+k+24)[0]
            if ICB+0x9a000<=p4<ICB+0x210000 and off(p4-ICB) is not None and p4!=ptr:
                pa=off(p4-ICB);alt=cur[pa:pa+ln];orig=cur[po:po+ln]
                if alt==orig:put(pa,nbb);blob.append((p4-ICB,p4-ICB+ln));blob.append((r+24,r+32));rep['descD-buf2']+=1
                elif sum(1 for i2 in range(ln) if alt[i2]!=orig[i2])<=max(8,ln//4):
                    nalt=bytearray(nbb);moved=[]
                    if X.imask('old',rva,ln)==X.imask('new',n,ln):
                        for i2 in range(ln):
                            if alt[i2]!=orig[i2]:nalt[i2]=alt[i2]
                    else:
                        # 바뀐 함수: 차이 바이트가 든 옛 명령과 같은 글의 명령을 새 함수에서 찾아(유일) 명령 안 같은 자리에
                        Ao=X.asm('old');Bn=X.asm('new');ko=X._akeys('old');kn=X._akeys('new');import bisect as _b
                        difs=[i2 for i2 in range(ln) if alt[i2]!=orig[i2]];ok_all=True
                        for i2 in difs:
                            oi=ko[_b.bisect_right(ko,rva+i2)-1];txt=Ao[oi][:2];ins_off=rva+i2-oi
                            cands=[kn[j] for j in range(_b.bisect_left(kn,n),_b.bisect_left(kn,n+ln)) if Bn[kn[j]][:2]==txt]
                            if len(cands)==1 and 0<=cands[0]+ins_off-n<ln:nalt[cands[0]+ins_off-n]=alt[i2];moved.append((i2,cands[0]+ins_off-n))
                            else:ok_all=False
                        if not ok_all:Hd('descD-alt',hex(r),hex(rva),ln,'바뀐 함수에서 차이 바이트 자리 못 옮김',str(difs));nalt=None
                    if nalt is not None:put(pa,bytes(nalt));blob.append((p4-ICB,p4-ICB+ln));blob.append((r+24,r+32));rep['descD-alt']+=1;L('descD-alt',hex(r),hex(rva),ln,'bytes@',hex(p4-ICB),'차이 바이트 유지'+(f' 자리 이동 {moved}' if moved else ''))
# 0xaefc0: game+0xffa2c8 의 32바이트(IC 시작 검사 첫 항목)
_v=0xffa2c8;_n,_h=tr(_v)
if _n is None or not equiv(_v,_n,32):Hd('0xaefc0',hex(_v),'→',_n and hex(_n),'번역/명령 문제 '+_h)
else:
    assert cur[off(0xaefc0):off(0xaefc0)+32]==X.ord_(_v,32);put(off(0xaefc0),X.nrd(_n,32));blob.append((0xaefc0,0xaefe0));rep['aefc0']+=1;L('0xaefc0',hex(_v),'→',hex(_n),_h)
# ---------- ④ 서술자(기대 바이트 인라인/포인터)
def matchrun(a,b):
    n=0
    while n<len(a) and n<len(b) and a[n]==b[n]:n+=1
    return n
hits=[]
for name,vs,va,rs,ra in S:
    if name not in('.rdata','.data'):continue
    pos=0
    while pos<=rs-16:
        r=va+pos
        if inblob(r):pos+=4;continue
        v=struct.unpack_from('<I',cur,ra+pos)[0]
        if 0x10000<=v<0x1340000 and v in OINS:
            ob_=X.ord_(v,0x2000);hi=struct.unpack_from('<I',cur,ra+pos+4)[0];ptr=struct.unpack_from('<Q',cur,ra+pos+8)[0]
            if 1<=hi<=0x2000 and ICB+0x9a000<=ptr<ICB+0x210000 and off(ptr-ICB) is not None and cur[off(ptr-ICB):off(ptr-ICB)+hi]==ob_[:hi]:
                hits.append(('B',r,v,hi,ptr-ICB));pos+=16;continue
            found=False
            for o_ in([4,8,12,16,20,24,28,32] if hi!=0 else [8,16,24,32,40,48]):
                if pos+o_+6>rs:break
                n=matchrun(cur[ra+pos+o_:ra+pos+o_+0x400],ob_)
                if n>=6:hits.append(('A' if hi!=0 else 'C',r,v,n,r+o_));found=True;break
            if found:pos+=4;continue
        pos+=4
hits.sort(key=lambda x:x[1]);capped=[]
for i,(kind,r,v,n,bp) in enumerate(hits):
    if kind!='B':
        nxt=hits[i+1][1] if i+1<len(hits) else None;prv=hits[i-1] if i>0 else None;stride=None
        if nxt is not None and nxt-r<=0x100 and (i+2>=len(hits) or hits[i+2][1]-nxt==nxt-r or prv is None or r-prv[1]==nxt-r):stride=nxt-r
        elif prv is not None and prv[0]!='B' and r-prv[1]<=0x100:stride=r-prv[1]
        if stride is not None:n=min(n,stride-(bp-r))
        if nxt is not None:n=min(n,nxt-bp)
    capped.append((kind,r,v,n,bp))
for kind,r,v,ln,bp in capped:
    if ln<=0:continue
    n,how=tr(v)
    if n is None or not equiv(v,n,ln):
        n2,how2=X.map_site_fuzzy(v,ln)
        if n2 is not None and equiv(v,n2,ln):L('desc-fuzzy',kind,hex(r),hex(v),'→',hex(n2),ln,how2);n=n2;rep['desc-fuzzy']+=1
        elif n is None:Hd('desc',kind,hex(r),hex(v),ln,'번역 못함 '+how,'| 문맥: '+how2);continue
        else:Hd('desc',kind,hex(r),hex(v),'→',hex(n),ln,'옛/새 명령 다름',X.ord_(v,ln).hex(),X.nrd(n,ln).hex(),'| 문맥: '+how2);continue
    if not same_as_cur(off(r),8 if kind=='C' else 4) or not same_as_cur(off(bp),ln):Hd('desc','이미 바뀜',kind,hex(r));continue
    put(off(r),struct.pack('<Q',n) if kind=='C' else struct.pack('<I',n));put(off(bp),X.nrd(n,ln));blob.append((r,r+(8 if kind=='C' else 4)));blob.append((bp,bp+ln));rep['desc-'+kind]+=1
    if n!=v:rep['desc-moved']+=1
    L('desc',kind,hex(r),hex(v),'→',hex(n),ln,'bytes@',hex(bp),how)
# ---------- ⑥-1 손으로: 7번 포로 묶음 패치 자리 표(0x1069e0, 0x38바이트 항목) entry1 @0x106a18
#  옛: 자리 0x2e111d(call A) 를 'e8 → 0x2339c0'(=함수 0x233960+0x60 으로 들어가 앞 검사를 건너뜀) 로 바꿈.  새: 자리 0x2e347b, 대상 = map_code(0x2339c0)(같은 함수 같은 오프셋, 명령 동일 확인).
_o=off(0x106a18);_rva,_ln=struct.unpack_from('<QI',cur,_o)
if _rva==0x2e111d and _ln==5 and cur[_o+0xc:_o+0x11]==bytes.fromhex('e89e28f5ff') and same_as_cur(_o,0x38):
    _ns=0x2e347b;_nt=X.map_code(0x2339c0)[0];assert _nt is not None and X.same_code(0x2339c0,_nt,24),('우회 진입점 0x2339c0 대응 실패',_nt)
    assert X.nrd(_ns,5)[0]==0xe8 and (_ns+5+struct.unpack('<i',X.nrd(_ns,5)[1:5])[0])&0xffffffff==0x235150,'새 자리가 call A 가 아님'
    rel=(_nt-(_ns+5))&0xffffffff;put(_o,struct.pack('<Q',_ns));put(_o+0xc,b'\xe8'+struct.pack('<I',rel));blob.append((0x106a18,0x106a18+0x38));rep['prisoner-site']+=1
    L('prisoner-site','0x106a18','0x2e111d → 0x%x'%_ns,'call 0x2339c0 → call 0x%x'%_nt,'bytes e8'+struct.pack('<I',rel).hex())
else:Hd('prisoner-site','0x106a18 항목이 예상과 다름',hex(_rva),_ln,cur[_o+0xc:_o+0x11].hex())
# ---------- ⑥ 남은 8정렬 qword RVA
pe=struct.unpack_from('<I',cur,0x3c)[0];dd=pe+24+112;dirs=[struct.unpack_from('<II',cur,dd+8*i) for i in range(16)]
imp=[];iva,isz=dirs[1];o=off(iva)
while True:
    oft,ts,fc,nm,ft=struct.unpack_from('<IIIII',cur,o)
    if oft==0 and ft==0:break
    imp.append((nm,nm+64))
    for arr in(oft,ft):
        if not arr:continue
        k=0
        while True:
            q=struct.unpack_from('<Q',cur,off(arr)+8*k)[0]
            if q==0:break
            if not(q>>63):imp.append((q,q+2+len(cur[off(q)+2:off(q)+200].split(b'\0')[0])+1))
            k+=1
        imp.append((arr,arr+8*(k+1)))
    o+=20
def indir(r):return any(sz and va<=r<va+sz for va,sz in dirs) or any(a<=r<b for a,b in imp)
def plausible(v):
    if v<0x1340000:return v in OINS
    if v in X.SITE or v in X.DATA_MANUAL:return True
    i=bisect.bisect_right(X._dx,v)-1
    if i>=0 and (v-X._dx[i])%8==0 and v-X._dx[i]<0x400:return True
    return i>=0 and v-X._dx[i]<0x200
def textlike(fo):
    b=cur[fo-16:fo+8]
    def oku(u):return (0x20<=u<0x7f) or 0xac00<=u<=0xd7a3 or 0x3130<=u<=0x318f or 0x3000<=u<=0x303f or u in(0xa,0xd)
    units=[struct.unpack_from('<H',b,i)[0] for i in range(0,24,2)];prev=units[:8]
    u16=all(oku(u) or u==0 for u in prev) and sum(oku(u) for u in prev)>=3 and oku(units[8]) and (units[9]==0 or oku(units[9]))
    lo4=b[16:20];asc2=all(0x20<=c<0x7f for c in lo4[:3]) and lo4[3]==0 and all(0x20<=c<0x7f for c in b[12:16])
    asc=all(0x20<=c<0x7f or c in(0xa,0xd,0) for c in b[:16]) and sum(1 for c in b[:16] if 0x20<=c<0x7f)>=10 and all(0x20<=c<0x7f or c==0 for c in b[16:20])
    return u16 or asc or asc2
for name,vs,va,rs,ra in S:
    if name not in('.rdata','.data'):continue
    for k in range(0,rs-7,8):
        r=va+k
        if inblob(r) or indir(r):continue
        q=struct.unpack_from('<Q',cur,ra+k)[0]
        if not(0x1000<=q<0x2a00000) or not plausible(q):continue
        if textlike(ra+k):rep['q-textskip']+=1;continue
        if (name=='.rdata' and r>=0xe4000) or (name=='.data' and r>=0x107000):rep['q-regionskip']+=1;continue
        def nq(x):return struct.unpack_from('<Q',cur,ra+k+x)[0] if 0<=k+x<=rs-8 else 0
        if not any((0x10000<=x<0x2a00000 and plausible(x)) or (ICB<=x<ICB+0x210000) or (0<x<0x4000) for x in(nq(-16),nq(-8),nq(8),nq(16))):rep['q-isolated']+=1;continue
        if not same_as_cur(ra+k,8):rep['q-already']+=1;continue
        n,how=tr(q)
        if n is None:Hd('qword',name,hex(r),hex(q),'번역 못함 '+how);continue
        if n!=q:put(ra+k,struct.pack('<Q',n));rep['q-moved']+=1;L('qword',name,hex(r),hex(q),'→',hex(n),how)
        else:rep['q-same']+=1
        blob.append((r,r+8))
# ---------- ⑦ 지문(번역한 코드 자리의 옛 바이트 열이 자료 구역에 남은 것)
refs=sorted(v for v in _mc if v<0x1340000 and _mc[v][0] is not None)
for v in refs:
    n=_mc[v][0];ob_=X.ord_(v,64);nb_=X.nrd(n,64)
    if len(ob_)<8 or len(nb_)<8:continue
    for Ln in range(min(len(ob_),len(nb_)),7,-1):
        chunk=ob_[:Ln]
        if chunk==nb_[:Ln]:break
        found=[]
        for name,vs,va,rs,ra in S:
            if name not in('.rdata','.data'):continue
            i=cur.find(chunk,ra,ra+rs)
            while i>=0:
                if not inblob(va+i-ra) and same_as_cur(i,Ln):found.append((name,va+i-ra,i))
                i=cur.find(chunk,i+1,ra+rs)
        if not found:continue
        if Ln<12 and len(found)>1:Hd('지문',hex(v),f'길이 {Ln} 가 {len(found)}곳 — 짧아서 안 바꿈');break
        if not equiv(v,n,Ln):Hd('지문',hex(v),'→',hex(n),Ln,'옛/새 명령 다름',' '.join(hex(f[1]) for f in found));break
        for name,r,fo in found:put(fo,nb_[:Ln]);blob.append((r,r+Ln));rep['sig']+=1;L('지문',name,hex(r),hex(v),'→',hex(n),Ln)
        break
# ---------- 남은 옛 주소 점검(보고용): 8정렬 qword / 4정렬 dword 가 옛 명령 시작이고 대응이 다른 것
resid=collections.Counter()
for name,vs,va,rs,ra in S:
    if name not in('.rdata','.data'):continue
    for k in range(0,rs-7,8):
        r=va+k
        if inblob(r):continue
        q=struct.unpack_from('<Q',d,ra+k)[0]
        if 0x10000<=q<0x1340000 and q in OINS and q in X._fm:resid[name]+=1;Hd('남은qword(보고만)',name,hex(r),hex(q),'→',hex(X._fm[q]),'주변',cur[ra+k-8:ra+k+16].hex())
out=['[요약] '+str(dict(rep)),'[남은 qword(옛 함수 시작과 같은 값, 보고만)] '+str(dict(resid)),'','[바꾼 곳]']+log+['','[손으로 볼 곳]']+hand
open(NEWP+'.ic_port_r43.txt','w',encoding='utf-8').write('\n'.join(out)+'\n')
if not DRY:open(NEWP,'wb').write(d)
print(dict(rep));print('남은',dict(resid));print('손으로',len(hand));print('\n'.join(hand[:80]))
print('새 해시',hashlib.sha256(bytes(d)).hexdigest())
