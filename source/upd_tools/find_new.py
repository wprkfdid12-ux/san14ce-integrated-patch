# 새 판 자리 자동 찾기: 옛 판(1.0.1) ↔ 새 판 실행 파일을 함수 단위로 맞춘 뒤, 대장(연결자리_대장.tsv)의 모든 주소를 새 판 주소로 옮기고 검증한다.
#  사용: python3 find_new.py <옛 해제본.exe> <새 해제본.exe> <연결자리_대장.tsv> <출력폴더>
#  (해제본 = 분석/stubdec.py 로 SteamStub 를 푼 exe)
#  결과: 출력폴더/자리_대응.tsv (항목마다 새 주소·판정), 출력폴더/요약.txt
#  판정: 정확(함수 내용 같음) / 정렬(함수가 조금 바뀌었지만 그 명령은 같은 자리) / 확인필요(근처가 바뀜, 사람이 볼 것) / 못찾음
#  알려진 함정 대비: 상수를 주소로 옮기지 않음(대장에 있는 것만 옮김), 4자리 주소 포함, 닮은 함수는 호출 관계 다수결로만 붙임,
#                   함수 안 위치가 명령 경계가 아니면 '확인필요', 훅 자리는 새 판 원본 바이트(상대 주소 제외)가 옛 판과 같은지 검사.
import struct,re,sys,os,subprocess,pickle,collections,hashlib,bisect,difflib
BASE=0x140000000
OLD,NEW,LEDGER,OUT=sys.argv[1:5];STRUCT=sys.argv[5] if len(sys.argv)>5 else None;os.makedirs(OUT,exist_ok=True)
def secs_of(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0];out=[]
    for i in range(n):
        o=pe+24+opt+40*i;out.append((d[o:o+8].rstrip(b'\0').decode(),)+struct.unpack_from('<IIII',d,o+8))
    return out
def pdata(d,secs):
    for nm,vs,va,rs,ra in secs:
        if nm=='.pdata':
            out=[]
            for k in range(0,vs,12):
                b,e,u=struct.unpack_from('<III',d,ra+k)
                if b==0:break
                out.append((b,e))
            return sorted(set(out))
def text_end(secs):
    for nm,vs,va,rs,ra in secs:
        if nm=='.text':return va+vs
