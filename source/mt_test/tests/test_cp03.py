"""CP03 regression tests: execute the newly compiled Win64 DLL, not a Python clone.
OS/game operations are mocked. No Windows, actual game FPS or live rendering claim.
"""
from test_rc4 import *
import copy


def records(e):
 out=[]
 for n in range(e.mod.val('log_read'),e.mod.val('log_write')):
  out.append(queued_record(e,n%128))
 return out

def load_bytes(e,data):
 e.load_buf=e.alloc(len(data),data);return e.fn('hook_load',0,0,0)

def finish(e,r,s):
 o=get(r.addr+0x178);e.fn('finish_result',o,0,0,0)


def engine_integrity():
 e=E();t=target();s=State();d=Distribution();e.fn('g_init',C.addressof(s),C.addressof(t.traits))
 check(e.fn('g_validate',C.addressof(s))==1,'fresh state passes semantic validation')
 check(call_roll(e,t,s,2,999,0,0,d)==1,'an already unlocked third chance can be chosen first')
 before=bytes(s);check(call_roll(e,t,s,0,1000,0,0,d)==-5 and bytes(s)==before,'reverse-order second pending roll rejected atomically')
 check(e.fn('g_validate',C.addressof(s))==1,'single pending state passes validation')
 for label,mutate in [
  ('NaN cost',lambda x:setattr(x.slots[2],'cost',float('nan'))),
  ('infinite weight',lambda x:setattr(x.slots[2],'weight',float('inf'))),
  ('negative fit',lambda x:setattr(x.slots[2],'quality',-1.0)),
  ('uninitialized with payload',lambda x:setattr(x,'initialized',0)),
  ('reroll sum mismatch',lambda x:setattr(x,'rerolls_used',1)),
  ('nonboolean star',lambda x:setattr(x.slots[2],'star',2)),
  ('budget mismatch',lambda x:setattr(x,'budget',2.0)),
  ('transcend without anomaly',lambda x:(setattr(x.slots[2],'transcend',1),setattr(x.slots[2],'anomaly',0))),
  ('invalid original trait',lambda x:x.base_traits.__setitem__(0,500)),
  ('two pending results',lambda x:C.memmove(C.addressof(x.slots[0]),C.addressof(x.slots[2]),C.sizeof(Slot))),
 ]:
  bad=State.from_buffer_copy(before);mutate(bad)
  check(e.fn('g_validate',C.addressof(bad))==0,'semantic validator rejects '+label)
 e.closeall()


def rng_failures_and_fault():
 from r2_fixture import R2
 e=R2();o,r,s=e.prepare(top=1);before=bytes(e.gs(596));e.rng_fail=True;e.click(r,0)
 check(bytes(e.gs(596))==before and e.mod.val('roll_sequence',fmt='<Q')==0,'initial entropy failure preserves chance and counters')
 check(e.mod.val('ui_mode')==0,'entropy failure does not leave a spinning UI')
 e.rng_fail=False;e.click(r,0);e.reveal(s);before=bytes(e.gsui(596));owned=e.owned(o);e.rng_fail=True;e.action(0)
 check(bytes(e.gsui(596))==before and e.owned(o)==owned and e.mod.val('ui_mode')==3,'reroll entropy failure preserves preview and remaining rerolls')
 e.rng_fail=False;e.mod.val('metadata_fault',1)
 check(e.fn('perform_roll',o.addr,0,1,0)==0 and bytes(e.gsui(596))==before,'metadata fault rejects direct reroll')
 check(e.fn('finish_result',o.addr,0,0,0)==0 and e.owned(o)==owned,'metadata fault rejects finalization')
 e.closeall()
 e=E();o,r,s=e.prepare();e.confirm_batch=e.batch([(596,1,0)]);before=o.get(0,0x200);e.rng_fail=True;e.fn('hook_confirm',s.addr)
 check(e.native_commits==0 and o.get(0,0x200)==before,'preflight entropy failure prevents native fee/rank commit')
 e.rng_fail=False;e.fail_commit=True;e.fn('hook_confirm',s.addr)
 check(e.gs(596).slots[0].phase==0 and o.get(0,0x200)==before,'native commit failure creates no trait result')
 e.closeall()


