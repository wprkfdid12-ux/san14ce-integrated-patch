# 새 판 한 번에 고치기 (게임 업데이트 날 쓰는 도구)
#  ① 소스 없는 DLL(바이너리만 있는 모듈)의 'exe 판 지문'(시간표·크기)을 새 값으로 바꿈
#  ② --addr 를 주면: 같은 DLL 안 게임 주소 상수 중 find_new.py 가 '정확/정렬'로 판정한 것만 새 주소로 바꿈
#     (0x10000 밑 작은 값, 확인필요·못찾음은 절대 안 건드리고 '손으로 볼 것'에 적음)
#  ③ IC 훅표(IntegratedConvenience 안 51칸)의 호출 자리·원래 대상도 같은 규칙으로 바꿈
#  ④ 소스 쪽은 지문 값이 박힌 줄을 새 값으로 바꾼 '사본'을 만듦(원본 소스는 안 건드림)
#  원본 payload·소스는 절대 덮어쓰지 않음. 결과는 --out 폴더에 새로 만들고, 바꾼 곳은 전부 적용_보고.txt 에 적음.
#
#  사용:
#   python3 apply_new.py --map <find_new 결과 자리_대응.tsv> --payload <옛 payload 폴더> --out <새 폴더>
#                        --stamp 0x새시간표 --size 0x새크기 [--sha 새원본exe해시] [--addr] [--src <소스 폴더> ...]
#  주의: 지문만 바꾸고 주소를 안 고치면 모듈이 엉뚱한 자리를 건드려 튕길 수 있음.
#        지문은 '안전 잠금'이므로 자리_대응.tsv 의 확인필요 항목을 다 본 뒤 마지막에 바꿀 것.
import os,sys,re,struct,shutil,subprocess,argparse,collections
ap=argparse.ArgumentParser()
ap.add_argument('--map',required=True);ap.add_argument('--payload',required=True);ap.add_argument('--out',required=True)
ap.add_argument('--stamp',required=True);ap.add_argument('--size',required=True);ap.add_argument('--sha',default='')
ap.add_argument('--addr',action='store_true');ap.add_argument('--strict',action='store_true',help='정확 판정만 바꿈(정렬은 손으로)');ap.add_argument('--src',nargs='*',default=[])
A=ap.parse_args()
OLD_STAMP,OLD_SIZE=0x6a983c25,0x29ff000
OLD_SHA='87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a'
NEW_STAMP,NEW_SIZE=int(A.stamp,16),int(A.size,16)
# 소스로 다시 빌드하는 우리 모듈(바이트 교체 대상 아님 — 소스 고치고 다시 빌드)
OURS={'SAN14_ModSettings.dll','SAN14_ICGuard.dll','SAN14_HiredFace.dll','SAN14_RegionOfficer.dll','SAN14_Range.dll','SAN14_StatusColor.dll','SAN14_SceneSkip.dll',
      'SAN14_ExecuteLock.dll','SAN14_ItemPicture.dll','SAN14_Traits9Check.dll','SAN14_Notice.dll','SAN14_Patch27.dll','SAN14_DomesticTalent.dll',
      'SAN14_AutoPersonnelRepair.dll','SAN14_PrisonerReturn.dll','SAN14_SettingsHelpScenic.dll','SAN14_DomesticAI.dll',
      'SAN14_DomesticPlayer.dll','SAN14_AutoMerit.dll','SAN14_Diag.dll'}   # R43: 22번 플레이어·2번 새 엔진·종합 진단도 소스 모듈
SKIPPRE=('python','libcrypto','libffi','vcruntime','dinput8','dxgi','version')
if os.path.exists(A.out):sys.exit(f'{A.out} 가 이미 있음 — 새 폴더 이름을 주세요(덮어쓰기 안 함)')
shutil.copytree(A.payload,A.out)
rep=[];hand=[];changed={}   # 파일이름 -> (옛 해시, 새 해시)
import hashlib
def R(s):rep.append(s)
# ---- 자리 대응표 읽기
M=collections.defaultdict(dict)   # 모듈 -> 옛RVA -> (새RVA, 판정)
ICM={}
for l in open(A.map,encoding='utf-8').read().splitlines()[1:]:
    c=l.split('\t')
    if len(c)<13:continue
    src,mod,kind,rva,newr,ver=c[0],c[1],c[2],c[3],c[11],c[12]
    try:o=int(rva,16)
    except ValueError:continue
    n=int(newr,16) if newr.startswith('0x') else None
    if src.startswith('DLL상수'):M[mod][o]=(n,ver)
    if src=='IC훅표':ICM[o]=(n,ver)
