"""Incremental R4 display/interop regressions. Production Win64 code on Linux
with substituted OS/widget endpoints. This is NOT a live Windows/game test.
The pinned RC25 binary is a test input only; it is never an install payload.
"""
from test_r3 import *

SITES=[0x1101b,0x11062,0x11176,0x119a1,0x12a13,0x131dd,0x13615,0x1395a,0x139dd,0x13a1d,0x13a53,0x13b50,0x14045]
RC25=ROOT/'references/compatibility/sortie_rc25/SAN14_SortiePresets.dll'

def jump(m,rva,name):m.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))

def preset_module(e,installed=1,loaded=True):
 pe=PE(RC25);m=e.alloc(pe.size+0x20000,bytes(pe.m));m.u64(0x958e8,e.game.addr);m.u32(0x1b028,installed)
 e.preset_loaded=loaded;e.preset=m
 BR.callbacks['GetModuleHandleW']=lambda p:(m.addr if e.preset_loaded else 0) if wide(p)=='SAN14_SortiePresets.dll' else e.getmodule(p)
 return m

def preset_widgets(e,m,officer_id=596):
 panel=e.alloc(0x200);labels=[e.alloc(0x198) for _ in range(9)]
 m.u64(0x34040+0x10,panel.addr);m.u32(0x33d40,1);m.u32(0x34bb0,1);m.u32(0x34040+0xb64,0)
 m.u32(0x35360,1);m.u32(0x35364,1);m.u32(0x35368,officer_id)
 for i,w in enumerate(labels):w.u64(0,e.widget_vt.addr);w.u64(8,panel.addr);m.u64(0x34040+0x58+i*8,w.addr)
 for r,n in [(0x85c270,'Text'),(0x856bc0,'CaptionColor')]:jump(e.game,r,n)
 return labels

def actual_modcore_contract():
 e=Env();core_path=ROOT/'payload/SAN14Mods/Core/SAN14_ModCore.dll';c=Loaded(core_path);e.mem.append(c)
 # Execute the supplied Core's registry, not the permissive earlier Patch
 # stub. Substitute only its OS VirtualQuery/protection/cache endpoints.
 c.u64(0x6f38,e.game.addr);c.u32(0x6f34,e.res.image_size);c.u64(0x6e80,BR.addr('VirtualQuery'))
 for loc,n in [(0x3030,'VirtualProtect'),(0x3038,'FlushInstructionCache'),(0x3040,'GetCurrentProcess')]:c.u64(loc,BR.addr(n))
 def query(p,out,size):
  if e.game.addr<=p<e.game.addr+e.res.image_size:
   C.memmove(out,struct.pack('<QQIIQIIII',e.game.addr,e.game.addr,0x20,0,e.res.image_size,0x1000,0x20,0x1000000,0),48);return 48
  return e.virtualquery(p,out,size)
 BR.callbacks['VirtualQuery']=query;e.core.patch=c.addr+0x1140
 owner=C.create_string_buffer(b'Merit.R4.Registry.Test');p=CorePatch();p.address=e.ui.addr+0x2740;p.length=5;p.before[:5]=e.ui.get(0x2740,5);p.after[:5]=bytes(p.before[:5])
 before=e.ui.get(0x2740,5)
 check(BR.call(e.core.patch,C.addressof(owner),C.addressof(p),1)==0 and e.ui.get(0x2740,5)==before,'actual supplied Core rejects DLL-local addresses without modifying them (R3 activation defect reproduced)')
 p.address=e.game.addr+0x10000;e.game.put(0x10000,b'\x90'*5);p.before[:5]=b'\x90'*5;p.after[:5]=b'\xcc'*5
 check(BR.call(e.core.patch,C.addressof(owner),C.addressof(p),1)==1 and e.game.get(0x10000,5)==b'\xcc'*5,'actual Core still accepts verified game-image hooks')
 check(BR.call(e.core.patch,C.addressof(owner),C.addressof(p),1)==0,'actual Core conflict/byte ownership protections remain intact')
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1 and e.mod.val('enabled')==1 and e.mod.val('hover_x_count')==7,'complete Merit initialization succeeds with actual shipped Core code and seven Hover bridges')
 check(c.u32(0x5000)==7+e.res.hook_count,'actual Core owns 5 trait slots + 35 Merit hooks + 1 Hover game site + isolated test hook; no DLL-local targets')
 m=preset_module(e);e.fn('probe_presets_x')
 check(e.mod.val('presets_x_status')==1 and e.mod.val('presets_x_count')==13 and c.u32(0x5000)==7+e.res.hook_count,'RC25 optional display calls coexist with real Core without changing its game-only registry')
 check(hashlib.sha256(core_path.read_bytes()).hexdigest()=='f54c7d4848dacf0c2414b07c7573cdbac8bf3ca9b5aec84345d8f549b292fc67','shipped shared Core file stays byte-identical; its guard is not widened')
 e.closeall()

