# R43 시험3: '게임 base 레지스터' 흐름 추적으로 즉시값·변위 번역 (imm_port_r43 의 빈틈 메움)
#  발견 계기: 두 번째 PC 실행 튕김 — IC 0x26b92 `lea rdx,[rsi+0x14769e8]`(rsi = 인자로 받은 게임 base) 가 번역 안 됨.
#            imm_port 는 base 출처를 `mov R,[rip+slot]` 로만 봤다. 인자(rcx→rsi 복사)·다른 레지스터 복사로 온 base 는 놓침.
#  규칙: 함수(pdata) 단위로 선형 추적. 레지스터 정의 자리(def-site)마다 그 레지스터로 만든 주소([R+V], add R,V, lea X,[R+V])의 V 를 모아
#        '확실한 옛 exe 주소'(기대 바이트 일치·함수 시작·자리_대응 키·옛 코드가 rip 로 참조하는 자료) 점수가 문턱을 넘으면 그 def-site 는 게임 base.
#        base 레지스터를 복사(mov R2,R1)한 def-site 도 base. base 로 판정된 def-site 의 모든 V(옛 exe 범위) 를 newmap 으로 번역.
#        이미 바뀐 칸은 같은 값인지 확인만(다르면 CONFLICT 보고). 못 옮기는 V 는 '손으로' 목록.
#  사용: python3 base_flow_r43.py <payload_cur 루트> <payload_new 루트> <상대경로 목록 파일> [--dry] [--verbose]
import sys,os,re,struct,collections,bisect
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_argv=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_argv
DRY='--dry' in sys.argv;VERB='--verbose' in sys.argv;ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
IB=P.IB;OIMG_END=P.OIMG_END;OTEXT_END=P.OTEXT_END;OINS=P.OINS;OSTART=P.OSTART;ova=P.ova;OT=P.OT;nva=P.nva;NT=P.NT
P._refs(0);REFS=P._refidx
SITEKEYS=set(X.SITE)|set(X.DATA_MANUAL)|set(X.MANUAL)
R64=set(P.REGS)
SUB={}
for i,n in enumerate(['rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi']):
    e='e'+n[1:];SUB[e]=n;SUB[n]=n
for n,(w,b,l) in {'rax':('ax','al','ah'),'rcx':('cx','cl','ch'),'rdx':('dx','dl','dh'),'rbx':('bx','bl','bh'),'rsp':('sp','spl',None),'rbp':('bp','bpl',None),'rsi':('si','sil',None),'rdi':('di','dil',None)}.items():
    SUB[w]=n;SUB[b]=n
    if l:SUB[l]=n
for i in range(8,16):
    for suf in('','d','w','b'):SUB['r%d%s'%(i,suf)]='r%d'%i
VOLATILE=['rax','rcx','rdx','r8','r9','r10','r11']
WRITES_FIRST={'mov','lea','add','sub','and','or','xor','imul','movzx','movsx','movsxd','movabs','pop','inc','dec','neg','not','shl','shr','sar','rol','ror','adc','sbb','bswap','cmovne','cmove','cmova','cmovb','cmovg','cmovl','cmovae','cmovbe','cmovge','cmovle','cmovs','cmovns','sete','setne','movsd','movss','cvtsi2sd','cvttsd2si','bsf','bsr','popcnt','lzcnt','tzcnt','xchg','cmpxchg','setb','seta','setg','setl','setge','setle','setae','setbe','movd','movq','mul','div','idiv','rep','lods','cdqe','cdq','cqo'}
MEM=re.compile(r'\[(r[a-z0-9]+)(?:\+(r[a-z0-9]+)\*(\d))?\+0x([0-9a-f]+)\]')
def pdata(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];opt=pe+24;ns=struct.unpack_from('<H',d,pe+6)[0];osz=struct.unpack_from('<H',d,pe+20)[0]
    rva,size=struct.unpack_from('<II',d,opt+0x70+3*8)
    S=P.secs(d)
    def off(r):
        for n,vs,va,rs,ra,ch in S:
            if va<=r<va+max(vs,rs):return ra+r-va
    fo=off(rva);out=[]
    if fo is None:return out
    for i in range(0,size,12):
        b,e,u=struct.unpack_from('<III',d,fo+i)
        if b:out.append((b,e))
    out.sort();return out
