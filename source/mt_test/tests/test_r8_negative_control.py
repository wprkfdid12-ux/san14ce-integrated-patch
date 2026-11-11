"""Compare current DLL with immutable R7 DLL when --r7 points to source root.
This is a reproduction of two routing defects, not a game/crash reproduction.
"""
from test_r6 import R6,settled
import test_native as n
from test_native import *
import argparse,hashlib
p=argparse.ArgumentParser();p.add_argument('--r7',type=Path,required=True);args=p.parse_args()
original=n.Loaded;results=[]
for root in [args.r7,ROOT]:
 def loaded(path,bridge=None,mapfile=None):
  return original(root/'source/build'/path.name,bridge,None) if path.name in ['SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll'] else original(path,bridge,mapfile)
 n.Loaded=loaded
 for route in ['shared_code_1','bottom_group_hit']:
  e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s)
  if route=='shared_code_1':e.fn('hook_close',e.root_layout.addr,1)
  else:
   g=e.cancel_button;e.ctor(g.addr);e.widget_init(g.addr,600,900,500,120,0,3)
   e.input.u32(0x34,640);e.input.u32(0x38,950);e.current_target=g.addr
   e.keys={1};e.fn('hook_state_tick',s.addr);e.keys=set()
  e.fn('hook_state_tick',s.addr)
  warning=bool(e.mod.val('exit_modal') and e.mod.val('warning_kind')==1)
  results.append({'dll':str(root/'source/build/SAN14_MeritTraits.dll'),'sha256':hashlib.sha256((root/'source/build/SAN14_MeritTraits.dll').read_bytes()).hexdigest(),'route':route,'incorrect_back_warning':warning})
  assert warning==(root==args.r7),(root,route,warning)
  e.closeall()
n.Loaded=original
(ROOT/'audit/r8_negative_control.json').write_text(json.dumps({'scope':__doc__,'results':results},indent=2)+'\n')
print('PASS: R7 reproduces both wrong-cancel routes; R8 rejects both')
