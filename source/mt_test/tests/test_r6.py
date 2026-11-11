"""R6 production Win64 DLL + preserved native click/tick/render/close branches.
OS, widget allocation, native status/getter/refresh and cost/fade endpoints are
substituted. These tests do not certify Windows, DirectX or live-game crashes.
"""
from r6_fixture import *
import subprocess

def settled(e,s):
 e.reveal(s);e.notice(s)
 if e.mod.val('miss_modal'):e.dismiss(s)

def basic():
 e=R6();o,r,s=e.prepare(lower=2,capacity=3)
 check(e.adapter_enable.get(get(r.addr+0x1b8))==1,'Zhao Yun lower 2: only upper first is enabled')
 for j in [1,2]:
  rng=e.rng_calls;e.click(r,j);check(e.rng_calls==rng and e.adapter_enable.get(get(r.addr+0x1b8+8*j))==0,'blocked future upper tile '+str(j))
 e.click(r,0);check(e.selection_counts(596)==(1,2),'captured native click registers military choice exactly once')
 settled(e,s);check(e.decision_enabled and e.selected_state(r,0)==1,'result keeps original selected skin and native decision is enabled')
 e.bottom(s);check(e.native_commits==1 and e.pop_calls==1 and e.gs(596).slots[0].phase==2,'bottom commits one trait/one native award/one pop')
 check(b(o.addr+0x1c2)==1 and b(o.addr+0x1c3)==2,'native total stays 3, domestic rewards preserved')
 e.closeall()

def allocation_rules():
 for cap in [1,2,3]:
  for lower in range(cap):
   e=R6();o,r,s=e.prepare(lower=lower,capacity=cap);available=cap-lower
   for stage in range(available):
    rng=e.rng_calls
    for later in range(stage+1,3):e.click(r,later)
    check(e.rng_calls==rng,'left prefix rejects skipped slots cap/lower/stage '+str((cap,lower,stage)))
    e.click(r,stage);settled(e,s)
    check(e.selection_counts(596)==(stage+1,lower) and e.selected_state(r,stage)==1,'native selection advances one upper reward '+str((cap,lower,stage)))
   rng=e.rng_calls
   for stage in range(available,3):e.click(r,stage)
   for j in range(3+lower,6):e.click(r,j)
   check(e.rng_calls==rng and e.selection_counts(596)==(available,lower),'shared budget blocks both rows after all points used '+str((cap,lower)))
   check(e.decision_enabled,'native decision available at exact shared limit '+str((cap,lower)))
   e.bottom(s)
   check(e.pop_calls==1 and e.native_commits==1 and sum(x.phase==2 for x in e.gs(596).slots)==available,'all eligible stage results commit independently '+str((cap,lower)))
   e.closeall()
 # Both row directions use same native prefix, including deselection.
 e=R6();o,r,s=e.prepare(capacity=3);e.click(r,5)
 check(e.selection_counts(596)==(0,0),'domestic third cannot skip earlier domestic rewards')
 e.click(r,3);e.click(r,4);e.click(r,0);settled(e,s);result=e.gsui(596).slots[0].result;rng=e.rng_calls
 e.click(r,0)
 check(e.selection_counts(596)==(0,2) and not e.decision_enabled and not e.adapter_enable[e.control(0)],'deselect upper uses native radio and disables its reroll')
 e.action(0);e.click(r,5);e.bottom(s)
 check(e.rng_calls==rng and e.pop_calls==1 and b(o.addr+0x1c2)==0 and b(o.addr+0x1c3)==3 and e.gs(596).slots[0].phase==0 and result not in e.owned(o),'switch to domestic final reward drops speculative trait, commits only native domestic choice')
 e.closeall()


