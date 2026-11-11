#!/bin/bash
# FieldLeader3D 빌드 (clang+lld+mingw-w64 headers/CRT+libc++ 자가 툴체인). 사용: field_build.sh <native_dir> <out_dir>
set -e
N=$1; OUT=$2; mkdir -p $OUT
export PATH=/home/claude/tc/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64/bin:$PATH
CXX=x86_64-w64-mingw32-clang++
COMMON="-std=c++17 -O2 -Wall -Wextra -Werror -Wno-cast-function-type -Wno-misleading-indentation -Wno-unused-but-set-variable -Wno-unused-but-set-global -fno-exceptions -fno-rtti -static -Wl,--no-insert-timestamp -Wl,--disable-auto-image-base -s"
python3 - "$N" "$OUT" "$CXX" "$COMMON" <<'PY'
import sys,subprocess,struct,hashlib,ast,re,json,os
N,OUT,CXX,COMMON=sys.argv[1:5]; common=[CXX]+COMMON.split()
def exports(b):
    nt=struct.unpack_from('<I',b,60)[0];opt=nt+24;sh=opt+struct.unpack_from('<H',b,nt+20)[0]
    sec=[struct.unpack_from('<IIII',b,sh+i*40+8) for i in range(struct.unpack_from('<H',b,nt+6)[0])]
    def at(rva):
        for vs,va,rs,rp in sec:
            if va<=rva<va+rs:return rp+rva-va
        raise ValueError(rva)
    u32=lambda p:struct.unpack_from('<I',b,p)[0]
    p=at(u32(opt+112));count=u32(p+24);funcs,names,ordinals=struct.unpack_from('<III',b,p+28);out={}
    for i in range(count):
        q=at(u32(at(names)+4*i));name=b[q:b.index(0,q)].decode();o=struct.unpack_from('<H',b,at(ordinals)+2*i)[0];out[name]=u32(at(funcs)+4*o)
    return out
tool=open(os.path.join(N,'..','tools','build_field_poc.py')).read()
mapping=None
for node in ast.walk(ast.parse(tool)):
    if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='mapping' for t in node.targets): mapping=ast.literal_eval(node.value)
rep={}
for variant in ['normal','fog']:
    out=os.path.join(OUT,variant); os.makedirs(out,exist_ok=True)
    subprocess.run(common+['-shared','-DFL_FOG_VARIANT='+str(int(variant=='fog')),os.path.join(N,'field_poc.cpp'),'-o',os.path.join(out,'FieldLeader3D_PoC.dll')],check=True)
    dll=open(os.path.join(out,'FieldLeader3D_PoC.dll'),'rb').read(); ex=exports(dll); assert set(ex)==set(mapping.values()),set(mapping.values())-set(ex)
    open(os.path.join(N,'dll_build.h'),'w').write('#pragma once\nstatic const unsigned char fl_dll_sha256[32]={'+','.join('0x%02x'%v for v in hashlib.sha256(dll).digest())+'};\n'+'\n'.join('#define FL_EXPORT_%s 0x%x'%(k,ex[v]) for k,v in mapping.items())+'\n')
    open(os.path.join(N,'embedded_runtime.h'),'w').write('#pragma once\nstatic const unsigned char fl_embedded_dll[]={\n'+',\n'.join(','.join('0x%02x'%v for v in dll[i:i+32]) for i in range(0,len(dll),32))+'\n};\n')
    subprocess.run(common+[os.path.join(N,'poc_loader.cpp'),'-mwindows','-municode','-lbcrypt','-luser32','-lgdi32','-lshell32','-o',os.path.join(out,'FieldLeader3D_Runtime.exe')],check=True)
    rt=open(os.path.join(out,'FieldLeader3D_Runtime.exe'),'rb').read(); assert rt.count(dll)==1
    rep[variant]={'dll_sha256':hashlib.sha256(dll).hexdigest(),'runtime_sha256':hashlib.sha256(rt).hexdigest(),'dll_bytes':len(dll),'runtime_bytes':len(rt),'exports':ex}
    print('BUILT',variant,len(dll),len(rt),flush=True)
json.dump(rep,open(os.path.join(OUT,'build_report.json'),'w'),indent=1)
PY
