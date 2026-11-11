"""R16/R17 exact machine-code comparisons; substituted OS/game boundaries.
No full-game turn, Windows scheduler, GPU or real filesystem latency claim.
"""
from test_rc4 import *
from test_r15_gacha import all_targets
from test_native_integration import EN, slot_roundtrip, interrupted, copies_and_corruption, flush_guard
from test_cp03 import rolling_logs, log_number_encoding
import test_native

BASELINE=ROOT/'audit/r16_release_baseline/SAN14_MeritTraits.dll'

def exact_distributions():
 e=E();old=Loaded(BASELINE,BR);e.mem.append(old)
 a=Distribution();d=Distribution();st=State();count=0
 for t in all_targets():
  for sex in (0,1):
   t.sex=sex;e.fn('g_init',C.addressof(st),C.addressof(t.traits))
   for anomaly,transcend in ((0,0),(1,0),(1,1)):
    args=(C.addressof(t),C.addressof(st),C.addressof(COLORS),anomaly,transcend)
    x=BR.call(old.sym('g_distribution'),*args,C.addressof(a));y=e.fn('g_distribution',*args,C.addressof(d))
    assert x==y and bytes(a)==bytes(d),(t.id,sex,anomaly,transcend)
    count+=1
 check(count==6528,'all 1,088 profiles, both sexes and three modes: complete distribution bytes equal R16')
 rng=random.Random(17)
 for case in range(256):
  t=Target(5001,(C.c_uint16*9)(*[rng.choice([0,65535,4,17,62,94,171,219,256,354,500]) for _ in range(9)]),
   (C.c_uint8*5)(*[rng.randrange(256) for _ in range(5)]),case%2,619,952,3,case%3)
  e.fn('g_init',C.addressof(st),C.addressof(t.traits))
  for k in range(9):
   if k%2:st.base_traits[k]=0
  st.budget=rng.choice([-6,.38,2,3]);st.heat=rng.choice([0,.16,.48]);st.passion=case%2
  args=(C.addressof(t),C.addressof(st),C.addressof(COLORS),case%2,0)
  x=BR.call(old.sym('g_distribution'),*args,C.addressof(a));y=e.fn('g_distribution',*args,C.addressof(d))
  assert x==y and bytes(a)==bytes(d),case
 check(True,'256 custom/duplicate/out-of-range/gained-trait inputs retain exact R16 arithmetic, budgets and heat')
 e.closeall()

class Rule(C.Structure):
 _fields_=[('condition_value',C.c_uint16),('amount',C.c_int16*2),('condition',C.c_uint8),
  ('effect',C.c_uint8*2),('linked',C.c_uint8),('present',C.c_uint8),('pad',C.c_uint8)]

def incremental_rules():
 e=E();old=Loaded(BASELINE,BR);e.mem.append(old);rules=(Rule*355)()
 for row in MASTER:
  i=row['trait_id'];raw=bytes.fromhex(row['raw_param_hex']);q=rules[i]
  q.present=1;q.condition=raw[0];q.condition_value=int.from_bytes(raw[2:4],'little')
  q.effect[:]=raw[4],raw[7];q.amount[:]=struct.unpack('<h',raw[5:7])[0],struct.unpack('<h',raw[8:10])[0]
 # All-empty first snapshot, followed by changed rows/columns, restore, no-op,
 # and a full DB change exercise both directions of the asymmetric rule.
 baseline=bytes(rules);rng=random.Random(1701)
 cases=[None,31,94,256,219,31,94,256,219,None]
 for index,key in enumerate(cases):
  if key is None:C.memmove(C.addressof(rules),baseline,len(baseline))
  else:
   q=rules[key];q.present=1;q.linked=1-q.linked;q.condition=rng.choice([0,21,22,23])
   q.condition_value=rng.choice([0,65535]);q.effect[:]=rng.choice([9,21,56]),rng.choice([0,83,84])
   q.amount[:]=rng.choice([-10,0,10]),rng.choice([-5,0,5])
  BR.call(old.sym('g_rules_update'),C.addressof(rules));e.fn('g_rules_update',C.addressof(rules))
  # Compare every ordered pair, including unmodified rows vs changed columns.
  for a in range(1,355):
   for b in range(1,355):
    assert BR.call(old.sym('g_blocks'),a,b)==e.fn('g_blocks',a,b),(index,a,b)
  revision=e.mod.val('g_rules_revision');e.fn('g_rules_update',C.addressof(rules));assert e.mod.val('g_rules_revision')==revision
 check(True,'ten live rule snapshots: all 125,316 ordered pairs equal full R16 rebuild; identical refresh does no rebuild')
 e.closeall()

