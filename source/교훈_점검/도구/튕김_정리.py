# 튕김 기록(게임 폴더 SAN14_기록\SAN14_CrashLog.log)을 자리별로 묶고, 그 자리를 누가 고치는지 대조해 자료/튕김자리.tsv 를 만든다.
#   python3 교훈_점검/도구/튕김_정리.py <SAN14_CrashLog.log> [다른 모드 자리 파일(SAN14Mods\sites\*.txt) ...] [--exe 게임복호화본.exe]
# 교훈 L138(11-11): 15:27 접근 위반 exe+0x98ade — 스택에 우리 모듈 없음. 맵 확장 자리 파일(MapExpand.txt)을 보니 같은 함수(0x989e0) 안
#   0x98bbd·0x98bcd 를 맵 확장이 고침 → 맵 확장 빼고 재현 없음(15:49~16:05). 판정 순서를 기계로 돌리게 만든 도구.
# 대조: ① 자료/우리자리*.tsv(통합패치) ② 자료/다른모드자리.tsv(기색)·계절자리.tsv(계절)·AI모드자리.tsv(AI 호환판에서 도는 자리) ③ 넘겨준 sites 파일(맵 확장 등 자기 로더 모드)
#   --exe 가 있으면 튕긴 곳이 든 함수 범위(앞뒤 int3 채움으로 추정)를 찾아 '같은 함수 안' 으로 판정, 없으면 ±0x400 바이트.
# 판정·비고 칸은 사람이 적는 칸 — 다시 돌려도 그대로 둔다(같은 위치 줄을 이어 씀).
# 사용자(11-11): 다른 모드(맵 확장 등, sites 파일로 넘긴 모드)가 고치는 함수에서 난 튕김은 그 모드에서 할 일 → 표에 안 남김(몇 곳 뺐는지만 화면에 출력).
import re,sys,os,struct
HERE=os.path.dirname(os.path.abspath(__file__));LESS=os.path.dirname(HERE)
OUT=LESS+'/자료/튕김자리.tsv'
NEW_BUILD_FROM='2026-11-05'   # 게임 새 판(stamp 0x6ad06f53)이 나온 날 — 그 전 기록은 옛 판(1.0.1) 주소라 자리 대조를 안 함(06_기준값.md)
args=sys.argv[1:];exe=None
if '--exe' in args:i=args.index('--exe');exe=args[i+1];del args[i:i+2]
if not args:print(__doc__ if __doc__ else '사용: 튕김_정리.py <SAN14_CrashLog.log> [sites.txt ...] [--exe 파일]');sys.exit(1)
log=args[0];sitefiles=args[1:]
# ---- 튕김 묶기
crashes={}
cur=None
for line in open(log,encoding='utf-8',errors='replace'):
    line=line.rstrip('\r\n')
    m=re.match(r'=== (\d{4}-\d\d-\d\d [\d:]+) thread \d+ code (0x[0-9a-f]+)(?: \((.*)\))?\s*$',line)
    if m:cur={'t':m.group(1),'code':m.group(2),'kind':m.group(3) or '','loc':None,'mods':''};continue
    if cur is None:continue
    m=re.match(r'위치: (\S+)',line)
    if m:
        cur['loc']=m.group(1)
        if cur['code']!='0xe06d7363':   # C++ 예외(게임이 처리)는 튕김 아님
            k=cur['loc']+('' if cur['t']>=NEW_BUILD_FROM else ' (옛 판)');c=crashes.setdefault(k,{'n':0,'first':cur['t'],'last':cur['t'],'code':cur['code'],'kind':cur['kind'],'mods':''})
            c['n']+=1;c['last']=cur['t'];cur['c']=c
        continue
    m=re.match(r'모드 요약: (.*)',line)
    if m and cur.get('c'):cur['c']['mods']=m.group(1).strip();cur=None
# ---- 자리 표
def tsvsites(path,modcol):
    out=[]
    if not os.path.exists(path):return out
    for l in open(path,encoding='utf-8'):
        if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
        c=l.rstrip('\n').split('\t')
        try:out.append((int(c[0],16),int(c[1]),c[modcol] if len(c)>modcol else '?'))
        except ValueError:pass
    return out