def dll_local_transaction_guards():
 e=Env();m=e.alloc(0x4000);patches=(CorePatch*2)();calls=[]
 for i,p in enumerate(patches):
  p.address=m.addr+0x1100+i*16;p.length=5;p.before[:5]=b'\xe8\1\0\0\0';p.after[:5]=b'\xe8\2\0\0\0';m.put(0x1100+i*16,bytes(p.before[:5]))
 def reset():
  for p in patches:C.memmove(p.address,bytes(p.before[:5]),5)
 def protect(p,n,f,out):calls.append((p,f));put(out,0x20,'<I');return 1
 BR.callbacks['VirtualProtect']=protect
 def install():return e.fn('install_display_calls',m.addr,m.size,C.addressof(patches),2)
 check(install()==1 and all(C.string_at(p.address,5)==bytes(p.after[:5]) for p in patches),'DLL-local transaction installs only prevalidated five-byte display calls')
 check([p for p,f in calls]==[patches[0].address,patches[1].address,patches[1].address,patches[0].address],'DLL-local transaction restores repeated page protection in reverse order')
 reset();m.put(0x1110,b'\x90');calls.clear()
 check(install()==0 and not calls and m.get(0x1100,5)==bytes(patches[0].before[:5]),'unknown module call bytes rejected before any protection/write')
 reset();saved=patches[1].address;patches[1].address=patches[0].address
 check(install()==0,'overlapping display call targets rejected');patches[1].address=saved
 for address in [e.game.addr+0x1000,m.addr+0x500,m.addr+m.size-4]:
  patches[1].address=address;check(install()==0,'display call target outside verified DLL text/image rejected '+hex(address-m.addr))
 patches[1].address=saved;calls.clear()
 def fail_open(p,n,f,out):
  calls.append((p,f));put(out,0x20,'<I');return int(not(p==saved and f==0x40))
 BR.callbacks['VirtualProtect']=fail_open
 check(install()==0 and all(C.string_at(p.address,5)==bytes(p.before[:5]) for p in patches) and calls[-1]==(patches[0].address,0x20),'page-opening failure writes no call and restores already-open page')
 def race(p,n,f,out):
  put(out,0x20,'<I')
  if p==saved and f==0x40:m.put(0x1110,b'\x90')
  return 1
 BR.callbacks['VirtualProtect']=race
 check(install()==0 and m.get(0x1100,5)==bytes(patches[0].before[:5]) and m.get(0x1110,1)==b'\x90','pre-write recheck preserves a concurrent external call edit')
 e.closeall()

def persistent_cache_failure():
 e=Env();sites=[0x2740,0x29a3,0x2e79,0x4355,0x398b,0x53ec];before={i:e.ui.get(i,5) for i in sites}
 BR.callbacks['FlushInstructionCache']=lambda process,p,n:int(not(e.ui.addr<=p<e.ui.addr+0x6af8))
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0 and not e.mod.val('enabled') and all(e.ui.get(i,5)==v for i,v in before.items()),'required Hover cache failure refuses activation and restores written display bytes')
 check(e.mod.val('hover_x_relays',fmt='<Q') and e.mod.val('relay_page',fmt='<Q') and not e.free_pages,'uncertain instruction cache retains required relay memory despite restored data bytes')
 e.closeall()
 e=R3();e.prepare(top=1);m=preset_module(e);before={a:m.get(a,5) for a in SITES}
 BR.callbacks['FlushInstructionCache']=lambda process,p,n:int(not(m.addr<=p<m.addr+0x16168))
 e.fn('probe_presets_x')
 check(e.mod.val('presets_x_status',fmt='<i')==-1 and e.mod.val('enabled')==1 and all(m.get(a,5)==v for a,v in before.items()),'optional RC25 cache failure restores bytes without disabling gacha')
 check(e.mod.val('presets_rollback_live')==1 and e.mod.val('presets_x_relay',fmt='<Q') and not e.free_pages,'uncertain RC25 instruction cache never releases a potentially callable relay')
 e.closeall()