def asm_of(path):
    cache=os.path.join(OUT,os.path.basename(path)+'.asm.pkl')
    if os.path.exists(cache):return pickle.load(open(cache,'rb'))
    ins={};pat=re.compile(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*(\S+)\s*(.*)$')
    p=subprocess.Popen(['objdump','-d','-M','intel','--no-show-raw-insn',path],stdout=subprocess.PIPE,text=True,errors='replace')
    pat2=re.compile(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$')
    for line in p.stdout:
        m=pat2.match(line)
        if not m:continue
        ops=m.group(3);tail=''
        if '#' in ops:ops,tail=ops.split('#',1)
        try:a=int(m.group(1),16)-BASE
        except ValueError:continue
        ins[a]=(m.group(2),ops.split('<')[0].strip(),tail.strip())
    p.wait();pickle.dump(ins,open(cache,'wb'));return ins
HEX=re.compile(r'0x[0-9a-f]+')
def norm(mn,ops):return mn+' '+HEX.sub(lambda x:'X' if int(x.group(0),16)>=0x1000 else x.group(0),ops)
def build(path):
    d=open(path,'rb').read();secs=secs_of(d);funcs=pdata(d,secs);ins=asm_of(path);ks=sorted(ins)
    F={}
    for b,e in funcs:
        lo=bisect.bisect_left(ks,b);hi=bisect.bisect_left(ks,e);sig=[];calls=[]
        for a in ks[lo:hi]:
            mn,ops,_=ins[a];sig.append(norm(mn,ops))
            if mn=='call' or mn.startswith('j'):
                m=re.fullmatch(r'0x([0-9a-f]+)',ops)
                if m:
                    t=int(m.group(1),16)-BASE
                    if not(b<=t<e):calls.append(t)
        F[b]=dict(end=e,n=len(sig),h=hashlib.md5('\n'.join(sig).encode()).hexdigest(),calls=calls,sig=sig)
    return d,secs,F,ins,ks
print('옛 판 읽는 중…',flush=True);od,osecs,old,oins,oks=build(OLD)
print('새 판 읽는 중…',flush=True);nd,nsecs,new,nins,nks=build(NEW)
print('함수',len(old),len(new),flush=True)
# ---- 함수 맞추기 (port/match3.py 와 같은 단계: 같은 내용 유일 → 같은 내용 무리는 가까운 쪽 → 호출 관계 다수결 → 근처 유사도)
oh=collections.defaultdict(list);nh=collections.defaultdict(list)
for a,f in old.items():oh[f['h']].append(a)
for a,f in new.items():nh[f['h']].append(a)
m={}
for h,la in oh.items():
    if len(la)==1 and len(nh.get(h,[]))==1:m[la[0]]=nh[h][0]
anchors=sorted(m.items());akeys=[a for a,_ in anchors]
def expected(a):
    i=bisect.bisect_left(akeys,a);c=[]
    if i<len(akeys):c.append(anchors[i])
    if i>0:c.append(anchors[i-1])
    if not c:return a
    p=min(c,key=lambda p:abs(p[0]-a));return a+(p[1]-p[0])
used=set(m.values())
for h,la in oh.items():
    lb=[b for b in nh.get(h,[]) if b not in used];la=[a for a in la if a not in m]
    if not la or not lb:continue
    for dd,a,b in sorted((abs(expected(a)-b),a,b) for a in la for b in lb):
        if a in m or b in used or dd>0x40000:continue
        m[a]=b;used.add(b)
ocall=collections.defaultdict(set);ncall=collections.defaultdict(set)
for a,f in old.items():
    for t in f['calls']:ocall[t].add(a)
for a,f in new.items():
    for t in f['calls']:ncall[t].add(a)
for it in range(8):
    added=0
    for a,f in old.items():
        if a in m:continue
        c=collections.Counter()
        for t in f['calls']:
            if t in m:
                for x in ncall.get(m[t],()):
                    if x not in used and x in new:c[x]+=1
        for x in ocall.get(a,()):
            if x in m:
                for t in new[m[x]]['calls']:
                    if t not in used and t in new:c[t]+=1
        if not c:continue
        ex=expected(a);best=[x for x,_ in c.most_common(6) if abs(new[x]['n']-f['n'])<=max(2,f['n']*0.15) and abs(x-ex)<0x40000]
        if best:
            best.sort(key=lambda x:(-c[x],abs(x-ex)));m[a]=best[0];used.add(best[0]);added+=1
    if not added:break
cand=sorted(b for b in new if b not in used)
for a in [a for a in old if a not in m]:
    f=old[a];ex=expected(a);lo=bisect.bisect_left(cand,ex-0x8000);hi=bisect.bisect_left(cand,ex+0x8000);best=None
    for b in cand[lo:hi]:
        g=new[b]
        if b in used or abs(g['n']-f['n'])>max(3,f['n']*0.3):continue
        r=difflib.SequenceMatcher(None,f['sig'],g['sig'],autojunk=False).ratio()
        if r>0.85 and(best is None or r>best[0]):best=(r,b)
    if best:m[a]=best[1];used.add(best[1])
okeys=sorted(old)
print('맞춘 함수',len(m),'/',len(old),flush=True)
pickle.dump({'m':m,'old':{a:(f['end'],f['n'],f['h']) for a,f in old.items()},'new':{a:(f['end'],f['n'],f['h']) for a,f in new.items()}},open(os.path.join(OUT,'함수대응.pkl'),'wb'))   # R42i4: 바뀐 함수 목록용
# ---- 주소 옮기기
def func_of(keys,F,r):
    i=bisect.bisect_right(keys,r)-1
    if i>=0 and r<F[keys[i]]['end']:return keys[i]
    return None
def seg(ks,b,e):lo=bisect.bisect_left(ks,b);hi=bisect.bisect_left(ks,e);return ks[lo:hi]
def gap(r):
    i=bisect.bisect_right(okeys,r)-1;ob=old[okeys[i]]['end'] if i>=0 else 0;oe=okeys[i+1] if i+1<len(okeys) else text_end(osecs)
    j=i
    while j>=0 and okeys[j] not in m:j-=1
    k=i+1
    while k<len(okeys) and okeys[k] not in m:k+=1
    if j<0 or k>=len(okeys):return None
    nb=new[m[okeys[j]]]['end'];ne=m[okeys[k]]
    if ne<nb or ne-nb>0x8000:return None
    return ob,oe,nb,ne
def trans_code(r):
    b=func_of(okeys,old,r)
    if b is None:
        g=gap(r)
        if g is None:return None,'함수 밖(맞출 이웃 없음)'
        ob,oe,nb,ne=g;oi=seg(oks,ob,oe);ni=seg(nks,nb,ne);kind='함수밖'
    elif b not in m:return None,'함수 못 맞춤'
    else:
        nb=m[b];oi=seg(oks,b,old[b]['end']);ni=seg(nks,nb,new[nb]['end']);kind='함수'
    if r not in oins:
        i=bisect.bisect_right(oks,r)-1;a=oks[i];na,note=trans_code(a) if a!=r else (None,'')
        if na is None:return None,'명령 경계 아님'
        return na+(r-a),note+f'+{r-a}(명령 안)'
    k=oi.index(r);os_=[norm(*oins[a][:2]) for a in oi];ns_=[norm(*nins[a][:2]) for a in ni]
    if os_==ns_:return ni[k],'정확'
    for tag,i1,i2,j1,j2 in difflib.SequenceMatcher(None,os_,ns_,autojunk=False).get_opcodes():
        if i1<=k<i2:
            if tag=='equal':
                # 앞뒤 3줄도 같은지
                ok=all(0<=k+t-i1+j1<len(ns_) and 0<=k+t<len(os_) and os_[k+t]==ns_[k+t-i1+j1] for t in (-3,-2,-1,1,2,3) if 0<=k+t<len(os_))
                return ni[j1+(k-i1)],'정렬' if ok else '확인필요(앞뒤 바뀜)'
            if tag=='replace' and i2-i1==j2-j1:return ni[j1+(k-i1)],'확인필요(명령 바뀜)'
            return None,f'못찾음(바뀐 구간 {tag})'
    return None,'?'
ripidx=collections.defaultdict(list)
for a,(mn,ops,tail) in oins.items():
    t=re.match(r'0x([0-9a-f]+)',tail)
    if t:ripidx[int(t.group(1),16)-BASE].append(a)
def qword(d,secs,r):
    o=off(d,secs,r)
    return struct.unpack_from('<Q',d,o)[0] if o is not None else None
_nptr=None
def new_ptr_index():
    global _nptr
    if _nptr is None:
        _nptr=collections.defaultdict(list)
        for nm,vs,va,rs,ra in nsecs:
            if nm in ('.rdata','.data'):
                for k in range(0,rs-7,8):
                    q=struct.unpack_from('<Q',nd,ra+k)[0]
                    if BASE<=q<BASE+0x1400000:_nptr[q-BASE].append(va+k)
    return _nptr
def trans_ptr(r):
    """함수 포인터 칸(vtable 등): 칸 안 함수 → 새 함수 → 그 함수를 가리키는 새 칸. 여럿이면 칸 묶음(vtable) 시작 기준으로"""
    q=qword(od,osecs,r)
    if q is None or not(BASE<=q<BASE+0x1400000):return None,None
    f=q-BASE;nf,_=trans_code(f)
    if nf is None:return None,'못찾음(칸 안 함수 못 맞춤)'
    c=new_ptr_index().get(nf,[])
    if len(c)==1:return c[0],'정확(칸 내용 유일)'
    if not c:return None,'못찾음(새 판에 그 함수 칸 없음)'
    # vtable 시작(코드가 rip 로 가리키는 가장 가까운 앞 주소) 기준 같은 칸
    k=r
    while k>r-0x800 and k not in ripidx:k-=8
    if k in ripidx:
        nb,_=trans_data_rip(k)
        if nb is not None and nb+(r-k) in c:return nb+(r-k),'정확(vtable 기준)'
    return None,f'확인필요(같은 함수 칸 {len(c)}곳)'
def trans_data(r):
    v,n=trans_data_rip(r)
    if v is not None:return v,n
    pv,pn=trans_ptr(r)
    if pn:return pv,pn
    return v,n
def trans_data_rip(r):
    refs=ripidx.get(r,[]);votes=collections.Counter()
    for a in refs[:60]:
        na,note=trans_code(a)
        if na is None or na not in nins:continue
        t=re.match(r'0x([0-9a-f]+)',nins[na][2])
        if t:votes[int(t.group(1),16)-BASE]+=1
    if not votes:return None,f'못찾음(옛 판 참조 {len(refs)}곳)'
    v,c=votes.most_common(1)[0];tot=sum(votes.values())
    return v,('정확' if c==tot and c>=2 else '확인필요')+f'(참조 {c}/{tot})'
def off(d,secs,r):
    for nm,vs,va,rs,ra in secs:
        if va<=r<va+rs:return r-va+ra
def ins_span(ins,ks,a,ln):
    i=bisect.bisect_left(ks,a);out=[]
    while i<len(ks) and ks[i]<a+ln:out.append(norm(*ins[ks[i]][:2]));i+=1
    return out
def bytes_same(r,nr,ln):
    o1=off(od,osecs,r);o2=off(nd,nsecs,nr)
    if o1 is None or o2 is None:return '?'
    a=od[o1:o1+ln];b=nd[o2:o2+ln]
    if a==b:return '같음'
    if r<text_end(osecs):
        # 상대 주소(rip·호출 대상)만 다르고 명령이 같으면 같음으로 봄 — 훅 원본 바이트는 새 판 값으로 다시 뜨면 됨
        if ins_span(oins,oks,r,ln)==ins_span(nins,nks,nr,ln) and r in oins and nr in nins:
            if a[0] in (0xe8,0xe9) and b[0]==a[0]:
                ta=r+5+struct.unpack_from('<i',a,1)[0];tb=nr+5+struct.unpack_from('<i',b,1)[0];tt,_=trans_code(ta)
                if tt!=tb:return f'다름(호출 대상 0x{ta:x}→새 0x{tb:x}, 대응 {hex(tt) if tt else "없음"})'
                return '같음(호출 대상 대응)'
            return '같음(상대 주소만 다름)'
    return '다름'
rows=[l.rstrip('\n').split('\t') for l in open(LEDGER,encoding='utf-8')];head=rows[0];rows=rows[1:]
out=open(os.path.join(OUT,'자리_대응.tsv'),'w',encoding='utf-8');out.write('\t'.join(head+['새RVA','판정','바이트'])+'\n')
stat=collections.Counter();bad=[]
cache={}
for r in rows:
    v=int(r[3],16);ln=int(r[4]) if r[4].isdigit() else 0
    if v not in cache:
        cache[v]=trans_code(v) if v<text_end(osecs) else trans_data(v)
    nv,note=cache[v]
    if nv is None and v not in oins and not ripidx.get(v) and r[0]=='소스' and (v<0x1000 or v>=text_end(osecs) and qword(od,osecs,v) is None or v in (0x29ff000,0x10000) or r[2]=='코드' and v not in oins):note='상수·구조오프셋으로 보임(주소 아님)'
    bs=bytes_same(v,nv,ln) if (nv is not None and ln) else ''
    verdict=note.split('(')[0]
    if bs.startswith('다름'):verdict='확인필요'
    stat[(r[0],verdict)]+=1
    if verdict.startswith('상수'):pass
    elif verdict!='정확' and verdict!='정렬':bad.append((r[0],r[1],r[3],note,bs))
    out.write('\t'.join(r+[f'0x{nv:x}' if nv is not None else '',note,bs])+'\n')
out.close()
with open(os.path.join(OUT,'요약.txt'),'w',encoding='utf-8') as f:
    f.write(f'옛 판 {OLD}\n새 판 {NEW}\n함수 {len(old)} / {len(new)}, 맞춘 함수 {len(m)}\n\n판정 (출처별)\n')
    for k,c in sorted(stat.items()):f.write(f'  {k[0]}\t{k[1]}\t{c}\n')
    f.write(f'\n사람이 볼 것 {len(bad)}건 (자리_대응.tsv 에서 판정이 정확·정렬이 아닌 줄)\n')
    for b in bad[:400]:f.write('  '+'\t'.join(b)+'\n')
# ---- 구조값 검사
if STRUCT:
    srows=[l.rstrip('\n').split('\t') for l in open(STRUCT,encoding='utf-8')][1:]
    so=open(os.path.join(OUT,'구조값_대응.tsv'),'w',encoding='utf-8');so.write('종류\t이름\t옛 오프셋\t판정\t새 오프셋(다수)\t증인 결과\n')
    sstat=collections.Counter()
    def vote(lst,v):
        votes=collections.Counter();lost=0
        for w in [x for x in lst.split(',') if x][:12]:
            w=int(w,16);nw,_=trans_code(w)
            if nw is None or nw not in nins or oins[w][0]!=nins[nw][0]:lost+=1;continue
            od_=[int(x,16) for x in re.findall(r'\+0x([0-9a-f]+)\]',oins[w][1])];nd_=re.findall(r'\+0x([0-9a-f]+)\]',nins[nw][1])
            if len(od_)==len(nd_) and od_:votes[int(nd_[od_.index(v) if v in od_ else 0],16)]+=1
            else:lost+=1
        return votes,lost
    for kind,nm,v,ws,_ in srows:
        v=int(v,16);pri,_,sec=ws.partition('|')
        votes,lost=vote(pri,v);sv,sl=vote(sec,v) if sec else (collections.Counter(),0)
        note=''
        if not votes and sv:votes=sv;note='(보조 증인만으로 — 한 번 더 볼 것)'
        if not votes:verdict='못찾음(사람이 볼 것)';nv=''
        else:
            nv_,c=votes.most_common(1)[0];nv=f'0x{nv_:x}';tot=sum(votes.values())
            verdict=('그대로' if nv_==v else '바뀜') if c==tot else '확인필요(증인끼리 다름)'
            if not note and verdict!='확인필요(증인끼리 다름)' and tot<2:
                note='' if sv and sv.most_common(1)[0][0]==nv_ else '(증인 1개뿐 — 한 번 더 볼 것)'
            verdict+=note
        so.write(f'{kind}\t{nm}\t0x{v:x}\t{verdict}\t{nv}\t주{dict(votes)} 보조{dict(sv)} 못맞춤{lost}\n')
    so.close()
    # 서로 다른 옛 값이 같은 새 값으로 몰리면 하나는 틀린 것 → 확인필요
    rows=[l.rstrip('\n').split('\t') for l in open(os.path.join(OUT,'구조값_대응.tsv'),encoding='utf-8')]
    cnt=collections.Counter((r[0],r[4]) for r in rows[1:] if r[4] and r[3]=='바뀜')
    sstat=collections.Counter()
    with open(os.path.join(OUT,'구조값_대응.tsv'),'w',encoding='utf-8') as so:
        so.write('\t'.join(rows[0])+'\n')
        for r in rows[1:]:
            if r[3]=='바뀜' and cnt[(r[0],r[4])]>1:r[3]='확인필요(다른 값과 겹침)'
            pass
            sstat[r[3].split('(')[0]]+=1;so.write('\t'.join(r)+'\n')
    with open(os.path.join(OUT,'요약.txt'),'a',encoding='utf-8') as f:
        sh=collections.defaultdict(collections.Counter)
        for r in rows[1:]:
            if r[0]=='객체칸' and r[4]:sh[r[1].split()[0]][int(r[4],16)-int(r[2],16)]+=1
        f.write('\n구조별 이동량(모든 칸이 같은 만큼 움직였으면 믿을 만함): '+'; '.join(f"{k} "+', '.join(f"{'+' if d>=0 else '-'}0x{abs(d):x}×{c}" for d,c in v.most_common()) for k,v in sh.items())+'\n')
        f.write('\n구조값 판정: '+', '.join(f'{k} {c}' for k,c in sstat.items())+' (구조값_대응.tsv)\n')
print(open(os.path.join(OUT,'요약.txt'),encoding='utf-8').read()[:3000])
