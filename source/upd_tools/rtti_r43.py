# RTTI 이름 ↔ vtable 표 (옛/새 exe). python3 rtti_r43.py 로 불러 씀: names(path) -> {vt_rva: name}, by_name
import struct
def load(p):
    d=open(p,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    secs=[(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
    return d,secs
def vtables(p):
    d,secs=load(p)
    def fo(r):
        for n,vs,va,rs,ra in secs:
            if va<=r<va+rs:return ra+r-va
    rd=[s for s in secs if s[0]==b'.rdata'][0];_,vs,va,rs,ra=rd
    out={}
    for off in range(0,rs-8,8):
        q=struct.unpack_from('<Q',d,ra+off)[0]-0x140000000
        if not(va<=q<va+rs):continue
        f=fo(q)
        if f is None or struct.unpack_from('<I',d,f)[0]!=1:continue
        try:
            td=struct.unpack_from('<I',d,f+12)[0];self_=struct.unpack_from('<I',d,f+20)[0]
            if self_!=q:continue
            o=fo(td+16);nm=d[o:d.index(b'\0',o)].decode('latin1')
        except Exception:continue
        out[va+off+8]=(nm,struct.unpack_from('<I',d,f+4)[0])
    return out