def collect_log_fixture(dll):
 original=test_native.Loaded
 test_native.Loaded=lambda path,bridge=None,mapfile=None:original(dll if Path(path).name=='SAN14_MeritTraits.dll' else path,bridge,mapfile)
 try:
  e=E();o,r,s=e.prepare(top=1)
  e.fn('perform_roll',o.addr,0,0,0)
  for _ in range(5):assert e.fn('perform_roll',o.addr,0,1,0)==1
  assert e.fn('finish_result',o.addr,0,0,0)==1
  result=[queued_record(e,i) for i in range(e.mod.val('log_write'))]
  for row in result:row.pop('mod')
  e.closeall();return result
 finally:test_native.Loaded=original

def deferred_log():
 a=collect_log_fixture(BASELINE);b=collect_log_fixture(ROOT/'source/build/SAN14_MeritTraits.dll')
 check(a==b,'worker JSON fields, seeds, results and evaluation exactly match R16 across initial draw, five rerolls and confirm')
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0)
 expected=queued_record(e,0);assert get(e.mod.sym('log_kinds'),'<B')==1
 check(get(e.mod.sym('log_lengths'),'<I')<1024,'game callback enqueues bounded binary award data, not JSON text')
 # Clear every mutable world source before the worker sees the record.
 e.fn('clear_state');e.mod.val('g_rules_revision',999);e.mod.val('world_epoch',999,'<Q');e.mod.val('session_id',999,'<Q')
 C.memset(e.mod.sym('active_save_hash'),255,32);C.memset(o.addr,0,0x200)
 check(queued_record(e,0)==expected,'delayed formatting survives world reset without reading changed officer/state/hash/rule values')
 # Run the real worker until its next wait, capturing its real WriteFile output.
 waits=[];written=[];original_write=BR.callbacks['WriteFile'];original_create=BR.callbacks['CreateFileW']
 def create(p,access,share,sec,creation,flags,template):
  if creation==4:e.files.setdefault(wide(p),b'')
  return original_create(p,access,share,sec,creation,flags,template)
 BR.callbacks['CreateFileW']=create
 BR.callbacks['WaitForSingleObject']=lambda *a:waits.append(1) or int(len(waits)>1)
 def write(h,p,n,out,over):
  written.append(C.string_at(p,n));return original_write(h,p,n,out,over)
 BR.callbacks['WriteFile']=write
 assert e.fn('log_worker',0)==0
 check(e.mod.val('log_read')==e.mod.val('log_write') and [json.loads(x) for x in written]==[expected],
  'actual worker drains, formats and writes the captured award after reset')
 # Exercise the ring boundary and a producer arriving while the worker writes
 # its last item. The newly queued record must not be lost at the next wait.
 e.mod.val('log_read',124);e.mod.val('log_write',124);waits.clear();written.clear();wakes=[]
 BR.callbacks['SetEvent']=lambda *a:wakes.append(1) or 1
 event=C.create_string_buffer(b'ring');t=target();st=State()
 def emit():e.fn('record_event',C.addressof(event),C.addressof(t),C.addressof(st),C.addressof(st),0,0,1,0)
 for _ in range(8):emit()
 check(len(wakes)==1,'burst crossing queue wrap wakes the existing worker once while work remains queued')
 def write_and_produce(h,p,n,out,over):
  result=write(h,p,n,out,over)
  if len(written)==8:emit()
  return result
 BR.callbacks['WriteFile']=write_and_produce;assert e.fn('log_worker',0)==0
 check(len(written)==9 and len(wakes)==2 and e.mod.val('log_read')==133 and e.mod.val('log_write')==133,
  'worker drains wrapped queue and a new arrival during final write without losing its wake or record')
 # No timer/JSON/file work is attempted when the queue cannot accept a record.
 e.mod.val('log_read',0);e.mod.val('log_write',128)
 event=C.create_string_buffer(b'overflow');stamp=[]
 BR.callbacks['GetSystemTimeAsFileTime']=lambda *a:stamp.append(1)
 dropped=e.mod.val('log_dropped');seq=e.mod.val('event_sequence',fmt='<Q')
 e.fn('record_event',C.addressof(event),C.addressof(t),C.addressof(st),C.addressof(st),0,0,1,0)
 check(e.mod.val('log_dropped')==dropped+1 and not stamp and e.mod.val('event_sequence',fmt='<Q')==seq+1,
  'saturated queue returns before timestamp/snapshot work, preserving drop count and event sequence')
 e.mod.val('log_thread',0,'<Q');e.fn('record_event',C.addressof(event),C.addressof(t),C.addressof(st),C.addressof(st),0,0,1,0)
 check(not stamp and e.mod.val('log_dropped')==dropped+2,'missing worker remains nonblocking and never falls back to synchronous I/O')
 e.closeall();rolling_logs();log_number_encoding()

