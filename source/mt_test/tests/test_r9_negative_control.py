"""Immutable R8 vs current: independent main/transition object layout and actual
shipped Hover release routine. Reproduces rejected commit/lost context, not the
complete live game crash (native allocator/GPU cleanup endpoint is substituted).
"""
from test_r9 import hover_context_case
from test_r6 import R6,settled
from test_native import *
import test_native as n
import argparse
p=argparse.ArgumentParser();p.add_argument('--r8',type=Path,required=True);a=p.parse_args()
original=n.Loaded;results=[]
for root in [a.r8.resolve(),ROOT]:
 def loaded(path,bridge=None,mapfile=None):
  return original(root/'source/build'/path.name,bridge,None) if path.name in ['SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll'] else original(path,bridge,mapfile)
 n.Loaded=loaded
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);e.bottom(s)
 committed=e.native_commits;assert committed==int(root==ROOT),(root,committed)
 e.closeall()
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s);e.request_back(s);e.answer_exit(s,True)
 restored=e.selection_counts(596)==(0,0);assert restored==(root==ROOT)
 e.closeall()
 e=R6();o,r,s=e.prepare();e.click(r,0);settled(e,s)
 received=hover_context_case(e,r);correct=received==[e.resources.addr]
 assert correct==(root==ROOT),(root,received)
 assert len(received)==1
 results.append({'dll_sha256':hashlib.sha256((root/'source/build/SAN14_MeritTraits.dll').read_bytes()).hexdigest(),'baseline':root!=ROOT,'native_commit_count':committed,'cancel_restores_selection':restored,'hover_resource_context_preserved':correct,'hover_received_null':received==[0]})
 e.closeall()
n.Loaded=original
(ROOT/'audit/r9_negative_control.json').write_text(json.dumps({'scope':__doc__,'results':results},indent=2)+'\n')
print(json.dumps(results,indent=2))
