#!/usr/bin/env python3
"""Validate captured G2A tables and the ASRS sound package, entirely offline.

G2A table framing was derived from these 125 supplied payloads and checked to
their exact ends. Curve values and track-type meanings are not decoded here.
KTSR field layout reference: vgmstream's original src/meta/ktsr.c implementation,
https://github.com/vgmstream/vgmstream/blob/master/src/meta/ktsr.c
No third-party code is executed or bundled. FFmpeg decodes standard MS ADPCM WAV.
"""
import collections
import hashlib
import json
import struct
import subprocess
import wave
from pathlib import Path
from state_analysis import u16, u32, hx, write


def parse_g2a(data):
    if data[:4] != b'_A2G' or data[4:8] not in (b'0300', b'0500') or len(data) < 32:
        raise ValueError('Unsupported G2A header')
    if u32(data, 8) != len(data):
        raise ValueError('G2A file-size mismatch')
    version = data[4:8].decode()
    head, shift = (32, 14) if version == '0500' else (28, 16)
    fps = struct.unpack_from('<f', data, 12)[0]
    frames, joints = u32(data, 16) & 0xfffff, u32(data, 16) >> 20
    key_bytes, point_count = u32(data, 20), u32(data, 24)
    key_start = head+joints*4
    point_start = key_start+key_bytes
    if not frames or fps != 60 or point_start+point_count*32 != len(data):
        raise ValueError('Unexpected G2A table framing')
    rows, intervals, points = [], [], []
    for n in range(joints):
        word = u32(data, head+n*4)
        count = word & 15
        joint = (word >> 4) & ((1 << (shift-4))-1)
        pos = key_start+(word >> shift)*4
        tracks = []
        if not count:
            raise ValueError('Empty declared joint')
        for _ in range(count):
            if not key_start <= pos < pos+8 <= point_start:
                raise ValueError('Track header outside key table')
            kind, count = struct.unpack_from('<HH', data, pos)
            point_index = u32(data, pos+4)
            end = (pos+8+count*2+3) & ~3
            if kind not in (0, 1, 2) or not count or end > point_start or point_index+count > point_count:
                raise ValueError('Invalid track span')
            times = list(struct.unpack_from('<'+'H'*count, data, pos+8))
            if times != sorted(set(times)) or times[0] != 0 or times[-1] >= frames:
                raise ValueError('Invalid keyframe times')
            if any(data[pos+8+count*2:end]):
                raise ValueError('Nonzero track padding')
            tracks.append(dict(type_code=kind, key_count=count, control_point_index=point_index,
                               key_frames=times, offset=pos, length=end-pos))
            intervals.append((pos, end))
            points.append((point_index, point_index+count))
            pos = end
        rows.append(dict(joint_index=joint, descriptor_offset=head+n*4, tracks=tracks))
    def exact_cover(spans, first, last):
        for start,end in sorted(spans):
            if start != first or end <= start:
                raise ValueError('Gap or overlap in G2A tables')
            first = end
        if first != last:
            raise ValueError('Incomplete G2A table coverage')
    exact_cover(intervals, key_start, point_start)
    exact_cover(points, 0, point_count)
    if len({r['joint_index'] for r in rows}) != joints:
        raise ValueError('Repeated joint descriptor')
    return dict(magic='_A2G', version=version, bytes=len(data), fps=fps, frames=frames,
                nominal_duration_seconds=frames/fps, joint_count=joints,
                track_count=len(intervals), control_point_count=point_count,
                key_table_offset=key_start, key_table_bytes=key_bytes,
                packed_curve_offset=point_start, packed_curve_bytes=point_count*32,
                curve_sha256=hashlib.sha256(data[point_start:]).hexdigest(),
                unknown_header_1c=u32(data, 28) if head == 32 else None,
                joints=rows, exact_table_coverage=True,
                caveat='Packed curve values, transform-type meanings, bone names and action semantics are not decoded.')


