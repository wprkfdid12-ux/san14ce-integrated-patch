# R43: 바이너리 모듈 자료 구역의 '기록(record) 칸' 번역 — apply_data_new 의 '강한 근거' 조건에 걸리지 않아 남은 것들
#  발견 계기(Traits9_UI): 36바이트 기록 {kind, a, idx, site, len, bytes/qword, …} 에서
#    kind1: a=호출 자리(함수 중간 RVA), site=호출 대상, bytes=`e8 rel32`(5바이트)   → a·bytes 가 옛 값 그대로
#    kind2: a=vtable 칸(자료 RVA), site=코드 RVA, qword=0x140000000+함수 RVA(칸의 기대 내용) → a·qword 가 옛 값 그대로
#  또 apply_data_new 가 {u32 rva, u32 1} 을 '절대 주소'로 잘못 보고 u64 로 써 버린 70곳은 이미 되돌렸다(u32 만 번역).
#  규칙(근거가 있는 칸만):
#   ① u64 절대 포인터(0x140000000+RVA): 코드면 map_code, 자료면 map_data
#   ② u32 코드 RVA(명령 경계) 뒤 64바이트 안에 그 자리의 옛 exe 바이트(≥5)가 그대로 있으면: RVA 와 기대 바이트 모두 번역(명령 단위 같은 코드일 때)
#   ③ ①②·apply_data_new 가 이미 바꾼 칸의 ±32바이트 안에 있는 u32 함수 시작 RVA → map_code / u32 자료 RVA → map_data
#   ④ 이미 바뀐 칸은 newmap 결과와 대조만(다르면 ★불일치 보고)
#  payload_cur 와 바이트가 같은 칸만 건드린다.
#  사용: python3 record_fields_r43.py <payload_cur dll> <payload_new dll(제자리 수정)> [--dry]
import sys,os,struct,bisect,collections;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
DRY='--dry' in sys.argv;ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
def text_of(data):
    p=struct.unpack_from('<I',data,0x3c)[0];o=struct.unpack_from('<H',data,p+20)[0];vs,va,rs,ra=struct.unpack_from('<IIII',data,p+24+o+8);return va,bytes(data[ra:ra+vs])
ova,OT=text_of(X.OLD_D);nva,NT=text_of(X.NEW_D);OINS=set(X._akeys('old'));OSTART=set(X.ostart);IB=X.IB
OTEXT_END=ova+len(OT);OIMG_END=0x29ff000
def secs(D):
    p=struct.unpack_from('<I',D,0x3c)[0];k=struct.unpack_from('<H',D,p+6)[0];o=struct.unpack_from('<H',D,p+20)[0];return [struct.unpack_from('<IIII',D,p+24+o+40*i+8) for i in range(k)]
SN=secs(X.NEW_D)
def rdq_new(rva):
    for vs,va,rs,ra in SN:
        if va<=rva<va+rs:return struct.unpack_from('<Q',X.NEW_D,ra+rva-va)[0]
def sig_regions(c,DATA):
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
    return skip
