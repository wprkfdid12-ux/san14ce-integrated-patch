#!/usr/bin/env python3
"""R21 evidence from original resources and executed native bytes (no game/GPU claim)."""
import hashlib,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from verify_batch1 import BatchBytes,fbits
ROOT=Path(__file__).resolve().parents[1]
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
class Evidence(BatchBytes):
    DEVICE=0x650100
    def __init__(self):
        super().__init__();self.observed=[];self.start_result=7
        for r in (0xc0da00,0x402f70,0x404990,0x1c9de0):
            self.uc.hook_add(UC_HOOK_CODE,self.observe21,begin=self.base+r,end=self.base+r)
        self.uc.hook_add(UC_HOOK_CODE,self.observe21,begin=self.DEVICE,end=self.DEVICE)
        self.stub_start=True;self.stub_volume=False
    def observe21(self,uc,address,size,user):
        r=address-self.base
        args=[uc.reg_read(x) for x in (UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9)]
        if address==self.DEVICE:
            self.observed.append(('device',args[:2],*[uc.reg_read(x)&0xffffffff for x in (UC_X86_REG_XMM2,UC_X86_REG_XMM3)]));self.ret(1)
        elif r==0xc0da00:self.ret(args[0])
        elif r==0x1c9de0:self.observed.append(('army',args[:3]));self.ret(0)
        elif r==0x402f70 and self.stub_start:
            sp=uc.reg_read(UC_X86_REG_RSP)
            self.observed.append(('start',args[:2],uc.reg_read(UC_X86_REG_XMM2)&0xffffffff,uc.reg_read(UC_X86_REG_XMM3)&0xffffffff,self.geti(sp+0x28)))
            self.ret(self.start_result)
        elif r==0x404990 and self.stub_volume:
            self.observed.append(('volume',args[:2],uc.reg_read(UC_X86_REG_XMM2)&0xffffffff));self.ret(1)