def cancel():
 e=R6();o,r,s=e.prepare(lower=2,capacity=3);before=o.get(0,0x200);e.click(r,0);settled(e,s);e.action(0);settled(e,s)
 e.request_back(s);check(e.mod.val('exit_modal') and '초기화' in e.dialog_text(11) and '복구되지 않습니다' in e.dialog_text(11),'Back opens result reset and no-refund warning')
 before_ui=bytes(e.gsui(596));e.answer_exit(s,False);check(bytes(e.gsui(596))==before_ui and e.gs(596).rerolls_used==1,'No keeps exact result and spent coupon')
 e.request_back(s);e.answer_exit(s,True)
 check(e.pop_calls==1 and e.native_commits==0 and o.get(0,0x200)==before,'Yes cancels through native close without costs/rank/traits mutation')
 check(e.gs(596).slots[0].phase==0 and e.gs(596).rerolls_used==1 and e.selection_counts(596)==(0,2),'cancel resets result/native choice but does not refund personal reroll')
 saved=bytes(e.gs(596));native=b'R6 native save unchanged by gacha cancellation'
 check(e.save(native)==1,'cancelled personal ticket reservation can be saved')
 e.gs(596).rerolls_used=e.gs(596).cancelled_rerolls=0
 check(e.loadsave(native)==1 and bytes(e.gs(596))==saved,'save/load restores spent tickets without restoring discarded result')
 e.closeall()

def cancel_modes_and_focus():
 for mode in ['spin','result','reroll_spin','miss']:
  for route in ['back','esc']:
   e=R6();o,r,s=e.prepare();before=o.get(0,0x200)
   if mode=='miss':e.seed(miss_seed(e))
   e.click(r,0)
   if mode!='spin':settled(e,s)
   if mode=='reroll_spin':e.action(0)
   before_ui=bytes(e.gsui(596));used=e.gs(596).rerolls_used
   queued=[int(route=='esc')];BR.callbacks['Extra']=lambda:queued[0];BR.callbacks['ConsumeInput']=lambda:queued.__setitem__(0,0)
   if route=='esc':e.keys={27};e.fn('hook_state_tick',s.addr)
   else:e.keys={1};e.request_back(s)
   check(e.mod.val('exit_modal') and not e.native_commits and not e.pop_calls,'warning before any exit '+mode+'/'+route)
   e.raw_action(12);e.tick+=5000;e.fn('hook_state_tick',s.addr)
   check(e.mod.val('exit_modal') and not e.mod.val('exit_choice'),'held opener never implies Yes '+mode+'/'+route)
   e.keys=set();e.fn('hook_state_tick',s.addr);e.keys={27};e.fn('hook_state_tick',s.addr)
   check(not e.mod.val('exit_modal') and bytes(e.gsui(596))==before_ui and e.gs(596).rerolls_used==used,'fresh ESC means No with identical preview '+mode+'/'+route)
   e.keys=set();e.request_back(s);e.answer_exit(s,True)
   check(e.pop_calls==1 and e.cancel_invocations==1 and not e.native_commits and o.get(0,0x200)==before and e.gs(596).rerolls_used==used and not e.gs(596).x_mask and not e.gs(596).passion,'Yes discards through one cancel without X/passion/refund '+mode+'/'+route)
   e.closeall()


def multiple_officers():
 e=R6();o,r,s=e.prepare(id=649);o2,r2=e.second();other=bytes(e.gsui(619));untouched=o2.get(0,0x200)
 e.click(r,0)
 check(bytes(e.gsui(619))==other and o2.get(0,0x200)==untouched and e.adapter_enable[get(r2.addr+0x1b8)]==1,'Jo Tong spin does not mutate or disable Jo Gwang')
 settled(e,s);e.action(0);settled(e,s);tong=bytes(e.gsui(649));stale=e.widget_callbacks[e.control(0)]
 e.click(r2,0);settled(e,s);rng=e.rng_calls;e.fn('control_invoke',stale.addr,0);e.fn('hook_state_tick',s.addr)
 check(e.rng_calls==rng and bytes(e.gsui(649))==tong,'callback captured for previous officer cannot spend next officer coupon')
 for _ in range(5):e.action(0);settled(e,s)
 check(e.gs(649).rerolls_used==1 and e.gs(619).rerolls_used==5,'officers reserve their own five tickets independently')
 rng=e.rng_calls;e.action(0)
 check(e.rng_calls==rng and not e.adapter_enable[e.control(0)],'exhausted officer sixth reroll is blocked at widget and command boundary')
 # Returning to an existing result is original deselection, then reselect; no RNG.
 e.click(r,0);e.click(r,0)
 check(e.rng_calls==rng and e.selection_counts(649)==(1,0) and '4/5' in e.texts[e.control(0)],'returning to another officer retains result/native choice and individual remaining count')
 keys=[e.gsui(i).slots[0].result for i in (649,619)];e.bottom(s)
 check(e.native_commits==1 and e.pop_calls==1 and keys[0] in e.owned(o) and keys[1] in e.owned(o2),'one original decision applies separate per-officer results')
 check(e.gs(649).rerolls_used==1 and e.gs(619).rerolls_used==5 and all(e.fn('g_validate',C.addressof(e.gs(i))) for i in (649,619)),'independent coupon accounting remains valid after commit')
 e.closeall()


