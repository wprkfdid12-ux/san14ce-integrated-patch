# 14번 TraitHover(모음팩 v1.7) 를 9·13번 개성 9개 UI(UI_Traits9_Hover, 'Traits9 UI v2.6') 와 함께 쓰는 판.
# R42 A10: 9·13 공용 개성 9칸 화면(SAN14_Traits9_UI.dll)을 Traits9 주인으로 인식하도록 이름을 인자로 받음.
#  - 두 모듈은 같은 화면 슬롯(UI 클래스 35종 × 4 + 특수 6)을 종류(kind 0~12)별 처리기 하나로 바꾼다.
#  - 이 판은 Traits9 가 먼저 차지한 슬롯 위에 얹힌다: 종류별 '원래 함수' 를 고정 게임 주소 대신 P[kind]
#    (시작 때 슬롯에서 읽은 Traits9 처리기, 없으면 게임 원래 함수) 로 부른다 → 각 틱에 Traits9 → 게임 순서로 이어짐.
#  - 개성 설명은 Traits9 가 맡으므로 이 판의 말풍선은 전법(키 0x200~0x2c8) 만 띄우고 나머지 키는 자기 말풍선을 닫는다.
#  - 시작 트리거는 kind 3(인물 목록 틱) 만 쓴다(kind 0/4/5/8 은 Traits9 처리기가 종류마다 달라 공용 시작 처리기로 못 이음).
import struct,sys
SRC,DST=sys.argv[1],sys.argv[2]
T9NAME=sys.argv[3] if len(sys.argv)>3 else 'UI_Traits9_Hover.dll'  # R42 A10: 공용 개성 9칸 화면 이름
d=bytearray(open(SRC,'rb').read())
nt=struct.unpack_from('<I',d,0x3c)[0]; opt=nt+24; so=struct.unpack_from('<H',d,nt+20)[0]; sh=opt+so; ns=struct.unpack_from('<H',d,nt+6)[0]
S={}
for i in range(ns):
    o=sh+i*40; S[d[o:o+8].rstrip(b'\0').decode()]=(o,)+struct.unpack_from('<IIII',d,o+8)
def off(rva):
    for n,(o,vs,va,rs,rp) in S.items():
        if va<=rva<va+rs: return rva-va+rp
    raise ValueError(hex(rva))
def get(rva,n): return bytes(d[off(rva):off(rva)+n])
def expect(rva,h): b=bytes.fromhex(h); assert get(rva,len(b))==b,(hex(rva),get(rva,len(b)).hex(),h)
def put(rva,b): d[off(rva):off(rva)+len(b)]=b
BASE=0x9000; TAB=0x62a0; NENT=152; IAT_GMH=0x8568
assert struct.unpack_from('<Q',d,off(0x8568))[0] and get(0x2e22,6)==bytes.fromhex('ff1540570000')  # GetModuleHandleW(NULL) 사용처 확인
# 종류별 원래 함수 호출 13곳 + 시작 처리기 2곳: mov eax,F ; add rax,[base]
SITES={0x1b69:(0,0x92ad0),0x1d2f:(1,0x7c9080),0x2d02:(2,0x8fbd40),0x1c29:(3,0x2dc340),0x1ef1:(4,0x92ad0),0x263d:(5,0x92ad0),
       0x2762:(6,0x924b20),0x270f:(7,0x9143c0),0x27a3:(8,0x92ad0),0x2a1f:(9,0x869820),0x2a72:(10,0x8782e0),0x2d52:(11,0x8782e0),
       0x2d9f:(12,0x869820),0x3685:(3,0x2dc340),0x36d5:(0,0x92ad0)}
for a,(k,f) in SITES.items():
    b=get(a,12); assert b[0]==0xb8 and struct.unpack_from('<I',b,1)[0]==f and b[5:8]==b'\x48\x03\x05' and a+12+struct.unpack_from('<i',b,8)[0]==BASE,(hex(a),b.hex())
# 표의 종류별 원래 함수가 위와 같은지 (표가 바뀌지 않았는지)
ents=[struct.unpack_from('<III',d,off(TAB)+12*i) for i in range(NENT)]
nat={}
for s,e,k in ents:
    if k<=12: nat.setdefault(k,set()).add(e)
