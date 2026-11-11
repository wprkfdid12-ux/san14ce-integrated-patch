# R40 패키지 조립 (R39 build_pkg_r39.py 기반): /home/claude/r41/pkg/ (support/payload, support/manifest101.xml, support/installer101, data/)
import os,sys,shutil,hashlib,re,zipfile,io,json
from xml.sax.saxutils import quoteattr
P='/home/claude/port'; OLD='/home/claude/p27/x/SAN14CE_Rollback_Patch27/support'
SC='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad'
MP=SC+'/modpack/x/payload'; FOG16=SC+'/fog16/_files/payload/SAN14_FogTrial.dll'
R='/home/claude/r41'; PKG=R+'/pkg'; BO=R+'/build_out'; PL=R+'/payload'; R39='/home/claude/r39'
import os as _os
def pick(*c):
    for x in c:
        if _os.path.exists(x): return x
    raise SystemExit('missing: '+' / '.join(c))
def addtree(group_dir,when,id_):
    for root_,_,files in _os.walk(group_dir):
        for fn in files:
            full=_os.path.join(root_,fn); add(_os.path.relpath(full,group_dir).replace(_os.sep,'/'),full,when,id_)
def sha(b): return hashlib.sha256(b).hexdigest()
def shaf(p): return sha(open(p,'rb').read())
reg=lambda id_,file,comment=None:((comment+'\r\n') if comment else '')+f"[Mod]\r\nId={id_}\r\nFile={file}\r\nEnabled=1\r\nCoreABI=65536\r\n"

# R41f: build_out 새 모듈이 payload 에 실제로 들어갔는지 확인(sync_payload.sh 빠뜨림 방지)
for _n,_d in [('SAN14_RegionOfficer.dll','f34'),('SAN14_StatusColor.dll','f29'),('SAN14_ModSettings.dll','base'),('SAN14_ICGuard.dll','base'),('SAN14_HiredFace.dll','f32'),('SAN14_Range.dll','f33'),('SAN14_ItemPicture.dll','f30'),('SAN14_SceneSkip.dll','f04'),('SAN14_ExecuteLock.dll','f26')]:
    assert shaf(BO+'/'+_n)==shaf(PL+'/'+_d+'/'+_n),'payload 가 build_out 과 다름: '+_n+' (sync_payload.sh 먼저)'
# (target, source bytes or path, when, id)
F=[]
def add(target,src,when,id_):
    data=open(src,'rb').read() if isinstance(src,str) else src
    F.append((target,data,when,id_))
# R41: 12번(필드 안개) 삭제 — 12 행·11&12(안개 연동 3D) 행 없음. 옛 판이 깐 안개 파일은 아래 Known(R20 payload/R39/R40) 으로 지울 수 있음
# 공통 런타임
add('SAN14Mods/Core/SAN14_ModCore.dll',BO+'/SAN14_ModCore.dll','base',0)  # R40 A22: 게임 확인 실패 때 안내창 DLL 불러옴(modcore_patch.py, 정상 길 그대로)
add('SAN14Mods/Core/SAN14_Notice.dll',BO+'/SAN14_Notice.dll','base',0)  # R40 A22: 통합패치 전체가 안 켜질 때만 뜨는 게임 안 안내창(코드)
add('SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll',P+'/resolver_new.dll','base',0)
add('SAN14_IntegratedConvenience.dll','/home/claude/r40ai/ic/ic_r40.dll','base',0)  # R40: AI 모드 호환(전역 호환 검사 허용 규칙) ae365eab
add('SAN14Mods/modules/IntegratedConvenience_GreenlandCompat.ini',reg('SAN14CE.IntegratedConvenience.Compat','SAN14_IntegratedConvenience.dll').encode(),'base',0)
add('SAN14_SettingsHelpScenic.dll',pick(BO+'/SAN14_SettingsHelpScenic.dll',P+'/build_out/SAN14_SettingsHelpScenic.dll'),'base&!22',0)
add('SAN14_SettingsHelpScenic.dll',pick(BO+'/SAN14_SettingsHelpScenic_domestic.dll',P+'/build_out/SAN14_SettingsHelpScenic_domestic.dll'),'base&22',0)
add('SAN14Mods/modules/SettingsHelpScenic.ini',reg('SAN14CE.SettingsHelpScenic','SAN14_SettingsHelpScenic.dll').encode(),'base',0)
# 튕김 기록기: 게임이 튕기면 어느 파일 어느 자리였는지 SAN14_CrashLog.log 에 남김(게임 코드 변경 없음)
add('SAN14_CrashLog.dll',P+'/bridge/SAN14_CrashLog.dll','base',0)
add('SAN14Mods/modules/CrashLog.ini',reg('SAN14CE.CrashLog','SAN14_CrashLog.dll','; 튕김 기록기 (진단용)').encode(),'base',0)
# 2번 자동 인사 수정 모듈
add('SAN14_AutoPersonnelRepair.dll',pick(BO+'/SAN14_AutoPersonnelRepair.dll',P+'/build_out/SAN14_AutoPersonnelRepair.dll'),'2',2)
add('SAN14Mods/modules/AutoPersonnelRepair.ini',reg('SAN14CE.AutoPersonnelRepair','SAN14_AutoPersonnelRepair.dll').encode(),'2',2)
# 8번 BGM 런타임
z=zipfile.ZipFile(P+'/orig_payload/bgm_runtime.zip')
for i in z.infolist():
    if i.is_dir(): continue
    add(i.filename,z.read(i),'8',8)
