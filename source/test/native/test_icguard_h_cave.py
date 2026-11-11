# R42e ICGuard 묶음 H 동굴 바이트 시험(네이티브, 에뮬 없이): 소스의 h_cave 를 떼어 컴파일해 전달판(a810d27d) 동굴과 바이트 비교
#  전달판 동굴 = install_h 의 조각(t_, t__0..t__8, f3+4) + 주소 3개 를 이어 붙인 0x42 바이트
import re,subprocess,sys,os,tempfile
src=open(os.path.join(os.path.dirname(__file__),'../../src/ic_guard.cpp'),encoding='utf-8').read()
fn=re.search(r'static U32 h_cave\(.*?\n\}\n',src,re.S).group(0)
OLD,BLK,BACK=0x1111111111111111,0x2222222222222222,0x3333333333333333
q=lambda v:v.to_bytes(8,'little')
want=(bytes.fromhex('3d00400000'+'7614'+'48b9')+q(OLD)+bytes.fromhex('f0ff01'+'3d00000010'+'7705'+'4883c448c3'+'48b9')+q(BLK)
      +bytes.fromhex('f0ff01'+'8bc8b800400000'+'ff2500000000')+q(BACK))
assert len(want)==0x42
c='#include <stdint.h>\n#include <string.h>\n#include <stdio.h>\ntypedef uint8_t U8;typedef uint32_t U32;typedef uint64_t U64;\n'+fn+ \
  'int main(){U8 b[0x80];U32 n=h_cave(b,0x1111111111111111ull,0x2222222222222222ull,0x3333333333333333ull);for(U32 i=0;i<n;i++)printf("%02x",b[i]);return 0;}\n'
d=tempfile.mkdtemp();open(d+'/t.c','w').write(c);subprocess.check_call(['clang','-O1','-w','-o',d+'/t',d+'/t.c'])
got=bytes.fromhex(subprocess.check_output([d+'/t']).decode())
assert got==want,(got.hex(),want.hex())
# 원래 13바이트·뒤 7바이트·이어 붙일 자리 상수가 전달판과 같은지
for k in['0x8b,0xc8,0xb8,0x00,0x40,0x00,0x00,0x3b,0xc8,0x77,0x07,0x8b,0xc1','0x48,0x83,0xc4,0x48,0xc3,0x8b,0x15','ic+0x1806','ic+0x1813','ic+0x1818']:
    assert k in src,k
print('ICGuard H 동굴 바이트 = 전달판과 같음 (0x42 바이트)')
