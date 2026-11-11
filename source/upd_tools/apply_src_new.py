# 새 판: 우리 소스(r41/src)의 게임 주소 상수를 자리_대응.tsv(find_new.py 결과)대로 바꾼다. (2-1 도구, 2026-11-05)
#  규칙: 판정이 '정확' 또는 '정렬'(정확+N·확인필요·못찾음·상수로 보임 은 제외)이고 옛 값 ≥ 0x10000 인 줄만, 그 소스 파일 안의 같은 16진 글자(단어 경계)를 전부 새 값으로.
#        0x29ff000 / 0x6a983c25 글자는 version.h 의 S14_GAME_IMAGE / S14_GAME_STAMP 로 바꾼다(한 곳 관리).
#  검증: 코드 자리는 새 exe 의 새 RVA 바이트와 옛 바이트(16)를 비교해 다르면 '손으로 볼 것'에 적는다(rel32 가 든 훅 기대 바이트 등).
#  사용: python3 apply_src_new.py <자리_대응.tsv> <새 해제본 exe> <r41/src> [--apply]   (--apply 없으면 보고만)
import sys,re,os,struct,collections
MAP,NEWEXE,SRC=sys.argv[1:4];APPLY='--apply' in sys.argv
rows=[l.split('\t') for l in open(MAP,encoding='utf-8').read().splitlines()[1:]]
d=open(NEWEXE,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
secs=[]
for i in range(ns):
    o=pe+24+opt+40*i;vs,va,rs,ra=struct.unpack_from('<IIII',d,o+8);secs.append((va,max(vs,rs),ra,rs))
def rd(rva,n=16):
    for va,sz,ra,rs in secs:
        if va<=rva<va+sz:
            o=ra+(rva-va)
            if rva-va<rs:return d[o:o+n]
            return b''
    return b''
def good(v):return (v.startswith('정확') and not v.startswith('정확+')) or v.startswith('정렬')
per=collections.defaultdict(dict);manual=[];check=[]
for r in rows:
    if r[0]!='소스' or len(r)<13:continue
    f,kind,old,ver,memo,newr=r[1],r[2],int(r[3],16),r[12],r[10],r[11]
    new=int(newr,16) if newr.startswith('0x') else None
    if old in(0x29ff000,0x6a983c25):continue
    if not good(ver) or new is None or old<0x10000:
        manual.append((f,memo,f'0x{old:x}',newr or '-',ver));continue
    if new!=old:per[f][old]=new
    if kind=='코드':
        ob=bytes.fromhex(r[9]) if r[9] else b'';nb=rd(new,len(ob) or 16)
        if ob and nb and ob!=nb:
            # rel32 자리만 다른지(e8/e9 뒤 4바이트, 0f 8x 뒤 4바이트, 그 밖은 같음) 본다
            def mask(b):
                b=bytearray(b);i=0
                while i<len(b):
                    if b[i] in(0xe8,0xe9) and i+5<=len(b):b[i+1:i+5]=b'\0\0\0\0';i+=5
                    elif b[i]==0x0f and i+6<=len(b) and 0x80<=b[i+1]<=0x8f:b[i+2:i+6]=b'\0\0\0\0';i+=6
                    else:i+=1
                return bytes(b)
            check.append((f,memo,f'0x{old:x}',f'0x{new:x}','rel32만 다름' if mask(ob)==mask(nb) else '바이트 다름',ob.hex(),nb.hex()))
rep=[]
HEXW=lambda v:re.compile(r'(?<![\w])0x0*%x\b'%v,re.I)
for f,m in sorted(per.items()):
    p=os.path.join(SRC,f)
    if not os.path.exists(p):rep.append(f'{f}\t파일 없음');continue
    t=open(p,encoding='utf-8').read();t2=t;cnt=collections.Counter()
    # 큰 값부터(접두 충돌 없게 \b 를 쓰지만 안전하게)
    for old,new in sorted(m.items(),key=lambda x:-x[0]):
        t2,k=HEXW(old).subn(f'0x{new:x}',t2);cnt[f'0x{old:x}→0x{new:x}']=k
    for k,v in cnt.items():rep.append(f'{f}\t{k}\t{v}곳')
    if APPLY and t2!=t:open(p,'w',encoding='utf-8').write(t2)
# 판 지문 글자 → 매크로
for f in sorted(os.listdir(SRC)):
    if not f.endswith(('.cpp','.h')) or f=='version.h':continue
    p=os.path.join(SRC,f);t=open(p,encoding='utf-8').read();t2=re.sub(r'(?<![\w])0x0*29ff000\b','S14_GAME_IMAGE',t,flags=re.I);t2=re.sub(r'(?<![\w])0x0*6a983c25\b','S14_GAME_STAMP',t2,flags=re.I)
    if t2!=t:
        rep.append(f'{f}\t0x29ff000/0x6a983c25 → S14_GAME_IMAGE/S14_GAME_STAMP\t{len(re.findall("S14_GAME_(IMAGE|STAMP)",t2))-len(re.findall("S14_GAME_(IMAGE|STAMP)",t))}곳')
        if APPLY:open(p,'w',encoding='utf-8').write(t2)
out=os.path.join(SRC,'..','upd_tools','소스_적용_보고.txt')
with open(out,'w',encoding='utf-8') as w:
    w.write(('적용함' if APPLY else '보고만')+f'\n[바꾼 글자 {len(rep)}줄]\n'+'\n'.join(rep)+f'\n\n[손으로 볼 것 {len(manual)}건 — 자동으로 안 바꿈: 파일\t줄\t옛\t새후보\t판정]\n'+'\n'.join('\t'.join(x) for x in manual)
            +f'\n\n[코드 자리 바이트 비교: 옛 자리와 새 자리 16바이트가 다른 곳 {len(check)}건 — 기대 바이트 배열 손봐야 함]\n'+'\n'.join('\t'.join(x) for x in check)+'\n')
print(f'바꾼 글자 {len(rep)}줄, 손으로 {len(manual)}건, 바이트 다른 코드 자리 {len(check)}건 → {os.path.normpath(out)}')
print('rel32만 다름',sum(1 for c in check if c[4]=="rel32만 다름"),'/ 바이트 다름',sum(1 for c in check if c[4]=="바이트 다름"))
