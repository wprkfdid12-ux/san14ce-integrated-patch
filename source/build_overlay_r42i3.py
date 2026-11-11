# R42i3(R42i2 대신, 22번 플레이어 없음 처리·자동논공 판 확인 고침): 정식판(11월2일 최종판 = R42g) 위에 덮어쓰는 '누적' 판 — R42g2·R42h·R42i2 바뀐 것 전부 포함(앞 덮어쓰기 안 깔았어도 이것만으로 됨).
#  R42i 바뀜: (1) 2번 자동논공 새 엔진 SAN14_AutoMerit.dll + AutoPersonnelRepairMerit.ini (2) base/SAN14_ICGuard.dll(IC 논공 실행 자리에서 새 엔진 부름)
#            (3) 22번 DomesticPlayer.ini 에 빠져 있던 [Mod] 칸 추가(R42h 는 이 칸이 없어 모듈이 안 올라왔음)
#  검사: XML·해시·누적 파일 + '모듈 DLL 마다 [Mod] File= 등록 ini 가 같은 항목에 있는지'(R42h 같은 누락 재발 방지)
import hashlib,os,re,shutil,zipfile,subprocess
FINAL='/mnt/user-data/uploads/통합패치/통합패치 11월2일 최종판'
PREV=['/home/claude/r41/overlay_r42h/pkg','/home/claude/r41/overlay_r42g2/pkg','/home/claude/r41/overlay_r42g/pkg']
OUT='/home/claude/r41/overlay_r42i3';PKG=OUT+'/pkg';NEW=OUT+'/new';BO='/home/claude/r41/build_out'
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(open(p,'rb').read())
shutil.rmtree(OUT,ignore_errors=True);os.makedirs(PKG);os.makedirs(NEW)
def put_new(rel,data):
    p=NEW+'/'+rel;os.makedirs(os.path.dirname(p),exist_ok=True);open(p,'wb').write(data);return sha(data)
def locate(rel):
    for d in [NEW]+PREV+[FINAL]:
        p=d+'/'+rel
        if os.path.exists(p):return p
    return None
m=open(PREV[0]+'/support/manifest101.xml','rb').read().decode('utf-8')
known_add=[]
# (1) ICGuard
ic_rel='support/payload/base/SAN14_ICGuard.dll';old_ic=shaf(locate(ic_rel));new_ic=put_new(ic_rel,open(BO+'/SAN14_ICGuard.dll','rb').read())
assert old_ic!=new_ic and m.count(old_ic)==1;m=m.replace(old_ic,new_ic);known_add.append((old_ic,'R42h ICGuard'))
# (2) DomesticPlayer.ini 에 [Mod] 등록 칸
dpi_rel='support/payload/f22/SAN14Mods/modules/DomesticPlayer.ini';old_dpi=shaf(locate(dpi_rel))
dpi=('; R42h 22번 플레이어 내정 자동(모든 내정·최소 인구 1만) — 통합패치 자체 모듈. R42i: 빠져 있던 [Mod] 등록 칸 추가\r\n'
     '[Mod]\r\nId=SAN14CE.DomesticPlayer\r\nFile=SAN14_DomesticPlayer.dll\r\nEnabled=1\r\nCoreABI=65536\r\n\r\n'
     '[DomesticPlayer]\r\nAllDomestic=1\r\nPopulationFloor=0\r\n').encode('utf-8')
new_dpi=put_new(dpi_rel,dpi);assert m.count(old_dpi)==1;m=m.replace(old_dpi,new_dpi);known_add.append((old_dpi,'R42h DomesticPlayer.ini (no [Mod])'))
# (3) 2번 자동논공 새 엔진
am_rel='support/payload/f02/SAN14_AutoMerit.dll';am_sha=put_new(am_rel,open(BO+'/SAN14_AutoMerit.dll','rb').read())
ami_rel='support/payload/f02/SAN14Mods/modules/AutoPersonnelRepairMerit.ini'
ami=('; R42i3 2번 자동논공 새 엔진(게임 코드는 안 바꿈) — 통합편의 자동논공이 고른 계획을 게임 논공 확정 함수(손 논공·AI 논공과 같은 길)로 넣음\r\n'
     '; Pending·Disabled 는 모듈이 스스로 씀(Disabled=1 이면 새 엔진 꺼짐 → 예전 방식)\r\n'
     '[Mod]\r\nId=SAN14CE.AutoMerit\r\nFile=SAN14_AutoMerit.dll\r\nEnabled=1\r\nCoreABI=65536\r\n\r\n'
     '[AutoMerit]\r\nEngine=1\r\n').encode('utf-8')
ami_sha=put_new(ami_rel,ami)
add=(f'  <File target="SAN14_AutoMerit.dll" source="{am_rel}" sha256="{am_sha}" when="2" id="2" />\n'
     f'  <File target="SAN14Mods/modules/AutoPersonnelRepairMerit.ini" source="{ami_rel}" sha256="{ami_sha}" when="2" id="2" />\n')
