"""R8-CommandRouting packaging/static audit. Preserves previous rules; not live game."""
from pathlib import Path
import json,hashlib,struct,subprocess,sys,re
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'analysis'));from peutil import PE
out=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,text):
 if not ok:raise AssertionError(text)
 out.append({'name':text,'pass':True});print('PASS',text,flush=True)
def balanced(s):
 stack=[];i=0;pairs={')':'(',']':'[','}':'{'}
 while i<len(s):
  if s.startswith('<#',i):
   z=s.find('#>',i+2)
   if z<0:return False
   i=z+2;continue
  if s[i]=='#':
   z=s.find('\n',i);i=len(s)if z<0 else z;continue
  if s.startswith("@'",i)or s.startswith('@"',i):
   z=s.find('\n'+s[i+1]+'@',i+2)
   if z<0:return False
   i=z+3;continue
  if s[i]in("'",'"'):
   q=s[i];i+=1;closed=False
   while i<len(s):
    if q=='"'and s[i]=='`':i+=2;continue
    if s[i]==q:
     if q=="'"and i+1<len(s)and s[i+1]==q:i+=2;continue
     i+=1;closed=True;break
    i+=1
   if not closed:return False
   continue
  if s[i]in'([{':stack.append(s[i])
  elif s[i]in')]}':
   if not stack or stack.pop()!=pairs[s[i]]:return False
  i+=1
 return not stack
pkg=json.loads((R/'package.manifest.json').read_text());meta=json.loads((R/'payload/SAN14Mods/manifests/MeritTraits.json').read_text())
check(pkg['Version']=='RC4-R8-CommandRouting'and pkg['ReleaseReady'] is True,'R7 installer enabled, incremental continuation of our R6')
check(pkg['LiveGameTested']is False and meta['live_game_tested']is False,'live-game limitation is explicit in both manifests')
inv={p.relative_to(R/'payload').as_posix()for p in(R/'payload').rglob('*')if p.is_file()};check(inv==set(pkg['Files'])and len(inv)==8,'exact eight install target allowlist')
for n,h in pkg['Files'].items():check(sha(R/'payload'/n)==h,'payload SHA256 '+n)
old={n:sha(R/'source/build'/n)for n in ['SAN14_MeritTraits.dll','SAN14_MeritResolver_CEKO.dll']}
subprocess.run(['bash',str(R/'source/build.sh')],check=True)
for n,h in old.items():
 check(sha(R/'source/build'/n)==h,'deterministic build '+n)
 dest='SAN14Mods/MeritTraits/'+n if 'Resolver'in n else n
 check(pkg['Files'][dest]==h,'shipped DLL equals executed test build '+n)
expected={'dxgi.dll':'653aaaea26842224533ec31d590562712ede9785ed2fe418be2ae85fb5974c8f','SAN14Mods/Core/SAN14_ModCore.dll':'f54c7d4848dacf0c2414b07c7573cdbac8bf3ca9b5aec84345d8f549b292fc67','SAN14Mods/resolvers/SAN14_Resolver_CEKO.dll':'a0b52ed963d0ae3b4b70c32ac4f04e9f22c1ad01739c7cb447802f8a74d57e51','SAN14Mods/MeritTraits/UI_Traits9_Hover.dll':'2926c3ef05abd004bf0e5d9f7113321568800a6cf4d60b7e7e0bf25367b0f164'}
for n,h in expected.items():check(pkg['Files'][n]==h,'common/support bytes unchanged '+n)
for p in(R/'payload').rglob('*.dll'):
 q=PE(p)
 check(struct.unpack_from('<H',q.b,q.nt+4)[0]==0x8664 and struct.unpack_from('<H',q.b,q.opt)[0]==0x20b,'Win64 architecture '+p.name)
 check(all(not(f&0x80000000 and f&0x20000000)for _,_,_,_,_,f in q.sections),'no RWX section '+p.name)
 check(any(a<=q.entry<a+n and f&0x20000000 for _,a,n,_,_,f in q.sections),'valid PE entry '+p.name)
 check({d.lower()for d,n in q.imports.values()}<={'kernel32.dll','user32.dll','bcrypt.dll'},'OS-only direct imports '+p.name)
 if p.name=='SAN14_MeritTraits.dll':check('SAN14_ModInitV1'in q.exports and q.dirs[3][1]>0,'feature initialization export and x64 unwind metadata')
