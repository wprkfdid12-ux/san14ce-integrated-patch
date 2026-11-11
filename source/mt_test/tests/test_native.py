"""Production PE-code tests. Synthetic API/game boundaries; NOT live SAN14/Windows.
Run on Linux x86_64 with GCC and Python 3.11+. No third-party Python modules.
"""
from native_support import *
import hashlib, math, collections, random
REPORT=[]
def check(ok,name,detail=None):
 if not ok:raise AssertionError(name)
 REPORT.append({'name':name,'pass':True,**({'detail':detail} if detail is not None else {})})
 print('PASS',name,flush=True)
def get(p,fmt='<Q'):return struct.unpack(fmt,C.string_at(p,struct.calcsize(fmt)))[0]
def put(p,v,fmt='<Q'):C.memmove(p,struct.pack(fmt,v),struct.calcsize(fmt))
def b(p):return get(p,'<B')
ARGS={'Sound':4,'TextParam':2,'SafeFlush':1,'AIEligible':1,'SteamContext':1,'SteamID':2,'HoverShow':2,'HoverRelease':3,'ResetText':1,'Speaker':3,'Talk':2,'App':0,'Input':0,'Target':1,'ManagerText':2,'FindFirstFileW':2,'FindNextFileW':2,'FindClose':1,'QueryPerformanceCounter':1,'QueryPerformanceFrequency':1,'RewardName':1,'RadioEnable':3,'Selected':2,'AcquireSRWLockExclusive':1,'ReleaseSRWLockExclusive':1,'CloseHandle':1,'CreateDirectoryW':2,'CreateFileW':7,'DeleteFileW':1,'FlushFileBuffers':1,'FlushInstructionCache':3,'GetCurrentProcess':0,'GetCurrentThreadId':0,'GetEnvironmentVariableW':3,'GetFileSizeEx':2,'GetLastError':0,'GetModuleFileNameW':3,'GetModuleHandleW':1,'GetProcAddress':2,'GetTickCount64':0,'LoadLibraryExW':3,'MoveFileExW':3,'ReadFile':5,'VirtualProtect':4,'WriteFile':5,'MessageBoxW':4,'BCryptGenRandom':4,'VirtualAlloc':4,'VirtualFree':3,'VirtualQuery':3,'GetAsyncKeyState':1,'GetForegroundWindow':0,'GetWindowThreadProcessId':2,'GetCurrentProcessId':0,
 'Readable':2,'Patch':3,'Relay':2,'FreeRelay':1,'Log':1,'Random':2,'Valid':1,'Eligible':1,'RowRefresh':1,'Status':3,'Confirm':1,'StreamOpen':7,'Load':3,'Newgame':2,'SavePrep':5,'SaveEnd':2,'Bonus':2,'CaptionColor':3,'LabelColor':2,'UIExit':1,'Text':2,'Visible':2,'Enabled':2,'GetForce':1,'GetRuler':1,'Policy':1,'Advance':1,'Pay':2,'Extra':0,'NativeWrite':4,'ApplyUI':0,'NativeCommit':1,'StateTick':1,'NativeClose':2,'PresentResume':0,'PresentFinish':0,'PopState':1,'Manager':0,'ButtonEvent':2,'RowDestroy':2,'CreateEventW':4,'CreateThread':6,'WaitForSingleObject':2,'SetEvent':1,'GetSystemTimeAsFileTime':1}
