# 11번 FieldLeader3D(3D 조형) 소스를 새 판 주소로 번역하고 game_guards.h 를 다시 만든다. (R43, 2026-11-05)
#  옛 이식(1.0.0→1.0.1) 의 /home/claude/src42d/port/field_port.py 규칙을 그대로 따르되, 대응은 upd_tools/newmap.py(함수대응·자리_대응·바이트 확인) 로.
#  사용: python3 field_port_r43.py <field_src(1.0.1)> <출력 field_src_r43> <새 원본 exe(해시용)> <새 IC dll>
import re,os,sys,shutil,struct,hashlib,bisect,ast,pickle,collections
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
SRC,DST,NEWEXE,NEWIC=sys.argv[1:5]
if os.path.exists(DST):shutil.rmtree(DST)
shutil.copytree(SRC,DST);N=os.path.join(DST,'native');TOOL=os.path.join(DST,'tools','build_field_poc.py')
log=[];M={}
tool=open(TOOL,encoding='utf-8').read()
ns={};exec(re.search(r'ENTRY_POINTS = \[.*?\nVTABLES \+= \[\(0x13a03a8\+0x28,0x1c6ec0\),\(0x13a03c8,0x1ca600\)\]\n',tool,re.S).group(0),ns)
ENTRY=ns['ENTRY_POINTS'];PATCHES=ns['PATCHES'];LEAVES=ns['LEAVES'];VTABLES=ns['VTABLES']
vt=open(os.path.join(DST,'tools','verify_field_poc.py'),encoding='utf-8').read()
# 범위·특수 상수는 1.0.1 tool(build_field_poc.py) 본문에서 읽는다(verify_field_poc.py 의 범위 목록은 1.0.0 값 그대로라 쓰지 않음)
RANGES=[(int(a,16),int(b,16)) for a,b in re.findall(r'\((0x[0-9a-f]+),(0x[0-9a-f]+)\)',re.search(r'for start,end in \[(.*?)\]:',tool,re.S).group(1))]
SP_TAIL=int(re.search(r'rva,size=(0x[0-9a-f]+),109',tool).group(1),16);SP_COLOR=int(re.search(r'rva,size=(0x[0-9a-f]+),0x58',tool).group(1),16)
SP_DRAW=int(re.search(r'dis=tool\.disassemble\((0x[0-9a-f]+)\)\n\s*chunk=',tool).group(1),16);SP_DRAW_END=int(re.search(r'assert chunk_start\+len\(chunk\)==(0x[0-9a-f]+)\n',tool).group(1),16)
SP_LEAF32=int(re.search(r'if rva==(0x[0-9a-f]+): code=code\[:32\]',tool).group(1),16)
cb=X.ord_(SP_COLOR,0x58);assert cb[0x52]==0xe9;SP_COLOR_TJ=(SP_COLOR+0x57+struct.unpack('<i',cb[0x53:0x57])[0])&0xffffffff
allc=set(ENTRY)|{s for s,t in PATCHES}|{t for s,t in PATCHES}|{r for r,l in LEAVES}|{t for r,t in VTABLES}|{a for a,b in RANGES}|{b for a,b in RANGES}|{SP_TAIL,SP_COLOR,SP_DRAW,SP_DRAW_END,SP_COLOR_TJ}
code_consts={v for v in allc if v<0x1340000};data_consts={v for v in allc if v>=0x1340000}|{r for r,t in VTABLES}
pc=open(os.path.join(N,'poc_contract.h'),encoding='utf-8').read()
for m in re.finditer(r'FL_\w+ = (0x[0-9a-f]+),',pc):
    v=int(m.group(1),16)
    if v in(0x464c0003,0x29ff000,0x2a02000):continue
    (code_consts if v<0x1340000 else data_consts).add(v)
