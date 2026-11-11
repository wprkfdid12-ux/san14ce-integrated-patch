"""Production Win64 PE keyboard skip timing under synthetic input/time."""
from r2_fixture import *
for rare in [False,True]:
 e=R2();o,r,s=e.prepare(top=1);e.click(r,0);x=e.gsui(596).slots[0];x.anomaly=int(rare);x.transcend=0
 if not rare:x.result=4
 e.tick+=100;e.keys={13};e.fn('hook_state_tick',s.addr)
 check(e.mod.val('ui_mode')==(2 if rare else 3),'keyboard skip ordinary/high-tier distinction '+str(rare))
 if rare:
  e.keys=set();e.fn('hook_state_tick',s.addr);e.tick+=500;e.keys={13};e.fn('hook_state_tick',s.addr);check(e.mod.val('ui_mode')==3,'high-tier skip available at 600ms')
 e.closeall()
(ROOT/'audit/r2_skip_tests.json').write_text(json.dumps({'scope':'Win64 PE under simulated OS/game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
