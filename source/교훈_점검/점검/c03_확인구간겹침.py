# 교훈 L030·L031: 우리 모듈이 고치는 게임 자리가 통합편의(IC)가 시작 때 '원래 모양인지 확인'하는 구간 안이면,
#   그 모듈은 반드시 통합편의 확인이 끝난 뒤에 고쳐야 한다. 먼저 고치면 통합편의 기능이 조용히 꺼지고, PC 속도에 따라 일부 PC 에서만 난다.
#   겪은 일: 32·29번 → 1~7번 전부(11-08), 19번 → 6번 진행 배속(11-09 제보), 15·30번 → 5번 수행·대열(위험), AI 모드 → 5번(11-09).
# 보는 표(통합편의 9차 47a62d84 기준): 전역 확인 0xdd240(174) · 포인터 확인 0xde800(93) · 후크 0xdedd0(51) · 6번 준비 0xa3150(28)·0xa34d0 · 원본 확인 0xb7310(2346)
# 우리 자리: ① 자료/우리자리.tsv(실제 게임 기록) ② src/*.cpp 의 .jump/.call(0x…) 자리
# '확인 뒤 설치' 인정: 소스에 icw::run_after_restore 가 있거나, 자료/확인뒤설치.tsv 에 근거(시험 이름)와 함께 적힌 DLL.
import re,glob,struct,os
from _공통 import *
o=Out('c03 우리 자리 × 통합편의 확인 구간 겹침')
ic=PE(IC_DLL);d=ic.d;h=sha(IC_DLL)
KNOWN_IC='47a62d84a81a66b741d9fc94ef25fe45d59180d828b9b66d8c32dec379b5b892'
if h!=KNOWN_IC:
    o.bad('통합편의 파일이 바뀜(%s…) — 이 점검의 표 위치(0xdd240 등)를 새 파일에서 다시 확인하고 KNOWN_IC 를 고칠 것'%h[:8]);o.done()
rng=[]   # (시작, 길이, 표 이름)
r=0xdd240;i=0
while r<0xde800:
    a,l,p1,p2=struct.unpack_from('<QQQQ',d,ic.off(r));rng.append((a,l,'전역 확인 %d번'%i));r+=32;i+=1
r=0xde800
while r<0xdedd0:
    a,b=struct.unpack_from('<QQ',d,ic.off(r));rng.append((a,8,'포인터 확인'));r+=16
for k in range(51):
    a,tg,det,l,op=struct.unpack_from('<5Q',d,ic.off(0xdedd0+0x28*k));rng.append((a,8 if l==8 else 5,'후크 %d번'%k))
for k in range(28):
    a,l,p1,p2=struct.unpack_from('<QQQQ',d,ic.off(0xa3150+32*k))
    if 0<a<0x2a02000 and 0<l<0x10000:rng.append((a,l,'6번 진행 배속 준비 %d번'%k))
r=0xa34d0
while True:
    a,b=struct.unpack_from('<QQ',d,ic.off(r))
    if not(0x1000<a<0x2a02000 and 0x1000<b<0x2a02000):break
    rng.append((a,8,'6번 진행 배속 포인터'));r+=16
t=ic.off(0xb7310)
for k in range(0x92a):
    a,l,_=struct.unpack_from('<QII',d,t+16*k);rng.append((a,l,'원본 확인(모델 등록 묶음)'))
assert len(rng)>2600,'표를 못 읽음'
# DLL → 소스
head,rows=tsv('05_소스장부.tsv');SRC={c[0]:c[3] for n,c in rows}
def deferred_src(dll):
    for pat in (SRC.get(dll) or '').split(';'):
        for f in glob.glob(R+'/'+pat.strip()):
            if os.path.isfile(f) and 'icw::run_after_restore' in open(f,encoding='utf-8',errors='ignore').read():return True
    return False
head,rows=tsv('자료/확인뒤설치.tsv');LATE={c[0]:c[1] for n,c in rows}
# 통합편의를 일부러 감싸는 우리 모듈(통합편의 확인을 스스로 기다린 뒤 통합편의 코드 쪽을 고침) — 게임 자리 겹침 대상 아님
sites=[]
for l in open(LESS+'/자료/우리자리.tsv',encoding='utf-8'):
    if l.startswith('#') or l.startswith('게임RVA'):continue
    c=l.rstrip('\n').split('\t');sites.append((int(c[0],16),int(c[1]),c[2],'게임 기록'))
import sys;sys.path.insert(0,LESS+'/도구');import 자리표_만들기 as ST
_add,_drop=ST.load_extra()                      # 이번 판 새 자리(게임 기록에 아직 없음, L118)
sites=[x for x in sites if (x[0],x[1]) not in _drop]+[(r_,l_,dll_,'이번 판 추가표') for r_,l_,dll_,no_,nm_ in _add]
o.note('이번 판 추가표(자료/우리자리_추가.tsv): 더함 %d, 뺌 %d'%(len(_add),len(_drop)))
MODOF={}
for n,c in tsv('05_소스장부.tsv')[1]:
    for pat in c[3].split(';'):
        for f in glob.glob(R+'/'+pat.strip()):MODOF[os.path.basename(f)]=c[0]
for f in sorted(glob.glob(R+'/src/*.cpp')):
    s=open(f,encoding='utf-8',errors='ignore').read();dll=MODOF.get(os.path.basename(f))
    if not dll:continue
    for m in re.finditer(r'\b\w+\.(?:jump|call)\(\s*0x([0-9a-fA-F]+)\s*,\s*\w+\s*,\s*(\d+)',s):
        sites.append((int(m.group(1),16),int(m.group(2)),dll,'소스 '+os.path.basename(f)))
seen={};hitn=0
for a,n,dll,how in sites:
    hit=sorted({nm for s,l,nm in rng if a<s+l and s<a+max(n,1)})
    if not hit:continue
    key=(dll,a)
    if key in seen:continue
    seen[key]=1;hitn+=1
    if deferred_src(dll):o.ok('%s 0x%x(%d) ⊂ %s — 소스가 통합편의 확인 뒤 설치'%(dll,a,n,', '.join(hit)))
    elif dll in LATE:o.ok('%s 0x%x(%d) ⊂ %s — 확인 뒤 설치(근거: %s)'%(dll,a,n,', '.join(hit),LATE[dll]))
    else:o.bad('%s 0x%x(%d바이트, %s) ⊂ 통합편의 %s — 통합편의 확인 뒤에 고치게 해야 함'%(dll,a,n,how,', '.join(hit)))
o.note('우리 자리 %d곳 중 겹침 %d곳'%(len({(x[2],x[0]) for x in sites}),hitn))
o.note('한계: ModCore 를 안 거치는 모듈(26·27·28번, 30번 FastMenu 등 바이너리)의 자리는 게임 기록에 없어 못 봄 → 소스 자립 뒤 전부 소스 검사로 들어옴')
o.done()
