"""R3 regressions: actual Win64 DLL, synthetic native/OS endpoints, NOT live game."""
from test_r2 import Evaluation
from r2_fixture import *

class R3(R2):
 def prepare(self,*a,**kw):
  o,r,s=super().prepare(*a,**kw)
  self.cancel_button=self.alloc(0x198);self.active_layout.u64(0x1c8,self.cancel_button.addr)
  self.game.put(self.res.funcs[15]+0x3a,bytes.fromhex('e8a13812004883c4205be9b7f9cbff'))
  self.game.put(0x856830,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('CancelButton')))
  self.game.put(0x3f2950,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('ConsumeInput')))
  self.cancel_invocations=0;self.consumed=0
  def cancel(button,kind):
   assert button==self.cancel_button.addr and kind==4
   self.cancel_invocations+=1;self.fn('hook_close',self.active_layout.addr,0x7fffffff);self.pop_state(0)
  BR.callbacks['CancelButton']=cancel
  BR.callbacks['ConsumeInput']=lambda:setattr(self,'consumed',self.consumed+1)
  return o,r,s
 def seed(self,seed):self.random_values=[seed>>32,seed&0xffffffff]
 def notice(self,s):self.fn('hook_state_tick',s.addr)
 def dismiss(self,s):self.action(10);self.fn('hook_state_tick',s.addr)

def miss_seed(e):
 t=target();s=State();d=Distribution()
 for seed in range(500):
  e.fn('g_init',C.addressof(s),C.addressof(t.traits))
  if call_roll(e,t,s,0,seed,0,0,d)==1 and s.slots[0].outcome==1:return seed
 raise AssertionError('no MISS seed in bounded fixture search')

def events(e):
 return [queued_record(e,i) for i in range(e.mod.val('log_write'))]

def engine_miss_passion():
 e=E();seed=miss_seed(e);t=target();s=State();d=Distribution();e.fn('g_init',C.addressof(s),C.addressof(t.traits));owned=bytes(t.traits)
 check(C.sizeof(Slot)==64 and C.sizeof(State)==232,'wire sizes preserved; v3 consumes reserved padding only')
 assert call_roll(e,t,s,0,seed,0,0,d)==1
 x=s.slots[0];ev=Evaluation();e.fn('g_evaluate',C.addressof(d),C.addressof(x),C.addressof(ev))
 check(x.outcome==1 and x.result==0 and x.x_index==255 and not any(bytes(d)),'MISS is separate outcome with no synthetic trait ID/candidate list')
 check(s.x_mask==0 and s.passion==0 and bytes(t.traits)==owned and s.budget==3 and s.heat==0,'preview MISS has no slot, ability, budget, heat or buff effects')
 check(ev.version==0 and ev.grade==0xffffffff,'MISS excluded from all six assessment grades')
 check(e.fn('g_accept',C.addressof(t),C.addressof(s),0,0,1)&0xffffffff==0xffffffff,'MISS cannot delete or decline-replace an existing trait')
 assert e.fn('g_accept',C.addressof(t),C.addressof(s),0,0,0)==1
 idx=s.slots[0].x_index
 check(s.x_mask==1<<idx and s.passion==1 and bytes(t.traits)==owned,'accept MISS occupies exact empty physical slot only in metadata and grants passion')
 check(e.fn('g_free_slot',C.addressof(t),C.addressof(s))!=idx and e.fn('g_validate',C.addressof(s))==1,'X counts toward capacity without becoming an effect or corrupting state')
 base=State.from_buffer_copy(s);base.passion=0;a=Distribution();b_=Distribution()
 e.fn('g_distribution',C.addressof(t),C.addressof(base),C.addressof(COLORS),0,0,C.addressof(a));e.fn('g_distribution',C.addressof(t),C.addressof(s),C.addressof(COLORS),0,0,C.addressof(b_))
 ratios=[]
 for i in range(355):
  if a.weight[i]>0:
   want=(1.35 if a.quality[i]>=.5 else 1)*(1.5 if COLORS[i]==3 else 1)
   assert abs(b_.weight[i]/a.weight[i]-want)<1e-9;ratios.append(want)
 check(any(v>1 for v in ratios),'passion multiplies good-fit/gold weights without changing raw DB similarities')
 assert call_roll(e,t,s,1,seed,0,0,d)==1
 for k in range(5):
  check(s.slots[1].outcome==0 and s.slots[1].passion_applied==1 and s.passion==1,'passion suppresses MISS and remains through next chance reroll '+str(k))
  assert call_roll(e,t,s,1,seed+k,1,0,d)==1
 result=s.slots[1].result;assert e.fn('g_accept',C.addressof(t),C.addressof(s),1,0,0)==1
 check(not s.passion and t.traits[idx]==0 and result in t.traits and e.fn('g_validate',C.addressof(s))==1,'actual next trait acceptance consumes buff once, leaves X intact')
 t.traits[idx]=4;e.fn('g_reconcile',C.addressof(t),C.addressof(s))
 check(s.x_mask==0 and t.traits[idx]==4,'external real trait edit removes overlapping X; never relocates it')
 e.closeall()

