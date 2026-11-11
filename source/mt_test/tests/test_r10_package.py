"""R10-MeritRules build/package/static audit. Not a live Windows/game test."""
from pathlib import Path
import json,hashlib,struct,subprocess,sys,collections
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'analysis'));from peutil import PE
OUT=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,text):
 if not ok:raise AssertionError(text)
 OUT.append({'name':text,'pass':True});print('PASS',text,flush=True)
def balanced(s):
 stack=[];pairs={')':'(',']':'[','}':'{'};i=0
 while i<len(s):
  if s.startswith('<#',i):
   z=s.find('#>',i+2)
   if z<0:return False
   i=z+2;continue
  if s[i]=='#':
   z=s.find('\n',i);i=len(s) if z<0 else z;continue
  if s[i] in ('"',"'"):
   q=s[i];i+=1
   while i<len(s):
    if q=='"' and s[i]=='`':i+=2;continue
    if s[i]==q:
     if q=="'" and i+1<len(s) and s[i+1]==q:i+=2;continue
     i+=1;break
    i+=1
   continue
  if s[i] in '([{':stack.append(s[i])
  elif s[i] in ')]}':
   if not stack or stack.pop()!=pairs[s[i]]:return False
  i+=1
 return not stack
pkg=json.loads((R/'package.manifest.json').read_text());meta=json.loads((R/'payload/SAN14Mods/manifests/MeritTraits.json').read_text())
check(pkg['Version']=='RC4-R10-MeritRules' and pkg['ReleaseReady'] is True,'R10 package version/release flag')
check(meta['version']=='RC4-R10-MeritRules' and meta['live_game_tested'] is False and pkg['LiveGameTested'] is False,'R10 live-game limitation explicit')
for key in ('same_turn_return','ai_reward_policy','ruler_veteran','collaboration_candidates'):check(key in meta,'R10 manifest section '+key)
check(meta['collaboration_candidates']['unique_traits']==39 and meta['collaboration_candidates']['teacher_records']==44,'R10 manifest declares 39 traits / 44 collaboration teachers')
inv={p.relative_to(R/'payload').as_posix() for p in (R/'payload').rglob('*') if p.is_file()};check(inv==set(pkg['Files']) and len(inv)==8,'exact eight install targets retained')
for n,h in pkg['Files'].items():check(sha(R/'payload'/n)==h,'payload SHA256 '+n)
old={n:sha(R/'source/build'/n) for n in ('SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll')}
subprocess.run(['bash',str(R/'source/build.sh')],check=True,stdout=subprocess.DEVNULL)
for n,h in old.items():
 check(sha(R/'source/build'/n)==h,'deterministic rebuild '+n)
 dest='SAN14Mods/MeritTraits/'+n if 'Resolver' in n else n
 check(pkg['Files'][dest]==h,'shipped DLL equals rebuilt '+n)
expected={'dxgi.dll':'653aaaea26842224533ec31d590562712ede9785ed2fe418be2ae85fb5974c8f','SAN14Mods/Core/SAN14_ModCore.dll':'f54c7d4848dacf0c2414b07c7573cdbac8bf3ca9b5aec84345d8f549b292fc67','SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll':'a0b52ed963d0ae3b4b70c32ac4f04e9f22c1ad01739c7cb447802f8a74d57e51','SAN14Mods/MeritTraits/UI_Traits9_Hover.dll':'2926c3ef05abd004bf0e5d9f7113321568800a6cf4d60b7e7e0bf25367b0f164'}
for n,h in expected.items():check(pkg['Files'][n]==h,'shared/support bytes unchanged '+n)
for p in (R/'payload').rglob('*.dll'):
 q=PE(p);check(struct.unpack_from('<H',q.b,q.nt+4)[0]==0x8664 and struct.unpack_from('<H',q.b,q.opt)[0]==0x20b,'Win64 PE '+p.name)
 check(all(not(f&0x80000000 and f&0x20000000) for _,_,_,_,_,f in q.sections),'no RWX '+p.name)
check('SAN14_ModInitV1' in PE(R/'payload/SAN14_MeritTraits.dll').exports,'feature DLL initialization export')
r9=json.loads((R/'audit/r9_release_baseline/package.manifest.json').read_text())
for n in ('SAN14_MeritTraits.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini'):
 check(r9['Files'][n] in pkg['RecognizedPrevious'][n],'exact R9 upgrade hash recognized '+n)
records=json.loads((R/'data/SAN14CE_콜라보39_교사44_DB.json').read_text())['records'];ids=collections.Counter(x['trait_id'] for x in records)
expected_ids=set(range(155,181))|set(range(197,201))|set(range(202,211))
check(len(records)==44 and set(ids)==expected_ids and len(ids)==39,'collaboration source DB exact 44/39 coverage')
check(all(ids[x]==2 for x in (155,161,162,163,164)) and all(v==1 for k,v in ids.items() if k not in (155,161,162,163,164)),'collaboration duplicate holder counts')
prov=json.loads((R/'data/collab_provenance.json').read_text());check(prov['sha256']==sha(R/'data/SAN14CE_콜라보39_교사44_DB.json') and prov['records']==44 and prov['exclusive_traits']==39,'generated collaboration provenance matches source DB')
for report in ('r10_tests.json','r10_r8_compat_tests.json','r6_tests.json','r7_tests.json','r9_tests.json','r7_compat_tests.json','cp03_stack_tests.json'):
 d=json.loads((R/'audit'/report).read_text());check(d['passed']>0 and len(d['tests'])==d['passed'],'executed report '+report)
for p in R.glob('*.ps1'):
 raw=p.read_bytes();check(raw.startswith(b'\xef\xbb\xbf') and b'\r\n' in raw,'Windows script encoding '+p.name);check(balanced(raw.decode('utf-8-sig')),'PowerShell balance '+p.name)
s=(R/'Installer.Common.ps1').read_text(encoding='utf-8-sig')
for token in ('Assert-GameClosed','With-MeritLock','Recover-Pending $Root','RecognizedPrevious','Invoke-MeritRollback','Test-PrivateDllLoad'):
 check(token in s,'installer protection '+token)
for p in R.glob('*.cmd'):
 s=p.read_text();check('DisableDelayedExpansion' in s and 'Sysnative' in s and '"%~dp0' in s,'quoted 64-bit wrapper '+p.name)
check('실게임' in (R/'검증결과.md').read_text() and '아직' in (R/'검증결과.md').read_text(),'verification document does not claim live-game success')
result={'version':'RC4-R10-MeritRules','scope':'Build/PE/package/script static checks; NOT Windows PowerShell or live game','passed':len(OUT),'checks':OUT,'dll_sha256':old}
(R/'audit/r10_package_tests.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print('PASS R10 PACKAGE',len(OUT))