def save_digest():
 e=EN();o=e.officer(596,owned=(4,37,51,62,65),top=1);e.fn('hook_newgame',0,0)
 assert e.fn('perform_roll',o.addr,0,0,0)==1
 saved=bytes(e.gs(596));data=bytes(range(256))*8192+b'R17 save digest'
 check(e.save(data)==1,'two-MiB native save and companion complete through production journal')
 expected=hashlib.sha256(data).digest()
 check(C.string_at(e.mod.sym('active_save_hash'),32)==expected and bytes(e.ref()[1].native)==expected,
  'reused digest matches independent SHA-256 and committed slot reference')
 assert e.loadsave(data)==1
 check(bytes(e.gs(596))==saved and not e.mod.val('metadata_fault'),'save/load preserves pending outcome, seed and tickets byte-for-byte')
 e.native_ok=False;before=C.string_at(e.mod.sym('active_save_hash'),32);e.save(data+b'failed')
 check(C.string_at(e.mod.sym('active_save_hash'),32)==before and not e.mod.val('metadata_fault'),
  'failed native write cannot publish a new active digest; journal recovery protections remain')
 e.closeall()
 e=EN();e.officer(596,owned=(4,37,51,62,65),top=1);e.fn('hook_newgame',0,0)
 data=b'cloud native bytes R17';buf=e.alloc(len(data),data);prep=e.fn('hook_save_prep',0,0,0,0,0);assert prep
 vt=e.alloc(0x80);vt.u64(0,BR.addr('NativeWrite'));obj=e.alloc(0x30);obj.u64(0,vt.addr);name=e.alloc(16,b'save000.s14\0')
 assert e.fn('merit_write_steam',obj.addr,name.addr,buf.addr,buf.size,e.stream.addr)==1
 e.fn('hook_save_end',0,prep)
 check(C.string_at(e.mod.sym('active_save_hash'),32)==hashlib.sha256(data).digest() and bytes(e.ref()[1].native)==hashlib.sha256(data).digest(),
  'Steam write route publishes the same checked prepared digest as the file route')
 e.closeall()
 slot_roundtrip();interrupted();copies_and_corruption();flush_guard()

if __name__=='__main__':
 try:
  for fn in (exact_distributions,incremental_rules,deferred_log,save_digest):print('RUN',fn.__name__,flush=True);fn()
 finally:(ROOT/'audit/r17_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
