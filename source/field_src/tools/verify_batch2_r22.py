#!/usr/bin/env python3
"""Execute preserved native ABI/state bytes and audit all native fall clips.
No Windows process, GPU rendering or measured game FPS is asserted.
"""
import hashlib,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from verify_batch1 import BatchBytes
from state_assets_analysis import parse_g2a
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
class Evidence(BatchBytes):
    ANCHOR=0x650100; POSITION=0x650200; TRANSITION=0x650300; SIZE=0x650400; ATTACH=0x650500
    def __init__(self):
        super().__init__();self.seen=[];self.fx_result=0
        for r in (0xed00,0x37c050,0x31dcc0,0x379740,0x179110):
            self.uc.hook_add(UC_HOOK_CODE,self.observe22,begin=self.base+r,end=self.base+r)
        for address in (self.ANCHOR,self.POSITION,self.TRANSITION,self.SIZE,self.ATTACH):
            self.uc.hook_add(UC_HOOK_CODE,self.observe22,begin=address,end=address)
    def observe22(self,uc,address,size,user):
        args=[uc.reg_read(r) for r in (UC_X86_REG_RCX,UC_X86_REG_RDX,UC_X86_REG_R8,UC_X86_REG_R9)]
        r=address-self.base
        if r==0xed00:self.ret(self.AUDIO)
        elif r==0x37c050:
            sp=uc.reg_read(UC_X86_REG_RSP)
            self.seen.append(('fx',args,self.geti(sp+0x28)));self.ret(self.fx_result)
        elif r==0x31dcc0:self.seen.append(('commit',args[:2]));self.ret(0)
        elif r==0x379740:self.seen.append(('timeline',args[:2]));self.ret(0)
        elif r==0x179110:self.seen.append(('pair_set',args[:2]));self.ret(0)
        elif address==self.ANCHOR:
            self.seen.append(('anchor',args[:3]));self.i(args[1],350);self.i(args[1]+4,552);self.ret(args[1])
        elif address==self.TRANSITION:self.ret(0)
        elif address==self.POSITION:
            self.seen.append(('position',args[:3]));self.i(args[0]+0x20,args[1]);self.i(args[0]+0x24,args[2]);self.ret(0)
        elif address==self.SIZE:self.ret(0)
        elif address==self.ATTACH:self.seen.append(('attach',args[:2]));self.ret(1)
