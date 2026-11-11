from test_native import *
class Slot(C.Structure):
 _fields_=[('roll_id',C.c_uint64),('seed',C.c_uint64),('result',C.c_uint16),('replace_trait',C.c_uint16),('phase',C.c_uint8),('rerolls',C.c_uint8),('anomaly',C.c_uint8),('transcend',C.c_uint8),('full',C.c_uint8),('star',C.c_uint8),('outcome',C.c_uint8),('passion_applied',C.c_uint8),('x_index',C.c_uint8),('pad',C.c_uint8*3),('raw',C.c_double),('weight',C.c_double),('quality',C.c_double),('cost',C.c_double)]
class State(C.Structure):
 _fields_=[('base_traits',C.c_uint16*9),('initialized',C.c_uint8),('rerolls_used',C.c_uint8),('x_mask',C.c_uint16),('passion',C.c_uint8),('cancelled_rerolls',C.c_uint8),('budget',C.c_double),('heat',C.c_double),('slots',Slot*3)]
class Target(C.Structure):
 _fields_=[('id',C.c_uint16),('traits',C.c_uint16*9),('stats',C.c_uint8*5),('star',C.c_uint8),('father',C.c_uint16),('mother',C.c_uint16),('category',C.c_uint8),('sex',C.c_uint8)]
class Score(C.Structure):_fields_=[('id',C.c_uint16),('score',C.c_double)]
class Distribution(C.Structure):
 _fields_=[('similar',Score*8),('candidates',Score*8),('raw',C.c_double*355),('weight',C.c_double*355),('quality',C.c_double*355),('family',C.c_uint16*6),('family_count',C.c_uint32),('count',C.c_uint32),('total',C.c_double),('anomaly',C.c_uint8),('transcend',C.c_uint8),('family_weight',C.c_double*6),('similar_stats',(C.c_uint8*5)*8)]
MASTER=json.loads((ROOT/'data/SAN14CE_개성마스터219_DB.json').read_text())
PEOPLE=json.loads((ROOT/'data/SAN14CE_공식무장1000_DB.json').read_text())
COLORS=(C.c_uint8*355)()
for x in MASTER:COLORS[x['trait_id']]=x['category_code'] if x['category_code'] in (1,3) else 0
class E(Env):
 def __init__(self):
  super().__init__();self.activate();self.mod.val('log_thread',1,'<Q')
  # R2 regression cases explicitly exercise ordinary outcomes. R3 tests below
  # choose MISS seeds separately; no production probability is overridden.
  self.ordinary_roll_fixture=True
  BR.callbacks['GetSystemTimeAsFileTime']=lambda p:put(p,133000000000000000)
 def gs(self,id):return State.from_address(self.mod.sym('states')+id*C.sizeof(State))
 def click(self,r,j):
  functor=self.alloc(16);functor.u64(8,get(r.addr+8));arg=self.alloc(8);arg.u64(0,get(r.addr+0x1b8+8*j));self.fn('hook_button_event',functor.addr,arg.addr)
 def prepare(self,id=596,owned=(4,37,51,62,65),top=0):
  p=PEOPLE[id-1];o=self.officer(id,tuple(p[k] for k in ('leadership','war','intelligence','politics','charisma')),owned,top)
  self.fn('hook_newgame',0,0);r=self.row(o);s=self.alloc(0x600);s.u64(0x4c8,self.active_layout.addr);self.fn('hook_row_refresh',r.addr);self.fn('hook_state_tick',s.addr);return o,r,s

def queued_record(e,i):
 length=get(e.mod.sym('log_lengths')+i*4,'<I');item=e.mod.sym('log_queue')+i*8192
 try:kind=get(e.mod.sym('log_kinds')+i,'<B')
 except (KeyError,AssertionError):return json.loads(C.string_at(item,length))
 out=C.create_string_buffer(8192);n=e.fn('format_log_item',kind,item,length,C.addressof(out))
 return json.loads(out.raw[:n])

def target(id=596):
 p=PEOPLE[id-1];return Target(id,(C.c_uint16*9)(*p['trait_slots']),(C.c_uint8*5)(*[p[k] for k in ('leadership','war','intelligence','politics','charisma')]),int(p['star_unique_tactic']),p['father_id'],p['mother_id'],1)

