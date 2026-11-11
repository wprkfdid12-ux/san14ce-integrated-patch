# R43: 바이너리 모듈 코드의 '게임 base + 즉시값' 꼴 주소 상수 중 apply_new(find_new 자리_대응)가 못 본 것 번역
#  발견 계기: 첫 PC 실행 튕김 — IC 훅 37 처리기가 `mov rax,[게임base]; add rax,0x2318b0; call rax` 로 옛 주소(pdata 없는 작은 함수)를
#            그대로 불러 새 exe 의 엉뚱한 자리(0x231770 함수 안)로 들어감. IC 안에만 이런 즉시값이 249개(0x141e0 ×65 등) 남아 있었다.
#  명령 경계는 objdump 로 푼 결과를 쓴다(바이트 패턴만 보면 `80 3d disp32` 의 disp 를 `cmp eax,imm` 로 오해함).
#  규칙(1단계, 근거 강함): `add REG,imm32` / `lea R2,[REG+imm32]` 의 REG 가 바로 앞 ≤3명령 안에서 `mov REG,[rip+slot]`(slot = 그 모듈의 게임 base 칸)로 채워졌고
#         imm32 가 옛 exe 범위 → newmap(.text 는 map_code, 자료는 map_data)
#        (2단계, 근거 보통): `cmp eax,imm32` 로 imm 이 옛 명령 경계(OINS)이고 ≥0x10000 → 되돌아갈 주소 비교로 보고 번역(보고 따로)
#        (3단계, 보고만): `mov r32,imm32` 로 imm 이 옛 함수 시작이고 ≥0x10000 → 손으로 볼 목록
#  base slot 은 모듈마다 자동 추정(1단계 짝의 최빈 slot, 3번 이상). payload_cur 와 바이트가 같은 칸만 건드린다.
#  사용: python3 imm_port_r43.py <payload_cur 루트> <payload_new 루트> <상대경로 목록 파일> [--dry]
import sys,os,re,struct,collections,subprocess;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
DRY='--dry' in sys.argv;ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
def text_of(data):
    p=struct.unpack_from('<I',data,0x3c)[0];o=struct.unpack_from('<H',data,p+20)[0];vs,va,rs,ra=struct.unpack_from('<IIII',data,p+24+o+8);return va,bytes(data[ra:ra+vs])
