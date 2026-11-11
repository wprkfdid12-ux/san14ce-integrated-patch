"""R40 A2/A12: 13번은 오래 해도 막히지 않는다 — 실패는 그 무장/그 저장 하나만 건너뛰고 나머지는 계속.
Synthetic production-code fixture (same harness as test_native_integration), not a live game.
"""
from test_native_integration import *
from r11_fixture import install_native_rules,force_ruler

def interrupted_then_save_again():
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);old=b'old compressed';new=b'new compressed';state=bytes(e.gs(596));assert e.save(old)
 e.fn('perform_roll',o.addr,0,1,0);newstate=bytes(e.gs(596));e.native_ok=False
 check(e.save(new)==0,'native write failure is still reported as not saved (game result passed through)')
 path,ref=e.ref();check(ref.phase==1 and not e.mod.val('metadata_fault'),'failed native write leaves INTENT but does NOT latch a module-wide fault')
 e.native_ok=True
 check(e.save(new)==1,'R40: the next save to the same slot is no longer blocked by the earlier INTENT')
 path,ref=e.ref();check(ref.phase==0 and ref.previous_exists,'after the retry the slot journal is committed and keeps the last committed generation as previous')
 check(e.loadsave(new)==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==newstate,'new save loads its own state')
 check(e.loadsave(old)==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==state,'older committed generation still loads its own state')
 e.closeall()

def identical_bytes_interrupted():
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'same bytes';committed=bytes(e.gs(596));assert e.save(native);e.fn('perform_roll',o.addr,0,1,0)
 e.after_native=lambda:setattr(e,'fail_move',True)
 r=e.save(native);path,ref=e.ref()
 check(ref.phase==1 and not e.mod.val('metadata_fault'),'reference commit failure after native write: INTENT kept, no fault latch')
 check(r==1,'R40: native save that really wrote is reported saved (no false failure)')
 e.fail_move=False;e.after_native=None
 check(e.loadsave(native)==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==committed,'identical-byte interrupted state loads the last committed generation instead of refusing')
 check(e.save(native+b'2')==1,'saving keeps working afterwards')
 e.closeall()

def unmatched_companion_fresh_start():
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'copy bytes';assert e.save(native)
 e.fn('perform_roll',o.addr,0,1,0);assert e.save(native)
 check(e.loadsave(native,'third-name.s14')==1 and not e.mod.val('metadata_fault'),'ambiguous copied save: no fault latch, loads as a fresh merit start')
 check(e.save(b'after fresh start')==1,'saving is allowed after the fresh start')
 e.closeall()
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);assert e.save(b'A');e.fn('perform_roll',o.addr,0,1,0);e.native_ok=False;assert not e.save(b'B');path,ref=e.ref()
 key=bytes(ref.key).hex();obj=next(p for p in e.files if p.endswith(key+f'.slot{ref.current}.mt4dat'));e.files[obj]=e.files[obj][:-1]+b'\xff'
 e.native_ok=True
 check(e.loadsave(b'B')==1 and not e.mod.val('metadata_fault'),'corrupt companion snapshot: load continues as fresh start, no fault latch')
 check(e.save(b'C')==1,'saving works after a corrupt companion')
 e.closeall()

def ledger_rebuilt_per_officer():
 e=EN();install_native_rules(e);o=e.officer(771,top=1,bottom=2);o.put(0xe6,b'\x01');force_ruler(e,771);e.fn('hook_newgame',0,0)
 other=e.officer(772,top=0,bottom=1);e.fn('hook_newgame',0,0)
 e.fn('sync_merit_history',o.addr);e.fn('sync_merit_history',other.addr)
 before_other=bytes(e.profile(772))
 e.profile(771).reserved[1]=0x33;e.fn('sync_merit_history',o.addr)
 check(not e.mod.val('metadata_fault'),'corrupt six-reward ledger no longer disables the module')
 check(e.profile(771).reserved[1]==(1|(2<<4)) and (b(o.addr+0x1c2),b(o.addr+0x1c3))==(1,2),'ledger rebuilt from the real native counts (no free refill)')
 check(bytes(e.profile(772))==before_other,'other officers untouched')
 e.closeall()

def ai_batch_partial():
 e=EN();install_native_rules(e);o=e.officer(596,owned=tuple(range(4,12)));o.put(0xe6,b'\x01');force_ruler(e,596)
 bad=e.officer(13,top=2,owned=(4,37,51,62,65));e.fn('hook_newgame',0,0)
 oldcommit=BR.callbacks['NativeCommit']
 def clamped(ptr):
  v=oldcommit(ptr);o.put(0x1bc,b'\x06');return v
 BR.callbacks['NativeCommit']=clamped
 n=e.native_commits;batch=e.batch([(596,0,0),(13,1,0)])
 check(e.fn('hook_commit_ai',batch.addr)==1 and e.native_commits==n+1,'R40: one unsuitable AI officer (batch lower than earned) no longer refuses the whole native AI batch')
 check(e.profile(596).award_mask==1,'the suitable officer in the same batch still receives his gacha award')
 check(not e.mod.val('metadata_fault'),'no module-wide fault')
 BR.callbacks['NativeCommit']=oldcommit;e.closeall()

def preview_cap():
 import re
 src=(ROOT/'source/runtime_flow.inc').read_text(encoding='utf-8')
 check(re.search(r'#define PREVIEW_CAP 512',src) is not None,'preview capacity raised from 64 to 512 (large forces)')

for t in (interrupted_then_save_again,identical_bytes_interrupted,unmatched_companion_fresh_start,ledger_rebuilt_per_officer,ai_batch_partial,preview_cap):t()
print('R40 done')
