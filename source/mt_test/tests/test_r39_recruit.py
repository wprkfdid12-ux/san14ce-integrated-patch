"""R39 A1: recruited officer (항장) ledger reset vs plain merit deduction.
Synthetic R17 fixture (same merit_history.inc as the 1.0.1 build), not a live game.
"""
from r11_fixture import *

def setup(id=771):
 e=EN();install_native_rules(e);o=e.officer(id,top=1,bottom=2);o.put(0xe6,b'\x01');force=force_ruler(e,id);e.fn('hook_newgame',0,0)
 check(e.fn('perform_roll',o.addr,0,0,1)==1 and e.fn('finish_result',o.addr,0,0,1)==1,'fixture: one finalized upper gacha award')
 award=e.profile(id).awards[0];traits=e.owned(o)
 check(e.profile(id).award_mask==1 and award in traits and e.gs(id).slots[0].phase==2,'fixture: award installed and ledger records it')
 return e,o,force,award,traits

def recruit(e,o,force,rank=1,merit=-1000):
 force.put(e.res.native.offsets[5],b'\0\0');o.put(0xe6,b'\x04')
 o.put(0x1bc,bytes([rank]));o.put(0x1be,struct.pack('<h',merit));o.put(0x1c2,b'\0\0')

def recruited_officer_reset():
 e,o,force,award,traits=setup()
 # per-slot parallel arrays (+0x184 bit mask, +0x186 words) must follow the traits
 slot=traits.index(award);bits=0;words=[]
 for k in range(9):
  if traits[k]:bits|=1<<k
  words.append(0x100+k)
 o.put(0x184,struct.pack('<H',bits|0x8000));o.put(0x186,struct.pack('<9H',*words));o.put(0x15e,b'\0\0')
 resets=e.mod.val('r39_recruit_resets')
 recruit(e,o,force);e.fn('sync_merit_history',o.addr)
 now=e.owned(o);expect=[t for t in traits if t and t!=award]
 check((b(o.addr+0x1bc),b(o.addr+0x1c2),b(o.addr+0x1c3))==(1,0,0),'recruit: native rank 1 and zero reward counts kept (no restore)')
 check(struct.unpack('<h',o.get(0x1be,2))[0]==-1000,'recruit: negative merit untouched')
 check(award not in now and list(now[:len(expect)])==expect and all(x==0 for x in now[len(expect):]),'recruit: only gacha trait removed, original traits kept in order and compacted')
 nb=struct.unpack('<H',o.get(0x184,2))[0];nw=struct.unpack('<9H',o.get(0x186,18))
 kept=[k for k in range(9) if traits[k] and traits[k]!=award]
 check(nb==(0x8000|((1<<len(kept))-1)) and list(nw[:len(kept)])==[0x100+k for k in kept] and all(x==0 for x in nw[len(kept):]),'recruit: slot bit mask and slot words moved with their traits; bits above slot 9 kept')
 check(struct.unpack('<H',o.get(0x15e,2))[0]&0x8000,'recruit: native trait-changed flag raised as on award')
 p=e.profile(771);g=e.gs(771)
 check(p.award_mask==0 and list(p.awards)==[0xffff]*3 and list(p.reserved)==[2,0,1],'recruit: ledger wiped (awards, counts, ruler marker), history rank = native rank')
 check(g.initialized==1 and all(s.phase==0 for s in g.slots) and g.rerolls_used==0 and list(g.base_traits)==list(now),'recruit: gacha state restarts from stage one with remaining traits as base')
 check(e.mod.val('r39_recruit_resets')==resets+1 and e.mod.val('r39_recruit_removed')>=1 and not e.mod.val('metadata_fault'),'recruit: counted once, no metadata fault')
 before=(bytes(p),bytes(g),o.get(0x118,18),o.get(0x1bc,8))
 for _ in range(3):e.fn('sync_merit_history',o.addr)
 check((bytes(e.profile(771)),bytes(e.gs(771)),o.get(0x118,18),o.get(0x1bc,8))==before and e.mod.val('r39_recruit_resets')==resets+1,'recruit: idempotent on repeated sync')
 check(e.save(b'R39 recruited save')==1,'recruit: v3 sidecar saves wiped ledger')
 snap=(bytes(e.profile(771)),bytes(e.gs(771)))
 check(snap==before[:2],'recruit: save-time sync does not change the wiped ledger')
 check(e.loadsave(b'R39 recruited save')==1 and not e.mod.val('metadata_fault'),'recruit: save/load round trip passes validate_loaded_disk')
 check((bytes(e.profile(771)),bytes(e.gs(771)))==snap and e.owned(o)==now and e.mod.val('r39_recruit_resets')==resets+1,'recruit: load restores the wiped ledger exactly, no second reset, removed trait not resurrected')
 # vanilla order afterwards: repaid merit and promoted -> counts grow monotonically, ledger follows
 o.put(0x1bc,b'\x03');o.put(0x1be,struct.pack('<h',500));o.put(0x1c2,b'\x01\x00');e.fn('sync_merit_history',o.addr)
 check(e.profile(771).reserved[1]==1 and e.profile(771).reserved[2]==3 and b(o.addr+0x1c2)==1,'after recruit: new promotion recorded by the fresh ledger')
 o.put(0x1bc,b'\x02');o.put(0x1be,struct.pack('<h',100));e.fn('sync_merit_history',o.addr)
 check(b(o.addr+0x1bc)==3 and b(o.addr+0x1c2)==1 and e.mod.val('r39_recruit_resets')==resets+1,'after recruit: later plain deduction restored by the fresh ledger')
 e.closeall()

