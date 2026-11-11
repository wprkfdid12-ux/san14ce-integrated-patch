"""Execute production native-integration code; substitute external OS/game calls.
No Windows, real GPU, or live game is implied. Exercises real new slot codec and
UI/Steam ABI adapters, not an independent Python implementation of the patch.
"""
from test_rc4 import *
import fnmatch

class Ref(C.Structure):
 _fields_=[('magic',C.c_uint8*8),('version',C.c_uint32),('phase',C.c_uint32),('key',C.c_uint8*32),('account',C.c_uint8*16),('current',C.c_uint32),('previous_exists',C.c_uint32),('sequence',C.c_uint64),('native',C.c_uint8*32),('body',C.c_uint8*32),('previous_native',C.c_uint8*32),('previous_body',C.c_uint8*32),('previous',C.c_uint32),('reserved',C.c_uint32),('checksum',C.c_uint8*32)]
class EN(E):
 def __init__(self):
  super().__init__();self.filename='save000.s14';self.account=76561198000000001;self.native_ok=True;self.after_native=None;self.moves=0;self.move_failure=None;self.enumerations={};self.radio=[];self.selected=[];self.hover=[];self.releases=[];self.talks=[];self.speakers=[];self.current_target=0
  self.context=self.alloc(8);self.user=self.alloc(16);self.user_vt=self.alloc(24);self.context.u64(0,self.user.addr);self.user.u64(0,self.user_vt.addr)
  self.user_vt.u64(16,BR.addr('SteamID'));self.game.put(0x26f0000,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('SteamContext')));self.game.u64(self.res.native.offsets[7],self.game.addr+0x26f0000)
  self.input=self.alloc(0x100);self.app=self.alloc(0x200);self.manager_text=self.alloc(64,'장수 고유 감사 대사\0'.encode('utf-16le'))
  BR.callbacks.update({'SteamContext':lambda p:self.context.addr,'SteamID':lambda u,p:put(p,self.account) or p,'FindFirstFileW':self.first,'FindNextFileW':self.next,'FindClose':lambda h:1,'HoverShow':lambda owner,key:self.hover.append((owner,key)), 'HoverRelease':lambda owner,destroy,r:self.releases.append((owner,destroy)), 'RadioEnable':lambda g,j,on:self.radio.append((g,j,on)), 'Selected':lambda b,on:self.selected.append((b,on)),'ResetText':lambda p:None,'Speaker':lambda m,id,idx:self.speakers.append((id,idx)),'Talk':lambda id,text:self.talks.append((id,wide(text))),'App':lambda:self.app.addr,'Input':lambda:self.input.addr,'Target':lambda app:self.current_target,'ManagerText':lambda m,id:self.manager_text.addr,'NativeWrite':self.nativewrite,'SaveEnd':lambda *a:0})
  for idx,name in {0:'RadioEnable',1:'ResetText',2:'Speaker',3:'Talk',4:'App',5:'Input',6:'Target'}.items():self.game.put(self.res.native.funcs[idx],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
  vt=self.alloc(32);vt.u64(8,BR.addr('ManagerText'));self.game.u64(self.res.native.offsets[1],vt.addr)
  self.mod.val('native_ui_ready',1);self.mod.val('shared_hover_show',BR.addr('HoverShow'),'<Q');self.mod.val('shared_hover_release',BR.addr('HoverRelease'),'<Q')
 def string(self,text):
  data=text.encode()+b'\0';obj=self.alloc(32)
  if len(data)>16:storage=self.alloc(len(data),data);obj.u64(0,storage.addr);obj.u64(24,len(data)-1)
  else:obj.put(0,data);obj.u64(24,15)
  obj.u64(16,len(data)-1);return obj
 def saveprep(self,*a):
  result=super().saveprep(*a);name=self.string(self.filename);self.stream.put(0,name.get(0,32));return result
 def nativewrite(self,*a):
  self.native_writes+=1
  if self.after_native:self.after_native()
  return int(self.native_ok)
 def move(self,p,q,flags):
  self.moves+=1
  if self.move_failure and self.move_failure(wide(p),wide(q),self.moves):return 0
  return super().move(p,q,flags)
 def emit_find(self,path,out):
  C.memset(out,0,592);put(out,0x80,'<I');writewide(out+44,path.split('\\')[-1],260)
 def first(self,pattern,out):
  found=sorted(p for p in self.files if fnmatch.fnmatchcase(p,wide(pattern)))
  if not found:self.last_error=2;return -1
  h=90000+len(self.enumerations);self.enumerations[h]=[found,0];self.emit_find(found[0],out);self.last_error=0;return h
 def next(self,h,out):
  items,index=self.enumerations[h];index+=1;self.enumerations[h][1]=index
  if index>=len(items):self.last_error=18;return 0
  self.emit_find(items[index],out);self.last_error=0;return 1
 def load(self,*a):
  if not self.load_ok:return 0
  if self.load_restore:self.load_restore()
  stream=self.alloc(0x100);name=self.string(self.filename);self.fn('hook_stream_open',stream.addr,name.addr,1,0,0,0,0);return 1
 def loadsave(self,data,filename=None):
  if filename:self.filename=filename
  self.load_buf=self.alloc(len(data),data);return self.fn('hook_load',0,0,0)
 def ref(self):
  key=C.string_at(self.mod.sym('save_slot_key'),32).hex();path=next(p for p in self.files if p.endswith(key+'.mt4ref'));return path,Ref.from_buffer_copy(self.files[path])
 def row(self,o,layout=None):
  r=super().row(o,layout);group=self.alloc(0x40);r.u64(self.res.native.offsets[0],group.addr)
  for j in range(6):
   button=self.row_buttons[j];put(get(button.addr)+0x208,BR.addr('Selected'));button.u64(8,r.addr);button.u32(0x20,j*150);button.u32(0x28,140);button.u32(0x2c,40);button.u32(0x40,1)
  r.u32(0x28,900);r.u32(0x2c,50);r.u32(0x40,1);self.active_layout.u32(0x40,1)
  self.current_target=self.row_buttons[0].addr;self.input.u32(0x34,10);self.input.u32(0x38,10);return r

def identities():
 e=EN();key=e.alloc(32);tag=e.alloc(16)
 for text in ['a.s14','scenario/manual/save001.s14','x'*1023]:
  s=e.string(text);check(e.fn('slot_identity',s.addr,key.addr,tag.addr)==1,'native filename accepted length '+str(len(text)))
  expect=hashlib.sha256(b'SAN14CE.MeritTraits.slot.v1'.ljust(32,b'\0')+struct.pack('<Q',e.account)+text.encode()).digest()
  check(key.get(0,32)==expect,'slot key includes complete exact filename and verified SteamID ABI')
 old=key.get(0,32);e.account+=1;e.fn('slot_identity',s.addr,key.addr,tag.addr);check(key.get(0,32)!=old,'different Steam accounts cannot alias identical slot names')
 e.account=0;check(e.fn('slot_identity',s.addr,key.addr,tag.addr)==0,'missing native account identity fails closed')
 e.account=1;s=e.string('x'*1024);check(e.fn('slot_identity',s.addr,key.addr,tag.addr)==0,'overlong filename rejected without out-of-bounds hash')
 e.closeall()

def slot_roundtrip():
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'identical unmodified compressed game bytes';first=bytes(e.gs(596))
 check(e.save(native)==1 and e.mod.val('save_slot_key_ok')==1,'native slot save commits actual sidecar journal')
 path,ref=e.ref();check(ref.phase==0 and ref.sequence==1 and len(e.files[path])==C.sizeof(Ref),'reference is checksummed COMMITTED not mere staging')
 e.fn('perform_roll',o.addr,0,1,0);second=bytes(e.gs(596));check(second!=first,'reroll changes sidecar without altering native game bytes')
 check(e.save(native)==1,'same native bytes with changed gacha state can overwrite SAME slot safely')
 _,ref=e.ref();check(ref.sequence==2 and ref.previous_exists==1,'previous and current gacha generations retained')
 e.filename='save001.s14';check(e.save(native)==1,'same native bytes can be stored under DIFFERENT slot binding')
 check(e.loadsave(native,'save000.s14')==1 and bytes(e.gs(596))==second and not e.mod.val('metadata_fault'),'load original slot restores latest reroll state exactly')
 # Direct reroll is legitimate pending state after load; no UI/session dependency.
 check(e.fn('perform_roll',o.addr,0,1,0)==1,'loaded pending result can spend next shared reroll')
 third=bytes(e.gs(596));check(e.save(native)==1,'third state commits despite identical native bytes')
 check(e.loadsave(native,'save001.s14')==1 and bytes(e.gs(596))==second,'other slot keeps its independent historical gacha state')
 check(e.loadsave(native,'save000.s14')==1 and bytes(e.gs(596))==third,'newer slot returns its own later state, not most recent globally')
 for _ in range(8):check(e.save(native)==1,'idempotent native save remains available')
 key=bytes(e.ref()[1].key).hex();objs=[p for p in e.files if key in p and p.endswith('.mt4dat')]
 check(len(objs)==2 and not any(p.endswith('.tmp') for p in e.files),'bounded two generations per slot and no successful-save temp debris')
 e.closeall()

def interrupted():
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);old=b'old compressed';new=b'new compressed';state=bytes(e.gs(596));assert e.save(old)
 e.fn('perform_roll',o.addr,0,1,0);newstate=bytes(e.gs(596));e.native_ok=False
 check(e.save(new)==0,'native write failure is propagated, never reported saved')
 path,ref=e.ref();check(ref.phase==1 and ref.previous_exists and not e.mod.val('metadata_fault'),'R40: failed native write preserves INTENT and both recoverable states without a fault latch')
 before=e.files[path];e.native_ok=True
 check(e.loadsave(old)==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==state,'actual old native bytes recover the correct old gacha generation')
 e.files[path]=before
 check(e.loadsave(new)==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==newstate,'actual new native bytes recover the new generation after interrupted write')
 e.closeall()
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'same bytes';assert e.save(native);e.fn('perform_roll',o.addr,0,1,0)
 e.after_native=lambda:setattr(e,'fail_move',True)
 check(e.save(native)==1,'R40: native save that really wrote is reported saved; journal stays INTENT')
 path,ref=e.ref();check(ref.phase==1 and e.native_writes==2,'native written/metadata uncertain leaves durable INTENT')
 e.fail_move=False;e.after_native=None;old=e.files[path];check(e.loadsave(native)==1 and not e.mod.val('metadata_fault'),'R40: identical-byte interrupted state loads the last committed generation')
 check(len([p for p in e.files if p.endswith('.mt4dat')])==2,'R40: both snapshots kept (reference may be committed to the older generation)')
 e.closeall()

