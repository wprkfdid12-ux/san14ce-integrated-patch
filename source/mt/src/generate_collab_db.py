from pathlib import Path
import json, hashlib
root=Path(__file__).resolve().parents[1]
p=root/'data'/'SAN14CE_콜라보39_교사44_DB.json'
d=json.loads(p.read_text(encoding='utf-8'))
rows=d['records']
expected=list(range(155,181))+[197,198,199,200]+list(range(202,211))
ids=sorted({r['trait_id'] for r in rows})
assert len(rows)==44 and ids==expected and len(ids)==39
for r in rows:
    assert len(r['stats'])==5 and all(0<=x<=100 for x in r['stats'])
holders=[0]*355
for r in rows: holders[r['trait_id']]+=1
lines=[
 '/* Generated from data/SAN14CE_콜라보39_교사44_DB.json.',
 ' * Pairings are documented collaboration officers/traits; numeric stats are source-transcribed.',
 ' * This table supplements, and never rewrites, the uploaded 1088-officer official DB. */',
 f'#define G_COLLAB_SOURCES {len(rows)}',
 'typedef struct {u16 trait;u8 stats[5];const char*name;} GCollabSource;',
 'static const GCollabSource g_collab_sources[G_COLLAB_SOURCES]={'
]
for r in rows:
    lines.append('{%d,{%s},%s},'%(r['trait_id'],','.join(map(str,r['stats'])),json.dumps(r['name'],ensure_ascii=False)))
lines += ['};','static const u8 g_collab_holders[MAX_TRAITS]={']
for i in range(0,355,32): lines.append(','.join(map(str,holders[i:i+32]))+',')
lines += ['};']
(root/'source'/'collab_db.h').write_text('\n'.join(lines)+'\n',encoding='utf-8')
prov=root/'data'/'collab_provenance.json'
prov.write_text(json.dumps({'source_file':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'records':len(rows),'exclusive_traits':len(ids)},indent=2)+'\n')
