# R43.28(사용자 11-11): 깃허브 새 판 파일 만들기 — 관문 도장이 있는 판만.
#   python3 make_release_r43.py <판 예: R43.28> [--base] [--repo <저장소 clone 폴더>]
#  이 작업 환경에서는 깃허브 '배포' 칸에 못 올려서(HTTP 403) 저장소 안 updates 폴더에 둔다. 설치기(Update.cs)는 updates/index.json 을 읽는다.
#  유저에게 보일 바뀐 내용: release/<판>_설명.txt (첫 줄 = 제목)
#  만드는 것(release/<판>/repo/ — 저장소에 그대로 복사):
#   updates/<판>/SAN14CE_IntegratedPatch_<판>_<게임판>.zip  기본판(11월 7일 최종판)과 다른 파일 + 이 판 전체 목록 SHA256SUMS.txt
#   updates/base/SAN14CE_IntegratedPatch_base_R43-1107_<n>of<m>.zip  (--base) 기본판 전체를 90MB 미만 조각으로(깃허브 파일 한도 100MB)
#   updates/index.json   판 목록(저장소에 있던 목록에 이 판을 더함)
#  검사: 설치기 Build 와 같은 방법으로 ① 기본판 폴더 + 판 ZIP ② 빈 폴더 + 판 ZIP + 기본판 조각 에서 새 판 폴더를 다시 만들어 지문 전부 대조.
import sys,os,zipfile,hashlib,re,shutil,tempfile,json,zlib,glob,datetime
R='/home/claude/r41';BASE=R+'/final_r43/통합패치 11월 7일 최종판';PKG=R+'/overlay_r43/pkg'
STAMP='6ad06f53';BASE_STEM='SAN14CE_IntegratedPatch_base_R43-1107';PART_MAX=90*1048576
a=sys.argv[1:];pan=a[0];want_base='--base' in a;repo=a[a.index('--repo')+1] if '--repo' in a else '/home/claude/san14ce-integrated-patch'
assert re.fullmatch(r'R\d+\.\d+',pan),pan
sys.path.insert(0,R+'/교훈_점검');import 관문
if 관문.check_stamp()!=pan:raise SystemExit('관문 도장의 판 이름과 다름')
spec=open(R+'/installer101/Spec.cs',encoding='utf-8-sig').read()
assert f'public const string Version="{pan}";' in spec,'Spec.Version 이 판 번호와 다름'
notes_path=R+'/release/'+pan+'_설명.txt'
assert os.path.exists(notes_path),'release/<판>_설명.txt(유저에게 보일 바뀐 내용, 첫 줄 제목)를 먼저 쓸 것'
OUT=R+'/release/'+pan;REPO_OUT=OUT+'/repo';shutil.rmtree(REPO_OUT,ignore_errors=True);os.makedirs(REPO_OUT+'/updates/'+pan)
def shaf(p):
    h=hashlib.sha256()
    with open(p,'rb') as f:
        for b in iter(lambda:f.read(1<<20),b''):h.update(b)
    return h.hexdigest()
def sums(p):
    d={}
    for l in open(p,encoding='utf-8-sig'):
        l=l.strip()
        if l:h,f=l.split(None,1);d[f.lstrip('*')]=h
    return d
base=sums(BASE+'/SHA256SUMS.txt');new=sums(PKG+'/SHA256SUMS.txt')
new={f:h for f,h in new.items() if not f.startswith('README_덮어쓰기_')}   # 덮어쓰기 안내문은 빼고
for f,h in base.items():assert shaf(BASE+'/'+f)==h,('기본판 파일 지문 다름',f)
changed=sorted(f for f,h in new.items() if base.get(f)!=h)
for f in changed:assert os.path.exists(PKG+'/'+f) and shaf(PKG+'/'+f)==new[f],('꾸러미에 없거나 다름',f)
pname=f'SAN14CE_IntegratedPatch_{pan}_{STAMP}.zip';zp=f'{REPO_OUT}/updates/{pan}/{pname}'
with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
    for f in changed:z.write(PKG+'/'+f,f)
    z.writestr('SHA256SUMS.txt',''.join(f'{new[f]}  {f}\n' for f in sorted(new)).encode('utf-8'))
