# 교훈 L110(11-10 내 실수): 게임 기록 머리줄의 판 글자는 새 판에서도 옛 값(6a983c25)으로 찍힌다. 그 글자로 새 판 기록을 골라
#   다른 모더용 호환표 01 에 옛 판 주소 209줄이 섞였고, 15번이 통합편의 확인 구간과 겹친다는 잘못된 결론까지 나왔다.
# 규칙: 게임 기록에서 자리를 뽑는 도구는 교훈_점검/도구/자리표_만들기.py 하나만 쓰고, 어떤 도구도 'timestamp <판>' 글자로 판을 고르지 않는다.
import re,glob,os
from _공통 import *
o=Out('c10 게임 기록의 판 구분(자리표)')
h=open(LESS+'/자료/우리자리.tsv',encoding='utf-8').readline()
if '새 판 표식' in h:o.ok('자료/우리자리.tsv: '+h.strip('# \n')[:80])
else:o.bad('자료/우리자리.tsv 가 자리표_만들기.py 로 만든 것이 아님')
for f in sorted(glob.glob(R+'/kit_tools/*.py')+glob.glob(R+'/upd_tools/*.py')+glob.glob(R+'/ai_compat/**/*.py',recursive=True)):
    s=open(f,encoding='utf-8',errors='ignore').read()
    if 'runtime.log' in s and re.search(r"timestamp [0-9a-f]{8}",s):o.bad('%s 가 기록의 판 글자(timestamp …)로 기록을 고름 → 자리표_만들기.py 의 load() 를 쓸 것'%os.path.relpath(f,R))
if o.fail==0:o.ok('판 글자로 기록을 고르는 도구 없음')
o.done()