def run(CUR,NEW,DRY):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read());assert len(c)==len(d),'크기 다름'
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    S=[(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8)+(struct.unpack_from('<I',d,pe+24+opt+40*i+36)[0],) for i in range(ns)]
    DATA=[(n,vs,va,rs,ra) for n,vs,va,rs,ra,ch in S if rs and not(ch&0x20000000) and n in('.rodata','.data','.rdata')]
    rep=[];hand=[];changed=0;SKIP=sig_regions(c,DATA)
    def in_sig(fo):
        return any(a<=fo<b for a,b in SKIP)
    def R(*a):rep.append(a)
    def H(*a):hand.append(a)
    def map_any(v):
        return X.map_code(v) if v<OTEXT_END else X.map_data(v)
    for n,vs,va,rs,ra in DATA:
        anchors=set()   # 근거가 선 칸(파일 오프셋)
        patreg=[]       # ② 기대 바이트 구역 (pos,L,새 바이트) — ③ 에서 제외·마지막 검증
        # 이미 apply_data_new 가 바꾼 칸(4바이트 단위) = 근거
        for k in range(0,rs-3,4):
            if c[ra+k:ra+k+4]!=d[ra+k:ra+k+4] and not in_sig(ra+k):anchors.add(ra+k)
        # ① 절대 포인터
        for k in range(0,rs-7,4):
            q=struct.unpack_from('<Q',c,ra+k)[0]
            if not(IB+0x1000<=q<IB+OIMG_END) or in_sig(ra+k):continue
            v=q-IB;fo=ra+k
            if d[fo:fo+8]!=c[fo:fo+8]:
                nq=struct.unpack_from('<Q',d,fo)[0];m,how=map_any(v)
                if m is not None and nq!=IB+m:H(hex(va+k),'절대포인터 이미 바뀜 ★불일치',hex(v),'지금',hex(nq-IB),'newmap',hex(m),how)
                anchors.add(fo);continue
            m,how=map_any(v)
            if m is None:H(hex(va+k),'절대포인터 번역 못함',hex(v),how);continue
            anchors.add(fo)
            if m!=v:
                if not DRY:struct.pack_into('<Q',d,fo,IB+m)
                changed+=1;R(hex(va+k),'절대포인터',hex(v),'→',hex(m),how)
        # ② 코드 RVA + 뒤따르는 기대 바이트
        for k in range(0,rs-3,4):
            fo=ra+k;v=struct.unpack_from('<I',c,fo)[0]
            if not(0x1000<=v<OTEXT_END) or v not in OINS or in_sig(fo):continue
            ob=OT[v-ova:v-ova+16]
            if len(ob)<5:continue
            # 뒤 64바이트 안에서 옛 exe 바이트(≥5) 찾기
            win=c[fo+4:fo+4+64];pos=-1;L=0
            for Ltry in (16,12,10,8,6,5):
                p=win.find(ob[:Ltry])
                if p>=0:pos=fo+4+p;L=Ltry;break
            if pos<0:continue
            # 최대 길이
            while L<16 and pos+L<ra+rs and c[pos+L]==OT[v-ova+L]:L+=1
            m,how=X.map_code(v)
            if m is None:H(hex(va+k),'기대 바이트 딸린 코드 RVA 번역 못함',hex(v),how,c[pos:pos+L].hex());continue
            nb=NT[m-nva:m-nva+L];same=X.same_code(v,m,L)
            patreg.append((pos,L,bytes(nb)))
            if set(c[pos:pos+L])=={0xcc} and set(nb)!={0xcc}:
                H(hex(va+k),'동굴(cc) 기대 바이트인데 새 자리가 cc 가 아님 — 손으로',hex(v),'→',hex(m),how,nb.hex());continue
            anchors.add(fo);anchors.add(pos)
            cur=struct.unpack_from('<I',d,fo)[0]
            if cur!=v:
                if cur!=m:H(hex(va+k),'코드 RVA 이미 바뀜 ★불일치',hex(v),'지금',hex(cur),'newmap',hex(m),how)
            elif m!=v:
                if not same:H(hex(va+k),'코드 RVA(기대 바이트 딸림) 명령이 달라짐 — 손으로',hex(v),'→',hex(m),how,c[pos:pos+L].hex(),nb.hex());continue
                if not DRY:struct.pack_into('<I',d,fo,m)
                changed+=1;R(hex(va+k),'코드RVA',hex(v),'→',hex(m),how,'기대바이트',hex(va+pos-ra),L)
            if d[pos:pos+L]==c[pos:pos+L] and nb!=c[pos:pos+L]:
                if same:
                    if not DRY:d[pos:pos+L]=nb
                    changed+=1;R(hex(va+pos-ra),'기대바이트',L,c[pos:pos+L].hex(),'→',nb.hex())
                else:H(hex(va+pos-ra),'기대 바이트 명령 다름 — 손으로',hex(v),'→',hex(m),c[pos:pos+L].hex(),nb.hex())
        # ③ 근거 칸 ±32 안의 함수 시작 RVA / 자료 RVA
        if anchors:
            A=sorted(anchors)
            for k in range(0,rs-3,4):
                fo=ra+k
                if fo in anchors or d[fo:fo+4]!=c[fo:fo+4] or in_sig(fo):continue
                if any(p0-3<=fo<p0+pl for p0,pl,_ in patreg):continue   # 기대 바이트 안의 u32 는 RVA 가 아님
                v=struct.unpack_from('<I',c,fo)[0]
                if not(0x10000<=v<OIMG_END) or (v>=OTEXT_END and v%4):continue   # 자료 RVA 는 4배수만
                i=bisect.bisect_left(A,fo);near=(i<len(A) and A[i]-fo<=32) or (i>0 and fo-A[i-1]<=32)
                if not near:continue
                if v<OTEXT_END:
                    if v not in OSTART:continue
                    m,how=X.map_code(v);kind='함수RVA'
                else:
                    m,how=X.map_data(v);kind='자료RVA'
                if m is None:H(hex(va+k),kind+' 번역 못함(근거 칸 옆)',hex(v),how);continue
                if m!=v:
                    if not DRY:struct.pack_into('<I',d,fo,m)
                    changed+=1;R(hex(va+k),kind,hex(v),'→',hex(m),how)
                anchors.add(fo)
        # ③-검증: 기대 바이트 구역은 '그대로' 거나 '새 바이트' 여야 한다(엉뚱한 번역이 섞이면 ★)
        for p0,pl,nb in patreg:
            cur=bytes(d[p0:p0+pl])
            if cur!=c[p0:p0+pl] and cur!=nb:H(hex(va+p0-ra),'기대 바이트 구역 안 바이트가 엉뚱하게 바뀜 ★',c[p0:p0+pl].hex(),cur.hex(),nb.hex())
        # ④ kind2 일관성(Traits9_UI/TraitHover 36바이트 기록: {kind=2, 칸 rva, idx, 칸의 원래 내용(site), len=8, qword=0x140000000+다른 함수, …}):
        #    새 vtable 칸 내용 == 번역한 site 인지 (qword 의 함수는 칸 내용이 아니라 훅이 따로 쓰는 함수)
        for k in range(0,rs-7,4):
            q=struct.unpack_from('<Q',d,ra+k)[0]
            if not(IB+0x1000<=q<IB+OTEXT_END) or in_sig(ra+k) or k<20:continue
            kind,slot_c,idx,site_c,ln=struct.unpack_from('<5I',c,ra+k-20);slot_d,site_d=struct.unpack_from('<I',d,ra+k-16)[0],struct.unpack_from('<I',d,ra+k-8)[0]
            if kind!=2 or ln!=8 or not(0x1340000<=slot_c<OIMG_END) or not(0x1000<=site_c<OTEXT_END):continue
            got=rdq_new(slot_d)
            if got is None or got-IB!=site_d:H(hex(va+k-16),'vtable 칸 새 내용 ≠ 번역한 site ★',hex(slot_c),'→',hex(slot_d),'칸 내용',hex(got-IB) if got else None,'site',hex(site_c),'→',hex(site_d))
            else:R(hex(va+k-16),'vtable 칸 확인',hex(slot_d),'내용 = site',hex(site_d))
    if not DRY and changed:open(NEW,'wb').write(d)
    print('==',os.path.basename(NEW),'바꾼 칸',changed,'손으로',len(hand))
    for r in rep:print(' ',*r)
    for h in hand:print('  손으로:',*h)
    return changed


def run_fix(cur,new,dry):
    # ③ 은 근거 칸이 늘면 다시 걸리므로 바뀜이 없을 때까지(최대 8번) 반복
    for it in range(8):
        ch=run(cur,new,dry)
        if dry or not ch:break
if __name__=='__main__':
    if len(ARGS)==2:run_fix(ARGS[0],ARGS[1],DRY)
    else:
        # 여러 개: <payload_cur 루트> <payload_new 루트> <상대경로 목록 파일>
        for rel in open(ARGS[2],encoding='utf-8').read().split():
            try:run_fix(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel),DRY)
            except Exception as e:print('==',rel,'오류',repr(e))
