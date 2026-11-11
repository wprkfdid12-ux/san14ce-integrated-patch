# P204(2026-11-09 사용자 화면): 모드 설정 창 '사운드' 탭에 다른 탭 항목과 빈 슬라이더 줄이 나옴.
#  원인: 탭별 줄 수(layout+0x17c)가 게임 기본값(9,8,7,8,10,19)으로 돌아가 있으면 우리 줄 표가 그 숫자로 잘림.
#  게임 목록 만들기(0x658df0, 디스어셈블로 확인): 탭 t 의 줄 = 표[앞 탭들 줄 수 합 … +줄 수[t]).
# 이 시험: ① 소스의 줄 표(rows[])를 읽어 채우기 규칙대로 표·줄 수를 만들고, 게임 목록 만들기를 흉내 내 탭마다 그 탭 항목만·빈 줄 없이 나오는지
#          ② 게임 기본 줄 수로는 실제로 사용자 화면처럼 깨지는지(시험이 그 버그를 잡는지) ③ 소스 규칙(줄 수·표를 매 Update 확인, 아래 글 판 이름)
#  실행: python3 test/test_settings_tabs.py
import re,sys,random,os
R=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
s=open(R+'/src/mod_settings.cpp',encoding='utf-8').read()
body=s[s.index('static const Row rows[]={'):s.index('static const unsigned NROWS')]
rows=[]
for l in body.splitlines():
    m=re.match(r'\s*(?:TG\((\d),|\{(\d),(K_\w+),)\s*\w+\s*,\s*\d+\s*,\s*L"([^"]+)"',l)
    if m:rows.append((int(m.group(1) or m.group(2)),m.group(3) or 'K_TOGGLE',m.group(4)))
fails=0
def ck(ok,msg):
    global fails
    print(('  ok   ' if ok else '  FAIL ')+msg);fails+=0 if ok else 1
TAB=['편의','밸런스·규칙','정보·UI','그래픽·화면','무장·얼굴','사운드']
nsrc=len([l for l in body.splitlines() if re.match(r'\s*(TG\(|\{\d,)',l)])
ck(len(rows)==nsrc and nsrc>=50,'줄 표 읽음: %d줄(소스 %d줄)'%(len(rows),nsrc))
ck(len(rows)+6<=61,'줄 수 %d + 빈 탭 안내 최대 6 ≤ 게임 표 61칸'%len(rows))
snd=[r[2] for r in rows if r[0]==5]
ck(snd==['BGM 플레이어'],'사운드 탭 = BGM 플레이어만 (지금: %s)'%snd)
MUST={'투석 사거리(칸)':1,'안행·누선 사거리(칸)':1,'항장·부상 색 표시':2,'지역담당관 공석 경고':2,'독립 미니맵(안개 없이)':3,'고용무장 원본 얼굴':4,
      '관직 없는 무장 병력 수':1,'AI 명령서 99장 유지':1,'최소 인구 1만 유지':1,'저돌 수정':1,'인사 말풍선 설명':2,'전사 일러스트·메시지·기록':2,'포로 초상(포박 일러스트)':4,
      '모든 내정 자동':0,'카메라 자동 이동':0,'개성·전법 마우스 설명':2}
for name,t in MUST.items():
    got=[r[0] for r in rows if r[2]==name]
    ck(got==[t],'%s → %s 탭'%(name,TAB[t]))
def fill(avail):
    """mod_settings.cpp fill_table 과 같은 규칙: 탭 순서로 쓸 수 있는 줄을 넣고, 빈 탭은 안내 줄 1개."""
    table=[];counts=[]
    for t in range(6):
        c=0
        for i,r in enumerate(rows):
            if r[0]==t and avail(i) and len(table)<61:table.append(i);c+=1
        if not c and len(table)<61:table.append(-1);c=1
        counts.append(c)
    return table,counts
def game_list(table,counts,t):
    """게임 0x658df0: 시작 = 앞 탭 줄 수 합, 개수 = 줄 수[t]. 표 끝을 넘으면 빈 칸(0) = 이름 없는 슬라이더 줄."""
    st=sum(counts[:t]);return [table[k] if k<len(table) else None for k in range(st,st+counts[t])]
def good(table,counts):
    for t in range(6):
        for e in game_list(table,counts,t):
            if e is None:return '탭 %s 에 빈 줄'%TAB[t]
            if e>=0 and rows[e][0]!=t:return '탭 %s 에 다른 탭 항목 "%s"'%(TAB[t],rows[e][2])
    return ''
random.seed(14);cases=[('전부 설치',lambda i:True),('아무것도 없음',lambda i:False)]
for k in range(200):
    keep={i for i in range(len(rows)) if random.random()<random.choice((0.2,0.5,0.9))};cases.append(('무작위 %d'%k,lambda i,keep=keep:i in keep))
bad=[(n,good(*fill(a))) for n,a in cases if good(*fill(a))]
ck(not bad,'설치 조합 %d가지: 탭마다 그 탭 항목만, 빈 줄 없음%s'%(len(cases),'' if not bad else ' — '+str(bad[0])))
# ② 버그 재현: 줄 수가 게임 기본값이면 깨져야 한다(이 시험이 그 화면을 잡는다는 확인)
table,counts=fill(lambda i:True);native=[9,8,7,8,10,19]
last=game_list(table,native,5);names=[rows[e][2] if e is not None and e>=0 else '(빈 줄)' for e in last]
ck(good(table,native)!='' and names.count('(빈 줄)')>=8 and 'BGM 플레이어' in names and len(names)==19,
   '게임 기본 줄 수(9,8,7,8,10,19)면 사용자 화면처럼 깨짐(이 시험이 그 버그를 잡음): 사운드 탭 = %s … 빈 줄 %d개'%(', '.join(n for n in names if n!='(빈 줄)')[:60],names.count('(빈 줄)')))
# ③ 소스 규칙
ck('static void fill_table(U8*layout)' in s and 'at<int>(layout,0x17c+4*tab)=(int)cnt' in s,'fill_table 이 탭별 줄 수(layout+0x17c)를 씀')
ck(re.search(r'state==g_ourState\)\{keep_layout\(state\);reassert\(state\);\}',s) is not None,'우리 창이 떠 있는 동안 매 Update 에 줄 수·표 확인(keep_layout)')
ck('fix_this_open>=30' in s and 'tab layout restored' in s,'되돌림은 원인 기록을 남기고 30번에서 그만둠')
ck('통합패치 R39' not in s and 'S14_PATCH_VERW L" 모드 설정)"' in s,'아래 안내 글의 판 이름은 version.h 에서')
ck('plan.jump(0x658dfa,ec,9,(void*)&MsCntBridge)' in s and 'MsCnt[tab]=(int)cnt' in s,'목록 만들기 안(0x658dfa)에서 우리 줄 수를 다시 씀 — 기색 모드가 되돌려도(P204 확정 원인)')
print('전부 통과' if not fails else '실패 %d'%fails);sys.exit(1 if fails else 0)
