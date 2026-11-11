#!/usr/bin/env python3
"""Audit all original body resources and execute original selector/color bytes.

Unicorn only: synthetic dependency values, no Windows process or GPU. The full
officer-ID domain is not a claim to possess every officer's live scenario data.
R16's bounded in-game audit supplies that missing per-person selection evidence.
"""
import collections
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE
from unicorn.x86_const import *
from runtime_disasm import RuntimeDisassembler
from link_analysis import Graph

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/appearance_validation_r16'
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

class NativeBytes:
    STOP=0x100000
    DUEL=0x210000
    ACTOR=0x220000
    PERSON=0x230000
    FORCE=0x240000
    DATA=0x300000
    ITEMS=0x400000
    NODES=0x500000
    VTABLE=0x510000
    PREDICATE=0x520000
    STACK=0x880008
    def __init__(self):
        d=RuntimeDisassembler(ROOT/'inputs'); self.pe=d.pe; self.base=d.pe.image_base
        self.uc=Uc(UC_ARCH_X86,UC_MODE_64)
        self.uc.mem_map(self.base,(self.pe.image_size+4095)&~4095)
        self.uc.mem_map(0x100000,0x800000)
        for s in self.pe.sections:
            self.uc.mem_write(self.base+s['rva'],self.pe.data[s['raw']:s['raw']+s['size']])
        self.stats=(60,50,40,30); self.items=[]
        self.q(self.base+0x277df18,self.DATA); self.q(self.DATA+0x77b18,self.ITEMS)
        self.q(self.VTABLE+8,self.PREDICATE)
        # Environmental calls only; the selector, force-color selection,
        # horse-ID branches, item-index arithmetic and request bytes stay intact.
        hooks=[0x32e4c0,0x22bf60,0x22f210,0x91b60,0x91c10,0x3722b0]
        for rva in hooks:self.uc.hook_add(UC_HOOK_CODE,self.hook,begin=self.base+rva,end=self.base+rva)
        self.uc.hook_add(UC_HOOK_CODE,self.hook,begin=self.PREDICATE,end=self.PREDICATE)
    def q(self,a,n):self.uc.mem_write(a,struct.pack('<Q',n))
    def i(self,a,n):self.uc.mem_write(a,struct.pack('<i',n))
    def byte(self,a,n):self.uc.mem_write(a,bytes([n]))
    def getq(self,a):return struct.unpack('<Q',self.uc.mem_read(a,8))[0]
    def geti(self,a):return struct.unpack('<i',self.uc.mem_read(a,4))[0]
    def ret(self,value):
        sp=self.uc.reg_read(UC_X86_REG_RSP)
        self.uc.reg_write(UC_X86_REG_RAX,value&0xffffffffffffffff)
        self.uc.reg_write(UC_X86_REG_RSP,sp+8)
        self.uc.reg_write(UC_X86_REG_RIP,self.getq(sp))
    def hook(self,uc,address,size,user):
        r=address-self.base
        if address==self.PREDICATE:self.ret(1)
        elif r==0x32e4c0:self.ret(1 if uc.reg_read(UC_X86_REG_RCX) else 0)
        elif r==0x22bf60:self.ret(self.FORCE)
        elif r==0x22f210:self.ret(self.stats[uc.reg_read(UC_X86_REG_EDX)])
        elif r==0x3722b0:self.ret(0) # Weapon resource coverage is audited separately.
        elif r in (0x91b60,0x91c10):
            out=uc.reg_read(UC_X86_REG_RDX)
            self.q(out,self.VTABLE); self.q(out+8,self.NODES if r==0x91b60 and self.items else 0)
            self.q(out+16,0); self.ret(out)
        else:raise AssertionError(hex(address))
    def prepare(self,officer=1000,gender=0,monarch=0,emperor=0,style=0,force_color=0,stats=(60,50,40,30),items=()):
        self.stats=stats; self.items=list(items)
        self.uc.mem_write(self.DUEL,bytes(0x1c0)); self.uc.mem_write(self.PERSON,bytes(0x1f0))
        self.i(self.DUEL+0xa8,officer); self.q(self.DUEL+0xb0,self.PERSON)
        self.i(self.DUEL+0x20,-1); self.i(self.DUEL+0x24,-1)
        self.byte(self.PERSON+0xa9,gender); self.byte(self.PERSON+0xe6,monarch); self.byte(self.PERSON+0xe7,style)
        self.byte(self.FORCE+0x47,emperor); self.byte(self.FORCE+0x49,force_color)
        for j,item in enumerate(self.items):
            self.q(self.NODES+j*16,self.ITEMS+item*128)
            self.q(self.NODES+j*16+8,self.NODES+(j+1)*16 if j+1<len(self.items) else 0)
    def run(self,rva,rcx=None,rdx=0,r8=0):
        for r in (UC_X86_REG_RAX,UC_X86_REG_RBX,UC_X86_REG_RBP,UC_X86_REG_RSI,UC_X86_REG_RDI,
                  UC_X86_REG_R9,UC_X86_REG_R10,UC_X86_REG_R11):self.uc.reg_write(r,0)
        self.q(self.STACK,self.STOP); self.uc.reg_write(UC_X86_REG_RSP,self.STACK)
        self.uc.reg_write(UC_X86_REG_RCX,self.DUEL if rcx is None else rcx)
        self.uc.reg_write(UC_X86_REG_RDX,rdx); self.uc.reg_write(UC_X86_REG_R8,r8&0xffffffff)
        self.uc.emu_start(self.base+rva,self.STOP,count=20000)
        assert self.uc.reg_read(UC_X86_REG_RIP)==self.STOP,'Unexpected or unbounded native path'
        return struct.unpack('<i',struct.pack('<I',self.uc.reg_read(UC_X86_REG_EAX)))[0]

