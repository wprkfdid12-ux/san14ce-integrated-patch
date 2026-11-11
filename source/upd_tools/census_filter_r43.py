# census 결과에서 '모듈 자기 것'(unwind 정보·예외 처리표·자기 RVA 표) 칸을 걸러 낸다.
import csv,struct,sys,os,collections
P=sys.argv[1];TSV=sys.argv[2];OUT=sys.argv[3]
def unwind_cells(path):
    d=open(path,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    secs=[(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
    def fo(r):
        for n,vs,va,rs,ra in secs:
            if va<=r<va+rs:return ra+r-va
    size=struct.unpack_from('<I',d,pe+24+56)[0]
    cells=set()
    pd=[s for s in secs if s[0]==b'.pdata']
    if pd:
        n,vs,va,rs,ra=pd[0]
        for k in range(0,vs,12):
            b,e,u=struct.unpack_from('<III',d,ra+k)
            if not b:break
            o=fo(u&~1)
            if o is None:continue
            ver,cnt=d[o],d[o+2];flags=ver>>3
            ln=4+((cnt+1)&~1)*2
            if flags&3:ln+=4+16    # 처리기 RVA + 일부 처리기 자료(대략)
            if flags&4:ln+=12      # chained
            for x in range(u&~1,(u&~1)+ln,4):cells.add(x)
    return cells,size
cache={}
rows=list(csv.DictReader(open(TSV,encoding='utf-8'),delimiter='\t'))
keep=[];drop=collections.Counter()
for r in rows:
    m=r['모듈']
    if m not in cache:cache[m]=unwind_cells(os.path.join(P,m))
    cells,size=cache[m]
    if r['구역']!='code':
        a=int(r['자리'],16)&~3
        if a in cells:drop['unwind']+=1;continue
    keep.append(r)
with open(OUT,'w',encoding='utf-8',newline='') as f:
    w=csv.DictWriter(f,fieldnames=list(rows[0].keys()),delimiter='\t');w.writeheader();[w.writerow(r) for r in keep]
print('남음',len(keep),'뺌',dict(drop))
