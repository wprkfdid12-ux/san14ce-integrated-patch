# 화면에 보이는 글자에 옛 판 이름이 남아 있으면 실패(11-09 사용자: 모드 설정 창 아래 '통합패치 R39 모드 설정').
import re,glob,os
from _공통 import *
o=Out('c08 화면 글자에 옛 판 이름 없음')
cur=re.search(r'통합패치 판 번호\s*[:：]\s*R(\d+)',open(LESS+'/06_기준값.md',encoding='utf-8').read()).group(1)
SKIP=('scene_test.cpp','scene_test_launcher.cpp','patch_tester.cpp')   # 시험 도구(본판에 안 들어감)
for f in sorted(glob.glob(R+'/src/*.cpp')+glob.glob(R+'/src/*.h')+glob.glob(R+'/installer101/*.ps1')+glob.glob(R+'/installer101/App.cs')):
    if os.path.basename(f) in SKIP:continue
    s=open(f,encoding='utf-8',errors='ignore').read()
    for m in re.finditer(r'L"((?:[^"\\]|\\.)*)"',s):
        t=m.group(1)
        for k in re.finditer(r'통합패치\s*R(\d+)',t):
            if k.group(1)!=cur:
                line=s.count('\n',0,m.start())+1
                o.bad('%s:%d 화면 글자에 옛 판 이름 "통합패치 R%s" (지금 R%s): %s'%(os.path.relpath(f,R),line,k.group(1),cur,t[:60]))
if not o.fail:o.ok('옛 판 이름 없음(지금 R%s)'%cur)
o.done()
