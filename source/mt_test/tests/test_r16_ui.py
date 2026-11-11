"""Controlled Zhang Fei reroll/physical-label regression, not the reporter's save."""
from test_r3 import *
from test_r6 import R6

def zhang_fei():
 e=R6();original=tuple(PEOPLE[655]['trait_slots']);o,row,state=e.prepare(id=656,owned=original,capacity=1)
 e.click(row,0);e.reveal(state)
 for n in range(5):
  e.seed(1001+n);e.action(0);e.reveal(state);e.reveal(state)
  if e.gsui(656).slots[0].outcome==1:e.notice(state);e.dismiss(state)
  assert e.gsui(656).slots[0].result!=62 and e.owned(o)==original
 check(e.gs(656).rerolls_used==5,'Zhang Fei: five actual UI rerolls preserve incumbent traits; owned Single Combat never drawn')
 e.accept_preview();e.bottom(state);actual=e.owned(o)
 check(actual[:5]==original[:5] and len([x for x in actual if x==62])==1,'final Decision keeps all five original slots and exactly one Single Combat')
 check(not e.mod.val('metadata_fault') and e.profile(656).award_mask==1,'actual final award committed without corrupting metadata')
 e.mod.val('hover_module',e.ui.addr,'<Q');base=[e.alloc(0x198) for _ in range(5)];labels=[e.alloc(0x198) for _ in range(9)];cache=e.alloc(0x60)
 for p in base+labels:p.u64(0,e.widget_vt.addr)
 for i,p in enumerate(labels):cache.u64(8+i*8,p.addr)
 info=e.alloc(0x180);vt=e.alloc(0x240);info.u64(0,vt.addr);vt.u64(0x1a0,BR.addr('InfoPerson'))
 stale=e.alloc(0x200);stale.put(0x10,struct.pack('<H',656));stale.put(0x118,struct.pack('<9H',62,4,145,1,35,62,0,0,0))
 def old_renderer(p):
  for label in labels:e.texts[label.addr]='stale wrong label';e.visibility[label.addr]=True
 BR.callbacks.update({'InfoOriginal':old_renderer,'InfoPerson':lambda p:stale.addr,'LabelGet':lambda p,i:base[(i-31)//2].addr,'InfoCache':lambda p,c:cache.addr})
 for module,rva,name in [(e.ui,0x16a0,'InfoOriginal'),(e.ui,0x1190,'InfoCache'),(e.game,0x92c390,'LabelGet'),(e.game,0x85c270,'Text'),(e.game,0x856bc0,'CaptionColor')]:module.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
 before=o.get(0,0x200);e.fn('hover_x_info',info.addr)
 for i,key in enumerate(actual):
  assert get(labels[i].addr+0x80,'<I')==i
  if key:assert e.visibility[labels[i].addr] and e.texts[labels[i].addr]==wide(e.traits.addr+key*0x100+0x10)
  elif not (e.gs(656).x_mask&(1<<i)):assert not e.visibility[labels[i].addr]
 check(o.get(0,0x200)==before,'information labels and hover indices match live physical slots even with stale sorted base person; native traits unchanged')
 check(not BR.errors,'combined reroll and information callbacks complete without ABI errors')
 e.closeall()

if __name__=='__main__':
 zhang_fei();(ROOT/'audit/r16_ui_tests.json').write_text(json.dumps({'passed':len(REPORT),'tests':REPORT,'original_report_reproduced':False,'live_game_tested':False},ensure_ascii=False,indent=2)+'\n')
