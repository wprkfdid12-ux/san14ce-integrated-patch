# 정식판 뒤(11-08 갤 제보 '32번 켜면 먹통'): 우리 모듈이 고치는 게임 자리가 통합편의 '원본 확인' 표(2,346곳) 안이면
#  그 모듈은 반드시 통합편의 확인이 끝난 뒤 고쳐야 함(icw::run_after_restore). 새 모듈·새 자리가 이 규칙을 어기면 실패.
#  실행: python3 test/test_ic_restore_overlap.py
import re,glob,struct,sys
R='/home/claude/r41'
d=open(R+'/newpkg/payload_new/base/SAN14_IntegratedConvenience.dll','rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0];opt=pe+24;nsec=struct.unpack_from('<H',d,pe+6)[0];osz=struct.unpack_from('<H',d,pe+20)[0]
secs=[struct.unpack_from('<8sIIII',d,opt+osz+40*k) for k in range(nsec)]
def off(r):
    for n,vs,va,rs,rp in secs:
        if va<=r<va+max(vs,rs):return r-va+rp
t=off(0xb7310);ents=[struct.unpack_from('<QII',d,t+16*i) for i in range(0x92a)]
assert ents[0][:2]==(0x1f473,3),'IC 표 위치가 다름(통합편의 판 바뀜?)'
fails=0;seen=0
for f in sorted(glob.glob(R+'/src/*.cpp')):
    s=open(f,encoding='utf-8',errors='ignore').read()
    for m in re.finditer(r'\b\w+\.(jump|call)\(\s*0x([0-9a-f]+)\s*,\s*\w+\s*,\s*(\d+)',s):
        a=int(m.group(2),16);n=int(m.group(3))
        hit=[o for o,l,e in ents if a<o+l and o<a+max(n,5)]
        if not hit:continue
        seen+=1
        ok='icw::run_after_restore' in s
        print(('  ok   ' if ok else '  FAIL ')+'%s 0x%x (%d바이트) → 통합편의 확인 자리 %s%s'%(f.split('/')[-1],a,n,','.join(hex(x) for x in hit),'' if ok else ' — 통합편의 확인 뒤 고치게 해야 함'))
        fails+=0 if ok else 1
print('겹치는 자리 %d곳, 규칙 위반 %d'%(seen,fails));print('전부 통과' if not fails else '실패');sys.exit(1 if fails else 0)
