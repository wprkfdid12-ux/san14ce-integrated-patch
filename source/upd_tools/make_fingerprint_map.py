# 지문 의존 지도: "게임 exe 판(1.0.1)" 을 알아보려고 박아 둔 값이 어디 있는지 전부 찾음.
#  게임이 업데이트되면 이 값들이 전부 새 값으로 바뀌어야 함 → 하나라도 빠지면 '모드가 안 켜짐 / 설치기가 거부' 같은 찐빠.
#  소스(글자)와 배포 DLL(이진, 소스 없는 모듈 포함) 둘 다 찾음.
#  사용: python3 make_fingerprint_map.py <소스 폴더들...> --bin <배포 payload 폴더> > 지문_지도.tsv
import os,re,sys,struct
args=sys.argv[1:];bins=[];built=set()
if '--built' in args:i=args.index('--built');built={f.lower() for f in os.listdir(args[i+1]) if f.lower().endswith('.dll')};del args[i:i+2]
if '--bin' in args:i=args.index('--bin');bins=args[i+1:];args=args[:i]
STAMP=0x6a983c25;SIZE=0x29ff000;OLDSTAMP=0x6a6d9f72
EXE_SHA='87ed4e46590e0939c9efcaef14fa0f892917604dfda86e0a4be319fccffa2d6a'          # 원본(암호) SAN14CE_KO.exe 1.0.1
DEC_SHA='9a64e0bd88d34e1d46718dc9739b272eca10ad2d0bbebc368b0e5ba36b5c0014'          # 해제본
TXT=[('exe 시간표(1.0.1)',re.compile(r'0x0*6a983c25|\b1788361765\b',re.I)),
     ('exe 크기(1.0.1)',re.compile(r'0x0*29ff000\b|\b44036096\b',re.I)),
     ('exe 시간표(옛 1.0.0)',re.compile(r'0x0*6a6d9f72',re.I)),
     ('원본 exe SHA256',re.compile(EXE_SHA,re.I)),
     ('해제본 exe SHA256',re.compile(DEC_SHA,re.I)),
     ('판 번호 글자',re.compile(r'(?<![\d.])1\.0\.1(?![\d.])')),
     ('db 뿌리 주소(exe+0x277df18)',re.compile(r'0x0*277df18|0x14277df18',re.I))]
EXT=('.cpp','.h','.c','.cc','.S','.cs','.py','.ps1','.xml','.json','.txt','.md','.cmd','.src','.sh','.ini','.def')
SKIP=('build_out','overlay_r','audit','references','upd_tools','.git','__pycache__')
# 배포 DLL 해시(다른 모듈이 '알려진 판인지' 확인하려고 박아 둔 값 찾기용)
PH={}
for root in bins:
    import hashlib
    for d,ds,fs in os.walk(root):
        for f in fs:
            if f.lower().endswith('.dll'):PH[hashlib.sha256(open(os.path.join(d,f),'rb').read()).hexdigest()]=f
ARR=re.compile(r"\{\s*((?:0x[0-9a-fA-F]{2}\s*,\s*){31}0x[0-9a-fA-F]{2})\s*,?\s*\}")
HEX64=re.compile(r'(?<![0-9a-fA-F])[0-9a-fA-F]{64}(?![0-9a-fA-F])')
print('구분\t파일\t줄\t종류\t내용\t소스')
for root in args:
    for d,ds,fs in os.walk(root):
        ds[:]=[x for x in ds if not any(x.startswith(s) for s in SKIP)]
        for f in fs:
            if not f.endswith(EXT):continue
            p=os.path.join(d,f)
            try:L=open(p,encoding='utf-8',errors='replace').read().splitlines()
            except Exception:continue
            T='\n'.join(L)
            for m in ARR.finditer(T):
                n=T.count('\n',0,m.start())+1
                h=bytes(int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})',m.group(1))).hex()
                print(f'소스\t{p}\t{n}\t모듈 해시(바이트 배열)\t{h[:16]}… = '+(f'지금 배포 {PH[h]}' if h in PH else '게임 원본 exe 해시' if h==EXE_SHA else '해제본 exe 해시' if h==DEC_SHA else '지금 배포본 아님(옛 판 허용값)')+'\t-')
            for n,l in enumerate(L,1):
                for k,rx in TXT:
                    if rx.search(l):print(f'소스\t{p}\t{n}\t{k}\t{l.strip()[:160]}\t-')
                for m in []:
                    print(f'소스\t{p}\t{n}\t모듈 해시(바이트 배열)\t{h[:16]}… = '+(f'지금 배포 {PH[h]}' if h in PH else '게임 원본 exe 해시' if h==EXE_SHA else '해제본 exe 해시' if h==DEC_SHA else '지금 배포본 아님(옛 판 허용값)')+'\t-')
                for m in HEX64.finditer(l):
                    h=m.group(0).lower()
                    if h in PH:print(f'소스\t{p}\t{n}\t모듈 해시(글자)\t{h[:16]}… = 지금 배포 {PH[h]}\t-')
BIN=[('exe 시간표(1.0.1)',struct.pack('<I',STAMP)),('exe 크기(1.0.1)',struct.pack('<I',SIZE)),('exe 시간표(옛 1.0.0)',struct.pack('<I',OLDSTAMP)),
     ('원본 exe SHA256(글자)',EXE_SHA.encode()),('원본 exe SHA256(대문자)',EXE_SHA.upper().encode()),('원본 exe SHA256(바이트)',bytes.fromhex(EXE_SHA)),
     ('원본 exe SHA256(UTF-16)',EXE_SHA.encode('utf-16-le')),('원본 exe SHA256(UTF-16 대문자)',EXE_SHA.upper().encode('utf-16-le')),
     ('db 뿌리 주소 RVA',struct.pack('<I',0x277df18))]
for root in bins:
    for d,ds,fs in os.walk(root):
        for f in fs:
            if not f.lower().endswith(('.dll','.exe','.asi')):continue
            p=os.path.join(d,f);b=open(p,'rb').read()
            for h2,nm2 in PH.items():
                for form,pat in(('바이트',bytes.fromhex(h2)),('글자',h2.encode()),('대문자',h2.upper().encode())):
                    if pat in b and nm2!=f:print(f'배포DLL\t{p}\t-\t다른 모듈 해시({form})\t{nm2} 의 해시가 박혀 있음\t'+('소스로 다시 빌드' if f.lower() in built else '소스 없음 → 바이트 교체 필요'))
            for k,pat in BIN:
                i=b.find(pat);c=0;offs=[]
                while i>=0:c+=1;offs.append(i);i=b.find(pat,i+1)
                if c:print(f'배포DLL\t{p}\t-\t{k}\t{c}곳 (파일 위치 '+','.join(f'0x{o:x}' for o in offs[:6])+')\t'+('소스로 다시 빌드' if f.lower() in built else '소스 없음 → update_fingerprint.py 로 바이트 교체'))
