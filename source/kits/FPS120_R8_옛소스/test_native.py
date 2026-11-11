"""Execute the shipped x64 module and patched CALLs under Unicorn.

Arguments: verified reference directory (original EXE + runtime capture).
Win32 clocks/timers and the original duel update's body are modeled. The
original game clock routine executes its captured instructions. No live game.
"""
import hashlib,json,struct,sys,math
from pathlib import Path
import pefile
from unicorn import Uc,UC_ARCH_X86,UC_MODE_64,UC_HOOK_CODE
from unicorn.x86_const import *

HERE=Path(__file__).resolve().parent
REF=Path(sys.argv[1] if len(sys.argv)>1 else 'analysis/game_reference')
EXE=REF/'SAN14CE_KO.exe';TEXT=REF/'SAN14CE_KO.runtime_text.bin'
assert hashlib.sha256(EXE.read_bytes()).hexdigest()=='07a047826cfc316dc9e3db6bfddc72b85e8d7de99743cc73a2d7199a1ad3aa40'
assert hashlib.sha256(TEXT.read_bytes()).hexdigest()=='e22f87700cf7d711fa4f10b3c7ba3ccfc0b62c03ae1b90a411acf082f1119e72'
gamepe=pefile.PE(str(EXE),fast_load=True)
dll=pefile.PE(str(HERE/'SAN14_FPS120.dll'))
exports={s.name.decode():s.address for s in dll.DIRECTORY_ENTRY_EXPORT.symbols}
symbols=json.loads((HERE/'build.json').read_text())['functions']
checks=[]
def check(ok,name):
 assert ok,name
 checks.append(name)
def put(u,a,fmt,*v):u.mem_write(a,struct.pack('<'+fmt,*v))
def get(u,a,fmt):return struct.unpack('<'+fmt,u.mem_read(a,struct.calcsize('<'+fmt)))[0]
def cstr(u,a):
 out=b''
 while u.mem_read(a,1)!=b'\0':out+=u.mem_read(a,1);a+=1
 return out.decode()
NONVOL=[UC_X86_REG_RBX,UC_X86_REG_RBP,UC_X86_REG_RSI,UC_X86_REG_RDI,UC_X86_REG_R12,UC_X86_REG_R13,UC_X86_REG_R14,UC_X86_REG_R15]

