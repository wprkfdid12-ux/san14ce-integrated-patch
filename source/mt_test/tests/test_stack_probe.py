"""Execute the rebuilt PE's stack probe on a controlled Linux stack.
This tests register preservation/page touching, not Windows SEH/guard-page growth.
"""
from test_rc4 import *
import signal, resource
report=[]
def verify(ok,name):
 assert ok,name
 report.append(name);print('PASS',name)
e=E();stack=Mapping(18*4096);libc=C.CDLL(None)
try:
 # Read the address from THIS binary's diagnostic catalogue. A map retained
 # from an older compiler/build can otherwise test an unrelated instruction.
 helper=e.mod.sym('__chkstk');rva=helper-e.mod.addr
 source=ROOT/'tests/probe_bridge.s';so=source.with_suffix('.so')
 subprocess.run(['gcc','-shared','-fPIC',str(source),'-o',str(so)],check=True)
 bridge=C.CDLL(str(so));bridge.probe_test.argtypes=[U,U,U];bridge.probe_test.restype=C.c_int
 top=stack.addr+17*4096
 for n in (0,1,4095,4096,4097,8192,65536):
  verify(bridge.probe_test(helper,n,top)==0,f'PE __chkstk preserves RAX/RSP/arguments for {n} bytes')
 verify(any(a==rva and u for a,z,u in e.mod.pe.funcs),'probe has PE unwind metadata')
 callers=0
 for name in ('g_distribution','format_log_item'):
  r=e.mod.sym(name)-e.mod.addr; dis=e.mod.pe.dis(r)
  verify(re.search(r'call\s+0x'+format(e.mod.pe.base+rva,'x')+r'\b',dis) is not None,name+' calls real stack probe before allocation')
  (ROOT/'audit'/('cp03_'+name+'_stack.asm')).write_text(dis)
 libc.mprotect.argtypes=[C.c_void_p,C.c_size_t,C.c_int];libc.mprotect.restype=C.c_int
 assert libc.mprotect(stack.addr+8*4096,4096,0)==0
 pid=os.fork()
 if pid==0:
  resource.setrlimit(resource.RLIMIT_CORE,(0,0));bridge.probe_test(helper,65536,top);os._exit(0)
 _,status=os.waitpid(pid,0)
 verify(os.WIFSIGNALED(status) and os.WTERMSIG(status)==signal.SIGSEGV,'probe touches inaccessible intermediate page instead of skipping it')
finally:
 stack.close();e.closeall()
 (ROOT/'audit/cp03_stack_tests.json').write_text(json.dumps({'scope':'Production PE code on controlled Linux stack; not Windows SEH','passed':len(report),'tests':report},indent=2))
print('COMPLETE',len(report))
