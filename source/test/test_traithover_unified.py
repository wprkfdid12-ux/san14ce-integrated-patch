# R43 8차(사용자 11-09 "9·13·14번이 서로 기생하지 말고 따로 작동하게"): 14번 TraitHover 한 파일 + 9번 스스로 시작 — 오프라인 검사
#  ① 꾸러미의 TraitHover .dll 과 .v2 가 같은 파일(설치기가 9·13 유무로 고르던 갈림이 결과를 안 바꿈)
#  ② 진짜 DLL 코드 흉내(test/th_sim: PE 를 기본 주소에 올려 시작 함수 0x5070 · 시작 마스크 stubX · 칸 검사 0x4c80 를 그대로 실행)
#     화면 부품 없음 / 화면 부품이 먼저 / 화면 부품이 나중(TH 가 먼저 건 칸 때문에 화면 부품이 물러남) → 거부 0, 남의 DLL 은 여전히 거부
#  ③ 설명 고르기(0x21b4)가 .th8 갈림으로 가고, 갈림 조건이 시작 마스크와 같은 값(종류0 기대값 vs 게임+0x8ca1c0)
#  ④ 9칸 5자리·화면 가드가 통합편의 원본 확인 표(2,346곳)와 안 겹침(9번이 시작 때 바로 고쳐도 됨)
#  ⑤ 9번 확인 모듈 소스 규칙: 본체 SAN14_ApplyTraits9 를 부름, 14번 설정(ModPackBridge.ini)을 안 읽음
#  실행: python3 test/test_traithover_unified.py
import os,re,struct,subprocess,sys,hashlib
R='/home/claude/r41';P=R+'/newpkg/payload_new'
TH=P+'/f14/SAN14Mods/ModPack/SAN14_TraitHover.dll';UI=P+'/f09/SAN14_Traits9_UI.dll'
fails=0
def check(c,m):
    global fails
    print(('  ok   ' if c else '  FAIL ')+m);fails+=0 if c else 1
h=lambda p:hashlib.sha256(open(p,'rb').read()).hexdigest()
check(h(TH)==h(TH+'.v2'),'TraitHover .dll == .v2 (%s)'%h(TH)[:8])
sim='/tmp/th_sim_%d'%os.getpid()
subprocess.check_call(['clang','-O0','-w','-o',sim,R+'/test/th_sim/th_sim.c'])
def run(sc):
    o=subprocess.run([sim,TH,UI,sc,'v2','uni'],capture_output=True,text=True).stdout
    m=re.search(r'MASK 0x([0-9a-f]+)',o);r=re.search(r'reject=(\d+)/152',o)
    return (int(m.group(1),16) if m else None,int(r.group(1)) if r else None,o)
m,r,o=run('native');check(m==0x131 and r==0,'화면 부품 없음: 거부 0, 시작 마스크 0x131(.dll 과 같이 종류 0·4·5·8 도 미리 검) — mask=%s rej=%s'%(hex(m) if m is not None else m,r))
m,r,o=run('ui');check(m==0 and r==0,'화면 부품이 먼저(9·13): 거부 0(예전 .dll 146칸·.v2 2칸 거부), 마스크 0 — mask=%s rej=%s'%(hex(m) if m is not None else m,r))
m,r,o=run('ui_late');check(m==0x131 and r==0 and 'HOVER_SKIPPED' in o,'화면 부품이 나중: 화면 부품이 물러나고 TH 거부 0 — mask=%s rej=%s'%(hex(m) if m is not None else m,r))
m,r,o=run('foreign');check(r==1,'남의 DLL 이 칸을 차지: 여전히 거부(충돌 알림 유지) — rej=%s'%r)
os.unlink(sim)
# ③
d=open(TH,'rb').read()
dis=lambda a,b:subprocess.run(['objdump','-d','--start-address=%#x'%a,'--stop-address=%#x'%b,TH],capture_output=True,text=True).stdout
j=dis(0x1800021b4,0x1800021b9);mm=re.search(r'jmp\s+0x(1800aa[0-9a-f]+)',j);check(mm is not None,'0x21b4 설명 고르기 → .th8 갈림')
if mm:
    y=int(mm.group(1),16);ty=dis(y,y+0x3c);tx=dis(0x1800aa000,0x1800aa01f)
    cond=lambda t:[re.sub(r'.*\t','',l).split('#')[-1].strip() if '#' in l else re.sub(r'.*\t','',l).strip() for l in t.splitlines() if re.match(r'\s+1800aa',l)][:3]
    cx=[re.sub(r'%r[sc][ix]','R',c) for c in cond(tx)];cy=[re.sub(r'%r[sc][ix]','R',c) for c in cond(ty)]
    check(cx==cy and '0x180009000' in cx[0] and '0x8ca1c0' in cx[1] and '0x1800a7250' in cx[2],'갈림 조건 = 시작 마스크 조건(게임 기준·0x8ca1c0·종류0 기대값) %s'%cx)
    check('mov    0x2780f60(%rax),%rcx' in ty and 'jmp    0x1800021bb' in ty and 'jbe    0x18000220b' in ty and 'jmp    0x180002281' in ty,'화면 부품 없음 → .dll 원래 명령(게임 개성 설명 표), 있음 → .v2 갈림')
c=dis(0x180004b3c,0x180004b41);check('$0x2320aa' in c,'9칸 자리 기록 숫자 0x2320aa(옛 0x23064a 아님)')
# ④
ic=open(P+'/base/SAN14_IntegratedConvenience.dll','rb').read()
pe=struct.unpack_from('<I',ic,0x3c)[0];nsec=struct.unpack_from('<H',ic,pe+6)[0];osz=struct.unpack_from('<H',ic,pe+20)[0]
secs=[struct.unpack_from('<8sIIII',ic,pe+24+osz+40*k) for k in range(nsec)]
def off(r):
    for n,vs,va,rs,rp in secs:
        if va<=r<va+max(vs,rs):return r-va+rp
t=off(0xb7310);ents=[struct.unpack_from('<QII',ic,t+16*i) for i in range(0x92a)]
src=open(R+'/src/traits9_check.cpp',encoding='utf-8').read()
sites=[(int(a,16),int(n)) for a,n in re.findall(r'\{0x([0-9a-f]+),(\d+),\{',src)]
check(len(sites)==5,'9칸 5자리 읽음 %s'%[hex(a) for a,_ in sites])
hit=[hex(a) for a,n in sites+[(0x494431,14)] for o,l,e in ents if a<o+l and o<a+n]
check(not hit,'9칸 5자리·화면 가드 0x494431 이 통합편의 원본 확인 표와 안 겹침'+(' — 겹침 '+','.join(hit) if hit else ''))
# ⑤
code=re.sub(r'//[^\n]*','',src)
body=code[code.index('void early_nine()'):code.index('extern "C" __declspec(dllexport) void*SAN14_Traits9CheckTestTable')]
check('"SAN14_ApplyTraits9"' in body and 'SAN14_Traits9.dll' in body,'9번 모듈이 본체 SAN14_Traits9.dll 의 SAN14_ApplyTraits9 를 부름(본체 → 화면 부품 순서)')
check('ModPackBridge' not in body and 'TraitHover' not in body,'14번 유무·설정을 안 봄')
check('early_nine();' in code and 'early_ui' not in code,'ModInit 에서 early_nine, 옛 early_ui 없음')
print('전부 통과' if not fails else '실패 %d'%fails);sys.exit(1 if fails else 0)