def engine():
 e=E();t=target();st=State();d=Distribution();e.fn('g_init',C.addressof(st),C.addressof(t.traits));
 def draw(stage,seed,reroll=0,inherit=0):
  # g_roll has 9 args; use a native-ABI wrapper for the last argument.
  return call_roll(e,t,st,stage,seed,reroll,inherit,d)
 check(draw(0,100)==1,'official database distribution and draw execute in production Win64 PE')
 check(st.slots[0].phase==1 and st.budget==3 and st.heat==0,'pending result has no budget or heat consumption')
 old=st.slots[0].result;rid=st.slots[0].roll_id;raw=list(d.weight)
 for i in range(5):check(draw(0,100,1)==1 and st.rerolls_used==i+1 and st.slots[0].roll_id==rid,'shared reroll persisted '+str(i+1))
 check(st.slots[0].result==old and list(d.weight)==raw and st.heat==0 and st.budget==3,'same result allowed; reroll has no probability/budget/heat bonus')
 snapshot=bytes(st);check(draw(0,3,1)&0xffffffff==0xfffffffc and bytes(st)==snapshot,'sixth reroll rejected without mutations')
 check(e.fn('g_accept',C.addressof(t),C.addressof(st),0,0,0)==1,'final accept inserts actual trait')
 check(old in t.traits and st.slots[0].phase==2 and st.budget<3,'accepted result consumes personal budget only once')
 snapshot=bytes(st);check(e.fn('g_accept',C.addressof(t),C.addressof(st),0,0,0)&0xffffffff==0xffffffff and bytes(st)==snapshot,'duplicate finalize rejected')
 check(draw(1,123)==1 and st.rerolls_used==5,'next stage shares exhausted reroll count')
 check(all(not d.weight[i] for i in range(355) if COLORS[i] not in (1,3)),'red/hidden/uninstalled traits excluded')
 before=list(d.raw);t.id=5001;e.fn('g_distribution',C.addressof(t),C.addressof(st),C.addressof(COLORS),0,0,C.addressof(d));
 check(d.count>0,'custom officer remains a valid target without being added to teacher pool')
 e.closeall()

def call_roll(e,t,st,stage,seed,reroll,inherit,d):
 # Explicit nine-argument Microsoft x64 ABI call, no reimplemented algorithm.
 if not hasattr(call_roll,'lib'):
  src=ROOT/'tests/roll_bridge.c';src.write_text('#include <stdint.h>\nuint64_t callroll(uint64_t f, uint64_t t,uint64_t s,uint64_t stage,uint64_t colors,uint64_t seed,uint64_t rid,uint64_t inherit,uint64_t reroll,uint64_t dist){return ((int(__attribute__((ms_abi))*)(void*,void*,uint32_t,void*,uint64_t,uint64_t,uint16_t,int,void*))f)((void*)t,(void*)s,stage,(void*)colors,seed,rid,inherit,reroll,(void*)dist);}')
  so=src.with_suffix('.so');subprocess.run(['gcc','-O2','-shared','-fPIC',str(src),'-o',str(so)],check=True);call_roll.lib=C.CDLL(str(so));call_roll.lib.callroll.argtypes=[C.c_uint64]*10;call_roll.lib.callroll.restype=C.c_int
 return call_roll.lib.callroll(e.mod.sym('g_roll'),C.addressof(t),C.addressof(st),stage,C.addressof(COLORS),seed,789,inherit,reroll,C.addressof(d))

def persistence():
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);e.fn('perform_roll',o.addr,0,1,0)
 state=bytes(e.gs(596));e.save(b'exact native compressed save test');names=[x for x in e.files if x.endswith('.mt4dat')];check(len(names)==1,'new sidecar uses separate version extension')
 data=e.files[names[0]];check(struct.unpack_from('<I',data,8)[0]==3,'V3 wire format includes MISS/X/passion in formerly reserved padding')
 e.mod.val('pending_ui',0,'<Q');e.load_buf=e.alloc(len(b'exact native compressed save test'),b'exact native compressed save test');e.fn('hook_load',0,0,0)
 check(bytes(e.gs(596))==state,'pending result, seed, budget and used reroll restored exactly')
 e.files[names[0]]=data[:-1]+bytes([data[-1]^1]);e.fn('hook_load',0,0,0);check(e.mod.val('metadata_fault')==0,'R40: corrupted sidecar -> fresh merit start, no module-wide fault')
 e.closeall()

def logs():
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);e.fn('finish_result',o.addr,0,0,0)
 n=e.mod.val('log_write');check(n==2,'only unlock and confirmation events queued')
 rec=[]
 for i in range(n):
  rec.append(queued_record(e,i))
 check(rec[0]['roll_id']==rec[1]['roll_id'] and rec[0]['seed']==rec[1]['seed'],'log replay seed and Roll ID correlate finalization')
 check(rec[0]['similar'] and rec[0]['top_candidates'] and rec[1]['budget_after']<rec[1]['budget_before'],'structured audit contains calculation basis and budget transition')
 for _ in range(1000):e.fn('hook_row_tick',r.addr)
 check(e.mod.val('log_write')==n,'idle row ticks generate no log records')
 (ROOT/'audit/rc4_event_samples.json').write_text(json.dumps(rec,ensure_ascii=False,indent=2))
 e.closeall()

if __name__=='__main__':
 try:
  for f in [engine,persistence,logs]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/rc4_tests.json').write_text(json.dumps({'scope':'Actual compiled Win64 PE under substituted OS/game primitives; not Windows or live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('COMPLETE',len(REPORT))
