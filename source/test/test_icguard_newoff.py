import os
# R42d ICGuard 묶음 G: 10번 고유무장 등록 '신무장' — IC 가 게임 폴더\SteamData\editdataexceKO.s14 를 열 때 실제 스팀 저장 위치로 돌림(2D v25 와 같은 순서)
import sys,struct
sys.path.insert(0,os.environ.get('S14_AUDIT','/home/claude/p27/audit'))
from emu import *
from unicorn.x86_const import *
DLL='/home/claude/r41/build_out/SAN14_ICGuard.dll'
GP='C:\\Program Files (x86)\\Steam\\steamapps\\common\\Romance_of_the_Three_Kingdoms_14\\SteamData\\editdataexceKO.s14'
ST='C:/Program Files (x86)/Steam'
def boot(files,reg=None,active=0):
    fs={'SAN14_ICGuard.dll':open(DLL,'rb').read()};fs.update(files)
    w=Win(os.environ.get('S14_DEC','/home/claude/port/new.dec.exe'),fs,ini_text='',log=lambda s:None)
    meta={}
    old=w.api._fill_find
    def fill(h,buf):
        r=old(h,buf);f=w.handles[h];n=f['names'][f['i']-1];full=(f['base']+'/'+n) if f['base'] else n;d=w.fs_get(full)
        if d is not None:w.w32(buf+32,len(d));mt=meta.get(full.lower(),1000);w.w32(buf+20,mt&0xffffffff);w.w32(buf+24,mt>>32)
        return r
    w.api._fill_find=fill
    ol=w.api._loadlib
    w.api._loadlib=lambda n:KERNEL32_BASE if 'advapi32' in n.lower() else ol(n)
    def RegGetValueW():
        key=w.rstr(w.api.a(1),True);val=w.rstr(w.api.a(2),True);out=w.api.a(5);szp=w.api.a(6)
        if val=='ActiveUser':
            if not active:return 2
            w.w32(out,active);return 0
        v=(reg or {}).get(val)
        if v is None:return 2
        w.wstr(out,v,True);return 0
    w.api.RegGetValueW=RegGetValueW
    base=w.api._loadlib('SAN14_ICGuard.dll')
    if isinstance(base,str):
        rip=w.mu.reg_read(UC_X86_REG_RIP);w.run(rip,label='DllMain')
    ex=w.modules['san14_icguard.dll']['exports']
    root=w.alloc(0x400);w.wstr(root,'C:\\Game\\',True);w.run(ex['SAN14_ICGuardNewOff'],(5,root,0))
    return w,ex,meta
def resolve(w,ex,gp=GP):
    a=w.alloc(0x800);w.wstr(a,gp,True);o=w.alloc(0x800);w.w(o,b'\0'*0x800)
    r=w.run(ex['SAN14_ICGuardNewOff'],(2,a,o));st=w.run(ex['SAN14_ICGuardNewOff'],(1,0,0))
    return r&0xff,w.rstr(o,True),st