check(len(meta['hooks'])==35 and len(meta['native_function_guards'])==27,'35 hooks plus 27 native adapters described')
check(meta['presentation']['duration_ms']==1200 and meta['presentation']['gold_minimum_skip_ms']==600 and not meta['presentation']['guaranteed_third_gold'],'presentation and all-three-equal rules retained')
check(meta['storage']['steam_cloud'].startswith('mod companions are LOCAL'),'local companion cloud limitation not hidden')
# If private original bytes are available, independently compare compiled adapter
# fingerprints and original hook sites. Private whole game images are not shipped.
private=Path('/mnt/data/private_references/game_analysis_only.bin')
if private.exists():
 pe=PE(private)
 check(all(pe.m[g['rva']:g['rva']+16].hex()==g['expected_first16']for g in meta['function_guards']+meta['native_function_guards']),'all 48 function guards match private original game analysis bytes')
 check(all(pe.m[h['rva']:h['rva']+h['length']].hex()==h['expected']for h in meta['hooks']),'all 35 hook bytes match private original game')
for p in R.glob('*.ps1'):
 raw=p.read_bytes();s=raw.decode('utf-8-sig');check(raw.startswith(b'\xef\xbb\xbf')and b'\r\n'in raw,'Korean Windows script encoding '+p.name);check(balanced(s),'static PowerShell string/delimiter balance '+p.name)
s=(R/'Installer.Common.ps1').read_text(encoding='utf-8-sig')
for token,name in [('Assert-GameClosed','running game protection'),('With-MeritLock','exclusive installation lock'),('Recover-Pending $Root','interrupted transaction recovery'),('if ($h -ne $previous.ExpectedHash)','externally modified own-file protection'),('if ($script:Targets -notcontains $Name)','target path allowlist'),('Test-PrivateDllLoad','Windows native DLL load/export probe before copying'),('RecognizedPrevious','exact-hash RC3 migration'),('function Invoke-MeritRollback','one-click previous patch restoration'),("State-Path $Root 'last-change.json'",'durable previous transaction record'),('if ($script:Shared -contains $e.Name) { continue }','rollback preserves shared components'),('if (-not $e.Managed -or $script:Shared -contains $e.Name) { continue }','uninstall preserves shared/unowned files'),("$changes.Count -eq 0 -and $old -and $old.Active",'idempotent reinstall retains prior rollback history')]:check(token in s,name)
check(s.index('# Validate every conflict')<s.index('foreach ($entry in $entries)'),'all install conflicts checked before backing up or changing targets')
for p in R.glob('*.cmd'):
 s=p.read_text();check('DisableDelayedExpansion'in s and 'Sysnative'in s and '"%~dp0'in s,'quoted 64-bit launch wrapper '+p.name)
backup=(R/'BackupSaves.ps1').read_text(encoding='utf-8-sig');check('mt4ref'in backup and 'slot[01]'in backup and 'SHA256=$hash'in backup,'backup includes native saves, two generation files and slot references with hashes')
check('16384'in backup and '1GB'in backup and 'Assert-GameClosed'in backup,'backup bounds and consistent closed-game snapshot')
check(meta['logging']['files']==4 and meta['logging']['bytes_per_file']==2097152,'diagnostic logs bounded to four two-MiB files')
reports=['r6_tests.json','r7_tests.json','r8_tests.json','r7_compat_tests.json','cp03_stack_tests.json'];total=0
for name in reports:
 d=json.loads((R/'audit'/name).read_text());count=d['passed'];check(count>0 and len(d['tests'])==count and all(x['pass'] if isinstance(x,dict) else bool(x) for x in d['tests']),'executed-code result '+name);total+=count
oldpkg=json.loads((R/'audit/r1_release_baseline/package.manifest.json').read_text())
for name in ['SAN14_MeritTraits.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini']:
 check(oldpkg['Files'][name] in pkg['RecognizedPrevious'][name],'recognized exact R1 hash for upgrade '+name)
check(meta['ability_comparison']['target_official'].startswith('original official DB') and meta['ability_comparison']['teachers']=='original official DB' and meta['ability_comparison']['game_abilities']=='never overwritten','target and comparison DB basis matches final user instruction')
check(meta['presentation']['auto_gratitude_on_input_release'] is False,'automatic thanks on input release is disabled')
check(meta['evaluation']['observation_only'] is True,'result assessment cannot adjust draw probabilities')
r2pkg=json.loads((R/'audit/r2_release_baseline/package.manifest.json').read_text())
for name in ['SAN14_MeritTraits.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini']:
 check(r2pkg['Files'][name] in pkg['RecognizedPrevious'][name],'recognized exact R2 hash for upgrade '+name)