def replacements():
 e=R6();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12));before=e.owned(o)
 e.click(r,0);check(e.mod.val('ui_mode')==1 and e.selection_counts(1)==(0,0),'full non-star chooses actual incumbent before rolling')
 e.action(2);old=get(e.mod.sym('selected_replacement')+2,'<H')
 e.action(3);settled(e,s);new=e.gsui(1).slots[0].result
 e.bottom(s);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and e.mod.val('warning_kind')==2 and '기존:' in e.dialog_text(11) and '획득:' in e.dialog_text(11) and e.owned(o)==before and not e.native_commits,'Decision asks explicit old/new replacement confirmation before any mutation')
 e.answer_exit(s,False);check(e.owned(o)==before and not e.native_commits and not e.pop_calls,'replacement No keeps old traits and preview on original screen')
 e.bottom(s);e.fn('hook_state_tick',s.addr);e.answer_exit(s,True)
 check(e.pop_calls==1 and e.native_commits==1 and new in e.owned(o) and old not in e.owned(o) and len([x for x in e.owned(o) if x])==9,'replacement Yes applies exact old-to-new pair once through original decision')
 e.closeall()
 # More than one replacement requires separate consent, then one commit.
 e=R6();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),capacity=2)
 for stage in (0,1):
  e.click(r,stage)
  if e.mod.val('ui_mode')==1:e.action(3)
  settled(e,s)
 e.bottom(s);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('replacement_count')==2 and '(1/2)' in e.dialog_text(11),'two independently selected replacements are both listed')
 e.answer_exit(s,True);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('exit_modal') and '(2/2)' in e.dialog_text(11) and not e.native_commits,'first replacement Yes does not implicitly consent to second replacement')
 e.answer_exit(s,True);check(e.native_commits==1 and e.pop_calls==1 and all(x.phase==2 for x in e.gs(1).slots[:2]),'all replacement Yes answers finalize one transaction')
 e.closeall()


def replacement_native_change():
 e=R6();o,r,s=e.prepare(id=1,owned=(4,5,6,7,8,9,10,11,12),capacity=2);before=e.owned(o)
 for stage in (0,1):
  e.click(r,stage)
  if e.mod.val('ui_mode')==1:e.action(3)
  settled(e,s)
 e.bottom(s);e.fn('hook_state_tick',s.addr);e.answer_exit(s,True);e.fn('hook_state_tick',s.addr)
 # A native auto-selection/external UI path bypassed hook_button_event.
 # It changes the transient map but NOT our callback revision.
 revision=e.mod.val('choice_revision',fmt='<Q');e.choice_nodes[1].u32(0x20,0x11)
 e.answer_exit(s,True);e.fn('hook_state_tick',s.addr)
 check(e.mod.val('choice_revision',fmt='<Q')==revision and e.mod.val('exit_modal') and e.mod.val('replacement_count')==1 and not e.native_commits and e.owned(o)==before,'native selection change invalidates replacement consent even without a callback revision change')
 e.answer_exit(s,True)
 check(e.pop_calls==1 and e.native_commits==1 and b(o.addr+0x1c2)==1 and b(o.addr+0x1c3)==1 and e.gs(1).slots[1].phase==0,'new explicit consent applies only the current native-selected replacement')
 e.closeall()

