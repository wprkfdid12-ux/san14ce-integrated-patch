"""R16 compiled Win64 rule/transaction regression. Substituted game and OS only."""
from test_rc4 import *
from test_r15_gacha import Cache, all_targets

def pending(st, key):
 x=st.slots[0];x.phase=1;x.result=key;x.raw=x.weight=x.quality=1
 x.cost=2 if COLORS[key]==3 else .38;x.x_index=255

def native_guard():
 e=Env();assert e.fn('rules_prepare_native')==1
 e.game.put(0x361143,b'\xa8');assert e.fn('rules_prepare_native')==0 and not e.patch_calls
 check(True,'unrecognized live sex accessor is rejected before installing hooks')
 e.closeall()

def restrictions():
 e=E();t=target(596);t.traits=(C.c_uint16*9)();st=State();d=Distribution()
 missing=json.loads((ROOT/'analysis/r15_missing_trait_directions.json').read_text())
 for row in missing:
  t.traits=(C.c_uint16*9)(row['held']);e.fn('g_init',C.addressof(st),C.addressof(t.traits))
  e.fn('g_distribution',C.addressof(t),C.addressof(st),C.addressof(COLORS),0,0,C.addressof(d))
  pending(st,row['candidate']);before=bytes(t)
  assert d.weight[row['candidate']]==0 and e.fn('g_accept',C.addressof(t),C.addressof(st),0,0,0)==0xfffffffb and bytes(t)==before,row
  assert e.fn('g_blocks',row['candidate'],row['held'])==1,row
 check(len(missing)==139,'all 139 audited missing directions blocked in both distribution and commit; native exclusion symmetric')
 t.traits=(C.c_uint16*9)()
 for sex in (0,1,255):
  t.sex=sex;e.fn('g_init',C.addressof(st),C.addressof(t.traits))
  e.fn('g_distribution',C.addressof(t),C.addressof(st),C.addressof(COLORS),0,0,C.addressof(d))
  for key in (94,95,96,168,171):
   assert (d.weight[key]>0)==(sex==1),(sex,key)
   e.fn('g_init',C.addressof(st),C.addressof(t.traits));pending(st,key)
   test=Target.from_buffer_copy(t)
   assert e.fn('g_accept',C.addressof(test),C.addressof(st),0,0,0)==(1 if sex==1 else 0xfffffffb)
 check(True,'male and unknown sex exclude all five female-only traits; female draw and commit remain available')
 t.sex=0;cache=Cache();e.fn('g_init',C.addressof(st),C.addressof(t.traits))
 def calc():return e.fn('g_distribution_cached',C.addressof(t),C.addressof(st),C.addressof(COLORS),0,0,C.addressof(d),C.addressof(cache))
 calc();hits=e.mod.val('g_calc_counters',fmt='<Q') # below uses hit counter
 n=get(e.mod.sym('g_calc_counters')+8);calc();assert get(e.mod.sym('g_calc_counters')+8)==n+1
 t.sex=1;calc();assert get(e.mod.sym('g_calc_counters')+8)==n+1 and d.weight[94]>0
 check(True,'AI cache includes live sex while unchanged inputs reuse the distribution')
 # Existing original combinations remain untouched; only the new candidate is checked.
 t.traits=(C.c_uint16*9)(61,62);e.fn('g_init',C.addressof(st),C.addressof(t.traits));pending(st,62);old=bytes(t)
 assert e.fn('g_accept',C.addressof(t),C.addressof(st),0,0,0)==0xfffffffb and bytes(t)==old
 check(True,'existing incompatible initial traits preserved; duplicate acquired trait rejected')
 # Runtime fields, including current linked-officer validity, govern the extra branch.
 o=e.officer(596,owned=(31,),top=1);e.fn('hook_newgame',0,0)
 reads=e.api_counts['readable'];e.fn('refresh_trait_rules')
 check(e.api_counts['readable']-reads<=2,'unchanged native trait objects refresh current rule fields without per-trait OS memory queries')
 saved=e.traits.addr+94*0x100;e.db.u64(0x7dd18+94*8,1);e.fn('refresh_trait_rules')
 t.sex=1;t.traits=(C.c_uint16*9)();assert not e.fn('g_allowed',C.addressof(t),94)
 e.db.u64(0x7dd18+94*8,saved);e.fn('refresh_trait_rules');assert e.fn('g_allowed',C.addressof(t),94)
 check(True,'changed unreadable trait identity excluded safely; restored object revalidated')
 assert e.fn('g_blocks',31,30)==1
 e.traits.put(31*0x100+0xa2,struct.pack('<H',596));e.fn('refresh_trait_rules')
 assert e.fn('g_blocks',31,30)==0
 e.traits.put(31*0x100+0xa2,b'\xff\xff');e.fn('refresh_trait_rules');assert e.fn('g_blocks',31,30)==1
 # A changed live restriction invalidates an otherwise identical AI cache key.
 t.traits=(C.c_uint16*9)();t.sex=0;e.fn('g_init',C.addressof(st),C.addressof(t.traits));calc();assert d.weight[94]==0
 n=get(e.mod.sym('g_calc_counters')+8);e.traits.put(94*0x100+0xa4,b'\x00');e.fn('refresh_trait_rules');calc()
 assert d.weight[94]>0 and get(e.mod.sym('g_calc_counters')+8)==n
 check(True,'live trait-rule edit invalidates an otherwise identical cached candidate pool')
 e.traits.put(94*0x100+0xa4,bytes([bytes.fromhex(next(x['raw_param_hex'] for x in MASTER if x['trait_id']==94))[0]]));e.fn('refresh_trait_rules')
 check(True,'opposite-sign rule respects live linked-officer field, not a guessed fixed pair list')
 # Custom owned traits sharing a native effect are rejected; unequal effects stay eligible.
 ptr=e.traits.addr+256*0x100;e.db.u64(0x7dd18+256*8,ptr)
 e.traits.put(256*0x100+0x10,'시험\0'.encode('utf-16le'));e.traits.put(256*0x100+0xb6,bytes([get(e.traits.addr+30*0x100+0xb6,'<B')]))
 e.fn('refresh_trait_rules');assert e.fn('g_blocks',256,30)==1
 check(True,'custom owned trait native same-effect exclusion applies to official candidates')
 # An already-paid invalid legacy pending is reset without deleting traits or refunding tickets.
 gs=e.gs(596);pending(gs,94);gs.slots[0].rerolls=5;gs.rerolls_used=5
 before=o.get(0x118,18);e.fn('repair_pending',o.addr)
 assert not gs.slots[0].phase and gs.rerolls_used==gs.cancelled_rerolls==5 and o.get(0x118,18)==before
 assert e.fn('g_validate',C.addressof(gs))
 check(True,'invalid legacy pending migration preserves all five spent tickets and existing native slots')
 o.put(0xa9,b'\x01');live=Target();e.fn('target_read',o.addr,C.addressof(live));assert live.sex==1
 o.put(0xa9,b'\x00');e.fn('target_read',o.addr,C.addressof(live));assert live.sex==0
 check(C.sizeof(Target)==32,'actual +0xA9 sex read without changing target wire alignment')
 e.closeall()

