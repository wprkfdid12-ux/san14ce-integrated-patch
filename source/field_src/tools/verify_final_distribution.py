#!/usr/bin/env python3
"""Verify installer payload identity, original import coverage and file transactions."""
import hashlib,json,re,struct,subprocess,tempfile
from pathlib import Path
from pe_probe import PE
from build_field_poc import exports
R=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((R/'analysis/final_r23_build.json').read_text())
for row in b['files']:assert sha(R/row['path'])==row['sha256']
for name,h in b['sources'].items():assert sha(R/name)==h,name
payload=(R/'native/installer_payload.h').read_text();exe=(R/'native/bin/FieldLeader3D_Setup.exe').read_bytes()
for symbol,name in [('fl_runtime','FieldLeader3D_Runtime.exe'),('fl_version','version.dll'),('fl_dinput','dinput8.dll')]:
 data=bytes(int(n) for n in re.search(r'\b'+symbol+r'\[\]=\{(.*?)\};',payload,re.S)[1].replace('\n','').split(',') if n.strip())
 assert data==(R/'native/bin'/name).read_bytes() and data in exe
p=PE(R/'inputs/SAN14CE_KO.exe');r,_=p.directories[1];o=p.rva_to_offset(r);matched={}
for j in range(100):
 lookup,_,_,name,iat=struct.unpack_from('<5I',p.data,o+j*20)
 if not name:break
 n=p.rva_to_offset(name);dll=p.data[n:p.data.index(0,n)].decode().lower()
 if dll not in ('version.dll','dinput8.dll'):continue
 available=exports(R/'native/bin'/dll);required=[]
 for k in range(100):
  v=struct.unpack_from('<Q',p.data,p.rva_to_offset(lookup or iat)+k*8)[0]
  if not v:break
  assert not v>>63
  n=p.rva_to_offset(v)+2;symbol=p.data[n:p.data.index(0,n)].decode();assert symbol in available;required.append(symbol)
 matched[dll]=required
assert len(matched)==2
with tempfile.TemporaryDirectory() as d:
 test=Path(d)/'installer';subprocess.run(['g++','-std=c++17','-O2',str(R/'native/tests/install_transaction_test.cpp'),'-o',str(test)],check=True)
 result=subprocess.run([str(test)],check=True,text=True,capture_output=True)
assert len(result.stdout.splitlines())==4
report={'status':'PASS_OFFLINE_NOT_WINDOWS_INSTALLED','build_sha256':sha(R/'analysis/final_r23_build.json'),'embedded_payloads':3,'original_imports_covered':matched,'transaction_checks':result.stdout.splitlines(),'source_sha256':{str(p.relative_to(R)):sha(p) for p in [R/'tools/verify_final_distribution.py',R/'native/tests/install_transaction_test.cpp',R/'native/install_transaction.h']},'windows_installer_executed':False,'other_user_patches_live_verified':False}
(R/'analysis/final_r23_distribution_verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
