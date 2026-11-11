# R43 시험3: '패치 뒤 모습'을 기대 바이트로 가진 기록 칸 번역 (Traits9_UI 가 Traits9 의 5→9 패치 자리를 09 로 기대 — 옛 exe 와 안 맞아 record_fields 가 못 봄)
#  규칙: 자료 구역에서 {u32 rva, u32 len(1..64), bytes[len]} 꼴로, bytes 가 옛 exe 의 rva 자리와 '≤2바이트만 다르게' 일치하고 rva 가 옛 코드/자료 범위이면
#        → payload_new 의 그 rva 칸이 아직 옛 값이면 map_code/map_data 로 번역. 새 exe 의 새 자리 바이트도 같은 위치만 달라야 함(아니면 보고만).
#        같은 옛 rva 가 모듈 안 다른 칸(코드 즉시값 포함)에 남아 있으면 같이 번역(보고).
#  사용: python3 patched_records_r43.py <payload_cur> <payload_new> <목록> [--dry]
import sys,os,re,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
DRY='--dry' in sys.argv;ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
OT=P.OT;NT=P.NT;ova=P.ova;nva=P.nva;OIMG_END=P.OIMG_END
def run(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):return
    S=P.secs(c);name=os.path.basename(NEW);rep=[];changed=0;fixes={}
    for n,vs,va,rs,ra,ch in S:
        if n=='.text':continue
        for k in range(0,rs-12,4):
            rva,ln=struct.unpack_from('<II',c,ra+k)
            if not(1<=ln<=64) or not(0x10000<=rva<OIMG_END) or ra+k+8+ln>ra+rs:continue
            b=c[ra+k+8:ra+k+8+ln]
            if rva<ova or rva-ova+ln>len(OT):continue
            g=OT[rva-ova:rva-ova+ln]
            diff=[i for i in range(ln) if b[i]!=g[i]]
            if not diff or len(diff)>(2 if ln>=8 else 1) or len(diff)>=ln or ln<3:continue
            nrva=struct.unpack_from('<I',d,ra+k)[0]
            if nrva!=rva:continue   # 이미 번역됨
            m,how=X.map_code(rva) if rva<P.OTEXT_END else X.map_data(rva)
            if m is None:rep.append((hex(va+k),hex(rva),ln,'번역 못함',how));continue
            ng=NT[m-nva:m-nva+ln]
            diff2=[i for i in range(ln) if b[i]!=ng[i]]
            if diff2!=diff:rep.append((hex(va+k),hex(rva),'→',hex(m),'새 자리 바이트 다름',diff,diff2,ng.hex()));continue
            fixes[rva]=m;rep.append((hex(va+k),'패치뒤기대',hex(rva),'→',hex(m),ln,'다른 바이트',diff,(how or '')[:40]))
            if not DRY:struct.pack_into('<I',d,ra+k,m)
            changed+=1
    # 같은 옛 rva 가 남은 칸(자료·코드 즉시값)
    for rva,m in fixes.items():
        pat=struct.pack('<I',rva);i=c.find(pat)
        while i>=0:
            if d[i:i+4]==pat:
                if not DRY:d[i:i+4]=struct.pack('<I',m)
                changed+=1;rep.append(('파일',hex(i),'같은 옛값',hex(rva),'→',hex(m)))
            i=c.find(pat,i+1)
    if not DRY and changed:open(NEW,'wb').write(d)
    if rep:
        print('==',name,'바꾼 칸',changed)
        for r in rep:print(' ',*r)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:run(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:
            import traceback;traceback.print_exc()
