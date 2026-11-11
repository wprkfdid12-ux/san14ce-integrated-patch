#!/usr/bin/env python3
"""Run the original complete 37a720 wrapper with controlled engine resources.

Unicorn executes original bytes for argument setup, floating operations,
rounding, the false-reset branch, frame write and return. Resource lookup,
render evaluation and unrelated engine helpers are stubs. At the new commit
site the revised case calls compiled production frame_step_core.h via ctypes.
This is native-instruction regression coverage, not Windows/GPU execution.
Usage: python verify_r48_native.py INPUT_DIRECTORY OUTPUT_DIRECTORY
Requires g++, unicorn; original captured .text is not distributed here.
"""
import ctypes,hashlib,json,math,struct,subprocess,sys
from pathlib import Path
from pe_probe import PE
from unicorn import Uc,UC_ARCH_X86,UC_MODE_64,UC_HOOK_CODE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
INPUT=Path(sys.argv[1]).resolve();OUT=Path(sys.argv[2]).resolve();OUT.mkdir(parents=True,exist_ok=True)
source=OUT/'frame_bridge.cpp'
source.write_text('#include "frame_step_core.h"\nextern "C" void step(float p,float e,float r,float c,float *out){auto v=FrameStep::advance(p,e,r,c);out[0]=v.next;out[1]=v.period;out[2]=v.valid;out[3]=v.wrapped;}\n')
subprocess.run(['g++','-std=c++17','-O2','-shared','-fPIC','-I',str(ROOT/'native'),str(source),'-o',str(OUT/'frame_bridge.so')],check=True)
library=ctypes.CDLL(str(OUT/'frame_bridge.so'));library.step.argtypes=[ctypes.c_float]*4+[ctypes.POINTER(ctypes.c_float)]
pe=PE(INPUT/'SAN14CE_KO.exe');pe.attach_runtime_text(INPUT/'SAN14CE_KO.runtime_text.bin',INPUT/'runtime_manifest.json')
BASE=pe.image_base;MEM=0x70000000;STACK=MEM+0xeff8;STOP=MEM+0xf800
WRAPPER=MEM+0x100;HANDLE=MEM+0x200;SETTING=MEM+0x300;CHILD=MEM+0x400;MODEL=MEM+0x500;OVERRIDE=MEM+0x1000
ENGINE=MEM+0x2000;FRAME=MEM+0x2100;CONTEXT=MEM+0x2200
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def unbits(x):return struct.unpack('<f',struct.pack('<I',x&0xffffffff))[0]
class Native:
    def __init__(self,fixed):
        self.fixed=fixed;self.uc=Uc(UC_ARCH_X86,UC_MODE_64);u=self.uc
        u.mem_map(BASE,(pe.image_size+4095)&~4095);u.mem_map(MEM,0x10000)
        for s in pe.sections:u.mem_write(BASE+s['rva'],pe.data[s['raw']:s['raw']+s['size']])
        self.put(WRAPPER+0x10,'Q',HANDLE);self.put(WRAPPER+0x18,'I',1);self.put(WRAPPER+8,'I',1)
        self.put(HANDLE+0x20,'Q',SETTING);self.put(SETTING+0x30,'Q',CHILD);self.put(CHILD+0x20,'Q',MODEL)
        self.put(BASE+0x27d1208,'Q',CONTEXT)
        self.stubs={0xc07380,0xbcc5b0,0xd7f830,0xd7f7e0,0x10e90,0x934940,0xd9cd00,0xd9dbf0,0xdbea30,0xff9470}
        for rva in self.stubs|{0x37a97c,0x37a9a1}:
            u.hook_add(UC_HOOK_CODE,self.hook,begin=BASE+rva,end=BASE+rva)
    def put(self,a,fmt,value):self.uc.mem_write(a,struct.pack('<'+fmt,value))
    def get(self,a,fmt):return struct.unpack('<'+fmt,self.uc.mem_read(a,struct.calcsize(fmt)))[0]
    def hook(self,u,a,size,user):
        rva=a-BASE
        if rva==0x37a97c:self.native_reset+=1;return
        if rva==0x37a9a1:
            assert u.reg_read(UC_X86_REG_RCX)==SETTING
            self.native_next=unbits(u.reg_read(UC_X86_REG_XMM1))
            if self.fixed:
                result=(ctypes.c_float*4)();library.step(self.get(SETTING+0x24,'f'),self.evaluated,self.rate,self.clock,result)
                assert result[2]==1;self.true_wraps+=int(result[3]);self.period=result[1]
                u.reg_write(UC_X86_REG_XMM1,bits(result[0]))
            return
        sp=u.reg_read(UC_X86_REG_RSP)
        if rva==0xc07380:u.reg_write(UC_X86_REG_RAX,CHILD)
        elif rva==0xbcc5b0:u.reg_write(UC_X86_REG_RAX,55)
        elif rva==0x934940:u.reg_write(UC_X86_REG_XMM0,bits(self.clock))
        elif rva==0x10e90:u.reg_write(UC_X86_REG_RAX,CONTEXT)
        elif rva==0xd9dbf0:
            assert u.reg_read(UC_X86_REG_RCX)==MODEL and u.reg_read(UC_X86_REG_RDX)==FRAME
            assert u.reg_read(UC_X86_REG_R8)==ENGINE and u.reg_read(UC_X86_REG_R9)==55
            time=self.get(sp+0x28,'f');assert time==f32(self.get(SETTING+0x24,'f')/self.rate)
            assert MEM<self.get(sp+0x30,'Q')<STACK # native options argument
            current=max(0,time*self.rate)
            evaluated=min(current,self.period_limit) if self.clamp else current%self.period_limit
            self.evaluated=f32(evaluated/self.rate)
            self.put(self.get(sp+0x38,'Q'),'f',self.evaluated)
            self.evaluations+=1;u.reg_write(UC_X86_REG_RAX,1)
        u.reg_write(UC_X86_REG_RIP,self.get(sp,'Q'));u.reg_write(UC_X86_REG_RSP,sp+8)
    def step(self,current,hz,scale=1,period=120,rate=60,clamp=False,override=0):
        self.clock=f32(hz/scale);self.period_limit=period;self.rate=rate;self.clamp=clamp
        self.native_reset=self.true_wraps=self.evaluations=0;self.period=0
        self.put(SETTING+0x24,'f',current);self.put(SETTING+0x28,'f',rate);self.put(OVERRIDE,'f',override)
        self.put(STACK,'Q',STOP);self.put(STACK+0x28,'I',55)
        for reg,value in [(UC_X86_REG_RSP,STACK),(UC_X86_REG_RCX,WRAPPER),(UC_X86_REG_RDX,ENGINE),(UC_X86_REG_R8,FRAME),(UC_X86_REG_R9,OVERRIDE)]:self.uc.reg_write(reg,value)
        self.uc.emu_start(BASE+0x37a720,STOP,count=4000)
        assert self.uc.reg_read(UC_X86_REG_RIP)==STOP and self.uc.reg_read(UC_X86_REG_RAX)==1 and self.evaluations==1
        return self.get(SETTING+0x24,'f')

