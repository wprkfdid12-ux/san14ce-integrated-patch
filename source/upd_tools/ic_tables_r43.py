# 모듈 자료 구역의 기대 표 오프라인 검사(배치 형식 무관):
#  ① (u64 rva, u64 len) 뒤 1~3 qword 안에 모듈 안 포인터 → 그 바이트와 새 exe[rva..] 비교 (포인터 둘이면 원본/패치 중 하나 일치)
#  ② (u64 rva, u32/u64 len) 바로 뒤 인라인 바이트(len) 가 옛 exe 어딘가와 같았던 꼴 → 새 exe[rva..] 와 비교
#  ③ {u64 slot(.data 범위), u64 target(.text 범위)} 연속 2개 이상 → 새 exe [slot]==0x140000000+target
#  ④ 코드 'lea rcx,[R+V]; mov r8d,N; …call 0x1800580e0 … lea r8,[rip+BLOB]' 짧은 비교
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
MOD=sys.argv[1];d=open(MOD,'rb').read();S=P.secs(d);IB=P.IB;G=0x140000000
NTEXT_END=P.nva+len(P.NT)
def off(va):
    for nm,vs,va0,rs,ra,ch in S:
        if va0<=va-IB<va0+rs:return ra+(va-IB-va0)
def inimg(v):return IB<=v<IB+0x400000 and off(v) is not None
rec_ok=rec_bad=0;slot_ok=slot_bad=0;inl_ok=inl_bad=0;seen=set()
for nm,vs,va0,rs,ra,ch in S:
    if nm=='.text':continue
    for k in range(0,rs-16,8):
        rva,ln=struct.unpack_from('<QQ',d,ra+k)
        if not(0x1000<=rva<0x2a02000 and 1<=ln<=0x1000):continue
        nb=X.nrd(rva,ln)
        if len(nb)<ln:continue
        ptrs=[]
        for j in (2,3,4):
            if k+8*j+8>rs:break
            p=struct.unpack_from('<Q',d,ra+k+8*j)[0]
            if inimg(p):ptrs.append(p)
            elif p!=0 and j==2:break
        if ptrs:
            cands=[d[off(p):off(p)+ln] for p in ptrs]
            if any(nb==cb for cb in cands):rec_ok+=1
            else:
                o=cands[0];hit=P.NT.find(o) if len(o)>=4 else -1
                rec_bad+=1;print('기록 %x: rva %x len %d 불일치 새exe=%s 기대=%s %s'%(va0+k,rva,ln,nb[:12].hex(),o[:12].hex(),('기대바이트는 새exe %x 에 있음'%(P.nva+hit)) if hit>=0 else '기대바이트 새exe 없음'))
            continue
        # 인라인 바이트(len ≥5)
        if ln>=5 and k+16+ln<=rs:
            b=d[ra+k+16:ra+k+16+ln]
            if b.count(0)>=ln-1:continue
            if P.OT.find(b)>=0 or P.NT.find(b)>=0:
                if nb==b:inl_ok+=1
                else:
                    inl_bad+=1;hit=P.NT.find(b);print('인라인 %x: rva %x len %d 불일치 새exe=%s 기대=%s %s'%(va0+k,rva,ln,nb[:12].hex(),b[:12].hex(),('기대바이트는 새exe %x'%(P.nva+hit)) if hit>=0 else '새exe 없음'))
    # slot runs
    k=0;run=[]
    def flush(run):
        global slot_ok,slot_bad
        if len(run)<2:return
        for va,s,t in run:
            v=X.nrd(s,8)
            if len(v)<8:slot_bad+=1;print('슬롯 %x: slot %x 범위밖'%(va,s));continue
            v=struct.unpack('<Q',v)[0]
            if v==G+t:slot_ok+=1
            else:slot_bad+=1;print('슬롯 %x: [%x] 기대 %x 새exe값 %s'%(va,s,t,hex(v-G) if v>=G else hex(v)))
    while k+0x10<=rs:
        s,t=struct.unpack_from('<QQ',d,ra+k)
        if 0x1340000<=s<0x2a02000 and 0x1000<=t<NTEXT_END and s%8==0:run.append((va0+k,s,t));k+=0x10;continue
        flush(run);run=[];k+=8
    flush(run)
print(os.path.basename(MOD),'기록 OK',rec_ok,'불일치',rec_bad,'| 인라인 OK',inl_ok,'불일치',inl_bad,'| 슬롯 OK',slot_ok,'불일치',slot_bad)
ins=P.disasm(MOD);sb_ok=sb_bad=0
for i,(a,b,m,o) in enumerate(ins):
    if m!='call' or not o.startswith('0x'):continue
    V=N=None
    for j in range(i-1,max(i-8,0),-1):
        a2,b2,m2,o2=ins[j]
        mm=re.match(r'^rcx,\[(r[a-z0-9]+)\+0x([0-9a-f]+)\]$',o2) if m2=='lea' else None
        if mm and V is None:V=int(mm.group(2),16)
        mm=re.match(r'^r8d,0x([0-9a-f]+)$',o2) if m2=='mov' else None
        if mm and N is None:N=int(mm.group(1),16)
    if V is None or N is None or not(3<=N<=64) or not(0x1000<=V<0x2a02000):continue
    BLOB=None
    for j in range(i+1,min(i+10,len(ins))):
        a2,b2,m2,o2=ins[j]
        mm=re.search(r'^(r8|rdx|rcx),\[rip\+0x[0-9a-f]+\]\s*#\s*0x([0-9a-f]+)',o2) if m2=='lea' else None
        if mm:BLOB=int(mm.group(2),16);break
        if m2 in('call','ret'):break
    if BLOB is None or off(BLOB) is None:continue
    blob=d[off(BLOB):off(BLOB)+N];nb=X.nrd(V,N)
    if blob==nb:sb_ok+=1
    elif P.OT.find(blob)>=0:sb_bad+=1;print('짧은비교 %x: [game+%x] %d 기대 %s 새exe %s'%(a-IB,V,N,blob.hex(),nb.hex()))
print('짧은비교 OK',sb_ok,'불일치',sb_bad)
