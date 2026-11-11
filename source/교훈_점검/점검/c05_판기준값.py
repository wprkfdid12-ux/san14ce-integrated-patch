# 교훈 L014: 게임 판 기준값(빌드 시각·이미지 크기)을 소스마다 따로 적으면 옛 값으로 만든 모듈이 나온다.
#   겪은 일: 2번 AutoMerit(R42i~i3)을 1.0.0 값으로 만들어 1.0.1 에서 늘 '판 다름'(11-05).
# 규칙: 기준값은 src/version.h 한 곳. 옛 판 값(0x6a6d9f72·0x6a983c25·0x29ff000)은 허용 목록(옛 판 알림용) 말고는 코드에 없어야 함.
import re,glob,os
from _공통 import *
o=Out('c05 게임 판 기준값은 한 곳(version.h)')
v=open(R+'/src/version.h',encoding='utf-8',errors='ignore').read()
st=re.search(r'#define S14_GAME_STAMP (0x[0-9a-f]+)u',v).group(1);im=re.search(r'#define S14_GAME_IMAGE (0x[0-9a-f]+)u',v).group(1)
base=open(LESS+'/06_기준값.md',encoding='utf-8').read()
if st in base and im in base:o.ok('version.h %s / %s = 06_기준값.md'%(st,im))
else:o.bad('version.h(%s/%s)와 06_기준값.md 가 다름'%(st,im))
mr=open(R+'/mt/src/merit_resolver.c',encoding='utf-8',errors='ignore').read()
if st in mr and im in mr:o.ok('13번 리졸버(mt/src/merit_resolver.c)도 같은 값')
else:o.bad('13번 리졸버 mt/src/merit_resolver.c 의 판 값이 version.h 와 다름')
ALLOW={'src/notice.cpp':'옛 판 알림(kStampOld)','src/version.h':'설명 주석'}
OLD=['6a6d9f72','6a983c25','29ff000']
def code_only(s):   # 주석 빼기(대충: // 뒤와 /* */)
    s=re.sub(r'/\*.*?\*/','',s,flags=re.S);return re.sub(r'//[^\n]*','',s)
for f in sorted(glob.glob(R+'/src/*')+glob.glob(R+'/mt/src/*')):
    rel=os.path.relpath(f,R)
    if not f.endswith(('.cpp','.h','.c','.S','.s')) or rel in ALLOW:continue
    s=code_only(open(f,encoding='utf-8',errors='ignore').read()).lower()
    hit=[x for x in OLD if re.search(r'0x0*'+x+r'\b',s)]
    if hit:o.bad('%s 코드에 옛 판 값 %s — version.h 의 S14_GAME_STAMP/IMAGE 를 쓸 것'%(rel,','.join(hit)))
o.done()