def first_reg(ops):
    m=re.match(r'^([a-z0-9]+)\b',ops)
    return SUB.get(m.group(1)) if m else None
def dest_regs(mn,ops):
    if mn in('cmp','test','push','call','jmp','ret','nop','bt') or mn.startswith('j'):
        return set(['rax','rcx','rdx','r8','r9','r10','r11']) if mn=='call' else set()
    if ops.startswith(('BYTE','WORD','DWORD','QWORD','XMMWORD','TBYTE')):return set()   # 메모리 대상
    r=first_reg(ops)
    out=set()
    if r and (mn in WRITES_FIRST or mn.startswith('cmov') or mn.startswith('set')):out.add(r)
    if mn=='xchg':
        m=re.match(r'^[a-z0-9]+,([a-z0-9]+)$',ops)
        if m and SUB.get(m.group(1)):out.add(SUB[m.group(1)])
    if mn in('mul','div','idiv'):out|={'rax','rdx'}
    if mn in('cdq','cqo'):out.add('rdx')
    if mn in('cdqe',):out.add('rax')
    if mn.startswith('rep') or mn in('movs','stos','lods','movsb','movsq','movsd','stosb','stosq','stosd'):
        if 'xmm' not in ops:out|={'rcx','rsi','rdi'}
    return out
IMG_LIMIT=[0]
def score_use(mn,ops,v,width_imm):
    # (점수, 설명)  — 자리표(자리_대응)는 모듈 자기 이미지 범위 안 값이면 보통 근거로만(장부에 IC 내부 주소가 섞여 있었음: 0x105040·0x1a3ed0·0x19e6a0)
    if v in SITEKEYS:return (3 if v>=IMG_LIMIT[0] else 2),'자리표'
    if v in OSTART:return 3,'함수시작'
    if v>=OTEXT_END and v in REFS:return 3,'자료참조'
    if mn=='cmp' and width_imm and v<OTEXT_END:
        w,imm=width_imm;ob=OT[v-ova:v-ova+w]
        # 0·0xff 비교는 구조체 칸 검사와 구별이 안 됨(FogTrial construction_capture: [rcx+0x2f440],0 은 지도 칸 배열 오프셋이었다) → 근거로 안 씀
        if imm not in(0,0xff,0xffff,0xffffffff) and len(ob)==w and int.from_bytes(ob,'little')==imm:return 3,'기대바이트일치'
    if v<OTEXT_END and v in OINS:return (2 if mn in('lea','add') else 1),'명령경계'
    if v>=OTEXT_END and v%8==0:return 1,'자료정렬'
    return 0,''
def translate(v):
    m,how=X.map_code(v) if v<OTEXT_END else X.map_data(v)
    if m is None and v>=OTEXT_END:m,how=P.map_data_by_refs(v)
    m=P.page_fix(v,m);return m,how
