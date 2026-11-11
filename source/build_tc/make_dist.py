# 배포 ZIP 생성: 전체 6분할(1of6만 새로, 2~6은 이전과 동일) + 덮어쓰기 ZIP
import os,sys,zipfile,hashlib,io,shutil,re
rev=sys.argv[1];prev_dist=sys.argv[2];prev_rev=sys.argv[3];out=sys.argv[4]
root='/home/claude/p27/x/SAN14CE_Rollback_Patch27';top='SAN14CE_Rollback_Patch27'
os.makedirs(out,exist_ok=True)
def sha(b):return hashlib.sha256(b).hexdigest()
files=[]
for r,d,f in os.walk(root):
    for x in f: files.append(os.path.relpath(os.path.join(r,x),root).replace(os.sep,'/'))
files.sort()
big={'support/extras/21/RRPreview.rdb.bin99','support/OriginalPayload.data'}
# 이전 1of6 에서 합치기 cmd 와 파일별 해시
prev1=zipfile.ZipFile(f'{prev_dist}/SAN14CE_Patch27_{prev_rev}_전체_1of6.zip')
join_cmd=prev1.read(f'{top}/0_먼저실행_합치기.cmd');split_note=prev1.read(f'{top}/나눠받은_전체본_설명.txt').decode('utf-8-sig').replace('R3 전체본',rev+' 전체본').replace('나눠 받은 R7','나눠 받은 '+rev)
_sn=f'/home/claude/p27/build/{rev.lower()}/나눠받은_전체본_설명.txt'
if os.path.exists(_sn):split_note=open(_sn,encoding='utf-8').read()
prev_hash={n[len(top)+1:]:sha(prev1.read(n)) for n in prev1.namelist() if not n.endswith('/')}
prev_ov=zipfile.ZipFile(f'{prev_dist}/SAN14CE_Patch27_{prev_rev}_덮어쓰기_20261003.zip')
prev_ov_files={n[len(top)+1:] for n in prev_ov.namelist() if not n.endswith('/')}
def add(z,rel,data=None):
    if data is None:data=open(os.path.join(root,rel),'rb').read()
    zi=zipfile.ZipInfo(f'{top}/{rel}',date_time=(2026,10,3,20,30,0));zi.compress_type=zipfile.ZIP_DEFLATED;zi.external_attr=0o644<<16
    z.writestr(zi,data)
with zipfile.ZipFile(f'{out}/SAN14CE_Patch27_{rev}_전체_1of6.zip','w') as z:
    add(z,'0_먼저실행_합치기.cmd',join_cmd);add(z,'나눠받은_전체본_설명.txt',b'\xef\xbb\xbf'+split_note.encode('utf-8'))
    for f in files:
        if f not in big: add(z,f)
for i in range(2,7):
    shutil.copyfile(f'{prev_dist}/SAN14CE_Patch27_{prev_rev}_전체_{i}of6.zip',f'{out}/SAN14CE_Patch27_{rev}_전체_{i}of6.zip')
changed=[f for f in files if f not in big and prev_hash.get(f)!=sha(open(os.path.join(root,f),'rb').read())]
overlay=sorted(set(changed)|{f for f in prev_ov_files if f in files and not f.startswith(prev_rev+'_') and f!='옛파일_정리.cmd'})
note=open(f'/home/claude/p27/build/{rev}_덮어쓰기_설명.txt','rb').read()
with zipfile.ZipFile(f'{out}/SAN14CE_Patch27_{rev}_덮어쓰기_20261003.zip','w') as z:
    for f in overlay: add(z,f)
    add(z,f'{rev}_덮어쓰기_설명.txt',note)
    names=prev_ov.namelist();cleanup=[n for n in names if n.endswith('옛파일_정리.cmd')][0]
    newc=f'/home/claude/p27/build/{rev.lower()}/옛파일_정리.cmd'
    add(z,'옛파일_정리.cmd',open(newc,'rb').read() if os.path.exists(newc) else prev_ov.read(cleanup))
print('changed vs',prev_rev,len(changed));print('\n'.join(changed));print('overlay rows',len(overlay)+2)