def errors_and_original_auto():
 e=R6();o,r,s=e.prepare(capacity=2)
 # Native automatic selection filled two upper choices, no result exists yet.
 e.choice_nodes[596].u32(0x20,2);e.refresh_choices(e.list_owner.addr);e.bottom(s)
 check(not e.native_commits and not e.pop_calls,'native automatic military selection cannot silently grant an unrolled trait')
 e.click(r,0);settled(e,s);e.bottom(s)
 check(not e.native_commits and not e.pop_calls,'one preview cannot commit another selected stage with no result')
 e.click(r,1);settled(e,s);e.fail_commit=True;e.bottom(s);count=e.native_commits;e.fn('hook_state_tick',s.addr)
 check(count==1 and e.native_commits==count and not e.pop_calls and e.mod.val('preview_count')==1,'failed native cost/commit retains results without frame-by-frame retries')
 e.fail_commit=False;e.bottom(s);check(e.native_commits==2 and e.pop_calls==1,'fresh Decision retries a refused native commit safely')
 e.closeall()
 # New exact native binding checks before patch installation.
 from test_native import Env
 e=Env();check(e.fn('native_guard_check')==1,'resolver verifies both new native selection bindings')
 for field in ['counts_site','refresh_site']:
  p=getattr(e.res.selection,field);old=e.game.get(p,5);e.game.put(p,b'\x90')
  check(e.fn('native_guard_check')==0,'modified original selection call is rejected: '+field);e.game.put(p,old)
 e.closeall()


def style_and_original_layers():
 e=R6();o,r,s=e.prepare(lower=2,capacity=3)
 check(not e.control(1) and not e.control(8),'removed Confirm/Exit have no widget or hitbox')
 check(e.texts[e.control(4)]=='계산 근거','calculation wording is exact')
 for i in [0,4]:
  w=e.control(i);p=get(w+0x188);width=get(w+0x28,'<I');height=get(w+0x2c,'<I')
  check(get(w+0x2c,'<I')==44 and C.string_at(p+0x200,0x78)==C.string_at(e.selection_prim+0x200,0x78) and e.aligns[p]==4 and e.rects[p]=={'size':(width-8,height-8),'pos':(4,4)},'same Selection height/font and centered label '+str(i))
 check(get(e.control(0)+0x24,'<I')==26,'reroll baseline equals original Selection')
 # Independent original renderer, not our status/paint model.
 subprocess.run(['gcc','-shared','-fPIC','-Wl,-z,noexecstack',str(ROOT/'tests/native_layers_wrapper.s'),'-o',str(ROOT/'tests/native_layers_wrapper.so')],check=True,capture_output=True)
 shim=C.CDLL(str(ROOT/'tests/native_layers_wrapper.so'));entry=C.cast(shim.native_layers_wrapper,C.c_void_p).value
 evidence=json.loads((ROOT/'analysis/native_reward_layers.json').read_text());e.game.put(evidence['rva'],bytes.fromhex(evidence['bytes']));e.game.put(evidence['end'],bytes.fromhex('4881c480000000415e5f5e5d5bc3'))
 name=e.alloc(32,'원작 보상\0'.encode('utf-16le'));BR.callbacks['RewardName']=lambda kind:name.addr
 for k,n in [('reward_name','RewardName'),('radio_enabled','RadioEnable')]:e.game.put(evidence['calls'][k],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(n)))
 # No status3 path needed here; test available vs disabled and native selected bits.
 status=e.alloc(24,struct.pack('<6i',1,0,0,2,2,1));BR.call(entry,r.addr,status.addr,e.game.addr+evidence['rva'],1|(2<<8))
 check(e.selected_state(r,0)==1 and e.adapter_enable[get(r.addr+0x1b8)]==1,'captured original renderer marks selected status1 as native selected')
 check(e.selected_state(r,1)==0 and e.adapter_enable[get(r.addr+0x1c0)]==0,'captured original status0 branch disables future upper tile')
 BR.call(entry,r.addr,status.addr,e.game.addr+evidence['rva'],3<<8)
 check(e.selected_state(r,0)==0 and e.selected_state(r,5)==1,'same native Selected vslot governs upper and domestic button skin')
 e.closeall()


