"""R12/R13 actual module + actual shipped Core logger at substituted Win32 I/O.
Counts boundary calls, not latency, live game FPS or actual disk performance.
"""
from r11_fixture import *
from test_rc4 import State
import os,collections,bisect
VERSION=os.environ.get('MERIT_BENCH_VERSION','R13')
DLL=Path(os.environ.get('MERIT_BENCH_DLL',str(ROOT/'source/build/SAN14_MeritTraits.dll'))).resolve()
import test_native
OriginalLoaded=test_native.Loaded
def load(path,bridge=None,mapfile=None):
 return OriginalLoaded(DLL if Path(path).name=='SAN14_MeritTraits.dll' else path,bridge,mapfile)
test_native.Loaded=load

def counted_reads(e):
 ranges=sorted((m.addr,m.addr+m.size) for m in e.mem);starts=[x[0] for x in ranges];counts=collections.Counter();table=e.db.addr+e.res.offsets[1]
 def readable(p,n):
  counts['readable']+=1
  if table<=p<table+6000*8:counts['officer_table_lookup']+=1
  k=bisect.bisect_right(starts,p)-1
  return bool(p and n and k>=0 and p+n<=ranges[k][1])
 BR.callbacks['Readable']=readable
 return counts

def commit_and_columns():
 e=EN();install_native_rules(e)
 for id in range(1,1001):
  o=e.officer(id,owned=tuple(range(4,13)));o.put(0xe6,b'\x04');o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<H',9999))
 e.fn('hook_newgame',0,0);batch=e.batch([(1,1,0)]);counts=counted_reads(e);rng=e.rng_calls
 snapshot=[o.get(0,0x200) for i,o in e.officers.items() if i!=1]
 assert e.fn('hook_commit_ai',batch.addr)==1
 assert [(b(e.officers[1].addr+x)) for x in (0x1c2,0x1c3)]==[0,1] and e.rng_calls==rng
 assert snapshot==[o.get(0,0x200) for i,o in e.officers.items() if i!=1]
 result={'single_officer_commit_1000_profiles':dict(counts)};counts.clear()
 gs=C.string_at(e.mod.sym('states'),6000*C.sizeof(State))
 values=[e.fn('display_x_mask',o.addr) for o in e.officers.values() for slot in range(9)]
 assert all(v==0 for v in values) and gs==C.string_at(e.mod.sym('states'),len(gs))
 result['x_free_9000_display_checks']=dict(counts);e.closeall();return result

def actual_core_logging():
 e=R6();o,row,state=e.prepare();corepath=ROOT/'payload/SAN14Mods/Core/SAN14_ModCore.dll'
 c=OriginalLoaded(corepath);e.mem.append(c)
 # Real Core 0x10a0 code, exact supported hash. Only the three Win32 I/O imports
 # and initialized filename are provided, as normal CoreStart would provide.
 assert hashlib.sha256(corepath.read_bytes()).hexdigest()=='f54c7d4848dacf0c2414b07c7573cdbac8bf3ca9b5aec84345d8f549b292fc67'
 c.put(0x6680,('C:\\CoreFixture.log\0').encode('utf-16le'))
 e.files['C:\\CoreFixture.log']=b''
 for loc,name in ((0x3048,'CreateFileW'),(0x3050,'WriteFile'),(0x3058,'CloseHandle')):c.u64(loc,BR.addr(name))
 e.core.log=c.addr+0x10a0;counts=collections.Counter()
 for name in ('CreateFileW','WriteFile','ReadFile','CloseHandle'):
  original=BR.callbacks[name]
  def wrapped(*args,name=name,original=original):counts[name]+=1;return original(*args)
  BR.callbacks[name]=wrapped
 # Start existing worker (synthetic thread endpoint); queue is deliberately not
 # drained here so any game-callback synchronous writes are observable.
 e.mod.val('log_thread',0,'<Q');BR.callbacks['CreateEventW']=lambda *a:123;BR.callbacks['CreateThread']=lambda *a:456
 assert e.fn('start_logger')==1
 counts.clear();e.mod.val('log_read',0);e.mod.val('log_write',0);e.mod.val('log_dropped',0)
 e.fn('hook_ui_enter',e.root_layout.addr)
 for i in range(20):
  r=e.row(o);e.fn('hook_row_destroy',r.addr,0)
 result={'gameplay_io':dict(counts),'queued_records':e.mod.val('log_write'),'dropped':e.mod.val('log_dropped')}
 if VERSION=='R13':
  assert not any(counts.values()) and result['queued_records']>=61
  # Queue overflow is lossy diagnostic only, never fallback to synchronous disk.
  for i in range(80):e.fn('hook_ui_enter',e.root_layout.addr)
  assert e.mod.val('log_write')==128 and e.mod.val('log_dropped')>0 and not any(counts.values())
  result['overflow_dropped']=e.mod.val('log_dropped')
  e.mod.val('log_thread',0,'<Q');e.fn('hook_ui_enter',e.root_layout.addr)
  assert not any(counts.values()) and e.mod.val('enabled')==1 and not e.mod.val('metadata_fault')
  result['worker_absent_gameplay_io']=dict(counts)
 e.closeall();return result

if __name__=='__main__':
 result={'version':VERSION,'scope':__doc__,'dll_sha256':hashlib.sha256(DLL.read_bytes()).hexdigest(),'workloads':commit_and_columns(),'actual_shipped_core_logger':actual_core_logging()}
 p=ROOT/'audit'/('r13_ui_workload_'+VERSION.lower()+'.json');p.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