def last_full_and_rerolls():
 e=E();seed=miss_seed(e);t=target();s=State();d=Distribution();e.fn('g_init',C.addressof(s),C.addressof(t.traits))
 assert call_roll(e,t,s,2,seed,0,0,d)==1
 check(s.slots[2].outcome==0,'third physical reward tile is MISS-free even when rolled first')
 e.fn('g_accept',C.addressof(t),C.addressof(s),2,0,0);assert call_roll(e,t,s,1,100,0,0,d)==1;e.fn('g_accept',C.addressof(t),C.addressof(s),1,0,0)
 assert call_roll(e,t,s,0,seed,0,0,d)==1
 check(s.slots[0].outcome==0,'last chronological chance is MISS-free in reverse tile order')
 t=target(1);t.traits=(C.c_uint16*9)(4,5,6,7,8,9,10,11,12);e.fn('g_init',C.addressof(s),C.addressof(t.traits));old=bytes(t.traits)
 assert call_roll(e,t,s,0,seed,0,4,d)==1;assert e.fn('g_accept',C.addressof(t),C.addressof(s),0,4,0)==1
 check(s.slots[0].outcome==1 and s.slots[0].x_index==255 and s.x_mask==0 and s.passion==1 and bytes(t.traits)==old,'real 9/9 MISS keeps every incumbent and gives only result-X plus buff')
 e.fn('g_init',C.addressof(s),C.addressof(t.traits));assert call_roll(e,t,s,0,seed,0,4,d)==1;assert call_roll(e,t,s,0,100,1,4,d)==1
 check(s.slots[0].outcome==0 and s.rerolls_used==1 and not s.passion and not s.x_mask,'rerolling away a preview MISS spends one coupon but grants no X or buff')
 e.closeall()

def modal_and_transaction():
 e=R3();o,r,s=e.prepare(top=1);seed=miss_seed(e);old=o.get(0,0x200);e.seed(seed);e.click(r,0)
 check(e.mod.val('miss_modal')==0 and len(e.widget_inits)==7,'MISS button callback does not create a nested modal')
 e.tick+=1199;e.fn('hook_state_tick',s.addr);e.action(3)
 check(e.mod.val('ui_mode')==2 and not e.mod.val('miss_modal'),'MISS waits full 1200 ms; early skip cannot reveal or open notice')
 e.tick+=1;e.fn('hook_state_tick',s.addr);e.notice(s)
 check(e.mod.val('miss_modal')==1 and len(e.widget_inits)==9 and '열정 버프' in e.texts[e.control(9)],'one native blocking notice appears after animation on live state tick')
 rng=e.rng_calls;e.action(0);e.accept_preview();e.cancel_preview();e.fn('hook_confirm',s.addr)
 check(e.rng_calls==rng and e.gsui(596).slots[0].phase==1 and not e.native_commits,'notice blocks click-through reroll, accept, exit and bottom commit')
 e.dismiss(s);e.notice(s)
 check(not e.mod.val('miss_modal') and len(e.widget_inits)==9,'notice dismisses on safe tick without reopening same roll')
 e.accept_preview()
 check(o.get(0,0x200)==old and e.gs(596).passion==0 and e.gsui(596).passion==1,'toolbar accept remains speculative until original bottom commit')
 e.bottom(s)
 check(e.gs(596).passion==1 and e.gs(596).x_mask and e.owned(o)==struct.unpack('<9H',old[0x118:0x12a]),'bottom commit persists MISS without adding/deleting a real trait')
 miss=[q for q in events(e) if q['outcome']=='miss']
 check(miss and all(q['result_trait'] is None and q['evaluation'] is None and q['fit_quality'] is None for q in miss),'MISS audit has null trait/assessment/quality rather than misleading real-trait metrics')
 check(any(q['event']=='passion_awarded' for q in miss),'buff award is logged at actual commit')
 e.closeall()

