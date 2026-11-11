# R43 정식판(11월 7일 최종판) 전체 묶음: 11월 5일 최종판 + R43 덮어쓰기(overlay_r43/pkg) → 완성 폴더 + 분할 ZIP
# 파일 목록 = overlay_r43/pkg/SHA256SUMS.txt (11월5일 목록 + 바뀐 것). 덮어쓰기 안내 README 는 뺌.
import os,shutil,hashlib,zipfile,zlib
R='/home/claude/r41';OV=R+'/overlay_r43/pkg'
SRC=[OV,R+'/overlay_r42i4/pkg',R+'/overlay_r42h/pkg',R+'/overlay_r42g2/pkg',R+'/overlay_r42g/pkg',
     '/mnt/user-data/uploads/통합패치/통합패치 11월 5일 최종판','/mnt/user-data/uploads/통합패치/통합패치 11월2일 최종판']
NAME='통합패치 11월 7일 최종판';OUT=R+'/final_r43';PKG=OUT+'/'+NAME;DIST=OUT+'/dist';TAG='통합패치_R43_11월7일_최종판'
def shaf(p):
    h=hashlib.sha256()
    with open(p,'rb') as f:
        for b in iter(lambda:f.read(1<<20),b''):h.update(b)
    return h.hexdigest()
shutil.rmtree(OUT,ignore_errors=True);os.makedirs(PKG);os.makedirs(DIST)
want={}
for l in open(OV+'/SHA256SUMS.txt',encoding='utf-8'):
    l=l.rstrip('\n')
    if l:h,p=l.split('  ',1);want[p]=h
for p in [p for p in want if p.startswith('README_덮어쓰기_')]:del want[p]
miss=[];
for p,h in sorted(want.items()):
    for d in SRC:
        f=d+'/'+p
        if os.path.exists(f) and shaf(f)==h:
            t=PKG+'/'+p;os.makedirs(os.path.dirname(t),exist_ok=True);shutil.copy(f,t);break
    else:miss.append(p)
assert not miss,miss
allfiles=sorted(want)
open(PKG+'/SHA256SUMS.txt','w',encoding='utf-8',newline='\n').write(''.join(f'{want[p]}  {p}\n' for p in allfiles))
allfiles.append('SHA256SUMS.txt')
# 분할 ZIP: R42c 와 같은 규칙(공통 런타임·로더·data·작은 파일은 1번, 10번 모델 자료는 뒤쪽, 각 55MB 이하)
sizes={f:len(zlib.compress(open(PKG+'/'+f,'rb').read(),6)) for f in allfiles}
must=lambda f:f.startswith(('support/payload/base/','support/payload/loaders/','data/'))
model=lambda f:f.startswith('support/payload/f10/0002/')
small=[f for f in allfiles if sizes[f]<200000 or must(f)]
big=sorted([f for f in allfiles if f not in small],key=lambda f:(model(f),-sizes[f]))
LIMIT=55*1024*1024;parts=[list(small)];cur=sum(sizes[f] for f in small)
for i,f in enumerate(big):
    if (cur+sizes[f]>LIMIT or (model(f) and i>0 and not model(big[i-1]))) and parts[-1]:parts.append([]);cur=0
    parts[-1].append(f);cur+=sizes[f]
n=len(parts)
for i,files in enumerate(parts,1):
    zp=f'{DIST}/{TAG}_{i}of{n}.zip'
    with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for f in files:z.write(PKG+'/'+f,f,compress_type=zipfile.ZIP_STORED if '.rdb.bin' in f else zipfile.ZIP_DEFLATED)
    print('zip',os.path.basename(zp),os.path.getsize(zp)//1048576,'MB',len(files),'files')
print('files',len(allfiles))
