# 새 판 payload 의 바이너리(바이트 교체된 남의 모듈) 해시를 우리 소스의 '아는 판' 표에 반영한다. (R43 도구)
#  대상: common.h known_common(IC) 에 r43 추가 / DomesticCE 해시(domestic_talent·mod_settings·patch27_ui) 교체 / notice.cpp kResolver(IP01) 교체
#  사용: python3 update_hashes.py <payload 폴더> <r41/src>     (바뀐 파일·해시를 적는다. 다시 돌려도 같은 결과)
import sys,os,re,hashlib,glob
PAY,SRC=sys.argv[1:3]
def sha(path):return hashlib.sha256(open(path,'rb').read()).digest()
def find(name):
    r=glob.glob(os.path.join(PAY,'**',name),recursive=True);assert len(r)==1,(name,r);return r[0]
def arr(b):return ','.join('0x%02x'%x for x in b)
def sub(path,pairs):
    t=open(path,encoding='utf-8').read();o=t
    for a,b in pairs:
        assert t.count(a)>=1,(path,a[:50]);t=t.replace(a,b)
    if t!=o:open(path,'w',encoding='utf-8').write(t);print('고침',os.path.basename(path))
    else:print('그대로',os.path.basename(path))
ic=sha(find('SAN14_IntegratedConvenience.dll'));dce=sha(find('SAN14_DomesticCE.dll'));ip=sha(find('SAN14_Resolver_IP01.dll'))
print('IC',ic.hex()[:16],'DomesticCE',dce.hex()[:16],'IP01',ip.hex()[:16])
# 1) common.h: r43 줄 추가(이미 있으면 교체) + known_common 의 || 목록에 포함
p=os.path.join(SRC,'common.h');t=open(p,encoding='utf-8').read()
line=' static const U8 r43[]={'+arr(ic)+'}; // R43: r40 + 새 게임 판(2026-11-05) 주소 바이트 교체(upd_tools/apply_new.py)\n'
if 'static const U8 r43[]' in t:t=re.sub(r' static const U8 r43\[\]=\{[^}]*\};[^\n]*\n',line,t)
else:t=t.replace(' HMODULE m=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");return (module_sha(m,p1)',line+' HMODULE m=GetModuleHandleW(L"SAN14_IntegratedConvenience.dll");return (module_sha(m,p1)')
if 'module_sha(m,r43)' not in t:t=t.replace('module_sha(m,r40))?','module_sha(m,r40)||module_sha(m,r43))?')
open(p,'w',encoding='utf-8').write(t);print('common.h r43 =',ic.hex()[:16])
# 2) DomesticCE 해시(세 파일 같은 배열) — 옛 a1cd31a7… 또는 이전에 바꾼 값 → 새 값
OLD_DCE_PREFIX='0xa1,0xcd,0x31,0xa7,0x24,0x74,0x03,0x44'
for f in('domestic_talent.cpp','mod_settings.cpp','patch27_ui.cpp'):
    p=os.path.join(SRC,f);t=open(p,encoding='utf-8').read()
    m=re.search(r'static const U8 sha\[\]=\{((?:0x[0-9a-fA-F]{1,2},){31}0x[0-9a-fA-F]{1,2})\}',t)
    assert m,f
    new='static const U8 sha[]={'+arr(dce)+'}'
    if m.group(0)!=new:t=t.replace(m.group(0),new+' /* R43 DomesticCE(새 판 바이트 교체) */' if '/* R43 DomesticCE' not in t else new);open(p,'w',encoding='utf-8').write(t);print('고침',f)
    else:print('그대로',f)
# 3) notice.cpp kResolver
p=os.path.join(SRC,'notice.cpp');t=open(p,encoding='utf-8').read()
m=re.search(r'const U8 kResolver\[32\]=\{([^}]*)\};',t);assert m
new='const U8 kResolver[32]={'+arr(ip)+'};'
if m.group(0)!=new:t=t.replace(m.group(0),new);open(p,'w',encoding='utf-8').write(t);print('고침 notice.cpp kResolver')
else:print('그대로 notice.cpp')

# 4) R43: traits9_check.cpp kUiSha = 새 판 SAN14_Traits9_UI.dll 해시 (2-1 때 빠뜨렸던 것 — 9칸 요약 화면 보강이 통째로 꺼질 뻔)
ui=sha(find('SAN14_Traits9_UI.dll'));p=os.path.join(SRC,'traits9_check.cpp');t=open(p,encoding='utf-8').read()
t2=re.sub(r'static const U8 kUiSha\[32\]=\{[^}]*\};','static const U8 kUiSha[32]={'+arr(ui)+'};',t)
t2=t2.replace(re.search(r'"R43 Traits9_UI build \([0-9a-f]{8}…\)"',t2).group(0) if re.search(r'"R43 Traits9_UI build \([0-9a-f]{8}…\)"',t2) else '@@','"R43 Traits9_UI build (%s…)"'%ui.hex()[:8])
if t2!=t:open(p,'w',encoding='utf-8').write(t2);print('고침 traits9_check.cpp kUiSha =',ui.hex()[:16])
else:print('그대로 traits9_check.cpp kUiSha =',ui.hex()[:16])

# 5) R43 시험7: 13번 MeritTraits 리졸버 hover_text_hash = 새 판 SAN14_Traits9_UI.dll .text(VirtualSize) sha256.
#    Traits9_UI 를 고칠 때마다(시험7 개성 탭 0x2fc4) 같이 바뀌어야 함 — 안 바꾸면 13번 공유 hover 가 조용히 안 붙는다.
import struct
d=open(find('SAN14_Traits9_UI.dll'),'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];so=pe+24+struct.unpack_from('<H',d,pe+20)[0]
for i in range(struct.unpack_from('<H',d,pe+6)[0]):
    s=d[so+40*i:so+40*i+40]
    if s[:5]==b'.text':vs,va,rs,rp=struct.unpack_from('<IIII',s,8);th=hashlib.sha256(d[rp:rp+vs]).digest()
MR=os.path.join(os.path.dirname(SRC.rstrip("/")),"mt","merit_resolver.c")   # 원본은 mt/merit_resolver.c (build_new.sh 가 src/ 로 복사해 덮어씀)
if os.path.exists(MR):
    t=open(MR,encoding='utf-8').read()
    hx=','.join(hex(x) for x in th)
    t2=re.sub(r'(\}\},\{)((?:0x[0-9a-f]{1,2},){31}0x[0-9a-f]{1,2})(\},\{16,16,)',lambda m:m.group(1)+hx+m.group(3),t,count=1)
    if t2!=t:open(MR,'w',encoding='utf-8').write(t2);print('고침 merit_resolver.c hover_text_hash =',th.hex()[:16])
    else:print('그대로 merit_resolver.c hover_text_hash =',th.hex()[:16])
