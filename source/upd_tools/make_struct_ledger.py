# 게임 구조값 대장: 데이터 루트(db = *(exe+0x277df18)) 안 표 위치 + 객체 칸 오프셋.
#  각 값마다 옛 판에서 그 값을 쓰는 명령(증인) 주소를 모아 둠 → find_new.py 가 새 판 같은 명령의 오프셋을 읽어 '그대로/바뀜(새 값)' 판정.
#  사용: python3 make_struct_ledger.py <game.asm(옛 판 objdump)> > 구조값_대장.tsv
import re,sys,collections
ASM=sys.argv[1]
DB={0x158:'무장 표(id*8, ≤0x1770)',0xdf30:'세력 표(id*8, ≤0x33)',0xe270:'칸·거점 객체 표(id*8, ≤0xbd0f)',0x6caf0:'지역 표(0x98 간격, 1..500)',
    0x871e8:'world(연 +0x34·월 +0x36·플레이어 세력 +0x3a)',0x861a0:'고용 풀',0x73a50:'무장 표 기본칸(범위 밖일 때)',0x7e838:'부대 표(0x200 간격)',
    0x77428:'진형 표(0xc8 간격)',0xe0d0:'군단 표',0xdc80:'주(州) 표',0x78490:'작위 표',0x784f8:'관직 표',0x85f20:'개성 표',0x73958:'?(0x13 간격 객체 기준)',0x77b18:'전법 표',0x90:'날짜 항목'}
# 객체 칸: (이름, 오프셋, 증인 함수들(옛 판 RVA) — 그 함수 안에서 이 오프셋을 쓰는 명령을 증인으로)
FIELDS=[('무장 물자 금',0x170,[0x19a80,0xb1aa0]),('무장 물자 군량',0x172,[0x19a80,0xb1aa0]),('무장 물자 병사',0x174,[0x19a80,0xb1aa0]),
        ('무장 내면 호전',0x16a,[0x19a80]),('무장 내면 야망',0x17c,[0x19a80]),('무장 내면 건설',0x180,[0x344d10]),('무장 기본 능력치(+idx)',0xec,[0x22fce0]),
        ('무장 소속 세력',0xe0,[0x22b980]),('무장 부상 상태',0x161,[0x22fce0]),
        ('지역 담당관 id',0x36,[0xb1aa0,0x36320]),('지역 중심 칸',0x3a,[0x22c7c0,0x22c810]),('지역 상업',0x3c,[0x22e2b0]),('지역 농업',0x3e,[0x22c760]),('지역 병영',0x40,[0x231170]),('지역 개발 중점',0x72,[0xb1aa0,0x36320]),
        ('세력 군주 id',0x10,[]),('칸 소유 세력',0x14,[0x22c810])]
