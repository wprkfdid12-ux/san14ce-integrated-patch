# 다른 모더용 '통합패치 R43 호환 자료' 정리판 만들기 (사용자 11-09 "정리판")
#  넣음: 우리 모듈 원소스(빌드하면 배포판과 바이트까지 같은 25개) + 빌드 도구 + 옛 판 소스(15번) + 호환표 + 안내문
#  뺌: 소스 없는 남의/바이너리 파일, AI 모드 작업물, 개인 경로·작업 메모·시험용 모듈
import os,re,shutil,struct,hashlib,pickle,collections,sys,subprocess
R='/home/claude/r41';K=sys.argv[1];shutil.rmtree(K,ignore_errors=True);os.makedirs(K)
sys.path.insert(0,R+'/upd_tools');import newmap as X
def cp(src,dst):
    os.makedirs(os.path.dirname(dst),exist_ok=True);shutil.copy2(src,dst)
# ---------- 1) 소스
SKIP={'patch_tester.cpp'}
for f in sorted(os.listdir(R+'/src')):
    p=R+'/src/'+f
    if os.path.isfile(p) and f not in SKIP and not f.endswith(('.o','.bak')) and '.bak' not in f:cp(p,K+'/소스/src/'+f)
for f in ['crt_min.c','chkstk.S']:cp(R+'/build_tc/src/'+f,K+'/소스/build_tc/src/'+f)
for f in ['windows.h','string.h','bcrypt.h']:cp(R+'/build_tc/inc/'+f,K+'/소스/build_tc/inc/'+f)
for f in ['kernel32.def','user32.def','bcrypt.def','libkernel32.a','libuser32.a','libbcrypt.a']:cp(R+'/build_tc/lib/'+f,K+'/소스/build_tc/lib/'+f)
mt=R+'/mt'
for f in ['build_new.sh','merit_resolver.c','chkstk.obj','libkernel32.a','libuser32.a','libbcrypt.a']:cp(mt+'/'+f,K+'/소스/mt/'+f)
for f in sorted(os.listdir(mt+'/src')):
    if os.path.isfile(mt+'/src/'+f) and f not in('README_SOURCE.txt','build.sh'):cp(mt+'/src/'+f,K+'/소스/mt/src/'+f)
ro=R+'/mt_test/references/related_officers_rc24/source'
for f in sorted(os.listdir(ro)):
    if os.path.isfile(ro+'/'+f):cp(ro+'/'+f,K+'/소스/옛판_1.0.1/related_officers_rc24/'+f)
# 30번 휠클릭·전법서 습득: 키트 소스(각자 build.sh 로 빌드 — 배포판과 바이트까지 같음). 빌드 산출물·시험 결과 글은 뺌
for kit in ('WheelRegion','DragLearn'):
    for dp,dn,fn in os.walk(R+'/kits/'+kit):
        rel=os.path.relpath(dp,R+'/kits/'+kit)
        if rel.split(os.sep)[0]=='out' or rel.replace(os.sep,'/').startswith('test/results'):continue
        for f in fn:
            if f.endswith(('.o','.obj','.bak')):continue
            cp(dp+'/'+f,K+'/소스/kits/'+kit+('' if rel=='.' else '/'+rel)+'/'+f)
