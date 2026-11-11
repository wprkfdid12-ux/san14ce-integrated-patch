"""Execute shipped PE + captured native bonus/selection/commit code.
Native OS, factions and rendering endpoints are fixtures, not a live game run.
"""
from r11_fixture import *
import os
R15=json.loads((ROOT/'analysis/r15_domestic_native.json').read_text())['blocks']

def domestic_table(e):
 for j,(kind,amount) in enumerate(((2,1),(2,1),(3,1)),4):
  obj=e.alloc(32);obj.put(0x10,bytes([kind]));obj.put(0x12,struct.pack('<H',amount));e.db.u64(e.res.offsets[17]+j*8,obj.addr)

def real_bonus(e):
 d=R15['merit_bonus'];e.game.put(d['rva'],bytes.fromhex(d['bytes']))
 zero(e,0x2320b0);zero(e,0x232640)

def matrix():
 e=EN();install_native_rules(e);domestic_table(e);real_bonus(e)
 # All 1,088 DB identities plus the highest supported custom officer identity.
 ids=list(range(1,1089))+[5999]
 for id in ids:
  o=e.officer(id);o.put(0xe6,b'\x01');o.put(0x1bc,b'\x06')
 e.fn('hook_newgame',0,0);rng=e.rng_calls
 seen=0
 for id in ids:
  o=e.officers[id];state=bytes(e.gs(id));owned=e.owned(o)
  for top in range(4):
   for lower in range(4-top):
    o.put(0x1c2,bytes([top,lower]))
    for status in (1,4):
     o.put(0xe6,bytes([status]))
     original=[BR.call(e.game.addr+0x2a9010,o.addr,k) for k in range(4)]
     got=[e.fn('hook_bonus',o.addr,k) for k in range(4)]
     assert original[3]==(0 if status==1 else int(lower==3)),(id,status,top,lower,original)
     assert got==[0,0,2 if status==1 else min(lower,2),int(lower==3)],(id,status,top,lower,got)
     seen+=1
  assert bytes(e.gs(id))==state and e.owned(o)==owned
 check(seen==21780,'all 1,088 officer IDs and custom 5999: ruler/non-ruler, all ten upper/lower combinations, all four bonus kinds',{'combinations':seen})
 check(e.rng_calls==rng,'all domestic bonus queries leave entropy, gacha states, reroll tickets and traits untouched')
 o=e.officers[952];o.put(0xe6,b'\x01');o.put(0x1c2,b'\x00\x03');o.put(0x182,b'\x03')
 check(b(o.addr+0x182)+BR.call(e.game.addr+0x2a9010,o.addr,3)==3 and b(o.addr+0x182)+e.fn('hook_bonus',o.addr,3)==4,'reported base policy 3 reproduces missing native ruler bonus and resolves to effective level 4')
 for address in (0x2320b0,0x232640):
  e.game.put(address,b'\xb8\x01\0\0\0\xc3')
  check(e.fn('hook_bonus',o.addr,3)==0,'original bonus exclusion retained at '+hex(address))
  zero(e,address)
 o.put(0x15e,b'\x00\x40');check(e.fn('hook_bonus',o.addr,3)==0,'native flag 0x4000 cannot be bypassed by restoring ruler domestic bonus');o.put(0x15e,b'\0\0')
 e.mod.val('metadata_fault',1);check(e.fn('hook_bonus',o.addr,3)==0,'invalid metadata cannot create a ruler reward');e.mod.val('metadata_fault',0)
 # Role changes are observed immediately; the policy reward is never added twice.
 for status in (4,1,4,1):
  o.put(0xe6,bytes([status]));assert e.fn('hook_bonus',o.addr,3)==1 and e.fn('hook_bonus',o.addr,2)==2
 check(True,'current/former/reappointed rulers keep one policy reward and three territory capacity')
 # Execute the actual native display arithmetic and maximum, substituting only
 # the string-formatting endpoints and the already-covered wrapper gate.
 d=R15['policy_display'];e.game.put(d['rva'],bytes.fromhex(d['bytes']));jump(e,0x2a9190,e.mod.sym('hook_bonus'))
 # Run the contiguous native arithmetic head (UI formatting tail is a stub).
 # Preserve all relative targets by terminating immediately after the add.
 tail=bytes.fromhex('8bc7488b5c24404883c4305fc3')
 old=e.game.get(0x35a2b0,len(tail));e.game.put(0x35a2b0,tail)
 check(BR.call(e.game.addr+0x35a290,o.addr)==4,'native policy display arithmetic consumes restored ruler bonus')
 e.game.put(0x35a2b0,old)
 check(not BR.errors,'no native domestic callback failures');e.closeall()

