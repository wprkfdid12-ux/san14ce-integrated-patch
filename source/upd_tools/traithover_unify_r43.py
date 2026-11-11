# R43 8차(사용자 11-09 "9·13·14번이 서로 기생하지 말고 따로 작동하게"): 14번 TraitHover 를 한 파일로 합침.
#  예전: 설치기가 9·13 있으면 .v2(9칸 화면 부품 Traits9_UI 가 칸을 먼저 차지했다고 가정), 없으면 .dll(게임 원래 함수만 받음)을 깔았음.
#   → 실제로는 '깔린 것'과 '실제로 켜진 것'이 어긋날 때마다 깨짐(진짜 DLL 코드 흉내 test/th_sim 으로 확인):
#     · .dll + 화면 부품이 먼저 켜짐(9번 안 고른 PC 에 남은 9번 파일을 통합편의가 실음) → 146칸 거부(11-09 제보 ×38, 첫 칸 0x1463070)
#     · .v2 + 화면 부품 켜짐 → 원래 함수 그대로인 2칸(0x13affc8·0x13f2dd8, 화면 부품이 일부러 안 건드리는 칸) 거부
#     · .v2 + 화면 부품 안 켜짐(11-08 '13번 없이 개성 설명 안 나옴', 11-09 figekth) → 개성·전법 설명 자체를 안 함(.v2 는 설명을 화면 부품에 맡김)
#  이제: .v2 를 바탕으로, '화면 부품이 실제로 칸을 차지했는지'를 시작할 때(그리고 설명 띄울 때) 직접 보고 고른다. 설치기는 어느 경우든 같은 파일.
#   ① 칸 검사(0x5150): 게임 원래 함수도 늘 받음 + 화면 부품이 차지한 종류는 그 값도 받음(그 외 남의 DLL 은 그대로 거부)
#   ② 시작 때 미리 거는 칸(0x2ed4): 화면 부품 없음 → .dll 과 같이 종류 0·4·5·8 도 검(마스크 0x131), 있음 → .v2 대로(0)
#   ③ 설명 고르기(0x21b4): 화면 부품 없음 → .dll 과 같이 게임 개성 설명 표를 읽음, 있음 → .v2 대로(화면 부품에 맡김)
#   ④ 9칸 자리 기록 숫자 0x23064a(1.0.1 주소) → 0x2320aa
#  '화면 부품 있음' = 종류 0 기대값(0xa7250)이 게임 원래 함수(게임+0x8ca1c0)가 아님(시작 함수 0x5070 이 그 값을 채움).
#  새 코드는 새 실행 구역 .th8(RVA 0xaa000)에 둠(빈 자리 부족). 재배치·예외 표 필요 없음(rip 상대만, 잎 함수).
#  사용: python3 traithover_unify_r43.py <.v2 입력> <출력1> [출력2 ...]   (두 번 돌려도 같음: 입력이 이미 합친 판이면 그대로 씀)
import sys,struct
src=sys.argv[1];outs=sys.argv[2:]
d=bytearray(open(src,'rb').read())
pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0];sh=pe+24+opt
def secs():return [(bytes(d[sh+40*i:sh+40*i+8]).rstrip(b'\0'),)+struct.unpack_from('<IIII',d,sh+40*i+8) for i in range(struct.unpack_from('<H',d,pe+6)[0])]
def fo(rva):
    for nm,vs,va,rs,rp in secs():
        if va<=rva<va+max(vs,rs):return rp+rva-va
    raise SystemExit('자리 없음 %x'%rva)
def rel32(at,ln,target):return struct.pack('<i',target-(at+ln))
BASE_SLOT=0x9000;TBL=0xa7250;NATIVE0=0x8ca1c0;GAME_DATA=0x2780f60
if any(s[0]==b'.th8' for s in secs()):
    print('이미 합친 판 — 그대로 씀')
