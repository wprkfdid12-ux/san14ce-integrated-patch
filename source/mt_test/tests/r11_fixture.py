"""Execute captured native rule/commit/fade instructions. OS, membership source,
widgets, scheduling/transition-manager endpoints remain synthetic, not live SAN14.
No manual dictionary is used to grant ruler capacity in NativeR11UI.
"""
from test_r6 import R6,settled
from test_native_integration import EN
from test_native import *
EVIDENCE=json.loads((ROOT/'analysis/r11_native_boundaries.json').read_text())['blocks']
def jump(e,rva,address):e.game.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',address))
def zero(e,rva):e.game.put(rva,b'\x31\xc0\xc3')
def native(e,name):
 d=EVIDENCE[name];e.game.put(d['rva'],bytes.fromhex(d['bytes']));return e.game.addr+d['rva']
def force_ruler(e,id,fid=1):
 force=e.alloc(0x100);force.put(e.res.native.offsets[5],struct.pack('<H',id));e.db.u64(e.res.native.offsets[3]+fid*8,force.addr);return force

def install_native_rules(e):
 for name in ('eligible','rank','capacity','ruler_status','current_rank','promotion','membership'):native(e,name)
 zero(e,0x232640);zero(e,0x229fc0);zero(e,0x2295c0)
 e.rank_entries=[]
 for i in range(7):
  entry=e.alloc(0x40);entry.put(0x2c,struct.pack('<H',i*100));entry.u32(0x30,int(i>=4));e.rank_entries.append(entry);e.db.u64(0x85db8+i*8,entry.addr)
 bonus=json.loads((ROOT/'analysis/r15_domestic_native.json').read_text())['blocks']['merit_bonus']
 old_bonus=e.game.get(bonus['rva'],bonus['end']-bonus['rva']);e.game.put(bonus['rva'],bytes.fromhex(bonus['bytes']))
 check(e.fn('rules_prepare_native')==1,'full native eligible/rank fingerprints accepted before hooks')
 e.game.put(bonus['rva'],old_bonus)
 saved=[]
 for h in e.res.hooks[:e.res.hook_count]:
  saved.append((h,e.game.get(h.rva,h.length)));e.game.put(h.rva,struct.pack('<Q',e.game.addr+h.target) if h.is_vtable else bytes(h.before[:h.length]))
 check(e.fn('prepare_hooks')==1,'production installer constructs all 39 hooks and original-entry trampolines')
 patches=(CorePatch*e.res.hook_count).from_address(e.mod.sym('installed'))
 for (h,old),patch in zip(saved,patches):e.game.put(h.rva,bytes(patch.after[:h.length]) if h.kind in (24,25,26,27) else old)
 check(e.fn('senior_rank')==6,'senior rank derived from three actual rank-table grants, not a hard-coded test capacity')

class NativeR11UI(R6):
 def setup_rules(self):
  install_native_rules(self);native(self,'row_status');zero(self,0x56e2d0)
  # Native original bottom refresh rendering is substituted; capacity itself is
  # the captured game 3fb80->3fae0 chain (not fixture.capacities).
 def refresh_choices(self,owner):
  if not self.mod.val('rules_native_ready'):return super().refresh_choices(owner)
  self.selected_upper={i:self.selection_counts(i)[0] for i in self.choice_nodes}
  self.selected_lower={i:self.selection_counts(i)[1] for i in self.choice_nodes}
  self.decision_enabled=all(sum(self.selection_counts(i))==BR.call(self.game.addr+0x3fb80,self.officers[i].addr) for i in self.choice_nodes)
  for id,row in self.real_rows:self.fn('hook_row_refresh',row.addr)
  return 0
 def setup_native_commit(self):
  native(self,'native_commit');native(self,'advance')
  self.city=self.alloc(0x100);self.force_cost=self.alloc(0x100);self.ruler_cost=self.alloc(0x100);vt=self.alloc(0x100)
  vt.u64(0x80,BR.addr('GetForce'));self.city.u64(0,vt.addr);self.force_cost.put(0x10,b'\x01');self.force_cost.put(0x14,b'\x64');self.db.u64(0xdf30+8,self.ruler_cost.addr)
  BR.callbacks.update({'GetForce':lambda c:self.force_cost.addr,'GetRuler':lambda c:self.ruler_cost.addr,'Policy':lambda o:0,'Pay':lambda c,n:1,'Extra':lambda:0})
  for rva,name in [(0x3ff60,'GetRuler'),(0x1ed4f0,'Policy'),(0x27210,'Pay'),(0x1d2f10,'Extra')]:jump(self,rva,BR.addr(name))
  self.game.u32(0x1a48848,300);self.game.u32(0x1a48854,1)
 def confirm(self,state):
  if not hasattr(self,'city'):return super().confirm(state)
  if not self.decision_enabled:return 0
  self.confirm_batch=self.batch([(i,*self.selection_counts(i)) for i in self.choice_nodes]);self.confirm_batch.u64(16,self.city.addr)
  return EN.confirm(self,state)

class FadeR11(R6):
 def strict_fade(self,state):
  native(self,'complete_fade');native(self,'state_exit')
  self.fade_events=[];self.cancel_invocations=0
  def disabled_cancel(group,kind):self.cancel_invocations+=1 # native disabled/missing button is a no-op
  def start_fade(inner,code):self.fade_events.append(code&0xffffffff);self.close_calls+=1;return 0
  def emit(main):return self.fn('hook_native_exit_event',state.addr)
  BR.callbacks['CancelButton']=disabled_cancel;BR.callbacks['NativeClose']=start_fade;BR.callbacks['UIExit']=emit
  # One owned original fade event, no destructive synthetic UI teardown.
  BR.callbacks['PopState']=lambda manager:setattr(self,'pop_calls',self.pop_calls+1) or 0
  jump(self,0x860e60,BR.addr('UIExit'));jump(self,0x10f20,BR.addr('Manager'));jump(self,0x12380,BR.addr('PopState'))
 def finish_original_fade(self):
  functor=self.alloc(16);functor.u64(8,self.root_layout.addr);event=self.alloc(0x88);event.u32(0x80,1);arg=self.alloc(8);arg.u64(0,event.addr)
  return BR.call(self.game.addr+0x7a8450,functor.addr,arg.addr)
