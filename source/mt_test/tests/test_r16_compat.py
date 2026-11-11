"""Exact supplied relation patch: file identity and native write/guard compatibility.
Does not certify simultaneous live rendering, arbitrary third-party mods, or FPS.
"""
from test_native import *
import re

def num(x):return int(x,0) if isinstance(x,str) else x
def overlaps(a,b):return a[0]<b[0]+b[1] and b[0]<a[0]+a[1]
def run():
 e=Env();r=e.res;ref=ROOT/'references/related_officers_rc24';meta=json.loads((ref/'package.manifest.json').read_text(encoding='utf-8-sig'))
 def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
 for name in ('SAN14Mods/Core/SAN14_ModCore.dll','SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll'):
  check(sha(ROOT/'payload'/name)==sha(ref/'payload'/name)==meta['Files'][name],'same shared dependency bytes: '+name)
 rel_audit=json.loads((ref/'COMPATIBILITY_AUDIT_RC24.json').read_text())
 relation_writes=[(num(x['rva']),x['length'],'relation hook') for x in rel_audit['hooks']]
 profile=json.loads((ref/'source/relation_profile.json').read_text())
 relation_guards=[(num(x['rva']),len(bytes.fromhex(x['bytes'])),x['name']) for x in profile['patterns']]
 relation_guards += [(num(x['rva']),len(bytes.fromhex(x['bytes'])),x['name']) for x in json.loads((ref/'source/input_profile.json').read_text())]
 writes=[(x.rva,x.length,'merit hook') for x in r.hooks[:r.hook_count]]
 writes += [(a,b,'nine native traits') for a,b in zip(r.patch_rva,r.length)]
 writes += [(r.selection.counts_site,5,'selection counts'),(r.selection.refresh_site,5,'selection refresh'),(0x4915a1,14,'information bridge')]
 guards=[(a,16,'function') for a in r.funcs if a]+[(a,n,'native function') for a,n in zip(r.native.funcs,r.native.guard_lengths) if a and n]
 guards += [(a,16,'popup') for a in r.popup.funcs if a]
 guards += writes
 for file,key in [('r11_native_boundaries.json','blocks'),('r15_domestic_native.json','blocks'),('r16_trait_native.json','blocks')]:
  for name,x in json.loads((ROOT/'analysis'/file).read_text())[key].items():
   if file=='r11_native_boundaries.json' and name not in ('eligible','rank','current_rank','ruler_status'):continue
   guards.append((x['rva'],len(bytes.fromhex(x['bytes'])),name))
 conflicts=[]
 for label,left,right in [('write/write',writes,relation_writes),('merit write/relation guard',writes,relation_guards),('relation write/merit guard',relation_writes,guards)]:
  for a in left:
   for b in right:
    if overlaps(a,b):conflicts.append({'kind':label,'a':a,'b':b})
 check(not conflicts,'39 resolved merit hooks plus selection/nine-slot/display writes do not overlap six relation hooks or each other\'s native guards',conflicts)
 # The six owned Hover DLL call redirects do not alter the exported refresh
 # entry or game instructions observed by the relation resolver.
 check(sha(ROOT/'payload/SAN14Mods/MeritTraits/UI_Traits9_Hover.dll')=='2926c3ef05abd004bf0e5d9f7113321568800a6cf4d60b7e7e0bf25367b0f164','shared trait Hover DLL remains byte-identical on disk')
 source=(ref/'source/related_officers.c').read_text()
 check('memcopy(c->columns,oldcols,ncols*sizeof(RColumn))' in source and 'c->columns[ncols+i]' in source and 'attach_after_copy' in source,'supplied relation source copies expanded trait descriptors before appending its nine columns')
 pkg=json.loads((ROOT/'package.manifest.json').read_text())
 check(sha(ref/'payload/dxgi.dll') in pkg['KnownCoreLoaders']['dxgi.dll'],'exact ProxyCompat1 dxgi hash recognized for screenshot conflict')
 for name,spec in pkg['Loaders'].items():
  pe=PE(ROOT/'payload'/spec['Source'])
  check(sha(ROOT/'payload'/spec['Source'])==spec['SHA256'] and spec['AnchorExport'] in pe.exports,'verified alternative loader '+name)
  raw=pe.b
  check(b'SAN14_CoreStart' in raw and ('SAN14Mods\\Core\\SAN14_ModCore.dll'.encode('utf-16le') in raw or b'SAN14Mods\\Core\\SAN14_ModCore.dll' in raw),'alternative loader targets same common Core: '+name)
 report={'passed':len(REPORT),'tests':REPORT,'conflicts':conflicts,'merit_writes':[{'rva':hex(a),'length':b,'kind':c} for a,b,c in writes],'relation_hooks':rel_audit['hooks'],'scope':__doc__,'live_game_tested':False,'relation_files':{p.relative_to(ref/'payload').as_posix():sha(p) for p in (ref/'payload').rglob('*') if p.is_file()}}
 (ROOT/'audit/r16_compat_tests.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');e.closeall()

if __name__=='__main__':run()