def parse_audio(data):
    if data[:4] != b'ASRS' or data[16:20] != b'KTSR' or u32(data, 8) != len(data):
        raise ValueError('Unsupported ASRS package')
    if u32(data, 40) != len(data)-16 or u32(data, 44) != len(data)-16 or data[27] != 1:
        raise ValueError('KTSR size or platform mismatch')
    rows, songs, configs = [], [], []
    pos = 80
    while pos < len(data):
        kind, size, ident = struct.unpack_from('<3I', data, pos)
        end = pos+size
        if size < 0x20 or end > len(data) or kind not in (0x368c88bd,0x70cbccc5):
            raise ValueError('Unsupported KTSR chunk')
        row = dict(offset=pos, bytes=size, type_hex=hx(kind), id_hex=hx(ident))
        rows.append(row)
        if kind == 0x368c88bd:
            row['name_offset'] = u32(data, pos+0x28)
            configs.append(row)
            pos = end
            continue
        if u32(data, pos+16) != 1:
            raise ValueError('Multiple audio substreams not supported')
        pointer = pos+u32(data, pos+20)
        if not pos+28 <= pointer <= end-4:
            raise ValueError('Audio subheader pointer out of range')
        sub = pos+u32(data, pointer)
        if not pos+32 <= sub <= end-0x54 or u32(data, sub) != 0x2eb6ca6f or sub+u32(data,sub+4) != end:
            raise ValueError('Unexpected internal sound subheader')
        channels, rate, samples = u32(data, sub+12), u32(data, sub+20), u32(data, sub+24)
        if channels != 1 or data[sub+16] != 0 or not 1000 <= rate <= 192000:
            raise ValueError('Unsupported audio codec configuration')
        extra = sub+u32(data, sub+40)
        starts = sub+u32(data, sub+48)
        sizes = sub+u32(data, sub+52)
        if not all(sub <= o <= end-4 for o in (extra, starts, sizes)):
            raise ValueError('Sound table out of range')
        frame_samples, frame_bytes = u16(data, extra), u16(data, extra+2)
        start, byte_count = sub+u32(data, starts), u32(data, sizes)
        if frame_bytes < 7 or frame_samples != (frame_bytes-7)*2+2 or byte_count % frame_bytes:
            raise ValueError('Invalid mono MS ADPCM framing')
        if not sub <= start < start+byte_count <= end or byte_count//frame_bytes*frame_samples != samples:
            raise ValueError('Audio data size/sample count mismatch')
        if any(data[start+byte_count:end]):
            raise ValueError('Nonzero sound tail padding')
        if any(data[off] > 6 for off in range(start,start+byte_count,frame_bytes)):
            raise ValueError('ADPCM predictor out of range')
        row.update(flags_hex=hx(u32(data,pos+12)), name_offset=u32(data,pos+24),
                   subheader_offset=sub, channels=channels, sample_rate=rate, samples=samples,
                   duration_seconds=samples/rate, frame_samples=frame_samples, frame_bytes=frame_bytes,
                   stream_offset=start, stream_bytes=byte_count,
                   codec='MS_ADPCM_MONO', sha256=hashlib.sha256(data[start:start+byte_count]).hexdigest())
        songs.append(row)
        pos = end
    if len({s['id_hex'] for s in songs}) != len(songs):
        raise ValueError('Duplicate sound IDs')
    return dict(schema='fieldleader3d.sound_package.v1', source_resource_hex='0x24192634',
                bytes=len(data), chunks=rows, programs=configs, sounds=songs,
                reference='https://github.com/vgmstream/vgmstream/blob/master/src/meta/ktsr.c',
                exact_chunk_coverage=pos==len(data))


