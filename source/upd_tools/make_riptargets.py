# riptargets.pkl 만들기: 게임 코드가 rip 상대로 가리키는 자료 주소(RVA) 모음 — make_ledger.py 가 "진짜 게임 자료 주소" 거를 때 씀
#  사용: python3 make_riptargets.py game.asm riptargets.pkl
import re,sys,pickle
s=set()
for l in open(sys.argv[1],errors='replace'):
    m=re.search(r'# (0x14[0-9a-f]{7})',l)
    if m and 'rip' in l:s.add(int(m.group(1),16)-0x140000000)
pickle.dump(s,open(sys.argv[2],'wb'));print(len(s))