def exit_and_personal_coupons():
 for reroll in [False,True]:
  e=R3();o,r,s=e.prepare(top=1);old=o.get(0,0x200);e.click(r,0);e.reveal(s)
  if reroll:e.action(0);e.reveal(s)
  check(not e.control(8),'no upper exit control after '+('reroll' if reroll else 'first draw'))
  e.cancel_preview();check(not e.cancel_invocations,'exit request never destroys layout inside widget callback')
  e.fn('hook_state_tick',s.addr);e.answer_exit(s)
  check(e.cancel_invocations==0 and e.pop_calls==1 and not e.mod.val('ui_mode') and not e.mod.val('preview_count'),'exit accepts last result through one original decision chain '+str(reroll))
  check(e.gs(596).slots[0].phase==2 and e.gs(596).slots[0].result in e.owned(o) and e.gs(596).rerolls_used==int(reroll),'latest exit rule keeps last trait; used personal coupons are not refunded '+str(reroll))
  check(e.fn('g_validate',C.addressof(e.gs(596)))==1,'cancelled coupon accounting remains persistable '+str(reroll));e.closeall()
 e=R3();o,r,s=e.prepare(top=1);e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s);e.dismiss(s);e.keys={27};e.fn('hook_state_tick',s.addr);e.answer_exit(s)
 check(not e.cancel_invocations and e.gs(596).passion and e.gs(596).x_mask,'latest ESC rule accepts MISS with its X/buff through decision')
 e.closeall()
 e=R3();o,r,s=e.prepare(top=3);other=e.officer(619,(65,68,53,51,60),(32,),3);r2=e.row(other,e.active_layout);e.fn('hook_row_refresh',r2.addr)
 e.click(r,0);e.reveal(s);e.action(0);e.reveal(s);e.accept_preview();e.click(r2,0);e.reveal(s)
 check(e.gsui(596).rerolls_used==1 and e.gsui(619).rerolls_used==0 and '5/5' in e.texts[e.control(0)],'page/row switch shows second officer own five coupons, not first officer count')
 for _ in range(5):e.action(0);e.reveal(s)
 e.accept_preview();e.click(r,1);e.reveal(s)
 check(e.gsui(596).rerolls_used==1 and e.gsui(619).rerolls_used==5 and '4/5' in e.texts[e.control(0)],'five rerolls on second officer cannot consume first officer remaining four')
 e.closeall()

def hover_result_and_x():
 e=R3();o,r,s=e.prepare(top=1);e.click(r,0);e.reveal(s);key=e.gsui(596).slots[0].result
 e.input.u32(0x34,202);e.input.u32(0x38,102);e.current_target=e.active_layout.addr;e.hover.clear();e.fn('hook_state_tick',s.addr)
 check((r.addr,key) in e.hover,'held result refreshes same existing Hover renderer even when disabled-button hit targets parent layout')
 overlay=e.alloc(0x198);overlay.u64(8,e.active_layout.addr);e.current_target=overlay.addr;e.hover.clear();e.fn('hook_state_tick',s.addr)
 check(not any(k for _,k in e.hover),'unrelated overlay blocks underlying result tooltip')
 e.current_target=e.row_buttons[0].addr;e.action(0);e.hover.clear();e.fn('hook_state_tick',s.addr)
 check(not any(k for _,k in e.hover),'reroll animation does not leak old/new trait tooltip')
 e.closeall()
 e=R3();o,r,s=e.prepare(top=1);e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s);e.dismiss(s);e.input.u32(0x34,202);e.input.u32(0x38,102);e.hover.clear();e.fn('hook_state_tick',s.addr)
 check(not any(k for _,k in e.hover),'MISS/X never asks real-trait Hover renderer to explain a fake ID')
 e.accept_preview();e.bottom(s);mask=e.gs(596).x_mask;idx=(mask&-mask).bit_length()-1;trait=C.c_uint16();is_x=C.c_uint32()
 check(e.fn('SAN14_MeritSlotDisplayV1',596,idx,C.addressof(trait),C.addressof(is_x))==1 and trait.value==0 and is_x.value==1,'read-only display adapter exposes X separately for other patches')
 o.put(0x118+idx*2,struct.pack('<H',4));e.fn('SAN14_MeritSlotDisplayV1',596,idx,C.addressof(trait),C.addressof(is_x))
 check(trait.value==4 and not is_x.value and not e.gs(596).x_mask,'live external edit takes precedence in shared display API')
 e.closeall()

