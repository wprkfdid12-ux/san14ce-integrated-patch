# R43: 게임 2026-11-05 업데이트판(0x6ad06f53 / 0x2a02000 / 782cf229…) 호환 — 정식판(11월5일 최종판 = 11월2일 + R42g2·R42h·R42i4) 위에 덮어쓰는 '누적' 판
#  바뀜: (1) 바이너리 모듈 전부 새 판 주소(apply_new + apply_data_new + ic_port_r43) (2) 우리 소스 모듈 전부 다시 빌드(version.h 새 판·R43F 오류창·Diag)
#        (3) 13번 리졸버 번역 재빌드 (4) 11번 3D 런타임 새 판 빌드 (5) 데이터: 24번 fixdata 새 원본/미적용본, 10번·31번 rdb 새 원본 위에 우리 레코드
#        (6) 설치기 소스(새 판 해시·판 라벨·R43F/R43D/R43E 해석) (7) manifest: version/game/Values/File sha/Known
#  검사: XML·해시·누적 파일 + 모듈 DLL 마다 [Mod] 등록 ini + data Values == 실제 파일 해시
import time,hashlib,os,re,shutil,zipfile,subprocess,sys
FINAL='/mnt/user-data/uploads/통합패치/통합패치 11월2일 최종판'
FINAL5='/mnt/user-data/uploads/통합패치/통합패치 11월 5일 최종판'   # data/model101·screen101 (11월2일 사본엔 없음)
PREV=['/home/claude/r41/overlay_r42i4/pkg','/home/claude/r41/overlay_r42h/pkg','/home/claude/r41/overlay_r42g2/pkg','/home/claude/r41/overlay_r42g/pkg']
OUT='/home/claude/r41/overlay_r43';PKG=OUT+'/pkg';NEW=OUT+'/new';R='/home/claude/r41';PAY=R+'/newpkg/payload_new';CUR=R+'/newpkg/payload_cur';DATA=R+'/newpkg/data_new'
NEWGAME_SHA='782cf229a56cdde2b95399e0e7536740b3f5e96bea702679365e4dad061abdce';OLDGAME_SHA='87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a'
PREV101='/mnt/user-data/uploads/통합패치/통합패치 11월 5일 최종판/data'   # 옛 게임판(1.0.1) 원본 색인 — 새 판에서 옛 레코드가 남은 색인을 되돌릴 때 비교용
VER='R43';PART_MAX=4_500_000
def sha(b):return hashlib.sha256(b).hexdigest()
def shaf(p):return sha(open(p,'rb').read())
shutil.rmtree(OUT,ignore_errors=True);os.makedirs(PKG);os.makedirs(NEW)
def put_new(rel,data):
    p=NEW+'/'+rel;os.makedirs(os.path.dirname(p),exist_ok=True);open(p,'wb').write(data);return sha(data)
def locate(rel):
    for d in [NEW]+PREV+[FINAL5,FINAL]:
        p=d+'/'+rel
        if os.path.exists(p):return p
    return None
m=open(PREV[0]+'/support/manifest101.xml','rb').read().decode('utf-8')
known_add=[];replaced=[];added=[]
SKIP_NAMES={'적용_보고.txt','자료_적용_보고.txt','느슨한지문_적용_보고.txt','기록칸_적용_보고.txt','기록칸_적용_보고2.txt'}
def _skip(rel):return os.path.basename(rel) in SKIP_NAMES or (rel.count('/')==0 and rel.endswith('.txt')) or ('_보고' in os.path.basename(rel) and rel.endswith('.txt'))
# (1)(2)(3)(4) payload: payload_new 의 모든 파일 중 정식판(사슬)과 다른 것
for root,_,fs in os.walk(PAY):
    for fn in sorted(fs):
        if fn in SKIP_NAMES or fn.endswith('.ic_port_r43.txt') or ('_보고' in fn and fn.endswith('.txt')):continue
        full=os.path.join(root,fn);rel='support/payload/'+os.path.relpath(full,PAY).replace('\\','/')
        data=open(full,'rb').read();h=sha(data);old=locate(rel)
        if old and shaf(old)==h:continue
        put_new(rel,data)
        if not old:
            mm=re.search(r'source="%s" sha256="([0-9a-f]{64})"'%re.escape(rel),m)
            if mm:old=True;oh_m=mm.group(1)   # 작업 공간에 옛 사본은 없지만(큰 파일) manifest 에 있는 파일
        if old:
            oh=shaf(old) if old is not True else oh_m;replaced.append((rel,oh,h))
            n=m.count(f'sha256="{oh}"')
            if n==0:print('주의: manifest 에 없는 옛 해시(교체만)',rel)
            m=m.replace(f'source="{rel}" sha256="{oh}"',f'source="{rel}" sha256="{h}"')
            known_add.append((oh,'R42i4 '+os.path.basename(rel)))
        else:added.append((rel,h))