# 빌드 스크립트(상대 경로판)
b=open(R+'/build_all.sh',encoding='utf-8').read()
b=b.replace('B=/home/claude/r41/build_tc; S=/home/claude/r41/src; O=/home/claude/r41/build_out','ROOT="$(cd "$(dirname "$0")" && pwd)"; B=$ROOT/build_tc; S=$ROOT/src; O=$ROOT/build_out')
b=re.sub(r'^build SAN14_PatchTester\.dll.*\n','',b,flags=re.M)
b=b.replace('# R40 전체 재빌드: r40/src → r40/build_out (최종 패키지 전 반드시 실행)','# 통합패치 R43 우리 모듈 전체 빌드: src → build_out (clang + lld, 윈도 SDK 필요 없음)\n# 필요: clang·lld·llvm-dlltool (리눅스·WSL 에서 확인). 실행: bash build_all.sh')
assert '/home/claude' not in b
open(K+'/소스/build_all.sh','w',encoding='utf-8').write(b)
# ---------- 2) 모듈 목록 + 소스 여부
x=open(R+'/overlay_r43/pkg/support/manifest101.xml',encoding='utf-8').read()
F=re.findall(r'<File target="([^"]+)" source="([^"]+)" sha256="([0-9a-f]+)" when="([^"]+)" id="(\d+)"',x)
SRC={'SAN14_Notice.dll':'src/notice.cpp','SAN14_SettingsHelpScenic.dll':'src/settings_help.cpp','SAN14_CrashLog.dll':'src/crash_log.c','SAN14_Stats200New.dll':'src/stats200_new.cpp',
 'SAN14_Diag.dll':'src/diag.cpp','SAN14_AutoPersonnelRepair.dll':'src/auto_personnel_repair.cpp','SAN14_AutoMerit.dll':'src/auto_merit.cpp','SAN14_Traits9Check.dll':'src/traits9_check.cpp',
 'SAN14_MeritTraits.dll':'mt/src/*','SAN14_MeritResolver_CEKO.dll':'mt/merit_resolver.c','SAN14_PrisonerReturn.dll':'src/prisoner_return.cpp','SAN14_DomesticTalent.dll':'src/domestic_talent.cpp',
 'SAN14_DomesticAI.dll':'src/domestic_ai.cpp','SAN14_DomesticPlayer.dll':'src/domestic_player.cpp','SAN14_Patch27.dll':'src/patch27*.cpp','SAN14_ModSettings.dll':'src/mod_settings.cpp',
 'SAN14_SceneSkip.dll':'src/scene_skip*.cpp','SAN14_ExecuteLock.dll':'src/execute_lock.cpp','SAN14_ICGuard.dll':'src/ic_guard.cpp','SAN14_ItemPicture.dll':'src/item_picture.cpp',
 'SAN14_FPS120Late.dll':'src/fps120_late.cpp','SAN14_WheelRegion.dll':'kits/WheelRegion (build.sh)','SAN14_DragLearn.dll':'kits/DragLearn (build.sh)',
 'SAN14_HiredFace.dll':'src/hired_face.cpp','SAN14_Range.dll':'src/range.cpp','SAN14_StatusColor.dll':'src/status_color.cpp','SAN14_RegionOfficer.dll':'src/region_officer.cpp'}
OLD={'SAN14_RelatedOfficersTab.dll':'옛판_1.0.1/related_officers_rc24 (새 판 파일은 주소표로 옮긴 바이너리)','SAN14_RelatedOfficersResolver.dll':'옛판_1.0.1/related_officers_rc24 (새 판 파일은 주소표로 옮긴 바이너리)'}
THIRD={'libcrypto-3-x64.dll','libffi-8.dll','python3.dll','python312.dll','vcruntime140_1.dll','vcruntime140.dll'}
NOSRC_NOTE={'SAN14_IntegratedConvenience.dll':'소스 없음(처음부터 바이너리). 확인 구간은 호환표 02~04','SAN14_DomesticCE.dll':'소스 없음(처음부터 바이너리)'}
rows=[];seen=set()
for t,s,h,w,i in F:
    if not t.lower().endswith('.dll'):continue
    n=t.split('/')[-1]
    if (n,h) in seen:continue
    seen.add((n,h))
    if n in SRC:st='소스 있음 · 다시 빌드하면 배포판과 바이트까지 같음';loc=SRC[n]
    elif n in OLD:st='옛 판 소스만';loc=OLD[n]
    elif n in THIRD:st='남의 실행 파일(파이썬 런타임)';loc='-'
    elif n in NOSRC_NOTE:st=NOSRC_NOTE[n];loc='-'
    else:st='바이너리만(1.0.1 판을 새 판 주소로 옮긴 것, 이 판 소스 없음)';loc='-'
    rows.append((i,n,t,h,st,loc))
with open(K+'/모듈목록_소스여부.tsv','w',encoding='utf-8') as f:
    f.write('설치기번호(0=기본)\tDLL\t설치 위치(게임 폴더 기준)\tsha256\t소스\t소스 위치(소스/ 아래)\n')
    for r in rows:f.write('\t'.join(r)+'\n')
