# R43 2-3: 바이너리 모듈의 '자료 구역' 안 옛 게임 주소 번역 (apply_new.py 는 코드 즉시값과 IC 훅표만 바꿨음)
#  발견 계기: IC .rdata 0xc5dd0(lea 자리·모델표 짝)·0xc75e8(자리+기대 24바이트 guard 표)가 옛 1.0.1 값 그대로였음.
#  1.0.0→1.0.1 때 쓴 src42d/port/binport.py 의 규칙을 따른다:
#   ① 자료 구역 4바이트 값이 '그럴듯한 옛 RVA' 면 번역   ② 8바이트 절대 주소(0x140000000+RVA) 번역
#   ③ 지문: 번역한 코드 RVA 의 옛 exe 바이트 열(≥6)이 자료 구역에 있으면 새 exe 바이트로(guard 표의 기대 바이트)
#  R43 보강: (가) payload_cur(1.0.1 원본)와 같은 바이트일 때만 건드림(이미 바꾼 자리 이중 번역 방지)
#            (나) 0x10000 미만 작은 값은 근거(절대주소·뒤 바이트 일치·정렬 군집)가 있을 때만 (작은 수·크기와 헷갈림)
#            (다) 지문 교체는 옛/새 바이트가 명령 단위 마스크로 같을 때만(함수가 바뀌었으면 손으로)
#  사용: python3 apply_data_new.py <payload_cur> <payload_new> [--dry]  → payload_new 의 DLL 을 제자리에서 고치고 자료_적용_보고.txt 를 남김
import sys,os,struct,bisect,collections,hashlib;sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)));import newmap as X
CUR,NEW=sys.argv[1:3];DRY='--dry' in sys.argv
OURS={'SAN14_IntegratedConvenience.dll','SAN14_ModSettings.dll','SAN14_ICGuard.dll','SAN14_HiredFace.dll','SAN14_RegionOfficer.dll','SAN14_Range.dll','SAN14_StatusColor.dll','SAN14_SceneSkip.dll',
      'SAN14_ExecuteLock.dll','SAN14_ItemPicture.dll','SAN14_Traits9Check.dll','SAN14_Notice.dll','SAN14_Patch27.dll','SAN14_DomesticTalent.dll',
      'SAN14_AutoPersonnelRepair.dll','SAN14_PrisonerReturn.dll','SAN14_SettingsHelpScenic.dll','SAN14_DomesticAI.dll',
      'SAN14_DomesticPlayer.dll','SAN14_AutoMerit.dll','SAN14_Diag.dll','SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll'}  # 13번은 mt 소스 재빌드
SKIPPRE=('python','libcrypto','libffi','vcruntime','dinput8','dxgi','version')
OINS=set(X._akeys('old'));IB=X.IB
def secs(d):
    pe=struct.unpack_from('<I',d,0x3c)[0];ns=struct.unpack_from('<H',d,pe+6)[0];opt=struct.unpack_from('<H',d,pe+20)[0]
    return [(d[pe+24+opt+40*i:pe+24+opt+40*i+8].rstrip(b'\0').decode(errors='replace'),)+struct.unpack_from('<IIII',d,pe+24+opt+40*i+8)+(struct.unpack_from('<I',d,pe+24+opt+40*i+36)[0],) for i in range(ns)]
_mc={}
def tr(v):
    if v not in _mc:_mc[v]=X.map_code(v) if v<0x1340000 else X.map_data(v)
    return _mc[v]
