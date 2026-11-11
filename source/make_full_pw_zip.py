# 통합패치 전체 압축본(비밀번호 123) 만들기 — 사용자 11-08 18:15 지시: 덮어쓰기 판을 만들면 정식 폴더에 덮어쓴 뒤 전체 압축본도 만들어 줄 것, 파일명에 '비번123'.
#  python3 make_full_pw_zip.py <차수 이름 예: 4차> <출력 폴더> [조각 크기 MB, 기본 380]
#  ① 11월 7일 최종판(11조각 푼 것) + overlay_r43/pkg 의 바뀐 파일 → 한 폴더  ② 모든 파일을 새 SHA256SUMS 와 대조
#  ③ zip -P 123 (윈도 탐색기로 열리는 방식)으로 압축, 조각마다 따로 풀리는 ZIP(1ofN…)  ④ 조각을 다시 풀어 해시 재검사
#  이름: '통합패치 11월 8일 <차수> 비번123_1ofN.zip' (조각 하나면 _1of1)
import sys,os,shutil,hashlib,subprocess,glob,zipfile,struct
def set_utf8(zp):   # 이 환경 zip 은 한글 이름을 UTF-8 로 쓰지만 표시(bit 11)를 안 붙임 → 윈도에서 깨짐. 두 머리글에 표시만 붙임(암호·CRC 무관)
    b=bytearray(open(zp,'rb').read())
    e=b.rfind(b'PK\x05\x06');n=struct.unpack_from('<H',b,e+10)[0];p=struct.unpack_from('<I',b,e+16)[0]
    for _ in range(n):
        assert b[p:p+4]==b'PK\x01\x02'
        struct.pack_into('<H',b,p+8,struct.unpack_from('<H',b,p+8)[0]|0x800)
        lo=struct.unpack_from('<I',b,p+42)[0];assert b[lo:lo+4]==b'PK\x03\x04'
        struct.pack_into('<H',b,lo+6,struct.unpack_from('<H',b,lo+6)[0]|0x800)
        nl,xl,cl=struct.unpack_from('<HHH',b,p+28);p+=46+nl+xl+cl
    open(zp,'wb').write(b)
R='/home/claude/r41';FX='/tmp/claude-0/fx'
tag=sys.argv[1];out=sys.argv[2];cap=int(sys.argv[3] if len(sys.argv)>3 else 380)*1024*1024
date='11월 8일'
def sums(p):
    d={}
    for l in open(p,encoding='utf-8-sig'):
        l=l.strip()
        if l:h,f=l.split(None,1);d[f.lstrip('*').strip()]=h
    return d
new={k:v for k,v in sums(R+'/overlay_r43/pkg/SHA256SUMS.txt').items() if not k.startswith('README_덮어쓰기_')}   # 덮어쓰기 안내문은 정식 폴더에도 안 넣음
work=os.path.join(out,'_full');shutil.rmtree(work,ignore_errors=True);os.makedirs(work)
for part in sorted(glob.glob(FX+'/통합패치_R43_11월7일_최종판_*of11')):
    for dp,dn,fn in os.walk(part):
        for f in fn:
            src=os.path.join(dp,f);rel=os.path.relpath(src,part);dst=os.path.join(work,rel)
            os.makedirs(os.path.dirname(dst),exist_ok=True);shutil.copy2(src,dst)
for rel in new:   # 바뀐/새 파일
    p=os.path.join(R,'overlay_r43/pkg',rel)
    if os.path.exists(p):
        dst=os.path.join(work,rel);os.makedirs(os.path.dirname(dst),exist_ok=True);shutil.copy2(p,dst)
shutil.copy2(R+'/overlay_r43/pkg/SHA256SUMS.txt',os.path.join(work,'SHA256SUMS.txt'))
bad=[];files=[]
for rel,h in new.items():
    p=os.path.join(work,rel)
    if not os.path.exists(p):bad.append('없음 '+rel);continue
    if hashlib.sha256(open(p,'rb').read()).hexdigest()!=h:bad.append('다름 '+rel)
    files.append(rel)
extra=[os.path.relpath(os.path.join(dp,f),work) for dp,dn,fn in os.walk(work) for f in fn]
extra=[x for x in extra if x not in new and x!='SHA256SUMS.txt']
print('파일',len(files),'불일치',len(bad),bad[:5],'목록 밖',extra[:10])
if bad:sys.exit(1)
# 조각 나누기(큰 파일부터, 크기 합 기준)
allf=sorted(files+['SHA256SUMS.txt']+extra,key=lambda r:-os.path.getsize(os.path.join(work,r)))
parts=[]
for r in allf:
    s=os.path.getsize(os.path.join(work,r))
    for p in parts:
        if p[0]+s<=cap:p[0]+=s;p[1].append(r);break
    else:parts.append([s,[r]])
# 설치.cmd·README·SHA256SUMS·support/installer101·manifest 는 1번 조각에 모음(1번만 풀어도 설치기가 뜨고 빠진 조각을 알려 줌)
core=[r for r in allf if r in('설치.cmd','README_KO.txt','SHA256SUMS.txt') or r.startswith('support/installer101/') or r=='support/manifest101.xml']
for p in parts:p[1]=[r for r in p[1] if r not in core]
parts[0][1]=core+parts[0][1];parts=[p for p in parts if p[1]]
N=len(parts);names=[]
for i,(s,lst) in enumerate(parts,1):
    zp=os.path.join(out,'통합패치 %s %s 비번123_%dof%d.zip'%(date,tag,i,N))
    if os.path.exists(zp):os.remove(zp)
    lf=os.path.join(out,'_list%d.txt'%i);open(lf,'w',encoding='utf-8').write('\n'.join(lst)+'\n')
    subprocess.check_call(['zip','-q','-P','123','-X',zp,'-@'],cwd=work,stdin=open(lf,'rb'));os.remove(lf);set_utf8(zp);names.append(zp)
# 재검사: 비밀번호로 풀어 해시 대조
seen=0
for zp in names:
    with zipfile.ZipFile(zp) as z:
        z.setpassword(b'123')
        for n in z.namelist():
            if n in new and hashlib.sha256(z.read(n)).hexdigest()!=new[n]:print('BAD',zp,n);sys.exit(1)
            seen+=1
print('조각',N,'담은 파일',seen,'(전체 %d)'%len(allf))
for zp in names:print('  ',os.path.basename(zp),os.path.getsize(zp))
shutil.rmtree(work)