def warning_allocations_and_lifetime():
 for fail_at in [1,2,3]:
  e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);e.action(0);settled(e,s);state=bytes(e.gsui(596));original=BR.callbacks['WidgetAlloc'];n=[0]
  def alloc(size):
   n[0]+=1;return 0 if n[0]==fail_at else original(size)
  BR.callbacks['WidgetAlloc']=alloc;e.request_back(s)
  check(not e.mod.val('exit_modal') and not e.pop_calls and not e.native_commits and bytes(e.gsui(596))==state and not any(e.control(i) in e.registered for i in [11,12,13]),'warning allocation failure remains hidden and never consents '+str(fail_at))
  BR.callbacks['WidgetAlloc']=original;e.request_back(s);check(e.mod.val('exit_modal'),'missing warning allocation can be retried '+str(fail_at))
  e.answer_exit(s);e.fn('hook_row_destroy',r.addr,e.resources.addr)
  check(e.pop_calls==1 and e.gs(596).rerolls_used==1 and not e.native_commits and len(e.widget_deletes)==len(set(e.widget_deletes)) and not BR.errors,'retry cancels once and destroys widgets once '+str(fail_at))
  e.closeall()
 e=R6();o,r,s=e.prepare();o2,r2=e.second();e.click(r2,0);settled(e,s);e.action(0);settled(e,s);stale=e.widget_callbacks[e.control(0)];rng=e.rng_calls
 e.fn('hook_row_destroy',r.addr,e.resources.addr);e.fn('hook_row_refresh',r2.addr);e.fn('control_invoke',stale.addr,0);e.fn('hook_state_tick',s.addr)
 check(e.rng_calls==rng and e.gs(619).rerolls_used==1,'page/anchor row destruction preserves another officer and invalidates callbacks')
 e.action(0);settled(e,s);e.request_back(s);stale=e.widget_callbacks[e.control(12)];e.fn('hook_ui_exit',e.root_layout.addr);e.fn('hook_row_destroy',r2.addr,e.resources.addr);e.fn('control_invoke',stale.addr,0)
 check(e.gs(619).rerolls_used==2 and not e.native_commits and not e.registered and not e.mod.val('preview_count') and not e.mod.val('exit_modal'),'forced detach cancels preview but preserves spent tickets; stale Yes cannot commit')
 check(len(e.widget_deletes)==len(set(e.widget_deletes)) and all(ctx==e.resources.addr for _,ctx in e.destroy_contexts),'all widgets use exact native resource context once')
 e.closeall()


def fallback_input():
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s)
 def cursor(w):
  x=y=0;p=w
  while p:x+=get(p+0x20,'<I');y+=get(p+0x24,'<I');p=get(p+8)
  e.input.u32(0x34,x+8);e.input.u32(0x38,y+8);e.current_target=w
 def tick(down):e.keys={1} if down else set();e.fn('hook_state_tick',s.addr)
 cursor(e.control(0));used=e.gs(596).rerolls_used;tick(True);tick(False)
 check(e.gs(596).rerolls_used==used+1,'logical game-coordinate fallback activates reroll when native callback is missing')
 settled(e,s);cursor(e.control(0));used=e.gs(596).rerolls_used;tick(True);e.raw_action(0);tick(False)
 check(e.gs(596).rerolls_used==used+1,'native callback plus fallback cannot spend two tickets for one gesture')
 settled(e,s);cursor(e.control(0));used=e.gs(596).rerolls_used;tick(True);e.current_target=e.alloc(0x200).addr;tick(False)
 check(e.gs(596).rerolls_used==used,'release over another overlay cancels captured click')
 cursor(e.control(0));tick(True);BR.callbacks['GetForegroundWindow']=lambda:0;tick(False);e.raw_action(0);e.fn('hook_state_tick',s.addr)
 check(e.gs(596).rerolls_used==used,'focus loss cancels press and rejects native callback')
 BR.callbacks['GetForegroundWindow']=lambda:123;cursor(e.control(4));e.raw_action(4);e.raw_action(4);tick(False)
 check(e.mod.val('details_expanded')==1,'duplicate native commands in one frame dispatch only once on safe tick')
 cursor(e.control(4));tick(True);tick(False);e.raw_action(4);tick(False)
 check(e.mod.val('details_expanded')==0,'delayed native callback after fallback cannot toggle details twice')
 e.request_back(s);e.keys=set();e.fn('hook_state_tick',s.addr);cursor(e.control(13));tick(True);tick(False)
 check(not e.mod.val('exit_modal') and not e.pop_calls,'warning No works through same logical-coordinate fallback')
 e.closeall()


