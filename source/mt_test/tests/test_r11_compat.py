"""Unaffected engine, original DB, save journal, Hover and RC25 contracts.
Historical R2-R5 UI/cancel and native-only pending-counter assertions are superseded by test_r6.py; do not run
those obsolete interaction assumptions as R6 release gates.
"""
from test_native import REPORT,ROOT
import importlib,json
from r6_fixture import R6
GROUPS={
 'test_rc4':['engine','persistence','logs'],
 'test_cp03':['engine_integrity','migration','save_atomicity','reentrant_guard_and_ai','initializer_atomicity','rolling_logs','log_number_encoding'],
 'test_native_integration':['identities','slot_roundtrip','interrupted','copies_and_corruption','flush_guard'],
 'test_r11_semantics':['semantic_file_damage_r11','live_targets_ai_r11'],
 'test_r2':['grades'],
 'test_r3':['engine_miss_passion','last_full_and_rerolls','information_labels','hover_patch_transaction'],
 'test_r4':['actual_modcore_contract','dll_local_transaction_guards','persistent_cache_failure','api_lifetime_guards','red_traits_in_information','preset_binding_and_display','original_rc25_trait_loop','load_order_and_failures']}
if __name__=='__main__':
 try:
  for module,functions in GROUPS.items():
   m=importlib.import_module(module)
   # Retain independent old contract assertions with the corrected native
   # selection fixture, never the obsolete status-0/selectable UI model.
   if module in ("test_r3","test_r4"):m.R3=R6
   for f in functions:print('RUN',module+'.'+f,flush=True);getattr(m,f)()
 finally:(ROOT/'audit/r11_compat_tests.json').write_text(json.dumps({'scope':__doc__,'functions':GROUPS,'passed':len(REPORT),'tests':REPORT},ensure_ascii=False,indent=2))
 print('PASS R11 compatibility',len(REPORT))