anc=re.search(r'  <File target="SAN14Mods/modules/AutoPersonnelRepair.ini"[^\n]*\n',m);m=m[:anc.end()]+add+m[anc.end():]
known_add+=[('d56c98ecf810dd4e06a6b2caa824ff206c44d896aecedbe6c97f0605ee6cdb7f','R42i AutoMerit (watch hooks)'),('82f026f026296ae7dd32b2f9477ca5a1b7a152e2b9e052eac8ee449b3c1bdab6','R42i AutoPersonnelRepairMerit.ini')]   # R42i 를 깔았던 경우도 우리 것으로 인식
known_add+=[('5fccb51b5fee0f5334b178bbcc72e02d568699e7c46a3fb168bb6a0cecb92e41','R42i2 AutoMerit'),('22f4ba15e4650264e1506d18ad065720cbef9274e3e53a7990f6f793d8e35f16','R42i2 AutoPersonnelRepairMerit.ini')]
# R42i3: 22번 DomesticPlayer.dll 교체(플레이어 없음·불러오기 전·빈 칸이면 안 씀)
dp_rel='support/payload/f22/SAN14_DomesticPlayer.dll';old_dp=shaf(locate(dp_rel));new_dp=put_new(dp_rel,open(BO+'/SAN14_DomesticPlayer.dll','rb').read())
assert old_dp!=new_dp and m.count(old_dp)==1;m=m.replace(old_dp,new_dp);known_add.append((old_dp,'R42h DomesticPlayer'))
# Known(예전 해시) — 정렬 자리에
for h,note in known_add:
    if f'<Known sha256="{h}"' in m:continue
    known=re.findall(r'  <Known sha256="([0-9a-f]{64})"',m);after=[k for k in known if k>h]
    pos=m.index(f'  <Known sha256="{after[0]}"') if after else m.index('</Manifest>');m=m[:pos]+f'  <Known sha256="{h}" note="{note}" />\n'+m[pos:]
mb=m.encode('utf-8');put_new('support/manifest101.xml',mb)
put_new('support/installer101/ManifestHash.cs',('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }\n'%sha(mb)).encode('utf-8'))
# README
rd=open(PREV[0]+'/README_KO.txt','rb').read().decode('utf-8-sig')
note=('■ R42i3: 2번 "턴 종료 자동 논공"을 통합패치 자체 엔진(SAN14_AutoMerit.dll)으로 넣게 바꿈.\n'
      '    - 예전엔 자동논공이 게임의 논공 처리를 거치지 않고 숫자만 맞춰서, 세이브를 불러오면 내정 보상(내정갯수·시정)이 사라질 수 있었음.\n'
      '    - 이제 무엇을 줄지는 2번 설정 그대로, 실제로 넣는 건 손으로 논공할 때와 같은 게임 처리로 → 저장도 손 논공과 똑같음.\n'
      '    - 새 엔진은 게임 코드를 한 바이트도 바꾸지 않음(통합편의·다른 모드와 겹치는 자리 없음). 첫 턴부터 동작.\n'
      '    - 문제가 생기면 예전 방식으로 돌아가고 게임 안에 이유를 띄움(글자 복사해 댓글). 13번(논공 가챠)을 쓰면 지금처럼 자동논공은 꺼짐.\n'
      '■ R42i3: 22번 플레이어 내정 자동(R42h 새 모듈)이 설정 파일 등록 칸이 빠져 실제로는 안 올라오던 것 고침.\n'
      '    - 데모·타이틀 화면처럼 플레이어가 없을 때나 게임을 불러오기 전에는 아무것도 안 하게 고침(빈 지역에 쓰던 것).\n'
      '■ R42i3: 2번 자동논공 새 엔진의 게임 판 확인을 빌드 시각·크기로 고침(다른 모드가 함수 입구를 우회해 둬도 동작).\n')
put_new('README_KO.txt',b'\xef\xbb\xbf'+(note+rd).encode('utf-8'))
put_new('README_덮어쓰기_R42i3.txt',b'\xef\xbb\xbf'+('통합패치 R42i3 덮어쓰기 (R42i·R42i2 대신 이것) (11월2일 최종판 위에 — R42g2·R42h 내용 포함, 앞 덮어쓰기 안 깔았어도 됨)\r\n\r\n'
 '1. 이 ZIP 안의 폴더·파일을 통합패치 최종판 폴더에 그대로 덮어쓰기.\r\n'
 '2. 설치.cmd(또는 설치기) 실행 → 켜 둔 항목대로 게임 폴더에 깔림.\r\n\r\n'
 '바뀐 것: 2번 자동논공 새 엔진(SAN14_AutoMerit.dll), 22번 DomesticPlayer, ICGuard(base), 22번 DomesticPlayer 등록 칸, 모드 설정(R42g2·R42h), 설치 목록.\r\n\r\n'
 '확인 기록(SAN14_기록\\적용확인.log):\r\n'
 '  R42D 2 SAN14_AutoMerit.dll start ... version=OK ... code_patch=none  → 시작됨(게임 코드 안 바꿈)\r\n'
 '  R42D 2 ... learn giver=ruler_of_same_force confirmed x2  → 새 엔진 켜짐(게임 논공 두 번 본 뒤)\r\n'
 '  R42D 2 ... exec native commit officers=N result=1  → 자동논공을 게임 처리로 넣음\r\n'
 '  R42D 22 SAN14_DomesticPlayer.dll start ... OK  → 22번 플레이어 내정 자동 올라옴\r\n').encode('utf-8'))