def main():
    checks=[];rows=[]
    def passed(x):checks.append(x)
    graph=json.loads((ROOT/'analysis/link_graph.json').read_text());settings={s['setting_hash_hex']:s for s in graph['settings']}
    bodies=json.loads((ROOT/'analysis/appearance_tables.json').read_text())['characters']
    for body in bodies:
        for action in (3,13,19,35,39,55):
            a=next(a for a in settings[body['setting_hash_hex']]['actions'] if a['action_id']==action)
            parents=[]
            for h in a['event_object_hashes']:
                p=graph['events'][h]['properties']
                if p.get('DisplaysetObjectNameHash')==[0x48b2c677]:
                    assert p['DataPacketFrame']==[0.0]
                    parents+=p['ParentObjectName']
            expected=24 if action in (3,55) else 23
            assert parents==[expected],(body['selector'],action,parents)
            rows.append(dict(body=body['selector'],action=action,weapon_parent=expected,private_duel_f8=int(expected==23)))
    assert len(rows)==150
    passed('All 25 body resources bind weapon to bone23 for sweep/spin/guard/hit and bone24 for move/idle at native frame0')
    horse=settings['0xe1866005'];effects={}
    for a in horse['actions']:
        if a['action_id'] not in (0,1):continue
        ids=[]
        for h in a['event_object_hashes']:
            p=graph['events'][h]['properties']
            if p.get('DataPacketValue')==[0xcc6674fe] and 'DisplaysetObjectNameHash' in p:ids+=p['DisplaysetObjectNameHash']
        effects[a['action_id']]=ids
    assert effects[0]==[] and len(effects[1])==20 and set(effects[1])=={0x7363ee3a}
    passed('Original idle horse has no particle events; all 20 run hoof-dust events use display7363ee3a')
    e=Evidence()
    e.q(e.ACTOR+0x40,e.DUEL);e.q(e.PERSON,e.VTABLE)
    for value,expected in ((0,24),(1,23),(2,23)):
        e.i(e.DUEL+0xf8,value)
        e.execute(0x1c1a4c,0x1c1a71,{UC_X86_REG_RBX:e.ACTOR,UC_X86_REG_RBP:e.FORCE,UC_X86_REG_R14:e.PERSON})
        assert e.uc.reg_read(UC_X86_REG_EDX)==expected
    passed('Original 1c19c0 hand-selection bytes turn private CDuelUnit+f8 into exactly the verified bone23/bone24')
    e.q(e.ACTOR,e.base+0x13a0358)
    assert e.getq(e.base+0x13a0378)==e.base+0x1c9de0
    e.execute(0x1cb2b0,0x1cb2bf,{UC_X86_REG_RBX:e.ACTOR,UC_X86_REG_RBP:e.PERSON,UC_X86_REG_RSI:e.FORCE})
    assert e.observed[-1]==('army',[e.ACTOR,e.PERSON,e.FORCE])
    passed('Native field manager resolves army vtable+20 to 1c9de0 and forwards actor/engine/frame unchanged')
    for result in (7,-1):
        e.start_result=result
        e.uc.mem_write(e.STACK-8+0x40,struct.pack('<3f',11,22,33))
        e.execute(0x405cae,0x405cdd,{UC_X86_REG_RDI:e.AUDIO,UC_X86_REG_R14:0x9e,UC_X86_REG_RBX:e.NODE})
        assert e.observed[-1]==('start',[e.AUDIO+0x2a0,0x9e],fbits(11),fbits(22),fbits(33))
        assert e.geti(e.NODE+0x10)==result
    passed('Original ambient-start call has five mixed integer/float arguments and retains success or failure slot in its own queue node')
    e.stub_volume=True
    e.execute(0x3a2670,0x3a2688,{UC_X86_REG_RBX:7,UC_X86_REG_XMM6:fbits(40)})
    assert e.observed[-1]==('volume',[e.AUDIO+0x2a0,7],fbits(40))
    passed('Original later-volume call supplies controller, slot, percent in RCX, EDX, XMM2')
    e.stub_volume=False;ctrl=e.AUDIO+0x2a0
    e.i(ctrl+0x118,1);e.q(ctrl+0x110,e.COLLECTOR);e.q(e.COLLECTOR+0x38,e.NODE)
    e.q(e.NODE,e.VTABLE);e.q(e.VTABLE+0x350,e.DEVICE)
    for slot in (0,7,61):
        e.i(ctrl+0x518+slot*4,1);e.i(ctrl+0x228+slot*4,12345+slot)
        before=bytes(e.uc.mem_read(ctrl,0x800))
        for percent in (40,24,100):
            n=len(e.observed)
            assert e.execute(0x404990,regs={UC_X86_REG_RCX:ctrl,UC_X86_REG_RDX:slot,UC_X86_REG_XMM2:fbits(percent)})==1
            assert e.observed[n:]==[('device',[e.NODE,12345+slot],fbits(percent/100),fbits(1))]
            assert bytes(e.uc.mem_read(ctrl,0x800))==before
    for slot in (-1,62):
        n=len(e.observed)
        assert e.execute(0x404990,regs={UC_X86_REG_RCX:ctrl,UC_X86_REG_RDX:slot&0xffffffff,UC_X86_REG_XMM2:fbits(40)})==0
        assert len(e.observed)==n
    e.i(ctrl+0x118,0);n=len(e.observed)
    assert e.execute(0x404990,regs={UC_X86_REG_RCX:ctrl,UC_X86_REG_RDX:7,UC_X86_REG_XMM2:fbits(40)})==0
    assert len(e.observed)==n
    passed('Full native volume setter validates slots0..61 and controller, converts percent to linear gain, and reads a fresh voice/device without game-state writes')
    sources=('tools/verify_combat_r21.py','tools/verify_batch1.py','tools/verify_all_appearances.py',
        'analysis/link_graph.json','analysis/appearance_tables.json','inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin')
    report=dict(schema='fieldleader3d.combat_r21_evidence.v1',status='PASS_OFFLINE_NOT_GAME_VERIFIED',checks=checks,
        body_action_hand_bindings=len(rows),rows=rows,horse_dust_events=len(effects[1]),source_sha256={s:sha(ROOT/s) for s in sources},
        actual_windows_game_execution=False,actual_gpu_motion_verified=False,actual_audio_device_test=False,
        field_effect_origin='Native actor scope suppression covers hidden source actors; exact residual-particle appearance requires live video confirmation',
        ambience_ids=['0x9e','0x9f','0xa0'],ambience_gain=0.4,ambience_classification='Original field combat branches; not a claim of listening to the live device')
    (ROOT/'analysis/combat_r21_evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status=report['status'],native_checks=len(checks),hand_bindings=len(rows))))
if __name__=='__main__':main()