def good(ver):return ver.startswith('정확') or (not A.strict and ver.startswith('정렬'))
# ---- PE 구역(가상주소 -> 파일 위치)
def sections(b):
    pe=struct.unpack_from('<I',b,0x3c)[0];ns=struct.unpack_from('<H',b,pe+6)[0];osz=struct.unpack_from('<H',b,pe+20)[0]
    base=struct.unpack_from('<Q',b,pe+24+24)[0];st=pe+24+osz;out=[]
    for i in range(ns):
        vs,va,rs,raw=struct.unpack_from('<IIII',b,st+i*40+8)
        out.append((va,max(vs,rs),raw))
    return base,out
def foff(secs,base,vaddr):
    r=vaddr-base
    for va,sz,raw in secs:
        if va<=r<va+sz:return raw+(r-va)
    return None
INS=re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(\S+)\s*(.*)$')
for d,ds,fs in os.walk(A.out):
    for f in fs:
        if not f.lower().endswith('.dll') or f in OURS or f.startswith(SKIPPRE):continue
        p=os.path.join(d,f);b=bytearray(open(p,'rb').read());ch=0
        # ① 지문: 명령 안 즉시값/자료 안 4바이트 모두
        for old,new,nm in((OLD_STAMP,NEW_STAMP,'시간표'),(OLD_SIZE,NEW_SIZE,'크기')):
            ob,nb=struct.pack('<I',old),struct.pack('<I',new);i=b.find(ob)
            while i>=0:b[i:i+4]=nb;ch+=1;R(f'{f}\t파일 0x{i:x}\t지문 {nm}\t0x{old:x} → 0x{new:x}');i=b.find(ob,i+4)
        if A.sha:
            for enc in(lambda x:x.lower().encode(),lambda x:x.upper().encode(),lambda x:x.lower().encode('utf-16-le'),lambda x:x.upper().encode('utf-16-le'),lambda x:bytes.fromhex(x)):
                ob,nb=enc(OLD_SHA),enc(A.sha);i=b.find(ob)
                while i>=0:b[i:i+len(ob)]=nb;ch+=1;R(f'{f}\t파일 0x{i:x}\t지문 exe 해시\t바꿈');i=b.find(ob,i+len(ob))
        # ② 주소 상수
        if A.addr and f in M:
            base,secs=sections(b)
            out=subprocess.run(['objdump','-d','-M','intel',p],capture_output=True,text=True,errors='replace').stdout
            for line in out.splitlines():
                m=INS.match(line)
                if not m or m.group(3) not in('add','lea','mov','cmp','sub'):continue
                if 'rip' in m.group(4):continue   # R43: [rip+x] 는 모듈 안 자기 자료 참조(게임 주소 아님) — 우연히 게임 함수 시작과 같은 값이면 망가뜨림
                for h in re.findall(r'(?<![\w#])0x([0-9a-f]{4,7})\b',m.group(4).split('#')[0]):
                    v=int(h,16)
                    if v not in M[f]:continue
                    n,ver=M[f][v]
                    if v<0x10000 or not good(ver) or n is None:
                        if n!=v:hand.append(f'{f}\t0x{int(m.group(1),16):x}\t{m.group(3)} {m.group(4)[:60]}\t판정 {ver}\t새 0x{n:x}' if n is not None else f'{f}\t0x{int(m.group(1),16):x}\t{m.group(3)} {m.group(4)[:60]}\t판정 {ver}')
                        continue
                    if n==v:continue
                    ib=bytes.fromhex(m.group(2).replace(' ',''));k=ib.rfind(struct.pack('<I',v))
                    fo=foff(secs,base,int(m.group(1),16))
                    if k<0 and fo is not None:   # R43: objdump 가 긴 명령을 두 줄로 끊어 즉시값이 다음 줄에 있을 때 → 파일에서 명령 바이트(15) 직접 읽음
                        ib=bytes(b[fo:fo+15]);k=ib.find(struct.pack('<I',v),len(m.group(2).replace(' ',''))//2-3)
                    if k<0 or fo is None:hand.append(f'{f}\t0x{int(m.group(1),16):x}\t즉시값 자리를 못 찾음\t{line.strip()[:80]}');continue
                    b[fo+k:fo+k+4]=struct.pack('<I',n);ch+=1;R(f'{f}\t파일 0x{fo+k:x}\t주소\t0x{v:x} → 0x{n:x} ({ver})')
        # ③ IC 훅표
        if A.addr and f=='SAN14_IntegratedConvenience.dll':
            o=0xdd9d0
            for _ in range(200):
                if o+0x28>len(b):break
                a,t,det,l,op=struct.unpack_from('<5Q',b,o)
                if not(0x1000<=a<0x2a00000 and l<64):break
                for slot,val in((0,a),(8,t)):
                    if val==0:continue
                    n,ver=ICM.get(val,(None,'대장에 없음'))
                    if n is None or not good(ver):hand.append(f'{f}\t훅표 0x{o:x}+{slot}\t0x{val:x}\t판정 {ver}');continue
                    if n!=val:struct.pack_into('<Q',b,o+slot,n);ch+=1;R(f'{f}\t파일 0x{o+slot:x}\tIC훅표\t0x{val:x} → 0x{n:x} ({ver})')
                o+=0x28
        if ch:
            changed[f]=(hashlib.sha256(open(p,'rb').read()).hexdigest(),hashlib.sha256(bytes(b)).hexdigest());open(p,'wb').write(b)
# ④ 소스 사본
SRC_SUB=[(re.compile(r'0x0*6a983c25',re.I),f'0x{NEW_STAMP:x}'),(re.compile(r'\b1788361765\b'),str(NEW_STAMP)),
         (re.compile(r'0x0*29ff000\b',re.I),f'0x{NEW_SIZE:x}'),(re.compile(r'\b44036096\b'),str(NEW_SIZE))]
if A.sha:SRC_SUB.append((re.compile(OLD_SHA,re.I),A.sha.lower()))
def arr(h):return ','.join(f'0x{x:02x}' for x in bytes.fromhex(h))
ARR=re.compile(r'\{\s*((?:0x[0-9a-fA-F]{2}\s*,\s*){31}0x[0-9a-fA-F]{2})\s*,?\s*\}')
for s in A.src:
    dst=os.path.join(A.out+'_소스사본',os.path.basename(os.path.normpath(s)));shutil.copytree(s,dst)
    for d,ds,fs in os.walk(dst):
        for f in fs:
            p=os.path.join(d,f)
            try:t=open(p,encoding='utf-8').read()
            except Exception:continue
            t2=t
            for rx,new in SRC_SUB:t2=rx.sub(new,t2)
            # 바이트 교체로 해시가 바뀐 모듈을 '알려진 판'으로 확인하는 곳 → 새 해시로
            def fix(m):
                h=bytes(int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})',m.group(1))).hex()
                for nm,(o,n) in changed.items():
                    if h==o:R(f'소스 {os.path.relpath(p,dst)}\t{nm} 해시 교체(바이트 배열)');return '{'+arr(n)+'}'
                return m.group(0)
            t2=ARR.sub(fix,t2)
            for nm,(o,n) in changed.items():
                if o in t2.lower():t2=re.sub(o,n,t2,flags=re.I);R(f'소스 {os.path.relpath(p,dst)}\t{nm} 해시 교체(글자)')
            if t2!=t:open(p,'w',encoding='utf-8').write(t2);R(f'소스 {os.path.relpath(p,dst)}\t지문 값 교체')
with open(os.path.join(A.out,'적용_보고.txt'),'w',encoding='utf-8') as w:
    w.write('[해시가 바뀐 배포 DLL — 설치기 manifest(Values·Known)와 SHA256SUMS 도 새 값으로]\n'+'\n'.join(f'{k}\t{o} → {n}' for k,(o,n) in sorted(changed.items()))+'\n\n')
    w.write(f'바꾼 곳 {len(rep)}개, 손으로 볼 곳 {len(hand)}개\n\n[바꾼 곳]\n'+'\n'.join(rep)+'\n\n[손으로 볼 곳 — 자동으로 안 바꿈]\n'+'\n'.join(hand)+'\n')
print(f'바꾼 곳 {len(rep)}개, 손으로 볼 곳 {len(hand)}개 → {A.out}/적용_보고.txt')