ARGS.update({'ChoiceCounts':3,'ChoiceRefresh':1,'NativeLocate':2})
ARGS.update({'WidgetAlloc':1,'WidgetFree':1,'WidgetCtor':1,'WidgetInit':7,'WidgetText':2,'WidgetBind':3,'CallbackType':0,'DynamicText':6,'PrimText':2,'PrimSize':3,'PrimPos':3,'Align':2,'WidgetRegister':1,'WidgetUnregister':1,'WidgetDestroy':2,'WidgetDelete':2,'WidgetLayer':2,'Font':2,'CollectionSize':1,'CollectionReserve':2})
ARGS.update({'CancelButton':2,'ConsumeInput':0,'TraitAt':2,'CollectTraits':7,'ColumnFill':4,'ColumnOriginal':1,'InfoOriginal':1,'InfoPerson':1,'LabelGet':2,'InfoCache':2,'InfoBuild':2,'UnitPerson':1})
ARGS.update({'PresetsRefresh':1,'PresetsEntity':3,'PresetsStatus':1})
ARGS.update({'LayoutCtor':1,'CollectionCtor':1,'BalloonInit':2,'BalloonAlpha':2,'CollectionWidth':3,'CollectionHeight':3,'CollectionVisible':3,'Window':5,'PrimAlpha':2,'WidgetPosition':3,'NativePicker':1})
BR=Bridge(ARGS)
class Profile(C.Structure):_fields_=[('id',C.c_uint16),('mask',C.c_uint8),('origin',C.c_uint8),('initial',C.c_uint8*5),('valid',C.c_uint8),('awards',C.c_uint16*3),('award_mask',C.c_uint8),('reserved',C.c_uint8*3)]
class Hook(C.Structure):_fields_=[('rva',C.c_uint32),('length',C.c_uint8),('kind',C.c_uint8),('is_vtable',C.c_uint8),('pad',C.c_uint8),('before',C.c_uint8*16),('target',C.c_uint32)]
class NativeConfig(C.Structure):_fields_=[('funcs',C.c_uint32*27),('offsets',C.c_uint32*19),('guards',(C.c_uint8*16)*27),('hover_hash',C.c_uint8*32),('guard_lengths',C.c_uint8*27)]
class SelectionConfig(C.Structure):_fields_=[("counts_site",C.c_uint32),("refresh_site",C.c_uint32),("tree_offset",C.c_uint32),("counts_call",C.c_uint8*8),("refresh_call",C.c_uint8*8)]
class PopupConfig(C.Structure):_fields_=[("funcs",C.c_uint32*9),("guards",(C.c_uint8*16)*9),("vtable",C.c_uint32),("object_size",C.c_uint32)]
class Resolver(C.Structure):_fields_=[('size',C.c_uint32),('abi',C.c_uint32),('timestamp',C.c_uint32),('image_size',C.c_uint32),('officer_count',C.c_uint32),('trait_count',C.c_uint32),('funcs',C.c_uint32*21),('offsets',C.c_uint32*23),('guards',(C.c_uint8*16)*21),('hook_count',C.c_uint32),('hooks',Hook*48),('patch_rva',C.c_uint32*5),('before',(C.c_uint8*8)*5),('after',(C.c_uint8*8)*5),('length',C.c_uint8*5),('pad',C.c_uint8*3),('native',NativeConfig),('selection',SelectionConfig),('popup',PopupConfig)]
class BaseResolver(C.Structure):_fields_=[('size',C.c_uint32),('abi',C.c_uint32),('layout',C.c_uint32),('fn_count',C.c_uint32),('offset_count',C.c_uint32),('image_size',C.c_uint32),('game',C.c_void_p),('functions',C.c_void_p),('offsets',C.c_void_p),('extra',C.c_void_p)]
class CorePatch(C.Structure):_fields_=[('address',C.c_void_p),('length',C.c_uint32),('before',C.c_uint8*16),('after',C.c_uint8*16),('pad',C.c_uint32)]
class ModCore(C.Structure):_fields_=[('size',C.c_uint32),('abi',C.c_uint32),('resolver',C.c_void_p),('readable',C.c_void_p),('patch',C.c_void_p),('relay',C.c_void_p),('free_relay',C.c_void_p),('log',C.c_void_p)]
CAT=json.loads((ROOT/'audit/개성분류표.json').read_text())['traits']
class Env:
 def __init__(self):
  self.mem=[];self.files={};self.handles={};self.last_error=0;self.logs=[];self.messages=[];self.rng_calls=0;self.random_values=[];self.rng_fail=False;self.tick=10000;self.fail_write=False;self.fail_flush=False;self.fail_move=False;self.native_writes=0;self.native_commits=0;self.fail_commit=False;self.load_ok=1;self.load_buf=None;self.load_restore=None;self.eligible=True;self.texts={};self.visibility={};self.colors=[];self.patch_calls=[];self.patch_fail_at=0;self.free_relays=[];self.relay_offset=0x2700000;self.api_counts=collections.Counter();self.keys=set();self.close_calls=0;self.pop_calls=0;self.dialogs=0;self.status_override=None;self.query_mode='free';self.alloc_fail=False;self.alloc_calls=0;self.query_calls=0;self.free_pages=[];self.text_history=[]
  BR.errors.clear();BR.callbacks={name:(lambda *a:0) for name in ARGS};BR.callbacks.update({'AcquireSRWLockExclusive':lambda p:None,'ReleaseSRWLockExclusive':lambda p:None,'GetCurrentThreadId':lambda:42,'GetCurrentProcess':lambda:123,'GetTickCount64':self.gettick,'QueryPerformanceCounter':lambda p:put(p,self.tick*1000) or 1,'QueryPerformanceFrequency':lambda p:put(p,1000000) or 1,'Readable':self.readable,'Log':lambda p:self.logs.append(C.string_at(p).decode()),'MessageBoxW':lambda h,s,t,f:self.messages.append(wide(s)) or 1,'BCryptGenRandom':self.bcrypt,'Random':self.rand,
  'CreateFileW':self.create,'WriteFile':self.write,'ReadFile':self.read,'CloseHandle':self.close,'GetFileSizeEx':self.size,'GetLastError':lambda:self.last_error,'FlushFileBuffers':lambda h:not self.fail_flush,'MoveFileExW':self.move,'DeleteFileW':self.delete,'CreateDirectoryW':lambda p,s:1,'GetEnvironmentVariableW':lambda p,o,n:writewide(o,'C:\\TestUser\\Local',n),'GetModuleFileNameW':lambda m,o,n:writewide(o,'C:\\Game\\SAN14CE_KO.exe',n),'FlushInstructionCache':lambda *a:1,'VirtualProtect':lambda p,n,f,o:put(o,0x20,'<I') or 1,'VirtualAlloc':self.virtualalloc,'VirtualFree':lambda p,n,f:self.free_pages.append(p) or 1,'VirtualQuery':self.virtualquery,'GetAsyncKeyState':self.getkey,'GetForegroundWindow':lambda:123,'GetWindowThreadProcessId':lambda h,o:put(o,777,'<I') or 1,'GetCurrentProcessId':lambda:777,
  'LoadLibraryExW':self.loadlib,'GetModuleHandleW':self.getmodule,'GetProcAddress':self.getproc,'Patch':self.patch,'Relay':self.relay,'FreeRelay':lambda p:self.free_relays.append(p),'Valid':lambda p:bool(p),'Eligible':lambda p:int(self.eligible),'RowRefresh':self.row_refresh,'Status':self.status,'Confirm':self.confirm,'StreamOpen':self.streamopen,'Load':self.load,'Newgame':lambda *a:1,'SavePrep':self.saveprep,'SaveEnd':lambda *a:1,'Bonus':lambda o,k:sum((500,500,1000)[:b(o+0x1c2)]) if k==0 else 0,'CaptionColor':lambda p,i,col:self.colors.append((p,i,col)),'LabelColor':lambda *a:(_ for _ in ()).throw(AssertionError('wrong color primitive')),'UIExit':lambda x:None,'Text':self.settext,'Visible':lambda o,v:self.visibility.__setitem__(o,v),'Enabled':lambda o,v:None,'NativeWrite':self.nativewrite,'ApplyUI':self.applyui,'NativeCommit':self.commit,'StateTick':self.state_tick,'NativeClose':self.native_close,'PopState':self.pop_state,'Manager':lambda:0x1234})
  self.mod=Loaded(ROOT/'source/build/SAN14_MeritTraits.dll',BR,ROOT/'source/build/merit.map');self.mem.append(self.mod)
  self.resolverdll=Loaded(ROOT/'source/build/SAN14_MeritResolver_CEKO.dll',mapfile=ROOT/'source/build/resolver.map');self.mem.append(self.resolverdll)
  self.game=self.alloc(0x2b10000);self.game.put(0,b'MZ');self.game.u32(0x3c,0x100);self.game.u32(0x100,0x4550);self.game.put(0x104,b'\x64\x86');self.game.u32(0x108,0x6a6d9f72);self.game.put(0x118,b'\x0b\x02');self.game.u32(0x150,0x29ff000)
  identity=json.loads((ROOT/'analysis/native_row_identity.json').read_text())
  self.native_row_vtable=identity['row_vtable_rva']
  for block in identity['blocks']:
   data=bytearray.fromhex(block['bytes'])
   for off in block['relocate_u64_offsets']:
    v=struct.unpack_from('<Q',data,off)[0]-identity['preferred_base']+self.game.addr
    struct.pack_into('<Q',data,off,v)
   self.game.put(block['rva'],bytes(data))
  self.resaddr=BR.call(self.resolverdll.sym('SAN14_MeritResolveV1'),self.game.addr,0x29ff000);assert self.resaddr
  self.res=Resolver.from_address(self.resaddr);assert self.res.size==C.sizeof(Resolver)
  for r,g in zip(self.res.funcs,self.res.guards):self.game.put(r,bytes(g))
  for r,g in zip(self.res.native.funcs,self.res.native.guards):self.game.put(r,bytes(g))
  for r,g in zip(self.res.popup.funcs,self.res.popup.guards):self.game.put(r,bytes(g))
  for site,expected in [(self.res.selection.counts_site,self.res.selection.counts_call),(self.res.selection.refresh_site,self.res.selection.refresh_call)]:self.game.put(site,bytes(expected[:5]))
  for h in self.res.hooks:self.game.put(h.rva,struct.pack('<Q',self.game.addr+h.target) if h.is_vtable else bytes(h.before[:h.length])) if h.length else None
  for name,block in json.loads((ROOT/'analysis/r11_native_boundaries.json').read_text())['blocks'].items():
   # R12 also validates the captured status accessor. Populate real bytes; do not weaken its guard.
   if name in ('eligible','rank','current_rank','ruler_status'):self.game.put(block['rva'],bytes.fromhex(block['bytes']))
  bonus=json.loads((ROOT/'analysis/r15_domestic_native.json').read_text())['blocks']['merit_bonus']
  self.game.put(bonus['rva'],bytes.fromhex(bonus['bytes']))
  for block in json.loads((ROOT/'analysis/r16_trait_native.json').read_text())['blocks'].values():self.game.put(block['rva'],bytes.fromhex(block['bytes']))
  for i,n in enumerate(self.res.length):self.game.put(self.res.patch_rva[i],bytes(self.res.before[i][:n]))
  self.base=BaseResolver(C.sizeof(BaseResolver),65536,1,0,0,0x29ff000,self.game.addr,None,None,None)
  self.core=ModCore(C.sizeof(ModCore),65536,C.addressof(self.base),*(BR.addr(k)for k in ('Readable','Patch','Relay','FreeRelay','Log')))
  self.db=self.alloc(0x90000);self.game.u64(0x277df18,self.db.addr)
  self.traits=self.alloc(355*0x100)
  for t in CAT:
   idx=t['id'];o=idx*0x100
   if idx>=355:continue
   self.db.u64(0x7dd18+idx*8,self.traits.addr+o);self.traits.put(o+0x10,(t['name']+'\0').encode('utf-16le'));self.traits.put(o+0xbe,bytes([t['color']]))
  for t in json.loads((ROOT/'data/SAN14CE_개성마스터219_DB.json').read_text()):
   idx=t['trait_id'];o=idx*0x100;raw=bytes.fromhex(t['raw_param_hex'])
   for off,v in [(0xa2,b'\xff\xff'),(0xa4,raw[:1]),(0xb4,raw[2:4]),(0xb6,raw[4:5]),(0xb8,raw[5:7]),(0xba,raw[7:8]),(0xbc,raw[8:10])]:self.traits.put(o+off,v)
  self.officers={};self.nodes=[]
  def base_counts(owner,out,id):
   o=self.officers.get(id);put(out,(b(o.addr+0x1c2)|(b(o.addr+0x1c3)<<8)) if o else 0,'<H');return out
  BR.callbacks['ChoiceCounts']=base_counts
  for rva,name in [(0x6dbdc0,'ChoiceCounts'),(0x781810,'ChoiceRefresh')]:self.game.put(rva,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
  for i,a in enumerate((500,500,1000)):
   obj=self.alloc(32);obj.put(0x10,b'\0');obj.put(0x12,struct.pack('<H',a));self.db.u64(0x85df0+(i+1)*8,obj.addr)
  self.mod.val('game',self.game.addr,'<Q');self.mod.val('res',self.resaddr,'<Q');self.mod.val('core',C.addressof(self.core),'<Q')
  ui_pe=PE(ROOT/'payload/SAN14Mods/MeritTraits/UI_Traits9_Hover.dll');self.ui=self.alloc(ui_pe.size+0x20000,bytes(ui_pe.m));self.ui.u64(0x32d68,self.game.addr);self.ui_loaded=False
  self.path_ready=False
 def alloc(self,n,data=b''):
  m=Mapping(n,data);self.mem.append(m);return m
 def readable(self,p,n):
  self.api_counts['readable']+=1
  if not p or not n:return 0
  # All game-owned pointers checked by the runtime are mapped and tracked.
  return any(m.addr<=p and p+n<=m.addr+m.size for m in self.mem)
 def closeall(self):
  for m in reversed(self.mem):m.close()
 def fn(self,name,*a):return BR.call(self.mod.sym(name),*a)
 def activate(self):
  self.fn('paths');self.mod.val('enabled',1)
  names=['NativeCommit','Bonus','RowRefresh','Status','Confirm','StreamOpen','Load','Newgame','SavePrep','SaveEnd','Valid','LabelColor','CaptionColor','UIExit','Eligible','StateTick','NativeClose','PresentResume','PresentFinish','ButtonEvent','RowDestroy']
  for r,name in zip(self.res.funcs,names):self.game.put(r,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
 def profile(self,id):return Profile.from_address(self.mod.sym('profiles')+id*C.sizeof(Profile))
 def officer(self,id=10,stats=(90,96,76,70,80),owned=(4,37,51,62,65),top=0,bottom=0):
  o=self.alloc(0x200);o.put(0x10,struct.pack('<H',id));o.put(0xec,bytes(stats));o.put(0x118,struct.pack('<9H',*owned,*([0]*(9-len(owned)))));o.put(0x1c2,bytes([top,bottom]));self.officers[id]=o;self.db.u64(0x158+8*id,o.addr);return o
 def batch(self,items):
  # Native ordered std::map layout with a sentinel and a balanced tree.
  head=self.alloc(0x30);head.put(0x19,b'\1');nodes=[]
  for id,top,bot in sorted(items):
   x=self.alloc(0x30);x.u32(0x1c,id);x.put(0x20,bytes([(bot<<4)|top]));nodes.append(x)
  def tree(ns,parent):
   if not ns:return head.addr
   k=len(ns)//2;node=ns[k];node.u64(8,parent);node.u64(0,tree(ns[:k],node.addr));node.u64(0x10,tree(ns[k+1:],node.addr));return node.addr
  root=tree(nodes,head.addr);head.u64(0,nodes[0].addr if nodes else head.addr);head.u64(8,root);head.u64(16,nodes[-1].addr if nodes else head.addr)
  batch=self.alloc(0x20);batch.u64(0,head.addr);batch.u64(8,len(nodes));batch.u64(16,0)
  self.nodes=nodes;return batch
 def commit(self,ptr):
  self.native_commits+=1
  if self.fail_commit:return 0
  head=get(ptr);node=get(head)
  seen=set()
  def visit(n):
   if n==head:return
   if n in seen:raise ValueError('cyclic batch')
   seen.add(n);visit(get(n));id=get(n+0x1c,'<I');v=b(n+0x20);o=self.officers.get(id)
   if o:o.put(0x1c2,bytes([v&15,v>>4]));o.put(0x1bc,bytes([b(o.addr+0x1bc)+1]))
   visit(get(n+16))
  visit(get(head+8));return 1
 def rand(self,ctx,p):
  self.rng_calls+=1
  if self.rng_fail:return 0
  x=self.random_values.pop(0) if self.random_values else ((0 if self.rng_calls%2 else 100) if getattr(self,'ordinary_roll_fixture',False) else int.from_bytes(os.urandom(4),'little'));put(p,x,'<I');return 1
 def bcrypt(self,h,p,n,flags):
  assert h==0 and n==4 and flags==2
  return 0 if self.rand(0,p) else 0xc0000001
 def create(self,p,access,share,sec,creation,flags,template):
  path=wide(p);creation &= 0xffffffff
  if creation==2:self.files[path]=b''
  if path not in self.files:self.last_error=2;return -1
  h=100+len(self.handles);self.handles[h]=[path,0];return h
 def write(self,h,p,n,out,over):
  if self.fail_write:return 0
  path,pos=self.handles[h];self.files[path]=C.string_at(p,n);put(out,n,'<I');return 1
 def read(self,h,p,n,out,over):
  path,pos=self.handles[h];data=self.files[path][pos:pos+n];C.memmove(p,data,len(data));put(out,len(data),'<I');self.handles[h][1]+=len(data);return 1
 def size(self,h,out):put(out,len(self.files[self.handles[h][0]]));return 1
 def close(self,h):return 1
 def delete(self,p):self.files.pop(wide(p),None);return 1
 def move(self,p,q,flags):
  if self.fail_move:return 0
  self.files[wide(q)]=self.files.pop(wide(p));return 1
 def nativewrite(self,*a):self.native_writes+=1;return 1
 def loadlib(self,p,h,flags):
  s=wide(p)
  if s.endswith('SAN14_MeritResolver_CEKO.dll'):return self.resolverdll.addr
  if s.endswith('UI_Traits9_Hover.dll'):self.ui_loaded=True;return self.ui.addr
  raise ValueError(s)
 def getmodule(self,p):return self.ui.addr if self.ui_loaded and wide(p)=='UI_Traits9_Hover.dll' else 0
 def getproc(self,h,p):
  s=C.string_at(p).decode()
  return self.resolverdll.sym('SAN14_MeritResolveV1') if s=='SAN14_MeritResolveV1' else BR.addr('ApplyUI') if s=='SAN14_ApplyTraits9UI' else 0
 def applyui(self):self.game.put(0x4915a1,b'\xff\x25\0\0\0\0'+struct.pack('<Q',self.ui.addr+0x16a0));return 1
 def patch(self,name,p,n):
  self.patch_calls.append((C.string_at(name).decode(),n))
  if self.patch_fail_at==len(self.patch_calls):return 0
  items=(CorePatch*n).from_address(p)
  for item in items:
   # Match the supplied Core, whose registry accepts GAME-image RVAs only.
   if not self.game.addr<=item.address<=self.game.addr+self.res.image_size-item.length:return 0
   assert self.readable(item.address,item.length)
   if C.string_at(item.address,item.length)!=bytes(item.before[:item.length]):return 0
  for item in items:C.memmove(item.address,bytes(item.after[:item.length]),item.length)
  return 1
 def relay(self,p,target):
  addr=self.game.addr+self.relay_offset;self.relay_offset+=32;C.memmove(addr,b'\xff\x25\0\0\0\0'+struct.pack('<Q',target),14);return addr
 def gettick(self):self.api_counts['timer']+=1;return self.tick
 def getkey(self,k):self.api_counts['keys']+=1;return 0x8000 if k in self.keys else 0
 def settext(self,o,t):
  s=wide(t);self.texts[o]=s;self.text_history.append((self.tick,o,s))
 def virtualquery(self,p,out,size):
  self.query_calls+=1;assert size==48
  if self.query_mode=='invalid':return 0
  if self.query_mode=='busy':
   base=p & ~65535;region=65536;state=0x1000
  else:
   base=p;region=0x100000;state=0x10000
  C.memmove(out,struct.pack('<QQIIQIIII',base,0,0,0,region,state,0,0,0),48);return 48
 def virtualalloc(self,p,n,flags,prot):
  self.alloc_calls+=1;assert n==4096 and flags==0x3000 and prot==4
  if self.alloc_fail:return 0
  assert self.readable(p,n);return p
 def status(self,layout,person,out):
  vals=self.status_override or [2 if i<b(person+0x1c2) else 1 if i==b(person+0x1c2) else 3 for i in range(3)]+[1,3,3]
  C.memmove(out,struct.pack('<6i',*vals),24)
 def row_refresh(self,r):
  o=get(r+0x178);a=(C.c_int*6)();self.fn('hook_status',get(r+8),o,C.addressof(a))
  for i,status in enumerate(a):
   text=get(r+0x180+8*i);button=get(r+0x1b8+8*i);label=get(r+0x1e8+8*i);icon=get(r+0x218+8*i)
   self.visibility[text]=int(status==2);self.visibility[button]=int(status!=2)
   self.visibility[label]=self.visibility[icon]=int(status==3)
   self.texts[button]='' if status==3 else '지휘병사 +500' if i<3 else '담당지역 +1'
   self.texts[text]='지휘병사 +500' if i<3 else '담당지역 +1'
   self.texts[label]='해금 : 노련'
 def native_close(self,layout,code):self.close_calls+=1;self.visibility[layout]=0;return 0
 def pop_state(self,manager):
  self.pop_calls+=1
  if hasattr(self,'active_layout'):self.fn('hook_ui_exit',self.active_layout.addr)
 def state_tick(self,state):
  layout=get(state+0x4c8)
  if get(layout+0x180,'<I')==1 and self.fn('hook_confirm',state):self.pop_state(0)
 def confirm(self,state):
  result=self.fn('hook_commit_player',self.confirm_batch.addr)
  if result:
   layout=get(state+0x4c8)
   if not self.mod.val('merit_presentation_skip',fmt='<Q'):
    self.dialogs+=1;self.visibility[layout]=0
   self.fn('hook_close',layout,0x7ffffffd)
  return result
 def load(self,*a):
  if not self.load_ok:return 0
  if self.load_restore:self.load_restore()
  s=self.alloc(0x100);self.fn('hook_stream_open',s.addr,0,1,0,0,0,0);return 1
 def streamopen(self,s,*a):
  if self.load_buf:
   put(s+0x40,self.load_buf.addr);put(s+0x24,self.load_buf.size,'<I')
  return 1
 def saveprep(self,*a):
  self.stream=self.alloc(0x100);obj=self.alloc(0x30);obj.u64(0x10,self.stream.addr);return obj.addr
 def save(self,data):
  m=self.alloc(len(data),data);a=self.fn('hook_save_prep',0,0,0,0,0);assert a
  vt=self.alloc(0x80);vt.u64(0x58,BR.addr('NativeWrite'));vt.u64(0,BR.addr('NativeWrite'));o=self.alloc(0x30);o.u64(0,vt.addr)
  ok=self.fn('merit_write_file',o.addr,m.addr,m.size,self.stream.addr);self.fn('hook_save_end',0,a);return ok
 def owned(self,o):return struct.unpack('<9H',o.get(0x118,18))
 def row(self,o,layout=None):
  row=self.alloc(0x280);row.u64(0,self.game.addr+self.native_row_vtable);row.u64(0x178,o.addr);self.row_text=[];self.row_buttons=[];self.row_labels=[];self.row_icons=[]
  vt=self.alloc(0x240);vt.u64(0x1d8,BR.addr('Text'));vt.u64(0xc8,BR.addr('Visible'));vt.u64(0xd0,BR.addr('Enabled'));vt.u64(0x208,BR.addr('Selected'))
  if layout is None:layout=self.alloc(0x300)
  if not layout.u64(0):layout.u64(0,vt.addr)
  row.u64(8,layout.addr);self.active_layout=layout;self.visibility[layout.addr]=1
  for i in range(6):
   text=self.alloc(0x200);button=self.alloc(0x200);label=self.alloc(0x200);icon=self.alloc(0x200)
   for obj in (text,button,label,icon):obj.u64(0,vt.addr)
   row.u64(0x180+8*i,text.addr);row.u64(0x1b8+8*i,button.addr);row.u64(0x1e8+8*i,label.addr);row.u64(0x218+8*i,icon.addr)
   self.row_text.append(text);self.row_buttons.append(button);self.row_labels.append(label);self.row_icons.append(icon)
  return row

def test_core():
 e=Env();e.activate();u16=C.c_uint16
 for name,data in [('empty',b''),('abc',b'abc'),('long',b'x'*10000),('unaligned',bytes(range(256))*7)]:
  src=C.create_string_buffer(data);out=(C.c_uint8*32)();e.fn('mc_sha256',C.addressof(src),len(data),C.addressof(out));check(bytes(out)==hashlib.sha256(data).digest(),'SHA256 '+name)
 for stats,mask in [((96,91,80,70,60),1),((91,96,76,70,80),2),((90,30,100,95,90),4),((80,30,90,99,95),8),((75,65,80,88,99),16),((99,99,20,30,40),3),((0,0,0,0,0),31),((200,200,200,200,200),31)]:
  a=(C.c_uint8*5)(*stats);check(e.fn('mc_type',C.addressof(a))==mask,'fixed top-stat mask '+str(stats))
 owned=(u16*9)();out=(u16*355)();active=[t for t in CAT if t['enabled']]
 check(len(active)==185 and sum(t['tier']==2 for t in active)==56,'185 unique actual-ID blue/gold catalog entries')
 for mask in [1,2,4,8,16,3,31]:
  for stage in [0,1,2]:
   n=e.fn('mc_pool',mask,stage,C.addressof(owned),C.addressof(out));actual=list(out[:n]);expected=[t['id']for t in active if t['mask']&mask and t['tier']==(2 if stage==2 else 1)]
   check(actual==expected and len(set(actual))==n,'pool tier/unique-ID/type '+str((mask,stage)))
 owned[0]=151
 n=e.fn('mc_pool',31,2,C.addressof(owned),C.addressof(out));x=list(out[:n]);check(all(t not in x for t in (151,157,163,204)),'Wolong family duplicate filter');check(148 in x or any(t['name']=='허실'and t['id']in x for t in active),'nonduplicate gold synergy candidates retained')
 for a in [(),(4,37,51,62,65),(4,37,51,62,65,73,80,100,120)]:
  ar=(u16*9)(*a);check((e.fn('mc_slot',C.addressof(ar))&0xffffffff)==(len(a) if len(a)<9 else 0xffffffff),'empty slot scan '+str(len(a)))
 ar=(u16*9)(4,37);before=bytes(ar);check(e.fn('mc_insert',C.addressof(ar),4)==0 and bytes(ar)==before,'existing trait never inserted twice');check(e.fn('mc_insert',C.addressof(ar),100)==1 and ar[2]==100,'native-slot addition preserves previous slots')
 result=C.c_uint32();e.random_values=[0,1,2,3,4,5,6];before=e.rng_calls
 check(e.fn('mc_uniform',BR.addr('Random'),0,10,C.addressof(result))==1 and result.value==6 and e.rng_calls-before==7,'rejection threshold removes modulo bias (n=10)')
 e.random_values=[123,123];check(e.fn('mc_uniform',BR.addr('Random'),0,17,C.addressof(result))==1,'uniform draw first');first=result.value
 check(e.fn('mc_uniform',BR.addr('Random'),0,17,C.addressof(result))==1 and result.value==first,'identical consecutive RNG values are not suppressed')
 e.rng_fail=True;check(e.fn('mc_uniform',BR.addr('Random'),0,17,C.addressof(result))==0,'RNG failure fails closed');e.rng_fail=False
 check(e.fn('mc_uniform',BR.addr('Random'),0,0,C.addressof(result))==0,'empty candidate set safe')
 e.random_values=[0]*128;check(e.fn('mc_uniform',BR.addr('Random'),0,10,C.addressof(result))==0,'unusable entropy retry bounded');e.random_values=[]
 # Frequency test is a diagnostic, not proof of independence of the Windows API.
 for n in [3,17,31]:
  counts=[0]*n;samples=51000
  for _ in range(samples):assert e.fn('mc_uniform',BR.addr('Random'),0,n,C.addressof(result));counts[result.value]+=1
  x2=sum((v-samples/n)**2/(samples/n)for v in counts)
  check(x2<n-1+8*math.sqrt(2*(n-1)),'uniform frequency diagnostic n='+str(n),{'samples':samples,'counts':counts,'chi2':x2,'test_entropy':'os.urandom stub, not Windows BCrypt'})
 e.closeall()

def test_runtime():
 e=Env();e.activate();o=e.officer();batch=e.batch([(10,1,0)]);original=e.owned(o);e.random_values=[10000]
 check(e.fn('hook_commit_player',batch.addr)==1,'player award commit');first=e.profile(10).awards[0]
 check(e.owned(o)[:5]==original[:5] and e.owned(o)[5]==first and e.profile(10).award_mask==1,'real-slot ID and awarded-slot metadata update together')
 check(e.fn('hook_bonus',o.addr,0)==0,'replaced +500 excluded from troop bonus')
 rng=e.rng_calls;row=e.row(o);e.fn('hook_row_refresh',row.addr);e.tick+=1000;e.fn('hook_row_tick',row.addr)
 check(e.texts[e.row_buttons[0].addr]==CAT[first]['name'],'one-second display resolves to native trait name')
 check(e.colors[-1]==(e.row_buttons[0].addr+0x130,16,15),'native resource caption palette path (no invalid text pointer)')
 for i in range(100):e.fn('hook_row_refresh',row.addr);e.fn('hook_row_tick',row.addr)
 check(e.rng_calls==rng and get(e.mod.sym('roll_started')+(10*3)*8)==0,'100 reopens do not consume RNG or reroll')
 # Already-completed rank can advance without giving duplicate trait.
 before=e.owned(o);check(e.fn('hook_commit_player',batch.addr)==1 and e.owned(o)==before and e.rng_calls==rng,'native rank advance with unchanged upper count gives no second reward')
 saved=o.get(0,0x200);check(e.save(b'fixture-save-A')==1,'post-award native save plus sidecar succeeds');filesA=dict(e.files);meta=[p for p in e.files if p.endswith('.mtdat')];check(len(meta)==1 and hashlib.sha256(b'fixture-save-A').hexdigest()in meta[0],'sidecar bound to exact compressed save bytes')
 e.load_buf=e.alloc(len(b'fixture-save-A'),b'fixture-save-A');e.load_restore=lambda:o.put(0,saved)
 check(e.fn('hook_load',0,1,0)==1 and e.profile(10).awards[0]==first,'post-award save reload restores fixed result');check(get(e.mod.sym('roll_started')+(10*3)*8)==0,'loading acquired save never repeats reveal')
 o.put(0xec,bytes([100,1,1,1,1]));check(e.profile(10).mask==2,'later stat growth does not change captured type')
 # Legacy save: does NOT retroactively roll old top rank; first-observed type labeled origin2.
 e.load_buf=e.alloc(11,b'legacy-save');e.load_restore=lambda:o.put(0,saved)
 check(e.fn('hook_load',0,1,0)==1 and e.profile(10).award_mask==0 and e.profile(10).origin==2,'legacy load preserves old rewards and identifies fallback base stats')
 check(e.fn('hook_bonus',o.addr,0)==500,'legacy top +500 stays native')
 batch2=e.batch([(10,2,0)]);e.random_values=[22222];check(e.fn('hook_commit_ai',batch2.addr)==1,'AI uses same award path');second=e.profile(10).awards[1]
 check(second!=first and e.owned(o)[6]==second and e.profile(10).award_mask==2,'AI owned-ID filter and slot7 write');check(get(e.mod.sym('roll_started')+(10*3+1)*8)==0,'AI has no reveal animation');check(e.fn('hook_bonus',o.addr,0)==500,'mixed legacy/new rewards deduct only converted stage')
 # Final stage gold including collaboration candidates.
 b3=e.batch([(10,3,0)]);check(e.fn('hook_commit_player',b3.addr)==1,'final gold award path');gold=e.profile(10).awards[2];check(CAT[gold]['tier']==2 and e.owned(o)[7]==gold,'gold native-ID written to slot8')
 # No-space refuses fee/command invocation, lower path remains available.
 full=e.officer(11,owned=(4,37,51,62,65,70,72,73,74));bfull=e.batch([(11,1,0)]);n=e.native_commits;before=full.get(0,0x200)
 check(e.fn('hook_commit_player',bfull.addr)==0 and n==e.native_commits and full.get(0,0x200)==before,'9/9 capacity refusal before native cost or rank change')
 check(e.fn('hook_commit_ai',bfull.addr)==1 and b(full.addr+0x1c2)==0 and b(full.addr+0x1c3)==1,'AI full-slot top request falls back to existing lower reward')
 # Batch preflight atomicity, corrupted map, RNG.
 other=e.officer(12);ba=e.batch([(12,1,0),(11,1,1)]);before=other.get(0,0x200);n=e.native_commits
 check(e.fn('hook_commit_player',ba.addr)==0 and n==e.native_commits and other.get(0,0x200)==before,'multi-officer capacity preflight prevents partial grant/cost')
 ba=e.batch([(12,1,0)]);e.rng_fail=True;check(e.fn('hook_commit_player',ba.addr)==0 and n==e.native_commits,'entropy failure does not commit');e.rng_fail=False
 e.fail_commit=True;check(e.fn('hook_commit_player',ba.addr)==0 and other.get(0,0x200)==before and e.profile(12).award_mask==0,'native commit failure grants no traits');e.fail_commit=False
 e.load_ok=0;pbytes=C.string_at(e.mod.sym('profiles'),6000*20);check(e.fn('hook_load',0,1,0)==0 and C.string_at(e.mod.sym('profiles'),6000*20)==pbytes,'failed load preserves prior metadata');e.load_ok=1
 # Corrupt companion must not be mistaken for a legacy/no-metadata save.
 e.files=filesA;e.files[meta[0]]=e.files[meta[0]][:-1]+b'\x99';e.load_buf=e.alloc(14,b'fixture-save-A');e.load_restore=lambda:o.put(0,saved)
 check(e.fn('hook_load',0,1,0)==1 and e.mod.val('metadata_fault')==1,'corrupt companion fails closed for new trait rewards')
 bnew=e.batch([(10,2,0)]);n=e.native_commits;check(e.fn('hook_commit_player',bnew.addr)==0 and n==e.native_commits,'corrupt companion cannot trigger replacement draw')
 check(e.fn('hook_save_prep',0,0,0,0,0)==0,'invalid metadata cannot silently overwrite associated game save')
 e.closeall()

def test_rewind_and_savefail():
 e=Env();e.activate();o=e.officer();before=o.get(0,0x200);e.fn('hook_newgame',0,0);check(e.profile(10).origin==1,'new scenario base captured once')
 check(e.save(b'pre-award-save')==1,'pre-award metadata saved')
 snapshots=[]
 for draw in [10000,10001,10000]:
  e.load_buf=e.alloc(14,b'pre-award-save');e.load_restore=lambda:o.put(0,before);e.fn('hook_load',0,1,0);batch=e.batch([(10,1,0)]);e.random_values=[draw];assert e.fn('hook_commit_player',batch.addr);snapshots.append(e.profile(10).awards[0])
 check(snapshots[0]!=snapshots[1] and snapshots[0]==snapshots[2],'pre-award reload: fresh RNG, repeats allowed, no history bias')
 e.fail_write=True;n=e.native_writes;check(e.save(b'cannot-save-A')==0 and n==e.native_writes,'sidecar WriteFile failure blocks native overwrite');e.fail_write=False
 e.fail_flush=True;n=e.native_writes;check(e.save(b'cannot-save-B')==0 and n==e.native_writes,'sidecar flush failure blocks native overwrite');e.fail_flush=False
 e.fail_move=True;n=e.native_writes;check(e.save(b'cannot-save-C')==0 and n==e.native_writes,'sidecar atomic-rename failure blocks native overwrite');e.fail_move=False
 # Confirm keeps the real row alive, skips native presentation, and defers close.
 e.load_buf=e.alloc(14,b'pre-award-save');e.fn('hook_load',0,1,0);e.confirm_batch=e.batch([(10,1,0)])
 state=e.alloc(0x600);row=e.row(o);layout=e.active_layout;state.u64(0x4c8,layout.addr);layout.u32(0x180,1);e.tick=50000
 e.fn('hook_row_refresh',row.addr);n=e.native_commits
 check(e.fn('hook_confirm',state.addr)==0 and e.native_commits==n+1,'commit then start nonblocking 1-second button reveal')
 check(e.close_calls==0 and e.dialogs==0 and e.visibility[layout.addr]==1,'native thank-you/fade/close suppressed only for new trait presentation')
 names=[]
 for step in [0,100,200,400,700,900,999]:
  e.tick=50000+step;e.fn('hook_state_tick',state.addr);assert layout.u32(0x180)==0
  names.append(e.texts[e.row_buttons[0].addr])
 check(len(set(names))>1 and e.close_calls==0,'eligible names visibly roll in original button while window stays open')
 e.tick=51000;e.fn('hook_state_tick',state.addr)
 check(e.texts[e.row_buttons[0].addr]==CAT[e.profile(10).awards[0]]['name'] and e.native_commits==n+1,'at 1000ms exact result in button, no repeat commit')
 e.tick=100000;e.fn('hook_state_tick',state.addr)
 check(e.pop_calls==0 and e.visibility[layout.addr]==1,'result stays readable until acknowledgement, no automatic disappearance')
 e.keys={1};e.fn('hook_state_tick',state.addr)
 check(e.close_calls==1 and e.pop_calls==1 and e.native_commits==n+1,'fresh click dismisses via native state cleanup without awarding again')
 check(e.mod.val('pending_ui',fmt='<Q')==0 and not any(C.string_at(e.mod.sym('roll_started'),6000*3*8)),'UI exit clears display state only')
 e.closeall()

def test_initializer():
 e=Env();r=BR.call(e.resolverdll.sym('SAN14_MeritResolveV1'),e.game.addr,0x29ff000);check(r!=0,'compiled resolver accepts exact CEKO header');e.game.u32(0x108,0)
 check(BR.call(e.resolverdll.sym('SAN14_MeritResolveV1'),e.game.addr,0x29ff000)==0,'compiled resolver rejects other game build');e.game.u32(0x108,0x6a6d9f72)
 check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1,'compiled mod initializer with synthetic Core/Win32 succeeds')
 check(e.mod.val('installed_count')==30 and e.patch_calls[-2:]==[('SAN14CE.MeritTraits',16),('SAN14CE.MeritTraits',14)],'30 guarded call/vtable sites, transactional bounded batches')
 patches=e.patch_calls[:];check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==1 and patches==e.patch_calls,'repeated initialization is idempotent');e.closeall()
 e=Env();h=e.res.hooks[0];e.game.put(h.rva,b'\x90');check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0 and not e.patch_calls,'unknown preexisting merit hook is not overwritten');e.closeall()
 e=Env();e.rng_fail=True;check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0 and not e.patch_calls,'unavailable BCrypt prevents installation of gameplay hooks');e.closeall()
 e=Env();e.patch_fail_at=3;old={h.rva:e.game.get(h.rva,h.length)for h in e.res.hooks if h.length};check(e.fn('SAN14_ModInitV1',C.addressof(e.core))==0,'injected second merit patch-batch failure')
 check(all(e.game.get(r,len(v))==v for r,v in old.items()) and e.mod.val('enabled')==0,'earlier merit patches rolled back, no active partial feature');e.closeall()

