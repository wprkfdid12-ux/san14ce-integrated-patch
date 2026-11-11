# P239(디시 11-11): 모드 설정 창 vtable 칸(숙련 설정 Enter exe+0x141f250 등)을 다른 모드가 먼저 바꾸면, 오류 창에 그 칸을 가진 DLL 이름이 나와야 한다.
#  26차까지는 주인 판정(owner_of_ptr)을 해 두고 창(fail_box → r39::fail)에는 안 넘겨, 제보 창에 "R43F-1-vtable" 만 있고 상대 모드 이름이 없었음.
import re,os
R=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
s=open(os.path.join(R,'src/mod_settings.cpp'),encoding='utf-8').read()
fb=re.search(r'static void fail_box\(([^)]*)\)\{([^\n]*)',s);assert fb,'fail_box 없음'
assert 'const r43c::Owner*own' in fb.group(1) and 'r39::fail_ex(' in fb.group(2) and ',own)' in fb.group(2),'fail_box 가 주인(own)을 창(fail_ex)에 넘겨야 함'
v=s[s.index('vtable 칸이 다름'):]
v=v[:v.index('set_blocked(&own)')]
assert 'own=r43c::owner_of_ptr(' in v and 'fail_box("vtable",where,ex,ac,&own)' in v,'vtable 칸 실패 창에 주인 판정을 넘길 것'
h=open(os.path.join(R,'src/r43_conflict.h'),encoding='utf-8').read()
assert 'cat(L"다른 모드와 충돌 — ");cat(o.name);' in h,'주인이 다른 모드면 창 제목에 DLL 이름'
print('모드 설정 창 vtable 충돌 창에 상대 DLL 이름: OK')
