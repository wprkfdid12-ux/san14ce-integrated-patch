"""RC2 resolver/feature tests using decoded user-owned CEKO reference bytes.
No claim of live Windows/game rendering is made by this harness.
"""
import ctypes as C, mmap, struct, subprocess, json, sys, hashlib, traceback, os
from pathlib import Path
from pe_minimal import PE
HERE=Path(__file__).resolve().parent
Q=C.c_uint64;U=C.c_uint32;I=C.c_int32;W=C.c_uint16;B=C.c_ubyte
class Limits(C.Structure):_fields_=[(x,U) for x in 'officer_max formation_max tactic_max trait_max person_table formation_table tactic_table trait_table trait_slots tactic_slots'.split()]
class Resolved(C.Structure):_fields_=[(x,U) for x in 'size abi layout_abi symbol_count offset_count image_size'.split()]+[(x,Q) for x in 'image symbols offsets ReadLimits profile'.split()]
class Core(C.Structure):_fields_=[('size',U),('abi',U)]+[(x,Q) for x in 'resolved Readable InstallHooks MakeRelay FreeRelay Log'.split()]
class Hook(C.Structure):_fields_=[('target',Q),('length',U),('expected',B*16),('replacement',B*16)]
class Tab(C.Structure):_fields_=[('columns',Q),('label',Q),('resource',I),('pad',U)]
class Column(C.Structure):_fields_=[('id',U),('pad0',U),('header',Q),('width',U),('flags',U),('Text',Q),('align',U),('pad1',U)]+[(x,Q) for x in 'Key color aux extra'.split()]+[('native_tail',B*16),('zero',U),('visible',U)]
class Layout(C.Structure):_fields_=[(x,U) for x in 'cfg_size cfg_tabs cfg_columns cfg_bar cfg_mode dlg_list list_bodies list_tabs list_columns list_index list_count body_rows body_count body_tabs body_columns body_index body_checked event_index'.split()]
class Native(C.Structure):_fields_=[('Select',Q),('Destroy',Q)]
class Callbacks(C.Structure):_fields_=[(x,Q) for x in 'Attach Select Activate Destroy Tick Pin'.split()]
class API(C.Structure):_fields_=[('size',U),('abi',U),('l',Layout),('Native',Native)]+[(x,Q) for x in 'Valid Pair RelationText Name Resource Headers Rows empty_slot node_table_slot node_capacity_slot'.split()]+[(x,U) for x in 'base_column_count compare_count tab_resource'.split()]+[('native_column',Column),('Install',Q),('ReadOnly',Q),('HoverPerson',Q)]
assert C.sizeof(Column)==96 and C.sizeof(Tab)==24

def build_bridge():
 p=HERE/'relation_msabi_bridge.c'
 s='#include <stdint.h>\ntypedef uint64_t U;\ntypedef U (*CB)(U,U,U,U,U,U,U,U,U,U);\nstatic CB callback;\nvoid set_callback(CB f){callback=f;}\n'
 for i in range(128):s+=f'__attribute__((ms_abi,noinline)) static U stub_{i}(U a,U b,U c,U d,U e,U f,U g,U h,U j){{return callback({i},a,b,c,d,e,f,g,h,j);}}\n'
 s+='void *get_stub(unsigned n){static void *v[]={'+','.join(f'(void*)&stub_{i}' for i in range(128))+'};return n<128?v[n]:0;}\n'
 s+='U call_ms(void *p,U *a){typedef U (__attribute__((ms_abi))*F)(U,U,U,U,U,U,U,U,U);return ((F)p)(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]);}\n'
 p.write_text(s);subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(p),'-o',str(HERE/'relation_msabi_bridge.so')],check=True)
 b=C.CDLL(str(HERE/'relation_msabi_bridge.so'));b.get_stub.argtypes=[U];b.get_stub.restype=Q;b.call_ms.argtypes=[C.c_void_p,C.POINTER(Q)];b.call_ms.restype=Q;return b