def plain_deduction_keeps_ledger():
 # one rank step down, counts unchanged (merit positive) -> R11 ledger kept, rank restored
 e,o,force,award,traits=setup()
 resets=e.mod.val('r39_recruit_resets');orig=bytes(e.gs(771));prof=bytes(e.profile(771))
 o.put(0x1bc,b'\x05');o.put(0x1be,struct.pack('<h',300));e.fn('sync_merit_history',o.addr)
 check(e.owned(o)==traits and bytes(e.gs(771))==orig and e.profile(771).award_mask==1,'deduction: gacha trait and state kept')
 check((b(o.addr+0x1c2),b(o.addr+0x1c3),b(o.addr+0x1bc))==(1,2,6) and e.mod.val('r39_recruit_resets')==resets,'deduction: counts kept, rank restored to ledger maximum, no reset')
 # negative merit but counts NOT reset by the game -> not a recruit
 o.put(0x1bc,b'\x01');o.put(0x1be,struct.pack('<h',-50));e.fn('sync_merit_history',o.addr)
 check(e.owned(o)==traits and e.profile(771).award_mask==1 and e.mod.val('r39_recruit_resets')==resets,'negative merit with unchanged counts: ledger kept')
 # counts reset but merit not negative (R11 status change) -> old restore path
 force.put(e.res.native.offsets[5],b'\0\0');o.put(0xe6,b'\x04');o.put(0x1bc,b'\0');o.put(0x1c2,b'\0\0');o.put(0x1be,b'\0\0')
 e.fn('sync_merit_history',o.addr)
 check((b(o.addr+0x1c2),b(o.addr+0x1c3),b(o.addr+0x1bc))==(1,2,6) and e.owned(o)==traits and e.mod.val('r39_recruit_resets')==resets,'counts reset without negative merit: R11 restore kept')
 # rank above 2 with zero counts and negative merit -> not the recruit signature
 o.put(0x1c2,b'\0\0');o.put(0x1bc,b'\x04');o.put(0x1be,struct.pack('<h',-1000));e.fn('sync_merit_history',o.addr)
 check(e.profile(771).award_mask==1 and e.mod.val('r39_recruit_resets')==resets,'rank 4 is not a recruit reset')
 e.closeall()

def declined_miss_and_pending():
 # declined (not installed) slot: its recorded trait must not be stripped
 e,o,force,award,traits=setup()
 e.gs(771).slots[0].phase=3;recruit(e,o,force);e.fn('sync_merit_history',o.addr)
 check(award in e.owned(o) and e.profile(771).award_mask==0,'declined slot: recorded trait left alone, ledger still wiped')
 e.closeall()
 # MISS slot records award 0: nothing to strip
 e,o,force,award,traits=setup()
 e.profile(771).awards[0]=0;e.gs(771).slots[0].outcome=1;recruit(e,o,force);e.fn('sync_merit_history',o.addr)
 check(award in e.owned(o) and e.profile(771).award_mask==0,'MISS slot: no trait removed, ledger wiped')
 e.closeall()
 # pending uncommitted draw: defer reset (old behaviour this time)
 e,o,force,award,traits=setup()
 resets=e.mod.val('r39_recruit_resets');e.gs(771).slots[1].phase=1;recruit(e,o,force);e.fn('sync_merit_history',o.addr)
 check(e.mod.val('r39_recruit_resets')==resets and e.profile(771).award_mask==1 and award in e.owned(o),'pending draw: reset deferred, nothing removed')
 e.closeall()

if __name__=='__main__':
 try:
  for f in (recruited_officer_reset,plain_deduction_keeps_ledger,declined_miss_and_pending):print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r39_recruit_tests.json').write_text(json.dumps({'scope':__doc__,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R39 recruit',len(REPORT))
