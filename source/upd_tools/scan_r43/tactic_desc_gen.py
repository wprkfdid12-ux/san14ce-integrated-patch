raw=open('raw.bin','rb').read()
W=0x5e;n1=0x2e0f7
KIND={1:'공격',2:'원거리',3:'방어',4:'제압',5:'계략',6:'지원',7:'병기',8:'배'}
ST={1:'무력',2:'지력'}
EF={1:'피해',2:'「혼란」 부여',3:'「저지」 부여',4:'「도발」 부여',5:'아군 기동상승',6:'아군 공군상승',7:'아군 공성상승',8:'아군 방어상승',9:'아군 파성상승',0xa:'적 기동저하',0xb:'적 공군저하',0xc:'적 공성저하',0xd:'적 방어저하',0xe:'적 파성저하',0xf:'아군 사기상승',0x10:'적 사기저하',0x11:'발화',0x12:'내구피해',0x13:'상태이상해소',0x14:'부상병회복',0x15:'아군 전능력상승',0x16:'적 전능력저하',0x17:'전상태이상부여'}
def make(k):
    o=n1+(k-1)*W;name=raw[o:o+48].decode('utf-16-le').split('\0')[0];f=raw[o+48:o+W];p=f[20:]
    if not f[0]:return name,None
    s='계통: %s / 의존능력: %s / 대거점: %s'%(KIND[f[0]],ST[p[20]],'○' if f[3]&1 else '×')
    s+='\\n효과1: '+EF[p[12]]
    if p[13]:s+='\\n효과2: '+EF[p[13]]
    return name,s
desc={}
for l in open('/mnt/user-data/uploads/Romance_of_the_Three_Kingdoms_14/SAN14_전법설명.txt',encoding='utf-8-sig'):
    if '=' in l and not l.startswith('#'):k,v=l.rstrip('\r\n').split('=',1);desc[k]=v
ok=bad=0;gen={}
for k in range(1,184):
    n,s=make(k)
    if s is None:continue
    gen.setdefault(n,(k,s))
    if n in desc:
        if desc[n]==s:ok+=1
        else:bad+=1;print('DIFF',k,n,'\n  file:',desc[n],'\n  gen :',s)
print('ok',ok,'bad',bad)
import json;json.dump(gen,open('gen.json','w'),ensure_ascii=False)