def main():
    checks=[];sources=set();e=Evidence()
    def passed(s):checks.append(s)
    original=bytes(e.uc.mem_read(e.PERSON,0x1f0))
    e.execute(0x2e060d,0x2e061d,{UC_X86_REG_RSI:e.PERSON})
    expected=bytearray(original);expected[0xe6]=6
    assert bytes(e.uc.mem_read(e.PERSON,0x1f0))==expected
    passed('Original captive handler executes its native person-status setter with6 and writes only person+e6 in this segment')
    e.execute(0x25b016,0x25b025,{UC_X86_REG_R13:e.ACTOR})
    assert e.seen[-1]==('commit',[e.DATA,e.ACTOR])
    passed('Accepted annihilation call uses data manager and army in RCX/RDX, after the native capture decision paths')
    for hand in (0,1):
        e.i(e.DUEL+0xf8,hand);e.i(e.DUEL+0xfc,1)
        xyz=struct.pack('<4f',11,22,33,1);e.uc.mem_write(e.DUEL+0xc0,xyz)
        e.execute(0x378d3d,0x378d69,{UC_X86_REG_RCX:e.DUEL,UC_X86_REG_RBX:e.DUEL})
        assert e.seen[-1]==('timeline',[e.DUEL,19 if hand else 20])
        assert e.geti(e.DUEL+0xfc)==0 and bytes(e.uc.mem_read(e.DUEL+0xd0,16))==xyz
    passed('Original duel fall state chooses timeline19/20 from weapon hand, copies origin and disables mounted attachment')
    for effect in (2,0x48,0x61):
        n=len(e.seen)
        e.execute(0x1c5b3b,0x1c5b66,{UC_X86_REG_RSI:e.ACTOR,UC_X86_REG_RBX:e.ACTOR,UC_X86_REG_RDI:effect})
        assert e.seen[n:]==[('fx',[e.AUDIO,effect,e.STACK-8+0x30,e.ACTOR],0)]
        assert e.getq(e.ACTOR+0x890)==0
    passed('Native ground-dust call has five arguments and NULL leaves the owning slot empty')
    for effect in (1,0x51):
        n=len(e.seen)
        e.execute(0x1c1322,0x1c1351,{UC_X86_REG_RBX:effect,UC_X86_REG_RDI:e.DUEL})
        assert e.seen[n:]==[('fx',[e.AUDIO,effect,e.STACK-8+0x40,e.DUEL],0)]
    passed('Native normal-field arc call has the distinct pair owner and its NULL path skips registration safely')
    e.q(e.ACTOR,e.VTABLE)
    for off,fn in ((0x290,e.ANCHOR),(0x1a8,e.TRANSITION),(0x78,e.POSITION),(0x248,e.SIZE)):e.q(e.VTABLE+off,fn)
    packed=600<<32|400;e.q(e.STACK-8+0x60,packed)
    e.execute(0x1aa3d4,0x1aa43f,{UC_X86_REG_RDI:e.ACTOR})
    assert ('anchor',[e.ACTOR,e.STACK-8+0x70,packed]) in e.seen
    assert ('position',[e.ACTOR,350,552]) in e.seen
    assert e.getq(e.ACTOR+0x20)==552<<32|350
    passed('Native UI caller forwards a packed point and applies returned XY to the same entire layout, not a separate overlay')
    e.q(e.VTABLE+0x68,e.ATTACH)
    e.execute(0x1cae8d,0x1cae99,{UC_X86_REG_RBX:e.ACTOR,UC_X86_REG_RSI:e.AUDIO})
    assert e.seen[-1]==('attach',[e.ACTOR,e.AUDIO])
    passed('Original final hand-attachment dispatch uses private actor vtable+68 after the body scale reset')
    g=json.loads((ROOT/'analysis/link_graph.json').read_text());settings={s['setting_hash_hex']:s for s in g['settings']}
    tables=json.loads((ROOT/'analysis/appearance_tables.json').read_text());rows=[]
    timing=json.loads((ROOT/'analysis/motion_timing_analysis.json').read_text())['tables']
    for tid,action in ((19,46),(20,47)):
        t=next(t for t in timing['timelines'] if t['timeline_id']==tid)
        reqs=[timing['requests'][r] for r in t['requests_in_original_order']]
        assert any(r['kind']=='PersonMotion' and r['value']==action and r['start_frame']==0 for r in reqs)
    for body in tables['characters']:
        for action in (46,47):
            a=next(a for a in settings[body['setting_hash_hex']]['actions'] if a['action_id']==action)
            frames=[];clips=[]
            for anim in a['animations']:
                for f in anim['files']:
                    rel='analysis/state_decoded/asset_'+f['resource_hash_hex'][2:]+'.bin';sources.add(rel)
                    frames.append(parse_g2a((ROOT/rel).read_bytes())['frames']);clips.append(f['resource_hash_hex'])
            assert frames and set(frames)=={120}
            rows.append(dict(body=body['selector'],action=action,frames=120,clips=clips))
    assert len(rows)==50
    passed('All 25 body classes have both original 120-frame fall clips; no new animation or guessed clip duration is used')
    appearance=json.loads((ROOT/'analysis/appearance_validation_r16/report.json').read_text())
    assert appearance['body_settings']==25 and appearance['total_body_appearances']==61 and appearance['horse_resource_variants']==3
    for rel,value in appearance['source_sha256'].items():assert sha(ROOT/rel)==value
    passed('Previous full original appearance/material/horse selection evidence still matches every preserved input')
    sources.update(('tools/verify_batch2_r22.py','tools/verify_batch1.py','tools/verify_all_appearances.py','tools/state_assets_analysis.py',
        'analysis/link_graph.json','analysis/appearance_tables.json','analysis/motion_timing_analysis.json',
        'analysis/appearance_validation_r16/report.json','inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin'))
    report=dict(schema='fieldleader3d.batch2_r22_evidence.v1',status='PASS_OFFLINE_NOT_GAME_VERIFIED',checks=checks,
        fall_bindings=rows,fall_binding_count=50,source_sha256={r:sha(ROOT/r) for r in sorted(sources)},
        windows_game_test=False,gpu_pixels_verified=False,game_fps_measured=False,
        live_pending=['visual effect identity/removal','leg-space badge placement at different zooms','capture timing and native fall in field','real frame pacing'])
    (ROOT/'analysis/batch2_r22_evidence.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status=report['status'],native_checks=len(checks),fall_bindings=len(rows))))
if __name__=='__main__':main()
