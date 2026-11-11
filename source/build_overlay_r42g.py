# R42g 덮어쓰기 묶음 만들기 — 전달본(통합패치 11월2일 최종판, R42e 매니페스트) 위에 바뀐 파일만 얹는다.
#  바뀌는 것: base/SAN14_ModSettings.dll (22번 스위치 5번째 줄), f22/SAN14_DomesticAI.dll + modules/DomesticAI.ini (새 모듈),
#            manifest101.xml(행 2개 추가·ModSettings 해시·version R42g·Known 추가), ManifestHash.cs, Spec.cs(22번 설명), README_KO.txt, SHA256SUMS.txt
import hashlib,os,re,shutil,sys,zipfile
SRC='/mnt/user-data/uploads/통합패치/통합패치 11월2일 최종판'   # 전달본(작은 파일 전부 스테이징됨)
BO='/home/claude/r41/build_out';OUT='/home/claude/r41/overlay_r42g';PKG=OUT+'/pkg'
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(open(p,'rb').read())
shutil.rmtree(OUT,ignore_errors=True);os.makedirs(PKG)
def put(rel,src_path):
    dst=PKG+'/'+rel;os.makedirs(os.path.dirname(dst),exist_ok=True);shutil.copy(src_path,dst);return shaf(dst)
# ---- 바뀐 파일
h_ms=put('support/payload/base/SAN14_ModSettings.dll',BO+'/SAN14_ModSettings.dll')
h_dai=put('support/payload/f22/SAN14_DomesticAI.dll',BO+'/SAN14_DomesticAI.dll')
ini=('; R42g 22번 AI 내정 순환(군주 반영): AI 세력 지역 개발 중점을 군주 정치(70↑ 전부 / 40~69 절반 / 40↓ 3분의 1 순환)·물자(금→상업, 군량→농업, 병사→병영)로 정함. 플레이어 지역은 그대로\r\n'
     '[Mod]\r\nId=SAN14CE.DomesticAI\r\nFile=SAN14_DomesticAI.dll\r\nEnabled=1\r\nCoreABI=65536\r\n\r\n[DomesticAI]\r\nAiRotate=1\r\n')
os.makedirs(PKG+'/support/payload/f22/SAN14Mods/modules',exist_ok=True)
open(PKG+'/support/payload/f22/SAN14Mods/modules/DomesticAI.ini','wb').write(ini.encode('utf-8'));h_ini=shaf(PKG+'/support/payload/f22/SAN14Mods/modules/DomesticAI.ini')
old_ms=shaf(SRC+'/support/payload/base/SAN14_ModSettings.dll');assert old_ms!=h_ms
# ---- manifest
m=open(SRC+'/support/manifest101.xml','rb').read().decode('utf-8')
assert 'version="R42e"' in m;m=m.replace('version="R42e"','version="R42g"',1)
assert m.count(old_ms)==1;m=m.replace(old_ms,h_ms)
anchor='  <File target="SAN14Mods/modules/DomesticTalent.ini" source="support/payload/f22/SAN14Mods/modules/DomesticTalent.ini"'
i=m.index(anchor);j=m.index('\n',i)+1
new_rows=(f'  <File target="SAN14_DomesticAI.dll" source="support/payload/f22/SAN14_DomesticAI.dll" sha256="{h_dai}" when="22" id="22" />\n'
          f'  <File target="SAN14Mods/modules/DomesticAI.ini" source="support/payload/f22/SAN14Mods/modules/DomesticAI.ini" sha256="{h_ini}" when="22" id="22" />\n')
m=m[:j]+new_rows+m[j:]
# Known: 예전 ModSettings(R42e) 해시 — 정렬 자리에 끼움
known=re.findall(r'  <Known sha256="([0-9a-f]{64})" note="[^"]*" />\n',m)
assert old_ms not in known
line=f'  <Known sha256="{old_ms}" note="R21 manifest_r42e_delivered.xml" />\n'
after=[k for k in known if k>old_ms]
if after:
    k=after[0];pos=m.index(f'  <Known sha256="{k}"');m=m[:pos]+line+m[pos:]
else:
    pos=m.index('</Manifest>');m=m[:pos]+line+m[pos:]
