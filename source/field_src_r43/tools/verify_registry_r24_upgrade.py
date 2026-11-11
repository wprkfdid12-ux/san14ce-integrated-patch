#!/usr/bin/env python3
import hashlib,json,os,subprocess,tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[1]
checks=[]
with tempfile.TemporaryDirectory() as d:
 for source,libs in [('install_upgrade_test.cpp',[]),('install_upgrade_disk_test.cpp',['-lcrypto'])]:
  out=Path(d)/source
  subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',str(R/'native/tests'/source),*libs,'-o',str(out)],check=True)
  p=subprocess.run([str(out)],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','UBSAN_OPTIONS':'halt_on_error=1'})
  assert p.returncode==0 and not p.stderr,p.stdout+p.stderr
  checks.append(p.stdout.strip())
files=[R/'native/install_upgrade.h',R/'native/install_upgrade_disk.h',R/'native/install_app.cpp',R/'native/tests/install_upgrade_test.cpp',R/'native/tests/install_upgrade_disk_test.cpp',Path(__file__)]
report={'status':'PASS_POSIX_BACKEND_NOT_WINDOWS_EXECUTED','abstract_cases':36,'real_filesystem_backend_cases':58,'checks':checks,'sanitizers':['ASan','UBSan'],'scope':'production upgrade header, SHA256 and real POSIX files with host Win32 primitive adapters; completed-operation interruption boundaries','source_sha256':{str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
(R/'analysis/registry_r24_upgrade_verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