def migration():
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);finish(e,r,s)
 result=e.gs(596).slots[0].result;native=b'legacy-migration-fixture';e.save(native)
 key=next(k for k in e.files if k.endswith('.mt4dat'));full=e.files.pop(key)
 legacy_size=88+6000*C.sizeof(Profile);v1=bytearray(full[:legacy_size]);struct.pack_into('<I',v1,8,1);v1[56:88]=hashlib.sha256(v1[88:]).digest();legacy=key[:-7]+'.mtdat';e.files[legacy]=bytes(v1)
 load_bytes(e,native);state=e.gs(596)
 check(not e.mod.val('metadata_fault') and state.slots[0].phase==2 and state.slots[0].result==result,'RC3 .mtdat reconstructs the accepted RC4 result')
 check(e.fn('g_validate',C.addressof(state))==1 and state.rerolls_used==0,'legacy migration reconstructs coherent budget and zero rerolls')
 check(e.fn('hook_bonus',o.addr,0)==0,'legacy migrated original troop bonus remains removed')
 rng=e.rng_calls;e.fn('hook_row_refresh',r.addr)
 check(e.rng_calls==rng and state.slots[0].phase==2,'migration does not reroll an already acquired result')
 check(e.save(native)==1 and key in e.files and e.files[legacy]==bytes(v1),'safe RC3 to RC4 migration creates separate file and preserves legacy bytes')
 e.closeall()


def save_atomicity():
 e=E();o,r,s=e.prepare(top=1);data=b'unchanged-native-stream';e.save(data);key=next(k for k in e.files if k.endswith('.mt4dat'));old=e.files[key]
 check(e.save(data)==1 and e.files[key]==old,'identical native and companion snapshots may be saved again')
 e.fn('perform_roll',o.addr,0,0,0);state=bytes(e.gs(596));writes=e.native_writes
 check(e.save(data)==1 and e.native_writes==writes+1 and e.files[key]==old and e.mod.val('sidecar_collision')==1,'R40: same-digest metadata collision never overwrites the old companion and never blocks the native save')
 check(bytes(e.gs(596))==state,'collision does not mutate current pending roll')
 check(e.save(b'different-native-stream')==1,'distinct native digest has independent companion snapshot')
 for label,field in [('write','fail_write'),('flush','fail_flush'),('rename','fail_move')]:
  setattr(e,field,True);n=e.native_writes
  check(e.save(('failure-'+label).encode())==1 and e.native_writes==n+1,'R40: sidecar '+label+' failure never blocks the native save (companion skipped, logged)')
  setattr(e,field,False)
 check(not any(k.endswith('.mt4tmp') for k in e.files),'failed sidecar writes leave no temporary file')
 expected=hashlib.sha256(b'different-native-stream').digest()
 check(C.string_at(e.mod.sym('active_save_hash'),32)==expected,'failed save cannot change active-save diagnostic identity')
 a=e.fn('hook_save_prep',0,0,0,0,0)
 check(a and e.fn('hook_save_prep',0,0,0,0,0)==0,'overlapping saves cannot replace in-flight snapshot')
 e.fn('hook_save_end',0,a);check(e.mod.val('save_active')==0,'native save end releases single-save guard')
 oldcb=BR.callbacks['SavePrep'];BR.callbacks['SavePrep']=lambda *a:0
 check(e.fn('hook_save_prep',0,0,0,0,0)==0 and e.mod.val('save_active')==0,'failed native save preparation releases guard')
 BR.callbacks['SavePrep']=oldcb
 e.fn('hook_newgame',0,0);check(e.mod.val('active_save_hash_ok')==0,'new scenario does not inherit prior save hash in logs')
 e.closeall()