def main():
    OUT.mkdir(exist_ok=True)
    tables=json.loads((ROOT/'analysis/appearance_tables.json').read_text())
    graph=Graph(ROOT); settings={r['setting_hash_hex']:r for r in json.loads((ROOT/'analysis/link_graph.json').read_text())['settings']}
    # The common shader is indexed in the later STATE capture's system.rdb,
    # which was not part of the earlier LINKS index. Its bytecode is not captured.
    system_index=json.loads((ROOT/'analysis/state_index.json').read_text())
    for record in system_index['records']:
        graph.resources[record['resource_id']].append(dict(index_file=system_index['file'],**record))
    rows=[]
    for body in tables['characters']:
        s=graph.setting(body)
        assert s['setting_hash_hex'] in settings and s['model']['indexes']
        for resources in s['descriptor_resources'].values():
            assert all(r['indexes'] for r in resources)
        for ident in s['wrapper_base_material_objects']:
            if int(ident,16):graph.object(int(ident,16))
        for row in s['color_material_matrix']:
            for ident in row:
                if int(ident,16):graph.object(int(ident,16))
        if s['color_variation_count']:
            assert s['wrapper_base_material_objects']==s['color_material_matrix'][0]
        rows.append(dict(selector=body['selector'],name=body['name'],setting=body['setting_hash_hex'],
                         model=s['model']['resource_hash_hex'],color_variants=s['color_variation_count'],
                         base_materials=s['wrapper_base_material_objects'],color_materials=s['color_material_matrix']))
    assert [r['selector'] for r in rows]==list(range(25))
    assert sum(r['color_variants']==5 for r in rows)==9
    assert sum(r['color_variants']==0 for r in rows)==16
    horse=settings['0xe1866005']; assert horse['color_variation_count']==3
    for row in horse['color_material_matrix']:
        for ident in row:
            if int(ident,16):graph.object(int(ident,16))
    for w in tables['weapons']:
        assert graph.setting(w)['model']['indexes']

    emu=NativeBytes(); seen=collections.defaultdict(list); id_cases=0
    for gender in (0,1):
        for officer in range(6001):
            emu.prepare(officer=officer,gender=gender)
            body=emu.run(0x371fe0); assert 0<=body<25
            if gender==0:seen[body].append(officer)
            id_cases+=1
    special={body:ids[0] for body,ids in seen.items() if len(ids)==1}
    assert len(special)==15
    for body,officer in special.items():
        emu.prepare(officer=officer,force_color=1,items=(1,))
        emu.run(0x371f60)
        assert emu.geti(emu.DUEL+0x14)==body and emu.geti(emu.DUEL+0x20)==-1
        assert emu.geti(emu.DUEL+0x24)==2
    emu.prepare(monarch=1,emperor=1); emu.run(0x371f60)
    assert emu.geti(emu.DUEL+0x14)==10 and emu.geti(emu.DUEL+0x20)==-1
    # General selection boundaries, high/low stats, ties, female and ruler paths.
    common_cases=[]
    for gender in (0,1):
        for monarch,emperor in ((0,0),(1,0),(1,1)):
            for style in (0,73):
                for stats in ((60,50,40,30),(69,0,0,0),(70,0,0,0),(89,0,0,0),(90,0,0,0),
                              (0,60,0,0),(0,0,69,0),(0,0,70,0),(0,0,89,0),(0,0,90,0),
                              (0,0,0,90),(90,90,90,90),(0,0,0,0)):
                    emu.prepare(gender=gender,monarch=monarch,emperor=emperor,style=style,stats=stats)
                    body=emu.run(0x371fe0); assert body==-1 or 0<=body<25
                    common_cases.append(dict(gender=gender,monarch=monarch,emperor=emperor,style=style,stats=stats,body=body))
    reached=set(special)|{r['body'] for r in common_cases if r['body']>=0}; assert reached==set(range(25))
    palette=struct.unpack_from('<64i',emu.pe.data,emu.pe.rva_to_offset(0x1a3e190))
    assert set(palette)==set(range(5))
    force_checks=0
    # Execute the ORIGINAL full resolver; only its weapon dependency is stubbed.
    for case in common_cases:
        if case['body'] not in (11,12,13,14,15,16,22,23,24):continue
        for color in range(64):
            emu.prepare(gender=case['gender'],monarch=case['monarch'],emperor=case['emperor'],
                        style=case['style'],stats=case['stats'],force_color=color)
            emu.run(0x371f60)
            assert emu.geti(emu.DUEL+0x14)==case['body'] and emu.geti(emu.DUEL+0x20)==palette[color]
            force_checks+=1
    horse_checks=[]
    for items in [(),(1,),(3,),(5,),(8,),(2,),(4,),(7,),(100,),(2,1),(2,5),(5,1),(1,5)]:
        emu.prepare(items=items); emu.run(0x371df0)
        expected=next((2 if n==1 else 1 for n in items if n in (1,3,5,8)),-1)
        assert emu.geti(emu.DUEL+0x24)==expected
        horse_checks.append(dict(item_indices=items,horse_variant=expected))
    queue_cases=0
    for body in range(25):
        colors=range(5) if rows[body]['color_variants'] else (-1,)
        for color in colors:
            for horse_color in (-1,0,1,2):
                emu.prepare(); emu.i(emu.DUEL+0x14,body)
                emu.uc.mem_write(emu.ACTOR,bytes(0x190)); emu.q(emu.ACTOR+0x40,emu.DUEL)
                before=bytes(emu.uc.mem_read(emu.ACTOR+0xb8,0x30))
                emu.run(0x1c8000,emu.ACTOR,0,color); emu.run(0x1c8000,emu.ACTOR,1,horse_color)
                secondary=5 if body==14 else 4 if body in (12,13,15,23,24) else 0
                assert (emu.geti(emu.ACTOR+0x7c),emu.geti(emu.ACTOR+0x80),emu.geti(emu.ACTOR+0x84))==(color,1,secondary)
                assert (emu.geti(emu.ACTOR+0xac),emu.geti(emu.ACTOR+0xb0),emu.geti(emu.ACTOR+0xb4))==(horse_color,1,0)
                assert bytes(emu.uc.mem_read(emu.ACTOR+0xb8,0x30))==before
                queue_cases+=1
    # Original initializer uses a qword store at +1c, setting +20 to -1.
    init_offset=emu.pe.rva_to_offset(0x372ee6)
    assert emu.pe.data[init_offset:init_offset+8]==bytes.fromhex('48 c7 41 1c ff ff ff ff')
    OUT.joinpath('rva_00372e80.asm.txt').write_text(RuntimeDisassembler(ROOT/'inputs').disassemble(0x372e80))
    # Execute original apply code on a no-variation setting. The negative
    # sentinel completes without resource calls; row zero cannot be applied.
    wrapper=emu.ACTOR+0x58; handle=0x260000; setting=0x270000
    emu.uc.mem_write(setting,bytes(0x100)); emu.q(wrapper+0x10,handle); emu.q(handle+0x20,setting)
    emu.i(wrapper+0x24,-1); emu.i(wrapper+0x2c,0)
    assert emu.run(0x3717c0,wrapper,emu.DATA,0)==1
    emu.i(wrapper+0x24,0)
    assert emu.run(0x3717c0,wrapper,emu.DATA,0)==0
    proof_files=['inputs/SAN14CE_KO.exe','inputs/SAN14CE_KO.runtime_text.bin','analysis/appearance_tables.json',
                 'analysis/link_graph.json','analysis/state_index.json','tools/verify_all_appearances.py',
                 'analysis/appearance_validation_r16/rva_00372e80.asm.txt']
    report=dict(schema='fieldleader3d.all_appearance_validation.r16',status='PASS_OFFLINE_NOT_PIXEL_VERIFIED',
        body_settings=25,unique_or_fixed_body_settings=16,common_body_settings=9,common_color_rows=45,
        total_body_appearances=61,horse_resource_variants=3,weapon_settings=6,
        native_id_domain_cases=id_cases,native_common_boundary_cases=len(common_cases),
        native_force_color_cases=force_checks,native_horse_selection_cases=len(horse_checks),native_color_queue_cases=queue_cases,
        native_bodies_reached=sorted(reached),native_fixed_body_base_checks=16,
        common_base_materials_match_variant_zero=True,special_officer_ids={str(k):v for k,v in sorted(special.items())},
        resources=rows,horse_materials=horse['color_material_matrix'],horse_selection=horse_checks,
        source_sha256={p:sha(ROOT/p) for p in proof_files},
        emulator='Unicorn 2.1.4 x86-64; original runtime instructions',
        stubbed_dependencies=['valid pointer predicate','effective statistics','force pointer','owned-item iterator construction and filter',
                              'weapon selector return (not part of emulated body/color claim)'],
        original_functions_executed=['371fe0 body selector','371f60 appearance/color selection','371df0 horse selector',
                                     '22d300 item-index arithmetic','1c8000 material-slot routing','379550 per-instance color request',
                                     '3717c0 no-variation base/no-op versus invalid-row branch (no resource load)'],
        original_default_body_color=-1,original_default_horse_color=-1,
        base_sentinel_native_apply_branch_checked=True,
        actual_live_officer_roster_verified=False,original_material_loader_executed=False,game_pixels_verified=False,
        limitations=['ID-domain and boundary tests use synthetic inputs, not every officer\'s actual attributes.',
                     'R16 automatically records the current game\'s full person-table selection separately from loaded/submitted field models.',
                     'No actual Windows/GPU rendering or FPS test occurred here. All-person pixel/color verification remains pending.',
                     'The common shader is indexed, not captured or independently executed; original installed game resources are used.',
                     'Horse treasure-name/color correspondence is not invented. Base and all alternatives apply to unique AND common bodies.'])
    (OUT/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('status','body_settings','total_body_appearances','horse_resource_variants',
          'native_id_domain_cases','native_common_boundary_cases','native_force_color_cases','native_horse_selection_cases','native_color_queue_cases')},ensure_ascii=False))

if __name__=='__main__':main()