# 9번: 개성 9개 단독 모듈(13번 없을 때) + 콜라보(브리지)
add('SAN14_Traits9.dll',P+'/out/SAN14_Traits9.dll','9|13',9)   # R42 A10: 9·13 공용 부품(어느 조합이든 같은 파일·이름)
add('SAN14_Traits9_UI.dll',P+'/out/UI_Traits9_Hover.dll','9|13',9)
add('SAN14_Traits9Check.dll',BO+'/SAN14_Traits9Check.dll','9|13',9)   # R42 A10: 9칸 못 붙으면 게임 안 오류 코드 창(읽기만)
add('SAN14Mods/modules/Traits9Check.ini',(reg('SAN14CE.Traits9Check','SAN14_Traits9Check.dll')+'[Traits9Check]\r\nNotice=1\r\n').encode(),'9|13',9)
add('SAN14Mods/ModPack/SAN14_Collab.dll',MP+'/SAN14_Collab.dll','9',9)
# 모음팩 브리지 (9·11·14·17)
add('SAN14_ModPackBridge.dll',P+'/bridge/SAN14_ModPackBridge.dll','9|11|14|17',14)
# 10번 추가 3D 모델 (R35): 옛 통합본 ID 25~173 — 1.0.1 색인 + 새 레코드 합본 색인 3개, 새 레코드만 담은 컨테이너, 모델 목록
M10='/home/claude/m10/out'
for fn in sorted(os.listdir(M10+'/0002')): add('0002/'+fn,M10+'/0002/'+fn,'10',10)
add('San14ModelRegistry.tsv',M10+'/San14ModelRegistry.tsv','10',10)
# 11번 필드 3D (12번 안개와 함께면 안개 연동판)
add('FieldLeader3D_Managed20/FieldLeader3D_Runtime.exe',pick(R+'/field_out/normal/FieldLeader3D_Runtime.exe',P+'/field_out/normal/FieldLeader3D_Runtime.exe'),'11&!12',11)
# 12번 안개 1.6
# 13번 논공 가챠
add('SAN14_MeritTraits.dll',R+'/mt/obj_new/SAN14_MeritTraits.dll','13',13)  # R40 A2/A12 + AI 저장 체인
add('SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll',P+'/mt_build/obj_new/SAN14_MeritResolver_CEKO.dll','13',13)
# R42 A10: 13번 폴더 사본(UI_Traits9_Hover.dll) 삭제 — 게임 폴더 공용 SAN14_Traits9_UI.dll 사용
add('SAN14Mods/modules/MeritTraits.ini',OLD+'/extras/14/SAN14Mods/modules/MeritTraits.ini','13',13)
# 14번 모음팩 설명 (9·13 없을 때 적용; 파일은 항상 두고 브리지 설정으로 결정)
add('SAN14Mods/ModPack/SAN14_TraitHover.dll',MP+'/SAN14_TraitHover.dll','14&!9&!13',14)
# 9·13번(개성 9개 UI)과 함께: 그 위에 얹혀 전법 설명만 띄우는 판 (th/th_coexist.py)
add('SAN14Mods/ModPack/SAN14_TraitHover.dll',R+'/a10/SAN14_TraitHover.dll.v3','14&9|14&13',14)   # R42 A10 v3: 공용 이름 SAN14_Traits9_UI.dll 위에 붙음
add('SAN14_전법설명.txt',MP+'/SAN14_TacticDescriptions.txt','14',14)
# 15번 연계 탭
add('SAN14_RelatedOfficersTab.dll',P+'/rel_build/SAN14_RelatedOfficersTab.dll','15',15)
add('SAN14Mods/resolvers/SAN14_RelatedOfficersResolver.dll',P+'/rel_build/SAN14_RelatedOfficersResolver.dll','15',15)
add('SAN14Mods/modules/RelatedOfficersTab.ini',OLD+'/extras/16/SAN14Mods/modules/RelatedOfficersTab.ini','15',15)
# 16번 출진 프리셋
add('SAN14_SortiePresets.dll',P+'/out/SAN14_SortiePresets.dll','16',16)
add('SAN14Mods/resolvers/SAN14_Resolver_PresetGroups_CEKO.dll',P+'/out/SAN14_Resolver_PresetGroups_CEKO.dll','16',16)
add('SAN14Mods/modules/SortiePresets.ini',OLD+'/extras/17/SAN14Mods/modules/SortiePresets.ini','16',16)
# 17번 능력치 200 (브리지)
add('SAN14Mods/ModPack/SAN14_Stats200.dll',MP+'/SAN14_Stats200.dll','17',17)
add('SAN14_Stats200Reader.dll',P+'/bridge/SAN14_Stats200Reader.dll','17',17)
add('SAN14Mods/modules/Stats200Reader.ini',reg('SAN14CE.Stats200Reader','SAN14_Stats200Reader.dll').encode(),'17',17)
# 18번 경험치 (게임 안 전환용 사본 포함)
EXP_MOD=OLD+'/extras/19/experience_data_exce.bin'; EXP_ORIG=PKG+'/data/experience_original.bin'
add('SAN14Mods/DataToggle/experience_original.bin',EXP_ORIG,'18',18)
add('SAN14Mods/DataToggle/experience_modified.bin',EXP_MOD,'18',18)
# 19번 120프레임 (모음팩판, ModCore 모듈로 등록)
add('SAN14_FPS120.dll',R39+'/fps120/SAN14_FPS120.dll','19',19)  # R40 A18: R39 성능판(매 프레임 readable 캐시) 2d476b9b
add('SAN14Mods/modules/FPS120.ini',reg('SAN14CE.FPS120','SAN14_FPS120.dll','; 모음팩 v1.7 FPS120 (CE 1.0.1)').encode(),'19',19)
# 20번 글꼴
add('0002/RRPreview.rdb',OLD+'/extras/21/RRPreview.rdb','20',20)
add('0002/RRPreview.rdb.bin99',OLD+'/extras/21/RRPreview.rdb.bin99','20',20)
# 21번 포로 귀환
add('SAN14_PrisonerReturn.dll',pick(BO+'/SAN14_PrisonerReturn.dll',P+'/build_out/SAN14_PrisonerReturn.dll'),'21',21)
add('SAN14Mods/modules/PrisonerReturn.ini',OLD+'/extras/23/SAN14Mods/modules/PrisonerReturn.ini','21',21)
# 22번 내정 자동화
add('SAN14_DomesticCE.dll',P+'/out/SAN14_DomesticCE.dll','22',22)  # 파일 그대로, R40 A14 는 ICGuard 가 실행 중에만
add('SAN14_DomesticCE.ini',OLD+'/extras/24/SAN14_DomesticCE.ini','22',22)
add('SAN14Mods/modules/DomesticCE.ini',OLD+'/extras/24/SAN14Mods/modules/DomesticCE.ini','22',22)
add('SAN14_DomesticTalent.dll',pick(BO+'/SAN14_DomesticTalent.dll',P+'/build_out/SAN14_DomesticTalent.dll'),'22',22)
add('SAN14Mods/modules/DomesticTalent.ini',OLD+'/extras/24/SAN14Mods/modules/DomesticTalent.ini','22',22)
# 23번 도시 상한
add('SAN14_CityCaps.dll',P+'/bridge/SAN14_CityCaps.dll','23',23)
add('SAN14Mods/modules/CityCaps.ini',reg('SAN14CE.CityCaps','SAN14_CityCaps.dll').encode(),'23',23)
# 24번 수여 능력치 미적용 (게임 안 전환용 사본)
add('SAN14Mods/DataToggle/fixdata_original.s14',PKG+'/data/fixdata_original_101.s14','24',24)
add('SAN14Mods/DataToggle/fixdata_nostat.s14',PKG+'/data/fixdata_nostat_101.s14','24',24)
# 25번 Patch27
add('SAN14_Patch27.dll',pick(BO+'/SAN14_Patch27.dll',P+'/build_out/SAN14_Patch27.dll'),'1|25',25)
add('SAN14Mods/modules/Patch27.ini',OLD+'/extras/28/SAN14Mods/modules/Patch27.ini','1|25',25)
# ---------- R39 새 항목 ----------
# 기본: 새 모드 설정 창 + 남의 DLL 다리
add('SAN14_ModSettings.dll',pick(BO+'/SAN14_ModSettings.dll'),'base',0)
add('SAN14Mods/modules/ModSettings.ini',pick(PL+'/base/SAN14Mods/modules/ModSettings.ini'),'base',0)
add('SAN14_ExtBridge.dll',PL+'/base/SAN14_ExtBridge.dll','base',0)
add('SAN14Mods/modules/ExtBridge.ini',PL+'/base/SAN14Mods/modules/ExtBridge.ini','base',0)
addtree(PL+'/f04','4',4)     # R42 B1~B3 4번 연출 생략(SAN14_SceneSkip.dll)
addtree(PL+'/f26','26',26)   # 기능확장 v1.5 3종
# R40: 1번 통합편의 장기 안정(ICGuard) — 통합편의 본체와 같이 늘 설치
add('SAN14_ICGuard.dll',PL+'/base/SAN14_ICGuard.dll','base',0)
add('SAN14Mods/modules/ICGuard.ini',PL+'/base/SAN14Mods/modules/ICGuard.ini','base',0)
addtree(PL+'/f27','27',27)   # 독립 미니맵
addtree(PL+'/f28','28',28)   # 인구
addtree(PL+'/f30','30',30)   # 편의성 4종
# 31번 그래픽: 새 컨테이너만 payload 로(색인은 설치기가 레코드 단위로 바꿈), 레코드는 f31/records
add('0002/ScreenLayout.rdb.bin3375',R+'/f31/0002/ScreenLayout.rdb.bin3375','31',31)
add('0002/ScreenLayout.rdb',R+'/f31/ScreenLayout.rdb.patched_check','31',31)  # 합본 색인(1.0.1 원본 + 레코드 9개), 설치기가 10번 방식으로 처리
addtree(PL+'/f32','32',32)   # 고용무장 얼굴
if _os.path.isdir(PL+'/f33'): addtree(PL+'/f33','33',33)
if _os.path.isdir(PL+'/f29'): addtree(PL+'/f29','29',29)
if _os.path.isdir(PL+'/f34'): addtree(PL+'/f34','34',34)
# ---------- 쓰기 ----------
PAY=PKG+'/support/payload'
if os.path.exists(PAY): shutil.rmtree(PAY)
seen={}
rows=[]
for target,data,when,id_ in F:
    group='base' if id_==0 else 'f%02d'%id_
    rel=f'support/payload/{group}/{target}'
    if rel in seen:  # 같은 대상에 변형이 둘(11번 normal/fog, 공통 Scenic): 변형 이름 붙임
        n=2
        while f'{rel}.v{n}' in seen: n+=1
        rel=f'{rel}.v{n}'
    seen[rel]=1
    full=os.path.join(PKG,rel); os.makedirs(os.path.dirname(full),exist_ok=True); open(full,'wb').write(data)
    rows.append((target,rel,sha(data),when,id_))
