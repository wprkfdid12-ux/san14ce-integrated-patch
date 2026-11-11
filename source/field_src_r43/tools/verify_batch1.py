#!/usr/bin/env python3
"""R18: execute original x64 control/ABI bytes and join original motion clips.

The renderer, audio device and real Windows process are not available here.
Only environmental/native dispatch dependencies are stubbed in Unicorn.
"""
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
def fbits(f): return struct.unpack('<I',struct.pack('<f',f))[0]

class BatchBytes(NativeBytes):
    COLLECTOR=0x600000; AUDIO=0x610000; NODE=0x620000; MATRIX=0x630000; POINT=0x640000
    def __init__(self):
        super().__init__(); self.calls=[]; self.stops=[]; self.interpolating=0
        for r in (0x171520,0x171190,0x1a3630,0x406720,0x405000,0xff7a80,0xff7aa4,0x1c9950):
            self.uc.hook_add(UC_HOOK_CODE,self.observe,begin=self.base+r,end=self.base+r)
        self.q(self.base+0x27d26c8,self.AUDIO)
    def observe(self,uc,address,size,user):
        r=address-self.base
        args=[uc.reg_read(x) for x in (UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9)]
        self.calls.append((r,args))
        if r==0x406720:
            # The original caller writes only DL for this bool parameter;
            # the upper RDX bits intentionally still contain the Army pointer.
            assert args[0]==self.COLLECTOR+0xa0 and args[1]&0xff==1 and args[2]==0
            self.ret(self.NODE)
        elif r==0x405000:
            assert args[0]==self.AUDIO+0x2a0; self.stops.append(args[1]); self.ret(1)
        elif r==0x1c9950:self.ret(self.interpolating)
        elif r in (0xff7a80,0xff7aa4):self.ret(0)
        else:self.ret(1)
    def execute(self,start,end=None,regs=None):
        self.calls=[]
        for r in (UC_X86_REG_RAX,UC_X86_REG_RBX,UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_RSI,
                  UC_X86_REG_RDI,UC_X86_REG_RBP,UC_X86_REG_R8,UC_X86_REG_R9,UC_X86_REG_R10,
                  UC_X86_REG_R11,UC_X86_REG_R12,UC_X86_REG_R13,UC_X86_REG_R14,UC_X86_REG_R15):
            self.uc.reg_write(r,0)
        stack=self.STACK if end is None else self.STACK-8
        self.q(stack,self.STOP); self.uc.reg_write(UC_X86_REG_RSP,stack)
        for r,v in (regs or {}).items():self.uc.reg_write(r,v)
        stop=self.STOP if end is None else self.base+end
        self.uc.emu_start(self.base+start,stop,count=20000)
        assert self.uc.reg_read(UC_X86_REG_RIP)==stop,hex(self.uc.reg_read(UC_X86_REG_RIP))
        return self.uc.reg_read(UC_X86_REG_EAX)