check(meta['storage']['schema'].startswith('S14MT001 v3') and 'v2' in meta['storage']['migration'],'v3 wire extension and v2 migration declared')
check(meta['presentation']['reroll_scope'].startswith('PER OFFICER') and meta['presentation']['rerolls_shared']==5,'five rerolls shared only within one officer three chances')
check(meta['miss']['initial_probability']==.05 and len(meta['miss']['excluded'])==3,'initial MISS tuning and three exclusion rules declared')
check(meta['passion']['fit_weight_multiplier']==1.35 and meta['passion']['gold_weight_multiplier']==1.5,'passion tuning explicitly matches compiled parameter block')
check(meta['evaluation']['miss'].endswith('null'),'MISS does not receive a six-level grade')
x=meta['hover_x_extension'];pe=PE(R/'payload/SAN14Mods/MeritTraits/UI_Traits9_Hover.dll')
check(x['site_count']==7 and x['info_bridge_game_rva']==0x4915a1,'separate seven-site X bridge contract')
for sites,dest in [(x['collector_call_rvas'],0x2080),(x['column_call_rvas'],0x3610)]:
 check(all(pe.m[r]==0xe8 and r+5+struct.unpack_from('<i',pe.m,r+1)[0]==dest for r in sites),'shipped Hover call-site targets '+hex(dest))
check('SAN14_MeritSlotDisplayV1' in PE(R/'payload/SAN14_MeritTraits.dll').exports,'shared read-only trait/X display API is exported')
check(any('independent custom mod panels' in t for t in meta['known_limitations']) and any('No verified X icon' in t for t in meta['known_limitations']),'unverified custom panels and native X icon are disclosed')
check('v3' in meta['storage']['downgrade'] and 'pre-R3' in meta['storage']['downgrade'],'older-DLL rollback requires matching pre-R3 save backup')
r3pkg=json.loads((R/'audit/r3_release_baseline/package.manifest.json').read_text())
for name in ['SAN14_MeritTraits.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini']:
 check(r3pkg['Files'][name] in pkg['RecognizedPrevious'][name],'recognized exact R3 hash for upgrade '+name)
baseline=json.loads((R/'audit/r4_release_baseline/SHA256.json').read_text())
for name in ['slot_store.inc','display_patch.inc','hover_x_adapter.inc','presets_x_adapter.inc','merit_display_api.h']:
 check(sha(R/'source'/name)==baseline['source/'+name],'R4 storage/compatibility source unchanged '+name)
check(sha(R/'data/SAN14CE_공식무장1000_DB.json')==baseline['data/SAN14CE_공식무장1000_DB.json'],'original official comparison DB unchanged')
px=meta['sortie_presets_x_extension'];pe=PE(R/'references/compatibility/sortie_rc25/SAN14_SortiePresets.dll')
check(px['optional'] is True and px['site_count']==13 and px['image_size']==pe.size,'RC25 optional display-only bridge scope declared')
check(px['dll_sha256']==sha(pe.path) and px['text_sha256']==hashlib.sha256(pe.m[0x1000:0x16168]).hexdigest(),'exact supplied RC25 file and text fingerprints')
check(all(pe.m[a]==0xe8 and a+5+struct.unpack_from('<i',pe.m,a+1)[0]==px['original_refresh_rva'] for a in px['call_rvas']),'thirteen original RC25 calls all target its unchanged refresh function')
check(not any('SortiePresets' in p for p in pkg['Files']),'optional preset bridge adds no other-mod file to installer targets')
check((R/'source/merit_display_api.h').exists() and 'before initialization' in meta['hover_x_extension']['export_lifetime'],'public display API lifetime/fallback contract documented')
for previous in ['r4_release_baseline/package.manifest.json','r5_release_baseline/package.manifest.json','r6_release_baseline/package.manifest.json','r7_release_baseline/package.manifest.json','other_r5_recognition.json']:
 d=json.loads((R/'audit'/previous).read_text())
 for name in ['SAN14_MeritTraits.dll','SAN14Mods/MeritTraits/SAN14_MeritResolver_CEKO.dll','SAN14Mods/manifests/MeritTraits.json','SAN14Mods/modules/MeritTraits.ini']:
  check(d['Files'][name] in pkg['RecognizedPrevious'][name],'recognized exact previous hash '+previous+' '+name)
