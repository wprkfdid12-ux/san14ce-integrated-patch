"""Supersedes one obsolete R10 assertion: a verified R11 paid-count ledger may
legitimately restore a dropped native unlock. Without either source of paid
history the same pending record must still be rejected. No checksum bypass.
"""
from test_cp03 import *
def semantic_file_damage_r11():
 e=E();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'checksummed-R11-state';e.save(native);key=next(k for k in e.files if k.endswith('.mt4dat'));original=e.files[key]
 off=88+6000*C.sizeof(Profile)+596*C.sizeof(State)
 for label,mutate in [('NaN cost',lambda x:setattr(x.slots[0],'cost',float('nan'))),('reroll mismatch',lambda x:setattr(x,'rerolls_used',4))]:
  raw=bytearray(original);st=State.from_buffer_copy(raw[off:off+C.sizeof(State)]);mutate(st);raw[off:off+C.sizeof(State)]=bytes(st);raw[56:88]=hashlib.sha256(raw[88:]).digest();e.files[key]=bytes(raw);load_bytes(e,native)
  check(e.mod.val('metadata_fault')==0 and e.gs(596).slots[0].phase!=1,'R40: R11 checksum-valid '+label+' -> that officer starts fresh, no module fault')
 raw=bytearray(original);raw[88+596*C.sizeof(Profile)+Profile.award_mask.offset]=1;raw[56:88]=hashlib.sha256(raw[88:]).digest();e.files[key]=bytes(raw);load_bytes(e,native)
 check(e.mod.val('metadata_fault')==0 and e.gs(596).slots[0].phase!=1,'R40: R11 contradicting award bit -> that officer starts fresh, no module fault')
 e.files[key]=original;o.put(0x1c2,b'\0');load_bytes(e,native)
 check(not e.mod.val('metadata_fault') and b(o.addr+0x1c2)==1,'verified paid-count ledger restores pending stage after native status reset')
 raw=bytearray(original);poff=88+596*C.sizeof(Profile);p=Profile.from_buffer_copy(raw[poff:poff+C.sizeof(Profile)]);p.reserved[:]=(0,0,0);raw[poff:poff+C.sizeof(Profile)]=bytes(p);raw[56:88]=hashlib.sha256(raw[88:]).digest();e.files[key]=bytes(raw);o.put(0x1c2,b'\0');load_bytes(e,native)
 check(e.mod.val('metadata_fault')==0 and e.gs(596).slots[0].phase!=1,'R40: pending stage without native unlock or verified ledger is not restored (that officer fresh), no module fault')
 e.closeall()

from test_native_integration import EN
def live_targets_ai_r11():
 e=EN();o=e.officer(5001,top=3);t=Target()
 e.fn('target_read',o.addr,C.addressof(t));check(t.category==3 and not t.star,'custom target is not an official teacher and has no invented star flag')
 ids=sorted({i for p in PEOPLE for i in p['unique_tactic_ids']});assert ids
 o.put(0x130,bytes([ids[0]]));o.put(0xb4,struct.pack('<HH',596,5001));e.fn('target_read',o.addr,C.addressof(t))
 check(t.star==1 and t.father==596 and t.mother==65535,'current custom unique tactic and valid parent read from native serializer layout')
 o.put(0x130,b'\0'*10);o.put(0xb4,struct.pack('<HH',6001,0));e.fn('target_read',o.addr,C.addressof(t))
 check(not t.star and t.father==65535 and t.mother==65535,'removed tactic and invalid/self/zero parent do not retain stale official flags')
 official=e.officer(596,top=0);e.fn('target_read',official.addr,C.addressof(t));check(not t.star,'edited official officer without unique tactic is not automatically a star')
 BR.callbacks['AIEligible']=lambda o:7;e.game.put(e.res.native.funcs[9],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('AIEligible')))
 e.fn('hook_newgame',0,0);rank=b(o.addr+0x1bc);v=e.fn('hook_ai_visit',o.addr)
 check(v==7 and all(x.phase==0 for x in e.gs(5001).slots),'R11 candidate visitation returns native qualification but never auto-awards legacy upper flags')
 before=bytes(e.gs(5001));rng=e.rng_calls;e.fn('hook_ai_visit',o.addr)
 check(bytes(e.gs(5001))==before and e.rng_calls==rng,'repeat native AI visit never duplicates accepted/declined awards')
 check(e.gs(5001).rerolls_used<=5 and b(o.addr+0x1bc)==rank and b(o.addr+0x1c2)==3,'AI uses same shared five rerolls without fabricating native rank/flags')
 check(not e.talks,'AI rewards do not open officer gratitude dialogue')
 other=e.officer(5002,top=1);e.mod.val('save_active',1);e.fn('hook_ai_visit',other.addr);check(not e.gs(5002).initialized,'AI mutation waits while native save snapshot is active')
 e.closeall()

