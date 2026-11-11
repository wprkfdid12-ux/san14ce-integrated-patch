#!/usr/bin/env python3
"""R20: execute original control/ABI bytes, not a Windows game or GPU test."""
import hashlib,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from verify_batch1 import BatchBytes,fbits
from state_assets_analysis import parse_g2a
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
class CombatBytes(BatchBytes):
    def __init__(self):
        super().__init__();self.extra=[]
        for r in (0x1694a0,0xe52490,0x22a5e0,0x2b1580,0x2a92b0,0x16ca80,0x16da60):
            self.uc.hook_add(UC_HOOK_CODE,self.contact,begin=self.base+r,end=self.base+r)
    def contact(self,uc,a,size,user):
        r=a-self.base;sp=uc.reg_read(UC_X86_REG_RSP)
        args=[uc.reg_read(x) for x in (UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9)]
        self.extra.append((r,args))
        if r==0x1694a0:
            assert args[0]==self.PERSON and args[3]==self.FORCE
            assert [uc.reg_read(x)&0xffffffff for x in (UC_X86_REG_XMM1,UC_X86_REG_XMM2)]==[fbits(11),fbits(12)]
            assert self.geti(sp+0x28)==fbits(21) and self.geti(sp+0x30)==fbits(22)
            self.i(self.getq(sp+0x38),789);self.ret(0)
        elif r==0xe52490:
            assert args==[self.ACTOR,23,self.PERSON,self.FORCE]
            assert self.getq(sp+0x28)==self.DATA and self.geti(sp+0x30)==67
            assert [self.geti(sp+0x38+8*i) for i in range(12)]==[fbits(i+1) for i in range(12)]
            self.ret(0)
        elif r==0x22a5e0:self.ret(self.PERSON)
        elif r in (0x2b1580,0x2a92b0):
            assert args[1] in (self.ACTOR,self.FORCE) and args[2]==0 and args[3]==self.PERSON
            val=(11 if args[1]==self.ACTOR else 21)+(r==0x2a92b0)
            uc.reg_write(UC_X86_REG_XMM0,fbits(val));self.ret(0)
        elif r==0x16ca80:self.ret(80)
        elif r==0x16da60:self.ret(60)