os.makedirs(K+'/호환표',exist_ok=True)
# ---------- 3) 호환표 01: 우리 모듈이 바꾸는 게임 자리 (ModCore runtime.log 최근 세션 합집합)
OWN=[('SAN14CE.MeritTraits','13','SAN14_MeritTraits.dll'),('SAN14CE.Patch27','25','SAN14_Patch27.dll'),('SAN14CE.Range','33','SAN14_Range.dll'),('SAN14CE.SceneSkip','4','SAN14_SceneSkip.dll'),
 ('SAN14CE.Stats200New','17','SAN14_Stats200New.dll'),('RelatedOfficersTab','15','SAN14_RelatedOfficersTab.dll'),('SAN14CE.StatusColor','29','SAN14_StatusColor.dll'),('SortiePresets','16','SAN14_SortiePresets.dll'),
 ('SAN14CE.ModSettingsUI','0','SAN14_ModSettings.dll'),('SAN14CE.DragLearn','30','SAN14_DragLearn.dll'),('SAN14CE.DragGive','30','SAN14_DragGive.dll'),('SAN14CE.RegionOfficer','34','SAN14_RegionOfficer.dll'),
 ('SAN14CE.SettingsHelp','0','SAN14_SettingsHelpScenic.dll'),('SAN14CE.Scenery','0','SAN14_SettingsHelpScenic.dll'),('SAN14CE.CityCaps','23','SAN14_CityCaps.dll'),('SAN14CE.FPS120','19','SAN14_FPS120.dll'),
 ('SAN14CE.PrisonerReturn','21','SAN14_PrisonerReturn.dll'),('SAN14CE.HiredFace','32','SAN14_HiredFace.dll'),('SAN14CE.WheelRegion','30','SAN14_WheelRegion.dll'),('SAN14CE.ExecuteLock','26','SAN14_ExecuteLock.dll'),
 ('SAN14CE.DomesticTalent','22','SAN14_DomesticTalent.dll'),('SAN14CE.DomesticAI','22','SAN14_DomesticAI.dll'),('SAN14CE.ItemPicture','30','SAN14_ItemPicture.dll')]
# 교훈 L110: 기록 머리줄의 판 글자는 새 판에서도 옛 값 그대로라 판 구분에 쓰면 안 됨(11-09 판은 옛 판 주소 209줄이 섞였음).
#   자리 뽑기는 교훈_점검/도구/자리표_만들기.py 하나만 씀(새 판에만 있는 모듈이 처음 나온 기록부터).
sys.path.insert(0,R+'/교훈_점검/도구');import 자리표_만들기 as ST
_s,_nnew,_nall,_unk=ST.load('/mnt/user-data/uploads/SAN14CE_ModCore/runtime.log')
assert not _unk,('주인 모르는 기록 이름',_unk)
sites={k:(v[0],v[1],v[2]) for k,v in _s.items()}
_add,_drop=ST.load_extra()                      # 이번 판에서 새로 넣은 자리(게임 기록에 아직 없음, L118)
for k in _drop:sites.pop(k,None)
for r_,l_,dll_,no_,nm_ in _add:sites[(r_,l_)]=(nm_,no_,dll_)
TEXT=[s for s in X.NEW_SECS]
def kind(r,l):
    for vs,va,rs,ra in X.NEW_SECS:
        if va<=r<va+vs:
            idx=X.NEW_SECS.index((vs,va,rs,ra))
            return ['코드','?','?'][0] if idx==0 else ('함수표 칸(포인터)' if l==8 else '자료')
    return '?'