sites=[];foreign=set()
for nm in ('우리자리.tsv','우리자리_추가.tsv'):sites+=[(a,l,'통합패치 '+m) for a,l,m in tsvsites(LESS+'/자료/'+nm,2)]
sites+=[(a,l,m) for a,l,m in tsvsites(LESS+'/자료/다른모드자리.tsv',2)]
sites+=[(a,l,m) for a,l,m in tsvsites(LESS+'/자료/계절자리.tsv',2)]
# AI 모드(행동교정) 자리: 호환판에서 실제로 도는 것(켜짐·늦게걸기)만 '그 함수를 고치는 모드' 로 붙임. 남의 모드지만 우리가 호환판을 내므로 표에서 빼지 않음(L142)
for l in (open(LESS+'/자료/AI모드자리.tsv',encoding='utf-8') if os.path.exists(LESS+'/자료/AI모드자리.tsv') else []):
    if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
    c=l.rstrip('\n').split('\t')
    if c[3] in ('켜짐','늦게걸기'):sites.append((int(c[0],16),int(c[1]),'AI 모드 '+c[2]))
for f in sitefiles:
    mod=os.path.basename(f)
    for l in open(f,encoding='utf-8',errors='ignore'):
        l=l.strip()
        if l.startswith('mod='):mod=l[4:];continue
        p=l.split()
        if len(p)==2:
            try:sites.append((int(p[0],16),int(p[1]),mod));foreign.add(mod)
            except ValueError:pass
# ---- 함수 범위(선택)
img=None
if exe and os.path.exists(exe):
    d=open(exe,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];osz=struct.unpack_from('<H',d,pe+20)[0];secs=[]
    for i in range(ns):
        o=pe+24+osz+40*i;vs,va,rs,ra=struct.unpack_from('<IIII',d,o+8);secs.append((va,max(vs,rs),ra))
    def rd(rva,n):
        for va,vs,ra in secs:
            if va<=rva<va+vs:return d[ra+rva-va:ra+rva-va+n]
        return b''
    img=rd
def func_range(rva):
    if not img:return (rva-0x400,rva+0x400,False)
    lo=rva
    while lo>rva-0x4000 and img(lo-2,2)!=b'\xcc\xcc':lo-=1
    hi=rva
    while hi<rva+0x4000 and img(hi,2)!=b'\xcc\xcc':hi+=1
    return (lo,hi,True)
# ---- 이전 판정 이어 쓰기
old={}
if os.path.exists(OUT):
    for l in open(OUT,encoding='utf-8'):
        if l.startswith('#') or l.startswith('위치\t') or not l.strip():continue
        c=l.rstrip('\n').split('\t')
        if len(c)>=9:old[c[0]]=(c[7],c[8])
rows=[];dropped=[]
for loc,c in sorted(crashes.items(),key=lambda x:(-x[1]['n'],x[0])):
    near=''
    m=re.match(r'SAN14CE_KO\.exe\+0x([0-9a-f]+)$',loc)
    if not m and loc.endswith(' (옛 판)'):near='옛 게임 판(1.0.1) 주소 — 지금 판과 대조 안 함'
    if m:
        r=int(m.group(1),16);lo,hi,real=func_range(r);hits={}
        for a,l,mod in sites:
            if a<hi and a+l>lo:hits.setdefault(mod,[]).append('0x%x'%a)
        if any(k in foreign for k in hits):dropped.append((loc,', '.join(k for k in hits if k in foreign)));continue
        near='; '.join('%s %s'%(k,','.join(v[:4])+(' 외 %d'%(len(v)-4) if len(v)>4 else '')) for k,v in hits.items())
        near=('[같은 함수 0x%x~0x%x] '%(lo,hi) if real else '[±0x400] ')+(near or '없음')
    jud,memo=old.get(loc,('',''))
    rows.append([loc,str(c['n']),c['first'],c['last'],c['code']+' '+c['kind'],c['mods'][:80],near,jud,memo])
with open(OUT,'w',encoding='utf-8') as f:
    f.write('# 튕김 자리별 묶음 — 만든 도구: 교훈_점검/도구/튕김_정리.py (판정·비고 칸은 사람이 적고, 다시 돌려도 남음). C++ 예외(0xe06d7363, 게임이 처리)는 뺌.\n')
    f.write('# 판정: 우리 · 우리아님(<모드>) · 의심(<모드>) · 게임 · 모름. 새 자리가 생기면 판정부터 적을 것(점검표 5 [L138]).\n')
    f.write('위치\t횟수\t처음\t마지막\t종류\t스택의 모드\t그 함수를 고치는 모드(자리)\t판정\t비고\n')
    for r in rows:f.write('\t'.join(r)+'\n')
print('튕김 자리 %d곳 → %s'%(len(rows),OUT))
for loc,m in dropped:print('  뺌(그 모드 일): %s — %s 가 같은 함수를 고침'%(loc,m))
for r in rows[:12]:print(' ',r[0],r[1]+'번',r[6][:110],'|',r[7])