def main():
    e=CombatBytes();checks=[]
    def passed(x):checks.append(x)
    e.uc.mem_write(e.DUEL,struct.pack('<10f',100,11,12,0,0,100,21,22,0,0))
    e.execute(0x169b57,0x169b91,{UC_X86_REG_RBP:e.DUEL,UC_X86_REG_R10:e.PERSON,UC_X86_REG_R9:e.FORCE})
    assert e.geti(e.STACK-8+0x50)==789
    passed('Original normal-damage call passes seven operands and caller-owned output unchanged')
    bp=e.DUEL;e.q(bp+8,e.PERSON);e.q(bp+0x10,e.FORCE);e.q(bp+0x18,e.DATA)
    e.i(bp+0x20,23);e.i(bp+0x24,67)
    for i in range(12):e.i(bp+0x28+4*i,fbits(i+1))
    e.execute(0xe4cc6c,0xe4cf11,{UC_X86_REG_RBP:bp,UC_X86_REG_R15:e.ACTOR})
    passed('Original effect call passes all 18 arguments and NULL skips native effect registration safely')
    e.q(e.DUEL,e.NODES);e.i(e.NODES,1);e.i(e.NODES+4,0x1b);e.i(e.DUEL+8,2);e.i(e.DUEL+12,0x1b)
    e.q(e.DATA+0x7e838+8,e.ACTOR);e.q(e.DATA+0x7e838+16,e.FORCE)
    e.byte(e.ACTOR+0x25,0);e.byte(e.FORCE+0x25,0)
    e.uc.mem_write(e.ITEMS,struct.pack('<10f',100,0,0,0,0,100,0,0,0,0))
    original_pair=bytes(e.uc.mem_read(e.DUEL,0x30));original_ref=bytes(e.uc.mem_read(e.NODES,0x20))
    e.execute(0x169e20,regs={UC_X86_REG_RCX:e.DUEL,UC_X86_REG_RDX:e.ITEMS})
    e.execute(0x169c60,regs={UC_X86_REG_RCX:e.DUEL,UC_X86_REG_RDX:e.ITEMS})
    values=struct.unpack('<10f',e.uc.mem_read(e.ITEMS,40));assert [values[i] for i in (1,2,6,7)]==[11,12,21,22]
    assert bytes(e.uc.mem_read(e.DUEL,0x30))==original_pair and bytes(e.uc.mem_read(e.NODES,0x20))==original_ref
    passed('Original pair-stat readers resolve both scratch descriptors and fill source/target A-D slots without modifying descriptors')
    e.q(e.DUEL,e.NODES);e.q(e.DUEL+0x10,e.ACTOR);e.q(e.DUEL+0x18,0)
    e.uc.mem_write(e.NODES,bytes(0x100));e.byte(e.NODES+0x6a,1)
    e.execute(0x16aa60,regs={UC_X86_REG_RCX:e.DUEL,UC_X86_REG_RDX:e.FORCE,UC_X86_REG_R8:0})
    got=struct.unpack('<f',struct.pack('<I',e.uc.reg_read(UC_X86_REG_XMM0)&0xffffffff))[0]
    divisor=e.geti(e.base+0x1a48ae0);floor=e.geti(e.base+0x1a48af8)/100
    assert abs(got-max(floor,1+20/divisor))<1e-6
    passed('Original tactic factor accepts scratch context and applies its own ability-difference constants')
    state=e.DUEL;node=e.PERSON;query=e.NODES;stack=e.ITEMS
    for name in (b'CUserStrategyState',b'CProgressState',b'CMenuState'):
        e.q(state+0x10,1);e.q(state+0x20,stack);e.q(stack,node)
        e.uc.mem_write(node+0x70,name+b'\0');e.uc.mem_write(query,b'CUserStrategyState\0')
        got=e.execute(0x56f350,regs={UC_X86_REG_RCX:state,UC_X86_REG_RDX:query})
        assert got==(name==b'CUserStrategyState')
    e.q(state+0x10,0);assert e.execute(0x56f350,regs={UC_X86_REG_RCX:state,UC_X86_REG_RDX:query})==0
    passed('Native top-state query distinguishes strategy pause, live progress, menus and empty state stack')
    # Execute the original 2D parameter writes, isolating documented register inputs.
    bp=e.DUEL
    e.execute(0xe50022,0xe5004c,{UC_X86_REG_RBP:bp,UC_X86_REG_R14:0x762497dc,
        UC_X86_REG_R11:3,UC_X86_REG_R12:0,UC_X86_REG_XMM0:fbits(.4),UC_X86_REG_XMM1:fbits(.75)})
    assert bytes(e.uc.mem_read(bp-0x50,16))==struct.pack('<IIff',0x762497dc,3,.4,.75)
    passed('Original sound writes prove ProgramIdx at +0, integer Pitch at +4, gain at +8; no guessed sound layout')
    graph=json.loads((ROOT/'analysis/link_graph.json').read_text());settings={s['setting_hash_hex']:s for s in graph['settings']}
    tables=json.loads((ROOT/'analysis/appearance_tables.json').read_text());sources=set();rows=[]
    for body in tables['characters']:
        for action in (13,19,35,39):
            a=next(a for a in settings[body['setting_hash_hex']]['actions'] if a['action_id']==action)
            frames=[];programs=[]
            for animation in a['animations']:
                for f in animation['files']:
                    rel='analysis/state_decoded/asset_'+f['resource_hash_hex'][2:]+'.bin';sources.add(rel)
                    frames.append(parse_g2a((ROOT/rel).read_bytes())['frames'])
            expected=180 if action==19 else 80 if action==39 and body['selector'] in (1,2,6,17,18,19,20) else 60
            assert set(frames)=={expected}
            for h in a['event_object_hashes']:
                p=graph['events'][h]['properties']
                if 'ProgramIdx' in p:programs.append(dict(program=p['ProgramIdx'],frames=p['DataPacketFrame']))
            if action==19:assert any(0xdd622565 in p['program'] for p in programs)
            if action==13:assert any(0x762497dc in p['program'] for p in programs)
            if action==39:assert any(0x6fd54d22 in p['program'] for p in programs)
            if action==35:assert not programs
            rows.append(dict(body=body['selector'],action=action,frames=expected,audio=programs))
    passed('All 25 body classes have native sweep13, spin19, guard35 and hit39; all strong clips expose original contact events')
    sources.update(('tools/verify_combat_r20.py','tools/verify_batch1.py','tools/verify_all_appearances.py','tools/state_assets_analysis.py',
        'analysis/link_graph.json','analysis/appearance_tables.json','analysis/sound_package_analysis.json','inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin'))
    report=dict(schema='fieldleader3d.combat_r20_evidence.v1',status='PASS_OFFLINE_NOT_GAME_VERIFIED',checks=checks,body_action_bindings=len(rows),rows=rows,
        source_sha256={r:sha(ROOT/r) for r in sorted(sources)},windows_game_test=False,gpu_audio_device_test=False)
    (ROOT/'analysis/combat_r20_evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status=report['status'],native_checks=len(checks),body_action_bindings=len(rows))))
if __name__=='__main__':main()