class Machine:
 def __init__(self,mode='',game=0x140000000,module=0x180000000,frequency=10000000):
  self.g=game;self.d=module;self.a=0x500000000;self.h=self.a+0x40000;self.s=0x400000000
  self.mode=mode;self.time=1000000;self.freq=frequency;self.waits=[];self.duels=[];self.logs=[];self.freed=[];self.closed=[];self.records=[];self.relays=[];self.queries=[];self.directions=[];self.config_null=False;self.fps_null=False
  self.u=u=Uc(UC_ARCH_X86,UC_MODE_64)
  u.mem_map(game,0x29ff000);u.mem_write(game,gamepe.get_data(0,gamepe.OPTIONAL_HEADER.SizeOfHeaders))
  for sec in gamepe.sections:
   if sec.Name.rstrip(b'\0') in (b'.rdata',b'.data'):u.mem_write(game+sec.VirtualAddress,sec.get_data())
  u.mem_write(game+0x1000,TEXT.read_bytes())
  u.mem_map(module,dll.OPTIONAL_HEADER.SizeOfImage);u.mem_write(module,dll.get_memory_mapped_image())
  u.mem_map(self.a,0x100000);u.mem_map(self.s,0x100000);u.mem_map(game+0x4000000,0x10000)
  self.names=['readable','patch','relay','free','log','qpc','qpf','sleep','wait','module','proc','create','settimer','close','fpsget','cfgget','direction']
  self.api={n:self.a+0x1000+0x100*i for i,n in enumerate(self.names)}
  self.lookup={v:k for k,v in self.api.items()}
  core=self.a+0x100;resolver=self.a+0x200
  put(u,core,'II7Q',56,0x10000,resolver,*[self.api[n] for n in self.names[:5]],0)
  put(u,resolver,'6IQ',64,0x10000,0x10000,0,0,0x29ff000,game)
  for rva,name in [(0x13400e0,'sleep'),(0x13400e8,'qpc'),(0x13400f0,'qpf'),(0x13401e0,'wait'),(0x1340228,'module'),(0x1340318,'proc')]:put(u,game+rva,'Q',self.api[name])
  put(u,game+0x27dc6a0,'Q',self.h+0x1000);put(u,self.h+0x1012,'B',1)
  self.manager=self.h+0x2000;put(u,game+0x2783730,'Q',self.manager);put(u,self.manager,'Q',game+0x13b5f98)
  self.engine=self.h+0x10000;self.clock=self.h+0x15000;self.frame=self.h+0x16000;self.ctx=self.h+0x17000;self.app=self.h+0x18000;self.cfg=self.h+0x19000;self.fpsvalue=self.h+0x1a000;self.vt=self.h+0x1b000;self.direction_object=self.h+0x1c000
  put(u,self.engine+8,'Q',self.vt);put(u,self.vt+0x10,'Q',self.api['cfgget']);put(u,self.vt+0x28,'Q',self.api['fpsget'])
  put(u,self.fpsvalue,'I',120);put(u,self.cfg+0x34,'f',1/120)
  put(u,self.h+0x1018,'Q',self.freq);put(u,self.clock+0x30,'I',120)
  put(u,self.direction_object,'Q',self.vt+0x100);put(u,self.vt+0x118,'Q',self.api['direction'])
  u.hook_add(UC_HOOK_CODE,self.hook)
 def ret(self,value):
  u=self.u;sp=u.reg_read(UC_X86_REG_RSP);u.reg_write(UC_X86_REG_RAX,value&0xffffffffffffffff);u.reg_write(UC_X86_REG_RIP,get(u,sp,'Q'));u.reg_write(UC_X86_REG_RSP,sp+8)
 def hook(self,u,a,n,data):
  if a==self.g+0x37a1b0:
   self.duels.append((self.time,u.reg_read(UC_X86_REG_RCX),u.reg_read(UC_X86_REG_RDX)));self.ret(0);return
  if a==self.g+0xb36310:
   self.time+=max(1,self.freq//1000);self.ret(1);return
  name=self.lookup.get(a)
  if not name:return
  assert u.reg_read(UC_X86_REG_RSP)%16==8,(name,'Win64 stack alignment')
  c,d,r8,r9=[u.reg_read(r) for r in [UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9]]
  value=0
  if name=='fpsget':
   assert c==self.engine+8;self.queries.append('fps');value=0 if self.fps_null else self.fpsvalue
  elif name=='cfgget':
   assert c==self.engine+8;self.queries.append('cfg');value=0 if self.config_null else self.cfg
  elif name=='direction':
   assert c==self.direction_object and r8==0;self.directions.append(d);value=0x7a9
  elif name=='readable':value=not(self.mode=='unreadable_manager' and c==self.manager)
  elif name=='log':self.logs.append(cstr(u,c))
  elif name=='qpc':put(u,c,'q',self.time);self.time+=max(1,self.freq//100000);value=self.mode!='qpc_fail'
  elif name=='qpf':put(u,c,'q',self.freq);value=self.mode!='qpf_fail'
  elif name=='sleep':assert c==0;self.time+=max(1,self.freq//10000);self.waits.append('yield')
  elif name=='module':assert cstr(u,c)=='kernel32.dll';value=self.a+0x8000
  elif name=='proc':
   func=cstr(u,d);value=0 if self.mode=='no_timer' else self.api[{'CreateWaitableTimerExW':'create','SetWaitableTimer':'settimer','CloseHandle':'close'}[func]]
  elif name=='create':
   assert c==d==0 and r8==2 and r9==0x1f0003
   value=0 if self.mode=='timer_create_fail' else self.a+0x9000+(self.api['create']&255)
   self.api['create']+=1
  elif name=='settimer':
   sp=u.reg_read(UC_X86_REG_RSP)
   assert (r8&0xffffffff)==r9==get(u,sp+40,'Q')==get(u,sp+48,'I')==0
   due=get(u,d,'q');assert due<0
   self.remaining=(-due)*self.freq//10000000;value=self.mode!='timer_set_fail'
  elif name=='wait':
   assert d==30;self.waits.append('timer');self.time+=self.remaining+(self.freq//40 if self.mode=='oversleep' else 0)
  elif name=='close':self.closed.append(c);value=1
  elif name=='relay':
   if self.mode=='relay_fail' and self.relays:value=0
   else:
    value=self.g+0x4000000+len(self.relays)*0x100
    u.mem_write(value,b'\xff\x25\0\0\0\0'+struct.pack('<Q',d));self.relays.append((value,d))
  elif name=='free':self.freed.append(c)
  elif name=='patch':
   assert cstr(u,c)=='SAN14CE.FPS120.R8.Duel60_StockTiming_Other120' and r8==7
   self.records=[]
   for i in range(r8):
    rec=d+i*48;target=get(u,rec,'Q');length=get(u,rec+8,'I');before=bytes(u.mem_read(rec+12,length));after=bytes(u.mem_read(rec+28,length))
    assert bytes(u.mem_read(target,length))==before
    self.records.append((target,length,before,after))
   value=self.mode!='transaction_reject'
   if value:
    for target,length,before,after in self.records:u.mem_write(target,after)
  self.ret(int(value))
 def call(self,where,rcx=0,rdx=0,end=None,regs=None):
  u=self.u;sp=self.s+0x80000;stop=end or self.a
  if end is None:sp-=8;put(u,sp,'Q',stop)
  u.reg_write(UC_X86_REG_RSP,sp);u.reg_write(UC_X86_REG_RCX,rcx);u.reg_write(UC_X86_REG_RDX,rdx)
  for i,reg in enumerate(NONVOL):u.reg_write(reg,0x12300000+i)
  for i in range(6,16):u.reg_write(UC_X86_REG_XMM0+i,0x123456789abcdef+i)
  for r,v in (regs or {}).items():u.reg_write(r,v)
  expected={r:u.reg_read(r) for r in NONVOL}
  u.emu_start(where,stop,count=3000000)
  assert u.reg_read(UC_X86_REG_RIP)==stop,'execution did not finish'
  assert u.reg_read(UC_X86_REG_RSP)==self.s+0x80000
  assert all(u.reg_read(reg)==v for reg,v in expected.items())
  assert all(u.reg_read(UC_X86_REG_XMM0+i)==0x123456789abcdef+i for i in range(6,16))
  return u.reg_read(UC_X86_REG_RAX)
 def init(self):return self.call(self.d+exports['SAN14_ModInitV1'],self.a+0x100)
 def active(self,yes=True):put(self.u,self.manager+0x588,'I',int(yes))
 def render(self):return self.call(self.g+0x953fda,end=self.g+0x953fdf)
 def update(self,p=None):return self.call(self.g+0x44ad50,self.manager if p is None else p,1,end=self.g+0x44ad55)

 def engine_tick(self):
  u=self.u;sp=self.s+0x80000
  put(u,sp+0x30,'Q',self.engine);put(u,sp+0x48,'Q',0)
  for reg,value in [(UC_X86_REG_RSP,sp),(UC_X86_REG_RBX,self.engine),(UC_X86_REG_RBP,self.frame),(UC_X86_REG_R15,self.clock),(UC_X86_REG_RDI,self.app),(UC_X86_REG_RSI,self.ctx),(UC_X86_REG_R12,0),(UC_X86_REG_R13,1)]:u.reg_write(reg,value)
  u.emu_start(self.g+0xf3e214,self.g+0xf3e554,count=3000000)
  assert u.reg_read(UC_X86_REG_RIP)==self.g+0xf3e554
  return get(u,self.clock+0x30,'I'),get(u,self.clock+0x18,'f'),get(u,self.engine+0x3350,'f')
 def direction(self):
  return self.call(self.g+0x36a7b5,self.direction_object,end=self.g+0x36a7c0)


class OriginalDuelMachine(Machine):
 """Run the captured manager, both actor timelines and actor movement.

 A minimal valid manager fixture avoids resource-dependent state transitions.
 Only the context provider, sqrt, TLS and OS clocks are supplied by the host.
 This checks real time advancement, rather than merely counting mock updates.
 """
 def __init__(self,fps_mode):
  super().__init__()
  self.context=self.h+0x5000
  put(self.u,self.context+8,'I',fps_mode)
  # Valid initialized native TLS for the camera singleton guard.
  self.u.reg_write(UC_X86_REG_GS_BASE,self.h+0x7000)
  put(self.u,self.h+0x7058,'Q',self.h+0x8000)
  put(self.u,self.g+0x27f1320,'I',0)
  put(self.u,self.h+0x8000,'Q',self.h+0x6000)
  put(self.u,self.h+0x6010,'I',1)
  put(self.u,self.g+0x2783d60,'i',0)
  put(self.u,self.g+0x2783740+0x5c8,'Q',0)
  put(self.u,self.manager+0x4a0,'I',23)  # default: actors still update
  self.actors=[self.manager+0x120,self.manager+0x2e0]
  for actor in self.actors:
   put(self.u,actor+0x10,'I',3)
   put(self.u,actor+0xe0,'3f',1000000.,0.,0.)
   put(self.u,actor+0x128+0x10,'Q',self.h+0x9000)
  put(self.u,self.h+0x9010,'I',0)  # no action callbacks in this fixture
 def hook(self,u,a,n,data):
  if a==self.g+0x37a1b0:
   self.duels.append((self.time,u.reg_read(UC_X86_REG_RCX),u.reg_read(UC_X86_REG_RDX)))
   return  # execute the captured original manager, not a mock return
  if a==self.g+0x10e90:
   self.ret(self.context);return
  if a==self.g+0x101de08:
   bits=u.reg_read(UC_X86_REG_XMM0)&0xffffffff
   value=struct.unpack('<f',struct.pack('<I',bits))[0]
   result=struct.unpack('<I',struct.pack('<f',math.sqrt(value)))[0]
   u.reg_write(UC_X86_REG_XMM0,result);self.ret(0);return
  super().hook(u,a,n,data)

for base,module in [(0x140000000,0x180000000),(0x7ff600000000,0x7ffd50000000)]:
 m=Machine(game=base,module=module);check(m.init()==1,'Initialize exact runtime code with ASLR '+hex(base))
 check([x[0]-base for x in m.records]==[0x151647,0x15322a,0x953fda,0x44ad50,0x36a7b5,0xf3e21f,0xf3e4ad],'Only seven declared patch20 sites committed '+hex(base))
 check(m.init()==1 and len(m.relays)==5,'Repeated init has no extra hooks '+hex(base))
 for i in range(8):m.render();m.update();m.time+=m.freq//120
 check(not m.waits and len(m.duels)==8,'Field / inactive duel uses R5 timing without additional waits '+hex(base))
 m.active();present=[];update=[]
 for i in range(80):
  m.update();update.append(m.duels[-1][0]);m.time+=m.freq*3//1000
  present.append(m.render());m.time+=m.freq*5//1000
 step=(m.freq+59)//60
 check(all(b-a>=step for a,b in zip(present,present[1:])),'Actual patched render calls limited to <=60 Hz '+hex(base))
 check(all(b-a>=step for a,b in zip(update,update[1:])),'Actual duel update calls limited to <=60 Hz '+hex(base))
 check(max(b-a for a,b in zip(present[3:],present[4:]))<m.freq/59,'Serial render/update gates stay near60, not30 '+hex(base))
 check(all(p==m.manager and f==1 for _,p,f in m.duels),'Original duel receives unchanged object and argument once per call '+hex(base))
 m.active(False);n=len(m.waits);m.render();m.update()
 check(len(m.waits)==n,'Duel teardown immediately removes both caps '+hex(base))
 check(get(m.u,base+0x1a8a5ec,'I')==60 and get(m.u,base+0x1a8a5f4,'I')==0x3c888888,'Shared native tables remain stock '+hex(base))

for mode in ['no_timer','timer_create_fail','timer_set_fail','oversleep']:
 m=Machine(mode);check(m.init()==1,'Timer scenario initializes: '+mode);m.active();times=[]
 for i in range(12):times.append(m.render());m.time+=m.freq//120
 check(all(b-a>=(m.freq+59)//60 for a,b in zip(times,times[1:])),'Clock cap holds without catch-up frames: '+mode)

m=Machine();assert m.init();m.active();m.render();m.update();n=len(m.waits)
put(m.u,m.manager,'Q',m.g+0x13a0570);m.render();m.update()
check(len(m.waits)==n,'Field CUnitDuel vtable with active-looking data cannot activate cap')
put(m.u,m.manager,'Q',m.g+0x13b5f98);m.active(2);m.render();m.update()
check(len(m.waits)==n,'Invalid activation value rejected')
put(m.u,m.g+0x2783730,'Q',0);m.render();m.update()
check(len(m.waits)==n,'Missing manager / title / shutdown has no cap')
m=Machine('unreadable_manager');assert m.init();m.active();m.render();m.update()
check(not m.waits,'Unreadable manager cannot activate cap')
m=Machine();assert m.init();m.active();m.update(m.manager+0x120);m.update(m.manager+0x120)
check(not m.waits,'Private / embedded duel unit update is not paced')
m=Machine();assert m.init();m.active();a=m.render();m.time+=m.freq//10;n=len(m.waits);b=m.render()
check(len(m.waits)==n and b-a>=m.freq//10,'A slow frame is not delayed or followed by a catch-up burst')
m.time=100;n=len(m.waits);m.render();check(len(m.waits)==n,'Clock reversal resets limiter without long blocking')
m.active(False);m.render();m.time+=m.freq;m.active();n=len(m.waits);m.render();check(len(m.waits)==n,'Second duel starts with fresh pacing state')

for rva in [0x151647,0x15322a,0x36a7b5,0xf3e21f,0xf3e4ad,0x44ad50,0x953fda,0x370900,0x372c25,0x379c04,0x37a1b6,0x953fc0,0x44ad49,0x1a8a5ec,0x1a8a5f4]:
 m=Machine();m.u.mem_write(m.g+rva,b'\xcc');check(m.init()==0 and not m.records and not m.relays,'Guard refuses unknown modification '+hex(rva))
for mode in ['relay_fail','transaction_reject','qpf_fail','qpc_fail']:
 m=Machine(mode);check(m.init()==0,'Initialization failure reported: '+mode)
 check(bytes(m.u.mem_read(m.g+0x151647,5))==bytes.fromhex('e8e4327e00') and bytes(m.u.mem_read(m.g+0x953fda,5))==bytes.fromhex('e86148feff'),'No partial 120/60 installation: '+mode)
 check(len(m.freed)==len(m.relays),'Failed initialization releases allocated relays: '+mode)

check(not getattr(dll,'DIRECTORY_ENTRY_IMPORT',[]),'No added DLL runtime dependency')
check(dll.verify_checksum(),'PE checksum is valid')
check(len(dll.DIRECTORY_ENTRY_EXCEPTION)==len(symbols),'Every compiled function has Win64 unwind metadata')

# Crucial new checks: run the ORIGINAL engine FPS read, native wait, fixed-step
# selection and propagation to the animation clock. No fabricated clock formula.
engine_report=[]
for label,duel in [('field',False),('duel',True),('field_restored',False)]:
 if label=='field':m=Machine();assert m.init()
 m.active(duel);start=m.time;values=[];times=[]
 for i in range(61):
  values.append(m.engine_tick());times.append(get(m.u,m.clock,'Q'))
  if duel:m.update();m.render()
  m.time+=m.freq//2000
 rate=60 if duel else 120
 check(all(v[0]==rate for v in values),'Original engine gets correct target FPS: '+label)
 check(all(abs(v[1]-1/rate)<1e-8 and v[1]==v[2] for v in values),'Original engine propagates correct timestep to animation clock: '+label)
 check(all(b-a>=m.freq//rate for a,b in zip(times,times[1:])),'Native engine pacing honors selected rate: '+label)
 if duel:
  check(0.99<=(times[-1]-times[0])/m.freq<1.09,'Complete native engine plus duel and render gates stay near60, not30')
  check(m.direction()==0x7a9 and m.directions[-1]==16,'Duel direction input is stock16 ms; native return preserved')
 else:check(m.direction()==0x7a9 and m.directions[-1]==8,'Nonduel direction input remains R5 8 ms: '+label)
 check(get(m.u,m.fpsvalue,'I')==120 and abs(get(m.u,m.cfg+0x34,'f')-1/120)<1e-8,'Stored120 configuration is never mutated: '+label)
 engine_report.append({'scene':label,'target_fps':values[-1][0],'fixed_step_seconds':values[-1][1],'animation_step_seconds':values[-1][2],'elapsed_seconds_for_60_intervals':(times[-1]-times[0])/m.freq})

# Baseline equality: same ORIGINAL engine code with an unpatched stock60 config.
stock=Machine();put(stock.u,stock.fpsvalue,'I',60);put(stock.u,stock.cfg+0x34,'f',1/60);put(stock.u,stock.clock+0x30,'I',60)
fixed=Machine();assert fixed.init();fixed.active()
for i in range(120):
 a=stock.engine_tick();b=fixed.engine_tick()
 assert a==b,(a,b)
 stock.time+=stock.freq//2000;fixed.time+=fixed.freq//2000
check(get(stock.u,stock.engine+0x3348,'f')==get(fixed.u,fixed.engine+0x3348,'f'),'Two seconds of native animation clock accumulation exactly match unpatched stock60')
check(stock.queries==fixed.queries and len(fixed.queries)==240,'Original configuration and FPS getters called once per frame, preserving side effects')
# Existing native pause flag wins over the fixed-step override.
# R13 reference at [rsp+48] supplies this flag; emulate downstream selection.
fixed.u.reg_write(UC_X86_REG_RSP,fixed.s+0x80000);put(fixed.u,fixed.s+0x80048,'Q',fixed.h+0x1d000);put(fixed.u,fixed.h+0x1d010,'I',8)
fixed.u.reg_write(UC_X86_REG_RBP,fixed.frame);fixed.u.reg_write(UC_X86_REG_R15,fixed.clock)
fixed.u.emu_start(fixed.g+0xf3e4c8,fixed.g+0xf3e554,count=100000)
check(get(fixed.u,fixed.clock+0x18,'f')==0.0 and get(fixed.u,fixed.engine+0x3350,'f')==0.0,'Native engine pause still sets both clock steps to zero')
# Both original fallback branches (no config, missing FPS pointer).
for nullfps in [False,True]:
 fallback=Machine();assert fallback.init();fallback.active();fallback.config_null=True;fallback.fps_null=nullfps
 put(fallback.u,fallback.frame+0xc4,'B',1)
 value=fallback.engine_tick()
 check(value[0]==60 and abs(value[1]-1/60)<1e-8,'Missing property/config uses stock60 native fallback: fpsnull='+str(nullfps))
 fallback.active(False);value=fallback.engine_tick()
 check(value[0]==120,'Native fallback restores original120 on duel exit: fpsnull='+str(nullfps))
# Bridge preserves every live volatile register and XMM register except EAX,
# which is the original destination. Test two independently relocated images.
for base,module in [(0x140000000,0x180000000),(0x7ff600000000,0x7ffd50000000)]:
 x=Machine(game=base,module=module);assert x.init();x.active()
 regs={UC_X86_REG_RAX:x.cfg,UC_X86_REG_R15:x.clock}
 vol=[UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9,UC_X86_REG_R10,UC_X86_REG_R11]
 regs.update({r:0x12344321+i for i,r in enumerate(vol)})
 regs.update({UC_X86_REG_XMM0+i:0x0123456789abcdef0123456789abcdef+i for i in range(6)})
 x.call(x.g+0xf3e4ad,end=x.g+0xf3e4b4,regs=regs)
 check(all(x.u.reg_read(r)==v for r,v in regs.items() if r not in [UC_X86_REG_RAX,UC_X86_REG_R15]),'Inline timestep bridge preserves live volatile and vector registers '+hex(base))
 check(get(x.u,x.clock+0x18,'I')==0x3c888889,'Bridge delivers exact stock float1/60 '+hex(base))

# Execute original duel movement/timeline in the user's stock60 game mode.
m=OriginalDuelMachine(1);assert m.init();m.active()
for i in range(61):m.engine_tick();m.update();m.render();m.time+=m.freq//2000
elapsed=(m.duels[-1][0]-m.duels[0][0])/m.freq
check(1.0<=elapsed<1.09,'61 real duel updates span one second at60 with native engine timing')
frames=[get(m.u,a+0x128,'f') for a in m.actors];positions=[get(m.u,a+0xc0,'f') for a in m.actors]
check(frames==[61.,61.],'Both original duel actor timelines advance one stock frame per update')
check(all(abs(x-1220.)<0.01 for x in positions),'Both original duel movements match the unmodified stock60 increment')
prior=OriginalDuelMachine(1);prior.active()
for i in range(61):prior.call(prior.g+0x37a1b0,prior.manager,1)
check(all(get(prior.u,a+0x128,'f')==frames[i] and get(prior.u,a+0xc0,'f')==positions[i] for i,a in enumerate(prior.actors)),'Original duel positions and timelines exactly equal stock60 after same elapsed updates')
check(get(m.u,m.context+8,'I')==1,'User FPS setting remains unchanged')
report={'passed':len(checks),'checks':checks,'original_engine_timing':engine_report,'stock60_animation_clock_exact_match':True,'original_duel_seconds_for_60_intervals':elapsed,'dll_sha256':hashlib.sha256((HERE/'SAN14_FPS120.dll').read_bytes()).hexdigest(),'scope':'Actual packaged x64 DLL, seven patched sites, captured native engine wait/FPS/timestep/animation-clock propagation and original duel manager/timeline/movement under Unicorn. Win32 APIs, config getter objects, work dispatcher, context provider and sqrt modeled. Resource-dependent combat states, sound, GPU and live Windows gameplay not tested.'}
(HERE/'validation/native.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
