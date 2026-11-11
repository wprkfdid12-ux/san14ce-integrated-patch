# 교훈 L123: 우리 다른 모드(기색 등)가 고치는 게임 자리를 통합패치와 대조한다. 통합패치만 보던 c03 의 사각지대.
#  ① 통합패치 모듈이 고치는 자리와 겹침  ② 같은 함수 안(±0x40)  ③ 통합패치 소스가 부르는 게임 함수(fn<…>(game,0x…))의 들머리(앞 16바이트)를 그 모드가 후크
#  ④ 통합편의 확인 구간 안.  걸린 자리는 자료/다른모드_확인됨.tsv 에 확인한 내용과 근거(시험 이름·제보 번호)가 있어야 통과.
import re,glob,struct,os
from _공통 import *
o=Out('c11 우리 다른 모드 자리 × 통합패치')
f=LESS+'/자료/다른모드자리.tsv'
if not os.path.exists(f):o.bad('자료/다른모드자리.tsv 없음 — 도구/다른모드자리_만들기.py <runtime.log>');o.done()
oth=[]
for ff in (f,LESS+'/자료/계절자리.tsv'):   # 11-11: 계절(SeasonRules) 새 판 자리는 DLL 판별 표에서(도구/계절자리_만들기.py)
    if not os.path.exists(ff):o.bad(os.path.basename(ff)+' 없음');continue
    for l in open(ff,encoding='utf-8'):
        if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
        c=l.rstrip('\n').split('\t');oth.append((int(c[0],16),int(c[1]),c[2],c[4]))
ours=[]
for nm in ('우리자리.tsv','우리자리_추가.tsv'):
    for l in open(LESS+'/자료/'+nm,encoding='utf-8'):
        if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
        c=l.rstrip('\n').split('\t');ours.append((int(c[0],16),int(c[1]),c[2]))
called={}
for s in sorted(glob.glob(R+'/src/*.cpp')):
    for m in re.finditer(r'fn<[^;]*?>\(game,0x([0-9a-fA-F]+)\)',open(s,encoding='utf-8',errors='ignore').read()):called.setdefault(int(m.group(1),16),os.path.basename(s))
ic=PE(IC_DLL);d=ic.d;rng=[]
r=0xdd240;i=0
while r<0xde800:
    a,l,p1,p2=struct.unpack_from('<QQQQ',d,ic.off(r));rng.append((a,l,'통합편의 전역 확인 %d번'%i));r+=32;i+=1
r=0xde800
while r<0xdedd0:
    a,b=struct.unpack_from('<QQ',d,ic.off(r));rng.append((a,8,'통합편의 포인터 확인'));r+=16
for k in range(51):
    a,tg,det,l,op=struct.unpack_from('<5Q',d,ic.off(0xdedd0+0x28*k));rng.append((a,8 if l==8 else 5,'통합편의 후크 %d번'%k))
for k in range(28):
    a,l,p1,p2=struct.unpack_from('<QQQQ',d,ic.off(0xa3150+32*k))
    if 0<a<0x2a02000 and 0<l<0x10000:rng.append((a,l,'통합편의 6번 준비 %d번'%k))
head,rows=tsv('자료/다른모드_확인됨.tsv');ACK={int(c[0],16):c for n,c in rows}
tests=open(R+'/test/run_offline_all.sh',encoding='utf-8').read();names=set(re.findall(r'^(?:run|nat)\s+(\S+)',tests,re.M))
rep={c[0] for n,c in tsv('04_제보장부.tsv')[1]}
hitn=0
for a,l,mod,nm in oth:
    why=[]
    for s,ll,dll in ours:
        if a<s+ll and s<a+l:why.append('%s 0x%x 와 겹침'%(dll,s))
        elif abs(a-s)<0x40:why.append('%s 0x%x 와 같은 함수(±0x40)'%(dll,s))
    for c,src in called.items():
        if c<=a<c+16 or a<=c<a+l:why.append('%s 가 부르는 게임 함수 0x%x 들머리'%(src,c))
    for s,ll,x in rng:
        if a<s+ll and s<a+l:why.append(x+' 구간 안')
    if not why:continue
    hitn+=1;k=ACK.get(a)
    if not k:o.bad('%s %s 0x%x(%d) — %s → 확인하고 자료/다른모드_확인됨.tsv 에 적을 것'%(mod,nm,a,l,'; '.join(sorted(set(why)))));continue
    ev=k[4].strip() if len(k)>4 else ''
    good=(ev.startswith('시험:') and ev[3:] in names) or ev in rep
    (o.ok if good else o.bad)('%s 0x%x — %s → 확인됨(%s)%s'%(mod,a,'; '.join(sorted(set(why)))[:90],ev,'' if good else ' — 근거가 시험 목록·제보 장부에 없음'))
o.note('다른 모드 자리 %d곳 중 통합패치와 만나는 곳 %d곳. 한계: 기색(ModCore 기록)·계절(DLL 판별 표) 만 봄 — 2D·맵 확장은 자리표 없음'%(len(oth),hitn))
o.done()