def all_db():
 e=E();old=Loaded(ROOT/'audit/r15_release_baseline/SAN14_MeritTraits.dll',BR);e.mem.append(old)
 st=State();a=Distribution();b_=Distribution();cache=Cache();count=0
 for t in all_targets():
  e.fn('g_init',C.addressof(st),C.addressof(t.traits))
  args=(C.addressof(t),C.addressof(st),C.addressof(COLORS),0,0)
  BR.call(old.sym('g_distribution'),*args,C.addressof(a));e.fn('g_distribution',*args,C.addressof(b_))
  assert bytes(a.raw)==bytes(b_.raw),t.id
  assert bool(e.fn('g_has_pool',*args[:3]))==bool(b_.count)
  for key in range(1,355):
   if b_.weight[key]:assert b_.weight[key]==a.weight[key] and e.fn('g_allowed',C.addressof(t),key)
  for an,tr in ((0,0),(1,0),(1,1)):
   e.fn('g_distribution',*args[:3],an,tr,C.addressof(a))
   e.fn('g_distribution_cached',*args[:3],an,tr,C.addressof(b_),C.addressof(cache))
   assert bytes(a)==bytes(b_),(t.id,an,tr)
  count+=1
 empty=(C.c_uint8*355)();assert not e.fn('g_has_pool',C.addressof(t),C.addressof(st),C.addressof(empty))
 check(count==1088,'all 1088 official targets preserve teacher raw weights; cached/direct outcomes match in all three modes')
 e.closeall()

def stale_preview():
 from test_r3 import R3
 e=R3();o,r,s=e.prepare(top=1);e.click(r,0);e.reveal(s)
 before=bytes(e.gs(596));rng=e.rng_calls;owned=o.get(0x118,18)
 o.put(0xa9,b'\x01')
 assert not e.fn('perform_roll',o.addr,0,1,0)
 assert not e.fn('finish_result',o.addr,0,0,0)
 assert e.rng_calls==rng and bytes(e.gs(596))==before and o.get(0x118,18)==owned
 check(True,'live sex edit rejects stale preview before RNG, ticket charge or native trait writes')
 e.closeall()

if __name__=='__main__':
 native_guard();restrictions();all_db();stale_preview()
 (ROOT/'audit/r16_trait_tests.json').write_text(json.dumps({'passed':len(REPORT),'tests':REPORT,'live_game_tested':False},ensure_ascii=False,indent=2)+'\n')
