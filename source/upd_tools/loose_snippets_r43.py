# R43: 바이너리 모듈 자료 구역의 '느슨한 지문'(옛 exe .text 바이트 열을 그대로 비교·검색하는 조각)과 그 짝 RVA 칸 점검·번역
#  발견 계기: SAN14_RelatedOfficersResolver.dll 이 지문 표 말고도 .rodata 에 44바이트짜리 코드 조각을 두고 게임 .text 에서 '정확히 한 번' 찾는다(안에 call rel32).
#            Traits9_UI({u32 rva,u32 len,bytes[len]} 기록)·FogTrial({u32 rva,u32 len,u64 ptr} 표)은 apply_data_new 의 '강한 근거' 조건에 걸리지 않아
#            rva 칸도 기대 바이트도 옛 값 그대로 남아 있었다.
#  규칙: ① 자료 구역에서 옛 exe .text 에 있는 ≥6바이트 열을 찾고, 옛 exe 에서 유일한 자리(osite)에 맞춰 앞뒤로 최대한 넓힌다(진짜 지문 시작 복원)
#        ② newmap 으로 새 자리(nsite). 못 찾으면 '다음 함수 시작' 기준으로 되짚어 명령 단위 일치(same_code)를 확인
#        ③ 자료 구역 어디든 u32 == 옛 rva(지문 안을 가리키는 값)가 있으면 짝 RVA 칸: 옛 값 그대로면 새 rva 로, 이미 바뀌었으면 newmap 결과와 같은지 대조(다르면 손으로)
#        ④ 지문 바이트는 길이 ≥8·명령 단위 같은 코드일 때만 새 exe 바이트로 교체(짧은 것·명령이 달라진 것은 보고만)
#  리졸버 지문 표 항목 구역(resolver_port_r43 가 처리)은 건너뛴다. payload_cur 와 바이트가 같은 자리만 건드린다(이중 번역 방지).
#  사용: python3 loose_snippets_r43.py <payload_cur dll> <payload_new dll(제자리 수정)> [--dry]
import sys,os,struct,bisect,collections;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
DRY='--dry' in sys.argv;ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
def text_of(data):
    p=struct.unpack_from('<I',data,0x3c)[0];o=struct.unpack_from('<H',data,p+20)[0];vs,va,rs,ra=struct.unpack_from('<IIII',data,p+24+o+8);return va,bytes(data[ra:ra+vs])
ova,OT=text_of(X.OLD_D);nva,NT=text_of(X.NEW_D);OINS=set(X._akeys('old'));OSTART=sorted(X.ostart)
def _secs(D):
    p=struct.unpack_from('<I',D,0x3c)[0];k=struct.unpack_from('<H',D,p+6)[0];o=struct.unpack_from('<H',D,p+20)[0];return [struct.unpack_from('<IIII',D,p+24+o+40*i+8) for i in range(k)]
_SO=_secs(X.OLD_D);_SN=_secs(X.NEW_D)
def _rva_of(D,fo):
    for vs,va,rs,ra in _SO:
        if ra<=fo<ra+rs:return va+fo-ra
def _rd_q(D,rva):
    for vs,va,rs,ra in _SN:
        if va<=rva<va+rs:return struct.unpack_from('<Q',D,ra+rva-va)[0]
