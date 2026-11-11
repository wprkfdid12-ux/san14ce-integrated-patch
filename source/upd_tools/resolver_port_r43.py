# R43: ModCore 리졸버(SAN14_Resolver_IP01/CEKO.dll)의 게임 함수 '지문 표'(0x220바이트 항목 140개: total,deref,reloff,add,core_off,core_len,패턴[0x104],마스크[0x104])를
#  새 exe 기준으로 다시 만든다. 1.0.0→1.0.1 때 src42d/port/port_resolver.py 가 하던 일 — 2-1 에서 빠뜨려 R43 첫 실행에서 "리졸버가 게임 함수 지문을 못 찾았습니다"(R43-MC1) 가 떴다.
#  규칙: 옛 exe .text 에서 항목 패턴(마스크)으로 유일 일치 자리 → newmap 으로 새 자리 → 새 exe 바이트로 패턴 교체(마스크 그대로) → 새 exe 에서 core 창이 유일한지 확인(아니면 창 다시 고름)
#  사용: python3 resolver_port_r43.py <입력 dll(payload_cur 것)> <출력 dll> [--dry]
import sys,os,struct,bisect,collections;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
SRC,DST=sys.argv[1:3];DRY='--dry' in sys.argv
d=bytearray(open(SRC,'rb').read())
pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
S=[(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8) for i in range(ns)]
def off(r):
    for n,vs,va,rs,ra in S:
        if va<=r<va+max(vs,rs):return ra+r-va
# 옛/새 .text
def text_of(data):
    p=struct.unpack_from('<I',data,0x3c)[0];o=struct.unpack_from('<H',data,p+20)[0];vs,va,rs,ra=struct.unpack_from('<IIII',data,p+24+o+8);return va,data[ra:ra+vs]
ova,OT=text_of(X.OLD_D);nva,NT=text_of(X.NEW_D)
def count_masked(hay,pat,mask,co,cl,limit=3):
    core=pat[co:co+cl];cnt=0;first=None;i=hay.find(core)
    while i!=-1:
        s=i-co
        if s>=0 and s+len(pat)<=len(hay) and all(mask[k]==0 or hay[s+k]==pat[k] for k in range(len(pat))):
            cnt+=1
            if first is None:first=s
            if cnt>=limit:break
        i=hay.find(core,i+1)
    return cnt,first
# 항목 찾기 — 두 가지 형식
#  A(ModCore 리졸버, 0x220): u32 total,deref,reloff,add,core_off,core_len @0 / 패턴 @0x18 / 마스크 @0x11c
ents=[]   # (entry rva, 형식, total, deref, reloff, add, core_off, core_len, 패턴 오프셋, 마스크 오프셋, core 저장 위치(없으면 None))
for n,vs,va,rs,ra in S:
    if n not in('.rodata','.data','.rdata'):continue
    for k in range(0,rs-0x100,8):
        total,deref,reloff,add,co,cl=struct.unpack_from('<6I',d,ra+k)
        if 8<=total<=0x104 and 3<cl<=total and co+cl<=total and deref<=0x104 and reloff<=0x104 and add<0x100000 and k+0x11c+total<=rs:
            mask=bytes(d[ra+k+0x11c:ra+k+0x11c+total])
            if len(mask)==total and all(b in(0,0xff) for b in mask) and mask.count(0xff)>=4:ents.append((va+k,'A',total,deref,reloff,add,co,cl,0x18,0x11c,('II',0x10)));continue
        # B(PresetGroups 리졸버, 0x240): u32 total,core_off,core_len,flag @0 / 패턴 @0x10 / 마스크 @0x128
        # C(RelatedOfficers 리졸버, 0x190): u32 total,core_off,core_len @0 / 패턴 @0xc(최대 0xc2) / 마스크 @0xce
        t32,co2,cl2,fl=struct.unpack_from('<4I',d,ra+k)
        for fmt,PO,MO,pmax in(('B',0x10,0x128,0x118),('C',0xc,0xce,0xc2)):
            if fmt=='B' and fl>1:continue
            if 8<=t32<=pmax and 3<cl2<=t32 and co2+cl2<=t32 and k+MO+t32<=rs:
                mask=bytes(d[ra+k+MO:ra+k+MO+t32])
                if len(mask)==t32 and all(b in(0,0xff) for b in mask) and mask.count(0xff)>=4 and all(b==0xff for b in mask[co2:co2+cl2]) and all(b==0 for b in d[ra+k+PO+t32:ra+k+MO]) and all(b==0 for b in d[ra+k+MO+t32:ra+k+MO+pmax]):
                    ents.append((va+k,fmt,t32,0,0,0,co2,cl2,PO,MO,('II',4)));break
