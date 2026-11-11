"""Bit-for-bit R14/R15 compiled distribution comparison and AI cache invalidation.
No probabilistic reimplementation; calls both original and updated Win64 code.
"""
from test_rc4 import *
import re,time

class Cache(C.Structure):
 _fields_=[('target',Target),('base_traits',C.c_uint16*9),('installed',C.c_uint8*355),('passion',C.c_uint8),('anomaly',C.c_uint8),('transcend',C.c_uint8),('valid',C.c_uint8),('budget',C.c_double),('heat',C.c_double),('distribution',Distribution),('rules_revision',C.c_uint32)]

def all_targets():
 pattern=r'^\{(\d+),(\d+),(\d+),\{([^}]+)\},(\d+),(\d+),\{([^}]+)\},"([^"]+)"\},?$'
 found=[]
 for line in (ROOT/'source/official_db.h').read_text().splitlines():
  m=re.match(pattern,line)
  if m:
   id,fa,mo,stats,star,role,traits,name=m.groups()
   found.append(Target(int(id),(C.c_uint16*9)(*map(int,traits.split(','))),(C.c_uint8*5)(*map(int,stats.split(','))),int(star),int(fa),int(mo),int(role)))
 assert len(found)==1088
 return found

def comparisons():
 e=E();old=Loaded(ROOT/'audit/r14_release_baseline/SAN14_MeritTraits.dll',BR);e.mem.append(old)
 first=Distribution();second=Distribution();st=State();count=0
 e.fn('g_prepare');start=time.perf_counter()
 for t in all_targets():
  e.fn('g_init',C.addressof(st),C.addressof(t.traits))
  for a,tr in ((0,0),(1,0),(1,1)):
   args=(C.addressof(t),C.addressof(st),C.addressof(COLORS),a,tr)
   before=BR.call(old.sym('g_distribution'),*args,C.addressof(first))
   after=e.fn('g_distribution',*args,C.addressof(second))
   assert before==after and bytes(first)==bytes(second),(t.id,a,tr,before,after)
   count+=1
 check(count==3264,'all 1,088 DB targets: normal/anomaly/transcend distributions exactly match shipped R14 bytes',{'distributions':count,'seconds':round(time.perf_counter()-start,3)})
 cache=Cache();t=all_targets()[951];e.fn('g_init',C.addressof(st),C.addressof(t.traits));colors=(C.c_uint8*355).from_buffer_copy(COLORS)
 def compare(a=0,tr=0):
  args=(C.addressof(t),C.addressof(st),C.addressof(colors),a,tr)
  expected=BR.call(old.sym('g_distribution'),*args,C.addressof(first))
  got=e.fn('g_distribution_cached',*args,C.addressof(second),C.addressof(cache))
  assert expected==got and bytes(first)==bytes(second)
 def hits():return get(e.mod.sym('g_calc_counters')+8)
 compare();n=hits();compare();check(hits()==n+1,'unchanged complete input reuses the exact distribution')
 # Each dependency can change at a new award, edited world, or rare RNG outcome.
 changes=[lambda:setattr(t,'id',5001),lambda:t.stats.__setitem__(0,1),lambda:t.traits.__setitem__(0,17),lambda:setattr(t,'star',1-t.star),lambda:setattr(t,'father',619),lambda:setattr(t,'mother',952),lambda:st.base_traits.__setitem__(0,99),lambda:setattr(st,'budget',0.38),lambda:setattr(st,'heat',0.48),lambda:setattr(st,'passion',1),lambda:colors.__setitem__(4,0)]
 for change in changes:
  n=hits();change();compare();assert hits()==n
 check(True,'changes to ID/stats/traits/star/parents/base traits/budget/heat/passion/installed colors invalidate AI cache')
 for a,tr in ((1,0),(1,1),(0,0)):
  n=hits();compare(a,tr);assert hits()==n
 check(True,'anomaly and transcend outcomes recompute their original probabilities')
 n=hits();st.rerolls_used=4;st.slots[0].rerolls=4;st.slots[0].seed=123;compare();check(hits()==n+1,'ticket/seed changes alone do not rerun unchanged probability calculation')
 st.initialized=0;cache.valid=1
 result=e.fn('g_distribution_cached',C.addressof(t),C.addressof(st),C.addressof(colors),0,0,C.addressof(second),C.addressof(cache))
 check(result&0xffffffff==0xffffffff,'cache never bypasses uninitialized-state rejection')
 check(get(e.mod.sym('g_calc_counters')+16)==1,'immutable neighbour ranking built only once for all tested targets')
 e.closeall()

if __name__=='__main__':
 try:comparisons()
 finally:(ROOT/'audit/r15_gacha_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R15 GACHA',len(REPORT))