# 10번: 1.0.1 원본 색인(제거·복원용, 1번 ZIP) + 무장 연결(게임 폴더 밖 %LOCALAPPDATA% 에 설치기가 합침)
os.makedirs(PKG+'/data/model101',exist_ok=True)
for n in ('CharacterEditor.rdb','MaterialEditor.rdb','KIDSSystemResource.rdb'): shutil.copy(M10+'/orig101/'+n,PKG+'/data/model101/'+n)
os.makedirs(PAY+'/f10',exist_ok=True); shutil.copy(M10+'/links/OfficerModels_R35.tsv',PAY+'/f10/OfficerModels_R35.tsv')
os.makedirs(PKG+'/data/screen101',exist_ok=True)
shutil.copy(R39+'/pkg/data/screen101/ScreenLayout.rdb',PKG+'/data/screen101/ScreenLayout.rdb')  # R40: 1.0.1 원본(R39 와 같은 파일). uploads 사본은 바뀌어 있을 수 있어 쓰지 않음
# 로더
for n in ('dxgi.dll','version.dll','dinput8.dll'):
    full=PKG+'/support/payload/loaders/'+n; os.makedirs(os.path.dirname(full),exist_ok=True); shutil.copy(OLD+'/payload/loaders/'+n,full)
# 18번 교체 파일 (설치기가 직접 참조)
os.makedirs(PKG+'/support/payload/f18',exist_ok=True); shutil.copy(EXP_MOD,PKG+'/support/payload/f18/experience_data_exce.bin')
# ---------- Known 해시 (R20 이하 설치기가 설치했던 파일: 덮어쓰기/정리 허용) ----------
known={}
def k(h,note):
    if re.fullmatch(r'[0-9a-f]{64}',h): known.setdefault(h,note)