def copies_and_corruption():
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);native=b'copy bytes';state=bytes(e.gs(596));assert e.save(native)
 check(e.loadsave(native,'renamed.s14')==1 and not e.mod.val('metadata_fault') and bytes(e.gs(596))==state,'unique same-account copied/renamed native save binds exact companion state')
 e.fn('perform_roll',o.addr,0,1,0);assert e.save(native)
 check(e.loadsave(native,'third-name.s14')==1 and not e.mod.val('metadata_fault'),'R40: ambiguous copy is not guessed: fresh merit start, no fault')
 e.closeall()
 e=EN();o,r,s=e.prepare(top=1);e.fn('perform_roll',o.addr,0,0,0);assert e.save(b'A');e.fn('perform_roll',o.addr,0,1,0);e.native_ok=False;assert not e.save(b'B');path,ref=e.ref();before=e.files[path]
 key=bytes(ref.key).hex();obj=next(p for p in e.files if p.endswith(key+f'.slot{ref.current}.mt4dat'));e.files[obj]=e.files[obj][:-1]+b'\xff'
 check(e.loadsave(b'B')==1 and not e.mod.val('metadata_fault'),'R40: corrupt interrupted snapshot -> fresh merit start, no fault')
 check(e.files[path]==before,'failed snapshot validation does not commit a recovery reference')
 e.closeall()

