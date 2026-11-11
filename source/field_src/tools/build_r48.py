#!/usr/bin/env python3
"""Build from the preserved guard contract; no original game binary is patched.
Usage: python build_r48.py /absolute/path/to/zig [output-directory]
"""
import ast,hashlib,json,re,struct,subprocess,sys,xml.etree.ElementTree as ET
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];N=ROOT/'native';PKG=ROOT.parents[2]
B=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else N/'bin';B.mkdir(parents=True,exist_ok=True)
ZIG=str(Path(sys.argv[1]).resolve())
common=[ZIG,'c++','-target','x86_64-windows-gnu','-O2','-g','-Wl,--strip-all','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti','-std=c++17']
def sha(b):return hashlib.sha256(b).hexdigest()
def exports(b):
 pe=struct.unpack_from('<I',b,0x3c)[0];opt=pe+24;sh=opt+struct.unpack_from('<H',b,pe+20)[0];sections=[]
 for i in range(struct.unpack_from('<H',b,pe+6)[0]):
  vs,va,size,raw=struct.unpack_from('<IIII',b,sh+i*40+8);sections.append((va,size,raw))
 def at(va):
  for v,n,r in sections:
   if v<=va<v+n:return r+va-v
  raise ValueError(va)
 u32=lambda p:struct.unpack_from('<I',b,p)[0]
 p=at(u32(opt+112));count=u32(p+24);funcs,names,ordinals=struct.unpack_from('<III',b,p+28);out={}
 for i in range(count):
  q=at(u32(at(names)+4*i));name=b[q:b.index(0,q)].decode();o=struct.unpack_from('<H',b,at(ordinals)+2*i)[0];out[name]=u32(at(funcs)+4*o)
 return out
subprocess.run(common+['-shared',str(N/'field_poc.cpp'),'-o',str(B/'FieldLeader3D_PoC.dll')],check=True)
dll=(B/'FieldLeader3D_PoC.dll').read_bytes();ex=exports(dll)
tree=ast.parse((ROOT/'tools/build_field_poc.py').read_text());mapping=None
for node in ast.walk(tree):
 if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='mapping' for t in node.targets):mapping=ast.literal_eval(node.value)
mapping.update(MOTION_EVALUATE='FL_HookMotionEvaluate',MOTION_COMMIT='FL_HookMotionCommit')
assert mapping and set(mapping.values())==set(ex)
(N/'dll_build.h').write_text('#pragma once\nstatic const unsigned char fl_dll_sha256[32]={'+','.join('0x%02x'%v for v in hashlib.sha256(dll).digest())+'};\n'+'\n'.join('#define FL_EXPORT_%s 0x%x'%(key,ex[name]) for key,name in mapping.items())+'\n')
(N/'embedded_runtime.h').write_text('#pragma once\nstatic const unsigned char fl_embedded_dll[]={\n'+',\n'.join(','.join('0x%02x'%v for v in dll[i:i+32]) for i in range(0,len(dll),32))+'\n};\n')
subprocess.run(common+[str(N/'poc_loader.cpp'),'-Wl,--subsystem,windows','-municode','-lbcrypt','-luser32','-lgdi32','-lshell32','-o',str(B/'FieldLeader3D_Runtime.exe')],check=True)
runtime=(B/'FieldLeader3D_Runtime.exe').read_bytes();assert runtime.count(dll)==1;offset=runtime.index(dll)
dest=PKG/'support/extras/13/FieldLeader3D_Runtime.exe';previous=sha(dest.read_bytes());dest.write_bytes(runtime)
p=PKG/'support/ExtraCatalog.xml';tree=ET.parse(p);item=tree.getroot().find("./Component[@id='13']/File");accepted=set(item.get('accepted','').split(','));accepted.update([previous,item.get('sha256','')]);accepted.discard('');item.set('accepted',','.join(sorted(accepted)));item.set('sha256',sha(runtime));ET.indent(tree,space='  ');p.write_bytes(b'\xef\xbb\xbf'+ET.tostring(tree.getroot(),encoding='utf-8',xml_declaration=True));catalog=sha(p.read_bytes())
p=PKG/'support/installer_r3_3/ExtraFeatures.cs';s=p.read_text('utf-8-sig');s=re.sub(r'const string CatalogHash="[a-f0-9]+"','const string CatalogHash="'+catalog+'"',s);p.write_text(s,encoding='utf-8-sig')
p=PKG/'support/installer_r3_3/FieldLoadCheck.cs';s=p.read_text('utf-8-sig');s=re.sub(r'DllHash="[a-f0-9]+"','DllHash="'+sha(dll)+'"',s);s=re.sub(r'RuntimeHash="[a-f0-9]+"','RuntimeHash="'+sha(runtime)+'"',s);s=re.sub(r'EmbeddedOffset=\d+, EmbeddedSize=\d+','EmbeddedOffset=%d, EmbeddedSize=%d'%(offset,len(dll)),s);s=re.sub(r'runtime.Length!=\d+','runtime.Length!='+str(len(runtime)),s);p.write_text(s,encoding='utf-8-sig')
report={'version':'R4.8','compiler':subprocess.check_output([ZIG,'version'],text=True).strip(),'dll_sha256':sha(dll),'runtime_sha256':sha(runtime),'dll_bytes':len(dll),'runtime_bytes':len(runtime),'embedded_offset':offset,'catalog_sha256':catalog,'exports':ex,'live_game_tested':False,'source_hashes':{f.name:sha(f.read_bytes()) for f in N.glob('*') if f.suffix in ('.cpp','.h') and f.name!='installer_payload.h'}}
(B/'build_report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('exports','source_hashes')},indent=2))