rep=[];hand=[];changed={}
def R(*a):rep.append('\t'.join(str(x) for x in a))
def H(*a):hand.append('\t'.join(str(x) for x in a))
for root,_,fs in os.walk(NEW):
    for f in sorted(fs):
        if not f.lower().endswith('.dll') or f in OURS or f.startswith(SKIPPRE):continue
        pn=os.path.join(root,f);rel=os.path.relpath(pn,NEW);pc=os.path.join(CUR,rel)
        if not os.path.exists(pc):H(rel,'payload_cur 에 없음(비교 불가) — 건너뜀');continue
        b=bytearray(open(pn,'rb').read());c=open(pc,'rb').read();S=secs(bytes(b));n0=len(rep)
        if len(b)!=len(c):H(rel,'크기가 payload_cur 와 다름 — 건너뜀');continue
        refs=set()
        for name,vs,va,rs,ra,ch in S:
            if rs==0 or (ch&0x20000000) or (ch&0x02000000) or name in('.pdata','.reloc','.idata','.edata','.xdata','.rsrc','.buildid'):continue   # 코드·버릴 수 있는(디버그) 구역 제외
            seg=bytes(b[ra:ra+rs]);cur=c[ra:ra+rs]
            # 후보 모으기
            cands={}
            for k in range(0,rs-3):
                v=struct.unpack_from('<I',seg,k)[0]
                if v<0x1000 or v>=0x2a00000 or seg[k:k+4]!=cur[k:k+4]:continue
                abs8=k+8<=rs and struct.unpack_from('<I',seg,k+4)[0]==1 and seg[k+4:k+8]==cur[k+4:k+8]
                if v<0x1340000:
                    if v not in OINS:continue
                else:
                    if not(v in X.SITE or v in X.DATA_MANUAL):
                        # 알려진 자료 자리 ±0x400 안의 8배수(vtable 칸)만
                        i=bisect.bisect_right(X._dx,v)-1
                        if not(i>=0 and (v-X._dx[i])%8==0 and v-X._dx[i]<0x400):continue
                tail=seg[k+4:k+4+32];ob=X.ord_(v,32);m=0
                while m<len(tail) and m<len(ob) and tail[m]==ob[m]:m+=1
                cands[k]=(v,abs8,m)
            ks=sorted(cands)
            for k in ks:
                v,abs8,m=cands[k]
                # R43: 근거가 강한 것만 — guard 표(뒤 ≥6바이트가 옛 exe), 절대 포인터(함수 시작·알려진 자리), 자리_대응에 있는 값(4정렬)
                strong=(m>=6) or (abs8 and (v in X.ostart or v in X.SITE)) or (k%4==0 and v in X.SITE and v>=0x10000)
                if not strong:continue
                n,how=tr(v)
                if n is None:H(rel,name,hex(va+k),hex(v),'번역 못함: '+how,'abs8' if abs8 else '',f'뒤일치{m}');continue
                refs.add(v)
                if n==v:continue
                # R43 교정: 여기 오는 v 는 항상 0x2a00000 미만의 '맨 RVA' 라서 {u32 rva, u32 1} 짝일 뿐 절대 주소가 아니다.
                #  전에는 abs8 이면 u64(0x140000000+n) 로 써서 Traits9_UI·TraitHover 70칸이 깨졌다(u32 가 0x405b6e10 따위가 됨) → 늘 u32 만 쓴다.
                #  진짜 절대 포인터(u64 0x140000000+RVA)·기록 칸은 record_fields_r43.py 가 맡는다.
                if not DRY:struct.pack_into('<I',b,ra+k,n)
                R(rel,name,hex(va+k),'RVA(뒤 u32=1)' if abs8 else 'RVA',hex(v),'→',hex(n),how,f'뒤일치{m}')
        # ③ 지문(기대 바이트 열)
        for v in sorted(refs):
            if v>=0x1340000:continue
            n,_=tr(v);ob=X.ord_(v,64);nb=X.nrd(n,64)
            if not ob or not nb:continue
            for L in range(min(len(ob),len(nb)),5,-1):
                chunk=ob[:L]
                if chunk==nb[:L]:break
                # 자료 구역에서만 찾기
                hits=[]
                for name,vs,va,rs,ra,ch in S:
                    if rs==0 or (ch&0x20000000) or (ch&0x02000000) or name in('.pdata','.reloc','.idata','.edata','.xdata','.rsrc','.buildid'):continue
                    i=bytes(b[ra:ra+rs]).find(chunk)
                    while i>=0:
                        if c[ra+i:ra+i+L]==chunk:hits.append((name,va+i,ra+i))
                        i=bytes(b[ra:ra+rs]).find(chunk,i+1)
                if not hits:continue
                if L<12 and len(hits)>1:H(rel,'지문',hex(v),f'길이 {L} 가 {len(hits)}곳 — 너무 짧아 안 바꿈');break
                if X.imask('old',v,L)!=X.imask('new',n,L):H(rel,'지문',hex(v),'→',hex(n),f'길이 {L} 옛/새 명령이 다름(바뀐 함수?) — 손으로',' '.join(hex(h[1]) for h in hits));break
                for name,mr,fo in hits:
                    if not DRY:b[fo:fo+L]=nb[:L]
                    R(rel,name,hex(mr),'지문',hex(v),'→',hex(n),f'길이 {L}')
                break
        if len(rep)>n0 and not DRY:
            open(pn,'wb').write(b);changed[rel]=(hashlib.sha256(c).hexdigest(),hashlib.sha256(bytes(b)).hexdigest())
out=['[바꾼 곳]']+rep+['','[손으로 볼 곳]']+hand+['','[해시가 바뀐 DLL]']+[f'{k}\t{v[0]}\t→\t{v[1]}' for k,v in changed.items()]
open(os.path.join(NEW,'자료_적용_보고.txt'),'w',encoding='utf-8').write('\n'.join(out)+'\n')
cnt=collections.Counter(r.split('\t')[0] for r in rep);print('바꾼 곳',len(rep),dict(cnt));print('손으로',len(hand));print('\n'.join(hand[:60]))