def run(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):print('==',os.path.basename(NEW),'크기 다름 — 건너뜀');return
    S=P.secs(c)
    def off(rva):
        for n,vs,va,rs,ra,ch in S:
            if va<=rva<va+max(vs,rs):return ra+rva-va
    ins=P.disasm(CUR);funcs=pdata(c);fb=[b for b,e in funcs]
    pe=struct.unpack_from('<I',c,0x3c)[0];opt=pe+24;IMG=struct.unpack_from('<I',c,opt+0x38)[0]
    iat_rva,iat_sz=struct.unpack_from('<II',c,opt+0x70+12*8);IMG_LIMIT[0]=IMG
    MODREFS=set()
    for addr,bs,mn,ops in ins:
        mr=re.search(r'\[rip\+0x[0-9a-f]+\]\s*#\s*(?:0x)?([0-9a-f]+)',ops)
        if mr:MODREFS.add(int(mr.group(1),16)-IB)
    def modscore(v):
        if iat_rva<=v<iat_rva+iat_sz:return 3
        if v in MODREFS:return 3
        return 0
    def fid(rva):
        i=bisect.bisect_right(fb,rva)-1
        if i>=0 and funcs[i][0]<=rva<funcs[i][1]:return i
        return -1-(rva>>8)   # pdata 밖: 256바이트 묶음을 임시 함수로
    # def-site: id → dict(reg, func, idx, score, uses[], copy_from, slot)
    defs=[];uses_by_def=collections.defaultdict(list)
    cur={};curf=None
    def newdef(reg,fi,k,copy_from=None,slot=None):
        defs.append({'reg':reg,'f':fi,'k':k,'copy':copy_from,'slot':slot,'score':0,'maxs':0,'mod':0,'base':False,'nobase':False});cur[reg]=len(defs)-1;return len(defs)-1
    for k,(addr,bs,mn,ops) in enumerate(ins):
        rva=addr-IB;fi=fid(rva)
        if fi!=curf:
            curf=fi;cur={}
            for r in R64:newdef(r,fi,-1)
        # 사용(쓰기 전에 읽기)
        imm_w=None
        cm=re.match(r'^(BYTE|WORD|DWORD|QWORD) PTR \[.*\],0x([0-9a-f]+)$',ops)
        if cm:imm_w=({'BYTE':1,'WORD':2,'DWORD':4,'QWORD':8}[cm.group(1)],int(cm.group(2),16))
        used=[]
        if 'rip' not in ops:
            m=MEM.search(ops)
            if m and m.group(1) in R64 and m.group(1)!='rsp':
                v=int(m.group(4),16);used.append((m.group(1),v,'disp'))
            m2=re.match(r'^(r[a-z0-9]+),0x([0-9a-f]+)$',ops) if mn in('add','sub','cmp') else None
            if m2 and m2.group(1) in R64:used.append((m2.group(1),int(m2.group(2),16),'imm'))
        for reg,v,kind in used:
            if not(0x1000<=v<OIMG_END) or P.is_bound(v):continue
            # 바이트 위치
            pos=None
            for p in range(1,len(bs)-3):
                if struct.unpack_from('<I',bs,p)[0]==v:pos=p;break
            if pos is None:continue
            di=cur.get(reg)
            if di is None:continue
            s,why=score_use(mn,ops,v,imm_w) if v>=0x10000 else (0,'')
            uses_by_def[di].append((k,addr,bs,mn,ops,v,pos,s,why,imm_w))
            defs[di]['score']+=s;defs[di]['maxs']=max(defs[di]['maxs'],s);defs[di]['mod']+=modscore(v)
        # 정의
        copy_from=None;slot=None
        if mn=='mov':
            mm=re.match(r'^(r[a-z0-9]+),(r[a-z0-9]+)$',ops)
            if mm and mm.group(1) in R64 and mm.group(2) in R64:copy_from=cur.get(mm.group(2))
            ms=re.match(r'^(r[a-z0-9]+),QWORD PTR \[rip\+0x([0-9a-f]+)\]\s*#\s*(?:0x)?([0-9a-f]+)',ops)
            if ms and ms.group(1) in R64:slot=int(ms.group(3),16)-IB
        if mn=='lea':
            mm=re.match(r'^(r[a-z0-9]+),\[(r[a-z0-9]+)\+0x([0-9a-f]+)\]$',ops)   # base+V → 결과도 게임 안 포인터지만 base 는 아님(변위 번역은 위에서)
        nob=False
        if mn=='lea' and (re.search(r'\[rsp[\-+]',ops) or re.search(r'#\s*(?:0x)?180000000\b',ops)):nob=True   # 스택 / 모듈 자기 base
        if mn=='mov' and re.match(r'^r[a-z0-9]+,rsp$',ops):nob=True
        for r in dest_regs(mn,ops):
            di=newdef(r,fi,k,copy_from if (mn=='mov' and copy_from is not None and r==first_reg(ops)) else None,slot if (r==first_reg(ops)) else None)
            if nob and r==first_reg(ops):defs[di]['nobase']=True
    # base 판정: 점수 → 복사 전파 → slot 전파 (고정점)
    for dd in defs:
        if dd['maxs']>=3 and dd['mod']==0 and not dd['nobase']:dd['base']=True
    # 모듈 base 복사 전파(nobase)
    ch=True
    while ch:
        ch=False
        for dd in defs:
            if not dd['nobase'] and dd['copy'] is not None and defs[dd['copy']]['nobase']:dd['nobase']=True;dd['base']=False;ch=True
    base_slots=collections.Counter()
    changed_flag=True
    while changed_flag:
        changed_flag=False
        for dd in defs:
            if dd['base'] and dd['slot'] is not None:base_slots[dd['slot']]+=1
        good_slots={s for s,n in base_slots.items() if n>=2}
        for dd in defs:
            if dd['base'] or dd['nobase'] or dd['mod']:continue
            if dd['copy'] is not None and defs[dd['copy']]['base']:dd['base']=True;changed_flag=True
            elif dd['slot'] is not None and dd['slot'] in good_slots:dd['base']=True;changed_flag=True
    # 모듈 자기 base(nobase/mod)로 쓰인 변위가 이미 바뀌어 있으면 원래대로(장부 오염으로 apply_new 가 건드린 것)
    rep=[];hand=[];conf=[];changed=0;seen=set();restored=0
    for di,dd in enumerate(defs):
        if not(dd['nobase'] or dd['mod']):continue
        for (k,addr,bs,mn,ops,v,pos,s,why,imm_w) in uses_by_def[di]:
            fo=off(addr-IB)+pos
            if d[fo:fo+4]!=c[fo:fo+4]:
                if not DRY:d[fo:fo+4]=c[fo:fo+4]
                restored+=1;changed+=1;rep.append((hex(addr-IB),mn,ops,'모듈 내부 변위 복원',hex(struct.unpack_from('<I',d,fo)[0]),'→',hex(v)))
    # 번역
    for di,dd in enumerate(defs):
        if not dd['base']:continue
        for (k,addr,bs,mn,ops,v,pos,s,why,imm_w) in uses_by_def[di]:
            if v<0x10000 and v not in SITEKEYS:continue
            key=(addr,pos)
            if key in seen:continue
            seen.add(key)
            fo=off(addr-IB)+pos
            cv=struct.unpack_from('<I',c,fo)[0];nv=struct.unpack_from('<I',d,fo)[0]
            m,how=translate(v)
            if m is None:
                if nv==cv:hand.append((hex(addr-IB),mn,ops,'번역 못함',hex(v),(how or '')[:60]))
                continue
            if nv!=cv:
                if nv!=m:conf.append((hex(addr-IB),mn,ops,'이미',hex(nv),'흐름 번역',hex(m),(how or '')[:40]))
                continue
            if m!=v:
                if not DRY:struct.pack_into('<I',d,fo,m)
                changed+=1;rep.append((hex(addr-IB),mn,ops,hex(v),'→',hex(m),why,(how or '')[:40]))
            # 기대 바이트 비교면 imm 도 새 exe 바이트로
            if mn=='cmp' and imm_w and v<OTEXT_END:
                w,imm=imm_w;ob=OT[v-ova:v-ova+w];nb=NT[m-nva:m-nva+w]
                if len(ob)==w and int.from_bytes(ob,'little')==imm and nb!=ob:
                    fo2=off(addr-IB)+len(bs)-w
                    if d[fo2:fo2+w]==c[fo2:fo2+w]:
                        if not DRY:d[fo2:fo2+w]=nb
                        changed+=1;rep.append((hex(addr-IB),'기대바이트',hex(v),ob.hex(),'→',nb.hex()))
    if not DRY and changed:open(NEW,'wb').write(d)
    nb=sum(1 for dd in defs if dd['base'])
    print('==',os.path.basename(NEW),'base def-site',nb,'바꾼 칸',changed,'(복원',restored,') 손으로',len(hand),'충돌',len(conf),'base slots',[hex(s) for s,n in base_slots.most_common(6)])
    for r in rep:print(' ',*r)
    for h in hand:print('  손으로:',*h)
    for cf in conf:print('  충돌:',*cf)
    if VERB:
        for di,dd in enumerate(defs):
            if dd['base'] and dd['k']>=0 and uses_by_def[di]:
                a=ins[dd['k']][0]-IB;print('   base@%x %s score=%d uses=%d'%(a,dd['reg'],dd['score'],len(uses_by_def[di])))
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:run(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:
            import traceback;traceback.print_exc();print('==',rel,'오류',repr(e))
