# 전수 조사 준비: 옛/새 exe 디스어셈블(대응/*.asm.pkl)에서 '주소 증거' 집합과 '구조 오프셋 증거' 개수를 뽑아 둔다.
#  BR = 분기·호출 대상, INS = 명령 시작, DREF = rip 참조 자료 주소, DISP = [reg+disp] 비-rip 변위 사용 횟수, IMM = 즉시값 사용 횟수
import pickle,re,sys,collections,gc
P='/tmp/claude-0/-home-claude/2a4b46c3-df33-5cc4-8237-3e2f820b3723/scratchpad/newver/대응/'
hexre=re.compile(r'0x[0-9a-f]+')
for tag in ('old','new'):
    d=pickle.load(open(P+tag+'.dec.exe.asm.pkl','rb'))
    INS=set(d.keys());BR=set();DREF=set();DISP=collections.Counter();IMM=collections.Counter()
    for a,(m,ops,tgt) in d.items():
        if tgt:
            t=int(tgt,16)-0x140000000
            if m.startswith('j') or m=='call':BR.add(t)
            else:DREF.add(t)
        elif m.startswith('j') or m=='call':
            mm=re.match(r'0x(14[0-9a-f]{7})$',ops)
            if mm:BR.add(int(mm.group(1),16)-0x140000000)
        if '[' in ops and 'rip' not in ops:
            for x in re.findall(r'[+-](0x[0-9a-f]+)\]',ops):DISP[int(x,16)]+=1
        mm=re.search(r',(0x[0-9a-f]+)$',ops)
        if mm and '[' not in ops.split(',')[-1]:IMM[int(mm.group(1),16)]+=1
    pickle.dump({'INS':INS,'BR':BR,'DREF':DREF,'DISP':DISP,'IMM':IMM},open('/tmp/census/%s_idx.pkl'%tag,'wb'))
    print(tag,len(INS),len(BR),len(DREF),len(DISP),len(IMM));del d,INS;gc.collect()