# ---- 누적: manifest 의 모든 파일을 찾아, 정식판과 다르거나 없는 것만 ZIP 에
missing=[];bad=[];cum={}
final_sums={}
for l in open(FINAL+'/SHA256SUMS.txt',encoding='utf-8'):
    l=l.rstrip('\n')
    if l.strip():h_,p_=l.split('  ',1);final_sums[p_]=h_
for src,s in re.findall(r'source="([^"]+)" sha256="([0-9a-f]{64})"',m):
    p=locate(src)
    if not p:
        if final_sums.get(src)==s:continue   # 작업 공간 사본엔 없지만(큰 파일) 정식판 목록과 같은 해시 = 안 바뀐 파일
        missing.append(src);continue
    if shaf(p)!=s:bad.append(src);continue
    fp=FINAL+'/'+src
    if not os.path.exists(fp) or shaf(fp)!=s:cum[src]=p
assert not missing,missing;assert not bad,bad
for rel in ['support/manifest101.xml','support/installer101/ManifestHash.cs','README_KO.txt','README_덮어쓰기_R42i3.txt']:cum[rel]=NEW+'/'+rel
for rel,p in cum.items():
    d=PKG+'/'+rel;os.makedirs(os.path.dirname(d),exist_ok=True);shutil.copy(p,d)
# SHA256SUMS: 정식판 목록 + 바뀐 것
sums={}
for l in open(FINAL+'/SHA256SUMS.txt',encoding='utf-8'):
    l=l.rstrip('\n')
    if l.strip():h,p=l.split('  ',1);sums[p]=h
for rel in cum:sums[rel]=shaf(PKG+'/'+rel)
open(PKG+'/SHA256SUMS.txt','w',encoding='utf-8',newline='\n').write(''.join(f'{sums[p]}  {p}\n' for p in sorted(sums)))
# ---- 검사: 모듈 DLL(SAN14_ModInitV1 내보냄) 마다 같은 항목에 [Mod] File= 등록 ini
import xml.etree.ElementTree as ET;root=ET.fromstring(mb)
files=[(f.get('target'),f.get('source'),f.get('when')) for f in root.iter('File')]
def exports(p):
    o=subprocess.run(['llvm-readobj','--coff-exports',p],capture_output=True,text=True).stdout
    return set(re.findall(r'Name: (\S+)',o))
reg={}
for tgt,src,when in files:
    if tgt.lower().endswith('.ini') and 'modules/' in tgt:
        t=open(locate(src),'rb').read().decode('utf-8-sig','replace')
        if re.search(r'^\[Mod\]',t,re.M):
            fm=re.search(r'^File=(.+?)\s*$',t,re.M);en=re.search(r'^Enabled=1\s*$',t,re.M);ab=re.search(r'^CoreABI=65536\s*$',t,re.M)
            if fm and en and ab:reg.setdefault(when,set()).add(fm.group(1).strip().replace('\\','/').lower())
problems=[]
OTHER_LOADER={'san14_modpackbridge.dll'}   # SAN14Mods\ModPack 쪽 다른 길로 올라감(예전부터 정상 동작)
for tgt,src,when in files:
    if not tgt.lower().endswith('.dll') or '/' in tgt.replace('\\','/'):continue
    if tgt.lower() in OTHER_LOADER:continue
    if 'SAN14_ModInitV1' not in exports(locate(src)):continue
    ok=set(reg.get('base',set()))
    for w in when.split('|'):ok|=reg.get(w,set())
    ok|=reg.get(when,set())
    if tgt.lower() not in ok:problems.append((when,tgt))
print('모듈 등록 누락:',problems if problems else '없음')
assert not problems,problems
# ZIP(분할 표기 1of1 — 작아서 한 개)
zp=OUT+'/통합패치_R42i3_덮어쓰기_1of1.zip'
with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
    for r_,_,fs in os.walk(PKG):
        for fn in sorted(fs):full=os.path.join(r_,fn);z.write(full,os.path.relpath(full,PKG))
print('누적 파일',len(cum),sorted(cum))
print('ICGuard',new_ic[:16],'AutoMerit',am_sha[:16],'DomesticPlayer.ini',new_dpi[:16],'manifest',sha(mb)[:16])
print('zip',zp,os.path.getsize(zp))
