# 'lea rdx,[R+S] … ReadProcessMemory … lea rax,[R+T]; cmp [rsp+..],rax' 꼴 vtable 칸 검사: 새 exe [S] == 0x140000000+T 인지
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
IB=P.IB;MOD=sys.argv[1];ins=P.disasm(MOD);ok=0;bad=[]
for i,(a,b,m,o) in enumerate(ins):
    mm=re.match(r'^rax,\[(r[a-z0-9]+)\+0x([0-9a-f]+)\]$',o) if m=='lea' else None
    if not mm:continue
    R=mm.group(1);T=int(mm.group(2),16)
    if R in('rsp','rbp'):continue
    nxt=ins[i+1:i+3]
    if not any(x[2]=='cmp' and 'rax' in x[3] and 'rsp' in x[3] for x in nxt):continue
    S=None
    for j in range(i-1,max(i-16,0),-1):
        a2,b2,m2,o2=ins[j]
        m3=re.match(r'^rdx,\[(r[a-z0-9]+)\+0x([0-9a-f]+)\]$',o2) if m2=='lea' else None
        if m3:S=int(m3.group(2),16);R2=m3.group(1);break
    if S is None:continue
    v=X.nrd(S,8)
    if len(v)<8:bad.append((hex(a-IB),'slot',hex(S),'범위밖'));continue
    v=struct.unpack('<Q',v)[0]
    if v==0x140000000+T:ok+=1
    else:
        ov=X.ord_(S,8);ov=struct.unpack('<Q',ov)[0] if len(ov)==8 else None
        bad.append((hex(a-IB),'slot',hex(S),'기대',hex(T),'새exe값',hex(v-0x140000000) if v>=0x140000000 else hex(v),'옛exe값',hex(ov-0x140000000) if ov else None))
print(os.path.basename(MOD),'슬롯검사 OK',ok,'불일치',len(bad))
for b in bad:print(' ',*b)
