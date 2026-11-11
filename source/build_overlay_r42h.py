# R42h: R42g2 위에 — 22번 플레이어 내정 자동을 우리 소스 모듈(SAN14_DomesticPlayer.dll)로.
#  옛 DomesticCE(바이너리)가 큰 맵에서 자기 상태 찾기 실패로 안 돌던 것 대체 + 토글 OFF/ON 에도 안 죽음.
#  바뀜: base/SAN14_ModSettings.dll(22번 모든내정·최소인구를 우리 모듈로 라우팅), 새 f22/SAN14_DomesticPlayer.dll + ini.
import hashlib,os,re,shutil,zipfile
BASE='/home/claude/r41/overlay_r42g2/pkg';OUT='/home/claude/r41/overlay_r42h';PKG=OUT+'/pkg'
BO='/home/claude/r41/build_out'
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(open(p,'rb').read())
shutil.rmtree(OUT,ignore_errors=True)
os.makedirs(PKG+'/support/payload/base');os.makedirs(PKG+'/support/payload/f22/SAN14Mods/modules');os.makedirs(PKG+'/support/installer101')
# 1) ModSettings 교체
old_ms=shaf(BASE+'/support/payload/base/SAN14_ModSettings.dll')
shutil.copy(BO+'/SAN14_ModSettings.dll',PKG+'/support/payload/base/SAN14_ModSettings.dll');new_ms=shaf(PKG+'/support/payload/base/SAN14_ModSettings.dll')
assert old_ms!=new_ms
# 2) 새 모듈 + ini
shutil.copy(BO+'/SAN14_DomesticPlayer.dll',PKG+'/support/payload/f22/SAN14_DomesticPlayer.dll')
dp_sha=shaf(PKG+'/support/payload/f22/SAN14_DomesticPlayer.dll')
ini_rel='support/payload/f22/SAN14Mods/modules/DomesticPlayer.ini'
open(PKG+'/'+ini_rel,'wb').write(b'[DomesticPlayer]\r\nAllDomestic=1\r\nPopulationFloor=0\r\n')
dpi_sha=shaf(PKG+'/'+ini_rel)
# 3) manifest: ModSettings 해시 교체 + DomesticPlayer 2줄 추가(22번) + 옛 ModSettings Known
m=open(BASE+'/support/manifest101.xml','rb').read().decode('utf-8')
assert m.count(old_ms)==1;m=m.replace(old_ms,new_ms)
addlines=(f'  <File target="SAN14_DomesticPlayer.dll" source="support/payload/f22/SAN14_DomesticPlayer.dll" sha256="{dp_sha}" when="22" id="22" />\n'
          f'  <File target="SAN14Mods/modules/DomesticPlayer.ini" source="support/payload/f22/SAN14Mods/modules/DomesticPlayer.ini" sha256="{dpi_sha}" when="22" id="22" />\n')
anchor=re.search(r'  <File target="SAN14Mods/modules/DomesticAI.ini"[^\n]*\n',m)
m=m[:anchor.end()]+addlines+m[anchor.end():]
known=re.findall(r'  <Known sha256="([0-9a-f]{64})"',m);kline=f'  <Known sha256="{old_ms}" note="R42g2 ModSettings" />\n'
after=[k for k in known if k>old_ms];pos=m.index(f'  <Known sha256="{after[0]}"') if after else m.index('</Manifest>');m=m[:pos]+kline+m[pos:]
mb=m.encode('utf-8');open(PKG+'/support/manifest101.xml','wb').write(mb)
open(PKG+'/support/installer101/ManifestHash.cs','w',encoding='utf-8').write('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }\n'%sha(mb))
# 4) README
rd=open(BASE+'/README_KO.txt','rb').read().decode('utf-8-sig')
note=('■ R42h: 22번 "모든 내정 자동"·"최소 인구 1만"을 통합패치 자체 모듈(SAN14_DomesticPlayer.dll)로 다시 만듦.\n'
      '    - 큰 맵(맵 확장 등)에서 옛 내정 처리가 어느 순간부터 안 돌던 것, 모드 설정에서 껐다 켜면 안 돌던 것 해결.\n'
      '    - 지역 표를 포인터로 안전하게 읽어, 다음 게임 버전에서도 덜 깨지게. AI 내정 순환(군주 반영)은 그대로.\n'
      '    - 최소 인구 1만은 첫 실행 로그로 자리 확인 뒤 켬(지금은 끔). AI 명령서 99·저돌은 아직 옛 처리.\n')
rd=note+rd
open(PKG+'/README_KO.txt','wb').write(b'\xef\xbb\xbf'+rd.encode('utf-8'))
# 5) SHA256SUMS
sums={}
for l in open(BASE+'/SHA256SUMS.txt',encoding='utf-8'):
    l=l.rstrip('\n')
    if l.strip():h,p=l.split('  ',1);sums[p]=h
for root,_,files in os.walk(PKG):
    for fn in files:
        full=os.path.join(root,fn);rel=os.path.relpath(full,PKG).replace('\\','/');sums[rel]=shaf(full)
open(PKG+'/SHA256SUMS.txt','w',encoding='utf-8',newline='\n').write(''.join(f'{sums[p]}  {p}\n' for p in sorted(sums)))
open(PKG+'/README_덮어쓰기_R42h.txt','wb').write(b'\xef\xbb\xbf'+('통합패치 R42h 덮어쓰기 (22번 플레이어 내정 자동 새 모듈)\r\n\r\n'
 '1. 이 ZIP 안의 폴더·파일을 통합패치 최종판 폴더에 그대로 덮어쓰기.\r\n'
 '2. 설치.cmd(또는 설치기) 실행 → 22번이 켜져 있으면 새 모듈이 게임 폴더에 깔림.\r\n\r\n'
 '바뀐/새 파일: support\\payload\\base\\SAN14_ModSettings.dll, support\\payload\\f22\\SAN14_DomesticPlayer.dll,\r\n'
 '  support\\payload\\f22\\SAN14Mods\\modules\\DomesticPlayer.ini, support\\manifest101.xml, support\\installer101\\ManifestHash.cs\r\n\r\n'
 '확인: 게임 한 번 돌린 뒤 SAN14_기록\\적용확인.log 에 "R42D 22 SAN14_DomesticPlayer.dll start ... OK" 와\r\n'
 '  "audit"·"pmonth" 줄이 생기면 정상. (DomesticCE: ACTIVE 가 안 떠도 이제 이 모듈이 대신 처리)\r\n').encode('utf-8'))
# 6) ZIP
zp=OUT+'/통합패치_R42h_덮어쓰기_플레이어내정_1of1.zip'
with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
    for root,_,files in os.walk(PKG):
        for fn in sorted(files):full=os.path.join(root,fn);z.write(full,os.path.relpath(full,PKG))
# 검증
import xml.etree.ElementTree as ET;ET.fromstring(mb)
for src,s in re.findall(r'source="([^"]+)" sha256="([0-9a-f]{64})"',m):
    p=PKG+'/'+src
    if os.path.exists(p):assert shaf(p)==s,src
print('ModSettings',new_ms[:16],'DomesticPlayer',dp_sha[:16],'manifest',sha(mb)[:16])
print('zip',zp,os.path.getsize(zp))