def native_ui():
 from r2_fixture import R2
 e=R2();o,r,s=e.prepare(top=1);e.input.u32(0x34,202);e.input.u32(0x38,102);e.fn('hover_merit_row',r.addr);check(e.hover[-1][1]==0,'unused unlock does not expose a fake description')
 e.click(r,0);e.fn('hover_merit_row',r.addr);check(e.hover[-1][1]==0,'spinning result remains hidden from shared Hover')
 e.reveal(s);e.fn('hover_merit_row',r.addr)
 check(e.hover[-1]==(r.addr,e.gsui(596).slots[0].result),'revealed result forwards the exact trait ID into existing Hover')
 check(any(j==0 and on for g,j,on in e.radio) and all(not selected for b,selected in e.selected),'native radio state is preserved for reward actions')
 e.current_target=e.app.addr;e.fn('hover_merit_row',r.addr);check(e.hover[-1][1]==0,'unrelated native modal suppresses tooltip behind it')
 e.accept_preview();check(not e.talks,'toolbar acceptance never invokes gratitude')
 e.bottom(s);check(e.talks==[(596,'장수 고유 감사 대사')] and e.pop_calls==1,'bottom decision owns one native portrait gratitude and one return')
 check((596,0) in e.speakers,'portrait speaker context uses actual officer')
 e.fn('hook_row_destroy',r.addr,e.resources.addr);check((r.addr,1) in e.releases,'destroyed merit row releases shared Hover record')
 e.closeall()