for m in re.finditer(r'\{(0x[0-9a-f]+),\s*FL_',pc):v=int(m.group(1),16);(code_consts if v<0x1340000 else data_consts).add(v)
# 1.0.1 소스의 값은 이미 1.0.1 기준이므로 위 목록(옛 1.0.0 기준 코드에서 온 상수)이 지금 소스에 없을 수 있다 → 소스에 실제 나오는 값만 대상으로
# 목록(tool) 밖에서 소스가 직접 쓰는 게임 주소(1.0.1 값). 구조체 오프셋(0x7e838·0x6da98)과 IC 안 오프셋(registry_bridge 의 module+…)은 제외.
code_consts|={0x1c5528,0x1cb67b,0x376d02,   # registry_bridge leas[]: IC 가 확장표로 돌려 쓰는 게임 lea 자리 3곳
              0x406b38,0x406b3a}            # combat_ambient/poc_loader: 'cmp edx,imm32' 칸 상한(맵 확장이 넓히는 즉치) 자리
data_consts|={0x1a3e290,                    # registry_bridge: 원본 모델표(25*68)
              0x27d10e8,0x27d10f0,0x27d1120,# batch2_presentation/final_motion: 전역 표·용량
              0x13981a0}                    # final_ui: vtable 확인
alltext=''.join(open(os.path.join(N,f),encoding='utf-8',errors='replace').read() for f in os.listdir(N) if f.endswith(('.h','.cpp')) and f not in('game_guards.h','embedded_runtime.h','dll_build.h','installer_payload.h'))+tool+vt
present=set(int(h,16) for h in re.findall(r'0x([0-9a-fA-F]{4,7})\b',alltext))
# 소스에 있는데 목록에 없는 게임 범위 값도 번역 대상 후보로(보고에 따로 적음)
extra=sorted(v for v in present if (0x10000<=v<0x1340000 or 0x1340000<=v<0x2a00000) and v not in code_consts and v not in data_consts)
unres=[]
def put(v,kind):
    if kind=='code':n,how=X.map_code(v)
    else:n,how=X.map_data(v)
    if n is None:unres.append((kind,hex(v),how));return
    M[v]=n;log.append((kind,hex(v),hex(n),how))
RANGE_END={b for a,b in RANGES};RANGE_START={a for a,b in RANGES};LEAFSET={r for r,l in LEAVES}
def map_range(a,b):
    # 범위 전체(명령 단위 마스크)를 새 함수 몸 안의 명령 시작마다 대 본다. 유일하면 그 자리.
    f=X.func_of(a);L=b-a;key=X.imask('old',a,L)
    if len(key)!=L:return None,None,f'옛 범위가 명령 경계에 안 맞음(마스크 길이 {len(key)})'
    if f:
        ns,how=X.map_code(f[0])
        if ns is None and f[0] in X._fm:ns=X._fm[f[0]];how='함수대응(머리 바이트 다름: '+('바뀐 함수' if f[0] in X.DIFF else '점프표 RVA 등')+')'
        if ns is not None:
            ne=X.nend.get(ns,ns+0x2000);ks=X._akeys('new');i0=bisect.bisect_left(ks,ns);i1=bisect.bisect_left(ks,ne)
            hits=[ks[j]-ns for j in range(i0,i1) if X.imask('new',ks[j],L)==key]
            if len(hits)==1:return ns+hits[0],ns+hits[0]+L,f'범위 전체 일치(함수 {how} 오프셋 0x{a-f[0]:x}→0x{hits[0]:x})'
            if len(hits)>1:
                k=min(hits,key=lambda k:abs(k-(a-f[0])));return ns+k,ns+k+L,f'범위 전체 일치 {len(hits)}곳 중 옛 오프셋에 가장 가까운 것(0x{a-f[0]:x}→0x{k:x})'
    na,how=X.map_code(a)
    if na is None:return None,None,how
    if X.imask('new',na,L)==key:return na,na+L,f'시작 {how}+같은 길이 바이트 일치'
    return None,None,f'시작 {how} 0x{na:x} 이나 범위 바이트 다름'
for a,b in RANGES:
    na,nb,how=map_range(a,b)
    if na is None:unres.append(('range',hex(a)+'..'+hex(b),how));continue
    M[a]=na;M[b]=nb;log.append(('range',hex(a)+'..'+hex(b),hex(na)+'..'+hex(nb),how))
