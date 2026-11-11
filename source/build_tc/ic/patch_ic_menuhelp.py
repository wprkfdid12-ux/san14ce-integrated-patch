# 통합편의 모듈(1번/4번 공용) 인재·외교 메뉴 말풍선 설명 — 안개(13.5번)와 같이 켜져도 붙도록 수정
# 원인: 통합편의는 메뉴 도움말 자리([게임+0x14769a0])가 원본 함수(0x898540)인지 확인하고, 아니면 "구조 검증 실패"로 포기한다.
#       안개 엔진(정찰 기능)이 같은 자리를 자기 함수로 바꿔 두므로, 안개가 켜진 뒤로는 항상 포기했다.
# 수정 1: 검증 표의 그 항목을 이미 있는 다른 항목(0x1476998 → 0x898b80)과 같게 바꿔 사실상 뺀다 (16바이트).
# 수정 2: "원본 도움말 함수" 로 상수 0x898540 대신 그 자리에 지금 들어 있는 함수를 저장한다 (7바이트:
#         lea rax,[rsi+0x898540] → mov rax,[rsi+0x14769a0]). 통합편의 버튼이 아니면 그 함수(안개 → 원본 순)로 넘어간다.
import struct,hashlib,sys,os
HERE=os.path.dirname(os.path.abspath(__file__))
src=sys.argv[1];dst=sys.argv[2]
d=bytearray(open(src,'rb').read())
assert hashlib.sha256(d).hexdigest()=='fa07cd5fdf5c26ab6decceb933f6256a73d2d0fcf05e95102f82812f3415da0d','알 수 없는 통합편의 모듈'
def off(rva):
    if 0x1000<=rva<0x9a000:return rva-0x1000+0x400
    if 0x9a000<=rva<0x105000:return rva-0x9a000+0x98c00
    if 0x105000<=rva<0x1ef000:return rva-0x105000+0x103200
    raise ValueError(hex(rva))
# 1) 검증 표 항목 2
t=off(0xcec70)+16*2
assert struct.unpack_from('<QQ',d,t)==(0x14769a0,0x898540)
struct.pack_into('<QQ',d,t,0x1476998,0x898b80)
# 2) 원본 도움말 함수 저장
c=off(0x26bbd)
assert d[c:c+7]==bytes.fromhex('488d8640858900')
d[c:c+7]=bytes.fromhex('488b86a0694701')
# PE 체크섬 다시 계산
pe=struct.unpack_from('<I',d,0x3c)[0];cs=pe+24+64
struct.pack_into('<I',d,cs,0)
s=0;n=len(d)
for i in range(0,n-1,2):
    if i==cs:continue
    s+=struct.unpack_from('<H',d,i)[0];s=(s&0xffff)+(s>>16)
if n&1:s+=d[-1];s=(s&0xffff)+(s>>16)
s=(s&0xffff)+(s>>16);struct.pack_into('<I',d,cs,(s+n)&0xffffffff)
open(dst,'wb').write(d)
print('새 통합편의 모듈',hashlib.sha256(d).hexdigest(),'바뀐 바이트',sum(1 for a,b in zip(open(src,'rb').read(),d) if a!=b))