def run(CUR,NEW,DRY):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read());assert len(c)==len(d),'크기 다름'
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    S=[(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8)+(struct.unpack_from('<I',d,pe+24+opt+40*i+36)[0],) for i in range(ns)]
    DATA=[(n,vs,va,rs,ra) for n,vs,va,rs,ra,ch in S if rs and not(ch&0x20000000) and n in('.rodata','.data','.rdata')]
    # 지문 표 항목 구역(건너뜀) — resolver_port_r43 와 같은 판별
    skip=[]
    for n,vs,va,rs,ra in DATA:
        for k in range(0,rs-0x100,8):
            total,deref,reloff,add,co,cl=struct.unpack_from('<6I',c,ra+k)
            if 8<=total<=0x104 and 3<cl<=total and co+cl<=total and deref<=0x104 and reloff<=0x104 and add<0x100000 and k+0x11c+total<=rs:
                mask=c[ra+k+0x11c:ra+k+0x11c+total]
                if all(b in(0,0xff) for b in mask) and mask.count(0xff)>=4:skip.append((ra+k,ra+k+0x220));continue
            t32,co2,cl2,fl=struct.unpack_from('<4I',c,ra+k)
            for fmt,PO,MO,pmax,sz in(('B',0x10,0x128,0x118,0x240),('C',0xc,0xce,0xc2,0x190)):
                if fmt=='B' and fl>1:continue
                if 8<=t32<=pmax and 3<cl2<=t32 and co2+cl2<=t32 and k+MO+t32<=rs:
                    mask=c[ra+k+MO:ra+k+MO+t32]
                    if all(b in(0,0xff) for b in mask) and mask.count(0xff)>=4 and all(b==0xff for b in mask[co2:co2+cl2]) and all(b==0 for b in c[ra+k+PO+t32:ra+k+MO]) and all(b==0 for b in c[ra+k+MO+t32:ra+k+MO+pmax]):
                        skip.append((ra+k,ra+k+sz));break
    def in_skip(fo):
        for a,b in skip:
            if a<=fo<b:return b
        return None
    def count(hay,pat,limit=3):
        cnt=0;first=None;i=hay.find(pat)
        while i!=-1:
            cnt+=1
            if first is None:first=i
            if cnt>=limit:break
            i=hay.find(pat,i+1)
        return cnt,first
    def boring(w):
        return len(set(w))<=2 or all(32<=b<127 or b==0 for b in w)
    def map_site(osite):
        n,how=X.map_code(osite)
        if n is None:
            ko=X._akeys('old');j=bisect.bisect_right(ko,osite)-1
            if j>=0 and ko[j]!=osite:
                nb,h2=X.map_code(ko[j])
                if nb is not None:n=nb+(osite-ko[j]);how='앞 명령 '+h2
        if n is None:
            # 다음 함수 시작 기준(pdata 없는 작은 thunk 등)
            j=bisect.bisect_right(OSTART,osite)
            if j<len(OSTART) and OSTART[j]-osite<=0x100:
                nb,h2=X.map_code(OSTART[j])
                if nb is not None:
                    cand=nb-(OSTART[j]-osite)
                    if cand>=nva and X.same_code(osite,cand,min(16,OSTART[j]-osite)):n=cand;how='다음 함수 기준(%s)'%h2
        if n is None:
            # 절대 포인터(vtable 칸 등)가 가리키는 자리: 참조 칸들을 map_data 로 옮겨 새 포인터가 모두 같으면 그것
            key=struct.pack('<Q',X.IB+osite);D=X.OLD_D;i=D.find(key);tg=set();cnt=0
            while i!=-1 and cnt<40:
                rv=_rva_of(D,i)
                if rv:
                    nd,h2=X.map_data(rv)
                    if nd is not None:
                        q=_rd_q(X.NEW_D,nd)
                        if q and q>=X.IB:tg.add(q-X.IB)
                cnt+=1;i=D.find(key,i+1)
            if len(tg)==1:n=tg.pop();how='절대 포인터 참조 %d곳 일치'%cnt
        return n,how
    rep=[];hand=[];patched=0;links=0;seen=set()
    for n,vs,va,rs,ra in DATA:
        k=0
        while k<rs-6:
            fo=ra+k;e=in_skip(fo)
            if e:k=e-ra;continue
            w=c[fo:fo+6]
            if boring(w) or OT.find(w)==-1:k+=1;continue
            lim=rs-k
            for a,b in skip:
                if a>fo:lim=min(lim,a-fo)
            hi=6
            while hi*2<=lim and OT.find(c[fo:fo+hi*2])!=-1:hi*=2
            lo=hi;hi=min(hi*2,lim)
            while lo<hi:
                m=(lo+hi+1)//2
                if OT.find(c[fo:fo+m])!=-1:lo=m
                else:hi=m-1
            L=lo;run=bytes(c[fo:fo+L])
            oc,of=count(OT,run)
            if oc!=1:
                nc,_=count(NT,run)
                if L>=12 and nc==0:hand.append((hex(va+k),L,'옛 exe 에서 %d곳(유일하지 않음)·새 exe 에 없음 — 손으로'%oc,run[:24].hex()))
                k+=L;continue
            # 자리에 맞춰 앞뒤로 넓히기(진짜 지문 범위)
            s0=fo;o0=of
            while s0>ra and o0>0 and c[s0-1]==OT[o0-1] and not in_skip(s0-1):s0-=1;o0-=1
            e0=fo+L;oe=of+L
            while e0<ra+rs and oe<len(OT) and c[e0]==OT[oe] and not in_skip(e0):e0+=1;oe+=1
            fo,of,L=s0,o0,e0-s0;run=bytes(c[fo:fo+L]);rva=va+(fo-ra);osite=ova+of
            if (fo,L) in seen:k=fo-ra+L;continue
            seen.add((fo,L))
            already=bytes(d[fo:fo+L])!=run   # apply_data_new 가 이미 바꾼 지문: 짝 RVA 대조만 하고 바이트는 안 건드림
            nc,nf=count(NT,run)
            nsite,how=map_site(osite)
            if nsite is None:
                if nc>=1:k=fo-ra+L;continue
                hand.append((hex(rva),L,'새 자리 못 찾음',hex(osite),how,run[:24].hex()));k=fo-ra+L;continue
            nb=NT[nsite-nva:nsite-nva+L]
            same=len(nb)==L and X.same_code(osite,nsite,L);note=''
            if not same and len(nb)==L:
                cut=run.find(b'\xcc\xcc\xcc')
                if cut>=8 and X.same_code(osite,nsite,cut):same=True;note=' (패딩 뒤 %d바이트는 다른 함수)'%(L-cut)
            # ③ 짝 RVA 칸
            linked=False
            for n2,vs2,va2,rs2,ra2 in DATA:
                seg=c[ra2:ra2+rs2]
                for v in range(osite,osite+min(L,64)):
                    if v!=osite and v not in OINS:continue
                    key=struct.pack('<I',v);i=seg.find(key)
                    while i>=0:
                        j=ra2+i;cur=struct.unpack_from('<I',d,j)[0];want=nsite+(v-osite)
                        if cur==v:
                            if same or L>=16:
                                if not DRY:struct.pack_into('<I',d,j,want)
                                links+=1;linked=True;rep.append((hex(va2+i),'RVA칸',hex(v),'→',hex(want),'지문',hex(rva),L,how+note))
                            else:hand.append((hex(va2+i),'RVA칸 옛 값 그대로지만 지문 명령이 달라 보류',hex(v),'→',hex(want),'지문',hex(rva),L))
                        elif cur!=want:hand.append((hex(va2+i),'RVA칸 이미 바뀜 ★불일치',hex(v),'apply_data_new',hex(cur),'newmap',hex(want),'지문',hex(rva),L,how))
                        i=seg.find(key,i+1)
            # ④ 지문 바이트
            if already or (nc>=1 and nb==run):k=fo-ra+L;continue
            if not same:
                if nc==0:hand.append((hex(rva),L,'명령이 달라짐(바뀐 함수?) — 손으로',hex(osite),'→',hex(nsite),how,run[:24].hex(),nb[:24].hex()))
                k=fo-ra+L;continue
            if nb==run:k=fo-ra+L;continue
            if L<8:hand.append((hex(rva),L,'짧아서 안 바꿈(확인만)',hex(osite),'→',hex(nsite),how,run.hex(),nb.hex()));k=fo-ra+L;continue
            # 바로 앞 8바이트 안의 u32 가 '다른' 옛 코드 RVA 면 기록 머리(rva,len,bytes)에 딸린 기대 바이트 → 그 기록의 RVA 가 맞고 지문 검색은 우연 일치일 수 있음 → record_fields 에 맡김
            hdr=[struct.unpack_from('<I',c,fo-q)[0] for q in (4,8) if fo-q>=ra]
            if not linked and any(h in OINS and h!=osite and 0x1000<=h<len(OT)+ova for h in hdr):
                hand.append((hex(rva),L,'앞 기록 머리의 RVA 와 다른 자리에서 유일 일치 — 보고만(record_fields 가 처리)',hex(osite),'→',hex(nsite),'머리',[hex(h) for h in hdr]));k=fo-ra+L;continue
            if osite not in OINS and not linked:hand.append((hex(rva),L,'명령 경계가 아님 — 보고만',hex(osite),'→',hex(nsite),how,run[:16].hex(),nb[:16].hex()));k=fo-ra+L;continue
            nc2,_=count(NT,nb)
            if nc2!=1 and L<16 and not linked:
                # 짧고 새 exe 에서 유일하지도 않고 짝 RVA 도 없는 조각(모듈 자신의 코드 조각일 수 있음) → 보고만
                hand.append((hex(rva),L,'새 exe 에서 유일하지 않고 짝 RVA 없음 — 보고만',hex(osite),'→',hex(nsite),how,'새 개수 %d'%nc2,run.hex(),nb.hex()));k=fo-ra+L;continue
            if not DRY:d[fo:fo+L]=nb
            patched+=1;rep.append((hex(rva),L,'지문 교체',hex(osite),'→',hex(nsite),how+note,'새 exe 개수 %d'%nc2+(' (옛 %d)'%oc if nc2!=1 else ''),run[:20].hex(),nb[:20].hex()))
            k=fo-ra+L
    if not DRY and (patched or links):open(NEW,'wb').write(d)
    print('==',os.path.basename(NEW),'지문표 항목',len(skip),'지문 교체',patched,'RVA칸',links,'손으로',len(hand))
    for r in rep:print(' ',*r)
    for h in hand:print('  손으로:',*h)


if __name__=='__main__':
    if len(ARGS)==2:run(ARGS[0],ARGS[1],DRY)
    else:
        # 여러 개: <payload_cur 루트> <payload_new 루트> <상대경로 목록 파일>
        for rel in open(ARGS[2],encoding='utf-8').read().split():
            try:run(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel),DRY)
            except Exception as e:print('==',rel,'오류',repr(e))
