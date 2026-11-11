"""R6 fixture uses the captured ORIGINAL 0x770080 button handler and state tick.
Only native status/row render, container lookup, widgets, OS, costs and fade are
substituted. The status model follows the captured native row-state switch.
"""
from test_r5 import R5
from test_r3 import *
class R6(R5):
 def __init__(self):
  super().__init__();self.capacities={};self.choice_nodes={};self.real_rows=[];self.selected_lower={};self.native_row_index=0;self.choice_reads=0
  BR.callbacks['ChoiceCounts']=self.counts_get;BR.callbacks['ChoiceRefresh']=self.refresh_choices;BR.callbacks['NativeLocate']=lambda a,b:self.native_row_index
  self.game.put(self.res.native.funcs[10],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('Sound')))
  sel=self.res.selection
  for site,expected in [(sel.counts_site,sel.counts_call),(sel.refresh_site,sel.refresh_call)]:self.game.put(site,bytes(expected[:5]))
  self.game.put(0x6dbdc0,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('ChoiceCounts')))
  self.game.put(0x781810,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('ChoiceRefresh')))
  self.game.put(0x92b610,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('NativeLocate')))
  evidence=json.loads((ROOT/'analysis/r6_native_evidence.json').read_text())
  block=evidence['blocks']['range_770080.asm'];self.game.put(block['rva'],bytes.fromhex(block['bytes']))
  self.game.put(self.res.funcs[19],bytes.fromhex('488b12488b4908e924aaecff'))
  for j in range(6):self.game.u32(0x14771e0+j*4,100+j)
 def selection_counts(self,id):
  n=self.choice_nodes.get(id)
  if n:return (b(n.addr+0x20)&15,b(n.addr+0x20)>>4)
  o=self.officers[id];return b(o.addr+0x1c2),b(o.addr+0x1c3)
 def counts_get(self,owner,out,id):
  assert self.list_owner is None or owner==self.list_owner.addr
  self.choice_reads+=1;a,b_=self.selection_counts(id);put(out,a|(b_<<8),'<H');return out
 def status(self,layout,person,out):
  if self.list_owner is not None:assert layout==self.list_owner.addr
  id=get(person+0x10,'<H');top,low=self.selection_counts(id);et=b(person+0x1c2);el=b(person+0x1c3);cap=self.capacities.get(id,min(3,max(1,et+el+1)))
  vals=[]
  for earned,selected in [(et,top),(el,low)]:
   vals += [3 if i>=cap else 2 if i<earned else 1 if i<selected or (i==selected and top+low<cap) else 0 for i in range(3)]
  if self.status_override:vals=self.status_override
  C.memmove(out,struct.pack('<6i',*vals),24)
 def row(self,o,layout=None):
  r=super().row(o,layout);id=get(o.addr+0x10,'<H');self.real_rows.append((id,r))
  for j in range(6):put(get(r.addr+0x1b8+8*j)+0x80,100+j,'<I')
  return r
 def install_tree(self):
  for id,o in self.officers.items():
   if id not in self.choice_nodes:
    n=self.alloc(0x30);n.u32(0x1c,id);n.u32(0x20,b(o.addr+0x1c2)|(b(o.addr+0x1c3)<<4));self.choice_nodes[id]=n
  head=self.alloc(0x30);head.put(0x19,b'\1')
  def tree(nodes,parent):
   if not nodes:return head.addr
   k=len(nodes)//2;n=nodes[k];n.u64(8,parent);n.u64(0,tree(nodes[:k],n.addr));n.u64(16,tree(nodes[k+1:],n.addr));return n.addr
  head.u64(8,tree([self.choice_nodes[i] for i in sorted(self.choice_nodes)],head.addr));self.list_owner.u64(0x198,head.addr)
  for i,(id,r) in enumerate(self.real_rows):self.list_owner.u64(0x1f8+i*8,r.addr)
 def refresh_choices(self,owner):
  assert owner==self.list_owner.addr
  self.selected_upper={i:self.selection_counts(i)[0] for i in self.choice_nodes}
  self.selected_lower={i:self.selection_counts(i)[1] for i in self.choice_nodes}
  self.decision_enabled=all(sum(self.selection_counts(i))==self.capacities.get(i,min(3,max(1,b(self.officers[i].addr+0x1c2)+b(self.officers[i].addr+0x1c3)+1))) for i in self.choice_nodes)
  for id,r in self.real_rows:self.fn('hook_row_refresh',r.addr)
  return 0
 def prepare(self,id=596,owned=(4,37,51,62,65),top=0,lower=0,capacity=1):
  self.capacities[id]=capacity;o,r,s=super().prepare(id,owned,top);o.put(0x1c3,bytes([lower]));self.install_tree();self.refresh_choices(self.list_owner.addr)
  self.state=s;return o,r,s
 def second(self,id=619,top=0,lower=0,capacity=1):
  self.capacities[id]=capacity;o,r=super().second(id,top);o.put(0x1c3,bytes([lower]));self.install_tree();self.refresh_choices(self.list_owner.addr);return o,r
 def click(self,row,j):
  self.native_row_index=next(i for i,(id,r) in enumerate(self.real_rows) if r.addr==row.addr)
  return super().click(row,j)
 def confirm(self,state):
  if not self.decision_enabled:return 0
  self.confirm_batch=self.batch([(id,*self.selection_counts(id)) for id in self.choice_nodes]);return EN.confirm(self,state)
 def bottom(self,s):self.root_layout.u32(0x180,1);self.fn('hook_state_tick',s.addr)
 def request_back(self,s):self.fn('hook_close',self.root_layout.addr,0x7fffffff);self.fn('hook_state_tick',s.addr)
 def raw_action(self,index):return R5.action(self,index)
 def action(self,index):
  self.raw_action(index)
  if hasattr(self,"state"):self.fn("hook_state_tick",self.state.addr)
 def selected_state(self,r,j):
  p=get(r.addr+0x1b8+8*j);return next((v for w,v in reversed(self.selected) if w==p),0)
