#!/usr/bin/env python3
"""Original resource and x64 ABI evidence for R17; no Windows/GPU claim."""
import hashlib
import json
from pathlib import Path
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from verify_all_appearances import NativeBytes
from state_assets_analysis import parse_g2a

ROOT=Path(__file__).resolve().parents[1]
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

class SoundABI(NativeBytes):
    A=0x600000; B=0x610000; C=0x620000; D=0x630000
    PARAMS=0x640000; RESULT=0x650000; DISPATCH=0x660000; CONTEXT=0x670000
    def __init__(self):
        super().__init__()
        self.hits=0
        for rva in (0xf5cbb0,0xf5f2c0,0xf5f880):
            self.uc.hook_add(UC_HOOK_CODE,self.sound_hook,begin=self.base+rva,end=self.base+rva)
        self.q(self.C+0x58,self.CONTEXT)
        self.q(self.C+0x50,self.CONTEXT)
        self.q(self.CONTEXT+0x78,0)
        self.q(self.DISPATCH+8,self.DISPATCH+0x100)
    def sound_hook(self,uc,address,size,user):
        if address==self.base+0xf5cbb0:
            assert uc.reg_read(UC_X86_REG_RCX)==self.CONTEXT
            self.ret(self.DISPATCH); return
        assert address==self.base+self.final_rva
        assert [uc.reg_read(r) for r in (UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9)]==[
            self.DISPATCH+0x100,self.DISPATCH,self.B,self.C]
        sp=uc.reg_read(UC_X86_REG_RSP)
        assert self.getq(sp+0x28)==self.D
        assert self.geti(sp+0x30)==17 and self.getq(sp+0x38)==self.A and self.geti(sp+0x40)==29
        copied=self.getq(sp+0x48)
        assert bytes(uc.mem_read(copied,len(self.payload)))==self.payload
        assert self.getq(sp+0x50)==self.RESULT
        self.i(self.RESULT,12345); self.hits+=1; self.ret(1)
    def check(self,positional,bank):
        self.final_rva=0xf5f880 if positional else 0xf5f2c0
        self.payload=bytes((i*13+7)%256 for i in range(52 if positional else 44))
        self.uc.mem_write(self.PARAMS,self.payload); self.i(self.RESULT,0)
        self.q(self.STACK,self.STOP)
        self.uc.reg_write(UC_X86_REG_RSP,self.STACK)
        for r,v in zip((UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9),(self.A,self.B,self.C,self.D)):
            self.uc.reg_write(r,v)
        for offset,value in ((0x28,17),(0x30,29),(0x38,self.PARAMS),(0x40,self.RESULT),(0x48,bank)):
            self.q(self.STACK+offset,value)
        before=self.hits
        self.uc.emu_start(self.base+(0xf0acc0 if positional else 0xf0aba0),self.STOP,count=2000)
        assert self.uc.reg_read(UC_X86_REG_RIP)==self.STOP and self.uc.reg_read(UC_X86_REG_AL)==1
        assert self.geti(self.RESULT)==12345 and self.hits==before+1

def main():
    source_files=['tools/verify_mounted_motion.py','tools/verify_all_appearances.py',
                  'tools/state_assets_analysis.py','analysis/link_graph.json','analysis/appearance_tables.json',
                  'analysis/motion_timing_analysis.json','inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin']
    graph=json.loads((ROOT/'analysis/link_graph.json').read_text())
    settings={s['setting_hash_hex']:s for s in graph['settings']}
    tables=json.loads((ROOT/'analysis/appearance_tables.json').read_text())
    timing=json.loads((ROOT/'analysis/motion_timing_analysis.json').read_text())['tables']
    for timeline,pair in ((0,(55,0)),(39,(3,1))):
        row=next(t for t in timing['timelines'] if t['timeline_id']==timeline)
        requests=[timing['requests'][r] for r in row['requests_in_original_order']]
        assert [(r['kind'],r['value'],r['start_frame']) for r in requests]==[
            ('PersonMotion',pair[0],0),('HorseMotion',pair[1],0)]
    rows=[]; clips={}; sounds=[]
    for body,setting,ids in [(b['selector'],b['setting_hash_hex'],(3,55)) for b in tables['characters']]+[
            ('horse','0xe1866005',(0,1))]:
        for action_id in ids:
            actions=[a for a in settings[setting]['actions'] if a['action_id']==action_id]
            assert len(actions)==1,(body,action_id)
            action=actions[0]; hashes=[]
            for anim in action['animations']:
                for file in anim['files']:
                    h=file['resource_hash_hex']; rel='analysis/state_decoded/asset_'+h[2:]+'.bin'
                    data=(ROOT/rel).read_bytes(); parsed=parse_g2a(data)
                    assert parsed['frames']>1 and parsed['joints']
                    clips[h]={'frames':parsed['frames'],'bytes':len(data),'sha256':sha(ROOT/rel)}
                    source_files.append(rel); hashes.append(h)
            assert hashes
            for h in action['event_object_hashes']:
                event=graph['events'][h]
                if any(s['type_hex']=='0xd8004a77' for s in event['sources']):
                    assert body=='horse' and action_id==1
                    props=event['properties']
                    sounds.append({'event':h,'properties':props})
            rows.append({'body':body,'setting':setting,'action_id':action_id,
                         'action':action['object_hash_hex'],'clips':hashes,'events':action['event_object_hashes']})
    assert len(rows)==52 and len(sounds)==4
    emu=NativeBytes(); command=0x600000; wrapper=0x610000
    emu.q(wrapper+8,emu.DUEL)
    for rva,offset,ids in ((0x3726d0,0x28,(3,55)),(0x372680,0x2c,(0,1))):
        for action_id in ids:
            emu.uc.mem_write(emu.DUEL,bytes(0x1c0)); emu.i(command+0x18,action_id)
            emu.run(rva,rcx=command,rdx=wrapper)
            assert emu.geti(emu.DUEL+offset)==action_id
            assert bytes(emu.uc.mem_read(emu.DUEL+offset+8,4))==struct.pack('<f',1)
    sound=SoundABI()
    for positional in (False,True):
        for bank in (0,0x680000):sound.check(positional,bank)
    report={'schema':'fieldleader3d.mounted_motion_evidence.r17','status':'PASS_OFFLINE_NOT_GAME_VERIFIED',
            'body_count':25,'body_action_bindings':50,'horse_action_bindings':2,'rows':rows,'clips':clips,
            'native_motion_execute_cases':4,'native_sound_abi_cases':4,'sound_events':sounds,
            'source_sha256':{rel:sha(ROOT/rel) for rel in sorted(set(source_files))},
            'game_execution':False,'gpu_motion_verified':False,'native_audio_playback_executed':False,
            'limitations':['Native sound final dispatch and audio devices are stubbed in the emulator.',
                'Native model playback and attachment rendering still need the user game test.',
                'R17 gates added sound events; replacement hoof mixing is not implemented.']}
    (ROOT/'analysis/mounted_motion_r17.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','body_count','body_action_bindings','horse_action_bindings',
                                        'native_motion_execute_cases','native_sound_abi_cases')}))

if __name__=='__main__':main()