for h in re.findall(r'(?:sha256|accepted)="([^"]*)"',open(OLD+'/ExtraCatalog.xml').read()):
    for x in h.split(','): k(x,'R20 extras')
for fn in os.listdir(OLD+'/installer_r3_3'):
    if fn.endswith('.cs'):
        for x in re.findall(r'"([0-9a-f]{64})"',open(OLD+'/installer_r3_3/'+fn,encoding='utf-8-sig').read()): k(x,'R20 installer')
for root_,_,files in os.walk(OLD+'/payload'):
    for fn in files: k(shaf(os.path.join(root_,fn)),'loader '+fn if 'loaders' in root_ else 'R20 payload '+fn)
for root_,_,files in os.walk(OLD+'/extras'):
    for fn in files: k(shaf(os.path.join(root_,fn)),'R20 extras '+fn)
for fn in os.listdir(P+'/orig_payload'):
    if fn!='bgm_runtime.zip': k(shaf(P+'/orig_payload/'+fn),'R20 resource '+fn)
for h in ('653aaaea26842224533ec31d590562712ede9785ed2fe418be2ae85fb5974c8f','4222090ff39da6c44750929452452be2b28ced50c00fa1548cecb43ddfe12eb2','7868521d4c6b86c5aa24007031a6f40a8ea78eb5ab7df189f93dda7e2ef99d2e','25a5ee0820a6029e04868d5f0744b08d638472aa7a5c2a152aa39fb294683dec'): known[h]='loader (R20)'
# 옛 설치기가 만들던 등록 파일 텍스트
for id_,file in [('SAN14CE.IntegratedConvenience.Compat','SAN14_IntegratedConvenience.dll'),('SAN14CE.SettingsHelpScenic','SAN14_SettingsHelpScenic.dll'),('SAN14CE.AutoPersonnelRepair','SAN14_AutoPersonnelRepair.dll'),('SAN14CE.DualCreditSaveRepair','SAN14_DualCreditSaveRepair.dll'),('SAN14CE.Stats200CEKO','SAN14_Stats200_CEKO.dll'),('SAN14CE.Compat20.Entry','SAN14_Compat20_Entry.dll')]:
    k(sha(reg(id_,file).encode()),'R20 module ini')