def shared_collect_and_columns():
 e=R3();o,r,s=e.prepare(top=1);e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s);e.dismiss(s);e.accept_preview();e.bottom(s)
 e.mod.val('hover_module',e.ui.addr,'<Q');put(e.mod.sym('hover_x_caption')+0x10,ord('X'),'<H')
 e.game.put(0x22fb00,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('TraitAt')));BR.callbacks['TraitAt']=lambda p,i:get(p+0x118+2*i,'<H')
 items=(C.c_void_p*9)();slots=(C.c_uint32*9)();active=(C.c_uint32*9)();args=(o.addr,o.addr,0,0,C.addressof(items),C.addressof(slots),C.addressof(active))
 n=e.fn('hover_x_collect',*args);idx=(e.gs(596).x_mask&-e.gs(596).x_mask).bit_length()-1
 check(n==9 and list(slots)==list(range(9)) and wide(items[idx]+0x10)=='X' and not active[idx],'original bundled Hover collector extended to physical X/empty nine-cell layout')
 check(all(items[i]==e.traits.addr+key*0x100 for i,key in enumerate(e.owned(o)) if key),'real trait UI objects come unchanged from existing Hover collector')
 saved=e.gs(596).x_mask;e.gs(596).x_mask=0;n=e.fn('hover_x_collect',*args)
 check(n==5 and list(slots)[:5]==[0,1,2,3,4],'officer without X keeps original compact rendering exactly')
 e.gs(596).x_mask=saved
 # Real descriptor-construction code executes; only callback target is mocked.
 for base,first in [(0x1a2b850,0x3d),(0x1a35820,0x23)]:
  for i in range(5):e.game.u32(base+(first+i)*96,first+i);e.game.u64(base+(first+i)*96+24,BR.addr('ColumnOriginal'))
 text=e.alloc(40,'원래 설명\0'.encode('utf-16le'));BR.callbacks['ColumnOriginal']=lambda p:text.addr
 table=e.alloc(0x6300);source=e.alloc(0x6300)
 for i in range(5):table.u32((0x3d+i)*96,0x3d+i);table.u64((0x3d+i)*96+24,BR.addr('ColumnOriginal'))
 e.fn('hover_x_columns',table.addr,source.addr,0,256)
 f=get(table.addr+(0x3d+idx)*96+24) if idx<5 else get(table.addr+(0x100+idx-5)*96+24)
 check(wide(BR.call(f,o.addr))=='X','list-column name callback draws X without modifying native trait getter')
 f=get(table.addr+0x3d*96+24);check(wide(BR.call(f,o.addr))=='원래 설명','real list-column names delegate original callback')
 base_table=e.alloc(0x6300)
 for i in range(5):base_table.u32((0x23+i)*96,0x23+i);base_table.u64((0x23+i)*96+24,BR.addr('ColumnOriginal'))
 e.fn('hover_x_columns',base_table.addr,source.addr,1,256)
 f=get(base_table.addr+((0x23+idx) if idx<5 else (0x100+idx-5))*96+24)
 check(wide(BR.call(f,o.addr))=='X','base-officer list extra trait column uses same X mask')
 e.ui.put(0x1fd0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('UnitPerson')));BR.callbacks['UnitPerson']=lambda id:e.officers[id].addr
 for i in range(5):table.u32((0xaa+i)*96,0xaa+i);table.u64((0xaa+i)*96+24,BR.addr('ColumnOriginal'))
 e.fn('hover_x_columns',table.addr,source.addr,0,256);unit=e.alloc(0x40);unit.put(0x12,struct.pack('<H',596))
 f=get(table.addr+((0xaa+idx) if idx<5 else (0x104+idx-5))*96+24)
 check(wide(BR.call(f,unit.addr))=='X','unit list resolves commander ID then draws its X column')
 check(not BR.errors,'shared collector/list Microsoft ABI endpoints report no callback errors')
 e.closeall()

