# 24번 능력치 보너스 제거(fix_nostat) 자료 만들기 — 설치기 Stat.cs(LWC 풀기/묶기 + 299칸 0으로)를 파이썬으로 그대로 옮김.
#  설치기는 설치 때 원본에서 직접 만들므로 이 도구는 (가) 새 원본에서 변환이 되는지 확인 (나) 결과 해시(manifest fix_nostat) 계산 용도.
#  사용: python3 fixdata_nostat.py <installer101/Stat.cs> <원본 fixdataexce.s14> [<출력 파일>]
import sys,re,base64,hashlib
STAT,SRC=sys.argv[1:3];OUT=sys.argv[3] if len(sys.argv)>3 else None
t=open(STAT,encoding='utf-8').read()
RAW=int(re.search(r'RawSize=(\d+)',t).group(1));START=int(re.search(r'Start=(\d+)',t).group(1));CK=int(re.search(r'ChecksumAt=(\d+)',t).group(1))
HDR=base64.b64decode(re.search(r'ExpectedHeader=Convert.FromBase64String\("([^"]+)"\)',t).group(1))
COUNTS=[(int(a),int(b)) for a,b in re.findall(r'new\[\]\{(\d+),(\d+)\}',re.search(r'Counts=\{([^;]*)\};',t).group(1))]
FIELDS=[(int(o),base64.b64decode(b)) for o,b in re.findall(r'new StatField\{Offset=(\d+),Before=Convert.FromBase64String\("([^"]+)"\)\}',t)]
MOD=0x3fde03a3
def u32(b,p):return int.from_bytes(b[p:p+4],'little')
def powm(a,n):r=1;v=a%MOD

def powm(a,n):
    r=1;v=a%MOD
    while n>0:
        if n&1:r=r*v%MOD
        v=v*v%MOD;n>>=1
    return r
def ssum(raw):return sum(raw)&65535
class Bits:
    def __init__(s,b,start):s.d=b;s.bit=start*8;s.end=len(b)*8
    def one(s):
        if s.bit>=s.end:raise IOError('LWC 비트열이 끊어졌습니다.')
        v=(s.d[s.bit>>3]>>(7-(s.bit&7)))&1;s.bit+=1;return v
    def number(s):
        n=1;v=0
        while s.one():
            n+=1
            if n>24:raise IOError('LWC 길이가 올바르지 않습니다.')
        for _ in range(n):v=v*2+s.one()
        return (1<<n)-2+v
def decode(inp):
    if len(inp)<START+268 or len(inp)>4000000:raise IOError('지원하지 않는 능력치 자료 크기입니다.')
    for i in range(START):
        if (i<CK or i>=CK+4) and inp[i]!=HDR[i]:raise IOError('지원하는 한국어 CE 공용 자료 형식이 아닙니다. (머리 %d 번째 바이트 다름)'%i)
    if u32(inp,START)!=0x1a43574c or u32(inp,START+4)!=RAW or u32(inp,START+8)!=len(inp)-START-268:raise IOError('LWC 자료 길이가 올바르지 않습니다. RawSize=%d(기대 %d)'%(u32(inp,START+4),RAW))
    table=bytes(inp[START+12:START+268]);header=bytes(inp[:START])
    if len(set(table))!=256:raise IOError('LWC 사전이 올바르지 않습니다.')
    raw=bytearray(RAW);bits=Bits(inp,START+268);used=0
    while used<RAW:
        v=bits.number()
        if v<256:raw[used]=table[v];used+=1
        else:
            dist=v-256;count=bits.number()+3
            if dist<1 or dist>used or count>RAW-used:raise IOError('LWC 복사 범위가 올바르지 않습니다.')
            for _ in range(count):raw[used]=raw[used-dist];used+=1
    if powm(u32(inp,CK),0x220f799)!=ssum(raw):raise IOError('공용 자료 체크섬이 일치하지 않습니다.')
    for off,cnt in COUNTS:
        if u32(raw,off)!=cnt:raise IOError('지원하는 공용 자료 테이블 구성이 아닙니다. (%d: %d, 기대 %d)'%(off,u32(raw,off),cnt))
    return raw,table,header
def encode(raw,table,header):
    inv=[0]*256
    for i,b in enumerate(table):inv[b]=i
    body=bytearray();acc=0;bits=0
    for b in raw:
        v=inv[b];n=1
        while (1<<(n+1))<=v+2:n+=1
        prefix=(1<<n)-2
        for part in(prefix,v-prefix):
            acc=(acc<<n)|part;bits+=n
            while bits>=8:
                bits-=8;body.append((acc>>bits)&0xff);acc&=(1<<bits)-1
    if bits:body.append((acc<<(8-bits))&0xff)
    out=bytearray(START+268+len(body));out[:START]=header
    out[CK:CK+4]=powm(ssum(raw),5).to_bytes(4,'little');out[START:START+4]=(0x1a43574c).to_bytes(4,'little');out[START+4:START+8]=RAW.to_bytes(4,'little');out[START+8:START+12]=len(body).to_bytes(4,'little')
    out[START+12:START+268]=table;out[START+268:]=body
    r2,_,_=decode(bytes(out))
    if bytes(r2)!=bytes(raw):raise IOError('공용 자료 재구성 검증에 실패했습니다.')
    return bytes(out)
def suppress(inp):
    raw,table,header=decode(inp)
    original=True;zero=True;diffs=[]
    for off,before in FIELDS:
        cur=bytes(raw[off:off+len(before)])
        if cur!=before:original=False;diffs.append((off,before.hex(),cur.hex()))
        if any(cur):zero=False
    if not original and not zero:raise IOError('능력치 보너스에 확인되지 않은 일부 수정이 있습니다. 다른 칸 %d개: %s'%(len(diffs),diffs[:5]))
    if zero:return inp,'이미 제거됨'
    for off,before in FIELDS:raw[off:off+len(before)]=bytes(len(before))
    return encode(raw,table,header),'제거함(%d칸)'%len(FIELDS)
src=open(SRC,'rb').read()
out,how=suppress(src)
print('원본',hashlib.sha256(src).hexdigest(),'→ nostat',hashlib.sha256(out).hexdigest(),how,'크기',len(src),'→',len(out))
if OUT:open(OUT,'wb').write(out)