# 옛 공용 데이터 변형 (1.0.1 이전 원본·봉인판)
sys.path.insert(0,P); import lwc,base64
old_fix=open(OLD+'/recovery_r8/fixdata_original.s14','rb').read(); k(sha(old_fix),'old fixdata original')
raw,t,h_=lwc.decode(old_fix); raw=bytearray(raw)
spec=open(R+'/installer101/Stat.cs').read()
for m in re.finditer(r'Offset=(\d+),Before=Convert\.FromBase64String\("([^"]+)"\)',spec):
    off=int(m.group(1)); bef=base64.b64decode(m.group(2)); raw[off:off+len(bef)]=bytes(len(bef))
k(sha(lwc.encode(bytes(raw),t,h_)),'old fixdata nostat')
# ---------- 값 ----------
V={'fix_original':shaf(PKG+'/data/fixdata_original_101.s14'),'fix_nostat':shaf(PKG+'/data/fixdata_nostat_101.s14'),
   'exp_original':shaf(EXP_ORIG),'exp_modified':shaf(EXP_MOD),
   'font_original':'a9bb649b4405a065f6330e70975a85a559cb8523af6c0df3ec63ea16685c6ae6','font_rdb':shaf(OLD+'/extras/21/RRPreview.rdb'),
   'loader_dxgi.dll':shaf(OLD+'/payload/loaders/dxgi.dll'),'loader_version.dll':shaf(OLD+'/payload/loaders/version.dll'),'loader_dinput8.dll':shaf(OLD+'/payload/loaders/dinput8.dll')}
