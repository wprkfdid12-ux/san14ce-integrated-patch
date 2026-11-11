# R43 9차: 통합편의 시작 대기는 해독 표시만 보고(2,346곳 전부 원본 조건 없음), 그 표 재확인(0x4b93f)은 그대로 남아 있어야 함
import subprocess,sys
IC='/home/claude/r41/newpkg/payload_new/base/SAN14_IntegratedConvenience.dll'
dis=lambda a,b:subprocess.run(['objdump','-d','--start-address=%#x'%a,'--stop-address=%#x'%b,IC],capture_output=True,text=True).stdout
f=0
def check(c,m):
    global f;print(('  ok   ' if c else '  FAIL ')+m);f+=0 if c else 1
check('jmp    0x18004b603' in dis(0x18004b5c1,0x18004b5c3),'시작 대기: 해독 표시 뒤 2,346곳 비교를 건너뜀(0x4b5c1 → 0x4b603)')
w=dis(0x18004b5a0,0x18004b5c1);check('call   0x180094d70' in w and '0xffcbe8' in w,'해독 표시(exe+0xffcbe8 32바이트) 대기는 그대로')
r=dis(0x18004b93f,0x18004b980);check('0x1800b7310' in r and 'cmp    $0x92a,%rbx' in r,'그 표를 쓰는 기능용 재확인(0x4b93f)은 그대로')
print('전부 통과' if not f else '실패 %d'%f);sys.exit(1 if f else 0)
