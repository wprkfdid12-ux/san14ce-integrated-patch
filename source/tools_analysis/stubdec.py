# SteamStub v3.1 (x64) .text decryptor — verified against the known-good pair before use.
import struct,sys
from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes
def decrypt(src,dst):
    d=bytearray(open(src,'rb').read())
    pe=struct.unpack_from('<I',d,0x3c)[0]; n=struct.unpack_from('<H',d,pe+6)[0]; opt=struct.unpack_from('<H',d,pe+20)[0]
    oep=struct.unpack_from('<I',d,pe+24+16)[0]
    secs=[]
    for i in range(n):
        o=pe+24+opt+40*i; secs.append((d[o:o+8].rstrip(b'\0'),)+struct.unpack_from('<IIII',d,o+8))
    def rva2off(r):
        for name,vs,va,rs,ra in secs:
            if va<=r<va+max(vs,rs): return ra+r-va
    hoff=rva2off(oep-0xF0)
    h=bytearray(d[hoff:hoff+0xF0])
    key=struct.unpack_from('<I',h,0)[0]
    for i in range(4,0xF0,4):
        v=struct.unpack_from('<I',h,i)[0]; struct.pack_into('<I',h,i,v^key); key=v
    sig=struct.unpack_from('<I',h,4)[0]
    assert sig in (0xC0DEC0DE,0xC0DEC0DF),hex(sig)
    flags=struct.unpack_from('<I',h,0x38)[0]
    code_va=struct.unpack_from('<I',h,0x48)[0]; code_rs=struct.unpack_from('<I',h,0x50)[0]
    aes_key=bytes(h[0x58:0x78]); aes_iv=bytes(h[0x78:0x88]); stolen=bytes(h[0x88:0x98])
    print('flags',hex(flags),'code va',hex(code_va),'rs',hex(code_rs))
    if flags&0x20: print('code section not encrypted'); open(dst,'wb').write(d); return
    off=rva2off(code_va)
    # decrypt IV with ECB
    dec=Cipher(algorithms.AES(aes_key),modes.ECB()).decryptor(); iv=dec.update(aes_iv)+dec.finalize()
    data=stolen+bytes(d[off:off+code_rs])
    dec=Cipher(algorithms.AES(aes_key),modes.CBC(iv)).decryptor(); out=dec.update(data)+dec.finalize()
    d[off:off+code_rs]=out[:code_rs]
    open(dst,'wb').write(d)
if __name__=='__main__': decrypt(sys.argv[1],sys.argv[2])
