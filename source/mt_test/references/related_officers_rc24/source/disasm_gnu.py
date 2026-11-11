"""Instruction boundaries from local GNU objdump; used only by the unwind builder."""
import re,subprocess,tempfile,os
from types import SimpleNamespace
CS_ARCH_X86=0;CS_MODE_64=0
class Cs:
    def __init__(self,*args):pass
    def disasm(self,data,address):
        fd,name=tempfile.mkstemp(suffix='.bin')
        try:
            with os.fdopen(fd,'wb') as f:f.write(data)
            text=subprocess.check_output(['objdump','-D','-b','binary','-m','i386:x86-64','-Mintel','--insn-width=16','--adjust-vma='+str(address),name],text=True)
        finally:os.unlink(name)
        out=[]
        for line in text.splitlines():
            m=re.match(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s)+)\s*([^\t]+)$',line)
            if not m:continue
            binary=bytes.fromhex(m[2]);words=m[3].strip().split(None,1)
            if not words:continue
            mnemonic=words[0];operand=words[1] if len(words)>1 else ''
            # GNU prints multi-byte alignment NOP prefixes as mnemonics.
            # Normalize only an actual NOP, never a prefixed memory operation.
            if re.fullmatch(r'(?:(?:data16|cs|ds|ss|es|fs|gs|rex\.[A-Za-z]+)\s+)*nop(?:\s+.*)?',m[3].strip()):
                mnemonic='nop';operand=''
            operand=operand.split('#')[0].strip();operand=re.sub(r'\s*,\s*',', ',operand)
            if operand.startswith('rsp, 0x') and mnemonic in ('sub','add'):
                v=int(operand.split(', ')[1],16)
                if v>=1<<63:operand='rsp, '+str(v-(1<<64))
            out.append(SimpleNamespace(address=int(m[1],16),size=len(binary),mnemonic=mnemonic,op_str=operand))
        return out
