# R43 11-08(사용자: 옛 설정 창은 아예 안 뜨게 · 통합패치만으로는 새 설정 창 실패가 절대 없게) — 1번 새 설정 창 자기 점검
#  ① 꾸러미의 통합편의(IC) 해시가 우리 모듈이 아는 판(common.h known_common)에 있음 → IC 를 바꾸고 이쪽을 깜빡하면 실패
#  ② 새 설정 창이 고치는 게임 자리 13곳(jump 7 — 11차 P204 줄 수 다시 쓰기 0x658dfa 포함 + vtable 칸 6)을 우리 다른 모듈이 안 건드림
#  ③ 소스 규칙: 옛 창 설정(UseNewWindow) 안 읽음 · 실패 때 옛 창 대신 충돌 안내(blocked) + DLL 남김(1) · IC 늦은 시작이면 시작 확인 끝까지 기다림
#  실행: python3 test/test_settings_window_r43.py
import re,glob,hashlib,sys
R='/home/claude/r41'
fails=0
def check(c,m):
    global fails
    print(('  ok   ' if c else '  FAIL ')+m);fails+=0 if c else 1
# ①
ic=open(R+'/newpkg/payload_new/base/SAN14_IntegratedConvenience.dll','rb').read()
h=hashlib.sha256(ic).digest()
com=open(R+'/src/common.h',encoding='utf-8').read()
known=[]
for m in re.finditer(r'static const U8 (\w+)\[\]=\{([^}]*)\}',com):
    b=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',m.group(2)))
    if len(b)==32:known.append((m.group(1),b))
check(any(b==h for _,b in known),'꾸러미 IC(%s…) 가 known_common 목록(%s)에 있음'%(h.hex()[:8],','.join(n for n,_ in known)))
# ②
ms=open(R+'/src/mod_settings.cpp',encoding='utf-8').read()
sites=[(int(a,16),int(n)) for a,n in re.findall(r'plan\.jump\(\s*0x([0-9a-f]+)\s*,\s*\w+\s*,\s*(\d+)',ms)]
slots=[int(a,16) for a in re.findall(r'vt_hook\(plan\.hooks\[plan\.count\+\+\],\s*0x([0-9a-f]+)',ms)]
check(len(sites)==7 and len(slots)==6,'새 창 자리 읽음: jump %d곳 + vtable 칸 %d곳'%(len(sites),len(slots)))
ranges=sites+[(s,8) for s in slots]
hits=[]
for f in sorted(glob.glob(R+'/src/*.cpp')+glob.glob(R+'/src/*.h')):
    if f.endswith('mod_settings.cpp'):continue
    s=open(f,encoding='utf-8',errors='ignore').read()
    for m in re.finditer(r'\b\w+\.(jump|call)\(\s*0x([0-9a-f]+)\s*,\s*\w+\s*,\s*(\d+)',s):
        a=int(m.group(2),16);n=max(int(m.group(3)),5)
        for b,k in ranges:
            if a<b+k and b<a+n:hits.append('%s 0x%x'%(f.split('/')[-1],a))
    code=re.sub(r'//[^\n]*','',s)
    for b in slots:
        if re.search(r'0x0*%x\b'%b,code):hits.append('%s vtable 0x%x'%(f.split('/')[-1],b))
check(not hits,'우리 다른 모듈이 새 창 자리를 안 건드림'+(' — 겹침: '+', '.join(hits) if hits else ''))
# ③
code=re.sub(r'//[^\n]*','',ms)
check('UseNewWindow",1)' not in code and 'use_new=true;' in code,'옛 창 설정(UseNewWindow=0) 안 읽음 — 늘 새 창')
check('set_blocked(&own)' in code and re.search(r'set_blocked\(&own\);[^\n]*\n[^\n]*\n\s*return 1;\}',code) is not None,'훅 실패 때 충돌 안내 모드 + DLL 남김(return 1)')
check('if(blocked){r43c::show(' in code,"'모드' 버튼: 충돌 안내 창(옛 창으로 안 넘김)")
check('SAN14_WaitConvenienceV1' in code and 't0+600000' in code,'IC 시작 확인 중이면 버튼 교체를 최대 10분 기다림')
check('옛 창(통합편의 창)으로 열림' not in ms,"실패 창 글자에 '옛 창으로 열림' 없음")
print('전부 통과' if not fails else '실패 %d'%fails);sys.exit(1 if fails else 0)