for a,(k,f) in SITES.items(): assert nat[k]=={f},(k,nat[k])
expect(0x2f26,'448b6c3bfc4d01e5'); expect(0x4c96,'458b4c8804'+'4c030d5e430000'+'b801000000')
expect(0x21b4,'488b8818df7702'); expect(0x2ed4,'be31010000'); expect(0x2eaa,'4889054f610000')
expect(0x220b,'488d8a00feffff'); expect(0x2281,'488b4e08')
to,tvs,tva,trs,trp=S['.text']; do,dvs,dva,drs,drp=S['.data']
CAVE=tva+tvs+(16-(tva+tvs)%16)%16
P=(dva+dvs+15)&~15; assert P+13*8<=((dva+dvs+0xfff)&~0xfff)
code=bytearray(); lab={}; fix=[]
def here(): return CAVE+len(code)
def emit(b): code.extend(b)
def rip(prefix,target,suffix=b''):
    end=here()+len(prefix)+4+len(suffix); emit(prefix+struct.pack('<i',target-end)+suffix)
def j8(op,name): emit(bytes([op,0])); fix.append((len(code)-1,name))
def mark(n): lab[n]=here()
def align(): 
    while len(code)%16: emit(b'\xcc')
# --- 시작 준비: [BASE]=rax(원래 명령) + P[] 채우기
INIT=here()
rip(b'\x48\x89\x05',BASE)                       # mov [BASE],rax
emit(b'\x53\x56\x57\x41\x54\x48\x83\xec\x28')   # push rbx,rsi,rdi,r12; sub rsp,0x28
emit(b'\x49\x89\xc4')                           # mov r12,rax (게임 기준)
rip(b'\x48\x8d\x0d',0); namefix=len(code)-4     # lea rcx,[T9 이름]
rip(b'\xff\x15',IAT_GMH)                        # call GetModuleHandleW
emit(b'\x48\x89\xc6\x31\xff')                   # mov rsi,rax ; xor edi,edi
emit(b'\x48\x85\xf6'); j8(0x74,'nomod')         # test rsi,rsi ; jz
emit(b'\x8b\x46\x3c\x8b\x7c\x06\x50')           # mov eax,[rsi+0x3c] ; mov edi,[rsi+rax+0x50]
mark('nomod')
rip(b'\x48\x8d\x1d',TAB)                        # lea rbx,[TAB]
mark('loop')
emit(b'\x8b\x53\x08\x83\xfa\x0c'); j8(0x77,'next')            # mov edx,[rbx+8]; cmp edx,12; ja next
emit(b'\x44\x8b\x0b\x4f\x8b\x14\x0c')                         # mov r9d,[rbx]; mov r10,[r12+r9]
emit(b'\x44\x8b\x5b\x04\x4d\x01\xe3\x4d\x39\xda'); j8(0x74,'next')  # mov r11d,[rbx+4]; add r11,r12; cmp r10,r11; je next
emit(b'\x4c\x89\xd0\x48\x29\xf0\x48\x39\xf8')                 # mov rax,r10; sub rax,rsi; cmp rax,rdi
rip(b'\x48\x8d\x0d',P)                                         # lea rcx,[P]
j8(0x73,'bad')                                                 # jae bad (Traits9 밖)
emit(b'\x4c\x8b\x1c\xd1\x4d\x85\xdb'); j8(0x74,'set')          # mov r11,[rcx+rdx*8]; test r11,r11; jz set
emit(b'\x4d\x39\xd3'); j8(0x74,'next')                          # cmp r11,r10; je next
mark('bad'); emit(b'\x48\xc7\x04\xd1\xff\xff\xff\xff'); j8(0xeb,'next')   # mov qword[rcx+rdx*8],-1
mark('set'); emit(b'\x4c\x89\x14\xd1')                          # mov [rcx+rdx*8],r10
mark('next'); emit(b'\x48\x83\xc3\x0c')                         # add rbx,12
rip(b'\x48\x8d\x05',TAB+12*NENT)                                # lea rax,[TAB_END]
emit(b'\x48\x39\xc3'); j8(0x72,'loop')                          # cmp rbx,rax; jb loop
# 2차: P[k] 가 0(아무도 안 씀) 또는 -1(충돌) 이면 게임 원래 함수
rip(b'\x48\x8d\x1d',TAB)
mark('loop2')
emit(b'\x8b\x53\x08\x83\xfa\x0c'); j8(0x77,'next2')
rip(b'\x48\x8d\x0d',P)
emit(b'\x4c\x8b\x1c\xd1\x4d\x85\xdb'); j8(0x74,'nat')           # 0 → 원래
emit(b'\x49\x83\xfb\xff'); j8(0x75,'next2')                    # cmp r11,-1 ; jne next2
mark('nat'); emit(b'\x44\x8b\x5b\x04\x4d\x01\xe3\x4c\x89\x1c\xd1')  # mov r11d,[rbx+4]; add r11,r12; mov [rcx+rdx*8],r11
mark('next2'); emit(b'\x48\x83\xc3\x0c')
rip(b'\x48\x8d\x05',TAB+12*NENT)
emit(b'\x48\x39\xc3'); j8(0x72,'loop2')
emit(b'\x4c\x89\xe0')                                           # mov rax,r12 (원래 rax 복원)
emit(b'\x48\x83\xc4\x28\x41\x5c\x5f\x5e\x5b\xc3')               # add rsp; pop r12,rdi,rsi,rbx; ret
align()
# --- 슬롯 주인 확인(0x4c80): r9 = P[종류] (원래: 게임 원래 함수), eax=1
PK=here()
emit(b'\x45\x8b\x4c\x88\x08\x41\x83\xf9\x0c'); j8(0x77,'pknat')   # mov r9d,[r8+rcx*4+8]; cmp r9d,12; ja
rip(b'\x48\x8d\x05',P); emit(b'\x4e\x8b\x0c\xc8\xb8\x01\x00\x00\x00\xc3')   # lea rax,[P]; mov r9,[rax+r9*8]; mov eax,1; ret
mark('pknat'); emit(b'\x45\x8b\x4c\x88\x04'); rip(b'\x4c\x03\x0d',BASE); emit(b'\xb8\x01\x00\x00\x00\xc3')
align()
# --- 시작 슬롯 고르기(0x2f26): r13 = P[eax]
P3=here()
emit(b'\x44\x8b\x6c\x3b\xfc'); rip(b'\x4c\x8d\x2d',P); emit(b'\x4d\x8b\x6c\xc5\x00\xc3')   # (원래 r13 로드 유지) lea r13,[P]; mov r13,[r13+rax*8]; ret
align()
# --- 말풍선 거르기(0x21b4): 전법 키만 원래 전법 경로(0x220b), 나머지는 자기 말풍선 닫기(0x2281)
FLT=here()
emit(b'\x48\x8d\x8a\x00\xfe\xff\xff\x48\x81\xf9\xc8\x00\x00\x00'); j8(0x77,'hide')   # lea rcx,[rdx-0x200]; cmp rcx,0xc8; ja hide
emit(b'\xe9'); emit(struct.pack('<i',0x220b-(here()+4)))
mark('hide'); emit(b'\xe9'); emit(struct.pack('<i',0x2281-(here()+4)))
align()
NAME=here(); emit(T9NAME.encode('utf-16le')+b'\0\0')
for pos,n in fix:
    rel=lab[n]-(CAVE+pos+1); assert -128<=rel<128,(n,rel); code[pos]=rel&0xff
