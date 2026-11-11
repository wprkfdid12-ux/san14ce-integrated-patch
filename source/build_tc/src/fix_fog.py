"""Repair two renderer addresses, only in the exact supplied Fog 1.2.1 DLL."""
from pathlib import Path
import hashlib,sys,json
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'pydeps'))
import pefile
OLD='b00e46497ed16d89a2c21031152257e9a712c1048bc8589b1dcb81bbb3fc8100'
NEW='603735d2afc39e26046e6581c6c9690a9f967fd3fee41055e468fce26488b3b8'
def repair(raw):
    if hashlib.sha256(raw).hexdigest()!=OLD:raise ValueError('Unknown fog DLL: left unchanged')
    pe=pefile.PE(data=raw);out=bytearray(raw)
    for rva,old,new in ((0xfe79a,'4c8d8200b7a700','4c8d8240aea700'),(0xfe7ae,'4881c2a0c2a700','4881c2e0b9a700')):
        at=pe.get_offset_from_rva(rva);before=bytes.fromhex(old);after=bytes.fromhex(new)
        if out[at:at+len(before)]!=before:raise ValueError('Renderer signature mismatch')
        out[at:at+len(after)]=after
    if hashlib.sha256(out).hexdigest()!=NEW:raise ValueError('Unexpected repair output')
    return bytes(out)
if __name__=='__main__':
    src=ROOT/'base/current/support/extras/22/SAN14_FogEngine121.dll'
    dst=ROOT/'product/payload/SAN14_FogEngine121.dll'
    dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(repair(src.read_bytes()))
    print(json.dumps({'source_sha256':OLD,'result_sha256':NEW,'output':str(dst)}))