mb=m.encode('utf-8');os.makedirs(PKG+'/support/installer101',exist_ok=True)
open(PKG+'/support/manifest101.xml','wb').write(mb);h_man=sha(mb)
open(PKG+'/support/installer101/ManifestHash.cs','w',encoding='utf-8').write('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }\n'%h_man)
old_mh=open(SRC+'/support/installer101/ManifestHash.cs',encoding='utf-8').read()
assert old_mh.strip()==('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }'%sha(open(SRC+'/support/manifest101.xml','rb').read())),'전달본 ManifestHash 형식 확인'
# ---- Spec.cs 22번 설명
sp=open(SRC+'/support/installer101/Spec.cs',encoding='utf-8-sig').read()
raw=open(SRC+'/support/installer101/Spec.cs','rb').read();bom=raw.startswith(b'\xef\xbb\xbf')
a='"내정 자동화(모든 내정·AI 명령서 99장 유지·최소 인구 1만)·저돌 수정 — 스위치 4개"';assert a in sp
sp=sp.replace(a,'"내정 자동화(모든 내정·AI 명령서 99장 유지·최소 인구 1만)·저돌 수정·AI 내정 순환(군주 반영) — 스위치 5개"')
b='"모든 내정 자동 / AI 명령서 99장 유지 / 최소 인구 1만 유지 / 저돌 수정 — 스위치 4개입니다."';assert b in sp
sp=sp.replace(b,'"모든 내정 자동 / AI 명령서 99장 유지 / 최소 인구 1만 유지 / 저돌 수정 / AI 내정 순환(군주 반영) — 스위치 5개입니다."')
c='④ 개성 \'저돌\' 효과를 고친 값으로 적용.';assert c in sp
sp=sp.replace(c,c+' ⑤ AI 세력이 지역을 상업→농업→병영 차례로 골고루 키움 — 군주 정치 70 이상이면 전 지역, 40~69 절반, 40 미만 3분의 1만 골고루, 나머지 지역은 군주가 중시하는 물자(금→상업, 군량→농업, 병사→병영)만. 어느 지역이 골고루인지는 달마다 바뀜. 플레이어 지역은 그대로.')
d='모드 설정 창 → 편의 탭 → 4개 줄 각각 켬/끔. 설정은 SAN14_DomesticCE.ini 에 보존됩니다.';assert d in sp
sp=sp.replace(d,'모드 설정 창 → 편의 탭 → 5개 줄 각각 켬/끔. 설정은 SAN14_DomesticCE.ini(앞 4개)·SAN14Mods\\\\modules\\\\DomesticAI.ini(AI 순환)에 보존됩니다.')
open(PKG+'/support/installer101/Spec.cs','wb').write((b'\xef\xbb\xbf' if bom else b'')+sp.encode('utf-8'))
# ---- README
rd=open(SRC+'/README_KO.txt','rb').read().decode('utf-8-sig')
assert rd.startswith('삼국지14 CE 통합패치 R42e');rd=rd.replace('삼국지14 CE 통합패치 R42e','삼국지14 CE 통합패치 R42g',1)
note=('■ R42g 추가 (22번 AI 내정 순환 — 군주 반영): AI 세력은 원작처럼 상업에만 몰빵한다는 제보(22번 순환은 플레이어 지역에만 걸려 있었음). 이제 AI 세력 지역도 상업→농업→병영(100·300·600·1000 단계) 차례로 골고루 키움. '
      '군주 정치가 70 이상이면 전 지역(조조·손권·유비), 40~69 는 절반(손책·도겸·공손찬), 40 미만은 3분의 1(유장·동탁·여포)만 골고루 키우고, 나머지 지역은 군주가 중시하는 물자(금→상업, 군량→농업, 병사→병영)만 키움. '
      '어느 지역이 골고루인지는 달마다 바뀌어 한 가지만 1000 인 기형 지역은 생기지 않음. 게임 AI 가 중점을 고르는 자리(행동트리 CDomesticPriorityDevelopmentSelectNode) 자체에서 우리 규칙으로 답하므로 AI 는 자기 결정으로 알고 임명함(되돌림 없음). '
      '플레이어 세력 지역은 손대지 않음. 모드 설정 창 편의 탭 "AI 내정 순환(군주 반영)" 스위치로 끔(SAN14Mods\\modules\\DomesticAI.ini AiRotate). 기록: 적용확인.log 에 시작 줄(R39 22 SAN14_DomesticAI.dll)과 게임 달마다 한 줄(R42D … month 연/월 calls/ai/rot/fix, 세력별 군주정치:순환비율:물자). '
      '다른 AI 모드가 같은 노드를 통째로 바꾸면 우리 규칙은 쉼(시작 줄 IDLE). 프레임: AI 가 중점을 고를 때만 몇 번 읽는 수준, 매 프레임 끼어드는 곳 없음.\n')