def api_lifetime_guards():
 e=Env();trait=C.c_uint16(0xbeef);x=C.c_uint32(99)
 def query(id=596,slot=0,a=None,b_=None):
  trait.value=0xbeef;x.value=99
  return e.fn('SAN14_MeritSlotDisplayV1',id,slot,C.addressof(trait) if a is None else a,C.addressof(x) if b_ is None else b_)
 saved={n:e.mod.val(n,fmt='<Q') for n in ['core','res','game']}
 for n in saved:e.mod.val(n,0,'<Q')
 check(query()==0 and trait.value==0 and x.value==0,'display API safely returns unavailable before mod initialization')
 for n,v in saved.items():e.mod.val(n,v,'<Q')
 e.activate();o=e.officer(596);e.fn('capture_profiles',1)
 check(query()==1 and trait.value==4 and x.value==0,'initialized display API returns actual native trait')
 for n in ['core','res','game']:
  old=e.mod.val(n,fmt='<Q');e.mod.val(n,0,'<Q')
  check(query()==0 and not trait.value and not x.value,'missing '+n+' safely disables public display API');e.mod.val(n,old,'<Q')
 for n,v in [('enabled',0),('metadata_ready',0),('metadata_fault',1)]:
  old=e.mod.val(n);e.mod.val(n,v)
  check(query()==0 and not trait.value and not x.value,'display API fallback for '+n);e.mod.val(n,old)
 for id,slot in [(6000,0),(0xffffffff,0),(596,9),(596,0xffffffff),(5999,0)]:
  check(query(id,slot)==0 and not trait.value and not x.value,'invalid/missing officer or slot is rejected '+str((id,slot)))
 check(query(a=0)==0 and x.value==0,'null trait output rejected; other supplied output cleared')
 check(query(b_=0)==0 and trait.value==0,'null X output rejected; other supplied output cleared')
 e.game.u64(0x277df18,0);check(query()==0 and not trait.value and not x.value,'no live database is a clean public API fallback')
 e.closeall()

