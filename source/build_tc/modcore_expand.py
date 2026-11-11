# SAN14_ModCore.dll 훅 등록 장부 확장: 128 → 1024
# 1) 초기화 안 된 새 섹션 .hooks (1024*24 바이트) 추가
# 2) 장부 주소를 가리키는 lea 2곳을 새 섹션으로 돌림
# 3) 상한 비교 cmp eax,0x80 → cmp eax,0x400
import struct, sys, hashlib
src, dst = sys.argv[1], sys.argv[2]
d = bytearray(open(src, 'rb').read())
assert hashlib.sha256(d).hexdigest() == 'f54c7d4848dacf0c2414b07c7573cdbac8bf3ca9b5aec84345d8f549b292fc67', '알 수 없는 ModCore'
pe = struct.unpack_from('<I', d, 0x3c)[0]
assert d[pe:pe+4] == b'PE\0\0'
nsec = struct.unpack_from('<H', d, pe + 6)[0]
optsz = struct.unpack_from('<H', d, pe + 20)[0]
opt = pe + 24
sect = opt + optsz
assert nsec == 9 and optsz == 240
size_hdr = struct.unpack_from('<I', d, opt + 60)[0]
assert sect + 40 * (nsec + 1) <= size_hdr, '섹션 표 공간 부족'
# 마지막 섹션(.reloc) 뒤 RVA
last = sect + 40 * (nsec - 1)
va, vs = struct.unpack_from('<I', d, last + 12), struct.unpack_from('<I', d, last + 8)
va, vs = va[0], vs[0]
new_rva = (va + vs + 0xfff) & ~0xfff
assert new_rva == 0xc000
ENTRIES = 1024
new_vs = ENTRIES * 24
hdr = struct.pack('<8sIIIIIIHHI', b'.hooks\0\0', new_vs, new_rva, 0, 0, 0, 0, 0, 0, 0xC0000080)
d[sect + 40 * nsec: sect + 40 * (nsec + 1)] = hdr
struct.pack_into('<H', d, pe + 6, nsec + 1)
size_img = struct.unpack_from('<I', d, opt + 56)[0]
assert size_img == 0xc000
struct.pack_into('<I', d, opt + 56, (new_rva + new_vs + 0xfff) & ~0xfff)
# .text RVA 0x1000 → 파일 0x400
def off(rva): return rva - 0x1000 + 0x400
# lea rax,[rip+0x3dc6] @0x1253 (7바이트: 48 8d 05 c6 3d 00 00) → 0x5020
# lea rcx,[rip+0x3c4b] @0x13ce (7바이트: 48 8d 0d 4b 3c 00 00) → 0x5020
for rva, opc in ((0x1253, b'\x48\x8d\x05'), (0x13ce, b'\x48\x8d\x0d')):
    o = off(rva)
    assert d[o:o+3] == opc, hex(rva)
    disp = struct.unpack_from('<i', d, o + 3)[0]
    assert rva + 7 + disp == 0x5020, hex(rva)
    struct.pack_into('<i', d, o + 3, new_rva - (rva + 7))
# cmp eax,0x80 @0x1184
o = off(0x1184)
assert d[o:o+5] == b'\x3d\x80\x00\x00\x00'
struct.pack_into('<I', d, o + 1, ENTRIES)
# 체크섬 0으로 (DLL은 검사 안 함)
struct.pack_into('<I', d, opt + 64, 0)
open(dst, 'wb').write(d)
print('새 ModCore', hashlib.sha256(d).hexdigest(), '크기', len(d))
