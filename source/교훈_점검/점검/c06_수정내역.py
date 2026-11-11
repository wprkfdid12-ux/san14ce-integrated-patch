# 사용자(11-10): 패치를 수정할 때마다 수정 내역을 교훈 폴더에 넣는다. 안 적고는 판을 못 낸다.
#   직전 판(자료/직전판_SHA256SUMS.txt)과 지금 꾸러미를 비교해, 바뀐 파일마다 03_수정내역.tsv 에 '이번 판' 줄이 있어야 통과.
#   이번 판 이름: 환경 변수 S14_PAN (관문.py 가 넣어 줌).
import os
from _공통 import *
o=Out('c06 바뀐 파일마다 수정 내역')
pan=os.environ.get('S14_PAN','')
def sums(p):
    d={}
    for l in open(p,encoding='utf-8-sig'):
        l=l.rstrip('\r\n')
        if l.strip() and not l.startswith('#'):h,f=l.split(None,1);d[f.lstrip('*').strip()]=h
    return d
old=sums(LESS+'/자료/직전판_SHA256SUMS.txt');new=sums(PKG+'/SHA256SUMS.txt')
prev=open(LESS+'/자료/직전판_이름.txt',encoding='utf-8').read().split()[0]
SKIP=('SHA256SUMS.txt','support/manifest101.xml','support/installer101/ManifestHash.cs')
ch=sorted(f for f,h in new.items() if old.get(f)!=h and f not in SKIP and not f.startswith('README_덮어쓰기'))
gone=sorted(f for f in old if f not in new)
if not ch and not gone:o.ok('직전 판(%s)과 같은 꾸러미 — 바뀐 파일 없음'%prev);o.done()
if not pan:o.bad('바뀐 파일이 %d개인데 이번 판 이름이 없음(관문.py --판 <이름> 으로 돌릴 것)'%len(ch));o.done()
head,rows=tsv('03_수정내역.tsv')
mine=[c for n,c in rows if c[0]==pan];names=set()
for c in mine:
    for x in c[2].split(';'):names.add(x.strip())
if not mine:o.bad('03_수정내역.tsv 에 이번 판(%s) 줄이 하나도 없음'%pan)
for f in ch+gone:
    b=f.split('/')[-1]
    if b in names or f in names:o.ok('%s — 수정 내역 있음'%b)
    else:o.bad('%s 가 바뀌었는데(직전 판 %s 대비) 수정 내역(판=%s)에 없음'%(f,prev,pan))
o.done()