def red_traits_in_information():
 e=R3();red=next(t['id'] for t in CAT if t['color']==2 and t['id']<355)
 o,r,s=e.prepare(owned=(4,red),top=1);e.gs(596).x_mask=1<<2
 e.mod.val('hover_module',e.ui.addr,'<Q')
 base=[e.alloc(0x198) for _ in range(5)];labels=[e.alloc(0x198) for _ in range(9)];cache=e.alloc(0x60)
 for p in base+labels:p.u64(0,e.widget_vt.addr)
 for i,p in enumerate(labels):cache.u64(8+i*8,p.addr)
 info=e.alloc(0x180);vt=e.alloc(0x240);info.u64(0,vt.addr);vt.u64(0x1a0,BR.addr('InfoPerson'))
 BR.callbacks.update({'InfoOriginal':lambda p:0,'InfoPerson':lambda p:o.addr,'LabelGet':lambda p,i:base[(i-31)//2].addr,'InfoCache':lambda p,c:cache.addr})
 for m,rva,n in [(e.ui,0x16a0,'InfoOriginal'),(e.ui,0x1190,'InfoCache'),(e.game,0x92c390,'LabelGet'),(e.game,0x85c270,'Text'),(e.game,0x856bc0,'CaptionColor')]:jump(m,rva,n)
 before=o.get(0,0x200);e.fn('hover_x_info',info.addr)
 check(e.visibility[labels[1].addr] and e.texts[labels[1].addr]==wide(e.traits.addr+red*0x100+0x10),'red trait remains visible beside a committed X in officer information')
 check((labels[1].addr,2,1) in e.colors,'real red trait keeps native red color, not gacha eligibility color')
 check(e.visibility[labels[2].addr] and e.texts[labels[2].addr]=='X' and o.get(0,0x200)==before,'information X overlay does not rewrite officer stats or traits')
 t=target();t.traits=(C.c_uint16*9)(*e.owned(o));d=Distribution()
 e.fn('g_distribution',C.addressof(t),C.addressof(e.gs(596)),C.addressof(COLORS),0,0,C.addressof(d))
 check(d.weight[red]==0,'displaying red traits does not admit them to the blue/gold gacha pool')
 check(e.fn('display_trait',0)==0 and e.fn('display_trait',65535)==0 and e.fn('display_trait',355)==0,'empty/sentinel/out-of-range display IDs never become fake traits')
 e.closeall()

def preset_binding_and_display():
 e=R3();o,r,s=e.prepare(owned=(4,),top=1);e.gs(596).x_mask=(1<<1)|(1<<8);m=preset_module(e)
 labels=preset_widgets(e,m);before={a:m.get(a,5) for a in SITES};input_code=m.get(0x13e70,514);tooltip=m.get(0xd630,882)
 e.fn('probe_presets_x')
 check(e.mod.val('presets_x_status',fmt='<i')==1 and e.mod.val('presets_x_count')==13 and not e.patch_calls,'exact RC25 installs thirteen owned display calls without passing DLL addresses to game-only Core')
 check(all(m.get(a,5)!=v for a,v in before.items()) and m.get(0x10280,16)==bytes(PE(RC25).m[0x10280:0x10290]),'all refresh callers redirect while original refresh entry remains intact')
 after=m.get(0x13e70,514);off=0x14045-0x13e70
 check(input_code[:off]==after[:off] and input_code[off+5:]==after[off+5:] and m.get(0xd630,882)==tooltip,'RC25 input routing and original tooltip code are not replaced')
 calls=[]
 def original(full):
  calls.append(full);selected=m.u32(0x34040+0xb64);id=m.u32(0x35368+selected*0x80);p=e.officers.get(id)
  for i,w in enumerate(labels):
   key=get(p.addr+0x118+i*2,'<H') if p else 0
   m.u32(0x34040+0xd0+i*4,key);e.visibility[w.addr]=bool(key)
   if key:e.settext(w.addr,e.traits.addr+key*0x100+0x10)
 BR.callbacks['PresetsRefresh']=original;jump(m,0x10280,'PresetsRefresh')
 def refresh(full=0):
  at=0x14045;target_=m.addr+at+5+struct.unpack('<i',m.get(at+1,4))[0];BR.call(target_,full)
 native=o.get(0,0x200);book=m.get(0x35360,0x584);refresh(1)
 check(calls==[1] and all(e.visibility[labels[i].addr] and e.texts[labels[i].addr]=='X' for i in (1,8)),'real Win64 relay calls original refresh first then draws physical X cells 2 and 9')
 check(e.texts[labels[0].addr]==wide(e.traits.addr+4*0x100+0x10) and m.u32(0x34040+0xd0)==4,'original real-trait caption and tooltip ID remain authoritative')
 check(m.u32(0x34040+0xd0+4)==0 and m.u32(0x34040+0xd0+32)==0,'X never receives a synthetic trait tooltip ID')
 check(native==o.get(0,0x200) and book==m.get(0x35360,0x584),'preset overlay changes neither officer data nor preset selection/book records')
 other=e.officer(619,owned=(32,),top=1);e.fn('capture_profiles',2);m.u32(0x353e4,1);m.u32(0x353e8,619);m.u32(0x34040+0xb64,1);refresh()
 check(not e.visibility[labels[1].addr] and not e.visibility[labels[8].addr] and e.texts[labels[0].addr]==wide(e.traits.addr+32*0x100+0x10),'switching selected preset officer clears previous officer X without mixing state')
 m.u32(0x34040+0xb64,0);o.put(0x118+2,struct.pack('<H',37));refresh()
 check(e.gs(596).x_mask==1<<8 and e.texts[labels[1].addr]==wide(e.traits.addr+37*0x100+0x10),'external trait edit wins in presets and permanently removes only the overlapping X')
 # An existing native projection different from raw slots also wins locally.
 m.u32(0x34040+0xd0+32,51);e.texts[labels[8].addr]='native projected trait';e.fn('presets_x_overlay')
 check(e.texts[labels[8].addr]=='native projected trait','preset bridge never overwrites a nonzero native-rendered trait ID')
 m.u32(0x34040+0xd0+32,0)
 for loc,val in [(0x33d40,0),(0x34bb0,0),(0x34040+0xb64,0xffffffff),(0x35364,0)]:
  old=m.u32(loc);m.u32(loc,val);history=len(e.text_history);e.fn('presets_x_overlay')
  check(len(e.text_history)==history,'hidden/destroyed/invalid preset context rejects X overlay '+hex(loc));m.u32(loc,old)
 labels[8].u64(8,0);history=len(e.text_history);e.fn('presets_x_overlay')
 check(len(e.text_history)==history,'stale/reparented preset label is not mutated')
 patches=e.patch_calls[:];e.fn('probe_presets_x');check(e.patch_calls==patches,'active preset integration is idempotent and never rehashes/repatches modified call sites')
 check(not BR.errors,'preset bridge Microsoft ABI and widget callbacks report no errors');e.closeall()

def original_rc25_trait_loop():
 # Unlike the relay argument test, this executes the supplied RC25 refresh
 # function itself. Only resolver/entity, portrait/status and native widgets
 # are substituted; its nine-slot loop and cache/visibility logic stay real.
 e=R3();red=next(t['id'] for t in CAT if t['color']==2 and t['id']<355)
 o,r,s=e.prepare(owned=(4,red),top=1);e.gs(596).x_mask=(1<<2)|(1<<8)
 m=preset_module(e);labels=preset_widgets(e,m);e.fn('probe_presets_x')
 original=m.get(0x10280,3147)
 m.put(0x11d0,b'\xb8\x01\0\0\0\xc3');m.put(0x5220,b'\xc3')
 jump(m,0x4780,'PresetsEntity');jump(m,0x84e0,'PresetsStatus')
 def entity(table,id,maximum):
  if table==1:return e.officers[id].addr if id in e.officers else 0
  if table==2:return e.traits.addr+id*0x100 if 0<id<355 else 0
  raise AssertionError('unknown RC25 table '+str(table))
 def visible(w,value):e.visibility[w]=bool(value);put(w+0x40,int(bool(value)),'<I')
 BR.callbacks.update({'PresetsEntity':entity,'PresetsStatus':lambda p:0,'TraitAt':lambda p,i:get(p+0x118+i*2,'<H'),'Visible':visible})
 for loc,v in [(0x958d0,1),(0x958dc,2),(0x958e0,9),(0x95b14,0x40),(0x95b18,0xc8),(0x95f7c,0x80),(0x95f80,0x10),(0x95f84,0xbe)]:m.u32(loc,v)
 for loc,n in [(0x97138,'TraitAt'),(0x972e0,'CaptionColor'),(0x972f8,'Text'),(0x938c0,'GetTickCount64')]:m.u64(loc,BR.addr(n))
 # Every real RC25 draw calls back into the new wrapper then returns normally.
 at=0x14045;relay=m.addr+at+5+struct.unpack('<i',m.get(at+1,4))[0]
 BR.call(relay,0)
 check(m.get(0x10280,3147)==original,'supplied RC25 refresh machine code executed without modifying its body')
 check([m.u32(0x34040+0xd0+i*4) for i in range(9)]==[4,red,0,0,0,0,0,0,0],'actual RC25 loop retains real blue/red tooltip IDs and zero IDs for X')
 check(e.texts[labels[1].addr]==wide(e.traits.addr+red*0x100+0x10) and (labels[1].addr,2,1) in e.colors,'actual RC25 loop retains native red caption/color beside X')
 check(all(e.visibility[labels[i].addr] and e.texts[labels[i].addr]=='X' for i in (2,8)),'X overlay follows actual RC25 empty-slot hiding at both physical positions')
 e.gs(596).x_mask=0;BR.call(relay,0)
 check(not e.visibility[labels[2].addr] and not e.visibility[labels[8].addr],'actual RC25 visibility/cache loop clears stale X after metadata change')
 e.gs(596).x_mask=1<<8;BR.call(relay,0);o.put(0x118+16,struct.pack('<H',37));BR.call(relay,0)
 check(e.texts[labels[8].addr]==wide(e.traits.addr+37*0x100+0x10) and m.u32(0x34040+0xd0+32)==37 and not e.gs(596).x_mask,'actual RC25 loop replaces displayed X with an externally added real trait and tooltip ID')
 check(not BR.errors,'actual RC25 refresh plus Merit relay completes without ABI callback errors');e.closeall()

def load_order_and_failures():
 e=R3();o,r,s=e.prepare(top=1);m=preset_module(e,installed=0,loaded=False);e.fn('probe_presets_x')
 check(e.mod.val('presets_x_status')==0,'absent optional preset DLL leaves integration pending, gacha usable')
 e.preset_loaded=True;e.fn('probe_presets_x');check(e.mod.val('presets_x_status')==0,'loaded but not initialized preset module remains retryable')
 m.u32(0x1b028,1);e.fn('hook_newgame',0,0)
 check(e.mod.val('presets_x_status')==1,'successful new-game boundary binds RC25 loaded after Merit registration');e.closeall()
 e=R3();o,r,s=e.prepare(top=1);m=preset_module(e,loaded=False);e.fn('probe_presets_x');e.preset_loaded=True;e.loadsave(b'R4 legacy native save')
 check(e.mod.val('presets_x_status')==1,'successful native load boundary also binds late RC25 registration');e.closeall()
 for fault in ['text','game','image','callsite']:
  e=R3();e.prepare(top=1);m=preset_module(e)
  if fault=='text':m.put(0x2000,bytes([m.get(0x2000,1)[0]^1]))
  if fault=='game':m.u64(0x958e8,e.game.addr+16)
  if fault=='image':m.u32(PE(RC25).nt+80,0x9c000)
  if fault=='callsite':m.put(SITES[0],b'\x90')
  before=m.get(0x1000,0x15168);e.fn('probe_presets_x');e.fn('probe_presets_x')
  check(e.mod.val('presets_x_status',fmt='<i')==-1 and not e.patch_calls and m.get(0x1000,0x15168)==before,'unsupported '+fault+' preserved without writing any preset bytes')
  check(e.mod.val('enabled')==1 and sum('unrecognized/modified SortiePresets' in s for s in e.logs)==1,'optional '+fault+' mismatch leaves gacha active and logs once');e.closeall()
 e=R3();e.prepare(top=1);m=preset_module(e);e.query_mode='busy';q=e.query_calls;e.fn('probe_presets_x')
 check(e.mod.val('presets_x_status',fmt='<i')==-1 and e.query_calls-q<=192 and not e.patch_calls,'preset relay search has a hard query budget, no unbounded loading freeze');e.closeall()
 for fail_protect in [False,True]:
  e=R3();e.prepare(top=1);m=preset_module(e);before={a:m.get(a,5) for a in SITES}
  if fail_protect:
   def protection(p,n,f,out):
    if m.addr+0x1000<=p<m.addr+0x16168 and f!=0x40:
     BR.callbacks['VirtualProtect']=lambda *a:0;return 0
    put(out,0x20,'<I');return 1
   BR.callbacks['VirtualProtect']=protection
  else:
   # Fail the post-write module flush once; rollback's per-site flush succeeds.
   BR.callbacks['FlushInstructionCache']=lambda process,p,n:int(p!=m.addr)
  e.fn('probe_presets_x')
  check(e.mod.val('enabled')==1 and e.mod.val('presets_x_status',fmt='<i')==-1,'optional post-write patch failure does not disable working gacha '+str(fail_protect))
  if not fail_protect:
   check(all(m.get(a,5)==v for a,v in before.items()) and not e.mod.val('presets_x_relay',fmt='<Q'),'partial preset patch rolls back owned bytes and frees unused relay')
  else:
   check(e.mod.val('presets_rollback_live')==1 and e.mod.val('presets_x_relay',fmt='<Q') and not e.free_pages,'failed rollback protection retains callable relay memory until process exit')
  e.closeall()

if __name__=='__main__':
 try:
  for f in [actual_modcore_contract,dll_local_transaction_guards,persistent_cache_failure,api_lifetime_guards,red_traits_in_information,preset_binding_and_display,original_rc25_trait_loop,load_order_and_failures]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/r4_tests.json').write_text(json.dumps({'scope':'Actual Win64 Merit DLL / exact RC25 bytes; substituted OS/widget endpoints; NOT Windows/live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R4',len(REPORT))