BR=build_bridge();CB=C.CFUNCTYPE(Q,*([Q]*10));BR.set_callback.argtypes=[CB]
class Machine:
 def __init__(self):
  self.maps=[];self.buffers=[];self.handlers=[];self.errors=[];self.logs=[];self.ranges=[];self.heap={};self.freed=[];self.tests=[];self.native_calls=[];self.dialogs={};self.fail_hooks=False;self.hook_transactions=[];self.relays=[];self.freed_relays=[]
  self.callback=CB(self.dispatch);BR.set_callback(self.callback)
 def dispatch(self,n,*a):
  try:return int(self.handlers[n](a) or 0)&((1<<64)-1)
  except BaseException:self.errors.append(traceback.format_exc());return 0
 def stub(self,fn):i=len(self.handlers);self.handlers.append(fn);assert i<128;return BR.get_stub(i)
 def call(self,fn,*a):
  args=(Q*9)(*[int(x)&((1<<64)-1) for x in a],*([0]*(9-len(a))));ret=BR.call_ms(fn,args);assert not self.errors,'\n'.join(self.errors);return ret
 def new(self,n):b=C.create_string_buffer(n);self.buffers.append(b);a=C.addressof(b);self.ranges.append((a,a+n));return a
 def map(self,n):b=mmap.mmap(-1,n,prot=7);self.maps.append(b);a=C.addressof(C.c_char.from_buffer(b));self.ranges.append((a,a+n));return a
 def read(self,p,n):return C.string_at(p,n)
 def q(self,p,v=None):
  if v is None:return Q.from_address(p).value
  Q.from_address(p).value=v
 def u(self,p,v=None):
  if v is None:return U.from_address(p).value
  U.from_address(p).value=v
 def text(self,s,p=None):
  raw=s.encode('utf-16le')+b'\0\0';p=p or self.new(max(len(raw),512));C.memmove(p,raw,len(raw));return p
 def wide(self,p):
  out=[]
  while p and W.from_address(p).value:out.append(W.from_address(p).value);p+=2
  return bytes(bytearray(sum(([x&255,x>>8] for x in out),[]))).decode('utf-16le')
 def readable(self,a):
  p,n=a[:2]
  for lo,hi in self.ranges:
   if lo<=p and p+n<=hi:return 1
  for ln in Path('/proc/self/maps').read_text().splitlines():
   span,perm,*_=ln.split();lo,hi=[int(x,16) for x in span.split('-')]
   if perm[0]=='r' and lo<=p and p+n<=hi:return 1
  return 0
 def pe(self,filename,meta):
  pe=PE(str(HERE/filename));p=self.map(pe.OPTIONAL_HEADER.SizeOfImage);C.memmove(p,pe.__data__[:pe.OPTIONAL_HEADER.SizeOfHeaders],pe.OPTIONAL_HEADER.SizeOfHeaders)
  for s in pe.sections:
   if s.SizeOfRawData:C.memmove(p+s.VirtualAddress,s.get_data(),s.SizeOfRawData)
  return p,json.loads((HERE/meta).read_text()),pe
 def limits(self,a):l=Limits.from_address(a[0]);l.officer_max=6000;l.formation_max=12;l.tactic_max=300;l.trait_max=300;l.trait_slots=9;l.tactic_slots=3;return 1
 def setup_core(self):
  self.resolved=Resolved();self.resolved.size=C.sizeof(Resolved);self.resolved.abi=0x10000;self.resolved.layout_abi=1;self.resolved.symbol_count=131;self.resolved.offset_count=476;self.resolved.image_size=0x29ff000;self.resolved.image=self.game;self.resolved.ReadLimits=self.stub(self.limits)
  self.core=Core();self.core.size=C.sizeof(Core);self.core.abi=0x10000;self.core.resolved=C.addressof(self.resolved);self.core.Readable=self.stub(self.readable);self.core.Log=self.stub(lambda a:self.logs.append(C.string_at(a[0]).decode()));self.core.MakeRelay=self.stub(self.make_relay);self.core.FreeRelay=self.stub(lambda a:self.freed_relays.append(a[0]));self.core.InstallHooks=self.stub(self.install_hooks)
 def make_relay(self,a):v=self.game+0x2980000+len(self.relays)*0x100;self.relays.append((v,*a[:2]));return v
 def install_hooks(self,a):
  owner=C.string_at(a[0]).decode();hooks=(Hook*a[2]).from_address(a[1]);items=[(h.target,h.length,bytes(h.expected)[:h.length],bytes(h.replacement)[:h.length]) for h in hooks];self.hook_transactions.append((owner,items))
  if self.fail_hooks:return 0
  if any(self.read(p,n)!=old for p,n,old,new in items):return 0
  for p,n,old,new in items:C.memmove(p,new,n)
  return 1
 def check(self,name,condition=True):assert condition,name;self.tests.append(name);print('PASS',name,flush=True)
 def resolver_tests(self):
  image=Path(os.environ['SAN14CE_REFERENCE_IMAGE']).read_bytes()
  self.game=self.map(len(image));C.memmove(self.game,bytes(image),len(image));self.setup_core();self.q(self.game+0x13adf60,self.game+0x7fe1f0)
  self.rb,self.rm,self.rpe=self.pe('SAN14_RelatedOfficersResolver.dll','relation_resolver_build.json');resolve=self.rb+self.rm['functions']['SAN14_RelatedResolverV1'][0]
  p=self.call(resolve,C.addressof(self.core));self.check('resolver runs against supplied CEKO build',p!=0);self.rapi=API.from_buffer_copy(self.read(p,C.sizeof(API)));r=self.rapi
  self.check('resolver uses RC2.4 private ABI',r.abi==0x20001)
  self.check('native relation functions resolved',r.Pair==self.game+0x7d6450 and r.Name==self.game+0x22e3f0 and r.Headers==self.game+0x2d5c50)
  self.check('native relation metadata retained',r.compare_count==9 and r.base_column_count==212 and r.tab_resource==0x235 and r.native_column.width==132 and r.native_column.flags==0x20000008 and r.native_column.visible==1)
  protected=self.read(self.game+0x50e1a0,16)
  cbs=Callbacks(*[self.stub(lambda a:0) for _ in range(6)])
  self.fail_hooks=True;self.check('six RC2.4 hooks submitted atomically and rejected cleanly',self.call(r.Install,C.addressof(cbs))==0 and len(self.hook_transactions[-1][1])==6);self.check('all RC2 relays freed on rejected transaction',len(self.freed_relays)==6)
  self.fail_hooks=False;self.check('valid atomic RC2 install succeeds',self.call(r.Install,C.addressof(cbs))==1)
  sites=[(p-self.game,n) for p,n,old,new in self.hook_transactions[-1][1]]
  self.check('RC2.4 uses baseline sites plus guarded-prologue-external post-input call',sites==[(0x2c00cd,7),(0x2efd90,5),(0x2db6b2,6),(0x6b032f,5),(0x50e1b2,5),(0x7fe1f0,7)])
  self.check('TraitHover guarded 0x50e1a0 prologue remains byte-identical',self.read(self.game+0x50e1a0,16)==protected)
  for p,n,old,new in self.hook_transactions[-1][1]:C.memmove(p,old,n)
  self.check('duplicate RC2 hook install refused',self.call(r.Install,C.addressof(cbs))==0)
 def setup_feature(self):
  self.fb,self.fm,self.fpe=self.pe('SAN14_RelatedOfficersTab.dll','related_officers_build.json');self.r=API.from_buffer_copy(bytes(self.rapi));r=self.r
  r.Native=Native(self.stub(self.native_select),self.stub(self.native_destroy));self.names={};self.temp_name=self.new(512);self.empty=self.text('');self.relation=self.text('연계');self.relation_value=self.text('원본관계결과')
  r.Valid=self.stub(lambda a:int(a[0] in self.names));r.Pair=self.stub(self.pair);r.Name=self.stub(lambda a:self.text(self.names[a[0]],self.temp_name));r.RelationText=self.stub(lambda a:self.native_calls.append(('RelationText',a[0])) or self.relation_value);r.Resource=self.stub(lambda a:self.native_calls.append(('Resource',a[0])) or self.relation);r.Headers=self.stub(lambda a:self.native_calls.append(('Headers',*a[:2])));r.Rows=self.stub(lambda a:self.native_calls.append(('Rows',a[0])))
  self.q(r.empty_slot,self.empty);self.heads=self.new(8*100);self.q(r.node_table_slot,self.heads);self.u(r.node_capacity_slot,100);r.Install=self.stub(self.capture_callbacks);self.hovered=0;r.HoverPerson=self.stub(lambda a:self.hovered)
  self.proc={'GetProcessHeap':self.stub(lambda a:123),'HeapAlloc':self.stub(self.heap_alloc),'HeapFree':self.stub(self.heap_free),'SAN14_RelatedResolverV1':self.stub(lambda a:C.addressof(self.r))};imports={'GetModuleHandleW':lambda a:0x1234,'GetModuleFileNameW':self.module_file,'LoadLibraryExW':lambda a:0x5678,'GetProcAddress':lambda a:self.proc.get(C.string_at(a[1]).decode(),0)}
  wi=self.fb+self.fm['globals']['win'][0]
  for i,name in enumerate(['GetSystemDirectoryW','LoadLibraryExW','GetProcAddress','InitOnceExecuteOnce','GetModuleHandleW','GetModuleFileNameW','VirtualProtect','FlushInstructionCache','GetCurrentProcess','CreateFileW','WriteFile','CloseHandle','GetLastError','SetLastError']):self.q(wi+i*8,self.stub(imports.get(name,lambda a:0)))
  self.check('compiled RC2.4 feature initializes with mocked shared Core API',self.call(self.fb+self.fm['functions']['SAN14_ModInitV1'][0],C.addressof(self.core))==1)
 def capture_callbacks(self,a):self.cb=Callbacks.from_buffer_copy(self.read(a[0],C.sizeof(Callbacks)));return 1
 def module_file(self,a):s='C:\\Games\\SAN14CE_KO.exe';self.text(s,a[1]);return len(s)
 def heap_alloc(self,a):p=self.new(a[2]);self.heap[p]=a[2];return p
 def heap_free(self,a):p=a[2];assert p in self.heap;self.freed.append(p);del self.heap[p];return 1
 def pair(self,a):out,person,other=a[:3];self.native_calls.append(('Pair',person,other));C.memset(out,0,16);self.u(out,0 if person in self.names and other in self.names else 2);self.u(out+8,3 if person in self.names and other in self.names else 0x7fffffff);return out
 def native_select(self,a):
  self.native_calls.append(('NativeSelect',a[0]));d=self.dialogs[a[0]];idx=self.u(a[1]+0x80);sel=d.get('selected',[]).copy();
  if d['mode']==2:
   sel.remove(idx) if idx in sel else sel.append(idx);self.select_set(d,sel)
  return 0x789
 def native_destroy(self,a):self.native_calls.append(('Destroy',a[0]));return 0x987
 def construct(self,d):
  dlg,cfg=d['parent'],d['cfg'];tabs=self.q(cfg+8);cols=self.q(cfg+16);n=0
  while self.q(tabs+n*24):n+=1;assert n<65
  li=self.new(0x240);bodies=self.new(8*n);rows=self.new(8*len(d['people']));d.update(list=li,tabs=tabs,cols=cols,n=n,bodies=[],rows=rows)
  for i,p in enumerate(d['people']):self.q(rows+8*i,p)
  self.q(dlg+0x180,li);self.q(dlg+0x1a8,cfg);self.u(li+0x228,d['mode']);self.q(li+0x130,bodies);self.q(li+0x150,tabs);self.q(li+0x158,cols);self.u(li+0x208,0);self.u(li+0x20c,n);d['handle']=len(self.dialogs);h=self.new(4);self.u(h,d['handle']);d['handleptr']=h
  for i in range(n):
   b=self.new(0x300);d['bodies'].append(b);self.q(b+8,li);self.u(b+0x170,d['mode']);self.q(bodies+i*8,b);self.q(b+0x198,rows);self.u(b+0x1a0,len(d['people']));self.q(b+0x1a8,tabs);self.q(b+0x1b0,cols);self.u(b+0x1b8,i);self.q(b+0x158,h)
 def new_dialog(self,ntabs=11,npeople=12,native=False,typename='.?AVPersonDialog@@',bar=1,traits9=False,mode=2):
  dlg=self.new(0x600);vt=self.new(128);col=self.new(24);self.q(dlg,vt+8);self.q(vt,col);self.u(col,1);td=0x2800000+len(self.dialogs)*0x200;self.u(col+12,td);C.memmove(self.game+td+16,typename.encode()+b'\0',len(typename)+1)
  cfg=self.new(0x400);basecols=264 if traits9 else 212;tabs=self.new((ntabs+1)*24);cols=self.new(basecols*96)
  # Actual native 212 descriptors, including repeated semantic identifiers.
  source_pe=PE(os.environ['SAN14CE_REFERENCE_EXE']);raw=source_pe.get_data(0x1a2b850,212*96)
  C.memmove(cols,raw,len(raw))
  for i in range(212,basecols):
   v=Column.from_address(cols+96*i);v.id=0 if i<256 else i;v.width=132;v.flags=0x1234;v.visible=1
  trait_ids=[256,257,258,259,260,261,262,263] if traits9 else []
  for i in range(ntabs):
   ids=[0,1]+(trait_ids if traits9 and i==min(6,ntabs-1) else [])+[-1];indices=self.new(4*len(ids));C.memmove(indices,struct.pack('<'+'i'*len(ids),*ids),4*len(ids));t=Tab.from_address(tabs+24*i);t.columns=indices;t.resource=0x235 if native and i==2 else 100+i;t.pad=0x789
  self.q(cfg+8,tabs);self.q(cfg+16,cols);self.u(cfg+0x9c,bar);self.u(cfg+0x18,mode);people=[];person_vt=self.new(64)
  for i in range(npeople):p=self.new(64);people.append(p);self.q(p,person_vt);self.names[p]='무장'+str(i)
  d={'parent':dlg,'cfg':cfg,'oldtabs':tabs,'oldcols':cols,'basecols':basecols,'people':people,'selected':[],'mode':mode};self.dialogs[dlg]=d
  before_tabs=self.read(tabs,ntabs*24);before_cols=self.read(cols,basecols*96);self.call(self.cb.Attach,dlg,cfg);d['attached']=self.q(cfg+8)!=tabs
  if d['attached']:
   self.check('attach preserves every pre-existing tab byte',self.read(self.q(cfg+8),ntabs*24)==before_tabs)
   self.check('attach preserves every pre-existing descriptor byte',self.read(self.q(cfg+16),basecols*96)==before_cols)
  self.construct(d);return d
 def select_set(self,d,selected):
  d['selected']=list(selected);prev=0
  for i in reversed(selected):node=self.new(16);self.u(node,i);self.q(node+8,prev);prev=node
  self.q(self.heads+d['handle']*8,prev)
 def active(self,d,tab=None):
  if tab is not None:self.u(d['list']+0x208,tab)
  return d['bodies'][self.u(d['list']+0x208)]
 def activate(self,d,tab=None):body=self.active(d,tab);self.call(self.cb.Activate,body);return body
 def headers(self,d):base=d['basecols'];return [self.wide(Column.from_address(d['cols']+(base+i)*96).header) for i in range(9)]
 def choose(self,d,i):e=self.new(0x100);self.u(e+0x80,i);return self.call(self.cb.Select,d['parent'],e)
 def feature_tests(self):
  d=self.new_dialog();self.check('relation tab appended at the right',d['attached'] and d['n']==12);self.check('relation tab uses native resource id',Tab.from_address(d['tabs']+11*24).resource==0x235 and Tab.from_address(d['tabs']+11*24).label==0)
  self.activate(d,d['n']-1);self.choose(d,2);self.choose(d,5);self.check('native checkbox comparison order retained',self.headers(d)[:3]==['무장2','무장5',''])
  self.choose(d,2);self.check('uncheck compacts native comparison columns',self.headers(d)[:2]==['무장5',''])
  col=Column.from_address(d['cols']+d['basecols']*96);out=self.new(16);self.native_calls.clear();value=self.call(col.Text,d['people'][0]);self.call(col.Key,out,d['people'][0]);self.check('relation cells delegate native Pair/RelationText',self.wide(value)=='원본관계결과' and ('Pair',d['people'][0],d['people'][5]) in self.native_calls and ('RelationText',3) in self.native_calls)
  self.select_set(d,[5,1,2,3,4,6,7,8,9,10,11]);self.activate(d,d['n']-1);self.check('over-nine command selections untouched while native comparison stays nine',len(d['selected'])==11 and len([x for x in self.headers(d) if x])==9)
  saved=self.headers(d);people=d['people'];reordered=list(reversed(people));chosen=[people[i] for i in d['selected']]
  for i,p in enumerate(reordered):self.q(d['rows']+8*i,p)
  self.select_set(d,[reordered.index(p) for p in chosen]);self.activate(d,d['n']-1);self.check('sort/reopen preserves selected identities',self.headers(d)==saved)
  self.select_set(d,[]);self.activate(d,d['n']-1);self.check('tab activation clears stale native headers',self.headers(d)==['']*9)
  d2=self.new_dialog(ntabs=3,typename='.?AVCollectionPerson@@');self.check('CollectionPerson/list window is no longer wrongly excluded',d2['attached'])
  x=self.new_dialog(typename='.?AVInfoDetailPerson@@');self.check('InfoDetail person-family list is included',x['attached'])
  x2=self.new_dialog(bar=0);self.check('no-tabbar screen remains unchanged',not x2['attached'])
  x3=self.new_dialog(native=True);self.check('existing native relation tab is never duplicated',not x3['attached'])
  t=self.new_dialog(ntabs=11,traits9=True);self.check('Traits9-expanded 264 descriptors are composed before relation descriptors',t['attached'] and t['basecols']==264 and Column.from_address(t['cols']+263*96).id==263 and Column.from_address(t['cols']+264*96).id==264)
  # Destroy live attached contexts and prove feature allocations are released.
  for z in [d,d2,x,t]:self.call(self.cb.Destroy,z['list'])
  self.check('feature-owned relation metadata released on native destructor',not self.heap)
 def run(self):
  self.resolver_tests();self.setup_feature();self.feature_tests();return self


