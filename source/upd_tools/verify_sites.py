# 새 판 훅 자리 검증: 우리 소스의 기대 바이트·호출 자리·vtable 칸을 (해제된) 게임 exe 와 대조한다. (2-1/2-3 도구)
#  사용: python3 verify_sites.py <해제 exe> <r41/src>
#  본다: X.jump(0xS,ARR,N,…) → exe[S:S+N]==ARR ; X.call(0xS,0xT,…) → exe[S]==e8 이고 rel32 가 T ; memcmp(game+0xS,ARR,N) ;
#        vt_hook(…,0xSLOT,0xORIG,…) / 자료 칸 배열(sites/originals) → *(u64*)SLOT == 0x140000000+ORIG
import sys,re,os,struct,collections
EXE,SRC=sys.argv[1:3]
d=open(EXE,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
IB=struct.unpack_from('<Q',d,pe+24+24)[0]
secs=[struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
def rd(rva,n):
    for vs,va,rs,ra in secs:
        if va<=rva<va+max(vs,rs):
            if rva-va+n<=rs:return d[ra+rva-va:ra+rva-va+n]
            return b''
    return b''
ok=bad=0;out=[]
def arr_bytes(t,name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\[\s*\d*\s*\]\s*=\s*\{([^}]*)\}',t)
    if not m:return None
    return bytes(int(x,0)&0xff for x in re.findall(r'0x[0-9a-fA-F]+|\d+',m.group(1)))
for f in sorted(os.listdir(SRC)):
    if not f.endswith('.cpp') or f.startswith(('scene_test','patch_tester')):continue
    t=open(os.path.join(SRC,f),encoding='utf-8',errors='replace').read()
    code='\n'.join(l.split('//')[0] for l in t.splitlines())
    # jump
    for m in re.finditer(r'\.jump\(\s*0x([0-9a-fA-F]+)\s*,\s*([A-Za-z_]\w*)\s*,\s*(\d+|sizeof\(\s*\w+\s*\))',code):
        s=int(m.group(1),16);name=m.group(2);a=arr_bytes(code,name)
        if a is None:out.append(f'{f}\tjump 0x{s:x}\t배열 {name} 못 찾음');bad+=1;continue
        n=len(a) if m.group(3).startswith('sizeof') else int(m.group(3));a=a[:n];g=rd(s,n)
        if g==a:ok+=1
        else:bad+=1;out.append(f'{f}\tjump 0x{s:x} {name}\t기대 {a.hex()}\texe {g.hex()}')
    # call
    for m in re.finditer(r'\.call\(\s*0x([0-9a-fA-F]+)\s*,\s*0x([0-9a-fA-F]+)',code):
        s,tg=int(m.group(1),16),int(m.group(2),16);g=rd(s,5)
        if len(g)==5 and g[0]==0xe8 and s+5+struct.unpack('<i',g[1:5])[0]==tg:ok+=1
        else:bad+=1;out.append(f'{f}\tcall 0x{s:x}→0x{tg:x}\texe {g.hex()}'+(f' (실제 대상 0x{s+5+struct.unpack("<i",g[1:5])[0]:x})' if len(g)==5 and g[0]==0xe8 else ''))
    # memcmp(game+0xS,ARR,N)
    for m in re.finditer(r'memcmp\(\s*game\s*\+\s*0x([0-9a-fA-F]+)\s*,\s*([A-Za-z_]\w*)\s*,\s*(\d+)\s*\)',code):
        s=int(m.group(1),16);a=arr_bytes(code,m.group(2));n=int(m.group(3))
        if a is None:continue
        g=rd(s,n)
        if g==a[:n]:ok+=1
        else:bad+=1;out.append(f'{f}\tmemcmp 0x{s:x} {m.group(2)}\t기대 {a[:n].hex()}\texe {g.hex()}')
    # vt_hook(h,0xSLOT,0xORIG,
    for m in re.finditer(r'vt_hook\([^,]+,\s*0x([0-9a-fA-F]+)\s*,\s*0x([0-9a-fA-F]+)',code):
        s,o=int(m.group(1),16),int(m.group(2),16);g=rd(s,8)
        if len(g)==8 and struct.unpack('<Q',g)[0]==IB+o:ok+=1
        else:bad+=1;out.append(f'{f}\tvtable 0x{s:x}\t기대 0x{o:x}\texe {hex(struct.unpack("<Q",g)[0]-IB) if len(g)==8 else "?"}')
    # 자료 칸 배열: xxxSites[]={..} / xxxOriginals[]={..}
    for m in re.finditer(r'(\w+)Sites\s*\[\s*\]\s*=\s*\{([^}]*)\}\s*,\s*(\w+)Originals\s*\[\s*\]\s*=\s*\{([^}]*)\}',code):
        ss=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]+)',m.group(2))];oo=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]+)',m.group(4))]
        for s,o in zip(ss,oo):
            g=rd(s,8)
            if len(g)==8 and struct.unpack('<Q',g)[0]==IB+o:ok+=1
            else:bad+=1;out.append(f'{f}\tvtable {m.group(1)} 0x{s:x}\t기대 0x{o:x}\texe {hex(struct.unpack("<Q",g)[0]-IB) if len(g)==8 else "?"}')
    # S14_Hook 수동 칸: target=game+0xS … old=(UPTR)(game+0xO)
    for m in re.finditer(r'target\s*=\s*game\s*\+\s*0x([0-9a-fA-F]+)\s*;[^;]*;\s*UPTR\s+old\s*=\s*\(UPTR\)\(\s*game\s*\+\s*0x([0-9a-fA-F]+)',code):
        s,o=int(m.group(1),16),int(m.group(2),16);g=rd(s,8)
        if len(g)==8 and struct.unpack('<Q',g)[0]==IB+o:ok+=1
        else:bad+=1;out.append(f'{f}\tvtable(S14_Hook) 0x{s:x}\t기대 0x{o:x}\texe {hex(struct.unpack("<Q",g)[0]-IB) if len(g)==8 else "?"}')
    # 상수 칸: SLOT=0x…,NATIVE=0x…
    for m in re.finditer(r'SLOT\s*=\s*0x([0-9a-fA-F]+)\s*,\s*NATIVE\s*=\s*0x([0-9a-fA-F]+)',code):
        s,o=int(m.group(1),16),int(m.group(2),16);g=rd(s,8)
        if len(g)==8 and struct.unpack('<Q',g)[0]==IB+o:ok+=1
        else:bad+=1;out.append(f'{f}\tvtable SLOT 0x{s:x}\t기대 0x{o:x}\texe {hex(struct.unpack("<Q",g)[0]-IB) if len(g)==8 else "?"}')
print(f'맞음 {ok} / 다름 {bad}')
for o in out:print(' ',o)
