# scan_data_rvas.py 결과(TSV)를 걸러 진짜 '남은 옛 주소' 후보만 남긴다.
#  기준: 4바이트 정렬 + (뒤 5바이트 이상이 옛 exe 그 자리 바이트와 같음[guard 표] | 8바이트 절대 포인터 | ±64바이트 안에 다른 후보가 있음[표 군집] | 값이 함수 시작/알려진 자리)
import sys,os,struct,collections,bisect;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
TSV,NEW=sys.argv[1:3]
rows=[l.rstrip('\n').split('\t') for l in open(TSV,encoding='utf-8')][1:]
mods={}
def secs(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    return [struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
def mod(rel):
    if rel not in mods:
        d=open(os.path.join(NEW,rel),'rb').read();S=secs(d)
        def rd(rva,n):
            for vs,va,rs,ra in S:
                if va<=rva<va+max(vs,rs):return d[ra+rva-va:ra+rva-va+n]
            return b''
        mods[rel]=rd
    return mods[rel]
cand=[]
for r in rows:
    rel,sec,kind,mrva,val,vk,abs8,state,new,how,ctx=r
    if state!='옛그대로' or new=='-' or new==val:continue
    a=int(mrva,16);v=int(val,16)
    if a%4:continue
    rd=mod(rel);tail=rd(a+4,32);ob=X.ord_(v,32)
    k=0
    while k<len(tail) and k<len(ob) and tail[k]==ob[k]:k+=1
    cand.append((rel,sec,kind,a,v,vk,abs8,int(new,16),how,k))
# 군집
byrel=collections.defaultdict(list)
for c in cand:byrel[c[0]].append(c[3])
for r in byrel:byrel[r].sort()
out=[]
for c in cand:
    rel,sec,kind,a,v,vk,abs8,n,how,k=c
    L=byrel[rel];near=bisect.bisect_right(L,a+64)-bisect.bisect_left(L,a-64)-1
    strong=k>=5 or abs8=='abs8' or vk in('fn','site') or near>=2
    if not strong:continue
    why=[]
    if k>=5:why.append(f'뒤 {k}바이트=옛exe')
    if abs8:why.append('절대포인터')
    if vk in('fn','site'):why.append(vk)
    if near:why.append(f'군집{near}')
    out.append((rel,sec,kind,a,v,n,how,'+'.join(why),k))
print('\t'.join(['모듈','구역','종류','모듈RVA','옛값','새값','근거','왜','뒤일치']))
for o in sorted(out,key=lambda o:(o[0],o[3])):print('\t'.join([o[0],o[1],o[2],hex(o[3]),hex(o[4]),hex(o[5]),o[6],o[7],str(o[8])]))