ins={};order=[]
pat=re.compile(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*(\S+)\s*(.*)$')
for line in open(ASM,errors='replace'):
    m=pat.match(line)
    if not m:continue
    a=int(m.group(1),16)-0x140000000;ins[a]=(m.group(2),m.group(3));order.append(a)
print('종류\t이름\t오프셋\t증인 명령(옛 판 RVA: 주증인|보조증인)\t증인 수')
# db 표: rip 로 0x14277df18 을 읽은 레지스터를 6줄 안에서 [reg+OFF]/[reg+idx*8+OFF] 로 쓰는 곳
wit=collections.defaultdict(list)
for i,a in enumerate(order):
    mn,ops=ins[a]
    if '# 0x14277df18' in ops and mn=='mov':
        r=re.match(r'(r[a-z0-9]+),',ops)
        if not r:continue
        reg=r.group(1)
        for b in order[i+1:i+7]:
            m2=re.search(r'\['+reg+r'(?:\+r[a-z0-9]+\*8)?\+0x([0-9a-f]+)\]',ins[b][1])
            if m2:
                v=int(m2.group(1),16)
                if v in DB:wit[v].append(b)
                break
for v,nm in DB.items():
    w=wit.get(v,[]);print(f'db표\t{nm}\t0x{v:x}\t'+','.join(f'0x{x:x}' for x in w[:12])+f'\t{len(w)}')
# 표에서 꺼낸 객체의 칸: [..+표오프셋] 으로 읽은 레지스터를 6줄 안에서 [reg+칸] 으로 쓰는 곳 (함수 지정 없이 찾음)
CHAIN={'세력 군주 id':(0xdf30,0x10),'무장 물자 금':(0x158,0x170),'무장 물자 군량':(0x158,0x172),'무장 물자 병사':(0x158,0x174),
       '무장 내면 호전':(0x158,0x16a),'무장 내면 야망':(0x158,0x17c),'무장 내면 건설':(0x158,0x180),'무장 소속 세력':(0x158,0xe0),'무장 부상 상태':(0x158,0x161)}
chainw=collections.defaultdict(list)
for i,a in enumerate(order):
    mn,ops=ins[a]
    for nm,(tb,fo) in CHAIN.items():
        m=re.match(r'(r[a-z0-9]+),QWORD PTR \[[^\]]*\+0x%x\]$'%tb,ops) if mn=='mov' else None
        if not m:continue
        reg=m.group(1)
        for b in order[i+1:i+7]:
            o2=ins[b][1]
            if re.search(r'\['+reg+r'\+0x%x\]'%fo,o2):chainw[nm].append(b);break
            if o2.startswith(reg+',') or ins[b][0] in('call','ret','jmp'):break
# 함께 쓰이는 칸 묶음: 30줄 창 안에 묶음의 칸이 모두 나오면 그 명령들을 증인으로 (같은 구조를 다루는 곳일 확률 높음)
GROUPS=[(('무장 물자 금',0x170),('무장 물자 군량',0x172),('무장 물자 병사',0x174)),
        (('지역 상업',0x3c),('지역 농업',0x3e),('지역 병영',0x40)),
        (('무장 내면 호전',0x16a),('무장 내면 야망',0x17c)),
        (('무장 소속 세력',0xe0),('무장 기본 능력치(+idx)',0xec)),
        (('무장 부상 상태',0x161),('무장 기본 능력치(+idx)',0xec)),
        (('지역 담당관 id',0x36),('지역 개발 중점',0x72)),
        (('지역 중심 칸',0x3a),('칸 소유 세력',0x14))]
grpw=collections.defaultdict(list)
fre=re.compile(r'\[r[a-z0-9]+\+0x([0-9a-f]+)\]')
offat=[]
for a in order:
    m=fre.search(ins[a][1]);offat.append(int(m.group(1),16) if m else -1)
for g in GROUPS:
    want={v for _,v in g};name={v:n for n,v in g};last=-999
    for i in range(len(order)):
        if offat[i] not in want or i-last<30:continue
        win=offat[i:i+30]
        if all(v in win for v in want):
            for j in range(i,i+30):
                v=offat[j]
                if v in want and len(grpw[name[v]])<12 and order[j] not in grpw[name[v]]:grpw[name[v]].append(order[j])
            last=i
# 객체 칸
import bisect
for nm,v,fs in FIELDS:
    w=[]
    for f in fs:
        i=bisect.bisect_left(order,f)
        for b in order[i:i+400]:
            if f!=b and ins[b][0]=='int3' and ins[order[order.index(b)-1]][0]=='ret':break
            if re.search(r'\+0x%x\]'%v,ins[b][1]):w.append(b)
            if len(w)>=6:break
    w+=chainw.get(nm,[])[:12-len(w)]
    g2=[x for x in grpw.get(nm,[]) if x not in w][:12]
    print(f'객체칸\t{nm}\t0x{v:x}\t'+','.join(f'0x{x:x}' for x in w[:12])+'|'+','.join(f'0x{x:x}' for x in g2)+f'\t{len(w[:12])}+보조{len(g2)}')