def test_actual_native_commit():
 path=ROOT/'analysis/analysis_image.bin'
 if not path.exists():
  print('SKIP full original commit fixture: private analysis image not present (not counted as pass)',flush=True);return
 e=Env();e.activate();p=PE(path)
 # Execute original commit machine code. Related services are controlled stubs.
 e.game.put(0x1ebca0,bytes(p.m[0x1ebca0:0x1ebf20]));o=e.officer();e.city=e.alloc(0x100);e.force=e.alloc(0x100);e.ruler=e.alloc(0x100);vt=e.alloc(0x100);vt.u64(0x80,BR.addr('GetForce'));e.city.u64(0,vt.addr);e.force.put(0x10,b'\1');e.force.put(0x14,b'\x64');e.db.u64(0xdf30+8,e.ruler.addr)
 BR.callbacks.update({'GetForce':lambda c:e.force.addr,'GetRuler':lambda c:e.ruler.addr,'Policy':lambda o:0,'Advance':lambda addr:put(addr+0x1bc,b(addr+0x1bc)+1,'<B'),'Pay':lambda c,amount:1,'Extra':lambda:0})
 for r,name in [(0x3ff60,'GetRuler'),(0x1ed4f0,'Policy'),(0x1ebc60,'Advance'),(0x27210,'Pay'),(0x1d2f10,'Extra')]:e.game.put(r,b'\xff\x25\0\0\0\0'+struct.pack('<Q',BR.addr(name)))
 e.game.u32(0x1a48848,300);e.game.u32(0x1a48854,1)
 batch=e.batch([(10,1,0)]);batch.u64(16,e.city.addr);check(e.fn('hook_commit_player',batch.addr)==1,'original CE 0x1ebca0 commit code executes through player wrapper (stub surroundings)')
 check(b(o.addr+0x1c2)==1 and b(o.addr+0x1bc)==1 and b(e.force.addr+0x14)==99 and e.owned(o)[5]==e.profile(10).awards[0],'actual native commit keeps native rank/command-cost plus registered trait')
 e.closeall()

if __name__=='__main__':
 try:
  for test in [test_core,test_runtime,test_rewind_and_savefail,test_initializer,test_actual_native_commit]:print('\nRUN',test.__name__,flush=True);test()
 finally:
  (ROOT/'audit/native_tests_rc3.json').write_text(json.dumps({'scope':'Production Win64 PE instructions executed on Linux using MS-ABI bridge; API/game services mocked. NOT real Windows or full SAN14 runtime.','tests':REPORT,'passed':len(REPORT)},ensure_ascii=False,indent=2))
 print('COMPLETE',len(REPORT),'checks')
