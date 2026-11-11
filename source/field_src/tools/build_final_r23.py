#!/usr/bin/env python3
"""Build reviewed native runtime, automatic loading proxies and one-time installer."""
import sys, subprocess, json, hashlib
from pathlib import Path
from build_field_poc import exports
ROOT=Path(__file__).resolve().parents[1]
ZIG=sys.argv[1]
N=ROOT/'native';B=N/'bin';G=N/'generated';G.mkdir(exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
subprocess.run([sys.executable,str(ROOT/'tools/build_field_poc.py'),'--zig',ZIG],check=True,stdout=subprocess.DEVNULL)
common=[ZIG,'c++','-target','x86_64-windows-gnu','-O2','-g','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-std=c++17']
version=['GetFileVersionInfoA','GetFileVersionInfoByHandle','GetFileVersionInfoExA','GetFileVersionInfoExW','GetFileVersionInfoSizeA','GetFileVersionInfoSizeExA','GetFileVersionInfoSizeExW','GetFileVersionInfoSizeW','GetFileVersionInfoW','VerFindFileA','VerFindFileW','VerInstallFileA','VerInstallFileW','VerLanguageNameA','VerLanguageNameW','VerQueryValueA','VerQueryValueW']
dinput=['DirectInput8Create','DllCanUnloadNow','DllGetClassObject','DllRegisterServer','DllUnregisterServer','GetdfDIJoystick']
for dll,names in [('version',version),('dinput8',dinput)]:
 folder=G/dll;folder.mkdir(exist_ok=True)
 (folder/'proxy_exports.h').write_text('#pragma once\n#define FL_PROXY_SYSTEM_NAME L"'+dll+'.dll"\n#define FL_PROXY_EXPORT_COUNT '+str(len(names))+'\nstatic const char *fl_proxy_export_names[]={'+','.join('"'+x+'"' for x in names)+'};\n')
 asm=['.text']
 for i,name in enumerate(names):
  entry='fl_forward_'+str(i)
  asm += [f'.globl {entry}',f'.def {entry}; .scl 2; .type 32; .endef',f'.seh_proc {entry}',entry+':','subq $168, %rsp','.seh_stackalloc 168','.seh_endprologue','movq %rcx, 32(%rsp)','movq %rdx, 40(%rsp)','movq %r8, 48(%rsp)','movq %r9, 56(%rsp)','movdqu %xmm0, 64(%rsp)','movdqu %xmm1, 80(%rsp)','movdqu %xmm2, 96(%rsp)','movdqu %xmm3, 112(%rsp)',f'movl ${i}, %ecx','call fl_proxy_address','movq %rax, %r11','movq 32(%rsp), %rcx','movq 40(%rsp), %rdx','movq 48(%rsp), %r8','movq 56(%rsp), %r9','movdqu 64(%rsp), %xmm0','movdqu 80(%rsp), %xmm1','movdqu 96(%rsp), %xmm2','movdqu 112(%rsp), %xmm3','addq $168, %rsp','jmp *%r11','.seh_endproc']
 (folder/'forward.S').write_text('\n'.join(asm)+'\n')
 (folder/'exports.def').write_text('LIBRARY '+dll+'\nEXPORTS\n'+'\n'.join(f' {name}=fl_forward_{i}' for i,name in enumerate(names))+'\n')
 subprocess.run(common+['-shared','-I',str(folder),str(N/'auto_proxy.cpp'),str(folder/'forward.S'),str(folder/'exports.def'),'-luser32','-o',str(B/(dll+'.dll'))],check=True)
 assert set(exports(B/(dll+'.dll')))==set(names)
def data(name,p):
 b=p.read_bytes();return 'static const unsigned char '+name+'[]={\n'+',\n'.join(','.join(str(v) for v in b[i:i+40]) for i in range(0,len(b),40))+'\n};\nstatic const unsigned char '+name+'_sha256[32]={'+','.join(str(v) for v in hashlib.sha256(b).digest())+'};\n'
(N/'installer_payload.h').write_text('#pragma once\n'+data('fl_runtime',B/'FieldLeader3D_Runtime.exe')+data('fl_version',B/'version.dll')+data('fl_dinput',B/'dinput8.dll')+'static const unsigned char fl_game_sha256[32]={'+','.join(str(v) for v in bytes.fromhex(sha(ROOT/'inputs/SAN14CE_KO.exe')))+'};\n')
subprocess.run(common+[str(N/'install_app.cpp'),'-Wl,--subsystem,windows','-municode','-lbcrypt','-luser32','-lgdi32','-lshell32','-lcomdlg32','-ladvapi32','-o',str(B/'FieldLeader3D_Setup.exe')],check=True)
files=[B/x for x in ['FieldLeader3D_PoC.dll','FieldLeader3D_Runtime.exe','version.dll','dinput8.dll','FieldLeader3D_Setup.exe']]
report={'revision':'R23','status':'BUILT_NOT_WINDOWS_EXECUTED','files':[{ 'path':str(p.relative_to(ROOT)),'bytes':p.stat().st_size,'sha256':sha(p)} for p in files],'sources':{str(p.relative_to(ROOT)):sha(p) for p in sorted(N.glob('*.cpp'))+sorted(N.glob('*.h'))},'windows_execution':False,'measured_game_fps':False}
(ROOT/'analysis/final_r23_build.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='sources'},indent=2))
