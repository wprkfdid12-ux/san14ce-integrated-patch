# 우리 모듈이 게임에서 바꾸는 자리 표를 실제 게임 기록(ModCore runtime.log)에서 뽑는다.
#   python3 교훈_점검/도구/자리표_만들기.py <runtime.log> [출력 tsv]
# 교훈 L110: 기록 머리줄의 판 글자("timestamp 6a983c25")는 새 판에서도 옛 값 그대로 찍힌다(이식한 ModCore 가 글자를 안 바꿈).
#   → 그 글자로 새 판 기록을 고르면 옛 판 주소가 섞인다. 새 판에만 있는 모듈 이름이 처음 나온 기록부터를 새 판으로 본다.
import re,sys,os
HERE=os.path.dirname(os.path.abspath(__file__));LESS=os.path.dirname(HERE)
NEW_MARK='SAN14CE.Stats200New'          # 새 판(0x6ad06f53)부터 들어간 모듈. 판이 또 바뀌면 그 판에만 있는 표식으로 바꿀 것(06_기준값.md)
OWN=[('SAN14CE.MeritTraits','13','SAN14_MeritTraits.dll'),('SAN14CE.Patch27','25','SAN14_Patch27.dll'),('SAN14CE.Range','33','SAN14_Range.dll'),
 ('SAN14CE.SceneSkip','4','SAN14_SceneSkip.dll'),('SAN14CE.Stats200New','17','SAN14_Stats200New.dll'),('RelatedOfficersTab','15','SAN14_RelatedOfficersTab.dll'),
 ('SAN14CE.StatusColor','29','SAN14_StatusColor.dll'),('SortiePresets','16','SAN14_SortiePresets.dll'),('SAN14CE.ModSettingsUI','1','SAN14_ModSettings.dll'),
 ('SAN14CE.DragLearn','30','SAN14_DragLearn.dll'),('SAN14CE.DragGive','30','SAN14_DragGive.dll'),('SAN14CE.RegionOfficer','34','SAN14_RegionOfficer.dll'),
 ('SAN14CE.SettingsHelp','4','SAN14_SettingsHelpScenic.dll'),('SAN14CE.Scenery','4','SAN14_SettingsHelpScenic.dll'),('SAN14CE.CityCaps','23','SAN14_CityCaps.dll'),
 ('SAN14CE.FPS120','19','SAN14_FPS120.dll'),('SAN14CE.PrisonerReturn','21','SAN14_PrisonerReturn.dll'),('SAN14CE.HiredFace','32','SAN14_HiredFace.dll'),
 ('SAN14CE.WheelRegion','30','SAN14_WheelRegion.dll'),('SAN14CE.ExecuteLock','26','SAN14_ExecuteLock.dll'),('SAN14CE.DomesticTalent','22','SAN14_DomesticTalent.dll'),
 ('SAN14CE.DomesticAI','22','SAN14_DomesticAI.dll'),('SAN14CE.ItemPicture','30','SAN14_ItemPicture.dll')]
SKIP=('SAN14CE.Condition','SAN14CE.Stats200.','SAN14CE.Stats200Reader')   # 기색 모드(통합패치 아님)·옛 이식본
def load_extra():
    """자료/우리자리_추가.tsv — 이번 판에서 새로 넣은 자리(게임 기록에 아직 없음). (더할 줄들, 뺄 (RVA,길이) 들)"""
    import os
    f=os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),'자료','우리자리_추가.tsv');add=[];drop=set()
    if not os.path.exists(f):return add,drop
    for l in open(f,encoding='utf-8'):
        if l.startswith('#') or l.startswith('게임RVA') or not l.strip():continue
        c=l.rstrip('\n').split('\t');add.append((int(c[0],16),int(c[1]),c[2],c[3],c[4]))
        if len(c)>6 and c[6].strip():r,n=c[6].split(':');drop.add((int(r,16),int(n)))
    return add,drop
def load(path):
    t=open(path,'rb').read().decode('utf-8',errors='replace')
    parts=re.split(r'(?=SAN14 ModCore 1\.0)',t)
    first=next((i for i,p in enumerate(parts) if NEW_MARK in p),None)
    if first is None:raise SystemExit('새 판 표식(%s)이 기록에 없음 — 새 판으로 실행한 적 없는 기록'%NEW_MARK)
    new=parts[first:];sites={};unknown=set()
    for p in new:
        seen=set()
        for m in re.finditer(r'((?:SAN14CE\.|SortiePresets|RelatedOfficersTab)[A-Za-z0-9_.]*) RVA=([0-9a-f]{8}) len=(\d+)',p):
            o=m.group(1)
            if o.startswith(SKIP):continue
            mod=next(((n,d) for pre,n,d in OWN if o.startswith(pre)),None)
            if not mod:unknown.add(o);continue
            k=(int(m.group(2),16),int(m.group(3)))
            if k in seen:continue
            seen.add(k);e=sites.setdefault(k,[o,mod[0],mod[1],0]);e[3]+=1
    return sites,len(new),len(parts),unknown
if __name__=='__main__':
    src=sys.argv[1];out=sys.argv[2] if len(sys.argv)>2 else LESS+'/자료/우리자리.tsv'
    sites,nnew,nall,unknown=load(src)
    with open(out,'w',encoding='utf-8',newline='\n') as f:
        f.write('# 기준: 새 판 표식 %s 가 처음 나온 뒤의 기록 %d회(전체 %d회 중). 만든 도구: 교훈_점검/도구/자리표_만들기.py\n'%(NEW_MARK,nnew,nall))
        f.write('게임RVA\t길이\tDLL\t설치기번호\t기록이름\t나온횟수\n')
        for (r,l),(o,n,d,c) in sorted(sites.items()):f.write('0x%x\t%d\t%s\t%s\t%s\t%d\n'%(r,l,d,n,o,c))
    print('자리 %d곳, 새 판 기록 %d회'%(len(sites),nnew))
    if unknown:print('주인 모르는 기록 이름(OWN 표에 넣을 것):',sorted(unknown));sys.exit(1)