# R43 시험6: 17번 옛 이식본 3개는 manifest 에서 빼고 옛 해시를 Known 에 남김(설치기가 state.xml 기록대로 지움). 큰 자료 파일(0002/*.bin 등)은 작업 공간에 없어도 manifest 에 남긴다.
REMOVED=['support/payload/f17/SAN14Mods/ModPack/SAN14_Stats200.dll','support/payload/f17/SAN14_Stats200Reader.dll','support/payload/f17/SAN14Mods/modules/Stats200Reader.ini']
for rel in REMOVED:
    mm=re.search(r'  <File target="([^"]+)" source="%s" sha256="([0-9a-f]{64})"[^\n]*\n'%re.escape(rel),m)
    if mm:m=m.replace(mm.group(0),'');known_add.append((mm.group(2),'R43 removed '+os.path.basename(rel)));print('제거 항목',mm.group(1))
    else:print('제거 대상이 manifest 에 없음',rel)
# 새 파일(Diag) manifest 항목
for rel,h in added:
    tgt=rel.replace('support/payload/base/','').replace('support/payload/','')
    if rel.startswith('support/payload/base/'):
        tgt=rel[len('support/payload/base/'):];when='base';idn='0'
    else:
        fid=re.match(r'support/payload/f(\d\d)/',rel).group(1);tgt=rel[len('support/payload/fNN/'):];when=str(int(fid));idn=str(int(fid))
    line=f'  <File target="{tgt}" source="{rel}" sha256="{h}" when="{when}" id="{idn}" />\n'
    anc=re.search(r'  <File target="SAN14Mods/modules/CrashLog.ini"[^\n]*\n',m);m=m[:anc.end()]+line+m[anc.end():]
    print('추가 항목',tgt)
# (5) data/
vals={}
for rel,src,key in (('data/fixdata_original_101.s14',DATA+'/fixdata_original_101.s14','fix_original'),('data/fixdata_nostat_101.s14',DATA+'/fixdata_nostat_101.s14','fix_nostat'),
                    ('data/model101/KIDSSystemResource.rdb',DATA+'/model101/KIDSSystemResource.rdb','model_orig_KIDSSystemResource.rdb'),('data/screen101/ScreenLayout.rdb',DATA+'/screen101/ScreenLayout.rdb','screen_orig_ScreenLayout.rdb'),
                    ('data/prev101/KIDSSystemResource.rdb',PREV101+'/model101/KIDSSystemResource.rdb','prev_KIDSSystemResource.rdb'),('data/prev101/ScreenLayout.rdb',PREV101+'/screen101/ScreenLayout.rdb','prev_ScreenLayout.rdb')):
    data=open(src,'rb').read();h=sha(data);old=locate(rel)
    if not(old and shaf(old)==h):put_new(rel,data);replaced.append((rel,shaf(old) if old else '-',h))
    vals[key]=h
    if f'<Value name="{key}" ' not in m:   # R43: 새 Value(옛 판 원본 색인) — 이름 순 자리에
        names=re.findall(r'  <Value name="([^"]+)"',m);after=[k for k in names if k>key]
        pos=m.index(f'  <Value name="{after[0]}"') if after else m.index('</Manifest>');m=m[:pos]+f'  <Value name="{key}" value="{h}" />\n'+m[pos:]
    m2=re.sub(r'(<Value name="%s" value=")[0-9a-f]{64}(")'%re.escape(key),r'\g<1>%s\2'%h,m);assert m2!=m or f'value="{h}"' in m,key;m=m2
