# R43 시험4: 한 모듈 안에서 '같은 옛 주소'가 어떤 칸에서는 번역되고 다른 칸에서는 안 된 경우(Traits9_UI 코드굴 목록: 기록표는 번역, 빠른 목록은 그대로 → 새 exe 살아 있는 코드에 덮어써 논공 화면 튕김).
#  규칙: payload_cur→payload_new 에서 바뀐 u32 칸(옛값 ≥0x10000, 옛 exe 범위)으로 모듈별 옛→새 표(한 옛값에 새값 하나일 때만)를 만들고,
#        아직 옛값 그대로인 4바이트 정렬/비정렬 칸(자료 구역, 그리고 코드의 즉시값·변위)을 찾아 보고/번역.
#  사용: python3 same_value_r43.py <payload_cur> <payload_new> <목록> [--dry]
import sys,os,struct,re
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
_a=sys.argv;sys.argv=['x','a','b','c']
import imm_port_r43 as P, newmap as X
sys.argv=_a
DRY='--dry' in sys.argv;ARGS=[a for a in sys.argv[1:] if not a.startswith('--')]
def run(CUR,NEW):
    c=open(CUR,'rb').read();d=bytearray(open(NEW,'rb').read())
    if len(c)!=len(d):return
    S=P.secs(c);pe=struct.unpack_from('<I',c,0x3c)[0];IMG=struct.unpack_from('<I',c,pe+24+0x38)[0]
    tbl={};bad=set()
    for n,vs,va,rs,ra,ch in S:
        for k in range(0,rs-3):
            ov=struct.unpack_from('<I',c,ra+k)[0]
            if not(0x10000<=ov<P.OIMG_END):continue
            nv=struct.unpack_from('<I',d,ra+k)[0]
            if ov==nv:continue
            if not(0x10000<=nv<0x2a02000):continue
            if ov in tbl and tbl[ov]!=nv:bad.add(ov)
            tbl.setdefault(ov,nv)
    for b in bad:tbl.pop(b,None)
    # newmap 과 일치하는 번역만(지문 조각 안 바이트 우연 일치 제외)
    ok={}
    for ov,nv in tbl.items():
        m=(X.map_code(ov)[0] if ov<P.OTEXT_END else X.map_data(ov)[0])
        if m==nv:ok[ov]=nv
    tbl=ok
    # 모듈 자기 이미지 범위 안 값은 위험(IC 내부 주소) → 코드 즉시값으로 쓰였을 때만, 자료 칸은 보고만
    ins=P.disasm(CUR);imm_pos={}
    for addr,bs,mn,ops in ins:
        if len(bs)>=5:
            for p in range(1,len(bs)-3):
                v=struct.unpack_from('<I',bs,p)[0]
                if v in tbl:imm_pos.setdefault(addr-P.IB+p,v)
    rep=[];changed=0
    for n,vs,va,rs,ra,ch in S:
        if n=='.text' or (ch&0x20000000):continue   # 코드는 우연 일치가 많아(nop 바이트 등) 자료 구역만
        for k in range(0,rs-3,4):
            ov=struct.unpack_from('<I',c,ra+k)[0]
            if ov not in tbl:continue
            if struct.unpack_from('<I',d,ra+k)[0]!=ov:continue
            rva=va+k;incode=(n=='.text')
            if incode and rva not in imm_pos:continue   # 명령 안 즉시값/변위 위치가 아니면(우연 바이트) 건너뜀
            if ov<IMG and not incode:rep.append((hex(rva),n,'자료칸 옛값 그대로(모듈 범위 안이라 보고만)',hex(ov),'→',hex(tbl[ov])));continue
            if not DRY:struct.pack_into('<I',d,ra+k,tbl[ov])
            changed+=1;rep.append((hex(rva),n,'옛값 그대로 → 같은 모듈 번역대로',hex(ov),'→',hex(tbl[ov])))
    if not DRY and changed:open(NEW,'wb').write(d)
    if rep:
        print('==',os.path.basename(NEW),'표',len(tbl),'바꾼 칸',changed)
        for r in rep:print(' ',*r)
if __name__=='__main__':
    for rel in open(ARGS[2],encoding='utf-8').read().split():
        try:run(os.path.join(ARGS[0],rel),os.path.join(ARGS[1],rel))
        except Exception as e:
            import traceback;traceback.print_exc()