print("항목 수",len(ents),collections.Counter(e[1] for e in ents));assert ents,"지문 항목 없음"
rep=[];changed=0;fails=0
for i,(ent,fmt,total,deref,reloff,add,co,cl,PO,MO,CS) in enumerate(ents):
    fo=off(ent);pat=bytes(d[fo+PO:fo+PO+total]);mask=bytes(d[fo+MO:fo+MO+total])
    oc,os_=count_masked(OT,pat,mask,co,cl)
    if oc!=1:rep.append((i,hex(ent),'FAIL 옛 exe 에서 유일하지 않음',oc));fails+=1;continue
    oldres=ova+os_
    n,how=X.map_code(oldres)
    if n is None:
        # 옛 패턴 그대로 새 exe 에서 유일하면 그 자리
        c2,f2=count_masked(NT,pat,mask,co,cl)
        if c2==1:n=nva+f2;how='옛 패턴 그대로 새 exe 유일'
    if n is None:
        # 함수 안 오프셋: 앞 명령 기준
        ko=X._akeys('old');j=bisect.bisect_right(ko,oldres)-1
        if j>=0 and ko[j]!=oldres:
            nb,h2=X.map_code(ko[j])
            if nb is not None:n=nb+(oldres-ko[j]);how='앞 명령 '+h2
    if n is None:rep.append((i,hex(ent),'FAIL 새 자리 못 찾음',hex(oldres),how));fails+=1;continue
    s=n-nva;npat=bytes(NT[s:s+total])
    if len(npat)<total:rep.append((i,hex(ent),'FAIL 새 자리 범위 밖',hex(oldres),hex(n)));fails+=1;continue
    same=X.same_code(oldres,n,total)
    diff_unmasked=sum(1 for k in range(total) if mask[k] and npat[k]!=pat[k])
    # 유일성(새 exe)
    def unique_with(c0,c1):
        c,f=count_masked(NT,npat,mask,c0,c1);return c==1 and f==s
    nco,ncl=co,cl
    if not unique_with(nco,ncl):
        best=None;runs=[];k=0
        while k<total:
            if mask[k]==0xff:
                j=k
                while j<total and mask[j]==0xff:j+=1
                runs.append((k,j));k=j
            else:k+=1
        for a,b in runs:
            for L in range(min(b-a,24),3,-1):
                for st in range(a,b-L+1):
                    if unique_with(st,L):best=(st,L);break
                if best:break
            if best:break
        if not best:rep.append((i,hex(ent),'FAIL 새 exe 에서 유일한 core 없음',hex(oldres),hex(n),how));fails+=1;continue
        nco,ncl=best
    if not same and diff_unmasked>0:
        # 명령이 달라진 자리: 보고만(패턴은 새 바이트로 바꿈 — 리졸버가 찾는 '자리'는 같으므로). 마스크 밖 차이가 큰 것은 손으로 봐야 함
        rep.append((i,hex(ent),'주의 명령 다름(마스크 밖 차이 %d/%d)'%(diff_unmasked,total),hex(oldres),'→',hex(n),how,'deref/reloff/add',(deref,reloff,add)))
    else:rep.append((i,hex(ent),'OK' if (npat==pat and (nco,ncl)==(co,cl)) else 'REPATTERN(차이 %d)'%diff_unmasked,hex(oldres),'→',hex(n),how,'core',(co,cl),'→',(nco,ncl)))
    if not DRY:
        d[fo+PO:fo+PO+total]=npat
        struct.pack_into('<II',d,fo+CS[1],nco,ncl)
    if npat!=pat or (nco,ncl)!=(co,cl):changed+=1
if not DRY:open(DST,'wb').write(d)
for r in rep:print(r)
print('항목',len(ents),'바뀜',changed,'실패',fails,collections.Counter(r[2].split('(')[0].split(' ')[0] for r in rep))