def main():
    sources=['tools/verify_batch1.py','tools/verify_all_appearances.py','tools/state_assets_analysis.py',
             'analysis/link_graph.json','analysis/appearance_tables.json','analysis/motion_timing_analysis.json',
             'analysis/sound_package_analysis.json','inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin']
    graph=json.loads((ROOT/'analysis/link_graph.json').read_text())
    settings={s['setting_hash_hex']:s for s in graph['settings']}
    tables=json.loads((ROOT/'analysis/appearance_tables.json').read_text())
    timing=json.loads((ROOT/'analysis/motion_timing_analysis.json').read_text())['tables']
    timeline_proof=[]
    for timeline,action in ((7,9),(21,19),(15,39)):
        row=next(t for t in timing['timelines'] if t['timeline_id']==timeline)
        requests=[timing['requests'][r] for r in row['requests_in_original_order']]
        assert any(r['kind']=='PersonMotion' and r['value']==action and r['start_frame']==0 for r in requests)
        assert any(r['kind']=='HorseMotion' and r['value']==1 and r['start_frame']==0 for r in requests)
        timeline_proof.append({'timeline':timeline,'body':action,'horse':1})
    rows=[]; clips={}
    for body in tables['characters']:
        for action in (9,19,39):
            entries=[a for a in settings[body['setting_hash_hex']]['actions'] if a['action_id']==action]
            assert len(entries)==1
            hashes=[]; frames=[]
            for anim in entries[0]['animations']:
                for file in anim['files']:
                    h=file['resource_hash_hex']; rel='analysis/state_decoded/asset_'+h[2:]+'.bin'
                    parsed=parse_g2a((ROOT/rel).read_bytes()); assert parsed['joints']
                    frames.append(parsed['frames']); hashes.append(h); sources.append(rel)
                    clips[h]={'frames':parsed['frames'],'sha256':sha(ROOT/rel)}
            expected=152 if action==9 else 180 if action==19 else 80 if body['selector'] in (1,2,6,17,18,19,20) else 60
            assert frames and set(frames)=={expected},(body['selector'],action,frames)
            rows.append({'body':body['selector'],'action':action,'frames':expected,'clips':hashes})
    assert len(rows)==75
    checks=[]
    def passed(s):checks.append(s)
    emu=BatchBytes(); uc=emu.uc
    # Native caller, not a reconstruction of the mod: root followed by ten
    # subrecords. Only the first exact E8 is patched by this release.
    emu.execute(0x171207,0x17123e,{UC_X86_REG_RBX:emu.DUEL})
    calls=[a for r,a in emu.calls if r==0x171520]
    assert len(calls)==11 and [a[0] for a in calls]==[emu.DUEL+i*0x40 for i in range(11)]
    assert all(a[1:3]==[emu.STACK-8+0x58,emu.STACK-8+0x38] for a in calls)
    passed('Native tactic root call and all ten child records preserve the three-argument ABI; one root event per execution')
    emu.q(emu.NODE+8,emu.DUEL)
    emu.execute(0x36cc40,regs={UC_X86_REG_RCX:emu.NODE})
    assert [(r,a[0]) for r,a in emu.calls]==[(0x171190,emu.DUEL)]
    passed('Original cutscene phase-four closure reaches the same tactic executor with its bound STacticsValue')
    for timer in (0,1,12,-1,-44):
        emu.i(emu.DUEL+0x310,timer)
        emu.execute(0x179b2b,0x179b4a,{UC_X86_REG_RBX:emu.DUEL})
        assert len([r for r,a in emu.calls if r==0x171190])==(timer==0)
        assert emu.geti(emu.DUEL+0x310)==timer-1
    passed('Original no-cutscene countdown executes tactics exactly at zero, not when they are enqueued')
    # Native map buckets distinguish direct damage (effect 1) from healing
    # (effect 20). Execute the index choice, style choice, and caller ABI.
    for kind,bucket,style in ((1,0,0x34),(20,1,0x2e)):
        emu.execute(0x17189c,0x1718be,{UC_X86_REG_R13:kind,UC_X86_REG_RDI:emu.NODE})
        assert uc.reg_read(UC_X86_REG_EBX)==bucket
        emu.execute(0x171270,0x17127b,{UC_X86_REG_RBP:bucket,UC_X86_REG_R15:0x34})
        assert uc.reg_read(UC_X86_REG_ESI)==style
        emu.q(emu.NODE+0x20,emu.ACTOR); emu.i(emu.NODE+0x28,345)
        emu.q(emu.base+0x1b91dc0,emu.DATA)
        emu.execute(0x171324,0x17133a,{UC_X86_REG_RBX:emu.NODE,UC_X86_REG_RSI:style})
        assert emu.calls==[(0x1a3630,[emu.DATA,345,emu.ACTOR,style])]
    # kind20 is tied to 177110 by the original switch table and heals soldiers
    # from the wounded pool. This prevents recovery from being mislabeled hit.
    p=emu.pe; switch=p.unpack('<23I',p.rva_to_offset(0x178098))
    assert switch[19]==0x177f96
    code=p.data[p.rva_to_offset(0x177fa0):p.rva_to_offset(0x177fa0)+5]
    assert code[0]==0xe8 and 0x177fa5+struct.unpack_from('<i',code,1)[0]==0x177110
    uc.mem_write(emu.ACTOR+0x16,struct.pack('<HH',1000,500))
    emu.execute(0x177313,0x177344,{UC_X86_REG_RSI:emu.ACTOR,UC_X86_REG_RDI:100,UC_X86_REG_R15:65535})
    assert bytes(uc.mem_read(emu.ACTOR+0x16,4))==struct.pack('<HH',1100,400)
    passed('Original damage/recovery buckets, native number styles, four-argument notification and actual recovery arithmetic agree')
    # Actual viewport math: second argument to 1c5690 is the NDC bound, NOT
    # additive padding. Zero would silence every source. Identity projection
    # tests call the original multiply and comparison bytes in 113e0.
    uc.mem_write(emu.MATRIX,struct.pack('<16f',*[1.0 if i//4==i%4 else 0.0 for i in range(16)]))
    emu.i(emu.base+0x1b951f4,0)
    view_cases=0
    for xyz,bound,expected in (((0,0,.5),1,1),((1.1,0,.5),1,0),((0,-1.1,.5),1,0),
                               ((0,0,-.5),1,0),((0,0,1.5),1,0),((0,0,.5),0,0)):
        uc.mem_write(emu.POINT,struct.pack('<4f',*xyz,1)); uc.reg_write(UC_X86_REG_XMM2,fbits(bound))
        result=emu.execute(0x113e0,regs={UC_X86_REG_RCX:emu.MATRIX,UC_X86_REG_RDX:emu.POINT})
        assert result==expected,(xyz,bound,result); view_cases+=1
    passed('Original frustum math accepts visible points and rejects sides, behind/far points, and an invalid zero extent')
    emu.i(emu.COLLECTOR+0xd0,0); emu.byte(emu.AUDIO+0x9a8+0x99,7)
    result=emu.execute(0x403c90,regs={UC_X86_REG_RCX:emu.COLLECTOR,UC_X86_REG_RDX:emu.ACTOR,
                                    UC_X86_REG_R8:123,UC_X86_REG_R9:0x99})
    assert result==1 and emu.getq(emu.NODE)==emu.ACTOR and emu.geti(emu.NODE+8)==123
    assert emu.geti(emu.NODE+0xc)==0x99 and emu.geti(emu.NODE+0x10)==-1
    emu.q(emu.COLLECTOR+0xd0,42)
    assert emu.execute(0x403c90,regs={UC_X86_REG_RCX:emu.COLLECTOR})==0 and not emu.calls
    passed('Original movement queue ABI, success value, source/sound identity and 42-node full branch are executed')
    emu.q(emu.COLLECTOR+0x20,emu.NODE); emu.q(emu.COLLECTOR+0x28,emu.NODE+0x40)
    emu.i(emu.NODE+0x10,7); emu.q(emu.NODE+0x18,emu.NODE+0x40)
    emu.i(emu.NODE+0x50,11); emu.q(emu.NODE+0x58,0)
    emu.execute(0x404da0,regs={UC_X86_REG_RCX:emu.COLLECTOR})
    assert emu.stops==[7,11] and emu.getq(emu.COLLECTOR+0x20)==0
    passed('Original cleanup stops exactly the unmatched old voices before returning and empties that list')
    # Request actions with original PersonMotion execute code.
    command=0x650000; wrapper=0x660000; emu.q(wrapper+8,emu.DUEL)
    for action in (9,19,39):
        emu.i(command+0x18,action); emu.run(0x3726d0,rcx=command,rdx=wrapper)
        assert emu.geti(emu.DUEL+0x28)==action
        assert bytes(uc.mem_read(emu.DUEL+0x30,4))==struct.pack('<f',1)
    passed('Original PersonMotion execute writes all three requested duel actions with the native first-frame convention')
    sound=json.loads((ROOT/'analysis/sound_package_analysis.json').read_text())
    gait=[r for r in sound['horse_event_links'] if r['action_id']==1]
    expected={0xe3c22636,0xc5540415,0xa6e5e1f4,0x8877bfd3}
    assert {int(r['program_id_hex'],16) for r in gait}==expected
    assert all(0<r['sound_duration_seconds']<.562 and r['properties']['Pitch']==[0] for r in gait)
    passed('Four original gait sound programs have pitch zero and finite captured sample durations below 562ms')
    report={'schema':'fieldleader3d.batch1_evidence.r18','status':'PASS_OFFLINE_NOT_GAME_VERIFIED',
            'body_count':25,'body_action_bindings':75,'timeline_pairs':timeline_proof,'rows':rows,'clips':clips,
            'native_checks':checks,'native_view_cases':view_cases,
            'source_sha256':{s:sha(ROOT/s) for s in sorted(set(sources))},
            'game_execution':False,'gpu_motion_verified':False,'native_audio_device_executed':False,
            'limitations':['Native camera/render/audio devices and live Windows hooks still need the integrated game test.',
                           'Exact visual alignment of the strong-attack strike with each cutscene is not asserted.',
                           'Hoof duration reservations include 150ms beyond the native samples; actual audibility and balance need listening.',
                           'Original land ambient-profile sound classification is based on native branches, not a recorded live listening test.']}
    (ROOT/'analysis/batch1_r18.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':report['status'],'body_action_bindings':len(rows),'native_checks':len(checks),'native_view_cases':view_cases}))

if __name__=='__main__':main()
