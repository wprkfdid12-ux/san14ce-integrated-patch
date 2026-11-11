# AI모드_설치관리자.exe(.NET, PE32) 항목 이름 앞에 통합패치 겹침/충돌 표시를 붙인다.
# 방법: 메타데이터 전체를 .rsrc 구역 뒤로 옮기고(#US 문자열 힙만 늘림), 바뀐 이름을 힙 끝에 새로 넣은 뒤
#       그 이름을 읽는 ldstr 명령의 토큰만 새 자리로 바꾼다. 옛 문자열·다른 코드는 그대로.
import struct,sys
src,dst=sys.argv[1],sys.argv[2]
d=bytearray(open(src,'rb').read())
pe=struct.unpack_from('<I',d,0x3c)[0];opt=pe+24;optsz=struct.unpack_from('<H',d,pe+20)[0]
assert struct.unpack_from('<H',d,opt)[0]==0x10b
nsec=struct.unpack_from('<H',d,pe+6)[0];st=opt+optsz
SA,FA=struct.unpack_from('<II',d,opt+32)
secs=[list(struct.unpack_from('<8sIIII',d,st+40*k)) for k in range(nsec)]
names=[s[0].rstrip(b'\0') for s in secs];assert names==[b'.text',b'.rsrc',b'.reloc'],names
dd=opt+96
def ddir(i):return struct.unpack_from('<II',d,dd+8*i)
def rva2off(r):
    for nm,vs,va,rs,rp in secs:
        if va<=r<va+max(vs,rs):return r-va+rp
clr=rva2off(ddir(14)[0]);mdrva,mdsz=struct.unpack_from('<II',d,clr+8)
md=bytes(d[rva2off(mdrva):rva2off(mdrva)+mdsz])
# 메타데이터 루트
assert md[:4]==b'BSJB';vl=struct.unpack_from('<I',md,12)[0];p=16+vl;flags,ns=struct.unpack_from('<HH',md,p);p+=4
hdr_start=p;streams=[]
for _ in range(ns):
    off,sz=struct.unpack_from('<II',md,p);q=p+8;nm=b''
    while md[q]!=0:nm+=bytes([md[q]]);q+=1
    q+=1;q=(q+3)&~3;streams.append([nm.decode(),off,sz,p]);p=q
hdr_end=p
us=[s for s in streams if s[0]=='#US'][0]
heap=bytearray(md[us[1]:us[1]+us[2]])
# 기존 문자열 표
LAST=[1]
def entries(h):
    q=1;out={}
    while q<len(h):
        b=h[q]
        if b==0:q+=1;continue
        if b&0x80==0:ln=b;hl=1
        elif b&0xc0==0x80:ln=((b&0x3f)<<8)|h[q+1];hl=2
        else:ln=((b&0x1f)<<24)|(h[q+1]<<16)|(h[q+2]<<8)|h[q+3];hl=4
        out.setdefault(h[q+hl:q+hl+ln-1].decode('utf-16-le'),q);q+=hl+ln;LAST[0]=q
    return out
E=entries(heap)
# 실제로 쓰인 힙 끝(뒤쪽 0 채움 제거)
end=LAST[0]  # 마지막 실제 항목 끝(뒤쪽 0 채움 제외)
newheap=bytearray(heap[:end])
def add(s):
    b=s.encode('utf-16-le');term=1 if any(ord(c)>=0x80 or ord(c) in (1,2,3,4,5,6,7,8,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x27,0x2d,0x7f) for c in s) else 0
    ln=len(b)+1;off=len(newheap)
    if ln<0x80:newheap.extend(bytes([ln]))
    elif ln<0x4000:newheap.extend(bytes([0x80|(ln>>8),ln&0xff]))
    else:raise ValueError
    newheap.extend(b+bytes([term]));return off
RELABEL=[l.rstrip('\n').split('\t') for l in open(sys.argv[3],encoding='utf-8') if l.strip()]
# IL 안 ldstr 토큰 찾기(.text 중 메타데이터 밖)
t=secs[0];tbeg=t[4];tend=t[4]+t[3];mbeg=rva2off(mdrva);mend=mbeg+mdsz
done=[]
for old,new in RELABEL:
    assert old in E,old
    tok=struct.pack('<I',0x70000000|E[old]);pat=b'\x72'+tok
    hits=[];i=d.find(pat,tbeg)
    while 0<=i<tend:
        if not(mbeg<=i<mend):hits.append(i)
        i=d.find(pat,i+1)
    assert hits,('ldstr 없음',old)
    no=add(new)
    for h in hits:struct.pack_into('<I',d,h+1,0x70000000|no)
    done.append((old,new,len(hits)))
while len(newheap)%4:newheap.append(0)
# 새 메타데이터 조립
order=sorted(streams,key=lambda s:s[1])
body=bytearray();newoff={}
base=hdr_end
for s in order:
    data=newheap if s[0]=='#US' else md[s[1]:s[1]+s[2]]
    newoff[s[0]]=(base+len(body),len(data));body.extend(data)
    while len(body)%4:body.append(0)
m=bytearray(md[:hdr_end])
for s in streams:
    o,z=newoff[s[0]];struct.pack_into('<II',m,s[3],o,z)
m+=body
# .rsrc 뒤에 붙이기, .reloc 뒤로 밀기
al=lambda x,a:(x+a-1)//a*a
rs=list(secs[1]);rl=list(secs[2])
mdoff_in=al(rs[1],16);new_rs_vs=mdoff_in+len(m);new_rs_rs=al(new_rs_vs,FA)
rsrc_raw=bytes(d[rs[4]:rs[4]+rs[3]])+bytes(new_rs_rs-rs[3])
rsrc_raw=bytearray(rsrc_raw);rsrc_raw[mdoff_in:mdoff_in+len(m)]=m
reloc_raw=bytes(d[rl[4]:rl[4]+rl[3]])
new_rl_va=al(rs[2]+new_rs_vs,SA);new_rl_rp=rs[4]+new_rs_rs
out=bytearray(d[:rs[4]])+rsrc_raw+reloc_raw
# 구역 머리
secs[1][1]=new_rs_vs;secs[1][3]=new_rs_rs
secs[2][2]=new_rl_va;secs[2][4]=new_rl_rp
for k,s in enumerate(secs):struct.pack_into('<8sIIII',out,st+40*k,*s)
# 자료 디렉터리: reloc
rlva,rlsz=ddir(5);assert rlva==rl[2];struct.pack_into('<I',out,dd+8*5,new_rl_va)
# SizeOfImage·초기화 자료 크기
struct.pack_into('<I',out,opt+56,al(new_rl_va+rl[1],SA))
struct.pack_into('<I',out,opt+8,new_rs_rs+rl[3])  # SizeOfInitializedData
# CLI 머리: 메타데이터 자리
struct.pack_into('<II',out,clr+8,rs[2]+mdoff_in,len(m))
struct.pack_into('<I',out,opt+64,0)  # checksum
open(dst,'wb').write(out)
for o,n,h in done:print(h,'|',o[:24],'→',n[:60])
print('metadata',hex(rs[2]+mdoff_in),hex(len(m)),'reloc',hex(new_rl_va),'size',len(out))
