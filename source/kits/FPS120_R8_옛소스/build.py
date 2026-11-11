#!/usr/bin/env python3
"""Build the small Win64 DLL with local GNU C/binutils, without a Windows SDK.

Dependency-free build variant. Uses the local PE reader and GNU objdump.
Based on the supplied RC14 builder, with the same compiler/linker flags. The C compiler emits Microsoft-ABI
position-independent code; GNU ld emits PE32+. This script adds a normal import
descriptor and x64 unwind records for the verified, simple compiler prologues.
It refuses unexpected code-generation patterns instead of guessing unwind data.
"""
import hashlib, json, re, struct, subprocess, sys
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

HERE = Path(__file__).resolve().parent
MODULES={'fps120_duel60stock':'SAN14_FPS120.dll'}
module='fps120_duel60stock'
exports=['SAN14_ModInitV1','SAN14_FPS120InfoV1']
(HERE/(module+'.def')).write_text('LIBRARY '+MODULES[module]+'\nEXPORTS\n '+'\n '.join(exports)+'\n')
if module=='dxgi_proxy':
    (HERE/'dxgi_exports.h').write_text('#define S14_DXGI_COUNT '+str(len(exports))+'\nstatic const char dxgi_names[][48]={'+','.join('"'+x+'"' for x in exports)+'};\n')
def run(*args):
    return subprocess.check_output(args, cwd=HERE, text=True)
def align(n, size): return (n + size - 1) & -size
flags = ['-c', '-mabi=ms', '-maccumulate-outgoing-args', '-mno-red-zone',
    '-fshort-wchar', '-fno-stack-protector', '-fno-asynchronous-unwind-tables',
    '-fno-unwind-tables', '-ffreestanding', '-fno-builtin', '-fno-ident',
    '-fno-zero-initialized-in-bss', '-fpie', '-fno-tree-vectorize',
    '-fno-shrink-wrap', '-fno-schedule-insns2', '-fno-optimize-sibling-calls',
    '-fno-merge-constants', '-fcf-protection=none', '-O2', '-Wall', '-Wextra', '-Wno-unused-function']
run('gcc', *[f for f in flags if f!='-c'], '-S', module+'.c', '-o', module+'.s')
# GNU sometimes chooses SUB RSP,-128 for a shorter epilogue. Windows x64's
# unwinder recognizes ADD RSP,+size, not the arithmetically equivalent SUB.
assembly=(HERE/(module+'.s')).read_text()
assembly,normalized_epilogs=re.subn(r'subq\s+\$-(\d+), %rsp',r'addq\t$\1, %rsp',assembly)
if module=='dxgi_proxy':
    for idx,name in enumerate(exports):
        assembly+='\n.text\n.globl '+name+'\n.type '+name+', @function\n'+name+':\n'
        assembly+='subq $136, %rsp\n'
        for reg,off in [('rcx',32),('rdx',40),('r8',48),('r9',56)]:assembly+='movq %'+reg+', '+str(off)+'(%rsp)\n'
        for reg in range(4):assembly+='movdqu %xmm'+str(reg)+', '+str(64+16*reg)+'(%rsp)\n'
        assembly+='movl $'+str(idx)+', %ecx\ncall dxgi_resolve\n'
        for reg in range(4):assembly+='movdqu '+str(64+16*reg)+'(%rsp), %xmm'+str(reg)+'\n'
        for reg,off in [('rcx',32),('rdx',40),('r8',48),('r9',56)]:assembly+='movq '+str(off)+'(%rsp), %'+reg+'\n'
        assembly+='addq $136, %rsp\njmp *dxgi_forward+'+str(idx*8)+'(%rip)\n.size '+name+', .-'+name+'\n'
assembly += (HERE/'engine_step_bridge.S').read_text()
(HERE/(module+'.s')).write_text(assembly)
run('gcc','-c',module+'.s','-o',module+'.o')
relocs = run('readelf', '-rW', module+'.o')
assert all(t in ('R_X86_64_PC32', 'R_X86_64_PLT32') for t in re.findall(r'R_X86_64_\w+', relocs)), relocs
run('ld', '-m', 'i386pep', '--dll', '--subsystem', 'windows', '-e', 'DllMain',
    '--image-base', '0x180000000', '--dynamicbase', '--nxcompat',
    '--no-insert-timestamp', '--strip-all', '-Map', module+'.map',
    module+'.o', module+'.def', '-o', module+'_base.dll')