def lower_selection_and_save():
 e=NativeR11UI();o,row,state=e.prepare(id=952,owned=(97,59,88,21,66),capacity=1)
 e.setup_rules();domestic_table(e);force_ruler(e,952);o.put(0xe6,b'\x01')
 e.fn('hook_ui_enter',e.root_layout.addr);e.fn('hook_merit_eligible',o.addr);e.fn('refresh_tables');e.refresh_choices(e.list_owner.addr)
 st=e.gs(952);st.rerolls_used=2;st.cancelled_rerolls=2;before=bytes(st);rng=e.rng_calls;owned=e.owned(o)
 for j in (3,4,5):
  e.click(row,j)
  check(e.rng_calls==rng and bytes(e.gs(952))==before and not e.mod.val('preview_count'),'lower selection '+str(j-2)+' creates no gacha, RNG or reroll-ticket charge')
 check(e.selection_counts(952)==(0,3) and e.decision_enabled,'all three domestic selections follow the actual ruler native prefix/capacity')
 e.setup_native_commit();e.bottom(state)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3))==(0,3) and e.rng_calls==rng and bytes(e.gs(952))==before and e.owned(o)==owned,'actual player native commit grants lower rewards only, preserving all upper states and tickets')
 real_bonus(e);check(e.fn('hook_bonus',o.addr,3)==1,'committed player ruler receives the earned policy bonus')
 native=b'R15 lower reward save';check(e.save(native)==1,'domestic-only reward and unchanged reroll state save with existing wire format')
 o.put(0x1c2,b'\0\0');e.gs(952).rerolls_used=0;e.gs(952).cancelled_rerolls=0
 check(e.loadsave(native)==1 and (b(o.addr+0x1c2),b(o.addr+0x1c3))==(0,3),'existing ledger restores all three paid lower choices after reload')
 check(bytes(e.gs(952))==before and e.fn('hook_bonus',o.addr,3)==1,'reload restores unchanged tickets and exactly one policy bonus')
 e.closeall()
 # AI 9/9 fallback takes the bottom row without any roll, just like the player.
 e=EN();install_native_rules(e);domestic_table(e);o=e.officer(952,owned=tuple(range(4,13)));o.put(0xe6,b'\x01');force_ruler(e,952);e.fn('hook_newgame',0,0)
 e.gs(952).rerolls_used=2;e.gs(952).cancelled_rerolls=2;before=bytes(e.gs(952));rng=e.rng_calls
 NativeR11UI.setup_native_commit(e)
 batch=e.batch([(952,0,0)]);batch.u64(16,e.city.addr)
 check(e.fn('hook_commit_ai',batch.addr)==1 and (b(o.addr+0x1c2),b(o.addr+0x1c3))==(0,3),'AI ruler at 9/9 takes all three domestic rewards through the unchanged choice policy')
 real_bonus(e);check(e.rng_calls==rng and bytes(e.gs(952))==before and e.fn('hook_bonus',o.addr,3)==1,'AI domestic fallback consumes no RNG/tickets and receives its policy reward')
 e.closeall()

if __name__=='__main__':
 try:matrix();lower_selection_and_save()
 finally:(ROOT/'audit/r15_domestic_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2)+'\n')
 print('PASS R15 DOMESTIC',len(REPORT))