else:
    # 입력이 우리가 아는 .v2 인지(바꿀 자리의 지금 바이트) 확인
    exp={0x5150:bytes.fromhex('458b4c880841'),0x2ed4:bytes.fromhex('be00000000'),0x21b4:bytes.fromhex('e9'),0x4b3c:bytes.fromhex('b94a062300')}
    for r,b in exp.items():assert bytes(d[fo(r):fo(r)+len(b)])==b,('예상과 다른 .v2',hex(r),bytes(d[fo(r):fo(r)+8]).hex())
    assert struct.unpack_from('<i',d,fo(0x21b5))[0]+0x21b9==0x51a0
    # 새 구역 .th8
    assert ns<8 and sh+40*(ns+1)<=struct.unpack_from('<I',d,pe+24+60)[0],'구역 머리 자리 없음'
    last=secs()[-1];va=(last[2]+max(last[1],1)+0xfff)&~0xfff;rp=(len(d)+0x1ff)&~0x1ff
    d+=b'\0'*(rp-len(d))
    X=va;Y=va+0x40
    code=bytearray(b'\xcc'*0x200)
    def put(rva,b):code[rva-va:rva-va+len(b)]=b
    # stubX: esi = (종류0 기대값 == 게임 원래 함수) ? 0x131 : 0
    sx=bytearray();a=X
    sx+=b'\x48\x8b\x35'+rel32(a+len(sx),7,BASE_SLOT)            # mov rsi,[게임 기준]
    sx+=b'\x48\x81\xc6'+struct.pack('<I',NATIVE0)               # add rsi,0x8ca1c0
    sx+=b'\x48\x3b\x35'+rel32(a+len(sx),7,TBL)                  # cmp rsi,[종류0 기대값]
    sx+=b'\xbe\x31\x01\x00\x00'                                  # mov esi,0x131
    sx+=b'\x74\x02\x31\xf6\xc3'                                  # je +2 ; xor esi,esi ; ret
    put(X,sx)
    # stubY: 설명 고르기 갈림
    sy=bytearray();a=Y
    sy+=b'\x48\x8b\x0d'+rel32(a+len(sy),7,BASE_SLOT)            # mov rcx,[게임 기준]
    sy+=b'\x48\x81\xc1'+struct.pack('<I',NATIVE0)               # add rcx,0x8ca1c0
    sy+=b'\x48\x3b\x0d'+rel32(a+len(sy),7,TBL)                  # cmp rcx,[종류0 기대값]
    sy+=b'\x75\x0c'                                              # jne 화면부품있음
    sy+=b'\x48\x8b\x88'+struct.pack('<I',GAME_DATA)             # mov rcx,[rax+0x2780f60]  (.dll 원래 명령)
    sy+=b'\xe9'+rel32(a+len(sy),5,0x21bb)                        # jmp 0x21bb
    assert len(sy)==35
    sy+=b'\x48\x8d\x8a\x00\xfe\xff\xff'                          # lea rcx,[rdx-0x200]   (.v2 0x51a0 과 같음)
    sy+=b'\x48\x81\xf9\xc8\x00\x00\x00'                          # cmp rcx,0xc8
    sy+=b'\x0f\x86'+rel32(a+len(sy),6,0x220b)                    # jbe 0x220b
    sy+=b'\xe9'+rel32(a+len(sy),5,0x2281)                        # jmp 0x2281
    put(Y,sy)
    d+=code
    # 구역 머리
    hdr=b'.th8\0\0\0\0'+struct.pack('<IIIIIIHHI',0x200,va,0x200,rp,0,0,0,0,0x60000020)
    d[sh+40*ns:sh+40*ns+40]=hdr
    struct.pack_into('<H',d,pe+6,ns+1)
    struct.pack_into('<I',d,pe+24+56,va+0x1000)                  # SizeOfImage
    struct.pack_into('<I',d,pe+24+4,struct.unpack_from('<I',d,pe+24+4)[0]+0x200)   # SizeOfCode
    struct.pack_into('<I',d,pe+24+64,0)                          # CheckSum(안 씀)
    # ① 칸 검사 0x5150 (r8=기록표, rcx=3*칸 번호, rdx=지금 값 → r9=받을 값, eax=1; r8·rcx·rdx 보존)
    c=bytearray();a=0x5150
    c+=b'\x45\x8b\x4c\x88\x04'                                   # mov r9d,[r8+rcx*4+4]   원래 함수 rva
    c+=b'\x4c\x03\x0d'+rel32(a+len(c),7,BASE_SLOT)              # add r9,[게임 기준]
    c+=b'\x4c\x39\xca'                                           # cmp rdx,r9
    c+=b'\x74\x15'                                               # je 끝(원래 함수 → 받음) : 5+7+4+4+7 = 0x15 뒤
    c+=b'\x41\x8b\x44\x88\x08'                                   # mov eax,[r8+rcx*4+8]   종류
    c+=b'\x83\xf8\x0c\x77\x0b'                                   # cmp eax,12 ; ja 끝
    c+=b'\x4c\x8d\x0d'+rel32(a+len(c),7,TBL)                    # lea r9,[기대값 표]
    c+=b'\x4d\x8b\x0c\xc1'                                       # mov r9,[r9+rax*8]
    c+=b'\xb8\x01\x00\x00\x00\xc3'                               # 끝: mov eax,1 ; ret
    assert len(c)<=0x30
    f=fo(0x5150);d[f:f+0x30]=c+b'\xcc'*(0x30-len(c))
    # ② 0x2ed4 mov esi,0 → call stubX
    f=fo(0x2ed4);d[f:f+5]=b'\xe8'+rel32(0x2ed4,5,X)
    # ③ 0x21b4 jmp 0x51a0 → jmp stubY
    f=fo(0x21b4);d[f:f+5]=b'\xe9'+rel32(0x21b4,5,Y)
    # ④ 기록 숫자
    f=fo(0x4b3c);d[f:f+5]=b'\xb9'+struct.pack('<I',0x2320aa)
    print('합침: .th8 RVA 0x%x, stubX 0x%x, stubY 0x%x'%(va,X,Y))
for o in outs:
    import os
    if os.path.exists(o):os.chmod(o,0o644)
    open(o,'wb').write(d)
print('쓴 파일',len(outs),'개, 크기',len(d))
