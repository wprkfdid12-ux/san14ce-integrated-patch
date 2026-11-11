# 27번 FogTrial(남의 DLL) cm_install 의 게임 코드 서명 검사 3곳을 새 판에 맞춘다. apply_new.py 가 못 고치는 부분:
#  (가) 서명 비교 루프의 음수 변위 [rcx+rdx-OLD-k] (k=0..3) 와 루프 끝 cmp rcx,OLD+0x20  (나) 내장 서명 32바이트 = 새 게임 바이트
#  사용: python3 fogtrial_fix.py <apply_new 뒤 FogTrial.dll> <새 해제 exe> <출력>   (자리 대응은 아래 표: 옛 자리, 새 자리, 서명 RVA)
import sys,struct,hashlib
SRC,NEWEXE,OUT=sys.argv[1:4]
SITES=[(0x8a2140,0x8a4380,0x3e970),(0x8741d0,0x876580,0x3e990),(0x874650,0x876ad0,0x3e9b0)]
b=bytearray(open(SRC,'rb').read())
pe=struct.unpack_from('<I',b,0x3c)[0];ns=struct.unpack_from('<H',b,pe+6)[0];opt=struct.unpack_from('<H',b,pe+20)[0]
secs=[struct.unpack_from('<IIII',b,pe+24+opt+40*i+8) for i in range(ns)]
def foff(rva):
    for vs,va,rs,ra in secs:
        if va<=rva<va+max(vs,rs):return ra+rva-va
d=open(NEWEXE,'rb').read();pe2=struct.unpack_from('<I',d,0x3c)[0];opt2=struct.unpack_from('<H',d,pe2+20)[0];vs,va,rs,ra=struct.unpack_from('<IIII',d,pe2+24+opt2+8)
game=lambda r,n:d[ra+r-va:ra+r-va+n]
# cm_install 구역(.text 0x33000~0x34100)
lo,hi=foff(0x33000),foff(0x34100);rep=[]
for old,new,sig in SITES:
    # (가) 변위
    for k in range(4):
        pat=struct.pack("<i",-(old-k));i=b.find(pat,lo,hi);cnt=0
        while i>=0:
            b[i:i+4]=struct.pack("<i",-(new-k));cnt+=1;rep.append(f"변위 -0x{old-k:x} → -0x{new-k:x} @file 0x{i:x}");i=b.find(pat,i+4,hi)
        if cnt==0:
            assert b.find(struct.pack("<i",-(new-k)),lo,hi)>=0,("변위 못 찾음",hex(old-k));rep.append(f'변위 -0x{new-k:x} 이미 고쳐짐')
    # 루프 끝: 48 81 f9 imm32 (옛 값 또는 apply_new 가 잘못 바꾼 값일 수 있음) → 루프 시작 mov ecx,new 뒤에서 처음 나오는 48 81 f9 를 잡는다
    mv=b'\xb9'+struct.pack('<I',new);i=b.find(mv,lo,hi);assert i>=0,('mov ecx,new 못 찾음',hex(new))
    j=b.find(b'\x48\x81\xf9',i,hi);assert j>=0 and j-i<0x100
    cur=struct.unpack_from('<I',b,j+3)[0];b[j+3:j+7]=struct.pack('<I',new+0x20);rep.append(f'루프 끝 cmp rcx,0x{cur:x} → 0x{new+0x20:x} @file 0x{j:x}')
    # (나) 서명
    so=foff(sig);old_sig=bytes(b[so:so+32]);new_sig=game(new,32)
    if old_sig!=new_sig:b[so:so+32]=new_sig;rep.append(f'서명 0x{sig:x}: {old_sig.hex()[:24]}… → {new_sig.hex()[:24]}…')
    else:rep.append(f'서명 0x{sig:x}: 새 게임 바이트와 같음(그대로)')
open(OUT,'wb').write(b)
print('\n'.join(rep));print('sha256',hashlib.sha256(b).hexdigest()[:16])
