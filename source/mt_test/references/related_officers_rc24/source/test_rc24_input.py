"""Execute compiled RC2.4 input adapter / native row delegate bridges.
Uses a reconstructed UI fixture, not a running game or visual UI test.
"""
from test_rc24_feature import *
m=Machine();m.resolver_tests();m.setup_feature()
def globalq(name,value):m.q(m.rb+m.rm['globals'][name][0],value)
def call_original_dispatch(d,index):
 delegate=m.new(16);m.q(delegate+8,d['parent']);e=m.new(0x100);m.u(e+0x80,index);ref=m.new(8);m.q(ref,e)
 return m.call(m.rb+m.rm['functions']['rel_dispatch_bridge'][0],delegate,ref)
def install_dispatch_fixture(d):
 # Keep RTTI prefix and give the original delegate its native virtual slot.
 old=m.q(d['parent']);vt=m.new(0x310);C.memmove(vt,old-8,128);m.q(d['parent'],vt+8)
 m.q(vt+8+0x238,m.stub(lambda a:m.native_calls.append(('OriginalVirtualRow',*a[:2])) or 0xabc))
globalq('relation_pin_callback',m.cb.Pin)
d=m.new_dialog(mode=1,npeople=30);b=m.activate(d,d['n']-1);install_dispatch_fixture(d)
m.native_calls.clear();ret=call_original_dispatch(d,4)
m.check('actual InfoList row delegate bridge consumes Relation click before virtual detail handler',ret==0 and m.headers(d)[0]=='무장4' and not any(x[0]=='OriginalVirtualRow' for x in m.native_calls))
m.activate(d,0);ret=call_original_dispatch(d,5)
m.check('same compiled delegate bridge preserves virtual detail handler outside Relation',ret==0xabc and any(x[0]=='OriginalVirtualRow' for x in m.native_calls))
m.activate(d,d['n']-1);call_original_dispatch(d,4) #unpin
native=m.new_dialog(mode=2);m.activate(native,native['n']-1);install_dispatch_fixture(native);m.native_calls.clear();ret=call_original_dispatch(native,2)
m.check('checkbox row delegate always reaches original virtual handler',ret==0xabc and any(x[0]=='OriginalVirtualRow' for x in m.native_calls))
# Real compiled adapter helpers with real validated native getter/row helpers.
app=m.new(0x200);inp=m.new(0x100);globalq('rel_get_app',m.stub(lambda a:app));globalq('rel_get_input',m.stub(lambda a:inp))
API.from_address(m.rb+m.rm['globals']['api'][0]).Valid=m.r.Valid
# rel_hover_target and rel_row_index still point to reference native machine code.
# Widget registry vtable is a fixture, while the row-widget helper is native.
target=m.new(0x100);control=m.new(0x100);r1=m.new(0x40);r2=m.new(0x40)
m.q(b+0x138,r1);m.q(r1+0x20,r2);m.q(r2+0x20,control);m.u(control+0x58,10);m.u(control+0x5c,4);B.from_address(control+0x68).value=1
regvt=m.new(32);cell=m.new(8);m.q(cell,target);m.q(control+0x70,regvt);m.q(regvt+0x10,m.stub(lambda a:cell))
m.q(target+0x48,b);m.u(target+0x80,2);m.q(app+0x170,target)
# Native logical geometry (parent-relative), including dynamic name header span.
def widget(w,x,y,width,height,parent=0):
 m.q(w+8,parent)
 for off,v in [(0x20,x),(0x24,y),(0x28,width),(0x2c,height),(0x40,3)]:m.u(w+off,v)
