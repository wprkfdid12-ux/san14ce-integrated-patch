# 11월 7일 최종판 위 누적 덮어쓰기 ZIP 만들기(R43 11-08 정리): build_overlay_r43.py 뒤에 실행.
#  python3 make_cum_overlay.py <이름 꼬리 예: 11월8일_4차> <출력 폴더>
#  최종판 SHA256SUMS(보낸 것)와 overlay_r43/pkg/SHA256SUMS.txt 를 비교해 바뀐/새 파일 + SHA256SUMS.txt 를 담고, 담은 파일을 다시 해시로 검사.
import sys,os,zipfile,hashlib
R='/home/claude/r41'
BASE='/tmp/claude-0/fx/통합패치_R43_11월7일_최종판_1of11/SHA256SUMS.txt'
tag=sys.argv[1];out=sys.argv[2]
# 관문(교훈_점검/관문.py)을 통과한 판만 묶는다. 도장이 없거나, 도장 뒤에 꾸러미·장부가 바뀌었으면 여기서 멈춤.
sys.path.insert(0,R+'/교훈_점검');import 관문
_pan=관문.check_stamp()
if _pan!=tag:raise SystemExit('관문 도장의 판 이름(%s)과 묶으려는 이름(%s)이 다름'%(_pan,tag))
def sums(p):
    d={}
    for l in open(p,encoding='utf-8-sig'):
        l=l.rstrip('\r\n')
        if not l.strip():continue
        h,f=l.split(None,1);d[f.lstrip('*').strip()]=h
    return d
old=sums(BASE);new=sums(R+'/overlay_r43/pkg/SHA256SUMS.txt')
pick=sorted(f for f,h in new.items() if old.get(f)!=h and not f.startswith('README_덮어쓰기_'))
gone=sorted(f for f in old if f not in new and not f.startswith('README_덮어쓰기_'))
zp=os.path.join(out,'통합패치_R43_11월7일_최종판_덮어쓰기_%s_1of1.zip'%tag)
with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
    for f in pick+['SHA256SUMS.txt']:z.write(R+'/overlay_r43/pkg/'+f,f)
bad=0
with zipfile.ZipFile(zp) as z:
    for f in pick:
        if hashlib.sha256(z.read(f)).hexdigest()!=new[f]:bad+=1;print('BAD',f)
print('zip',zp,'files',len(pick)+1,'bad',bad,'removed-in-new(참고)',gone)
for f in pick:print('  ',f,new[f][:8])
sys.exit(1 if bad else 0)