# (6) 설치기 소스
for fn in ('Core.cs','Spec.cs','Stat.cs','Engine.cs','Update.cs','App.cs','BuildAndRun.ps1'):   # R43.28: Update.cs(깃허브 새 판 받기)
    rel='support/installer101/'+fn;data=open(R+'/installer101/'+fn,'rb').read();old=locate(rel)
    if not(old and shaf(old)==sha(data)):put_new(rel,data);replaced.append((rel,shaf(old) if old else '-',sha(data)))
# 정식판 뒤(11-08): 9번 확인 모듈 등록 이름 변경(14번보다 먼저 올라오게). 자료(source)는 그대로, 설치 대상 이름만.
m=m.replace('<File target="SAN14Mods/modules/Traits9Check.ini" ','<File target="SAN14Mods/modules/Mod09_Traits9Check.ini" ')
assert 'target="SAN14Mods/modules/Mod09_Traits9Check.ini"' in m
# (7) manifest 머리
m=re.sub(r'<Manifest version="[^"]+" game="[0-9a-f]{64}">',f'<Manifest version="{VER}" game="{NEWGAME_SHA}">',m)
# R43 시험판 이전 빌드의 해시(설치기가 '알 수 없는 내용'으로 막지 않게) — overlay_r43_prev_hashes.txt 에 빌드마다 누적
PREVH=os.path.join(R,'overlay_r43_prev_hashes.txt');cur_new={h for rel,oh,h in replaced}|{h for rel,h in added}
if os.path.exists(PREVH):
    for ln in open(PREVH,encoding='utf-8'):
        ln=ln.strip()
        if not ln or ln.startswith('#'):continue
        h,note=ln.split('\t',1)
        if h not in cur_new and f'sha256="{h}"' not in m:known_add.append((h,note))
# Known(예전 해시) — 정렬 자리에
for h,note in known_add:
    if f'<Known sha256="{h}"' in m:continue
    known=re.findall(r'  <Known sha256="([0-9a-f]{64})"',m);after=[k for k in known if k>h]
    pos=m.index(f'  <Known sha256="{after[0]}"') if after else m.index('</Manifest>');m=m[:pos]+f'  <Known sha256="{h}" note="{note}" />\n'+m[pos:]
mb=m.encode('utf-8');put_new('support/manifest101.xml',mb)
put_new('support/installer101/ManifestHash.cs',('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }\n'%sha(mb)).encode('utf-8'))
# README
rd=open(PREV[0]+'/README_KO.txt','rb').read().decode('utf-8-sig')
note=('■ R43.30 (11월 11일): [업데이트]·[버전 되돌리기] 때 진행 창(받기→확인→새 폴더→완료 단계, 막대·%, 지금 하는 일, 남은 시간, 취소). 끝나면 바로 새 판 설치기가 열림.\n\n'
      '■ R43.29 (11월 11일): 게임 안 종합 진단 창에 "설치한 통합패치 판"을 적음(제보 글자만으로 판을 알게). R43.28 설치 기록을 진단이 못 읽어 안 깐 번호의 옛 파일까지 문제로 세던 것 고침. 설치기 오른쪽 "게임 적용 확인" 칸이 게임 폴더를 읽은 뒤에도 "먼저 확인"으로 남던 것 고침.\n\n'
      '■ R43.28 (11월 11일): 설치기에 [업데이트]·[버전 되돌리기]·[게임 폴더 변경] 버튼, 상태 줄(설치판·공개 최신판·게임 켜짐), 탭별 깔린 수, 업데이트·작업 기록 탭.\n'
      '    - 새 판은 깃허브(github.com/wprkfdid12-ux/san14ce-integrated-patch)에서 받습니다. 바뀐 파일만 받고 나머지는 지금 폴더에서 복사해 옆에 새 폴더를 만듭니다. 지금 폴더·게임은 그대로.\n'
      '    - 게임 기능은 27차와 같습니다.\n\n'
      '■ R43 (11월 7일 최종판): 게임 2026-11-05 공식 업데이트판 전용. 옛 판(1.0.1, 9월 2일 빌드)에서는 "판이 다릅니다" 창과 함께 아무것도 안 바꿈.\n'
      '    - 모든 모듈의 게임 주소·자료 위치를 새 판에 맞춤(통합편의 포함). 24번 능력치 미적용 자료·10번/31번 그래픽 자료는 새 원본 위에 다시 만듦. 11번 3D 조형 런타임 새로 빌드, 13번 논공 개성 리졸버 번역.\n'
      '    - 문전박대: 군주가 관문·항구에 있어도 그곳을 가진 도시 기준으로 사신을 막음.\n'
      '    - 고침: 사실무장 편집을 열 때 튕김, 27번 미니맵이 안 그려지던 것, 22번 내정 계산 상수, 통합편의 포로 처우·탐색 예산 수치.\n'
      '    - 14번 전법 설명: 콜라보·특수 전법 48개 설명 추가, "총명강의" 글자 고침.\n'
      '    - 10번·31번: 옛 판 설치기가 1.0.1 원본으로 되돌려 둔 색인 레코드(KIDSSystemResource 1개·ScreenLayout 8개)를 새 판 원본으로 바로잡음(다른 모드가 고친 레코드는 그대로).\n'
      '    - 진단: 시작 30초·90초에 모듈이 다 올라왔는지 확인(SAN14_Diag), 문제가 있을 때만 창(R43D). 모듈이 못 돌 때는 어디가 왜 다른지 창(R43F).\n'
      '    - 알려진 한계: 17번 능력치 200 의 경험치 성장 분기 1곳은 새 판 코드에 자리가 모자라 적용하지 않음(게임 로그에 XP_GROWTH_NOT_APPLIED 한 줄). 나머지 9곳은 적용.\n'
      '    - 27번 안의 옛 실험 기능(FEED90, 미니맵과 별개)은 새 판에 옮기지 않고 돌지 않게 막아 둠.\n')