ova,OT=text_of(X.OLD_D);nva,NT=text_of(X.NEW_D);OTEXT_END=ova+len(OT);OIMG_END=0x29ff000;OINS=set(X._akeys('old'));OSTART=set(X.ostart)
def secs(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    return [(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8)+(struct.unpack_from('<I',d,pe+24+opt+40*i+36)[0],) for i in range(ns)]
IB=0x180000000
LINE=re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*([a-z][a-z0-9]*)\s*(.*)$')
def disasm(path):
    out=subprocess.run(['objdump','-d','-M','intel','-w',path],capture_output=True,text=True).stdout
    ins=[]
    for ln in out.split('\n'):
        m=LINE.match(ln)
        if not m:continue
        addr=int(m.group(1),16);bs=bytes.fromhex(m.group(2).replace(' ',''));ins.append((addr,bs,m.group(3),m.group(4).strip()))
    # objdump 는 긴 명령을 두 줄로 나눠 적기도 함(바이트 이어짐, 두 번째 줄은 mnemonic 없음) — LINE 이 걸러냄; 길이 보정은 다음 주소로
    return ins
REGS={'rax':0,'rcx':1,'rdx':2,'rbx':3,'rsp':4,'rbp':5,'rsi':6,'rdi':7,'r8':8,'r9':9,'r10':10,'r11':11,'r12':12,'r13':13,'r14':14,'r15':15}
# 자료 전역(내용이 0 인 .data 칸 등): 옛 코드에서 그 칸을 [rip+x] 로 쓰는 명령들을 새 자리로 옮겨 새 코드의 [rip+x] 대상을 읽는다(2곳 이상 일치)
_refidx=None
def _refs(v):
    global _refidx
    if _refidx is None:
        _refidx=collections.defaultdict(list)
        for a,(mn,ops,tgt) in X.asm('old').items():
            if tgt.startswith('0x') and 'rip' in ops:
                t=int(tgt,16)-IB_GAME
                if t>=OTEXT_END:_refidx[t].append(a)
    return _refidx.get(v,[])
IB_GAME=0x140000000
def map_data_by_refs(v):
    # v 자체 또는 그 앞 0x200 안의 참조 자리(구조체 머리)를 쓰고, 이동량을 v 에 적용
    A=X.asm('new');AO=X.asm('old')
    for t in range(v,v-0x200,-1):
        refs=_refs(t)
        if not refs:continue
        votes=collections.Counter();tried=0
        for a in refs[:40]:
            m,how=X.map_code(a)
            if m is None or m not in A:continue
            mn,ops,tgt=A[m];mo,oo,to=AO[a]
            if mn!=mo or not(tgt.startswith('0x') and 'rip' in ops):continue
            tried+=1;votes[int(tgt,16)-IB_GAME-t]+=1
        if not votes:continue
        sh,cnt=votes.most_common(1)[0]
        if cnt>=2 or (cnt==1 and len(refs)==1 and abs(sh)<0x8000):return v+sh,'코드 참조(%s) %d/%d 일치 이동량 +0x%x'%(hex(t),cnt,tried,sh)
        return None,'코드 참조(%s) 근거 약함 %s'%(hex(t),dict(votes))
    return None,'코드 참조 없음(앞 0x200 안)'
def page_fix(imm,m):
    # 0x1000 배수(VirtualProtect 페이지 기준 등)는 번역 결과도 페이지 경계로(자리_대응 표에 있는 값은 그대로)
    if m is not None and imm%0x1000==0 and imm not in X.SITE and imm not in X.DATA_MANUAL:return m&~0xfff
    return m
def is_bound(v):
    # 0x1300000·0x2000000 같은 범위 경계(코드/자료 구역 크기) — 주소가 아님
    return v>=0x100000 and v%0x100000==0
def run(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):print('==',os.path.basename(NEW),'크기 다름 — 건너뜀');return
    S=secs(c)
    def off(rva):
        for n,vs,va,rs,ra,ch in S:
            if va<=rva<va+max(vs,rs):return ra+rva-va
    ins=disasm(CUR)
    # 1) 짝 찾기
    pairs=[];slots=collections.Counter()
    for k,(addr,bs,mn,ops) in enumerate(ins):
        imm=None;reg=None
        if mn=='add':
            m=re.match(r'^(r[a-z0-9]+),0x([0-9a-f]+)$',ops)
            if m and m.group(1) in REGS:reg=m.group(1);imm=int(m.group(2),16)
        elif mn=='lea':
            m=re.match(r'^r[a-z0-9]+,\[(r[a-z0-9]+)\+0x([0-9a-f]+)\]$',ops)
            if m and m.group(1) in REGS:reg=m.group(1);imm=int(m.group(2),16)
        if imm is None or not(0x1000<=imm<OIMG_END) or is_bound(imm):continue
        # imm32 위치: 명령 바이트 끝 4바이트
        if len(bs)<5 or struct.unpack_from('<I',bs,len(bs)-4)[0]!=imm:continue
        for back in range(1,65):
            if k-back<0:break
            a2,b2,m2,o2=ins[k-back]
            mm=re.match(r'^(r[a-z0-9]+),QWORD PTR \[rip\+0x([0-9a-f]+)\]\s*#\s*(?:0x)?([0-9a-f]+)(?:\s+<[^>]*>)?\s*$',o2)
            if m2=='mov' and mm and mm.group(1)==reg:
                slot=int(mm.group(3),16)-IB;slots[slot]+=1;pairs.append((addr,len(bs)-4,imm,slot));break
            if o2.startswith(reg+',') or o2.startswith(reg+' ') or m2 in('call','ret','jmp'):break   # REG 가 바뀌거나 흐름이 끊김(조건 분기는 통과)
            if m2 in('pop','xchg','cmpxchg') and reg in o2:break
    rep=[];hand=[];changed=0
    if pairs:
        base_slots={sl for sl,cnt in slots.items() if cnt>=3}   # 게임 base 를 담은 전역 칸(모듈 안에 여러 개일 수 있음)
        if base_slots:
            for addr,io,imm,slot in pairs:
                if slot not in base_slots:continue
                fo=off(addr-IB)+io
                if d[fo:fo+4]!=c[fo:fo+4]:continue
                m,how=X.map_code(imm) if imm<OTEXT_END else X.map_data(imm)
                if m is None and imm>=OTEXT_END:m,how=map_data_by_refs(imm)
                if m is None:hand.append((hex(addr-IB),'base+imm 번역 못함',hex(imm),how));continue
                if m==imm:continue
                if not DRY:struct.pack_into('<I',d,fo,m)
                changed+=1;rep.append((hex(addr-IB),'base+imm',hex(imm),'→',hex(m),(how or '')[:60]))
    # 2) cmp eax,imm32 / 3) mov r32,imm32(함수 시작)
    for addr,bs,mn,ops in ins:
        m=re.match(r'^(eax),0x([0-9a-f]+)$',ops) if mn=='cmp' else None
        if m:
            imm=int(m.group(2),16)
            if not(0x10000<=imm<OTEXT_END) or imm not in OINS or len(bs)<5 or struct.unpack_from('<I',bs,len(bs)-4)[0]!=imm:continue
            fo=off(addr-IB)+len(bs)-4
            if d[fo:fo+4]!=c[fo:fo+4]:continue
            mm,how=X.map_code(imm)
            if mm is None or mm==imm:continue
            if not DRY:struct.pack_into('<I',d,fo,mm)
            changed+=1;rep.append((hex(addr-IB),'cmp eax,imm',hex(imm),'→',hex(mm),(how or '')[:60]));continue
        m=re.match(r'^(e[a-z]{2}|r\d+d),0x([0-9a-f]+)$',ops) if mn=='mov' else None
        if m:
            imm=int(m.group(2),16)
            if 0x10000<=imm<OTEXT_END and imm in OSTART and len(bs)>=5 and struct.unpack_from('<I',bs,len(bs)-4)[0]==imm:
                fo=off(addr-IB)+len(bs)-4
                if d[fo:fo+4]==c[fo:fo+4]:
                    mm,how=X.map_code(imm);hand.append((hex(addr-IB),'mov r32,함수시작 — 손으로',hex(imm),'→',hex(mm) if mm else None,(how or '')[:50]))
    if not DRY and changed:open(NEW,'wb').write(d)
    print('==',os.path.basename(NEW),'바꾼 즉시값',changed,'손으로',len(hand),'base slots',[hex(sl) for sl,cnt in slots.most_common() if cnt>=3])
    for r in rep:print(' ',*r)
    for h in hand:print('  손으로:',*h)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:run(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:print('==',rel,'오류',repr(e))

# ---- 4단계(R43 시험2 발견): 컴파일러가 자료 표의 첫 칸 값을 코드 즉시값으로 끌어올린 경우(Traits9: `mov r9d,0x4a6cf3` = 표 첫 기록의 patch_rva)
#  규칙: 같은 모듈 자료 구역에서 (payload_cur → payload_new) 번역된 u32 칸의 옛값→새값 표를 만들고, 코드의 imm32(mov r32/cmp/add 등 어떤 명령이든 마지막 4바이트)가
#        그 옛값과 같으면 같은 새값으로. (옛값은 옛 exe 범위 & ≥0x10000 만)
def hoisted(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):return
    S=secs(c);tbl={}
    for n,vs,va,rs,ra,ch in S:
        if (ch&0x20000000) or n not in('.rdata','.rodata','.data'):continue
        for k in range(0,rs-3,4):
            ov=struct.unpack_from('<I',c,ra+k)[0];nv=struct.unpack_from('<I',d,ra+k)[0]
            if ov!=nv and 0x10000<=ov<OIMG_END and 0x1000<=nv<0x2a02000:
                if ov in tbl and tbl[ov]!=nv:tbl[ov]=None   # 서로 다른 번역 → 못 씀
                elif ov not in tbl:tbl[ov]=nv
    tbl={k:v for k,v in tbl.items() if v is not None}
    if not tbl:return
    def off(rva):
        for n,vs,va,rs,ra,ch in S:
            if va<=rva<va+max(vs,rs):return ra+rva-va
    ins=disasm(CUR);rep=[];changed=0
    for addr,bs,mn,ops in ins:
        if len(bs)<5:continue
        imm=struct.unpack_from('<I',bs,len(bs)-4)[0]
        if imm not in tbl:continue
        if not re.search(r'0x%x\b'%imm,ops):continue   # 피연산자에 그 값이 즉시값으로 보여야(변위가 아니라)
        fo=off(addr-IB)+len(bs)-4
        if d[fo:fo+4]!=c[fo:fo+4]:continue
        if not DRY:struct.pack_into('<I',d,fo,tbl[imm])
        changed+=1;rep.append((hex(addr-IB),mn,ops,'→',hex(tbl[imm])))
    if not DRY and changed:open(NEW,'wb').write(d)
    if changed:
        print('==',os.path.basename(NEW),'끌어올린 표 상수',changed)
        for r in rep:print(' ',*r)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:hoisted(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:print('==',rel,'4단계 오류',repr(e))

# ---- 5단계(R43 시험2 발견): '게임 base 레지스터 + 변위' 메모리 피연산자의 변위(disp32)는 명령 끝 4바이트가 아니라 apply_new 가 못 봤다.
#  예) FPS120: `cmp BYTE PTR [rcx+0x152267],0xe8` (게임 코드 바이트를 한 바이트씩 확인), TraitHover: `cmp DWORD PTR [r8+0x807d93],0x128f89` (call 의 rel32 확인),
#      DomesticCE: `mov BYTE PTR [rax+0x59100],0xe9` (코드 덮어쓰기). base 레지스터가 base slot 에서 읽힌 것만(≤8명령 안, 사이에 덮어쓰기·분기 없음).
#  기대 바이트 비교(cmp [..],imm)는 옛 exe 바이트와 imm 이 같을 때 새 exe 바이트로 imm 도 갱신한다(rel32 가 달라지는 call 자리 등).
def disp_pass(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):return
    S=secs(c)
    def off(rva):
        for n,vs,va,rs,ra,ch in S:
            if va<=rva<va+max(vs,rs):return ra+rva-va
    ins=disasm(CUR)
    # base slot 통계(1단계와 같은 규칙, 2번 이상)
    slots=collections.Counter()
    def prov(k,reg):
        # base 레지스터 출처: 뒤로 ≤64명령 안의 `mov reg,[rip+slot]`. 사이에 reg 를 쓰는 명령·call/ret/jmp 가 있으면 포기(조건 분기 jcc 는 통과 — `cmp; jne 실패` 사슬)
        for back in range(1,65):
            if k-back<0:return None
            a2,b2,m2,o2=ins[k-back]
            mm=re.match(r'^(r[a-z0-9]+),QWORD PTR \[rip\+0x([0-9a-f]+)\]\s*#\s*(?:0x)?([0-9a-f]+)(?:\s+<[^>]*>)?\s*$',o2)
            if m2=='mov' and mm and mm.group(1)==reg:return int(mm.group(3),16)-IB
            if o2.startswith(reg+',') or o2.startswith(reg+' ') or m2 in('call','ret','jmp'):return None
            if m2 in('pop','xchg','cmpxchg') and reg in o2:return None
        return None
    cand=[]
    for k,(addr,bs,mn,ops) in enumerate(ins):
        if 'rip' in ops or len(bs)<5:continue
        m=re.search(r'\[(r[a-z0-9]+)(?:\+r[a-z0-9]+\*\d)?\+0x([0-9a-f]+)\]',ops)
        if not m or m.group(1) not in REGS:continue
        disp=int(m.group(2),16)
        if not(0x1000<=disp<OIMG_END) or is_bound(disp):continue
        # disp32 위치 찾기(명령 안에서 처음 나오는 해당 u32, 끝 4바이트 제외 가능)
        pos=None
        for p in range(1,len(bs)-3):
            if struct.unpack_from('<I',bs,p)[0]==disp:pos=p;break
        if pos is None:continue
        slot=prov(k,m.group(1))
        if slot is None:continue
        slots[slot]+=1;cand.append((k,addr,bs,mn,ops,disp,pos,slot))
    # 1단계에서 확인된 slot 도 합침
    for k,(addr,bs,mn,ops) in enumerate(ins):
        mm=re.match(r'^(r[a-z0-9]+),0x([0-9a-f]+)$',ops) if mn=='add' else None
        if mm and 0x1000<=int(mm.group(2),16)<OIMG_END:
            s2=prov(k,mm.group(1))
            if s2 is not None:slots[s2]+=1
    # base slot 검증: 그 slot 에서 나온 레지스터의 사용 중 '확실한 게임 주소' (함수 시작 imm / 옛 exe 바이트와 맞는 기대 바이트 비교 / .rdata 절대주소 범위) 가 2번 이상
    valid=collections.Counter()
    for k,addr,bs,mn,ops,disp,pos,slot in cand:
        cm=re.match(r'^(BYTE|DWORD|WORD) PTR \[.*\],0x([0-9a-f]+)$',ops)
        if mn=='cmp' and cm and disp<OTEXT_END:
            width={'BYTE':1,'WORD':2,'DWORD':4}[cm.group(1)];ob=OT[disp-ova:disp-ova+width]
            if len(ob)==width and int.from_bytes(ob,'little')==int(cm.group(2),16):valid[slot]+=1
        if disp in X.ostart or disp in OINS and mn in('lea','add'):valid[slot]+=1
    for k,(addr,bs,mn,ops) in enumerate(ins):
        mm=re.match(r'^(r[a-z0-9]+),0x([0-9a-f]+)$',ops) if mn=='add' else None
        if mm and int(mm.group(2),16) in X.ostart:
            s2=prov(k,mm.group(1))
            if s2 is not None:valid[s2]+=1
    base=set(s for s,n in valid.items() if n>=2)
    rep=[];hand=[];changed=0
    for k,addr,bs,mn,ops,disp,pos,slot in cand:
        if slot not in base:continue
        fo=off(addr-IB)+pos
        if d[fo:fo+4]!=c[fo:fo+4]:continue
        m,how=X.map_code(disp) if disp<OTEXT_END else X.map_data(disp)
        if m is None and disp>=OTEXT_END:m,how=map_data_by_refs(disp)
        m=page_fix(disp,m)
        if m is None:hand.append((hex(addr-IB),'변위 번역 못함',hex(disp),how,mn,ops));continue
        note=''
        if m!=disp:
            if not DRY:struct.pack_into('<I',d,fo,m)
            changed+=1
        # 기대 바이트 비교면 imm 도 새 exe 바이트로
        cm=re.match(r'^(BYTE|DWORD|WORD) PTR \[.*\],0x([0-9a-f]+)$',ops)
        if mn=='cmp' and cm and disp<OTEXT_END:
            width={'BYTE':1,'WORD':2,'DWORD':4}[cm.group(1)];imm=int(cm.group(2),16)
            ob=OT[disp-ova:disp-ova+width];nb=NT[m-nva:m-nva+width]
            if len(ob)==width and int.from_bytes(ob,'little')==imm:
                if nb!=ob:
                    if not DRY:d[off(addr-IB)+len(bs)-width:off(addr-IB)+len(bs)]=nb
                    changed+=1;note=' 기대 %s→%s'%(ob.hex(),nb.hex())
            else:note=' (imm 이 옛 바이트와 다름: 비교 아님?)'
        if m!=disp or note:rep.append((hex(addr-IB),mn,ops,'변위',hex(disp),'→',hex(m),(how or '')[:40]+note))
    if not DRY and changed:open(NEW,'wb').write(d)
    if rep or hand:
        print('==',os.path.basename(NEW),'변위',changed,'손으로',len(hand),'base slots',[hex(s) for s in base])
        for r in rep:print(' ',*r)
        for h in hand:print('  손으로:',*h)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:disp_pass(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:print('==',rel,'5단계 오류',repr(e))

# ---- 6단계: `mov r32,imm32` 뒤 ≤3명령 안에 `add R64,QWORD PTR [rip+slot]` (게임 base 를 나중에 더하는 꼴, DomesticCE `mov edi,0x27d10f0; add rdi,[base]`)
def movadd_pass(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):return
    S=secs(c)
    def off(rva):
        for n,vs,va,rs,ra,ch in S:
            if va<=rva<va+max(vs,rs):return ra+rva-va
    ins=disasm(CUR);R32={'eax':'rax','ecx':'rcx','edx':'rdx','ebx':'rbx','esi':'rsi','edi':'rdi','ebp':'rbp'}
    for i in range(8,16):R32['r%dd'%i]='r%d'%i
    # base slot: 1단계·5단계 검증과 같은 취지로, 이 꼴에서 imm 이 함수 시작/명령 경계/자료 범위인 slot 을 센다
    pairs=[];slots=collections.Counter()
    for k,(addr,bs,mn,ops) in enumerate(ins):
        m=re.match(r'^(e[a-z]{2}|r\d+d),0x([0-9a-f]+)$',ops) if mn=='mov' else None
        if not m or m.group(1) not in R32:continue
        imm=int(m.group(2),16)
        if not(0x10000<=imm<OIMG_END) or is_bound(imm):continue
        r64=R32[m.group(1)]
        for fw in range(1,4):
            if k+fw>=len(ins):break
            a2,b2,m2,o2=ins[k+fw]
            mm=re.match(r'^(r[a-z0-9]+),QWORD PTR \[rip\+0x([0-9a-f]+)\]\s*#\s*(?:0x)?([0-9a-f]+)(?:\s+<[^>]*>)?\s*$',o2)
            if m2=='add' and mm and mm.group(1)==r64:
                slot=int(mm.group(3),16)-IB;ok=(imm in X.ostart) or (imm in OINS) or (imm>=OTEXT_END and imm%4==0)
                slots[slot]+=(2 if ok else 1)   # 확실한 것은 2점, 그럴듯한 것은 1점
                pairs.append((addr,bs,imm,slot));break
            if o2.startswith(r64+',') or o2.startswith(m.group(1)+',') or m2 in('call','ret','jmp'):break
    base=set(s for s,n in slots.items() if n>=4);rep=[];changed=0
    for addr,bs,imm,slot in pairs:
        if slot not in base:continue
        fo=off(addr-IB)+len(bs)-4
        if d[fo:fo+4]!=c[fo:fo+4]:continue
        m,how=X.map_code(imm) if imm<OTEXT_END else X.map_data(imm)
        if m is None and imm>=OTEXT_END:m,how=map_data_by_refs(imm)
        m=page_fix(imm,m)
        if m is None:rep.append((hex(addr-IB),'번역 못함',hex(imm),how));continue
        if m==imm:continue
        if not DRY:struct.pack_into('<I',d,fo,m)
        changed+=1;rep.append((hex(addr-IB),'mov r32,imm → add [base]',hex(imm),'→',hex(m),(how or '')[:50]))
    if not DRY and changed:open(NEW,'wb').write(d)
    if rep:
        print('==',os.path.basename(NEW),'mov+add[base]',changed,'base slots',[hex(s) for s in base])
        for r in rep:print(' ',*r)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:movadd_pass(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:print('==',rel,'6단계 오류',repr(e))