def map_leaf(o,size):
    n,how=X.map_code(o)
    if n is not None and X.imask('new',n,size)==X.imask('old',o,size):return n,how
    ob=X.ord_(o,size);i=bisect.bisect_left(X.ostart,o);c=collections.Counter(X._fm[X.ostart[j]]-X.ostart[j] for j in range(max(0,i-6),min(len(X.ostart),i+6)) if X.ostart[j] in X._fm)
    for sh,_ in c.most_common():
        cands=[k for k in range(o+sh-0x400,o+sh+0x400,16) if X.imask('new',k,size)==X.imask('old',o,size) and X.nrd(k-1,1) in(b'\xcc',b'\xc3')]
        if len(cands)==1:return cands[0],f'잎 함수(pdata 없음) 이웃 이동량 +0x{sh:x} 근처 16정렬 바이트 일치 0x{cands[0]:x}'
    return None,'잎 함수 못 찾음('+how+')'
for o,size in LEAVES:
    if o in M:continue
    n,how=map_leaf(o,size)
    if n is None:unres.append(('leaf',hex(o),how))
    else:M[o]=n;log.append(('leaf',hex(o),hex(n),how))
for v in sorted(code_consts):
    if v in present and v not in RANGE_END and v not in RANGE_START and v not in LEAFSET and v not in M:put(v,'code')
for v in sorted(data_consts):
    if v in present:put(v,'data')
# vtable 칸: base+offset 꼴은 base 대응 + offset (base 가 M 에 있으면)
for r,t in VTABLES:
    if r in M:continue
    for base_ in(0x13a0570,0x16c9e80,0x16914f8,0x13a0358,0x13a05c0,0x16c9f80,0x1691578,0x13a03a8,0x13a05d0,0x13a0618,0x13a03e8,0x1398ce8,0x1398a58,0x13a03c8):
        if base_ in M and 0<=r-base_<0x500:M[r]=M[base_]+(r-base_);log.append(('vtable-slot',hex(r),hex(M[r]),f'base 0x{base_:x}+0x{r-base_:x}'));break
    else:
        n,how=X.map_data(r)
        if n is None:unres.append(('vtable-slot',hex(r),how))
        else:M[r]=n;log.append(('vtable-slot',hex(r),hex(n),how))
# ---- 검증: PATCHES 호출 자리 / VTABLES 칸 내용
bad=[]
for s,t in PATCHES:
    if s in(0x13a03c8,0x13a03e8,0x1398ce8,0x1398a58):continue
    if s not in M or t not in M:bad.append(('patch 미대응',hex(s),hex(t)));continue
    b=X.nrd(M[s],5)
    if len(b)<5 or b[0]!=0xe8:bad.append(('patch 자리 e8 아님',hex(s),hex(M[s]),b.hex()));continue
    tgt=(M[s]+5+struct.unpack('<i',b[1:5])[0])&0xffffffff
    if tgt!=M[t]:
        # 새 함수 안에서 M[t] 를 부르는 e8 을 찾아 자리 바로잡기
        f=X.func_of(s);found=None
        if f and f[0] in X._fm or (f and f[0] in X.SITE):
            nst=X._fm.get(f[0]) or X.SITE.get(f[0]);body=X.nrd(nst,X.nend.get(nst,nst+0x2000)-nst)
            for k in range(len(body)-5):
                if body[k]==0xe8 and (nst+k+5+struct.unpack('<i',body[k+1:k+5])[0])&0xffffffff==M[t]:found=nst+k;break
        if found:log.append(('patch-fix',hex(s),hex(M[s]),'→',hex(found)));M[s]=found
        else:bad.append(('patch 대상 다름',hex(s),hex(M[s]),hex(t),hex(M[t]),hex(tgt)))
for r,t in VTABLES:
    if r not in M or t not in M:bad.append(('vtable 미대응',hex(r),hex(t)));continue
    b=X.nrd(M[r],8);v=struct.unpack('<Q',b)[0]-X.IB if len(b)==8 else -1
    if v!=M[t]:
        # 칸 내용이 가리키는 함수가 옛 대상과 같은 머리면 대상 대응을 칸 내용으로
        if 0<v<0x1341000 and X.imask('new',v,24)==X.imask('old',t,24):log.append(('vtable-target-fix',hex(t),hex(M[t]),'→',hex(v)));M[t]=v
        else:bad.append(('vtable 내용 다름',hex(r),hex(M[r]),hex(t),hex(M[t]),hex(v)))