k=rd.index('■ R42e 추가 (1번 길찾기 보호)');rd=rd[:k]+note+rd[k:]
e='22  모든 내정 자동 · AI 명령서 99장 유지 · 최소 인구 1만 · 저돌 수정 (편의 탭 스위치 4개)';assert e in rd
rd=rd.replace(e,'22  모든 내정 자동 · AI 명령서 99장 유지 · 최소 인구 1만 · 저돌 수정 · AI 내정 순환(군주 반영) (편의 탭 스위치 5개)')
open(PKG+'/README_KO.txt','wb').write(b'\xef\xbb\xbf'+rd.encode('utf-8'))
# ---- SHA256SUMS (전체 목록 다시: 바뀐 것은 새 해시, 나머지는 전달본 값)
sums={}
for l in open(SRC+'/SHA256SUMS.txt',encoding='utf-8'):
    l=l.rstrip('\n')
    if not l.strip():continue
    h,p=l.split('  ',1);sums[p]=h
for root,_,files in os.walk(PKG):
    for fn in files:
        full=os.path.join(root,fn);rel=os.path.relpath(full,PKG).replace('\\','/')
        if rel=='SHA256SUMS.txt':continue
        sums[rel]=shaf(full)
open(PKG+'/SHA256SUMS.txt','w',encoding='utf-8',newline='\n').write(''.join(f'{sums[p]}  {p}\n' for p in sorted(sums)))
# ---- 덮어쓰기 안내
guide=('통합패치 R42g 덮어쓰기 (22번 AI 내정 순환 — 군주 반영)\r\n\r\n'
 '1. 이 ZIP 안의 폴더·파일을 "통합패치 11월2일 최종판" 폴더에 그대로 덮어쓰기(같은 자리에 풀기).\r\n'
 '2. 설치.cmd 실행 → 22번이 체크돼 있으면 새 파일(SAN14_DomesticAI.dll)까지 설치됨. 이미 깔려 있던 분도 설치.cmd 한 번 더(갱신).\r\n'
 '3. 게임 안 모드 설정 창 편의 탭에 "AI 내정 순환(군주 반영)" 줄이 보이면 적용된 것.\r\n\r\n'
 '바뀐 파일: support\\payload\\base\\SAN14_ModSettings.dll, support\\payload\\f22\\SAN14_DomesticAI.dll, support\\payload\\f22\\SAN14Mods\\modules\\DomesticAI.ini,\r\n'
 '          support\\manifest101.xml, support\\installer101\\ManifestHash.cs, support\\installer101\\Spec.cs, README_KO.txt, SHA256SUMS.txt\r\n'
 '지우지 말 것: 예전 파일은 설치기가 알아서 바꿈. BUILD_ERROR.txt·SAN14CE_Patch101.exe 는 배포 ZIP 에 넣지 않아도 됨.\r\n')
open(PKG+'/README_덮어쓰기_R42g.txt','wb').write(b'\xef\xbb\xbf'+guide.encode('utf-8'))
# ---- ZIP
zp=OUT+'/통합패치_R42g_덮어쓰기_AI내정순환_1of1.zip'
with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
    for root,_,files in os.walk(PKG):
        for fn in sorted(files):
            full=os.path.join(root,fn);z.write(full,os.path.relpath(full,PKG))
print('manifest',h_man);print('ModSettings',h_ms);print('DomesticAI',h_dai);print('ini',h_ini);print('zip',zp,os.path.getsize(zp))