def live_targets_ai():
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
 check(v==7 and all(x.phase in (2,3) for x in e.gs(5001).slots),'initially earned AI flags are processed without waiting for another rank-up')
 before=bytes(e.gs(5001));rng=e.rng_calls;e.fn('hook_ai_visit',o.addr)
 check(bytes(e.gs(5001))==before and e.rng_calls==rng,'repeat native AI visit never duplicates accepted/declined awards')
 check(e.gs(5001).rerolls_used<=5 and b(o.addr+0x1bc)==rank and b(o.addr+0x1c2)==3,'AI uses same shared five rerolls without fabricating native rank/flags')
 check(not e.talks,'AI rewards do not open officer gratitude dialogue')
 other=e.officer(5002,top=1);e.mod.val('save_active',1);e.fn('hook_ai_visit',other.addr);check(not e.gs(5002).initialized,'AI mutation waits while native save snapshot is active')
 e.closeall()

def flush_guard():
 e=EN();stream=e.alloc(0x100);calls=[];BR.callbacks['SafeFlush']=lambda p:calls.append(p) or 1
 e.game.put(e.res.native.funcs[8],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('SafeFlush')))
 e.fn('hook_save_prep',0,0,0,0,0);stream=e.stream;e.mod.val('save_slot_key_ok',0)
 check(e.fn('hook_safe_flush',stream.addr)==0 and not calls,'missing save identity blocks before native flush can truncate the file')
 e.mod.val('save_slot_key_ok',1);check(e.fn('hook_safe_flush',stream.addr)==1 and calls==[stream.addr],'verified save identity forwards original flush')
 e.mod.val('save_slot_key_ok',0);other=e.alloc(0x100);check(e.fn('hook_safe_flush',other.addr)==1 and calls[-1]==other.addr,'unrelated game streams remain untouched by save guard')
 e.closeall()