print('미대응',unres);print('검증실패',bad);print('범위',len(RANGES),'특수',hex(SP_TAIL),hex(SP_COLOR),hex(SP_DRAW),hex(SP_DRAW_END),hex(SP_LEAF32),hex(SP_COLOR_TJ))
if unres and not os.environ.get('FORCE'):sys.exit('미대응이 있어 멈춤')
# ---- 소스 치환
HEX=re.compile(r'0x([0-9a-fA-F]{4,7})\b');unlisted=collections.Counter()
def xl(s,fn):
    def rep(m):
        v=int(m.group(1),16)
        if v in M and M[v]!=v:log.append(('src',fn,hex(v),hex(M[v])));return '0x%x'%M[v]
        if v in M:return m.group(0)
        if (0x10000<=v<0x1340000 or 0x1340000<=v<0x2a00000) and fn not in('game_guards.h','embedded_runtime.h','dll_build.h','installer_payload.h'):unlisted[(fn,hex(v))]+=1
        return m.group(0)
    return HEX.sub(rep,s)
for fn in os.listdir(N):
    p=os.path.join(N,fn)
    if not os.path.isfile(p) or fn in('game_guards.h','embedded_runtime.h','dll_build.h','installer_payload.h') or not fn.endswith(('.h','.cpp')):continue
    s=open(p,encoding='utf-8').read();s2=xl(s,fn)
    if fn=='poc_contract.h':s2=s2.replace('FL_IMAGE_SIZE = 0x29ff000','FL_IMAGE_SIZE = 0x2a02000')
    if s2!=s:open(p,'w',encoding='utf-8').write(s2)
NEWSHA=hashlib.sha256(open(NEWEXE,'rb').read()).hexdigest();OLDSHA='87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a'
t2=xl(tool,'build_field_poc.py').replace(OLDSHA,NEWSHA);open(TOOL,'w',encoding='utf-8').write(t2)
v2=xl(vt,'verify_field_poc.py').replace(OLDSHA,NEWSHA);open(os.path.join(DST,'tools','verify_field_poc.py'),'w',encoding='utf-8').write(v2)
pl=os.path.join(N,'poc_loader.cpp');ps=open(pl,encoding='utf-8').read();assert OLDSHA in ps or NEWSHA in ps;ps=ps.replace(OLDSHA,NEWSHA).replace('rva=00406b3a','rva=%08x'%M[0x406b3a]);open(pl,'w',encoding='utf-8').write(ps)
# ---- game_guards.h 재생성 (새 exe 명령 경계 = find_new 의 asm.pkl)
asm=pickle.load(open(X.NEWVER+'/대응/new.dec.exe.asm.pkl','rb'));nks=sorted(asm)
def ins_len(a):
    i=bisect.bisect_left(nks,a);assert i<len(nks) and nks[i]==a,('명령 경계 아님',hex(a));return nks[i+1]-a
def cut_chunks(start,end):
    out=[];a=start;chunk=b'';cs=a
    while a<end:
        L=ins_len(a);ins=X.nrd(a,L)
        if chunk and len(chunk)+len(ins)>64:out.append((cs,chunk));chunk=b''
        if not chunk:cs=a
        chunk+=ins;a+=L
    assert a==end,('범위 끝이 명령 경계 아님',hex(start),hex(end),hex(a))
    if chunk:out.append((cs,chunk))
    return out
guards=[]
for rva in ENTRY:
    nr=M[rva];code=b'';a=nr
    while len(code)<20:L=ins_len(a);code+=X.nrd(a,L);a+=L
    assert 20<=len(code)<=64;guards.append((nr,code))
nr=M[SP_TAIL];code=X.nrd(nr,109);assert code[-7:]==bytes.fromhex('48ffa018040000'),('0xdbfe90 꼬리',code[-7:].hex());guards.append((nr,code[:32]))
for rva,size in LEAVES:
    nr=M[rva];code=X.nrd(nr,size);assert code[-1]==0xc3,('leaf ret',hex(rva),hex(nr),code.hex())
    if rva==SP_LEAF32:code=code[:32]
    guards.append((nr,code))