def semantic_file_damage():
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'checksummed-state';e.save(native);key=next(k for k in e.files if k.endswith('.mt4dat'));original=e.files[key]
 off=88+6000*C.sizeof(Profile)+596*C.sizeof(State)
 for label,mutate in [('NaN cost',lambda x:setattr(x.slots[0],'cost',float('nan'))),('reroll mismatch',lambda x:setattr(x,'rerolls_used',4))]:
  raw=bytearray(original);st=State.from_buffer_copy(raw[off:off+C.sizeof(State)]);mutate(st);raw[off:off+C.sizeof(State)]=bytes(st);raw[56:88]=hashlib.sha256(raw[88:]).digest();e.files[key]=bytes(raw);load_bytes(e,native)
  check(e.mod.val('metadata_fault')==0 and e.gs(596).slots[0].phase!=1,'R40: checksum-valid file with '+label+' -> only that officer starts fresh, no module fault')
 raw=bytearray(original);raw[88+596*C.sizeof(Profile)+Profile.award_mask.offset]=1;raw[56:88]=hashlib.sha256(raw[88:]).digest();e.files[key]=bytes(raw);load_bytes(e,native)
 check(e.mod.val('metadata_fault')==0 and e.gs(596).slots[0].phase!=1,'R40: contradicting award bit -> that officer starts fresh, no module fault')
 e.files[key]=original;o.put(0x1c2,b'\0');load_bytes(e,native)
 check(e.mod.val('metadata_fault')==1,'pending roll cannot exist without its actual native unlock flag')
 e.closeall()


def finalized_history_survives_rank_drop():
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);finish(e,r,s)
 state=e.gs(596);result=state.slots[0].result;rerolls=state.rerolls_used;native=b'finalized-rank-drop'
 check(state.slots[0].phase==2 and e.profile(596).award_mask&1,'fixture has a finalized historical upper reward')
 check(e.save(native)==1,'finalized historical reward saves before status transition')
 o.put(0x1c2,b'\0');load_bytes(e,native);state=e.gs(596);profile=e.profile(596)
 check(e.mod.val('metadata_fault')==0,'lower current native upper count does not invalidate finalized historical metadata')
 check(state.slots[0].phase==2 and state.slots[0].result==result and profile.award_mask&1 and profile.awards[0]==result,'finalized trait result and award history survive status transition')
 check(state.rerolls_used==rerolls and e.fn('g_validate',C.addressof(state))==1,'personal reroll budget remains coherent after status transition load')
 check(e.save(native)==1,'status transition with finalized historical metadata does not block later saves')
 e.closeall()


def ui_lifetime_and_sequence():
 from r2_fixture import R2
 e=R2();o,r,s=e.prepare(top=1);e.click(r,0);seen=e.mod.val('visible_row_count');destroyed=[];BR.callbacks['RowDestroy']=lambda p,ctx:destroyed.append(p)
 e.fn('hook_row_destroy',r.addr,e.resources.addr);check(seen==1 and e.mod.val('visible_row_count')==0 and destroyed==[r.addr],'native row cleanup evicts cached row before original cleanup')
 n=len(e.text_history);e.tick+=50;e.fn('hook_state_tick',s.addr)
 check(len(e.text_history)==n,'destroyed row receives no later animation text writes')
 e.closeall()
 e=R2();o1,r1,s=e.prepare(596,top=3);o2=e.officer(1,(60,60,60,60,60),(4,5,6,7,8),2);r2=e.row(o2,e.active_layout);e.fn('hook_row_refresh',r2.addr)
 for row,stages in [(r1,[0,1,2]),(r2,[0,1])]:
  for stage in stages:e.click(row,stage);e.reveal(s);e.accept_preview()
 e.bottom(s)
 check([x.phase for x in e.gs(596).slots]==[2,2,2] and [x.phase for x in e.gs(1).slots]==[2,2,0],'two officers and five historical unlocked stages finish in one bottom decision')
 check(e.native_commits==0 and len(set(x.result for x in e.gs(596).slots))==3,'historical batch avoids second merit cost and duplicate traits')
 check(e.pop_calls==1 and len(e.talks)==2 and e.mod.val('ui_mode')==0,'one dialogue per officer and one normal return after batch')
 e.closeall()