widget(d['list'],100,100,800,600)
widget(b,0,50,800,500,d['list']);widget(target,0,80,800,40,b)
header=m.new(0x100);widget(header,0,0,140,40,d['list']);m.u(header+0x80,0)
headers=m.new(16);m.q(headers+8,header);m.q(d['list']+0x210,headers);m.q(d['list']+0x218,headers+16)
m.u(inp+0x34,120);m.u(inp+0x38,240)
hp=m.rapi.HoverPerson
m.check('compiled hover adapter uses real native scroll-index helper (4+2) to find officer',m.call(hp,b,d['list'])==d['people'][6])
m.u(control+0x5c,12);m.check('native scrolling changes hover reference via live data index, not old screen row',m.call(hp,b,d['list'])==d['people'][14])
m.q(d['rows']+14*8,d['people'][27]);m.check('hover follows actual sorted officer array',m.call(hp,b,d['list'])==d['people'][27])
m.u(inp+0x34,300);m.check('symbol-area hover outside name column is excluded',m.call(hp,b,d['list'])==0);m.u(inp+0x34,120)
m.u(inp+0x38,800);m.check('cursor outside native visible list is excluded',m.call(hp,b,d['list'])==0);m.u(inp+0x38,240)
m.u(app+0x18c,1);m.check('native modal/app hover lock disables preview',m.call(hp,b,d['list'])==0);m.u(app+0x18c,0)
m.q(target+0x48,native['bodies'][0]);m.check('row from another window cannot provide a reference',m.call(hp,b,d['list'])==0);m.q(target+0x48,b)
m.u(target+0x80,100);m.check('stale or out-of-range visible row rejected',m.call(hp,b,d['list'])==0);m.u(target+0x80,2)
m.u(target+0x40,0);m.check('hidden row rejected',m.call(hp,b,d['list'])==0);m.u(target+0x40,3)
m.q(header+8,header);m.check('corrupt/cyclic geometry chain fails closed',m.call(hp,b,d['list'])==0);m.q(header+8,d['list'])
m.q(app+0x170,0);m.check('no native hover target clears preview',m.call(hp,b,d['list'])==0);m.q(app+0x170,target)
# Exercise actual bridge with RBX supplied exactly as original body tick.
asmp=HERE/'tick_bridge_driver.s'
asmp.write_text('''.text
.globl call_tick_bridge
.type call_tick_bridge,@function
call_tick_bridge:
pushq %rbx
subq $32,%rsp
movq %rdi,%rax
movq %rsi,%rbx
call *%rax
addq $32,%rsp
popq %rbx
ret
.size call_tick_bridge,.-call_tick_bridge
.section .note.GNU-stack,"",@progbits
''')
subprocess.run(['gcc','-shared','-fPIC',str(asmp),'-o',str(HERE/'tick_bridge_driver.so')],check=True)
driver=C.CDLL(str(HERE/'tick_bridge_driver.so'));driver.call_tick_bridge.argtypes=[C.c_void_p,C.c_void_p];driver.call_tick_bridge.restype=Q
seen=[];globalq('relation_input_getter',m.stub(lambda a:seen.append(('getter',)) or 0x1122334455667788));globalq('relation_input_callback',m.stub(lambda a:seen.append(('observer',a[0])) or 0xdeadbeef))
ret=driver.call_tick_bridge(m.rb+m.rm['functions']['rel_input_bridge'][0],b)
m.check('post-input machine-code bridge preserves original getter RAX and forwards native RBX body',ret==0x1122334455667788 and seen==[('getter',),('observer',b)] and not m.errors)
# Complete production helper -> Tick state -> original relation header path.
m.r.HoverPerson=hp;globalq('relation_input_callback',m.cb.Tick);m.q(d['rows']+14*8,d['people'][14]);
ret=driver.call_tick_bridge(m.rb+m.rm['functions']['rel_input_bridge'][0],b)
m.check('compiled native-input bridge through actual hover adapter updates feature reference',m.headers(d)[0]=='무장14' and ret==0x1122334455667788)
call_original_dispatch(d,14);m.u(control+0x5c,20);driver.call_tick_bridge(m.rb+m.rm['functions']['rel_input_bridge'][0],b)
m.check('full bridge path keeps clicked reference while scrolling to different hover row',m.headers(d)[0]=='무장14')
m.call(m.cb.Destroy,d['list']);m.call(m.cb.Destroy,native['list']);m.check('input test cleanup leaves no feature-owned allocations',not m.heap)
(HERE/'test_rc24_input_results.json').write_text(json.dumps({'passed':len(m.tests),'tests':m.tests,'logs':m.logs},ensure_ascii=False,indent=2));print('TOTAL',len(m.tests))
