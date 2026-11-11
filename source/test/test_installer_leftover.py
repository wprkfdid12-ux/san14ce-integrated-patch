# 정식판 뒤(11-08): 설치기 '기록 없이 남은 옛 판 파일 정리' 규칙 — 모형 시험(이 환경엔 C# 컴파일러가 없어 Engine.cs 규칙을 그대로 옮겨 실제 manifest·옛 판 파일로 판정)
#  실행: python3 test/test_installer_leftover.py
import re,hashlib,os,sys
R='/home/claude/r41';M=open(R+'/overlay_r43/pkg/support/manifest101.xml',encoding='utf-8').read()
eng=open(R+'/installer101/Engine.cs',encoding='utf-8').read()
entries=[(t,s,w,int(i)) for t,s,w,i in re.findall(r'<File target="([^"]+)" source="[^"]+" sha256="([0-9a-f]{64})" when="([^"]*)" id="(\d+)"',M)]
known={h for h in re.findall(r'<Known sha256="([0-9a-f]{64})"',M)}
retired=re.search(r'RetiredTargets=\{([^}]*)\}',eng).group(1);retired=[x.strip().strip('"') for x in retired.split(',')]
retired=[('SAN14Mods/modules/Traits9Check.ini' if x=='OldTraits9CheckIniRel' else x) for x in retired]
loaders={'dxgi.dll','version.dll','dinput8.dll'}
ourids=set(re.findall(r'"(SAN14CE\.[A-Za-z0-9_.]+)"',re.search(r'OurModuleIds=new HashSet<string>\(StringComparer\.Ordinal\)\{([^}]*)\}',eng).group(1)))
retired=[('SAN14Mods/modules/ModPackBridge.ini' if x=='BridgeIniRel' else x) for x in retired]
def applies(when,on):
    when=when.replace('&amp;','&')
    def term(t):
        t=t.strip()
        if t=='base':return True
        if t.startswith('!'):return not on.get(int(t[1:]),False)
        return on.get(int(t),False)
    return any(all(term(x) for x in part.split('&')) for part in when.split('|'))
def is_base(r):return r.lower() in loaders or any(i==0 and t.lower()==r.lower() for t,s,w,i in entries)
def foreign_kit(r):return any(t.lower()==r.lower() and i in (26,27,28,30) for t,s,w,i in entries)
def mutable(r):return r.lower() in {x.lower() for x in re.findall(r'"(SAN14Mods/modules/[^"]+\.ini|SAN14_[A-Za-z]+\.ini)"',eng.split('static readonly HashSet<string> RuntimeIni')[1].split(';')[0])}
def plan(files,on,records=()):
    desired={t for t,s,w,i in entries if applies(w,on)}
    bridge_desired=any(on.get(i,False) for i in (9,11,14,17))
    cand={t for t,s,w,i in entries if t not in desired}|{r for r in retired if r not in desired}|(set() if bridge_desired else {'SAN14Mods/modules/ModPackBridge.ini'})
    out=[]
    for r in sorted(cand,key=str.lower):
        if r in records or is_base(r) or foreign_kit(r) or r.startswith('data/'):continue
        if r not in files:continue
        data=files[r];h=hashlib.sha256(data).hexdigest()
        if r.startswith('SAN14Mods/modules/') and r.endswith('.ini'):
            m=re.search(rb'Id=([^\r\n]+)',data)
            if not m or m.group(1).decode().strip() not in ourids:continue
        else:
            if mutable(r):continue
            cur=any(t.lower()==r.lower() and s==h for t,s,w,i in entries)
            if not cur and h not in known:continue   # R43 8차: 지금 판 파일도 정리, 모르는 내용만 둠
        out.append(r)
    return out
fails=0
def check(c,m):
    global fails;print(('  ok   ' if c else '  FAIL ')+m);fails+=0 if c else 1
old='/mnt/user-data/uploads/통합패치/통합패치 11월2일 최종판/support/payload/f09/'
rd=lambda p:open(p,'rb').read()
on={i:True for i in (1,2,3,4,5,6,7,8,10,14,15,16,17,18,19,20)}   # 갤 제보처럼 9·13 안 고름
print('[1] 제보 상황: 9·13 안 고름 + 1.0.1 때 옛 9번 파일이 기록 없이 남음')
files={'SAN14_Traits9.dll':rd(old+'SAN14_Traits9.dll'),'SAN14_Traits9_UI.dll':rd(old+'SAN14_Traits9_UI.dll'),'SAN14_Traits9Check.dll':rd(old+'SAN14_Traits9Check.dll'),
       'SAN14Mods/modules/Traits9Check.ini':rd(old+'SAN14Mods/modules/Traits9Check.ini')}
