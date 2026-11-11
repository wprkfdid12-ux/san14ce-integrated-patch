# 13번 FieldLeader3D: 수송부대(부대 자료 +0x1b 병종 번호 0x13/0x14)는 순정 그림 그대로 두기
# 내장 DLL 의 부대 수집 반복문에 검사 1개를 끼워 넣고(코드 끝 빈 공간 사용), 실행기 안의 내장본과 확인값을 함께 바꿈
import struct,hashlib,sys,subprocess
def sha(b):return hashlib.sha256(b).hexdigest()
def pe_checksum(d):
    s=0;n=len(d);pe=struct.unpack_from('<I',d,0x3c)[0];cs=pe+24+64
    for i in range(0,n-1,2):
        if i==cs:continue
        s+=struct.unpack_from('<H',d,i)[0];s=(s&0xffff)+(s>>16)
    if n&1:s+=d[-1];s=(s&0xffff)+(s>>16)
    s=(s&0xffff)+(s>>16);return (s+n)&0xffffffff
def patch_dll(d,site_rva,nxt,excluded,cont):
    d=bytearray(d)
    pe=struct.unpack_from('<I',d,0x3c)[0];optsz=struct.unpack_from('<H',d,pe+20)[0];sect=pe+24+optsz
    name=d[sect:sect+8];assert name.startswith(b'.text')
    vs,va,rs,ro=struct.unpack_from('<IIII',d,sect+8)
    site=site_rva-va+ro
    assert d[site:site+6]==bytes([0x80,0x7b,0x10,0x00,0x74,(nxt-(site_rva+6))&0xff]),'site bytes'
    cave=(va+vs+15)&~15;assert cave+40<=va+rs,'no slack'
    assert d[ro+cave-va:ro+rs]==b'\0'*(rs-(cave-va)),'slack not empty'
    def rel(frm,to):return struct.pack('<i',to-frm)
    c=bytearray()
    c+=bytes([0x80,0x7b,0x10,0x00])                      # cmp byte [rbx+0x10],0
    c+=bytes([0x0f,0x84])+rel(cave+len(c)+6,nxt)        # je next
    c+=bytes([0x0f,0xb6,0x43,0x1b])                      # movzx eax, byte [rbx+0x1b]  (병종 번호)
    c+=bytes([0x83,0xe8,0x13])                           # sub eax,0x13
    c+=bytes([0x83,0xf8,0x01])                           # cmp eax,1
    c+=bytes([0x0f,0x86])+rel(cave+len(c)+6,excluded)   # jbe excluded (수송 → 제외 집계 후 다음 부대)
    c+=bytes([0xe9])+rel(cave+len(c)+5,cont)             # jmp 원래 이어지는 자리
    d[ro+cave-va:ro+cave-va+len(c)]=c
    d[site:site+6]=bytes([0xe9])+rel(site_rva+5,cave)+bytes([0x90])
    struct.pack_into('<I',d,sect+8,cave+len(c)-va)       # VirtualSize 가 새 코드를 덮도록
    struct.pack_into('<I',d,pe+24+64,0)
    struct.pack_into('<I',d,pe+24+64,pe_checksum(d))
    return bytes(d),cave
def run(variant,site,nxt,excluded,cont):
    dll=open(f'/home/claude/p27/f13/{variant}.dll','rb').read();rt=open(f'/home/claude/p27/f13/rt_{variant}.exe','rb').read()
    new,cave=patch_dll(dll,site-0x180000000,nxt-0x180000000,excluded-0x180000000,cont-0x180000000)
    assert len(new)==len(dll)
    assert rt.count(dll)==1 and rt.count(hashlib.sha256(dll).digest())==1,'embedded dll/sha not unique'
    nrt=rt.replace(dll,new).replace(hashlib.sha256(dll).digest(),hashlib.sha256(new).digest())
    assert nrt.count(new)==1 and nrt.count(hashlib.sha256(new).digest())==1 and len(nrt)==len(rt)
    open(f'/home/claude/p27/f13/{variant}.R10.dll','wb').write(new);open(f'/home/claude/p27/f13/rt_{variant}.R10.exe','wb').write(nrt)
    print(variant,'cave',hex(cave),'dll',sha(new),'rt',sha(nrt))
run('normal',0x180009e20,0x180009ded,0x180009de6,0x180009e26)
run('fog',0x180009df0,0x180009dbd,0x180009db6,0x180009df6)
