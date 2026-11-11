# 계절 모드(SAN14_SeasonRules.dll)가 지금 게임 판에서 고치는 게임 자리 → 자료/계절자리.tsv
#   python3 교훈_점검/도구/계절자리_만들기.py <SAN14_SeasonRules.dll>
# 11-11: 새 판의 SeasonRules 는 ModCore 기록에 RVA 줄을 안 남김(self-host). 그래서 DLL 안 '판별 표'(판 도장 + 함수 주소 14개)에서 지금 판 줄을 읽는다.
#  표 칸 뜻은 롤백판 기록(SAN14CE.SeasonRules.Time/SnowSpeed/FireRate RVA·len 줄)과 같은 칸끼리 맞춰 정함: 2=Time(5) 4=SnowSpeed(7) 11=FireRate(6).
#  길찾기(RouteSearch)·길 막힘 검사(RouteGuard)는 실행 때 찾는 자리라 DLL 안 상수로: 0x183a30(경로 탐색 함수 — AI path_fast 와 같은 함수), 기록 문구의 0x9bbbc.
#  RouteCost(칸 비용 함수 들머리)·TileInfo 는 실행 때 찾고 상수가 없어 이 표에 없음(한계) — 계절 소스 자리표가 생기면 더할 것.
import struct,sys,os
HERE=os.path.dirname(os.path.abspath(__file__));LESS=os.path.dirname(HERE)
STAMP=0x6ad06f53
d=open(sys.argv[1],'rb').read()
p=struct.pack('<I',STAMP);hits=[]
i=d.find(p)
while i>=0:
    if struct.unpack_from('<I',d,i+8)[0]&0xfffff000==0x80044000:hits.append(i)
    i=d.find(p,i+1)
assert len(hits)==1,('판별 표 줄 못 찾음',hits)
rec=struct.unpack_from('<14I',d,hits[0]+16)
rows=[(rec[2],5,'SeasonRules.Time'),(rec[4],7,'SeasonRules.SnowSpeed'),(rec[11],6,'SeasonRules.FireRate')]
if struct.pack('<I',0x183a30) in d:rows.append((0x183a30,16,'SeasonRules.RouteSearch(경로 탐색 함수 들머리 쪽, 길이 짐작)'))
if b'0x9bbbc' in d:rows.append((0x9bbbc,6,'SeasonRules.RouteGuard(빈 경로 목록 읽기, 길이 짐작)'))
out=LESS+'/자료/계절자리.tsv'
with open(out,'w',encoding='utf-8') as f:
    f.write('# 계절 모드(SAN14_SeasonRules.dll)가 지금 판(0x6ad06f53)에서 고치는 게임 자리. 만든 도구: 교훈_점검/도구/계절자리_만들기.py — 계절 모드가 새 판을 내면 다시 만들 것\n')
    f.write('# 한계: RouteCost·TileInfo 자리는 실행 때 찾아서 여기 없음.\n게임RVA\t길이\t모드\tDLL\t기록이름\n')
    for a,l,nm in rows:f.write('0x%x\t%d\t계절\tSAN14_SeasonRules.dll\t%s\n'%(a,l,nm))
for r in rows:print('0x%x'%r[0],r[1],r[2])
print('→',out)
