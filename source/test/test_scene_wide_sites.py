# 22차(P219) 턴 끝 결과 넓은 창: SceneSkip 이 런타임에 확인하는 게임 자리(kWideChk)·가상 함수 표가 게임 실행 파일과 맞는지 대조.
#  게임 파일(복호화본)은 소스에 넣지 않음 → S14_NEW_EXE 또는 아는 자리에서 찾고, 없으면 건너뜀(정직하게 '건너뜀' 출력).
import os,re,struct,sys
R=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
cands=[os.environ.get('S14_NEW_EXE',''),'/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver/new.dec.exe','/home/claude/port/new.dec.exe']
exe=next((c for c in cands if c and os.path.exists(c)),None)
src=open(os.path.join(R,'src/scene_skip.cpp'),encoding='utf-8').read()
rows=re.findall(r"\{0x([0-9a-f]+),(\d+),\{([^}]*)\},\"",src)
assert len(rows)>=7,'kWideChk 줄을 못 읽음'
if not exe:print('건너뜀: 게임 실행 파일 복호화본 없음(S14_NEW_EXE)');sys.exit(0)
d=open(exe,'rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0];assert struct.unpack_from('<I',d,pe+8)[0]==0x6ad06f53,'게임 판(stamp)이 기준과 다름'
ns=struct.unpack_from('<H',d,pe+6)[0];osz=struct.unpack_from('<H',d,pe+20)[0];secs=[]
for i in range(ns):
    o=pe+24+osz+40*i;vs,va,rs,ra=struct.unpack_from('<IIII',d,o+8);secs.append((va,max(vs,rs),ra))
def rd(rva,n):
    for va,vs,ra in secs:
        if va<=rva<va+vs:return d[ra+rva-va:ra+rva-va+n]
    raise KeyError(hex(rva))
def val(x):
    x=x.strip()
    if x.startswith("'"):return ord(x[1])
    return int(x,0)
for a,n,b in rows:
    a=int(a,16);n=int(n);want=bytes(val(x) for x in b.split(','))[:n]
    assert rd(a,n)==want,'자리 0x%x: 기대 %s / 실제 %s'%(a,want.hex(' '),rd(a,n).hex(' '))
print('자리 %d곳 바이트 일치'%len(rows))
q=lambda a:struct.unpack('<Q',rd(a,8))[0]-0x140000000
for at,want in [(0x14679c8+8,0x83c680),(0x13a6160,0x203920),(0x13a6160+16,0x830ec0)]:assert q(at)==want,hex(at)
print('가상 함수 표 3곳 일치(CEventResultState 열기, 닫힐 때 부를 것 복사·부르기)')
c=rd(0x83c793,5);assert c[0]==0xe8 and 0x83c793+5+struct.unpack('<i',c[1:])[0]==0x842af0,'0x83c793 = call 0x842af0'
assert rd(0x83c78c,5)[0]==0xe8 and 0x83c78c+5+struct.unpack('<i',rd(0x83c78c,5)[1:])[0]==0x842ba0,'다른 갈래(flag!=0)는 0x842ba0'
assert rd(0x14684e8,30).startswith(b'09_01_02_02_event_talk_result'),'레이아웃 이름'
# 0x832f10 이 넘겨받은 r8 의 8바이트를 상태+0x470 에 그대로 → {사건번호, flag}
assert rd(0x832fb3,10)==bytes.fromhex('498b07 488983 70040000'.replace(' ','')),'state+0x470 = *(u64*)r8'
# 닫힐 때 부를 것은 [r9+0x38] (MSVC std::function 구현 주소 자리)
assert rd(0x832ff6,4)==bytes.fromhex('488b4d38'),'functor impl = [r9+0x38]'
# 나레이션 창도 같은 꼴: vtable 0x13a6160 + 잡은 값 = 0x572480(관리자), 쌓은 뒤 0x574750(그 상태)
n=rd(0x1f1fa5,7);assert n[:3]==bytes.fromhex('488d05') and 0x1f1fa5+7+struct.unpack('<i',n[3:])[0]==0x13a6160
# 26차: 저절로 넘김은 우리 창 레이아웃의 표 복사본 vt[1](프레임마다)에서 — 원래 표 모양 확인
assert q(0x1467820+8)==0x8ca1c0 and q(0x1467820+0x198)==0x8637f0 and q(0x1470fd8+0x198)==0x8637f0,'사건 결과 레이아웃 vt[1]=빈 함수, vt[0x198]=닫기(나레이션 창과 같은 닫기)'
assert q(0x1470fd8+8)==0x865520 and rd(0x865539,7)==bytes.fromhex('4881c198010000'),'나레이션 레이아웃 vt[1]=0x865520 이 +0x198 기다림을 재서 넘김(프레임마다 부르는 칸)'
assert 0x1467820+53*8==0x14679c8,'레이아웃 표 52칸 + 다음 표 RTTI 칸'
assert rd(0x864186,22)[8:9]==b'\xe8' and 0x86418e+5+struct.unpack('<i',rd(0x86418f,4))[0]==0x3ec1a0 and 0x864193+3+5+struct.unpack('<i',rd(0x864197,4))[0]==0x3ec470,'나레이션 창도 0x3ec1a0()→0x3ec470 으로 메시지 속도 기다림을 읽음'
print('메시지 속도 근거 확인 OK(설정+0x20 → 표 {수동,60,35,15})')
print('넓은 창 근거 확인 OK')
