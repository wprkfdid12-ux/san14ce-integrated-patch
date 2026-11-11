# 모듈 바이너리의 자료 구역(.rdata/.data 등)과 코드 구역 안에서 '옛 게임 RVA' 가 그대로 남은 4/8바이트를 찾는다. (R43 2-3)
#  apply_new.py 는 코드 즉시값(objdump)만 바꿨으므로, IC 처럼 .rdata 표에 자리 목록을 둔 모듈이 빠진다(0xc5dd0 lea 표에서 발견).
#  사용: python3 scan_data_rvas.py <payload_cur> <payload_new>  → 표준출력에 모듈별 목록(TSV)
import sys,os,struct,bisect;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
CUR,NEW=sys.argv[1:3]
SKIP={'libcrypto-3-x64.dll','libffi-8.dll','python3.dll','python312.dll','vcruntime140.dll','vcruntime140_1.dll'}
OLDFN=set(X.ostart);OLDSITE=set(X.SITE);NEWFN=set(X.nstart)
# 옛 .text/.rdata/.data 범위
def secs(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    return [(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8)+(struct.unpack_from('<I',d,pe+24+opt+40*i+36)[0],) for i in range(ns)]
OS=X.OLD_SECS;otext=[(va,va+max(vs,rs)) for vs,va,rs,ra in OS]
def in_old_image(v):return any(a<=v<b for a,b in otext) and v>=0x1000
def interesting(v):
    if v in OLDFN or v in OLDSITE:return 'fn' if v in OLDFN else 'site'
    f=X.func_of(v)
    if f and v>=0x10000:return 'in-fn'
    if 0x1340000<=v<0x2a00000 and v>=0x10000:return 'data'
    return None
rows=[]
for root,_,fs in os.walk(NEW):
    for f in fs:
        if not f.lower().endswith(('.dll','.exe','.asi')) or f in SKIP:continue
        pn=os.path.join(root,f);rel=os.path.relpath(pn,NEW);pc=os.path.join(CUR,rel)
        dn=open(pn,'rb').read();dc=open(pc,'rb').read() if os.path.exists(pc) else None
        for name,vs,va,rs,ra,ch in secs(dn):
            if rs==0:continue
            sec=dn[ra:ra+rs];code=bool(ch&0x20000000)
            for i in range(0,rs-3):
                v=struct.unpack_from('<I',sec,i)[0]
                if v<0x1000 or v>=0x2a00000:continue
                k=interesting(v)
                if not k:continue
                # 8바이트 절대 포인터(0x140000000+v)인지
                abs8=i+4<=rs-4 and struct.unpack_from('<I',sec,i+4)[0]==1
                same=dc is not None and len(dc)>=ra+i+4 and dc[ra+i:ra+i+4]==sec[i:i+4]
                n,how=(X.map_code(v) if v<0x1340000 else X.map_data(v))
                changed=n is not None and n!=v
                rows.append((rel,name,'code' if code else 'data',hex(va+i),hex(v),k,'abs8' if abs8 else '','옛그대로' if same else '바뀜',hex(n) if n else '-',how,sec[max(0,i-8):i+12].hex()))
# 출력: 자료 구역 + 옛 그대로 + 새 주소가 다른 것 먼저
print('\t'.join(['모듈','구역','종류','모듈RVA','값','값종류','abs8','상태','새값','근거','주변']))
for r in sorted(rows,key=lambda r:(r[2]!='data',r[7]!='옛그대로',r[0],int(r[3],16))):print('\t'.join(r))