def reentrant_guard_and_ai():
 e=E();o,r,s=e.prepare(top=1);attempt=[];locks=set()
 def lock(p):
  if p in locks:raise AssertionError('recursive SRW lock')
  locks.add(p)
 def unlock(p):
  if p not in locks:raise AssertionError('unmatched unlock')
  locks.remove(p)
 BR.callbacks['AcquireSRWLockExclusive']=lock;BR.callbacks['ReleaseSRWLockExclusive']=unlock
 def on_time(p):
  put(p,133000000000000000)
  attempt.append(e.fn('hook_save_prep',0,0,0,0,0))
  attempt.append(e.fn('perform_roll',o.addr,0,1,0))
 BR.callbacks['GetSystemTimeAsFileTime']=on_time;e.fn('perform_roll',o.addr,0,0,0)
 check(attempt==[0,0] and not locks,'reentrant save and reroll rejected inside locked draw')
 BR.callbacks['GetSystemTimeAsFileTime']=lambda p:put(p,133000000000000000)
 finish(e,r,s);check(not locks and e.gs(596).slots[0].phase==2,'finalization has balanced locks')
 e.closeall()
 e=E();o=e.officer(1,(60,60,60,60,60),(4,5,6,7,8));e.fn('hook_newgame',0,0);batch=e.batch([(1,3,0)])
 check(e.fn('hook_commit_ai',batch.addr)==1 and all(x.phase==2 for x in e.gs(1).slots),'AI completes three lifetime chances with same production engine')
 check(e.gs(1).rerolls_used<=5 and e.fn('g_validate',C.addressof(e.gs(1)))==1,'AI never exceeds shared five rerolls and persists coherent budget')
 check(all(x['ai']==1 for x in records(e)),'AI event records retain AI attribution')
 e.closeall()


def initializer_atomicity():
 e=Env();calls=[]
 old=BR.callbacks['BCryptGenRandom']
 def fail_second(*a):
  calls.append(1)
  return 0xc0000001 if len(calls)==2 else old(*a)
 BR.callbacks['BCryptGenRandom']=fail_second
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0 and not e.patch_calls,'second half of initial 64-bit RNG failure installs no hooks')
 e.closeall()
 e=Env();old={h.rva:e.game.get(h.rva,h.length) for h in e.res.hooks if h.length};orig=e.patch
 def partial(name,p,n):
  if C.string_at(name)==b'SAN14CE.MeritTraits' and len([x for x in e.patch_calls if x[0]=='SAN14CE.MeritTraits'])==1:
   e.patch_calls.append(('SAN14CE.MeritTraits',n));items=(CorePatch*n).from_address(p)
   for item in items[:3]:C.memmove(item.address,bytes(item.after[:item.length]),item.length)
   return 0
  return orig(name,p,n)
 BR.callbacks['Patch']=partial
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0,'injected partial current patch batch fails initializer')
 check(all(e.game.get(r,len(v))==v for r,v in old.items()) and e.mod.val('enabled')==0,'rollback includes partially applied current batch, not just earlier batch')
 e.closeall()
 e=Env();check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1,'exact-build initialization tolerates unavailable optional logger')
 check(e.mod.val('clock_frequency',fmt='<Q')==1000000,'high-resolution clock frequency cached once at initialization')
 calls=len(e.patch_calls);check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1 and len(e.patch_calls)==calls,'repeated successful initialization never patches again')
 e.closeall()