E=b'\x01'*0x45d
# ① 이 PC 모양: 게임 폴더엔 없음, 같은 스팀(게임 경로에서) 에 계정 1개
w,ex,m=boot({ST+'/userdata/75927797/872410/remote/editdataexceKO.s14':E})
r,p,st=resolve(w,ex);print('① 계정 1개:',r,p,st);assert r==1 and p.replace('\\','/').endswith('userdata/75927797/872410/remote/editdataexceKO.s14') and st==4
# ② 원래 경로에 파일 있으면 그대로(돌리지 않음)
w,ex,m=boot({'C:/Program Files (x86)/Steam/steamapps/common/Romance_of_the_Three_Kingdoms_14/SteamData/editdataexceKO.s14':E,ST+'/userdata/1/872410/remote/editdataexceKO.s14':E})
r,p,st=resolve(w,ex);print('② 원래 경로 있음:',r,st);assert r==0 and st==1
# ③ 계정 여러 개: 로그인 계정 우선(더 오래된 파일이어도)
w,ex,m=boot({ST+'/userdata/111/872410/remote/editdataexceKO.s14':E,ST+'/userdata/222/872410/remote/editdataexceKO.s14':E},active=111)
m[(ST+'/userdata/111/872410/remote/editdataexceKO.s14').lower()]=10;m[(ST+'/userdata/222/872410/remote/editdataexceKO.s14').lower()]=99
r,p,st=resolve(w,ex);print('③ 로그인 계정 우선:',p);assert r==1 and '/111/' in p.replace('\\','/')
# ④ 로그인 정보 없으면 최신
w,ex,m=boot({ST+'/userdata/111/872410/remote/editdataexceKO.s14':E,ST+'/userdata/222/872410/remote/editdataexceKO.s14':E})
m[(ST+'/userdata/111/872410/remote/editdataexceKO.s14').lower()]=10;m[(ST+'/userdata/222/872410/remote/editdataexceKO.s14').lower()]=99
r,p,st=resolve(w,ex);print('④ 최신:',p);assert r==1 and '/222/' in p.replace('\\','/')
# ⑤ 게임이 다른 드라이브 라이브러리(D:\SteamLibrary) → 레지스트리 SteamPath 의 userdata
GP2='D:\\SteamLibrary\\steamapps\\common\\Romance_of_the_Three_Kingdoms_14\\SteamData\\editdataexceKO.s14'
w,ex,m=boot({'C:/Steam/userdata/5/872410/remote/editdataexceKO.s14':E},reg={'SteamPath':'c:/steam'})
r,p,st=resolve(w,ex,GP2);print('⑤ 다른 드라이브 + SteamPath:',p);assert r==1 and p.replace('\\','/').lower().endswith('steam/userdata/5/872410/remote/editdataexceko.s14')
# ⑥ 다른 언어판 이름만 있음(editdataexceSC.s14)
w,ex,m=boot({ST+'/userdata/7/872410/remote/editdataexceSC.s14':E})
r,p,st=resolve(w,ex);print('⑥ 다른 언어판:',p);assert r==1 and p.endswith('editdataexceSC.s14')
# ⑦ 직접 지정(ICGuard.ini [NewOfficer] File=)
w,ex,m=boot({'SAN14Mods/modules/ICGuard.ini':'[NewOfficer]\r\nFile="E:\\my\\editdataexceKO.s14"\r\n'.encode(),'E:/my/editdataexceKO.s14':E,ST+'/userdata/7/872410/remote/editdataexceKO.s14':E})
r,p,st=resolve(w,ex);print('⑦ 직접 지정:',p,st);assert r==1 and p=='E:\\my\\editdataexceKO.s14' and st==2
# ⑧ 어디에도 없음 → 원래대로(0) + 상태 5
w,ex,m=boot({})
r,p,st=resolve(w,ex);print('⑧ 없음:',r,st);assert r==0 and st==5
# ⑨ 실제 열기: IC 가져오기 칸 바꾸기 → 그 경로로 열면 스팀 파일 내용이 읽힘, 쓰기 열기·다른 파일은 그대로
w,ex,m=boot({ST+'/userdata/75927797/872410/remote/editdataexceKO.s14':b'NEWOFF'+E})
ic=w.alloc(0x100000);w.wstr(ic+0xe91b8,'editdataexceKO.s14',True)
real=w.run(w.modules['san14_icguard.dll']['exports']['SAN14_ICGuardNewOff'],(4,0,0))
cfw=w.stub_for('kernel32.dll!CreateFileW');w.w64(ic+0x9a060,cfw)
core=w.alloc(0x40);ret1=w._tramp(b'\xb8\x01\x00\x00\x00\xc3');logs=[];lg=w._tramp(b'\xc3');w.override(lg,lambda:logs.append(w.rstr(w.arg(0))) or 0);w.w(core,bytes(0x40));w.w64(core+16,ret1);w.w64(core+48,lg)
ok=w.run(ex['SAN14_ICGuardNewOff'],(3,ic,core));print('⑨ 설치',ok&0xff,hex(w.r64(ic+0x9a060)))
assert ok&0xff==1 and w.r64(ic+0x9a060)==real
# IC 가 부르는 모양 그대로 새 칸으로 열기(읽기 전용, OPEN_EXISTING)
nm=w.alloc(0x400);w.wstr(nm,GP,True)
def cf(name,acc,disp):
    rsp=w.rsp() if hasattr(w,'rsp') else None
    return w.run(w.r64(ic+0x9a060),(name,acc,3,0),stack=[disp,0x80,0]) if False else None
# 스택 인자 다섯째~일곱째는 emu.run 이 4개까지만 받으므로 _tramp 로 부르는 작은 코드 사용
code=bytes.fromhex('4883ec48')+b'\x48\xc7\x44\x24\x20'+struct.pack('<I',3)+b'\x48\xc7\x44\x24\x28\x80\x00\x00\x00'+b'\x48\xc7\x44\x24\x30\x00\x00\x00\x00'+b'\x48\xb8'+struct.pack('<Q',w.r64(ic+0x9a060))+b'\xff\xd0'+bytes.fromhex('4883c448c3')
call=w._tramp(code)
h=w.run(call,(nm,0x80000000,1,0));print('   열기 손잡이',hex(h))
assert h not in (None,0xffffffffffffffff) and bytes(w.handles[h]['data'][:6])==b'NEWOFF'
st=w.run(ex['SAN14_ICGuardNewOff'],(0,0,0));print('   부름/찾음/못찾음',st&0xfffff,(st>>20)&0xfffff,st>>40);assert st&0xfffff==1 and (st>>20)&0xfffff==1
h2=w.run(call,(nm,0xc0000000,1,0));print('   쓰기 열기는 그대로(원래 경로 → 없음):',hex(h2));assert h2==0xffffffffffffffff
print('   기록:',[l for l in logs][:2])
# 다른 판(파일 이름 글자 다름)·이미 다른 모드가 바꾼 칸이면 안 바꿈
w.wstr(ic+0x9a060,'',True);w.w64(ic+0x9a060,0x12345678);ok=w.run(ex['SAN14_ICGuardNewOff'],(3,ic,core));assert ok&0xff==0
print('⑨ IC 가져오기 칸 1개만 바꿔 스팀 파일을 대신 엶 · 쓰기 열기는 그대로 · 칸이 이미 다르면 안 바꿈')
print('ALL OK')