def persistence_and_v2_migration():
 e=R3();o,r,s=e.prepare(top=3);e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s);e.dismiss(s);e.accept_preview();e.bottom(s)
 old=bytes(e.gs(596));native=b'R3 committed MISS exact native snapshot';owned=o.get(0,0x200)
 other=e.officer(619,(65,68,53,51,60),(32,),3)
 otherrow=e.row(other,e.active_layout);e.fn('hook_row_refresh',otherrow.addr)
 assert e.fn('perform_roll',other.addr,0,0,0)==1
 assert e.fn('perform_roll',other.addr,0,1,0)==1
 assert e.fn('perform_roll',other.addr,0,1,0)==1
 otherstate=bytes(e.gs(619));check(e.save(native)==1,'committed MISS and independent second officer coupons save through existing journal')
 rng=e.rng_calls;e.gs(596).passion=0;e.gs(596).x_mask=0;e.gs(619).rerolls_used=0
 check(e.loadsave(native)==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==old and bytes(e.gs(619))==otherstate,'V3 load restores MISS/X/passion and each officer coupon state exactly')
 check(e.rng_calls==rng,'loading committed MISS does not reroll or duplicate buff')
 e.active_layout.u32(0x180,0);e.fn('hook_row_refresh',r.addr);e.fn('hook_state_tick',s.addr);e.click(r,1);e.reveal(s);e.action(0);e.reveal(s)
 check(e.gsui(596).slots[1].passion_applied==1 and e.gsui(596).passion==1,'loaded passion remains through a next-opportunity reroll')
 e.cancel_preview();e.fn('hook_state_tick',s.addr);e.answer_exit(s)
 check(e.gs(596).passion==0 and e.gs(596).x_mask and e.gs(596).rerolls_used==1 and e.gs(596).slots[1].result in e.owned(o),'exit accepts next opportunity, consumes passion once and retains that officer spent coupon')
 assert e.save(b'R3 after cancel');expected=bytes(e.gs(596));assert e.loadsave(b'R3 after cancel')==1
 check(bytes(e.gs(596))==expected and e.gs(619).rerolls_used==2,'cancelled coupon accounting survives save/load without mixing officers')
 e.closeall()
 # Simulate authentic v2 ignored padding and update the existing journal hash.
 e=R3();o,r,s=e.prepare(top=1);e.click(r,0);e.reveal(s);e.accept_preview();e.bottom(s);assert e.save(b'R2 existing saved result')
 refpath,ref=e.ref();key=bytes(ref.key).hex();path=next(p for p in e.files if p.endswith(key+f'.slot{ref.current}.mt4dat'))
 raw=bytearray(e.files[path]);off=88+6000*C.sizeof(Profile)+596*C.sizeof(State);expected=State.from_buffer_copy(raw[off:off+C.sizeof(State)])
 struct.pack_into('<I',raw,8,2);raw[off+20:off+24]=b'\xa5'*4
 for j in range(3):
  start=off+State.slots.offset+j*C.sizeof(Slot)+26;raw[start:start+6]=b'\xa5'*6
  expected.slots[j].outcome=0;expected.slots[j].passion_applied=0;expected.slots[j].x_index=0;expected.slots[j].pad[:]=[0]*3
 raw[56:88]=hashlib.sha256(raw[88:]).digest();e.files[path]=bytes(raw);ref.body[:]=raw[56:88];rb=bytearray(bytes(ref));rb[-32:]=hashlib.sha256(rb[:-32]).digest();e.files[refpath]=bytes(rb)
 rng=e.rng_calls;check(e.loadsave(b'R2 existing saved result')==1 and not e.mod.val('metadata_fault'),'v2 journal with ignored padding migrates without invalidating its pinned bytes')
 check(bytes(e.gs(596))==bytes(expected) and e.rng_calls==rng and e.files[path]==bytes(raw),'v2 acquired result stays unchanged; only new in-memory fields default to zero')
 check(e.save(b'R3 migrated save')==1,'migrated v2 state can be written as a new v3 journal generation')
 e.closeall()

