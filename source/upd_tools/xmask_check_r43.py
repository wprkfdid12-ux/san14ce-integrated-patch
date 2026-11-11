# R43: 바이너리 모듈의 'x?' 문자열 마스크 패턴(DragGive 식: 패턴 바이트 바로 뒤에 "xxxx????xx…\0") 이 새 exe 에서도 유일하게 맞는지 확인(보고만)
#  사용: python3 xmask_check_r43.py <newver 폴더> <payload_new dll 또는 폴더>...
import sys,os,re,struct
NV=sys.argv[1]
def text_of(path):
    data=open(path,'rb').read();p=struct.unpack_from('<I',data,0x3c)[0];o=struct.unpack_from('<H',data,p+20)[0];vs,va,rs,ra=struct.unpack_from('<IIII',data,p+24+o+8);return va,data[ra:ra+vs]
ova,OT=text_of(NV+'/old.dec.exe');nva,NT=text_of(NV+'/new.dec.exe')
def count_masked(hay,pat,mask,limit=3):
    # 첫 'x' 구간을 앵커로
    a=mask.find(b'x');b=a
    while b<len(mask) and mask[b:b+1]==b'x':b+=1
    core=pat[a:b];cnt=0;first=None;i=hay.find(core)
    while i!=-1:
        s=i-a
        if s>=0 and s+len(pat)<=len(hay) and all(mask[k:k+1]!=b'x' or hay[s+k]==pat[k] for k in range(len(pat))):
            cnt+=1
            if first is None:first=s
            if cnt>=limit:break
        i=hay.find(core,i+1)
    return cnt,first
def check(path):
    d=open(path,'rb').read();pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    out=[]
    for i in range(ns):
        n=d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace');vs,va,rs,ra=struct.unpack_from('<IIII',d,pe+24+opt+40*i+8)
        if n not in('.rdata','.rodata','.data') or not rs:continue
        seg=d[ra:ra+rs]
        for m in re.finditer(rb'[x?]{6,}\x00',seg):
            mask=m.group(0)[:-1];L=len(mask)
            if mask.count(b'x')<4:continue
            best=None
            for pad in range(0,33):
                ps=m.start()-L-pad
                if ps<0:break
                pat=seg[ps:ps+L];oc,of=count_masked(OT,pat,mask)
                if oc>=1:best=(ps,pat,oc,of);break
            if best is None:ps=m.start()-L;pat=seg[ps:ps+L];oc,of=0,None
            else:ps,pat,oc,of=best
            nc,nf=count_masked(NT,pat,mask)
            out.append((hex(va+ps),L,'옛 %d'%oc,'새 %d'%nc,'' if (oc==nc==1) else '★', pat[:12].hex(),mask[:16].decode()))
    if out:
        print('==',os.path.relpath(path),len(out),'개')
        for o in out:print('  ',*o)
for p in sys.argv[2:]:
    if os.path.isdir(p):
        for root,_,fs in os.walk(p):
            for f in sorted(fs):
                if f.lower().endswith('.dll'):check(os.path.join(root,f))
    else:check(p)