struct.pack_into('<i',code,namefix,NAME-(CAVE+namefix+4))
end=CAVE+len(code); assert end<=tva+trs,('no room',hex(end),hex(tva+trs))
for i in range(tva+tvs,end): assert d[off(i)] in (0,0xcc),hex(i)
d[off(CAVE):off(CAVE)+len(code)]=code
# --- 원래 코드 수정
for a,(k,f) in SITES.items():
    put(a,b'\x48\x8b\x05'+struct.pack('<i',P+8*k-(a+7))+b'\x90'*5)
put(0x2eaa,b'\xe8'+struct.pack('<i',INIT-(0x2eaa+5))+b'\x90\x90')
put(0x4c96,b'\xe8'+struct.pack('<i',PK-(0x4c96+5))+b'\x90'*12)
put(0x2f26,b'\xe8'+struct.pack('<i',P3-(0x2f26+5))+b'\x90'*3)
put(0x21b4,b'\xe9'+struct.pack('<i',FLT-(0x21b4+5))+b'\x90\x90')
put(0x2ed4,b'\xbe\x00\x00\x00\x00')
struct.pack_into('<I',d,to+8,end-tva)
struct.pack_into('<I',d,do+8,P+13*8-dva)
struct.pack_into('<I',d,opt+64,0)
open(DST,'wb').write(d)
print('INIT %#x PK %#x P3 %#x FLT %#x P %#x end %#x (room %#x)'%(INIT,PK,P3,FLT,P,end,tva+trs-end))