def rolling_logs():
 e=E();e.prepare();append_handles=set()
 orig_create=e.create;orig_write=e.write
 def create(p,access,share,sec,creation,flags,template):
  name=wide(p);creation &= 0xffffffff;access &= 0xffffffff
  if creation==4:e.files.setdefault(name,b'')
  h=orig_create(p,access,share,sec,creation,flags,template)
  if access==4:append_handles.add(h)
  return h
 def write(h,p,n,out,over):
  if e.fail_write:return 0
  if h in append_handles:
   name=e.handles[h][0];e.files[name]+=C.string_at(p,n);put(out,n,'<I');return 1
  return orig_write(h,p,n,out,over)
 def move(p,q,flags):
  a,z=wide(p),wide(q)
  if e.fail_move:e.last_error=5;return 0
  if a not in e.files:e.last_error=2;return 0
  e.files[z]=e.files.pop(a);return 1
 BR.callbacks.update(CreateFileW=create,WriteFile=write,MoveFileExW=move)
 paths=[]
 for i in range(4):
  a=e.alloc(2048);e.fn('log_path',a.addr,i);paths.append(wide(a.addr))
 check(len(set(paths))==4 and all(x.endswith(f'events.{i}.jsonl') for i,x in enumerate(paths)),'all four log paths have correct distinct suffixes')
 cap=2*1024*1024;line=C.create_string_buffer(b'{"event":"test"}\n');n=len(line.value)
 for p in paths:e.files[p]=b'x'*cap
 check(e.fn('log_append',C.addressof(line),n)==1 and e.files[paths[0]]==line.value,'full log rotates and appends to fresh file')
 check(all(len(e.files[p])<=cap for p in paths) and sum(len(e.files[p]) for p in paths)<=4*cap,'rolling log storage stays within four times two MiB')
 e.files[paths[0]]=b'x'*cap;e.fail_move=True;before=len(e.files[paths[0]])
 check(e.fn('log_append',C.addressof(line),n)==0 and len(e.files[paths[0]])==before,'failed rotation never appends past file cap')
 check(e.mod.val('log_io_errors')>0,'failed log I/O is counted without gameplay failure')
 e.closeall()
 e=E();o,r,s=e.prepare(top=1);e.mod.val('log_write',128);before=bytes(e.gs(596));e.fn('perform_roll',o.addr,0,0,0)
 check(e.gs(596).slots[0].phase==1 and e.mod.val('log_write')==128 and e.mod.val('log_dropped')==1,'full 128-event queue drops diagnostics rather than blocking gacha')
 e.closeall()


def log_number_encoding():
 e=E();o,r,state=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);t=target();before=State.from_buffer_copy(bytes(e.gs(596)));after=State.from_buffer_copy(bytes(before));event=C.create_string_buffer(b"format_test")
 values=(2.99999999999999,0.9999999999999999,1.00000001,-0.999999999999,0.0,3.0)
 for v in values:
  after.budget=v
  e.fn('record_event',C.addressof(event),C.addressof(t),C.addressof(before),C.addressof(after),0,0,0,0)
 output=[x for x in records(e) if x['event']=='format_test']
 check(len(output)==len(values) and all(abs(x['budget_after']-v)<1e-6 for x,v in zip(output,values)),'float rounding carry produces valid JSON numbers near integer boundaries')
 check(all(x['mod']=='RC4-R17-TurnTransitionPerf' and 'log_io_errors' in x for x in output),'event records identify current R17 build and log I/O failures')
 e.closeall()


if __name__=='__main__':
 try:
  for test in (engine_integrity,rng_failures_and_fault,migration,save_atomicity,semantic_file_damage,finalized_history_survives_rank_drop,ui_lifetime_and_sequence,reentrant_guard_and_ai,initializer_atomicity,rolling_logs,log_number_encoding):
   print('RUN',test.__name__,flush=True);test()
 finally:
  (ROOT/'audit/cp03_tests.json').write_text(json.dumps({'scope':'Rebuilt Win64 PE executed on Linux through Microsoft x64 ABI; OS/game mocked; not Windows/live game.','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('COMPLETE',len(REPORT))