c=meta['exit_contract']
check(c['back_after_roll']=='discard_unconfirmed_results' and c['refund_rerolls'] is False and c['back_before_roll']=='native_cancel_without_reward','latest cancel/result/ticket rule is unambiguous')
check(c['commit_route'].startswith('bottom decision: one original decision') and 'without award commit' in c['commit_route'] and c['separate_top_accept'] is False and c['separate_top_exit'] is False,'single original exit chain and removed upper controls documented')
check(c['back_warning']['buttons']==['예','아니오'] and c['back_warning']['no']=='retain_current_result_and_tickets' and c['back_warning']['yes']=='discard_and_native_cancel','latest explicit Yes/No warning and nonrefunding No rule declared')
check(meta['presentation']['calculation_button']=='계산 근거' and meta['presentation']['reroll_alignment']=='native Selection baseline, height and font','requested native button wording/typography documented')
check(meta['miss']['apply'].startswith('successful original bottom decision') and 'neither X nor passion' in meta['miss']['apply'],'MISS is applied only on Decision, never on Back cancellation')
check((R/'analysis/user_video_log_review.md').exists() and (R/'references/proposals/R6_NativeChoice_추가수정지시서.txt').exists(),'video/log evidence and separate incremental instruction are preserved')
check(meta['native_choice']['example'].endswith('exactly one') and len(meta['native_choice']['bindings'])==2,'shared native choice limit and two verified bindings declared')
check(c['replacement_warning']['buttons']==['예','아니오'],'explicit replacement warning contract declared')
p=meta['native_notice'];evidence=json.loads((R/'analysis/r7_popup_evidence.json').read_text())
check(p['class']=='CSimpleBalloonLayout' and p['parentless'] and p['separate_text_lines']==6,'independent native window and explicit text-line contract declared')
check(p['object_size']==384 and p['vtable']==evidence['vtable'] and len(p['function_guards'])==9,'nine popup bindings and native object layout declared')
check(all(a['rva']==b['rva'] and a['expected_first16']==b['first16'] for a,b in zip(p['function_guards'],evidence['functions'])),'popup fingerprints match supplied original native captures')
check(p['visibility_failure']=='unlock_without_consent' and p['picker']=='suspend_then_restore_with_fresh_callbacks','hidden-dialog recovery and officer-selector lifetime documented')
check((R/'source/native_notice.inc').exists() and (R/'analysis/r7_user_evidence_review.md').exists() and (R/'references/proposals/R7_NativeDialogs_추가수정지시서.txt').exists(),'R7 incremental implementation, evidence review and separate directive preserved')
routing=meta['command_routing']
check(routing['cancel_code']=='0x7fffffff' and routing['group_rectangle_fallback'] is False,'only explicit native cancel can request discard; bottom group fallback removed')
check(p['button_parentless'] and p['layers']==[6,6] and p['blank_click_consent'] is False,'notice buttons independent and blank clicks never grant consent')
check(any(h['kind']=='H_UI_ENTER' and h['rva']==0x1447cc8 and h['target']==0x6b4f20 for h in meta['hooks']),'lifecycle registration hook guards one unregister per native registration')
negative=json.loads((R/'audit/r8_negative_control.json').read_text())
check([r['incorrect_back_warning'] for r in negative['results']]==[True,True,False,False],'immutable R7 negative control reproduces both defects; R8 passes both')
check(all(r['sha256']==old['SAN14_MeritTraits.dll'] for r in negative['results'][2:]),'negative control exercised the exact shipped R8 feature DLL')
check((R/'analysis/r8_user_evidence_review.md').exists() and (R/'references/proposals/R8_CommandRouting_추가수정지시서.txt').exists(),'R8 evidence and separate incremental instruction included')
result={'version':'RC4-R8-CommandRouting','scope':'Build/PE/package/PowerShell static checks, NOT Windows PowerShell or live game execution','passed':len(out),'checks':out,'executed_dll_assertions':total,'dll_sha256':old,'private_native_evidence_used':private.exists()}
(R/'audit/r8_package_tests.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print('COMPLETE',len(out),'static checks;',total,'executed-DLL assertions')