old=Native(False);new=Native(True);singles=[]
for current,hz,scale,period in [(10.25,60,1,120),(1,120,.75,120),(1.375,120,.75,120),(120.25,120,.75,120),(151.75,90,1,151)]:
    a=old.step(current,hz,scale,period);b=new.step(current,hz,scale,period)
    expected=current%period+60/(hz/scale);assert abs(b-expected)<.00002
    singles.append(dict(current=current,hz=hz,scale=scale,period=period,original=a,revised=b,native_reset_branch=old.native_reset,revised_true_wraps=new.true_wraps))
assert singles[0]['original']==0 and singles[0]['revised']==11.25
assert singles[2]['original']==-.625 and singles[2]['revised']==1.75
sequences=[]
for label,hzs,scale in [('move120',[120]*12,.75),('idle_variable',[60,61,59,60]*3,1)]:
    a=b=1.;oa=[a];nb=[b]
    for hz in hzs:a=old.step(a,hz,scale);b=new.step(b,hz,scale);oa.append(a);nb.append(b)
    sequences.append(dict(case=label,original=oa,revised=nb))
phases=[]
for i in range(20):
    h=((i+1)*0x9e3779b9)&0xffffffff;h^=h>>16;h=(h*0x7feb352d)&0xffffffff;h^=h>>15
    phases.append(f32(1+f32((h&0xffff)*f32(59/65536))))
a=phases[:];b=phases[:]
for hz in [60,61,59,60]:a=[old.step(x,hz) for x in a];b=[new.step(x,hz) for x in b]
assert len(set(a))==1 and len(set(b))==20
duration=[]
for hz in [30,60,90,120]:
    x=1.;wraps=0
    for n in range(hz*6):x=new.step(x,hz,.75);wraps+=new.true_wraps
    total=wraps*120+x-1;assert abs(total-270)<.015
    duration.append(dict(hz=hz,seconds=6,total_frames=total,true_wraps=wraps))
assert abs(new.step(75.25,60,clamp=True,period=60)-61)<.00001
assert new.step(1,120,.75,override=10.25)==10.625
report=dict(original_runtime_sha256=pe.runtime_info['runtime_text_sha256'],game_execution=False,rendering_verified=False,
    native_instruction_range='0x37a720..0x37a9fc; original floor/getters/setter included',
    revised_core_sha256=hashlib.sha256((ROOT/'native/frame_step_core.h').read_bytes()).hexdigest(),
    stubs=[hex(v) for v in sorted(new.stubs)],
    limitations=['model evaluation supplies controlled looping or clamped time; no game assets or GPU evaluated',
                 'ctypes bridge executes production C++ frame core; hook scoping separately exercised in host lifecycle tests'],
    single_steps=singles,sequences=sequences,phase_count_before=20,phase_count_original=len(set(a)),phase_count_revised=len(set(b)),
    six_second_movement=duration,clamped_end_and_subframe_override=True)
(OUT/'native_verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
