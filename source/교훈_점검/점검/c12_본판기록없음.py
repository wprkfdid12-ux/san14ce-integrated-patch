# 교훈 L132(사용자 11-11): 정식 폴더에 넣는 판 = 본판. 본판에는 확인용 기록이 없어야 한다.
#  확인용 기록 = R39C 셈 줄(r39::counter) + 달마다 쓰는 R42D 요약(world·census·pmonth·month·orders·audit·idle·ICGuard orders).
#  우리 소스로 만든 DLL(05_소스장부 '소스' 줄)을 꾸러미에서 열어, 그 기록 글자가 남아 있으면 실패.
#  확인판이 필요하면 -DS14_DIAG_RECORDS=1 로 따로 빌드하고 정식 폴더에는 넣지 않는다(시험 묶음은 이 값으로 컴파일).
#  읽기 도구(Diag·Notice)는 기록을 '읽는' 쪽이라 글자가 있어도 됨 → 뺌. 소스 없는 모듈은 우리가 못 고침 → 안 봄.
import glob,os
from _공통 import *
o=Out('c12 본판에 확인용 기록 없음 (L132)')
# 확인판 표시(r39_record.h, S14_DIAG_RECORDS=1 일 때만 들어감). 기록 글자 자체는 컴파일러가 명령으로 풀어 써서 글자로 안 남을 수 있어 표시로 본다.
MARK=[b'S14_DIAG_RECORDS_ON']
READERS={'SAN14_Diag.dll','SAN14_Notice.dll'}
src=set()
for l in open(LESS+'/05_소스장부.tsv',encoding='utf-8'):
    c=l.rstrip('\n').split('\t')
    if len(c)>2 and c[2]=='소스' and c[0].lower().endswith('.dll'):src.add(c[0])
seen=0
for f in sorted(glob.glob(PKG+'/support/payload/**/*.dll',recursive=True)):
    b=os.path.basename(f)
    if b not in src or b in READERS:continue
    d=open(f,'rb').read();seen+=1
    hit=[m.decode('ascii','replace').replace('\t','\\t') for m in MARK if m in d]
    if hit:o.bad('%s 에 확인용 기록 글자 남음: %s — S14_DIAG_RECORDS 로 막거나 r39::s14_diag() 로 감쌀 것'%(os.path.relpath(f,PKG),', '.join(hit)))
import re
for sh in [R+'/build_all.sh',R+'/mt/build_new.sh']:
    if os.path.exists(sh) and re.search(r'S14_DIAG_RECORDS\s*=\s*1',open(sh,encoding='utf-8',errors='ignore').read()):o.bad('%s 가 S14_DIAG_RECORDS=1 로 빌드함 — 본판 빌드에서는 빼야 함'%os.path.basename(sh))
if not seen:o.bad('소스 모듈 DLL 을 하나도 못 찾음(꾸러미 경로·소스장부 확인)')
else:o.ok('우리 소스 DLL %d개 확인'%seen)
o.done()
