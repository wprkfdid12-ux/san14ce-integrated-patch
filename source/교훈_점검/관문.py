#!/usr/bin/env python3
# 통합패치 관문 — 이걸 통과하지 못하면 판을 못 낸다. (사용자 11-10: 교훈 폴더는 기록장이 아니라 필터)
#
#   python3 교훈_점검/관문.py --판 11월10일_10차          전부 돌리고, 통과하면 도장(관문_기록/도장.json)을 찍는다
#   python3 교훈_점검/관문.py --보기                      지금 상태만 본다(도장 안 찍음, 시험도 돌림)
#   python3 교훈_점검/관문.py --빨리                      자동 점검만(전체 시험 생략, 도장 안 찍음) — 작업 중 수시로
#   python3 교훈_점검/관문.py --넘김 <판>                 그 판을 실제로 PC 정식 폴더에 넣은 뒤: 지금 꾸러미를 '직전 판'으로 기록
#
# 순서: ① 자동 점검(점검/c*.py) ② 오프라인 전체 시험(test/run_offline_all.sh) ③ 도장
# 포장 도구(make_cum_overlay.py)는 도장이 없거나, 도장 찍은 뒤 꾸러미가 바뀌었으면 묶지 않는다.
import sys,os,subprocess,glob,json,hashlib,time,shutil
LESS=os.path.dirname(os.path.abspath(__file__));R=os.path.dirname(LESS)
PKG=R+'/overlay_r43/pkg';SUMS=PKG+'/SHA256SUMS.txt';STAMP=LESS+'/관문_기록/도장.json'
def sha(p):return hashlib.sha256(open(p,'rb').read()).hexdigest()
def state_hash():
    """꾸러미 목록 + 장부 + 점검 도구를 한데 묶은 해시. 도장 찍은 뒤 이 중 하나라도 바뀌면 도장이 무효."""
    h=hashlib.sha256();h.update(open(SUMS,'rb').read())
    for f in sorted(glob.glob(LESS+'/0*.tsv')+glob.glob(LESS+'/점검/*.py')+glob.glob(LESS+'/자료/*')+[R+'/test/run_offline_all.sh']):
        if os.path.isfile(f):h.update(os.path.basename(f).encode());h.update(open(f,'rb').read())
    return h.hexdigest()
def check_stamp():
    """포장 도구가 부른다. 도장이 유효하면 판 이름, 아니면 SystemExit."""
    if not os.path.exists(STAMP):raise SystemExit('관문 도장이 없음 → python3 교훈_점검/관문.py --판 <이름> 을 먼저 통과시킬 것')
    j=json.load(open(STAMP,encoding='utf-8'))
    if j.get('상태해시')!=state_hash():raise SystemExit('관문 도장을 찍은 뒤 꾸러미·장부·점검 도구가 바뀜 → 관문을 다시 통과시킬 것')
    return j['판']
def main():
    a=sys.argv[1:]
    if '--넘김' in a:
        pan=a[a.index('--넘김')+1];check_stamp()
        shutil.copy2(SUMS,LESS+'/자료/직전판_SHA256SUMS.txt');shutil.copy2(LESS+'/05_소스장부.tsv',LESS+'/자료/직전판_소스장부.tsv');open(LESS+'/자료/직전판_이름.txt','w',encoding='utf-8').write(pan+'\n')
        print('직전 판 = %s 로 기록(다음 판의 수정 내역 비교 기준)'%pan);return 0
    pan=a[a.index('--판')+1] if '--판' in a else '';quick='--빨리' in a;look='--보기' in a or quick or not pan
    env=dict(os.environ,S14_PAN=pan,PYTHONDONTWRITEBYTECODE='1')
    res=[];log=[]
    print('■ 관문%s — %s\n'%(' (보기만)' if look else ' 판='+pan,time.strftime('%Y-%m-%d %H:%M')))
    for f in sorted(glob.glob(LESS+'/점검/c[0-9]*.py')):
        p=subprocess.run([sys.executable,f],cwd=LESS+'/점검',env=env,capture_output=True,text=True)
        out=p.stdout+(p.stderr if p.returncode else '');print(out.rstrip()+'\n');log.append(out);res.append((os.path.basename(f)[:-3],p.returncode==0))
    if not quick:
        print('== 오프라인 전체 시험(test/run_offline_all.sh)')
        p=subprocess.run(['bash',R+'/test/run_offline_all.sh'],capture_output=True,text=True)
        print(p.stdout.rstrip()+'\n');log.append(p.stdout);res.append(('오프라인 전체 시험',p.returncode==0))
    bad=[n for n,ok in res if not ok]
    print('■ 결과: %d개 중 실패 %d'%(len(res),len(bad)))
    for n,ok in res:print('   %s %s'%('통과' if ok else '실패',n))
    os.makedirs(LESS+'/관문_기록',exist_ok=True)
    name=(pan or '보기')+time.strftime('_%Y%m%d_%H%M')
    open(LESS+'/관문_기록/%s.txt'%name,'w',encoding='utf-8').write('\n'.join(log)+'\n결과: '+('통과' if not bad else '실패 '+', '.join(bad))+'\n')
    if bad:
        if os.path.exists(STAMP):os.remove(STAMP)
        print('\n판을 낼 수 없음 — 위 실패를 고친 뒤 다시.');return 1
    if look:print('\n전부 통과(보기만 — 도장은 --판 <이름> 으로 전체를 돌려야 찍힘)');return 0
    json.dump({'판':pan,'시각':time.strftime('%Y-%m-%d %H:%M:%S'),'상태해시':state_hash(),'꾸러미목록해시':sha(SUMS),'점검':[n for n,ok in res]},
              open(STAMP,'w',encoding='utf-8'),ensure_ascii=False,indent=1)
    print('\n전부 통과 — 도장 찍음(%s). 이제 포장 도구를 돌릴 수 있음.'%pan);return 0
if __name__=='__main__':sys.exit(main())