def modal_lifecycle_and_cancel_fallback():
 e=R3();o,r,s=e.prepare(top=1);e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s)
 e.active_layout.u32(0x180,1);e.fn('hook_state_tick',s.addr);e.dismiss(s);e.accept_preview();e.fn('hook_state_tick',s.addr)
 check(e.active_layout.get(0x180,4)==b'\0'*4 and e.gs(596).slots[0].phase==0,'bottom confirm queued behind MISS modal is discarded, never auto-commits after toolbar accept')
 e.cancel_preview();e.fn('hook_state_tick',s.addr);e.answer_exit(s)
 check(e.gs(596).passion and e.gs(596).x_mask,'latest exit rule commits accepted MISS with X and passion')
 e.closeall()
 e=R3();o,r,s=e.prepare(top=1);seed=miss_seed(e);e.seed(seed);e.click(r,0);e.reveal(s);e.notice(s);e.dismiss(s);e.seed(seed);e.action(0);e.reveal(s);e.notice(s)
 check(e.mod.val('miss_modal') and len(e.widget_inits)==9,'rerolled MISS reuses modal widgets and announces new roll once')
 stale=e.widget_callbacks[e.control(10)];e.fn('hook_row_destroy',r.addr,e.resources.addr);e.fn('control_invoke',stale.addr,0)
 check(not e.mod.val('miss_modal') and len(e.widget_deletes)==9,'row destruction dismisses notice, releases all widgets once, rejects stale callback')
 e.keys={27};e.fn('hook_state_tick',s.addr);e.answer_exit(s)
 check(not e.cancel_invocations and not e.pop_calls and e.mod.val('preview_count'),'missing warning widgets never imply consent after row destruction')
 e.keys=set();e.bottom(s)
 check(e.pop_calls==1 and not e.mod.val('preview_count'),'explicit original decision remains available when warning cannot be constructed')
 e.closeall()
 e=R3();o,r,s=e.prepare(top=1);e.click(r,0);e.reveal(s);e.game.put(e.res.funcs[15]+0x3a,b'\x90');e.cancel_preview();e.fn('hook_state_tick',s.addr);e.answer_exit(s)
 check(not e.mod.val('cancel_requested') and not e.mod.val('ui_mode') and not e.mod.val('preview_count') and e.pop_calls==1,'result exit uses decision and does not depend on the native cancel adapter')
 e.fn('hook_state_tick',s.addr);check(not e.cancel_invocations and not BR.errors,'next idle native tick remains safe after guarded cancel fallback')
 e.closeall()
 e=R3();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),top=1);e.click(r,0);e.seed(miss_seed(e));e.action(3);e.reveal(s);e.notice(s)
 check('기존 개성은 지워지지 않습니다' in e.texts[e.control(9)] and '빈 개성칸 하나' not in e.texts[e.control(9)],'full 9/9 MISS notice accurately promises no existing trait deletion')
 e.closeall()