print('판 ZIP',pname,len(changed),'개',os.path.getsize(zp)//1024,'KB')
# 기본판 조각: 파일마다 압축 크기를 재서 90MB 미만으로 묶음
bdir=REPO_OUT+'/updates/base';parts=[]
if want_base:
    os.makedirs(bdir)
    est={}
    for f in base:
        c=zlib.compressobj(6,zlib.DEFLATED,-15);n=0
        with open(BASE+'/'+f,'rb') as fh:
            for b in iter(lambda:fh.read(1<<20),b''):n+=len(c.compress(b))
        est[f]=n+len(c.flush())+200
    assert max(est.values())<PART_MAX,('한 파일이 조각보다 큼',max(est,key=est.get))
    groups=[]
    for f in sorted(base,key=lambda x:-est[x]):
        fit=[g for g in groups if g[0]+est[f]<PART_MAX]
        if fit:
            g=min(fit,key=lambda g:g[0]);g[0]+=est[f];g[1].append(f)
        else:groups.append([est[f],[f]])
    m=len(groups)
    for i,(s,fs) in enumerate(groups,1):
        p=f'{bdir}/{BASE_STEM}_{i}of{m}.zip'
        with zipfile.ZipFile(p,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
            for f in sorted(fs):z.write(BASE+'/'+f,f)
            if i==1:z.write(BASE+'/SHA256SUMS.txt','SHA256SUMS.txt')
        sz=os.path.getsize(p);assert sz<95*1048576,(p,sz);parts.append(p);print('기본판 조각',i,'/',m,len(fs),'개',sz//1048576,'MB')
# ---- 설치기 Build 흉내 ----
def rebuild(cur,patch,basezips):
    work=tempfile.mkdtemp(dir=OUT);fz=fh=fb=0
    with zipfile.ZipFile(patch) as z:
        names=set(z.namelist());lst={}
        for l in z.read('SHA256SUMS.txt').decode('utf-8').splitlines():
            l=l.strip()
            if len(l)<66:continue
            h,rel=l[:64].lower(),l[64:].strip().lstrip('*')
            assert '..' not in rel and not rel.startswith('/');lst[rel]=h
        miss=[]
        for rel,h in lst.items():
            d=os.path.join(work,rel);os.makedirs(os.path.dirname(d),exist_ok=True)
            if rel in names:open(d,'wb').write(z.read(rel));fz+=1
            elif cur and os.path.exists(os.path.join(cur,rel)) and shaf(os.path.join(cur,rel))==h:shutil.copy(os.path.join(cur,rel),d);fh+=1
            else:miss.append(rel)
    if miss:
        assert basezips,('기본판 조각 없이 빠진 파일',miss[:3])
        for bz in basezips:
            with zipfile.ZipFile(bz) as z:
                have=set(z.namelist())
                for rel in list(miss):
                    if rel in have:open(os.path.join(work,rel),'wb').write(z.read(rel));fb+=1;miss.remove(rel)
        assert not miss,('기본판 조각에도 없음',miss[:3])
    bad=[rel for rel,h in lst.items() if shaf(os.path.join(work,rel))!=h]
    shutil.rmtree(work)
    return fz,fh,fb,bad
r1=rebuild(BASE,zp,None);print('검사① 기본판 폴더 + 판 ZIP: 받은',r1[0],'복사',r1[1],'틀림',len(r1[3]));assert not r1[3] and r1[2]==0
allparts=parts or sorted(glob.glob(repo+'/updates/base/*.zip'))
if allparts:
    r2=rebuild(None,zp,allparts);print('검사② 빈 폴더 + 판 ZIP + 기본판 조각',len(allparts),'개: 받은',r2[0],'기본판',r2[2],'틀림',len(r2[3]));assert not r2[3]
# ---- 판 목록 ----
idx_path=repo+'/updates/index.json'
idx=json.load(open(idx_path,encoding='utf-8')) if os.path.exists(idx_path) else {'format':1,'releases':[],'base':[]}
notes=open(notes_path,encoding='utf-8').read().strip().splitlines()
idx['releases']=[r for r in idx['releases'] if r['tag']!=pan]
idx['releases'].append({'tag':pan,'date':datetime.date.today().isoformat(),'title':notes[0].strip(),'notes':'\n'.join(notes[1:]).strip(),'stamp':STAMP,
    'file':f'updates/{pan}/{pname}','sha256':shaf(zp),'size':os.path.getsize(zp)})
idx['releases'].sort(key=lambda r:tuple(int(x) for x in r['tag'][1:].split('.')),reverse=True)
if parts:idx['base']=[{'file':'updates/base/'+os.path.basename(p),'sha256':shaf(p),'size':os.path.getsize(p)} for p in parts]
assert idx['base'],'기본판 조각 목록이 비었음(--base 로 한 번 만들 것)'
json.dump(idx,open(REPO_OUT+'/updates/index.json','w',encoding='utf-8'),ensure_ascii=False,indent=1)
print('판 목록',len(idx['releases']),'판, 기본판 조각',len(idx['base']),'→',REPO_OUT)
