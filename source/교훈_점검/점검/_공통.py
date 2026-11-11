# 점검 도구 공통 부품. 경로는 이 폴더 위치에서 계산한다(작업 폴더를 어디에 풀어도 돌게).
import os,re,struct,hashlib,sys
HERE=os.path.dirname(os.path.abspath(__file__))
LESS=os.path.dirname(HERE)            # 교훈_점검
R=os.path.dirname(LESS)               # 작업 폴더(r41)
PKG=R+'/overlay_r43/pkg'              # 지금 낼 판의 꾸러미(판이 바뀌면 06_기준값.md 와 함께 고칠 것)
MANIFEST=PKG+'/support/manifest101.xml'
IC_DLL=PKG+'/support/payload/base/SAN14_IntegratedConvenience.dll'
def sha(p):
    h=hashlib.sha256()
    with open(p,'rb') as f:
        for b in iter(lambda:f.read(1<<20),b''):h.update(b)
    return h.hexdigest()
def tsv(name):
    """교훈_점검 안의 tsv 를 읽어 (머리줄, 줄들) 을 돌려준다. # 로 시작하는 줄과 빈 줄은 건너뜀."""
    rows=[];head=None
    for n,l in enumerate(open(LESS+'/'+name,encoding='utf-8-sig'),1):
        l=l.rstrip('\r\n')
        if not l.strip() or l.startswith('#'):continue
        c=l.split('\t')
        if head is None:head=c;continue
        rows.append((n,c))
    return head,rows
def manifest_files():
    x=open(MANIFEST,encoding='utf-8').read()
    return re.findall(r'<File target="([^"]+)" source="([^"]+)" sha256="([0-9a-f]+)" when="([^"]+)" id="(\d+)"',x)
class PE:
    def __init__(s,path):
        s.d=open(path,'rb').read();d=s.d
        pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];o=struct.unpack_from('<H',d,pe+20)[0]
        s.secs=[struct.unpack_from('<8sIIII',d,pe+24+o+40*k) for k in range(n)]
    def off(s,r):
        for n,vs,va,rs,rp in s.secs:
            if va<=r<va+max(vs,rs):return r-va+rp
        return None
class Out:
    """점검 결과 모으기. 끝에 done() 을 부르면 실패 수에 따라 종료 코드."""
    def __init__(s,title):s.fail=0;s.n=0;print('== '+title)
    def ok(s,msg):s.n+=1;print('  ok   '+msg)
    def bad(s,msg):s.n+=1;s.fail+=1;print('  FAIL '+msg)
    def note(s,msg):print('  ·    '+msg)
    def done(s):
        print('  → %s (항목 %d, 실패 %d)'%('통과' if not s.fail else '실패',s.n,s.fail));sys.exit(1 if s.fail else 0)
