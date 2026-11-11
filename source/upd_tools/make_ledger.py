# 연결 자리 대장 만들기 (1.0.1 기준): 우리 소스 + ModCore 시작 기록 + 통합편의 훅 표 + 소스 없는 DLL 안의 게임 주소 상수(추정)
#  사용: python3 make_ledger.py <해제된 1.0.1 exe> <runtime.log> <패치 payload 폴더> <r41/src> > 연결자리_대장.tsv
import re,sys,os,struct,glob,subprocess,collections
EXE,RTLOG,PAY,SRC=sys.argv[1:5]
d=open(EXE,'rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0];n=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
secs=[]
for i in range(n):
    o=pe+24+opt+40*i;nm=d[o:o+8].rstrip(b'\0').decode();vs,va,rs,ra=struct.unpack_from('<IIII',d,o+8);secs.append((nm,va,vs,ra,rs))
def sec_of(r):
    for nm,va,vs,ra,rs in secs:
        if va<=r<va+max(vs,rs):return nm
    return '?'
def off(r):
    for nm,va,vs,ra,rs in secs:
        if va<=r<va+rs:return r-va+ra
    return None
# .pdata 함수 경계
pd=[s for s in secs if s[0]=='.pdata'][0];funcs=[]
for k in range(0,pd[2],12):
    b,e,u=struct.unpack_from('<III',d,pd[3]+k)
    if b==0:break
    funcs.append((b,e))
funcs.sort();import bisect;fb=[f[0] for f in funcs];fstart=set(fb)
import pickle
riptgt=pickle.load(open(sys.argv[5],'rb')) if len(sys.argv)>5 else set()
def func_of(r):
    i=bisect.bisect_right(fb,r)-1
    if i>=0 and r<funcs[i][1]:return funcs[i]
    return None
rows=[]   # (출처, 모듈, 종류, rva, 길이, 메모)
# 1) 우리 소스
HEX=re.compile(r'0x([0-9a-fA-F]{4,7})\b')
for f in sorted(glob.glob(SRC+'/*.cpp')+glob.glob(SRC+'/*.S')+glob.glob(SRC+'/*.h')):
    base=os.path.basename(f)
    if base.startswith(('patch_tester','scene_test')):continue
    for ln,line in enumerate(open(f,encoding='utf-8',errors='replace'),1):
        code=line.split('//')[0]
        if not re.search(r'game|plan\.(jump|call)|Ret[A-Z]|\bRVA\b|fn<',code):continue
        if re.search(r'\b(ic|IC|icg|domestic|dm|mt|t9|conv)\b\s*[+,]',code) and 'game' not in code:continue
        for m in HEX.finditer(code):
            v=int(m.group(1),16)
            if not (0x1000<=v<0x2a00000):continue
            pre=code[max(0,m.start()-28):m.start()]
            if re.search(r'(\bdb\(\)|\bd|\bw|world\(\)|\bforce|\bruler|\barea|\bp|\ba|\bu|\bf|state|layout|ctl|row|def|list|tab|e)\s*,\s*$',pre) and 'game' not in pre:continue
            kind='훅' if 'plan.' in code else ('코드' if v<0x1341000 else '자료')
            rows.append(('소스',base,kind,v,'',f'{base}:{ln}'))
# 2) ModCore 시작 기록
s=open(RTLOG,'rb').read();i=s.rfind(b'SAN14 ModCore 1.0');s=s[i:].decode('utf-8','replace')
for m in re.finditer(r'(SAN14CE\.[A-Za-z0-9_.]+?|SortiePresets|RelatedOfficersTab\.RC2_4\.HoverPin) RVA=([0-9a-f]{8}) len=(\d+)',s):
    rows.append(('ModCore기록',m.group(1),'훅',int(m.group(2),16),m.group(3),''))
# 3) 통합편의 훅 표 (IC 파일 안 0x28 간격 표)
ic=open(PAY+'/base/SAN14_IntegratedConvenience.dll','rb').read()
def ok(o):
    if o+0x28>len(ic):return False
    a,t,det,l,op=struct.unpack_from('<5Q',ic,o);return 0x1000<a<0x2a00000 and 0x180000000<=det<0x181000000 and 1<=l<=16 and op in (0,0xe8,0xe9,0xeb)
o=0
while o<len(ic)-0x28:
    if ok(o):
        while ok(o):
            a,t,det,l,op=struct.unpack_from('<5Q',ic,o)
            rows.append(('IC훅표','SAN14_IntegratedConvenience.dll','훅' if op else '포인터',a,str(l),f'원래대상 0x{t:x} 우회 0x{det:x}'))
            rows.append(('IC훅표','SAN14_IntegratedConvenience.dll','코드',t,'',f'0x{a:x} 의 원래 대상'))
            o+=0x28
    else:o+=8
# 4) 소스 없는 DLL 안 게임 주소 상수(추정): add r,imm / lea [r+imm] / mov r,imm 중 게임 코드·자료 범위
ours={'SAN14_ModSettings.dll','SAN14_ICGuard.dll','SAN14_HiredFace.dll','SAN14_RegionOfficer.dll','SAN14_Range.dll','SAN14_StatusColor.dll','SAN14_SceneSkip.dll','SAN14_ExecuteLock.dll','SAN14_ItemPicture.dll','SAN14_Traits9Check.dll','SAN14_Notice.dll','SAN14_Patch27.dll','SAN14_DomesticTalent.dll','SAN14_AutoPersonnelRepair.dll','SAN14_PrisonerReturn.dll','SAN14_SettingsHelpScenic.dll','SAN14_DomesticAI.dll'}
IMM=re.compile(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*(add|lea|mov|cmp|sub)\s+(.*)$')
for dll in sorted(glob.glob(PAY+'/**/*.dll',recursive=True)):
    b=os.path.basename(dll)
    if b in ours or b.startswith(('python','libcrypto','libffi','vcruntime','dinput8','dxgi','version')) or b.endswith('.pyd'):continue
    try:out=subprocess.run(['objdump','-d','-M','intel',dll],capture_output=True,text=True,errors='replace').stdout
    except Exception:continue
    cnt=collections.Counter()
    for line in out.splitlines():
        m=IMM.match(line)
        if not m or 'rip' in m.group(3):continue   # R43: [rip+x] 는 모듈 안 참조 → 제외
        for h in re.findall(r'(?<![\w#])0x([0-9a-f]{4,7})\b',m.group(3).split('#')[0]):
            v=int(h,16)
            if 0x1000<=v<0x2a00000 and v not in (0xffff,0x10000,0x100000,0x7fffff,0xffffff,0x10ffff,0x921fb):cnt[v]+=1
    for v,c in cnt.items():
        if v<0x1341000:
            if v not in fstart:continue          # 함수 시작만(우연히 맞는 상수 거름)
            rows.append(('DLL상수(함수)',b,'코드',v,'',f'{c}곳'))
        elif v in riptgt:rows.append(('DLL상수(자료)',b,'자료',v,'',f'{c}곳'))
# 출력
seen=set();print('출처\t모듈\t종류\tRVA\t길이\t구역\t함수시작\t함수끝\t함수안위치\t원본바이트(16)\t메모')
for src,mod,kind,v,l,memo in rows:
    k=(src,mod,v,l)
    if k in seen:continue
    seen.add(k);f=func_of(v);o_=off(v);bs=d[o_:o_+16].hex() if o_ is not None else ''
    print('\t'.join([src,mod,kind,f'0x{v:x}',l,sec_of(v),f'0x{f[0]:x}' if f else '',f'0x{f[1]:x}' if f else '',f'+0x{v-f[0]:x}' if f else '',bs,memo]))