def last_chance_and_hover():
 for lower,cap in [(0,1),(1,3),(2,3)]:
  e=R6();o,r,s=e.prepare(lower=lower,capacity=cap);seed=miss_seed(e)
  if lower==1:e.click(r,0);settled(e,s);stage=1
  else:stage=0
  e.seed(seed);e.click(r,stage);settled(e,s)
  check(e.gsui(596).slots[stage].outcome==int(lower==0),'final lifetime upper chance is MISS-free after domestic allocation '+str(lower))
  e.closeall()
 e=R6();o,r,s=e.prepare(lower=2,capacity=3);e.click(r,0);settled(e,s);button=get(r.addr+0x1b8);e.current_target=button;e.input.u32(0x34,202);e.input.u32(0x38,102);e.fn('hook_state_tick',s.addr)
 check(any(key==e.gsui(596).slots[0].result for owner,key in e.hover),'revealed native reward remains connected to existing Hover description renderer')
 e.closeall()

def original_db_and_entropy():
 e=R6();o,r,s=e.prepare(id=619);other=e.officer(1,(100,1,100,1,100),(),0);t=Target();a=Distribution();d=Distribution()
 e.fn('target_read',o.addr,C.addressof(t));e.fn('g_distribution',C.addressof(t),C.addressof(e.gs(619)),C.addressof(COLORS),0,0,C.addressof(a))
 edited=bytes([99,98,97,96,95]);o.put(0xec,edited);other.put(0xec,bytes([1,100,1,100,1]))
 e.fn('target_read',o.addr,C.addressof(t));e.fn('g_distribution',C.addressof(t),C.addressof(e.gs(619)),C.addressof(COLORS),0,0,C.addressof(d))
 check(list(t.stats)==[65,68,53,51,60] and bytes(a)==bytes(d) and o.get(0xec,5)==edited,'official target and teacher use original DB distribution without rewriting live edited stats')
 e.rng_fail=True;e.click(r,0)
 check(not e.mod.val('preview_count') and e.selection_counts(619)==(0,0) and not e.mod.val('ui_mode'),'initial entropy failure neither selects reward nor traps animation')
 e.rng_fail=False;e.click(r,0);settled(e,s);before=bytes(e.gsui(619));e.rng_fail=True;e.action(0)
 check(bytes(e.gsui(619))==before and not e.gs(619).rerolls_used and e.mod.val('ui_mode')==3,'failed reroll consumes no personal ticket and retains current result')
 e.rng_fail=False;e.action(4);text=e.detail().replace('\n','')
 check('65/68/53/51/60' in text and 'DB 원본' in text and '99/98/97/96/95' not in text,'calculation panel shows original DB inputs')
 e.bottom(s);check(e.pop_calls==1 and o.get(0xec,5)==edited,'Decision preserves edited live abilities exactly')
 custom=e.officer(5001,(92,14,88,18,50),(),1);e.fn('target_read',custom.addr,C.addressof(t))
 check(list(t.stats)==[92,14,88,18,50] and t.category==3,'custom officer without DB entry uses explicit current-base fallback')
 e.closeall()

def original_close_guard():
 e=R6();o,r,s=e.prepare();block=json.loads((ROOT/'analysis/r6_native_evidence.json').read_text())['blocks']['fn_6be850.asm']
 e.game.put(block['rva'],bytes.fromhex(block['bytes']));e.game.put(0x856090,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('NativeClose')))
 for off in (0x178,0x17c):
  e.transition.u32(off,1);before=e.close_calls;e.fn('hook_close',e.root_layout.addr,0x7fffffff)
  e.fn('choice_cancel_all',e.list_owner.addr)
  check(e.close_calls==before and e.transition.u32(off)==1,'original busy close guard is retained, never forcibly overwritten '+hex(off));e.transition.u32(off,0)
 e.click(r,0);settled(e,s);e.action(0);settled(e,s);e.request_back(s);e.answer_exit(s)
 check(e.close_calls==1 and e.pop_calls==1 and e.cancel_invocations==1 and not e.native_commits and not e.mod.val('preview_count'),'captured original close branch reaches one native fade after confirmed cancellation')
 e.closeall()

FUNCTIONS=[replacement_native_change,original_close_guard,original_db_and_entropy,basic,allocation_rules,cancel,cancel_modes_and_focus,multiple_officers,replacements,errors_and_original_auto,style_and_original_layers,warning_allocations_and_lifetime,fallback_input,last_chance_and_hover]
if __name__=='__main__':
 try:
  for f in FUNCTIONS:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r6_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R6',len(REPORT))