def decode_wave(data, song, directory):
    """Wrap an unmodified captured mono stream in standard WAV, then decode."""
    rate, block, per_block = song['sample_rate'], song['frame_bytes'], song['frame_samples']
    coefficients = [(256,0),(512,-256),(0,0),(192,64),(240,0),(460,-208),(392,-232)]
    fmt = struct.pack('<HHIIHHHHH',2,1,rate,rate*block//per_block,block,4,32,per_block,7)
    fmt += b''.join(struct.pack('<hh',*c) for c in coefficients)
    def chunk(tag, body):
        return tag+struct.pack('<I',len(body))+body+(b'\0' if len(body)%2 else b'')
    raw = data[song['stream_offset']:song['stream_offset']+song['stream_bytes']]
    body = b'WAVE'+chunk(b'fmt ',fmt)+chunk(b'fact',struct.pack('<I',song['samples']))+chunk(b'data',raw)
    source = directory/(song['id_hex'][2:]+'_adpcm.wav')
    pcm = directory/(song['id_hex'][2:]+'_pcm.wav')
    source.write_bytes(b'RIFF'+struct.pack('<I',len(body))+body)
    subprocess.run(['ffmpeg','-v','error','-y','-i',str(source),'-c:a','pcm_s16le',str(pcm)],check=True)
    with wave.open(str(pcm),'rb') as wav:
        if (wav.getnchannels(),wav.getframerate(),wav.getnframes(),wav.getsampwidth()) != (1,rate,song['samples'],2):
            raise ValueError('Decoded PCM metadata does not match original sound')
    return dict(adpcm_file=source.name, pcm_file=pcm.name, pcm_sha256=hashlib.sha256(pcm.read_bytes()).hexdigest(),
                decoder='FFmpeg', original_sample_count_verified=True, listening_classification='NOT_PERFORMED')


def main():
    root = Path(__file__).resolve().parents[1]
    graph = json.loads((root/'analysis/link_graph.json').read_text())
    clips = {}
    for path in sorted((root/'analysis/state_decoded').glob('*.bin')):
        data = path.read_bytes()
        if data[:4] == b'_A2G':
            clips['0x'+path.stem.removeprefix('asset_')] = dict(output_name=path.name, **parse_g2a(data))
    used = {v['resource_hash_hex'] for a in graph['actions'].values() for obj in a['animations'] for v in obj['files']}
    if used != set(clips):
        raise ValueError('Captured clips do not exactly cover original action references')
    report = dict(schema='fieldleader3d.g2a_analysis.v1', clips=clips,
                  summary=dict(clips=len(clips), versions=dict(collections.Counter(c['version'] for c in clips.values())),
                               tracks=sum(c['track_count'] for c in clips.values()),
                               frames_range=[min(c['frames'] for c in clips.values()),max(c['frames'] for c in clips.values())]),
                  caveat='The DB property is G1AFileResourceHash, but all captured bytes use G2A magic. Action names are not inferred from durations.')
    write(root/'analysis/g2a_analysis.json',report)
    data = (root/'analysis/state_decoded/asset_24192634.bin').read_bytes()
    sounds = parse_audio(data)
    by_id = {s['id_hex']:s for s in sounds['sounds']}
    horse = next(s for s in graph['settings'] if s['setting_hash_hex']=='0xe1866005')
    out = root/'analysis/horse_audio';out.mkdir(exist_ok=True)
    links = []
    decoded = {}
    for action in horse['actions']:
        for event in action['event_object_hashes']:
            properties = graph['events'][event]['properties']
            programs = properties.get('ProgramIdx',[])
            # Use the exact recorded property name; never search arbitrary payload words.
            if not programs:
                continue
            if properties.get('BankObjectNameHash') != [0x11de6328]:
                raise ValueError('Unexpected horse audio bank')
            for program in programs:
                ident = hx(program)
                if ident not in by_id:
                    raise ValueError('Horse program has no matching sound in captured package')
                if ident not in decoded:
                    decoded[ident] = decode_wave(data,by_id[ident],out)
                links.append(dict(action_id=action['action_id'], event_hash_hex=event,
                                  program_id_hex=ident, properties=properties,
                                  sound_offset=by_id[ident]['offset'], sound_duration_seconds=by_id[ident]['duration_seconds']))
    sounds['horse_event_links'] = links
    sounds['decoded_previews'] = decoded
    sounds['limitations'] = ['Program/sound IDs and complete sample payloads are joined, but the clips have not been classified by listening.',
                            'Missing sound names remain missing; no synthetic sound is substituted.',
                            'Original event timing, data packets, volume and scheduling must remain with the original action.']
    write(root/'analysis/sound_package_analysis.json',sounds)
    print(json.dumps(dict(g2a=report['summary'], audio_sounds=len(sounds['sounds']),
                          audio_programs=len(sounds['programs']), horse_event_links=len(links), decoded_previews=len(decoded))))


if __name__ == '__main__':
    main()