put_new('README_KO.txt',b'\xef\xbb\xbf'+(note+rd).encode('utf-8'))
put_new(f'README_덮어쓰기_{VER}.txt',b'\xef\xbb\xbf'+(f'통합패치 {VER} 덮어쓰기 — 게임 2026-11-05 업데이트판 전용 (11월 5일 최종판 폴더 위에 덮어쓰기)\r\n\r\n'
 '1. 이 ZIP(들) 안의 폴더·파일을 "통합패치 11월 5일 최종판" 폴더에 그대로 덮어쓰기(분할 ZIP 은 전부).\r\n'
 '2. 설치.cmd 실행 → 켜 둔 항목대로 게임 폴더에 깔림. 옛 게임판이면 설치기가 판이 다르다고 알려 주고 멈춤.\r\n\r\n'
 '확인 기록(SAN14_기록\\적용확인.log):\r\n'
 '  R43F 줄 = 어떤 모듈이 어디서 왜 못 돌았는지(창도 뜸)   R43D 줄 = 30초·90초 종합 진단   R43E 줄 = 판 불일치\r\n').encode('utf-8'))
# ---- 누적: manifest 의 모든 파일을 찾아, 정식판과 다르거나 없는 것만 ZIP 에
final_sums={}
for l in open(PREV[0]+'/SHA256SUMS.txt',encoding='utf-8'):   # 11월5일 최종판 목록(= overlay_r42i4 의 SHA256SUMS, PC 와 같음 확인)
    l=l.rstrip('\n')
    if l.strip():h_,p_=l.split('  ',1);final_sums[p_]=h_
missing=[];bad=[];cum={}
for src,s in re.findall(r'source="([^"]+)" sha256="([0-9a-f]{64})"',m):
    p=locate(src)
    if not p:
        if final_sums.get(src)==s:continue
        missing.append(src);continue
    if shaf(p)!=s:bad.append((src,shaf(p)[:12],s[:12]));continue
    if final_sums.get(src)!=s:cum[src]=p
assert not missing,missing;assert not bad,bad
for rel in ['support/manifest101.xml','support/installer101/ManifestHash.cs','README_KO.txt',f'README_덮어쓰기_{VER}.txt']+[r for r,_,_ in replaced if r.startswith(('data/','support/installer101/'))]:cum[rel]=NEW+'/'+rel
for rel,p in cum.items():
    d=PKG+'/'+rel;os.makedirs(os.path.dirname(d),exist_ok=True);shutil.copy(p,d)
