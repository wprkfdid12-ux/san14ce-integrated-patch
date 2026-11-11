# R42g2: R42g 덮어쓰기본 위에 ModSettings(22번 AI 내정 순환 설명 짧게)만 바꿈
import hashlib,os,re,shutil,zipfile
BASE='/home/claude/r41/overlay_r42g/pkg';OUT='/home/claude/r41/overlay_r42g2';PKG=OUT+'/pkg'
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(open(p,'rb').read())
shutil.rmtree(OUT,ignore_errors=True);os.makedirs(PKG+'/support/payload/base');os.makedirs(PKG+'/support/installer101')
old=shaf(BASE+'/support/payload/base/SAN14_ModSettings.dll')
shutil.copy('/home/claude/r41/build_out/SAN14_ModSettings.dll',PKG+'/support/payload/base/SAN14_ModSettings.dll');new=shaf(PKG+'/support/payload/base/SAN14_ModSettings.dll')
assert old!=new
m=open(BASE+'/support/manifest101.xml','rb').read().decode('utf-8');assert m.count(old)==1;m=m.replace(old,new)
known=re.findall(r'  <Known sha256="([0-9a-f]{64})"',m);line=f'  <Known sha256="{old}" note="R21 manifest_r42g_delivered.xml" />\n'
after=[k for k in known if k>old];pos=m.index(f'  <Known sha256="{after[0]}"') if after else m.index('</Manifest>');m=m[:pos]+line+m[pos:]
mb=m.encode('utf-8');open(PKG+'/support/manifest101.xml','wb').write(mb)
open(PKG+'/support/installer101/ManifestHash.cs','w',encoding='utf-8').write('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }\n'%sha(mb))
rd=open(BASE+'/README_KO.txt','rb').read().decode('utf-8-sig')
k=rd.index('■ R42g 추가 (22번');rd=rd[:k]+'■ R42g2: 모드 설정 창의 "AI 내정 순환(군주 반영)" 설명이 길어 설명 칸에서 위아래가 잘리던 것 — 2줄 분량으로 줄임. 동작은 그대로.\n'+rd[k:]
open(PKG+'/README_KO.txt','wb').write(b'\xef\xbb\xbf'+rd.encode('utf-8'))
sums={}
for l in open(BASE+'/SHA256SUMS.txt',encoding='utf-8'):
    l=l.rstrip('\n')
    if l.strip():h,p=l.split('  ',1);sums[p]=h
for root,_,files in os.walk(PKG):
    for fn in files:
        full=os.path.join(root,fn);rel=os.path.relpath(full,PKG).replace('\\','/');sums[rel]=shaf(full)
open(PKG+'/SHA256SUMS.txt','w',encoding='utf-8',newline='\n').write(''.join(f'{sums[p]}  {p}\n' for p in sorted(sums)))
open(PKG+'/README_덮어쓰기_R42g2.txt','wb').write(b'\xef\xbb\xbf'+('통합패치 R42g2 덮어쓰기 (22번 AI 내정 순환 설명 짧게)\r\n\r\n1. 이 ZIP 안의 폴더·파일을 "통합패치 11월2일 최종판" 폴더에 그대로 덮어쓰기.\r\n2. 설치.cmd 실행(갱신).\r\n\r\n바뀐 파일: support\\payload\\base\\SAN14_ModSettings.dll, support\\manifest101.xml, support\\installer101\\ManifestHash.cs, README_KO.txt, SHA256SUMS.txt\r\nREADME_덮어쓰기_R42g.txt 는 지워도 됨.\r\n').encode('utf-8'))
zp=OUT+'/통합패치_R42g2_덮어쓰기_설명창_1of1.zip'
with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
    for root,_,files in os.walk(PKG):
        for fn in sorted(files):full=os.path.join(root,fn);z.write(full,os.path.relpath(full,PKG))
import xml.etree.ElementTree as ET;ET.fromstring(mb)
for src,s in re.findall(r'source="([^"]+)" sha256="([0-9a-f]{64})"',m):
    p=PKG+'/'+src
    if os.path.exists(p):assert shaf(p)==s,src
print('ok',new[:16],sha(mb)[:16],zp,os.path.getsize(zp))