for n in ('CharacterEditor.rdb','MaterialEditor.rdb','KIDSSystemResource.rdb'): V['model_orig_'+n]=shaf(PKG+'/data/model101/'+n)
V['screen_orig_ScreenLayout.rdb']=shaf(PKG+'/data/screen101/ScreenLayout.rdb')
V['officer_links']=shaf(PAY+'/f10/OfficerModels_R35.tsv')
for key in ('fix_nostat','exp_modified','font_rdb'): k(V[key],key)  # 원본 해시는 Known 에 넣지 않음(Known = 우리 파일만)
# R21(1.0.1 첫 판) 이 설치한 파일 + 2026-10-17 진단 중 게임 폴더에 직접 넣은 파일(튕김 기록기, 3D 수정판): 덮어쓰기/정리 허용
for fn in os.listdir(P+'/prev_manifests'):
    for x in re.findall(r'(?:sha256|after)="([0-9a-f]{64})"',open(P+'/prev_manifests/'+fn,encoding='utf-8-sig').read()):
        if x not in (V['fix_original'],V['exp_original'],V['font_original']): k(x,'R21 '+fn)
k('ab27c4a7a9afe3ac6d6cf65a104b4c02dde950cb61de129030397351a83975d4','R21 hotfix FieldLeader3D_Runtime (3D camera address)')
k('5c6a8fad45af0fb1958f888737857702d13ea332e3d0933504ae2d4e7bfa1864','R21 hotfix SAN14_CrashLog')
k('5923c4c1007805caf12a6e80c163fc2f01a7cde060689b8ef563abcfe9bba617','R21 hotfix CrashLog.ini')
# ---------- manifest ----------
# R40: R39 정식판·R40 AI 호환판(10/28 게임 폴더에 넣은 것)이 설치한 파일 → 덮어쓰기/정리 허용
for x in re.findall(r'(?:sha256|after)="([0-9a-f]{64})"',open(R39+'/pkg/support/manifest101.xml',encoding='utf-8-sig').read()):
    if x not in (V['fix_original'],V['exp_original'],V['font_original']): k(x,'R39 manifest')
# R41: R40 정식판(10/28 20시 다시 묶은 판 포함)이 설치한 파일도
for x in re.findall(r'(?:sha256|after)="([0-9a-f]{64})"',open('/home/claude/r40/pkg/support/manifest101.xml',encoding='utf-8-sig').read()):
    if x not in (V['fix_original'],V['exp_original'],V['font_original']): k(x,'R40 manifest')
for p_ in ['/home/claude/r40ai/ic/ic_r40.dll','/home/claude/r40ai/mt/obj_new/SAN14_MeritTraits.dll','/home/claude/r40ai/draggive/out/SAN14_DragGive.dll','/home/claude/r40ai/ms/out40/SAN14_ModSettings.dll','/home/claude/r40ai/ms/out40/SAN14_Patch27.dll','/home/claude/r40ai/ms/out40/SAN14_AutoPersonnelRepair.dll']:
    if os.path.exists(p_): k(shaf(p_),'R40 AI compat '+os.path.basename(p_))
