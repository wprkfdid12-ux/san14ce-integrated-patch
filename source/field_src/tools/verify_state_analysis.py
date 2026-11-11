#!/usr/bin/env python3
"""Verify real cross-file/state joins and reject plausible but corrupt inputs."""
import base64
import copy
import hashlib
import json
import struct
import wave
from pathlib import Path
from state_analysis import OriginalSelector, Snapshot, u32, write
from state_assets_analysis import parse_g2a, parse_audio


def main():
    root = Path(__file__).resolve().parents[1]
    checks = []
    def check(name, result):
        if not result:
            raise AssertionError(name)
        checks.append(dict(name=name, status='PASS'))
    def rejects(name, fn):
        try:
            fn()
        except (ValueError, KeyError, struct.error):
            check(name, True)
        else:
            check(name, False)
    live = json.loads((root/'inputs/state_capture/live_state.json').read_text())
    saved = json.loads((root/'analysis/field_state_analysis.json').read_text())
    graph = json.loads((root/'analysis/link_graph.json').read_text())
    selector = OriginalSelector(root)
    base = int(live['module_base_hex'], 16)
    snapshots = [Snapshot(s, base).build(selector,graph) for s in live['snapshots']]
    check('Field joins reproduce from captured pointers, typed objects and pool bounds', snapshots == saved['snapshots'])
    check('Both observations contain 31 complete field-to-officer bindings', len(snapshots)==2 and all(s['matched_armies']==31 for s in snapshots))
    check('Land, water and siege all agree with terrain/formation and original model slots', all(s['mode_counts']==dict(land_troops=22,water_ship=4,land_siege=5) for s in snapshots))
    first = {r['officer_id']:r for r in snapshots[0]['rows']}
    check('Zhao is actually deployed and reaches his original setting', first[596]['officer_name']=='조운' and first[596]['appearance']['setting_hash_hex']=='0x7c6a126d')
    check('Named common-body officers share the original monarch setting', all(first[i]['appearance']['setting_hash_hex']=='0x20d9cc18' for i in (266,268,720,953)))
    check('Common body variants come from captured ForceData and original color table', [(first[i]['officer_name'],first[i]['appearance']['color_variant']) for i in (266,268,720,953)]==[('공손찬',0),('공손도',1),('등애',2),('유표',1)])
    check('Uncaptured stat dependencies are not replaced by base-stat guesses', sum(r['appearance']['status']=='UNRESOLVED_STAT_DEPENDENCIES' for r in first.values())==25)
    check('No active duel or original game function call is claimed', all(s['duel_units']==0 for s in live['snapshots']) and not live['game_functions_called'])
    check('Separate snapshots explicitly retain 19 changed scheduling bytes', len(saved['cross_snapshot_changed_blocks'])==19 and all(c['kind']=='field_visual_state' and c['changed_byte_offsets']==[0x19c] for c in saved['cross_snapshot_changed_blocks']))
    def alter_snapshot(kind,index,offset,patch):
        value=copy.deepcopy(live['snapshots'][0])
        row=next(b for b in value['blocks'] if b['kind']==kind and b['index']==index)
        data=bytearray(base64.b64decode(row['bytes_base64']));data[offset:offset+len(patch)]=patch
        row['bytes_base64']=base64.b64encode(data).decode();row['sha256']=hashlib.sha256(data).hexdigest()
        return Snapshot(value,base).build(selector,graph)
    rejects('A valid-checksum but wrong person ID cannot be joined',lambda:alter_snapshot('officer',596,0x10,struct.pack('<H',597)))
    rejects('A valid-checksum but broken field binding cannot pass',lambda:alter_snapshot('field_header',24,0x40,b'\0'*8))
    rejects('A water mode inconsistent with terrain cannot pass',lambda:alter_snapshot('field_visual_state',24,0,struct.pack('<I',1)))
    g2a=json.loads((root/'analysis/g2a_analysis.json').read_text())
    check('All 125 G2A parses reproduce with exact key/curve table coverage',len(g2a['clips'])==125 and all(parse_g2a((root/'analysis/state_decoded'/c['output_name']).read_bytes())=={k:v for k,v in c.items() if k!='output_name'} for c in g2a['clips'].values()))
    used={r['resource_hash_hex'] for a in graph['actions'].values() for v in a['animations'] for r in v['files']}
    check('Every original action clip is captured, with no clip substituted',used==set(g2a['clips']))
    sample=bytearray((root/'analysis/state_decoded/asset_0137d0c9.bin').read_bytes())
    rejects('Truncated G2A is rejected',lambda:parse_g2a(sample[:-1]))
    struct.pack_into('<H',sample,g2a['clips']['0x0137d0c9']['key_table_offset']+2,65535)
    rejects('Out-of-range G2A track is rejected',lambda:parse_g2a(sample))
    data=(root/'analysis/state_decoded/asset_24192634.bin').read_bytes()
    parsed=parse_audio(data);audio=json.loads((root/'analysis/sound_package_analysis.json').read_text())
    check('Full sound package reproduces 13 programs and 45 internal sounds',len(parsed['programs'])==13 and len(parsed['sounds'])==45 and all(parsed[k]==audio[k] for k in parsed))
    check('Horse events reference actual same-ID sounds in the supplied bank',len(audio['horse_event_links'])==10 and len(audio['decoded_previews'])==5 and all(e['program_id_hex'] in {s['id_hex'] for s in audio['sounds']} for e in audio['horse_event_links']))
    check('Horse Action 1 has exactly four tracks and 20 frame events',len([e for e in audio['horse_event_links'] if e['action_id']==1])==4 and sum(len(e['properties']['DataPacketFrame']) for e in audio['horse_event_links'] if e['action_id']==1)==20)
    valid_pcm=True
    for ident,preview in audio['decoded_previews'].items():
        song=next(s for s in audio['sounds'] if s['id_hex']==ident)
        path=root/'analysis/horse_audio'/preview['pcm_file']
        with wave.open(str(path),'rb') as wav:
            valid_pcm &= (wav.getnframes(),wav.getframerate(),wav.getnchannels())==(song['samples'],song['sample_rate'],1)
        valid_pcm &= hashlib.sha256(path.read_bytes()).hexdigest()==preview['pcm_sha256']
    check('Five decoded PCM previews retain original sample counts and rates',valid_pcm)
    bad=bytearray(data);struct.pack_into('<I',bad,80+4,len(data))
    rejects('Audio chunk extending beyond package is rejected',lambda:parse_audio(bad))
    check('Sound names and listening classification remain unconfirmed',all(s['name_offset']==0 for s in audio['sounds']) and all(p['listening_classification']=='NOT_PERFORMED' for p in audio['decoded_previews'].values()))
    request=json.loads((root/'collect/state_request.json').read_text())
    check('Original game EXE remains unchanged',hashlib.sha256((root/'inputs/SAN14CE_KO.exe').read_bytes()).hexdigest()==request['expected_exe_sha256'])
    report=dict(schema='fieldleader3d.state_analysis_verification.v1',status='PASS',checks=checks,
                windows_state_collector='USER_CAPTURE_VERIFIED_20260920_154531',mod_in_game_test='NOT_RUN')
    write(root/'analysis/state_analysis_verification.json',report)
    print(json.dumps(dict(status='PASS',checks=len(checks))))


if __name__ == '__main__':
    main()
