# 6차(11-08 포로 회유 '모두 제외' 버그) 재발 방지: 통합편의(IC)를 새 게임 판에 옮길 때 IC '자기 자료 칸' 주소는 절대 바뀌면 안 된다
#  (IC 파일 배치는 1.0.1판과 같음 — 바뀌어야 하는 건 게임 주소뿐).
#  ① rip 상대 참조(# 0x18…)가 1.0.1 IC 와 다르면 실패
#  ② IC 이미지 기준 레지스터(lea R,[rip-…] = 0x180000000)로 만든 [R+…+disp] 접근의 바이트가 1.0.1 IC 와 다르면 실패
#  실행: python3 test/test_ic_self_refs.py      (objdump 필요)
import re,struct,subprocess,tempfile,os,sys
R='/home/claude/r41'
OLD=R+'/newpkg/payload_cur/base/SAN14_IntegratedConvenience.dll'
NEW=R+'/newpkg/payload_new/base/SAN14_IntegratedConvenience.dll'
def secs(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];o=struct.unpack_from('<H',d,pe+20)[0]
    return [struct.unpack_from('<8sIIII',d,pe+24+o+40*k) for k in range(n)]
def text_asm(d):
    t=[s for s in secs(d) if s[0].startswith(b'.text')][0];va,rs,rp=t[2],t[3],t[4]
    with tempfile.NamedTemporaryFile(delete=False,suffix='.bin') as f:f.write(d[rp:rp+rs]);fn=f.name
    out=subprocess.run(['objdump','-D','-b','binary','-m','i386:x86-64','--adjust-vma=0x%x'%(0x180000000+va),fn],capture_output=True,text=True).stdout
    os.unlink(fn);res={}
    for l in out.splitlines():
        m=re.match(r'^\s+([0-9a-f]+):\t([0-9a-f ]+)\t(.*)$',l)
        if m:res[int(m.group(1),16)]=(m.group(2).strip(),m.group(3))
    return res
a=open(OLD,'rb').read();b=open(NEW,'rb').read()
if len(a)!=len(b):print('  FAIL 파일 크기 다름(배치가 바뀜?)');sys.exit(1)
A=text_asm(a);B=text_asm(b)
fails=[]
# ①
for k,(bb,ins) in B.items():
    o=A.get(k)
    if o and o[1]!=ins and '# 0x18' in ins:fails.append('rip 상대 %x: %s → %s'%(k,o[1],ins))
# ②
base={};age=0
for k in sorted(B):
    bb,ins=B[k];age+=1
    if ins.startswith('ret') or age>400:base={};age=0
    m=re.match(r'lea\s+-?0x[0-9a-f]+\(%rip\),%(\w+)\s+# 0x180000000$',ins)
    if m:base[m.group(1)]=k;age=0;continue
    for r in list(base):
        if re.search(r'0x[0-9a-f]+\(%'+r+r'[,)]',ins):
            o=A.get(k)
            if o and o[0]!=bb:fails.append('IC 기준 %x: %s → %s'%(k,o[1],ins))
    d=re.search(r',%(\w+)$',ins)
    if d and d.group(1) in base and not ins.startswith(('cmp','test')):del base[d.group(1)]
for f in fails:print('  FAIL',f)
print('  ok   IC 자기 자료 칸 주소가 1.0.1 IC 와 같음' if not fails else '실패 %d'%len(fails))
sys.exit(1 if fails else 0)