# data Values == 실제 파일
for key,h in vals.items():
    rel={'fix_original':'data/fixdata_original_101.s14','fix_nostat':'data/fixdata_nostat_101.s14','model_orig_KIDSSystemResource.rdb':'data/model101/KIDSSystemResource.rdb','screen_orig_ScreenLayout.rdb':'data/screen101/ScreenLayout.rdb','prev_KIDSSystemResource.rdb':'data/prev101/KIDSSystemResource.rdb','prev_ScreenLayout.rdb':'data/prev101/ScreenLayout.rdb'}[key]
    assert shaf(locate(rel))==h,(key,rel)
# SHA256SUMS: 정식판 목록 + 바뀐 것
sums=dict(final_sums)
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
problems=[];OTHER_LOADER={'san14_modpackbridge.dll','san14_fps120.dll'}   # 19번 본체는 SAN14_FPS120Late.dll(등록됨)이 싣는다(2026-11-10 P201)
for tgt,src,when in files:
    if not tgt.lower().endswith('.dll') or '/' in tgt.replace('\\','/'):continue
    if tgt.lower() in OTHER_LOADER:continue
    p=locate(src)
    if not p or 'SAN14_ModInitV1' not in exports(p):continue
    ok=set(reg.get('base',set()))
    for w in when.split('|'):ok|=reg.get(w,set())
    ok|=reg.get(when,set())
    if tgt.lower() not in ok:problems.append((when,tgt))
print('모듈 등록 누락:',problems if problems else '없음');assert not problems,problems
# 옛 게임 해시가 설치기 소스 밖에 남아 있으면 안 됨(설치기 Spec 의 OldGames 는 예외)
for rel in cum:
    if rel.endswith(('.dll','.exe','.xml')):
        b=open(PKG+'/'+rel,'rb').read()
        for enc in(OLDGAME_SHA.encode(),bytes.fromhex(OLDGAME_SHA)):
            if enc in b and 'manifest' not in rel:print('주의: 옛 게임 해시 포함',rel)
# ---- 분할 ZIP(각각 독립, 용량 균등)
items=sorted(((os.path.getsize(PKG+'/'+rel),rel) for rel in cum),reverse=True)
parts=[];
for size,rel in items:
    for p in parts:
        if p[0]+size<=PART_MAX:p[0]+=size;p[1].append(rel);break
    else:parts.append([size,[rel]])
# SHA256SUMS·README·manifest 는 1부에
for must in ('SHA256SUMS.txt','README_KO.txt',f'README_덮어쓰기_{VER}.txt','support/manifest101.xml','support/installer101/ManifestHash.cs'):
    for p in parts:
        if must in p[1]:p[1].remove(must)
    parts[0][1].append(must)
N=len(parts);zips=[]
for i,(sz,rels) in enumerate(parts,1):
    zp=OUT+f'/통합패치_{VER}_덮어쓰기_{i}of{N}.zip'
    with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED) as z:
        for rel in sorted(set(rels)):z.write(PKG+'/'+rel,rel)
        if 'SHA256SUMS.txt' not in rels:z.write(PKG+'/SHA256SUMS.txt','SHA256SUMS.txt')
    zips.append((zp,os.path.getsize(zp)))
print('누적 파일',len(cum));print('교체',len(replaced),'추가',len(added))
for r,o,h in replaced:print('  ',r,o[:12],'→',h[:12])
print('manifest',sha(mb)[:16],'game',NEWGAME_SHA[:16])
with open(PREVH,'a',encoding='utf-8') as f:
    f.write('# '+time.strftime('%Y-%m-%d %H:%M')+' R43 빌드\n')
    for rel,oh,h in replaced:
        if rel.startswith('support/payload/'):f.write(f'{h}\tR43 시험 이전판 {os.path.basename(rel)}\n')
    for rel,h in added:
        if rel.startswith('support/payload/'):f.write(f'{h}\tR43 시험 이전판 {os.path.basename(rel)}\n')
print('zips',[(os.path.basename(z),s) for z,s in zips])