pe=struct.unpack_from('<I',X.NEW_D,0x3c)[0];ns=struct.unpack_from('<H',X.NEW_D,pe+6)[0];opt=struct.unpack_from('<H',X.NEW_D,pe+20)[0]
SN=[(X.NEW_D[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<II',X.NEW_D,pe+24+opt+40*i+8) for i in range(ns)]
def secname(r):
    for n,vs,va in SN:
        if va<=r<va+vs:return n
    return '?'
with open(K+'/호환표/01_우리모듈_바꾸는자리.tsv','w',encoding='utf-8') as f:
    f.write('게임 RVA\t길이\t구역\t원래 바이트\t바꾸는 방식\t설치기번호\tDLL\t기록 이름(runtime.log)\n')
    for (r,l),(o,n,dl) in sorted(sites.items()):
        sec=secname(r);how='함수표 칸(포인터)을 우리 함수로' if sec!='.text' and l==8 else ('코드: 우리 쪽으로 넘기는 jmp/call(남는 바이트 nop)' if sec=='.text' and l>=5 else ('코드: 짧은 값/조건 바꿈' if sec=='.text' else '자료 값 바꿈'))
        if sec=='.text' and o.endswith('.Caps'):how='코드: 값/조건 바꿈(같은 길이)'
        f.write('0x%x\t%d\t%s\t%s\t%s\t%s\t%s\t%s\n'%(r,l,sec,X.nrd(r,l).hex(' '),how,n,dl,o))
os.makedirs(K+'/호환표',exist_ok=True)
# 14번 TraitHover 152칸 / 9번 Traits9_UI 146칸 (각 DLL 안의 표)
def dll_rd(path):
    d=open(path,'rb').read();p=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,p+6)[0];o=struct.unpack_from('<H',d,p+20)[0]
    S=[struct.unpack_from('<IIII',d,p+24+o+40*i+8) for i in range(n)]
    def at(r):
        for vs,va,rs,ra in S:
            if va<=r<va+max(vs,rs):return ra+r-va
    return d,at
PAY=R+'/overlay_r43/pkg/support/payload'
def find(name):
    for root,_,fs in os.walk(PAY):
        if name in fs:return os.path.join(root,name)
    for root,_,fs in os.walk(R+'/newpkg/payload_new'):
        if name in fs:return os.path.join(root,name)
th=find('SAN14_TraitHover.dll');d,at=dll_rd(th)
with open(K+'/호환표/05_14번_개성전법설명_함수표칸.tsv','w',encoding='utf-8') as f:
    f.write('게임 RVA(함수표 칸)\t원래 들어 있는 함수 RVA\t종류 번호\n')
    for i in range(0x98):
        s,org,k=struct.unpack_from('<III',d,at(0x62a0)+12*i);f.write('0x%x\t0x%x\t%d\n'%(s,org,k))
t9=find('SAN14_Traits9_UI.dll');d,at=dll_rd(t9)
with open(K+'/호환표/06_9번_개성9칸화면_함수표칸.tsv','w',encoding='utf-8') as f:
    f.write('게임 RVA(함수표 칸)\t원래 들어 있는 함수 RVA\t종류 번호\n')
    f.write('0x13affa8\t0x8ca1c0\t(첫 칸)\n')
    for i in range(145):
        s,org,k=struct.unpack_from('<III',d,at(0x910c)+12*i);f.write('0x%x\t0x%x\t%d\n'%(s,org,k))
# ---------- 4) 통합편의(IC) 확인 구간
ic=find('SAN14_IntegratedConvenience.dll');d,at=dll_rd(ic);ICB=0x180000000
with open(K+'/호환표/02_통합편의_확인구간.tsv','w',encoding='utf-8') as f:
    f.write('번호\t게임 RVA\t길이\tIC가 하는 일\t원래 바이트(16진)\tIC 적용 뒤 바이트(16진, 같으면 확인만)\n')
    r=0xdd240;i=0
    while r<0xde800:
        rva,ln,p1,p2=struct.unpack_from('<QQQQ',d,at(r));b1=d[at(p1-ICB):at(p1-ICB)+ln];b2=d[at(p2-ICB):at(p2-ICB)+ln] if ICB<=p2<ICB+0x300000 else b1
        f.write('%d\t0x%x\t%d\t%s\t%s\t%s\n'%(i,rva,ln,'확인만' if b1==b2 else '바꿈',b1.hex(),b2.hex()));r+=32;i+=1
with open(K+'/호환표/03_통합편의_후크자리.tsv','w',encoding='utf-8') as f:
    f.write('게임 RVA\t종류\t원래 대상 RVA\t원래 바이트\n')
    r=0xdedd0
    for i in range(51):
        a,tg,det,l,op=struct.unpack_from('<5Q',d,at(r))
        f.write('0x%x\t%s\t0x%x\t%s\n'%(a,'함수표 칸(8바이트)' if l==8 else ('call' if op==0xe8 else 'jmp' if op==0xe9 else hex(op)),tg,X.nrd(a,8 if l==8 else 5).hex(' ')));r+=0x28
with open(K+'/호환표/04_통합편의_포인터확인.tsv','w',encoding='utf-8') as f:
    f.write('게임 RVA(포인터 칸)\t들어 있어야 하는 함수 RVA\n')
    r=0xde800
    while r<0xdedd0:
        a,b_=struct.unpack_from('<QQ',d,at(r));f.write('0x%x\t0x%x\n'%(a,b_));r+=16
# 6번 진행 배속이 따로 확인하는 구간(전역 확인과 별개 — 19번 FPS120 의 일기토 호출 자리가 여기 0번 안에 있음, 11-09 제보)
with open(K+'/호환표/07_통합편의_6번진행배속_확인구간.tsv','w',encoding='utf-8') as f:
    f.write('번호\t게임 RVA\t길이\t원래 바이트(16진, 앞 32바이트)\n');n7=0
    for k in range(28):
        rva,ln,p1,p2=struct.unpack_from('<QQQQ',d,at(0xa3150+32*k))
        if not(0<rva<0x2a02000 and 0<ln<0x10000):continue
        f.write('%d\t0x%x\t%d\t%s\n'%(k,rva,ln,X.nrd(rva,min(ln,32)).hex()));n7+=1
    r=0xa34d0
    while True:
        a,b_=struct.unpack_from('<QQ',d,at(r))
        if not(0x1000<a<0x2a02000 and 0x1000<b_<0x2a02000):break
        f.write('포인터\t0x%x\t8\t(들어 있어야 하는 함수 RVA 0x%x)\n'%(a,b_));r+=16;n7+=1
print('07 표',n7,'줄')
print('표 완료',len(sites),'자리')