lines=['<?xml version="1.0" encoding="utf-8"?>','<Manifest version="R42d" game="%s">'%shaf(P+'/new.exe')]
for target,rel,h,when,id_ in rows: lines.append(f'  <File target={quoteattr(target)} source={quoteattr(rel)} sha256="{h}" when={quoteattr(when)} id="{id_}" />')
for n,v in sorted(V.items()): lines.append(f'  <Value name="{n}" value="{v}" />')
for h,note in sorted(known.items()): lines.append(f'  <Known sha256="{h}" note={quoteattr(note)} />')
lines.append('</Manifest>')
mb=('\n'.join(lines)+'\n').encode('utf-8')
os.makedirs(PKG+'/support',exist_ok=True); open(PKG+'/support/manifest101.xml','wb').write(mb)
# 설치기 소스 + 해시 상수
INST=PKG+'/support/installer101'
if os.path.exists(INST): shutil.rmtree(INST)
shutil.copytree(R+'/installer101',INST)
open(INST+'/ManifestHash.cs','w').write('namespace San14Patch101 { public static class ManifestHash { public const string Value="%s"; } }\n'%sha(mb))
print('files',len(rows),'known',len(known),'manifest',sha(mb))
json.dump({'rows':rows,'values':V},open(R+'/pkg_rows.json','w'),ensure_ascii=False,indent=1)

# ---------- 최종 조립: 설치기 소스 정리 · 설치.cmd · README · SHA256SUMS · 분할 ZIP ----------
for fn in os.listdir(INST):
    if fn not in ('Core.cs','Spec.cs','Stat.cs','Engine.cs','ManifestHash.cs','App.cs','BuildAndRun.ps1'): os.remove(os.path.join(INST,fn))
open(PKG+'/설치.cmd','wb').write(open(pick(R+'/설치.cmd.src',P+'/pkg101_INSTALL.cmd'),encoding='utf-8').read().replace('chcp 65001 >nul\n','').replace('\n','\r\n').encode('cp949'))
open(PKG+'/README_KO.txt','wb').write(('﻿'+open(pick(R+'/README_R41.txt',P+'/pkg101_README.txt'),encoding='utf-8').read()).replace('\r\n','\n').replace('\n','\r\n').encode('utf-8'))
for stale in ('SHA256SUMS.txt',):
    if os.path.exists(PKG+'/'+stale): os.remove(PKG+'/'+stale)
allfiles=[]
for root_,_,files in os.walk(PKG):
    for fn in files: allfiles.append(os.path.relpath(os.path.join(root_,fn),PKG).replace(os.sep,'/'))
allfiles.sort()
open(PKG+'/SHA256SUMS.txt','w',encoding='utf-8').write(''.join('%s  %s\n'%(shaf(os.path.join(PKG,f)),f) for f in allfiles))
allfiles.append('SHA256SUMS.txt')
# 분할 ZIP (각각 독립 ZIP; 전부 같은 폴더에 풀면 완성). 압축 후 크기 약 55MB 이하씩
import zlib
OUT=R+'/dist'
if os.path.exists(OUT): shutil.rmtree(OUT)
os.makedirs(OUT)
TAG='SAN14CE_통합패치_R42d_1.0.1'
sizes={f:len(zlib.compress(open(os.path.join(PKG,f),'rb').read(),6)) for f in allfiles}
# 작은 파일(설치기·문서·manifest)은 1번에 몰고, 큰 것은 크기순으로 채움
# R29: 어떤 번호를 고르든 항상 필요한 자료(공통 런타임 base·로더·데이터 원본)는 무조건 1번 ZIP 에 (2번째 ZIP 없이도 기본 설치가 막히지 않게)
must=lambda f:f.startswith(('support/payload/base/','support/payload/loaders/','data/'))
small=[f for f in allfiles if sizes[f]<200000 or must(f)]; model=lambda f:f.startswith('support/payload/f10/0002/')  # R35: 10번 추가 모델 자료는 뒤쪽 ZIP 에 모음(앞 ZIP 만으로 나머지 전부 설치 가능)
big=sorted([f for f in allfiles if not(sizes[f]<200000 or must(f))],key=lambda f:(model(f),-sizes[f]))
LIMIT=55*1024*1024
parts=[list(small)]; cur=sum(sizes[f] for f in small)
for i,f in enumerate(big):
    if (cur+sizes[f]>LIMIT or (model(f) and i>0 and not model(big[i-1]))) and parts[-1]: parts.append([]); cur=0
    parts[-1].append(f); cur+=sizes[f]
n=len(parts)
for i,files in enumerate(parts,1):
    zp=os.path.join(OUT,f'{TAG}_{i}of{n}.zip')
    with zipfile.ZipFile(zp,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for f in files: z.write(os.path.join(PKG,f),f,compress_type=zipfile.ZIP_STORED if '.rdb.bin' in f else zipfile.ZIP_DEFLATED)
    print('zip',os.path.basename(zp),os.path.getsize(zp)//1048576,'MB',len(files),'files')
