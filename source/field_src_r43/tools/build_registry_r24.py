#!/usr/bin/env python3
"""Build R24 installer with the new verified appearance runtime."""
import hashlib,json,subprocess,sys
from pathlib import Path
from build_field_poc import exports
R=Path(__file__).resolve().parents[1];N=R/'native';B=N/'bin';G=N/'generated'
ZIG=sys.argv[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
common=[ZIG,'c++','-target','x86_64-windows-gnu','-O2','-g','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-std=c++17']
specs=json.loads((R/'tools/proxy_specs.json').read_text())
for slot,spec in enumerate(specs):
 dll=spec['dll'];entries=spec['entries'];folder=G/dll;folder.mkdir(exist_ok=True)
 keys=','.join('"'+x['name']+'"' if 'name' in x else '(const char*)'+str(x['ordinal']) for x in entries)
 (folder/'proxy_exports.h').write_text(f'#pragma once\n#define FL_PROXY_SYSTEM_NAME L"{dll}.dll"\n#define FL_PROXY_SLOT {slot}\n#define FL_PROXY_EXPORT_COUNT {len(entries)}\nstatic const char *fl_proxy_export_keys[]={{'+keys+'};\n')
 asm=['.text'];definition=['LIBRARY '+dll,'EXPORTS']
 for i,entry in enumerate(entries):
  symbol='fl_forward_'+str(i)
  asm += [f'.globl {symbol}',f'.def {symbol}; .scl 2; .type 32; .endef',f'.seh_proc {symbol}',symbol+':','subq $168, %rsp','.seh_stackalloc 168','.seh_endprologue','movq %rcx, 32(%rsp)','movq %rdx, 40(%rsp)','movq %r8, 48(%rsp)','movq %r9, 56(%rsp)','movdqu %xmm0, 64(%rsp)','movdqu %xmm1, 80(%rsp)','movdqu %xmm2, 96(%rsp)','movdqu %xmm3, 112(%rsp)',f'movl ${i}, %ecx','call fl_proxy_address','movq %rax, %r11','movq 32(%rsp), %rcx','movq 40(%rsp), %rdx','movq 48(%rsp), %r8','movq 56(%rsp), %r9','movdqu 64(%rsp), %xmm0','movdqu 80(%rsp), %xmm1','movdqu 96(%rsp), %xmm2','movdqu 112(%rsp), %xmm3','addq $168, %rsp','jmp *%r11','.seh_endproc']
  definition.append(f' {entry["name"]}={symbol}' if 'name' in entry else f' ordinal_{entry["ordinal"]}={symbol} @{entry["ordinal"]} NONAME')
 (folder/'forward.S').write_text('\n'.join(asm)+'\n');(folder/'exports.def').write_text('\n'.join(definition)+'\n')
 subprocess.run(common+['-shared','-I',str(folder),str(N/'auto_proxy.cpp'),str(folder/'forward.S'),str(folder/'exports.def'),'-lbcrypt','-luser32','-o',str(B/(dll+'.dll'))],check=True)
 assert set(exports(B/(dll+'.dll')))=={x['name'] for x in entries if 'name' in x}
 print('Built',dll,flush=True)
def data(name,p):
 b=p.read_bytes();return 'static const unsigned char '+name+'[]={\n'+',\n'.join(','.join(str(v) for v in b[i:i+40]) for i in range(0,len(b),40))+'\n};\nstatic const unsigned char '+name+'_sha256[32]={'+','.join(str(v) for v in hashlib.sha256(b).digest())+'};\n'
payload='#pragma once\n'+data('fl_runtime',B/'FieldLeader3D_Runtime.exe')
for spec in specs:payload+=data('fl_'+spec['dll'],B/(spec['dll']+'.dll'))
payload+='static const unsigned char fl_game_sha256[32]={'+','.join(str(v) for v in bytes.fromhex(sha(R/'inputs/SAN14CE_KO.exe')))+'};\n'
payload+='struct FLProxyPayload { const unsigned char *bytes; size_t size; const unsigned char *sha256; };\nstatic const FLProxyPayload fl_proxy_payloads[]={\n'
payload+=',\n'.join('{fl_'+s['dll']+',sizeof fl_'+s['dll']+',fl_'+s['dll']+'_sha256}' for s in specs)+'\n};\nstatic_assert(sizeof(fl_proxy_payloads)/sizeof(fl_proxy_payloads[0])==fl_install::slot_count,"proxy slots");\n'
(N/'installer_payload.h').write_text(payload)
subprocess.run(common+[str(N/'install_app.cpp'),'-Wl,--subsystem,windows','-municode','-lbcrypt','-luser32','-lgdi32','-lshell32','-lcomdlg32','-ladvapi32','-o',str(B/'FieldLeader3D_Setup.exe')],check=True)
files=[B/x for x in ['FieldLeader3D_PoC.dll','FieldLeader3D_Runtime.exe']+[s['dll']+'.dll' for s in specs]+['FieldLeader3D_Setup.exe']]
report={'revision':'R24','status':'BUILT_NOT_WINDOWS_EXECUTED','runtime_reused_byte_identical_to_R23':False,'files':[{'path':str(p.relative_to(R)),'bytes':p.stat().st_size,'sha256':sha(p)} for p in files],'sources':{str(p.relative_to(R)):sha(p) for p in sorted(N.glob('*.cpp'))+sorted(N.glob('*.h'))},'proxy_specs':specs,'windows_execution':False}
(R/'analysis/registry_r24_build.json').write_text(json.dumps(report,indent=2)+'\n')
print('Built installer R24; runtime registry bridge included',flush=True)
