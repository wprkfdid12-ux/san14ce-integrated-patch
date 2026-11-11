# 교훈 L133(22차, P230): 20차에 셈 줄(R39C)을 본판에서 빼자 30번 보물 그림은 시작 줄이 아예 없어져, 종합 진단(Diag)이 "시작 기록 없음"으로 봤다.
#  기록을 빼거나 바꿀 때는 그 기록을 읽는 쪽(Diag)이 무엇을 시작 증거로 보는지 같이 맞춘다.
#  이 점검: Diag 가 지켜보는 모듈(src/diag.cpp 규칙표의 DLL 이름)마다, 그 모듈 소스에 시작 줄을 쓰는 곳이 있어야 한다.
#   시작 줄 = r39::start(번호, 모듈, …)  또는  R42D 꼴 "start" 줄(자동논공·내정 모듈).
#   그 줄이 확인용 기록 스위치(#if S14_DIAG_RECORDS / r39::s14_diag()) 안에만 있으면 본판에서 빠지므로 실패.
import glob,os,re
from _공통 import *
o=Out('c13 Diag 가 보는 모듈마다 본판 시작 줄 (L133)')
diag=open(R+'/src/diag.cpp',encoding='utf-8').read()
mods=sorted(set(re.findall(r'\{L"[^"]*","(SAN14_[A-Za-z0-9_]+\.dll)"',diag)))
if not mods:o.bad('src/diag.cpp 규칙표에서 모듈 이름을 못 읽음')
srcs={}
for f in glob.glob(R+'/src/*.cpp')+glob.glob(R+'/src/*.h'):
    srcs[f]=open(f,encoding='utf-8',errors='ignore').read()
def strip_diag(t):
    # #if S14_DIAG_RECORDS … #endif 구간과 s14_diag() 가 붙은 줄을 지움
    out=[];skip=0
    for l in t.split('\n'):
        s=l.strip()
        if re.match(r'#\s*if\s+S14_DIAG_RECORDS',s):skip+=1;continue
        if skip and re.match(r'#\s*endif',s):skip-=1;continue
        if skip or 's14_diag()' in l:continue
        out.append(l)
    return '\n'.join(out)
for m in mods:
    found=[]
    for f,t in srcs.items():
        b=os.path.basename(f)
        if b=='diag.cpp':continue
        t2=strip_diag(t)
        mod_const=re.search(r'MOD\s*=\s*"'+re.escape(m)+'"',t2) or re.search(r'#define\s+MOD\s+"'+re.escape(m)+'"',t2)
        lit='r39::start\\(\\s*\\d+\\s*,\\s*"'+re.escape(m)+'"'
        if re.search(lit,t2):found.append(b);continue
        if mod_const and (re.search(r'r39::start\(\s*\d+\s*,\s*MOD\b',t2) or re.search(r'\bline\(\s*"start"',t2) or re.search(r'"start"',t2)):found.append(b);continue
        if re.search(r'"R42D\\t\d+\\t'+re.escape(m)+r'\\tstart\\t"',t2):found.append(b)
    if found:o.ok('%s 시작 줄: %s'%(m,', '.join(sorted(set(found)))))
    else:o.bad('%s: Diag 가 시작 기록을 찾는데 본판 소스에 시작 줄(r39::start 또는 R42D start)이 없음 — 셈 줄만 있거나 확인용 스위치 안에만 있음'%m)
o.done()