def information_labels():
 e=R3();o,r,s=e.prepare(top=1);e.seed(miss_seed(e));e.click(r,0);e.reveal(s);e.notice(s);e.dismiss(s);e.accept_preview();e.bottom(s)
 e.mod.val('hover_module',e.ui.addr,'<Q');idx=(e.gs(596).x_mask&-e.gs(596).x_mask).bit_length()-1
 base=[e.alloc(0x198) for _ in range(5)];labels=[e.alloc(0x198) for _ in range(9)];cache=e.alloc(0x60)
 for p in base+labels:p.u64(0,e.widget_vt.addr)
 for i,p in enumerate(labels):cache.u64(8+i*8,p.addr)
 info=e.alloc(0x180);vt=e.alloc(0x240);info.u64(0,vt.addr);vt.u64(0x1a0,BR.addr('InfoPerson'))
 baseperson=e.alloc(0x200);baseperson.put(0x10,struct.pack('<H',596));calls=[]
 BR.callbacks.update({'InfoOriginal':lambda p:calls.append(p),'InfoPerson':lambda p:baseperson.addr,'LabelGet':lambda p,i:base[(i-31)//2].addr,'InfoCache':lambda p,c:cache.addr,'CaptionColor':lambda *a:0})
 for module,rva,name in [(e.ui,0x16a0,'InfoOriginal'),(e.ui,0x1190,'InfoCache'),(e.game,0x92c390,'LabelGet'),(e.game,0x85c270,'Text'),(e.game,0x856bc0,'CaptionColor')]:module.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
 e.fn('hover_x_info',info.addr)
 check(calls==[info.addr] and e.texts[labels[idx].addr]=='X' and e.visibility[labels[idx].addr],'information panel calls existing renderer first and overlays X at physical slot')
 check(all(get(p.addr+0x80,'<I')==i for i,p in enumerate(labels)) and all(not e.visibility[p.addr] for p in base),'nine information labels retain physical hover indices; old five labels hidden')
 check(all(e.texts[labels[i].addr]==wide(e.traits.addr+key*0x100+0x10) for i,key in enumerate(e.owned(o)) if key),'real information labels reuse game trait names, not a replacement description table')
 check(all(not e.visibility[p.addr] for i,p in enumerate(labels) if not e.owned(o)[i] and i!=idx),'unoccupied non-X information labels remain hidden')
 before=len(e.text_history);e.gs(596).x_mask=0;e.fn('hover_x_info',info.addr)
 check(len(calls)==2 and len(e.text_history)>before and all(e.texts[labels[i].addr]==wide(e.traits.addr+key*0x100+0x10) for i,key in enumerate(e.owned(o)) if key),'non-X acquired information panel refreshes actual native physical traits')
 check(not BR.errors,'information overlay native ABI callbacks complete without errors');e.closeall()

def hover_patch_transaction():
 e=Env();sites=[0x2740,0x29a3,0x2e79,0x4355,0x398b,0x53ec];before={i:e.ui.get(i,5) for i in sites}
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1 and ('SAN14CE.MeritTraits.HoverX',1) in e.patch_calls,'full exact-build initialization installs seven guarded Hover display sites; only game site uses Core')
 check(all(e.ui.get(i,5)!=v for i,v in before.items()) and e.mod.val('hover_x_count')==7,'six renderer call sites and one information-panel bridge are installed')
 calls=e.patch_calls[:];check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1 and e.patch_calls==calls,'repeat initialization does not duplicate X bridges');e.closeall()
 e=Env();old={h.rva:e.game.get(h.rva,h.length) for h in e.res.hooks if h.length};before={i:e.ui.get(i,5) for i in sites};orig=e.patch
 def partial(name,p,n):
  if C.string_at(name)==b'SAN14CE.MeritTraits.HoverX':
   e.patch_calls.append(('SAN14CE.MeritTraits.HoverX',n));items=(CorePatch*n).from_address(p)
   for item in items[:4]:C.memmove(item.address,bytes(item.after[:item.length]),item.length)
   return 0
  return orig(name,p,n)
 BR.callbacks['Patch']=partial
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0 and not e.mod.val('enabled'),'partial Hover bridge installation fails closed without gameplay activation')
 check(all(e.ui.get(i,5)==v for i,v in before.items()) and all(e.game.get(i,len(v))==v for i,v in old.items()),'partial X installation rolls back its own bytes and all new gameplay hooks')
 check(get(e.game.addr+0x4915a1+6)==e.ui.addr+0x16a0 and e.mod.val('hover_x_relays',fmt='<Q')==0,'rollback retains original shared Hover jump and releases unused X relay page')
 e.closeall()

if __name__=='__main__':
 try:
  for f in [engine_miss_passion,last_full_and_rerolls,modal_and_transaction,exit_and_personal_coupons,hover_result_and_x,shared_collect_and_columns,persistence_and_v2_migration,modal_lifecycle_and_cancel_fallback,information_labels,hover_patch_transaction]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r3_tests.json').write_text(json.dumps({'scope':'Actual Win64 DLL plus bundled Hover code with substituted OS/game endpoints; NOT Windows/live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R3',len(REPORT))
