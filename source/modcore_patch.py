# R40 A22: ModCore 가 게임 확인(리졸버)에서 멈출 때 안내창 DLL(SAN14Mods\Core\SAN14_Notice.dll)을 불러오게 함.
# 원래 동작(기록 한 줄 남기고 모듈 안 싣고 끝)은 그대로, 그 뒤에 안내창만 추가. 정상 실행 길은 한 바이트도 안 바뀜.
import struct,hashlib,sys
SRC='/home/claude/r39/pkg/support/payload/base/SAN14Mods/Core/SAN14_ModCore.dll'
OUT='/home/claude/r41/build_out/SAN14_ModCore.dll'
b=bytearray(open(SRC,'rb').read())
assert hashlib.sha256(b).hexdigest().startswith('7f839777153448a0'),'ModCore 원본이 아님'
IB=0x180000000
def fo(va):  # .text 0x1000→0x400, .rodata 0x4000→0x1800
    r=va-IB
    if 0x1000<=r<=0x2200:return r-0x1000+0x400
    if 0x4000<=r<0x4600:return r-0x4000+0x1800
    raise ValueError(hex(va))
def rel32(src_next,dst):return struct.pack('<i',dst-src_next)
LOG=0x1800010a0;LOADREL=0x1800018e0;DONE=0x180001b9c
MSG_UNSUP=0x180004290;MSG_NORES=0x180004260
WSTR=0x180004430
def expect(va,hexs):
    got=bytes(b[fo(va):fo(va)+len(bytes.fromhex(hexs))])
    assert got==bytes.fromhex(hexs),(hex(va),got.hex(),hexs)
# 원래 바이트 확인
expect(0x180001b90,'488d0df9260000e804f5ffff')          # lea rcx,unsupported ; call log
expect(0x180001fc9,'488d0d90220000e8cbf0ffffe9c2fbffff')  # lea rcx,missing ; call log ; jmp done
assert bytes(b[fo(0x180002140):fo(0x180002200)])==bytes(0xc0),'.text 빈자리 아님'
assert bytes(b[fo(WSTR):fo(WSTR)+0x60])==bytes(0x60),'.rodata 빈자리 아님'
def cave(at,msg):
    c=bytearray()
    def emit(x):c.extend(x)
    emit(b'\x48\x8d\x0d'+rel32(at+len(c)+7,msg))
    emit(b'\xe8'+rel32(at+len(c)+5,LOG))
    emit(b'\x48\x8d\x0d'+rel32(at+len(c)+7,WSTR))
    emit(b'\xe8'+rel32(at+len(c)+5,LOADREL))
    emit(b'\xe9'+rel32(at+len(c)+5,DONE))
    return bytes(c)
C1=0x180002140;C2=0x180002170
c1=cave(C1,MSG_UNSUP);c2=cave(C2,MSG_NORES)
b[fo(C1):fo(C1)+len(c1)]=c1;b[fo(C2):fo(C2)+len(c2)]=c2
b[fo(0x180001b90):fo(0x180001b90)+7]=b'\xe9'+rel32(0x180001b95,C1)+b'\x90\x90'
b[fo(0x180001fc9):fo(0x180001fc9)+7]=b'\xe9'+rel32(0x180001fce,C2)+b'\x90\x90'
w='SAN14Mods\\Core\\SAN14_Notice.dll'.encode('utf-16le')+b'\0\0'
b[fo(WSTR):fo(WSTR)+len(w)]=w
# 구역 머리: .text / .rodata VirtualSize 를 늘려 덧붙인 자리가 확실히 올라오게
pe=struct.unpack_from('<I',b,0x3c)[0];nsec=struct.unpack_from('<H',b,pe+6)[0];opt=struct.unpack_from('<H',b,pe+20)[0];sec=pe+24+opt
for i in range(nsec):
    o=sec+i*40;name=bytes(b[o:o+8]).rstrip(b'\0')
    if name==b'.text':assert struct.unpack_from('<I',b,o+8)[0]==0x1138;struct.pack_into('<I',b,o+8,0x11a0)
    if name==b'.rodata':assert struct.unpack_from('<I',b,o+8)[0]==0x425;struct.pack_into('<I',b,o+8,0x4a0)
open(OUT,'wb').write(b);print(OUT,hashlib.sha256(b).hexdigest())