def presentation():
 from r2_fixture import R2
 e=R2();o,r,s=e.prepare(top=1);button=e.row_buttons[0];sounds=[]
 manager=e.alloc(0x300);e.game.u64(e.res.native.offsets[18],manager.addr);BR.callbacks['Sound']=lambda m,c,a,b:sounds.append(c) or 1
 e.game.put(e.res.native.funcs[10],b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr('Sound')))
 prims=e.alloc(17*8);button.u64(0x138,prims.addr);button.u32(0x140,17)
 vt=e.alloc(0x90);vt.u64(0x18,e.game.addr+e.res.native.funcs[13]);vt.u64(0x20,e.game.addr+e.res.native.funcs[14])
 frame=e.alloc(0x300);frame.u64(0,vt.addr);frame.put(0x112,b'\x01');original=[0xff103040,0xcc304050,0x00405060,0x88607080]
 for i,c in enumerate(original):frame.u32(0x180+4*i,c)
 prims.u64(0,frame.addr);prims.u64(8,frame.addr) # duplicated native resource pointer
 txt=e.alloc(0x300);txt.u64(0,vt.addr);txt.put(0x112,b'\x02');prims.u64(16*8,txt.addr)
 params=e.alloc(0x30);params.put(0x10,struct.pack('<ff',1.0,0.9));vt.u64(0x80,BR.addr('TextParam'));BR.callbacks['TextParam']=lambda p,i:params.addr
 geometry=button.get(0x20,0x10)
 e.click(r,0);check(sounds==[0],'native click sound starts once, no external sound engine or files')
 e.tick+=150;e.fn('fx_tick');e.fn('fx_tick');check(sounds==[0,0],'spinning sound is rate limited, no repeated tick bursts')
 x=e.gsui(596).slots[0];x.anomaly=1
 e.tick+=790;e.fn('fx_tick');check([get(frame.addr+0x180+i*4,'<I') for i in range(4)]==original,'high-tier hint does not leak before final 250 milliseconds')
 e.tick+=20;e.fn('fx_tick');tease=[get(frame.addr+0x180+i*4,'<I') for i in range(4)];check(tease!=original,'native frame colors give a late high-tier tease')
 e.tick+=240;e.fn('hook_state_tick',s.addr);flash=[get(frame.addr+0x180+i*4,'<I') for i in range(4)]
 check(e.mod.val('ui_mode')==3 and flash!=tease and sounds[-1]==8,'reveal triggers distinct rare native sound and stronger original-frame emphasis')
 check(abs(get(params.addr+0x10,'<f')-1.045)<1e-6 and abs(get(params.addr+0x14,'<f')-.9405)<1e-6,'caption punch uses 1.045 scale and retains original aspect')
 check(all(a>>24==b>>24 for a,b in zip(original,flash)) and button.get(0x20,0x10)==geometry,'frame alpha and original widget rectangle remain unchanged')
 count=len(sounds);e.fn('fx_reveal');check(len(sounds)==count,'revealed result does not play a second confirmation sound')
 e.tick+=100;e.fn('fx_tick');check([get(frame.addr+0x180+i*4,'<I') for i in range(4)]==original,'all four exact vertex colors restore after 100ms, including duplicate resource pointers')
 check(params.get(0x10,8)==struct.pack('<ff',1.0,.9),'exact original font parameters restore after punch')
 e.tick+=1000;e.fn('fx_tick');check(len(sounds)==count,'idle result state has no continued sound/effects')
 e.action(0);check(sounds[-1]==17,'reroll uses distinct native reroll cue')
 e.gsui(596).slots[0].anomaly=1;e.fn('fx_reveal');e.fn('hook_row_destroy',r.addr,e.resources.addr)
 check([get(frame.addr+0x180+i*4,'<I') for i in range(4)]==original and params.get(0x10,8)==struct.pack('<ff',1.0,.9),'row destruction restores color/font state before native destruction')
 check(not BR.errors,'all presentation/native ABI boundaries executed without callback errors')
 e.closeall()

if __name__=='__main__':
 try:
  for f in [identities,slot_roundtrip,interrupted,copies_and_corruption,native_ui,live_targets_ai,flush_guard,presentation]:print('RUN',f.__name__,flush=True);f()
 finally:(ROOT/'audit/native_integration_tests.json').write_text(json.dumps({'scope':'Real Win64 DLL code; synthetic native UI/Steam/OS endpoints, not live game','passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('COMPLETE',len(REPORT))