p=plan(files,on)
check('SAN14_Traits9.dll' in p and 'SAN14_Traits9_UI.dll' in p,'옛 9번 파일 두 개 정리(통합편의가 부르던 것)')
check('SAN14Mods/modules/Traits9Check.ini' in p,'옛 등록 파일 정리')
print('   정리 목록:',p)
print('[2] 모르는 파일은 그대로 / 지금 판과 같은 파일(안 고른 항목, 기록 없음)은 정리 — R43 8차')
cur=[s for t,s,w,i in entries if t=='SAN14_Traits9.dll'][0]
files2={'SAN14_Traits9.dll':b'other modder build','SAN14_Traits9_UI.dll':rd('/home/claude/r41/overlay_r43/pkg/support/payload/f09/SAN14_Traits9_UI.dll') if os.path.exists('/home/claude/r41/overlay_r43/pkg/support/payload/f09/SAN14_Traits9_UI.dll') else rd(R+'/newpkg/payload_new/f09/SAN14_Traits9_UI.dll')}
p2=plan(files2,on);check('SAN14_Traits9.dll' not in p2,'모르는 내용(남의 모드 판 SAN14_Traits9.dll)은 안 지움')
check('SAN14_Traits9_UI.dll' in p2,'지금 판과 같은 9번 화면 부품(9·13 안 고름, 기록 없음)은 정리 — 남으면 9칸·14번이 엉킴')
on_no14=dict(on);on_no14[14]=False
th=[(t,s) for t,s,w,i in entries if t=='SAN14Mods/ModPack/SAN14_TraitHover.dll']
thd=rd(R+'/overlay_r43/pkg/support/payload/f14/SAN14Mods/ModPack/SAN14_TraitHover.dll')
p2b=plan({'SAN14Mods/ModPack/SAN14_TraitHover.dll':thd},on_no14);check('SAN14Mods/ModPack/SAN14_TraitHover.dll' in p2b,'14번 안 고름 + 지금 판 TraitHover 가 기록 없이 남음 → 정리')
p2c=plan({'SAN14Mods/ModPack/SAN14_TraitHover.dll':thd},on);check(p2c==[],'14번 고름 → 정리 대상 아님(설치 쪽)')
print('[3] 9번을 고르면 9번 파일은 정리 대상 아님, 옛 이름 등록 파일만 정리(두 번 등록 막기)')
on9=dict(on);on9[9]=True;p3=plan(files,on9)
check('SAN14_Traits9.dll' not in p3 and 'SAN14Mods/modules/Traits9Check.ini' in p3,'고른 항목 파일은 설치(교체) 쪽, 옛 이름 ini 만 정리')
print('[4] 설치 기록에 있는 파일은 기존 규칙(기록대로 제거)로 — 여기선 건드리지 않음')
p4=plan(files,on,records={'SAN14_Traits9.dll'});check('SAN14_Traits9.dll' not in p4,'기록 있음 → 이 정리에서 제외')
print('[5] 남의 등록 파일(Id 가 SAN14CE. 아님)은 그대로')
p5=plan({'SAN14Mods/modules/Traits9Check.ini':b'[Mod]\r\nId=Other.Mod\r\nFile=x.dll\r\n'},on);check(p5==[],'남의 Id → 안 지움')
print('[6] 남의 모드가 SAN14CE. Id 를 써도(기색 SAN14CE.Condition) 우리 목록 아니면 그대로 / 옛 시험 키트·모음팩 연결 등록 파일은 정리')
cond=open('/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14/SAN14Mods/modules/Condition.ini','rb').read()
p6=plan({'SAN14Mods/modules/Traits9Check.ini':cond},on);check(p6==[],'SAN14CE.Condition → 안 지움')
on0={i:True for i in (1,2,3)}
p7=plan({'SAN14Mods/modules/ExtBridgeTest.ini':open('/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14/SAN14Mods/modules/ExtBridgeTest.ini','rb').read(),'SAN14Mods/modules/ModPackBridge.ini':b'[Mod]\r\nId=SAN14CE.ModPackBridge\r\nFile=SAN14_ModPackBridge.dll\r\n'},on0)
check('SAN14Mods/modules/ExtBridgeTest.ini' in p7 and 'SAN14Mods/modules/ModPackBridge.ini' in p7,'옛 시험 키트 등록·안 쓰는 모음팩 연결 등록 정리')
p8=plan({'SAN14Mods/modules/ModPackBridge.ini':b'[Mod]\r\nId=SAN14CE.ModPackBridge\r\n'},on);check(p8==[],'14번 고르면 모음팩 연결 등록은 설치 대상(정리 안 함)')
print('\n'+('전부 통과' if not fails else '실패 %d'%fails));sys.exit(1 if fails else 0)
