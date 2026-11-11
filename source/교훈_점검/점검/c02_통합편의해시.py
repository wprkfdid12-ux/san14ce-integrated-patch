# 교훈 L020·L021: 통합편의(IC) 파일 해시를 확인하는 모듈이 많다. 통합편의를 한 바이트라도 바꾸면 그 해시를 모르는 모듈은 오류 없이 스스로 꺼진다.
#   겪은 일: 4·11번 명승/설정 확장·3D 연결 꺼짐(A19), 4번 명승 연출 생략 꺼짐(11-09 — settings_help.cpp 가 자기 목록을 따로 가짐).
# 규칙: ① 통합편의 해시 목록은 src/common.h known_common() 한 곳만. 다른 소스가 해시 바이트를 따로 들고 있으면 실패.
#       ② 지금 꾸러미의 통합편의 해시가 known_common 과 3D 쪽 registry_bridge.h 에 들어 있어야 함.
import re,glob,os
from _공통 import *
o=Out('c02 통합편의 해시 목록은 한 곳(common.h)만')
cur=sha(IC_DLL)
def arr(h):return ','.join('0x'+h[i:i+2] for i in range(0,64,2))
ch=open(R+'/src/common.h',encoding='utf-8',errors='ignore').read()
m=re.search(r'inline U8\* known_common\(\)\{(.*?)\n\}',ch,re.S)
known=re.findall(r'\{((?:0x[0-9a-f]{2},){31}0x[0-9a-f]{2})\}',m.group(1))
if arr(cur) in known:o.ok('지금 통합편의(%s…)가 known_common 에 있음(목록 %d개)'%(cur[:8],len(known)))
else:o.bad('지금 통합편의(%s…)가 src/common.h known_common 에 없음 → 이 해시를 확인하는 모듈 전부 꺼짐'%cur[:8])
heads=[k[:39] for k in known]   # 앞 8바이트
for f in sorted(glob.glob(R+'/src/*.cpp')+glob.glob(R+'/src/*.h')+glob.glob(R+'/mt/src/*.c')+glob.glob(R+'/mt/src/*.h')):
    if f.endswith('/common.h'):continue
    s=open(f,encoding='utf-8',errors='ignore').read()
    hit=[h for h in heads if h in s]
    if hit:o.bad('%s 가 통합편의 해시를 따로 들고 있음(%d개) → known_common() 을 쓰게 고칠 것'%(os.path.relpath(f,R),len(hit)))
fb=R+'/field_src_r43/native/registry_bridge.h'
if os.path.exists(fb):
    s=open(fb,encoding='utf-8',errors='ignore').read()
    if arr(cur) in s:o.ok('3D 쪽 registry_bridge.h 에 지금 통합편의 해시 있음')
    else:o.bad('3D 쪽 field_src_r43/native/registry_bridge.h 에 지금 통합편의 해시 없음 → 11번 3D 연결 꺼짐')
o.done()
