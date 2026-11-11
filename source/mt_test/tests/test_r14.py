"""R14 native-query contract/counter regressions on actual Win64 machine code.
OS and world endpoints are substituted; not live Windows or frame timing.
"""
from r11_fixture import *

def counters(e):
 p=e.mod.sym('SAN14_MeritPerfV1')
 magic,version,size=struct.unpack('<8sII',C.string_at(p,16))
 assert (magic,version,size)==(b'MTPERF14',1,128)
 return struct.unpack('<14Q',C.string_at(p+16,112))

def query_boundary_and_observer():
 e=EN();install_native_rules(e)
 o=e.officer(952);o.put(0xe6,b'\x04');o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<H',9999))
 lord=e.officer(596);lord.put(0xe6,b'\x01');force_ruler(e,596)
 e.fn('hook_newgame',0,0)
 before=counters(e);logs=len(e.logs);files=dict(e.files);tick=e.api_counts['timer'];rng=e.rng_calls
 for _ in range(100):
  for person in (o,lord):
   for name in ('hook_merit_eligible','hook_merit_current_rank','hook_merit_rank','hook_ai_visit'):e.fn(name,person.addr)
 after=counters(e)
 check(all(after[k]>before[k] for k in (0,1,2,3)),'exported data counters observe all four native query paths')
 check(after[4:7]==before[4:7],'warm native queries use zero readable callbacks, cold-person checks or rank-table rebuilds')
 check(e.api_counts['timer']==tick and e.rng_calls==rng and len(e.logs)==logs and e.files==files,'observation adds no timers, entropy, logs or file writes')
 old=BR.callbacks['Readable'];reads=[]
 BR.callbacks['Readable']=lambda p,n:reads.append((p,n)) or old(p,n)
 e.mod.val('metadata_ready',0);e.fn('hook_merit_current_rank',o.addr)
 check((o.addr,e.res.offsets[7]+1) in reads,'unprepared world retains checked officer boundary')
 e.mod.val('metadata_ready',1);reads.clear();e.fn('display_x_mask',0x1234)
 check(reads and reads[0][0]==0x1234,'external/helper pointer path still rejects unreadable memory through Core')
 # Exported display API must still resolve the LIVE table entry, not a native
 # query shortcut; removing an entry cannot leak prior cached officer data.
 e.db.u64(e.res.offsets[1]+952*8,0);a=e.alloc(8);bfr=e.alloc(8)
 result=BR.call(e.mod.sym('SAN14_MeritSlotDisplayV1'),952,0,a.addr,bfr.addr)
 check(result==0 and a.u32(0)==0 and bfr.u32(0)==0,'external display API retains live lookup and unavailable/zero-output contract')
 check(not BR.errors,'no native boundary callback errors')
 e.closeall()

if __name__=='__main__':
 try:query_boundary_and_observer()
 finally:(ROOT/'audit/r14_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R14',len(REPORT))
