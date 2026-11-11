# 장부(교훈·수정내역·제보·소스)가 형식에 맞는지, 교훈마다 '다시 안 나게 하는 점검'이 실제로 있는지 본다.
#   점검 칸: 자동:<c번호>(이 폴더의 점검) · 시험:<이름>(test/run_offline_all.sh 의 이름) · 포장:<도구 파일> · 손:<점검표 파일>(그 파일에 [L번호] 가 적혀 있어야 함)
import re,glob,os
from _공통 import *
o=Out('c01 장부 형식·교훈마다 점검 연결')
auto={os.path.basename(f).split('_')[0] for f in glob.glob(HERE+'/c[0-9]*.py')}
runner=open(R+'/test/run_offline_all.sh',encoding='utf-8').read()
tests=set(re.findall(r'^(?:nat|run)\s+(\S+)',runner,re.M))
def cols(name,n):
    head,rows=tsv(name)
    bad=[ln for ln,c in rows if len(c)!=n]
    if len(head)!=n or bad:o.bad('%s: 칸 수가 %d 가 아닌 줄 %s'%(name,n,bad[:5] or '머리줄'))
    return rows
L=cols('02_교훈목록.tsv',5);ids=[c[0] for n,c in L]
if len(ids)!=len(set(ids)):o.bad('교훈 번호 중복: %s'%sorted({i for i in ids if ids.count(i)>1}))
nlink=0
for ln,c in L:
    if not re.fullmatch(r'L\d{3}',c[0]):o.bad('교훈목록 %d줄: 번호 형식(L000) 아님 %r'%(ln,c[0]));continue
    if len(c)<5:continue
    for k in [x.strip() for x in c[4].split(';') if x.strip()]:
        kind,_,arg=k.partition(':')
        if kind=='자동':
            if arg not in auto:o.bad('%s: 자동 점검 %s 없음'%(c[0],arg))
        elif kind=='시험':
            if arg not in tests:o.bad('%s: 시험 %s 가 run_offline_all.sh 에 없음'%(c[0],arg))
        elif kind=='포장':
            if not os.path.exists(R+'/'+arg):o.bad('%s: 포장 도구 %s 없음'%(c[0],arg))
        elif kind=='손':
            p=LESS+'/'+arg
            if not os.path.exists(p):o.bad('%s: 점검표 %s 없음'%(c[0],arg))
            elif '['+c[0]+']' not in open(p,encoding='utf-8').read():o.bad('%s: 점검표 %s 에 [%s] 항목이 없음'%(c[0],arg,c[0]))
        else:o.bad('%s: 점검 칸 형식 모름 %r'%(c[0],k));continue
        nlink+=1
    if not c[4].strip():o.bad('%s: 점검 칸이 비어 있음 — 교훈은 반드시 점검과 묶을 것'%c[0])
if not o.fail:o.ok('교훈 %d개 전부 점검과 연결(%d곳)'%(len(L),nlink))
# 점검표에 적힌 [L번호] 가 교훈목록에 있는지
for f in sorted(glob.glob(LESS+'/점검표/*.md')):
    for t in set(re.findall(r'\[(L\d{3})\]',open(f,encoding='utf-8').read())):
        if t not in ids:o.bad('%s 가 없는 교훈 %s 를 가리킴'%(os.path.basename(f),t))
P=cols('04_제보장부.tsv',11);pid=[c[0] for n,c in P]
if len(pid)!=len(set(pid)):o.bad('제보 번호 중복')
ST={'고침','대기','확인중','보류','지켜봄','우리아님','안고침'}
for ln,c in P:
    if len(c)==11 and c[7] not in ST:o.bad('제보장부 %d줄(%s): 상태 %r 는 %s 중 하나여야 함'%(ln,c[0],c[7],'·'.join(sorted(ST))))
    if len(c)==11 and c[7]=='고침' and (not c[8].strip() or not c[9].strip()):o.bad('제보장부 %s: 고침인데 고친 판/점검 칸이 비어 있음'%c[0])
C=cols('03_수정내역.tsv',5)
cols('05_소스장부.tsv',6)
if not o.fail or True:o.note('교훈 %d · 제보 %d · 수정내역 %d줄'%(len(L),len(P),len(C)))
o.done()