data = bytearray((HERE/(module+'_base.dll')).read_bytes())
p = pefile.PE(data=bytes(data))
text = next(s for s in p.sections if s.Name.rstrip(b'\0') == b'.text')
symbols = {}
for line in run('nm', '-n', '-S', module+'.o').splitlines():
    parts = line.split()
    if len(parts) == 4 and parts[2].lower() == 't':
        symbols[parts[3]] = (text.VirtualAddress + int(parts[0],16), int(parts[1],16))
assert set(exports+['DllMain']) <= set(symbols)
linkmap = (HERE/(module+'.map')).read_text()
md = Cs(CS_ARCH_X86,CS_MODE_64)
reg_number = {'rax':0,'rcx':1,'rdx':2,'rbx':3,'rsp':4,'rbp':5,'rsi':6,
    'rdi':7,'r8':8,'r9':9,'r10':10,'r11':11,'r12':12,'r13':13,'r14':14,'r15':15}
nonvolatile = {'rbx','rbp','rsi','rdi','r12','r13','r14','r15'}
unwinds=[];xmm_unwind_saves=[]
for name,(start,size) in sorted(symbols.items(),key=lambda x:x[1][0]):
    ins = list(md.disasm(p.get_data(start,size),start))
    assert ins and ins[-1].address+ins[-1].size == start+size
    operations=[];prolog_size=0;stack_alloc=0;pushed=[]
    for x in ins:
        end=x.address+x.size-start
        if x.mnemonic=='push':
            assert x.op_str in nonvolatile
            operations.append(bytes([end,reg_number[x.op_str]<<4]))
            pushed.append(x.op_str)
            prolog_size=end
        elif x.op_str.startswith('rsp, ') and (x.mnemonic=='sub' or (x.mnemonic=='add' and int(x.op_str.split(', ')[1],0)<0)):
            stack_alloc=abs(int(x.op_str.split(', ')[1],0))
            assert stack_alloc%8==0 and stack_alloc<4096
            if stack_alloc<=128:
                code=bytes([end,((stack_alloc//8-1)<<4)|2])
            else:
                code=bytes([end,1])+struct.pack('<H',stack_alloc//8)
            operations.append(code);prolog_size=end
            break
        elif x.mnemonic in ('ret','call','jmp','test','cmp'):
            break
        else:
            assert x.mnemonic in ('mov','lea','xor','nop','endbr64','add','sub','movzx','movsxd','and','or','shr','shl','inc','dec','movabs','movsx','not','neg','sbb','sar'),(name,x.mnemonic,x.op_str)
    # GCC saves nonvolatile XMM registers after the stack allocation. These
    # saves require UWOP_SAVE_XMM128 entries even though normal returns work
    # without them. Preserve machine code and describe its actual prologue.
    for x in ins:
        if x.mnemonic in ('call','ret','jmp') or x.mnemonic.startswith('j'):break
        m=re.fullmatch(r'xmmword ptr \[rsp(?:\+(0x[0-9a-f]+))?\],\s*xmm(\d+)',x.op_str.lower())
        if x.mnemonic in ('movaps','movups','movdqa','movdqu') and m and int(m[2])>=6:
            offset=int(m[1] or '0',16);reg=int(m[2]);end=x.address+x.size-start
            assert offset%16==0 and offset+16<=stack_alloc and end<256
            operations.append(bytes([end,(reg<<4)|8])+struct.pack('<H',offset//16))
            prolog_size=max(prolog_size,end);xmm_unwind_saves.append([name,reg,offset,end])
    if not operations:
        assert not any(x.mnemonic=='call' for x in ins), name
    else:
        if any(x.mnemonic=='call' for x in ins):
            assert stack_alloc and (stack_alloc+len(pushed)*8)%16==8,(name,stack_alloc,pushed)
        # Windows x64 requires a contiguous add-RSP, pop-registers, ret epilog.
        epilogs=0
        for j,x in enumerate(ins):
            if x.mnemonic=='add' and x.op_str.startswith('rsp, ') and int(x.op_str.split(', ')[1],0)>0:
                assert int(x.op_str.split(', ')[1],0)==stack_alloc
                following=ins[j+1:j+2+len(pushed)]
                expected=[('pop',v) for v in reversed(pushed)]+[('ret','')]
                actual=[(y.mnemonic,y.op_str) for y in following]
                if module=='dxgi_proxy' and name in exports:
                    assert not pushed and len(following)==1 and following[0].mnemonic=='jmp' and 'rip' in following[0].op_str
                else:assert actual==expected,(name,following)
                epilogs+=1
            elif not stack_alloc and x.mnemonic=='ret':
                assert [(y.mnemonic,y.op_str) for y in ins[j-len(pushed):j]] == [('pop',v) for v in reversed(pushed)],name
                epilogs+=1
        assert epilogs or name=='rel_original_init',(name,stack_alloc,[(x.mnemonic,x.op_str) for x in ins[-20:]])
    codes=b''.join(reversed(operations))
    info=bytes([1,prolog_size,len(codes)//2,0])+codes
    info+=b'\0'*((-len(info))%4)
    unwinds.append((name,start,start+size,info))

def add_section(name,body,characteristics):
    global data,p
    p=pefile.PE(data=bytes(data),fast_load=True)
    n=p.FILE_HEADER.NumberOfSections
    header=p.sections[-1].get_file_offset()+40
    assert header+40<=p.OPTIONAL_HEADER.SizeOfHeaders
    last=p.sections[-1]
    rva=align(last.VirtualAddress+max(last.Misc_VirtualSize,last.SizeOfRawData),p.OPTIONAL_HEADER.SectionAlignment)
    raw=align(len(data),p.OPTIONAL_HEADER.FileAlignment)
    raw_size=align(len(body),p.OPTIONAL_HEADER.FileAlignment)
    data.extend(b'\0'*(raw+raw_size-len(data)))
    data[raw:raw+len(body)]=body
    struct.pack_into('<8s6I2HI',data,header,name.encode().ljust(8,b'\0'),len(body),rva,raw_size,raw,0,0,0,0,characteristics)
    struct.pack_into('<H',data,p.FILE_HEADER.get_file_offset()+2,n+1)
    struct.pack_into('<I',data,p.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfImage'),align(rva+len(body),p.OPTIONAL_HEADER.SectionAlignment))
    return rva

last=p.sections[-1]
prva=align(last.VirtualAddress+max(last.Misc_VirtualSize,last.SizeOfRawData),p.OPTIONAL_HEADER.SectionAlignment)
pb=bytearray(len(unwinds)*12)
xrva=align(prva+len(pb),p.OPTIONAL_HEADER.SectionAlignment)
xb=bytearray()
for i,(name,start,end,info) in enumerate(unwinds):
    struct.pack_into('<III',pb,i*12,start,end,xrva+len(xb))
    xb.extend(info)
assert add_section('.pdata',pb,0x40000040)==prva
assert add_section('.xdata',xb,0x40000040)==xrva
rrva=add_section('.reloc',struct.pack('<IIHH',text.VirtualAddress,12,0,0),0x42000040)
p=pefile.PE(data=bytes(data))
directories={3:(prva,len(unwinds)*12),5:(rrva,12)}
for index,pair in directories.items():
    struct.pack_into('<II',data,p.OPTIONAL_HEADER.DATA_DIRECTORY[index].get_file_offset(),*pair)
struct.pack_into('<H',data,p.FILE_HEADER.get_field_absolute_offset('Characteristics'),p.FILE_HEADER.Characteristics&~1)
p=pefile.PE(data=bytes(data))
init_size=sum(s.SizeOfRawData for s in p.sections if s.Characteristics&0x40)
struct.pack_into('<I',data,p.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfInitializedData'),init_size)
p=pefile.PE(data=bytes(data))
struct.pack_into('<I',data,p.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'),p.generate_checksum())
output=HERE/MODULES[module];output.write_bytes(data)
p=pefile.PE(data=bytes(data))
assert p.verify_checksum()
assert len(getattr(p,"DIRECTORY_ENTRY_IMPORT",[]))==0
assert sorted(e.name.decode() for e in p.DIRECTORY_ENTRY_EXPORT.symbols)==sorted(exports)
metadata={'sha256':hashlib.sha256(data).hexdigest(),'functions':symbols,'unwind_entries':len(unwinds),'imports':[]}
(HERE/'build.json').write_text(json.dumps(metadata,indent=2))
print(json.dumps(metadata,indent=2))
