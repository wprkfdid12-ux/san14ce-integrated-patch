# 패키지 확인값 목록 3개 재생성: PACKAGE_HASHES.json → SHA256SUMS.txt → support/Patch27PackageFiles.xml
import os,json,re,hashlib,collections,sys
root=sys.argv[1];os.chdir(root)
def sha(p):return hashlib.sha256(open(p,'rb').read()).hexdigest()
allf=[]
for r,d,f in os.walk('.'):
    for x in f: allf.append(os.path.relpath(os.path.join(r,x),'.').replace(os.sep,'/'))
allf.sort()
ph=collections.OrderedDict((p,sha(p)) for p in allf if p not in('PACKAGE_HASHES.json','SHA256SUMS.txt','support/Patch27PackageFiles.xml'))
open('PACKAGE_HASHES.json','w',encoding='utf-8').write(json.dumps(ph,ensure_ascii=False,indent=2)+'\n')
lines=[f'{sha(p)}  {p}' for p in allf if p not in('SHA256SUMS.txt','support/Patch27PackageFiles.xml')]
open('SHA256SUMS.txt','w',encoding='utf-8').write('\n'.join(lines)+'\n')
x='support/Patch27PackageFiles.xml';s=open(x,encoding='utf-8').read()
head=s[:s.index('  <File ')];tail=s[s.rindex('/>')+2:]
old=re.findall(r'  <File path="([^"]+)" sha256="[0-9a-f]+" />\n',s)
present=[p for p in allf if p!=x]
order=[p for p in old if p in present]+[p for p in present if p not in old]
rows=''.join(f'  <File path="{p}" sha256="{sha(p)}" />\n' for p in order)
open(x,'w',encoding='utf-8',newline='').write(head+rows.rstrip('\n')+tail)
print('files',len(allf),'hashes',len(ph),'sums',len(lines),'manifest rows',len(order),'new',[p for p in present if p not in old],'gone',[p for p in old if p not in present])