nr=M[SP_COLOR];code=X.nrd(nr,0x58);assert code[-1]==0xc3 and code[0x52]==0xe9,('color leaf',code.hex())
tj=(nr+0x52+5+struct.unpack('<i',code[0x53:0x57])[0])&0xffffffff;assert tj==M[SP_COLOR_TJ],(hex(tj),hex(M[SP_COLOR_TJ]))
guards.extend([(nr,code[:0x3d]),(nr+0x3d,code[0x3d:])])
guards.extend(cut_chunks(M[SP_DRAW],M[SP_DRAW_END]))
for a,b in RANGES:guards.extend(cut_chunks(M[a],M[b]))
for site,target in PATCHES:
    if site in(0x13a03c8,0x13a03e8,0x1398ce8,0x1398a58):continue
    guards.append((M[site],X.nrd(M[site],5)))
nvt=[(M[r],M[t]) for r,t in VTABLES]
hdr=['#pragma once','#include <stdint.h>','typedef struct FLGuard { uint32_t rva; unsigned length; unsigned char bytes[64]; } FLGuard;',
 'static const unsigned char fl_expected_exe_sha256[32]={'+','.join('0x%02x'%x for x in bytes.fromhex(NEWSHA))+'};','static const FLGuard fl_guards[]={']
hdr+=['{0x%x,%d,{%s}},'%(r,len(c),','.join('0x%02x'%x for x in c)) for r,c in guards]
hdr+=['};','#define FL_GUARD_COUNT (sizeof(fl_guards)/sizeof(fl_guards[0]))','typedef struct FLVtableGuard { uint32_t rva,target; } FLVtableGuard;','static const FLVtableGuard fl_vtable_guards[]={']
hdr+=['{0x%x,0x%x},'%(r,t) for r,t in nvt];hdr+=['};','#define FL_VTABLE_GUARD_COUNT (sizeof(fl_vtable_guards)/sizeof(fl_vtable_guards[0]))']
open(os.path.join(N,'game_guards.h'),'w',encoding='utf-8').write('\n'.join(hdr)+'\n')
# ---- registry_bridge.h: 새 IC 해시 + IC+0x4940 확인 바이트
rb=os.path.join(N,'registry_bridge.h');s=open(rb,encoding='utf-8-sig').read();ic=open(NEWIC,'rb').read();h=hashlib.sha256(ic).digest();f=0x4940-0x1000+0x400;code=ic[f:f+51]
assert code.startswith(bytes.fromhex('48895c2408574883ec208bda488bf9e8')) and code.endswith(bytes.fromhex('48ffe0'))
s=re.sub(r'static const unsigned char code\[\]=\{[^}]*\};','static const unsigned char code[]={%s};'%','.join('0x%02x'%b for b in code),s,count=1)
arr=','.join('0x%02x'%b for b in h)
if 'module_sha256_r43' not in s:
    s=s.replace('inline bool module_known(','// R43 IC (2026-11-05 update port)\nstatic const unsigned char module_sha256_r43[32]={%s};\ninline bool module_known('%arr,1)
    s=s.replace('{return !memcmp(hash,module_sha256_r40,32)||','{return !memcmp(hash,module_sha256_r43,32)||!memcmp(hash,module_sha256_r40,32)||',1)
else:s=re.sub(r'module_sha256_r43\[32\]=\{[^}]*\}','module_sha256_r43[32]={%s}'%arr,s)
open(rb,'w',encoding='utf-8').write(s)
# ---- 보고
old_guards=open(os.path.join(SRC,'native','game_guards.h'),encoding='utf-8').read()
rep=[f'guards 옛 {old_guards.count(chr(10)+"{0x")} → 새 {len(guards)}, vtables {len(nvt)}',f'대응 {len(M)}개, 미대응 {len(unres)}개, 검증 실패 {len(bad)}개, 목록 밖 게임 범위 값 {len(unlisted)}종']
rep+=['[미대응] '+' | '.join(map(str,unres))] if unres else []
rep+=['[검증 실패] '+' | '.join(map(str,bad))] if bad else []
rep+=['[목록 밖(안 바꿈)] '+' | '.join(f'{k[0]}:{k[1]}x{v}' for k,v in sorted(unlisted.items()))] if unlisted else []
c=collections.Counter(l[0] for l in log);rep.append('기록 '+str(dict(c)))
open(os.path.join(DST,'번역_보고.txt'),'w',encoding='utf-8').write('\n'.join(rep)+'\n\n'+'\n'.join('\t'.join(map(str,l)) for l in log)+'\n')
print('\n'.join(rep))
