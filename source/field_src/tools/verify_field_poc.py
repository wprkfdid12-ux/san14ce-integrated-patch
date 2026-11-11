#!/usr/bin/env python3
"""Validate native build/ABI and mocked lifecycle; never claim a game test."""
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zipfile
from build_field_poc import exports, PATCHES

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    root=Path(__file__).resolve().parents[1]; native=root/'native'
    build=json.loads((root/'analysis/poc_native_build.json').read_text())
    checks=[]
    def passed(name): checks.append({'name':name,'status':'PASS'})
    for row in build['artifacts']:
        p=root/row['path']; assert sha(p)==row['sha256'] and p.stat().st_size==row['bytes']
    for rel,value in build['source_files'].items(): assert sha(root/rel)==value,rel
    passed('Windows executables and native sources match the recorded build')
    dll=native/'bin/FieldLeader3D_PoC.dll'; exe=native/'bin/FieldLeader3D_Runtime.exe'
    ex=exports(dll); assert ex==build['dll_exports']
    header=(native/'dll_build.h').read_text()
    expected={'REGISTRY':'FL_Registry','CONFIGURE':'FL_Configure','STATUS':'FL_Status','STATUS_SEQUENCE':'FL_StatusSequence','ROSTER_AUDIT':'FL_RosterAudit','STOP':'FL_RequestStop','UPDATE':'FL_HookUpdate',
              'DRAW':'FL_HookDraw','MANAGER_RELEASE':'FL_HookManagerRelease','RESOURCE_RELEASE':'FL_HookResourceRelease',
              'FIELD_GROUP_DRAW':'FL_HookFieldGroupDraw','FIELD_ACTION_DRAW':'FL_HookFieldActionDraw',
              'EVENT_SOUND_2D':'FL_HookEventSound2D','EVENT_SOUND_3D':'FL_HookEventSound3D',
              'TACTIC_EFFECT':'FL_HookTacticEffect','TACTIC_DAMAGE':'FL_HookTacticDamage',
              'AMBIENT_ENQUEUE':'FL_HookAmbientEnqueue','AMBIENT_CLEANUP':'FL_HookAmbientCleanup',
              'NORMAL_DAMAGE':'FL_HookNormalDamage','MOTION_EFFECT':'FL_HookMotionEffect','TACTIC_CHILD':'FL_HookTacticChild',
              'AMBIENT_START':'FL_HookAmbientStart','AMBIENT_VOLUME':'FL_HookAmbientVolume','ARMY_UPDATE':'FL_HookArmyUpdate',
              'GROUND_EFFECT':'FL_HookGroundEffect','NORMAL_FIELD_EFFECT':'FL_HookNormalFieldEffect',
              'DEFEAT_COMMIT':'FL_HookDefeatCommit','ARMY_DETACH':'FL_HookArmyDetach','INFO_ANCHOR':'FL_HookInfoAnchor','INFO_DESTROY':'FL_HookInfoDestroy'}
    expected['ANIMATION_RATE']='FL_HookAnimationRate'
    for key,name in expected.items():
        assert int(re.search(r'#define FL_EXPORT_'+key+r' (0x[0-9a-f]+)',header)[1],16)==ex[name]
    passed('Loader export RVAs and DLL hash are bound to the same compiled DLL')
    contract=(native/'poc_contract.h').read_text()
    constants={n:int(v,0) for n,v in re.findall(r'\b(FL_[A-Z0-9_]+)\s*=\s*(0x[0-9a-f]+|\d+)',contract)}
    specs=contract.split('fl_patch_specs[] = {',1)[1].split('};',1)[0]
    actual=[(int(site,16),constants[target]) for site,target in re.findall(r'\{(0x[0-9a-f]+),\s*(FL_[A-Z0-9_]+)\}',specs)]
    assert actual==PATCHES and len(actual)==35
    guards=json.loads((root/'analysis/poc_native_contract.json').read_text())
    guard_bytes={int(row['rva'],16):bytes.fromhex(row['bytes_hex']) for row in guards['entry_guards']}
    for site,target in actual:
        if site in (constants['FL_ARMY_UPDATE_VTABLE'],constants['FL_ARMY_DETACH_VTABLE'],constants['FL_INFO_ANCHOR_VTABLE'],constants['FL_INFO_VTABLE']):
            assert {'rva':hex(site),'target':hex(target)} in guards['vtable_guards']
            continue
        code=guard_bytes[site]
        assert len(code)==5 and code[0]==0xe8 and site+5+struct.unpack_from('<i',code,1)[0]==target
    covered=set()
    for row in guards['entry_guards']:
        rva,code=int(row['rva'],16),bytes.fromhex(row['bytes_hex'])
        if 0x1c66a0<=rva<0x1c67aa: covered.update(range(rva,rva+len(code)))
    assert set(range(0x1c66a0,0x1c67aa))<=covered
    assert constants['FL_REQUEST_COLOR']==0x1c8000
    color_guard=guard_bytes[0x1c8000]+guard_bytes[0x1c803d]
    assert len(color_guard)==88 and color_guard[0x52:0x57]==bytes.fromhex('e9 f9 14 1b 00')
    assert guard_bytes[0x379550]==bytes.fromhex('89 51 24 44 89 41 2c c7 41 28 01 00 00 00 c3')
    assert constants['FL_PROTOCOL']==0x464c0003 and constants['FL_PROXY_COUNT']==500
    for start,end in ((0xe4ffdc,0xe50067),(0xe502e4,0xe503a8),(0x1c9640,0x1c96be),
                      (0x171207,0x171219),(0x171324,0x17133a),(0x400555,0x400571),
                      (0x400581,0x40059d),(0x40066b,0x400679),(0x405be0,0x405c74),
                      (0x179b2b,0x179b4a),(0x36cc40,0x36cc49),(0x113f5,0x11477),
                      (0x169b57,0x169b91),(0xe4cc6c,0xe4cd30),(0x171222,0x171234),(0x405c74,0x405cdd),(0x3a2670,0x3a2688),
                      (0x1cb2b0,0x1cb2bf),(0x1c1a4c,0x1c1a77),(0x404990,0x404a05)):
        coverage=set()
        for rva,code in guard_bytes.items():coverage.update(range(rva,rva+len(code)))
        assert set(range(start,end))<=coverage
    loader=(native/'poc_loader.cpp').read_text()
    patch_exports=loader.split('const uint32_t exports[]={',1)[1].split('};',1)[0]
    names=re.findall(r'FL_EXPORT_([A-Z0-9_]+)',patch_exports)
    assert len(names)==35 and names[13:15]==['EVENT_SOUND_2D','EVENT_SOUND_3D']
    assert names[15:]==['TACTIC_EFFECT','TACTIC_DAMAGE','AMBIENT_ENQUEUE','AMBIENT_ENQUEUE','AMBIENT_ENQUEUE','AMBIENT_CLEANUP','NORMAL_DAMAGE','MOTION_EFFECT','TACTIC_CHILD','AMBIENT_START','AMBIENT_VOLUME','ARMY_UPDATE','GROUND_EFFECT','NORMAL_FIELD_EFFECT','DEFEAT_COMMIT','ARMY_DETACH','INFO_ANCHOR','INFO_DESTROY','ANIMATION_RATE','ANIMATION_RATE']
    passed('All 31 E8 calls and four relocated presentation/lifetime vtable pointers match guarded targets and paired exports')
    pe_results=[]
    for p in (dll,exe):
        b=p.read_bytes(); nt=struct.unpack_from('<I',b,0x3c)[0]; opt=nt+24
        assert b[nt:nt+4]==b'PE\0\0' and struct.unpack_from('<H',b,nt+4)[0]==0x8664
        assert struct.unpack_from('<H',b,opt)[0]==0x20b
        assert struct.unpack_from('<H',b,opt+70)[0]&0x140==0x140
        assert all(struct.unpack_from('<II',b,opt+112+i*8)[1]>0 for i in (3,5))
        dump=subprocess.check_output(['objdump','-p',str(p)],text=True)
        imports=re.findall(r'DLL Name: (\S+)',dump)
        assert imports and all(n.lower() in ('kernel32.dll','bcrypt.dll','user32.dll','gdi32.dll','shell32.dll') or n.lower().startswith('api-ms-win-crt-') for n in imports)
        if p==exe: assert struct.unpack_from('<H',b,opt+68)[0]==2,'Launcher must use GUI subsystem, not a second console'
        pe_results.append({'file':p.name,'machine':'AMD64','unwind_directory':True,'relocations':True,
                           'dynamic_base':True,'nx_compatible':True,'imports':imports})
    passed('Both PE64 files contain x64 unwind data, ASLR relocations, and only Windows/CRT imports')
    with tempfile.TemporaryDirectory(prefix='field_poc_verify_') as scratch:
        scratch=Path(scratch); test=scratch/'lifecycle'
        command=['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                 '-pthread','-I',str(native/'tests'),str(native/'tests/poc_lifecycle_test.cpp'),'-o',str(test)]
        subprocess.run(command,check=True)
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
        result=subprocess.run([str(test)],capture_output=True,text=True,env=env)
        if result.returncode: print(result.stdout, result.stderr); result.check_returncode()
        assert not result.stderr,result.stderr
        scenarios=[s[5:] for s in result.stdout.splitlines() if s.startswith('PASS ')]
        assert len(scenarios)==177
        for name in scenarios: passed('Mock engine: '+name)
        snapshot=scratch/'snapshot'
        subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined',
                        str(native/'tests/status_snapshot_test.cpp'),'-o',str(snapshot)],check=True)
        result=subprocess.run([str(snapshot)],check=True,capture_output=True,text=True,env=env)
        assert not result.stderr,result.stderr
        snapshot_scenarios=[s[5:] for s in result.stdout.splitlines() if s.startswith('PASS ')]
        assert len(snapshot_scenarios)==7
        for name in snapshot_scenarios: passed('Coherent status: '+name)
        bounds=scratch/'bounds'
        subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined',
                        str(native/'tests/capture_bounds_test.cpp'),'-o',str(bounds)],check=True)
        result=subprocess.run([str(bounds)],check=True,capture_output=True,text=True,env=env)
        assert not result.stderr,result.stderr
        bounds_scenarios=[s[5:] for s in result.stdout.splitlines() if s.startswith('PASS ')]
        assert len(bounds_scenarios)==5
        for name in bounds_scenarios: passed('Capture bounds: '+name)
        zipper=scratch/'zipper'; archive=scratch/'diagnostic.zip'
        subprocess.run(['g++','-std=c++17','-O2',str(native/'tests/zip_store_test.cpp'),'-o',str(zipper)],check=True)
        subprocess.run([str(zipper),str(archive)],check=True)
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            dynamic=[f'Slot_{i//3}_{("body","horse","weapon")[i%3]}.bin' for i in range(12)]
            assert z.namelist()==['diagnostic.bin','empty.log']+dynamic
            assert z.read('diagnostic.bin')==bytes([0,1,2,255,10]) and z.read('empty.log')==b''
            for name in dynamic:assert z.read(name)==bytes([0,1,2,255,10])
        passed('Diagnostic ZIP writer interoperates with Python zipfile, including CRC and empty logs')
        probe=scratch/'probe'
        subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                        str(native/'tests/remote_probe_test.cpp'),'-o',str(probe)],check=True)
        result=subprocess.run([str(probe)],check=True,capture_output=True,text=True,env=env)
        assert not result.stderr,result.stderr
        probe_scenarios=[s[5:] for s in result.stdout.splitlines() if s.startswith('PASS ')]
        assert len(probe_scenarios)==5
        for name in probe_scenarios: passed('Read-only branch diagnostics: '+name)
    if build.get('loader_only_revision'):
        old=json.loads((root/'history/first_field_r9_checkpoint/analysis/poc_native_build.json').read_text())
        old_sources=old['source_files']
        for name in ('native/field_poc.cpp','native/poc_contract.h','native/game_guards.h','native/dll_build.h'):
            assert sha(root/name)==old_sources[name],name
        assert sha(dll)=='fe6ec60eba22607007f4ceee1a1dd96f6381762812d6c1de80416ffb87dea4af'
        passed('R10 preserves the exact R9 engine DLL, engine sources, guards and export bindings')
    embedded=(native/'embedded_runtime.h').read_text()
    embedded_bytes=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',embedded))
    assert embedded_bytes==dll.read_bytes()
    assert embedded_bytes in exe.read_bytes()
    passed('The standalone GUI executable embeds the exact paired runtime DLL')
    appearance=json.loads((root/'analysis/appearance_validation_r16/report.json').read_text())
    assert appearance['status']=='PASS_OFFLINE_NOT_PIXEL_VERIFIED' and appearance['native_bodies_reached']==list(range(25))
    assert appearance['native_id_domain_cases']==12002 and appearance['native_color_queue_cases']==244
    for rel,value in appearance['source_sha256'].items():assert sha(root/rel)==value,rel
    passed('All original body resources and native selector/color byte tests match the preserved inputs')
    motion=json.loads((root/'analysis/mounted_motion_r17.json').read_text())
    assert motion['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED' and motion['body_action_bindings']==50
    assert motion['horse_action_bindings']==2 and motion['native_sound_abi_cases']==4
    for rel,value in motion['source_sha256'].items(): assert sha(root/rel)==value,rel
    passed('All 25 original body run/idle clips, horse clips, native motion writes and nine-argument sound ABIs match preserved inputs')
    batch=json.loads((root/'analysis/batch1_r18.json').read_text())
    assert batch['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED' and batch['body_action_bindings']==75
    assert len(batch['native_checks'])==9 and batch['native_view_cases']==6
    for rel,value in batch['source_sha256'].items():assert sha(root/rel)==value,rel
    passed('All 25 original attack/strong/hit clips, native tactic execution/damage/recovery, cleanup and viewport byte tests match preserved inputs')
    combat=json.loads((root/'analysis/combat_r20_evidence.json').read_text())
    assert combat['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED' and combat['body_action_bindings']==100
    for rel,value in combat['source_sha256'].items(): assert sha(root/rel)==value,rel
    passed('R20 original native damage/effect/stat/state/sound-layout byte proofs and all 100 motion bindings match the preserved inputs')
    r21=json.loads((root/'analysis/combat_r21_evidence.json').read_text())
    assert r21['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED' and r21['body_action_hand_bindings']==150
    assert len(r21['checks'])==7 and r21['horse_dust_events']==20
    for rel,value in r21['source_sha256'].items(): assert sha(root/rel)==value,rel
    passed('R21 original hand selection, 150 resource bindings, hoof-dust IDs, field vtable ABI and native volume setter agree with preserved inputs')
    batch2=json.loads((root/'analysis/batch2_r22_evidence.json').read_text())
    assert batch2['status']=='PASS_OFFLINE_NOT_GAME_VERIFIED' and batch2['fall_binding_count']==50
    assert len(batch2['checks'])==9
    for rel,value in batch2['source_sha256'].items(): assert sha(root/rel)==value,rel
    passed('R22 capture result, 50 native fall clips, particle ABI, whole-layout anchor and final attachment dispatch match preserved original bytes')
    report={'schema':'fieldleader3d.native_verification.v1' ,'status':'PASS','checks':checks,
            'build_report_sha256':sha(root/'analysis/poc_native_build.json'),
            'appearance_report_sha256':sha(root/'analysis/appearance_validation_r16/report.json'),
            'motion_report_sha256':sha(root/'analysis/mounted_motion_r17.json'),
            'batch1_report_sha256':sha(root/'analysis/batch1_r18.json'),
            'combat_r20_report_sha256':sha(root/'analysis/combat_r20_evidence.json'),
            'combat_r21_report_sha256':sha(root/'analysis/combat_r21_evidence.json'),
            'batch2_r22_report_sha256':sha(root/'analysis/batch2_r22_evidence.json'),
            'mock_engine_scenarios':177,'appearance_integration_combinations':1464,'coherent_snapshot_scenarios':7,
            'verifier_sha256':sha(__file__),
            'tested_source_hashes':{str(p.relative_to(root)):sha(p) for p in
                [native/'registry_bridge.h',native/'tests/registry_tests.h',native/'final_motion.h',native/'final_ui.h',native/'automatic_runtime.h',native/'tests/final_mocks.h',native/'tests/final_tests.h',native/'field_poc.cpp',native/'poc_loader.cpp',native/'poc_contract.h',native/'zip_store.h',
                 native/'batch2_presentation.h',native/'tests/batch2_mocks.h',native/'tests/batch2_tests.h',native/'combat_audio.h',native/'combat_r20.h',native/'combat_ambient.h',
                 native/'tests/combat_r21_tests.h',native/'tests/combat_r21_mocks.h',native/'tests/combat_r20_tests.h',native/'tests/combat_r20_mocks.h',native/'presentation_perf.h',native/'tests/batch1_tests.h',native/'tests/combat_r19_tests.h',
                 native/'tests/poc_lifecycle_test.cpp',native/'tests/windows.h',native/'tests/zip_store_test.cpp',
                 native/'remote_probe.h',native/'tests/remote_probe_test.cpp',native/'launcher_ui.h',native/'embedded_runtime.h',
                 native/'capture_bounds.h',native/'tests/capture_bounds_test.cpp',
                 native/'status_snapshot.h',native/'tests/status_snapshot_test.cpp']},
            'pe_validation':pe_results,'host':'Linux',
            'sanitizers':['AddressSanitizer','UndefinedBehaviorSanitizer'],
            'leak_sanitizer':'UNAVAILABLE_UNDER_RUNTIME_PTRACE; detect_leaks=0; release counts separately asserted',
            'windows_loader_execution':False,'original_engine_execution':False,'game_screenshot_verified':False}
    (root/'analysis/poc_native_verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':'PASS','checks':len(checks),'mock_engine_scenarios':177,
                      'read_only_probe_scenarios':5,'windows_game_test':'NOT_RUN'}))

if __name__=='__main__':main()
