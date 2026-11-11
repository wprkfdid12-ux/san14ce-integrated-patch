# 통합패치가 아닌 우리 다른 모드(기색 등)가 게임에서 바꾸는 자리를 게임 기록(ModCore runtime.log)에서 뽑는다 → 자료/다른모드자리.tsv
#   python3 교훈_점검/도구/다른모드자리_만들기.py <runtime.log>
# 교훈 L123: 통합패치 모드 설정 창이 깨진 원인이 기색 모드의 후크(0x658df0)였는데, 그 줄이 기록에 처음부터 있었다. 관문 c11 이 이 표로 대조한다.
# 한계: ModCore 를 거치는 모드만 기록에 남는다(기색, 계절 SeasonRules — 11-11 부터). 2D·맵 확장·계절 SeasonSync(그래픽)는 자기 로더라 여기 안 나옴.
# AI 모드(행동교정) 자리는 소스에서 뽑아 자료/AI모드자리.tsv 로 따로(점검 c14).
import re,sys,os
HERE=os.path.dirname(os.path.abspath(__file__));LESS=os.path.dirname(HERE);sys.path.insert(0,HERE)
import 자리표_만들기 as ST
MODS=[('SAN14CE.Condition','기색','SAN14_Condition.dll'),('SeasonRules.','계절','SAN14_SeasonRules.dll')]
def load(path):
    t=open(path,'rb').read().decode('utf-8',errors='replace');parts=re.split(r'(?=SAN14 ModCore 1\.0)',t)
    first=next((i for i,p in enumerate(parts) if ST.NEW_MARK in p),None)
    if first is None:raise SystemExit('새 판 표식이 기록에 없음')
    sites={}
    for p in parts[first:]:
        for m in re.finditer(r'((?:SAN14CE\.|SeasonRules\.)[A-Za-z0-9_.]+) RVA=([0-9a-f]{8}) len=(\d+)',p):
            mod=next(((n,d) for pre,n,d in MODS if m.group(1).startswith(pre)),None)
            if mod:sites[(int(m.group(2),16),int(m.group(3)))]=(mod[0],mod[1],m.group(1))
    return sites
if __name__=='__main__':
    s=load(sys.argv[1]);out=LESS+'/자료/다른모드자리.tsv'
    with open(out,'w',encoding='utf-8') as f:
        f.write('# 우리 다른 모드가 고치는 게임 자리(게임 기록에서). 만든 도구: 교훈_점검/도구/다른모드자리_만들기.py — 그 모드가 새 판을 내면 다시 만들 것\n게임RVA\t길이\t모드\tDLL\t기록이름\n')
        for (r,l),(mod,dll,nm) in sorted(s.items()):f.write('0x%x\t%d\t%s\t%s\t%s\n'%(r,l,mod,dll,nm))
    print(len(s),'자리 →',out)