def virtual_tests(m):
 d=m.new_dialog(mode=1,npeople=30);b=m.activate(d,d['n']-1)
 m.check('read-only classification uses matching native mode at config/list/body',m.call(m.r.ReadOnly,d['parent'],d['list'],b)==1)
 m.u(b+0x170,2);m.check('checkbox mode or inconsistent propagation never qualifies',m.call(m.r.ReadOnly,d['parent'],d['list'],b)==0);m.u(b+0x170,1)
 before=m.read(d['handleptr'],4)+m.read(m.heads+d['handle']*8,8)
 m.hovered=d['people'][2];m.call(m.cb.Tick,b)
 m.check('no-pin hover previews exactly one reference column',m.headers(d)==['무장2']+['']*8)
 m.check('hover only uses display state, not native checkbox registry',before==m.read(d['handleptr'],4)+m.read(m.heads+d['handle']*8,8))
 m.hovered=0;m.call(m.cb.Tick,b);m.check('unfixed hover clears after pointer leaves name',m.headers(d)==['']*9)
 m.hovered=d['people'][3];m.call(m.cb.Tick,b);m.native_calls.clear();result=m.choose(d,3)
 m.check('click pins without native detail dispatch',result==0 and not any(x[0]=='NativeSelect' for x in m.native_calls) and m.headers(d)[0]=='무장3')
 m.hovered=d['people'][29];m.call(m.cb.Tick,b);m.check('other hover cannot replace fixed reference',m.headers(d)[0]=='무장3')
 # Actual visible/filtered row arrays change but the pin is not a row index.
 original=d['people'];shown=list(reversed(original))
 for i,p in enumerate(shown):m.q(d['rows']+i*8,p)
 m.call(m.cb.Tick,b);m.check('pin survives native array reordering',m.headers(d)[0]=='무장3')
 m.u(b+0x1a0,2);m.call(m.cb.Tick,b);m.check('pin survives scrolling/filtering reference out of visible data',m.headers(d)[0]=='무장3')
 col=Column.from_address(d['cols']+d['basecols']*96);m.native_calls.clear();m.call(col.Text,original[29]);m.check('off-screen fixed reference is supplied to original Pair',('Pair',original[29],original[3]) in m.native_calls)
 m.u(b+0x1a0,30)
 for i,p in enumerate(original):m.q(d['rows']+i*8,p)
 m.activate(d,0);m.native_calls.clear();m.choose(d,5);m.check('other tabs still perform native detail action',any(x[0]=='NativeSelect' for x in m.native_calls))
 m.activate(d,d['n']-1);m.call(m.cb.Tick,b);m.check('pin survives tab switch and return',m.headers(d)[0]=='무장3')
 m.choose(d,7);m.check('different click replaces single pin instead of silently making multiselect',m.headers(d)==['무장7']+['']*8)
 m.hovered=original[7];m.choose(d,7);m.call(m.cb.Tick,b);m.check('same reference click unpins and suppresses immediate same-row preview',m.headers(d)==['']*9)
 m.hovered=0;m.call(m.cb.Tick,b);m.hovered=original[7];m.call(m.cb.Tick,b);m.check('returning cursor after leaving permits preview again',m.headers(d)[0]=='무장7')
 # Native checkbox dialog still bypasses virtual policy even with nothing checked.
 native=m.new_dialog(mode=2,npeople=30);nb=m.activate(native,native['n']-1);m.hovered=native['people'][8];m.call(m.cb.Tick,nb);m.check('hover never previews in checkbox dialog, even with empty selection',m.headers(native)==['']*9)
 m.native_calls.clear();m.choose(native,8);m.check('checkbox Relation click retains original dispatch and checkbox state',native['selected']==[8] and any(x[0]=='NativeSelect' for x in m.native_calls) and m.headers(native)[0]=='무장8')
 m.select_set(native,list(range(20)));m.activate(native,native['n']-1);m.check('20 command selections remain selected with only native nine comparison columns',len(native['selected'])==20 and len([x for x in m.headers(native) if x])==9)
 d2=m.new_dialog(mode=1,npeople=4);b2=m.activate(d2,d2['n']-1);m.choose(d2,1);m.check('reference states isolated between open information windows',m.headers(d2)[0]=='무장1' and m.headers(d)[0]=='무장7')
 m.choose(d,7);del m.names[d['people'][7]];m.hovered=0;m.call(m.cb.Tick,b);m.check('invalidated native reference is cleared without use',m.headers(d)==['']*9)
 m.call(m.cb.Destroy,d['list']);m.call(m.cb.Destroy,d2['list']);m.call(m.cb.Destroy,native['list']);m.check('hover/pin contexts release all owned allocations',not m.heap)
 fresh=m.new_dialog(mode=1);fb=m.activate(fresh,fresh['n']-1);m.hovered=0;m.call(m.cb.Tick,fb);m.check('new window never inherits old pinned reference',m.headers(fresh)==['']*9);m.call(m.cb.Destroy,fresh['list'])

if __name__=='__main__':
 m=Machine();m.resolver_tests();m.setup_feature();m.feature_tests();virtual_tests(m)
 one=m.new_dialog(ntabs=1,typename='.?AVCDataSelectDlgBase@VCPersonData@@@@');m.check('single-tab officer list receives relation as second tab',one['attached'] and one['n']==2);m.call(m.cb.Destroy,one['list']);m.check('single-tab allocation released',not m.heap)
 Path('test_rc24_feature_results.json').write_text(json.dumps({'passed':len(m.tests),'tests':m.tests,'logs':m.logs},ensure_ascii=False,indent=2));print('TOTAL',len(m.tests))
