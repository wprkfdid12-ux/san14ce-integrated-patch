from pathlib import Path
import json,hashlib
root=Path(__file__).resolve().parents[1]
load=lambda n:json.loads((root/'data'/n).read_text(encoding='utf-8-sig'))
people=load('SAN14CE_공식무장1000_DB.json')+load('SAN14CE_공식보너스88_DB.json')
traits=load('SAN14CE_개성마스터219_DB.json')
unique=sorted({v for person in people for v in person['unique_tactic_ids']})
assert [p['officer_id'] for p in people]==list(range(1,1089))
lines=['/* Generated ONLY from uploaded CE official database; do not hand-edit. */','const GTeacher g_teachers[G_TEACHERS]={']
for p in people:
 a=[p[k] for k in ['leadership','war','intelligence','politics','charisma']]
 star=bool(p['unique_tactic_ids'])
 lines.append('{%d,%d,%d,{%s},%d,%d,{%s},%s},'%(p['officer_id'],p['father_id'],p['mother_id'],','.join(map(str,a)),star,1 if p['officer_id']<=1000 else 2,','.join(map(str,p['trait_slots'])),json.dumps(p['name'],ensure_ascii=False)))
lines+=['};','const GTrait g_traits[MAX_TRAITS]={','{0,0,0,""},']
for p in traits:
 anchor=p['core_holder_ids'][0] if p['core_holder_count']==1 else 0
 lines.append('{%d,%d,%d,%s},'%(p['category_code'],p['core_holder_count'],anchor,json.dumps(p['name'],ensure_ascii=False)))
lines+=['};','const unsigned char g_unique_tactics[256]={'+','.join('1' if v in unique else '0' for v in range(256))+'};']
(root/'source/official_db.h').write_text('\n'.join(lines)+'\n')
(root/'data/provenance.json').write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (root/'data').glob('SAN14CE*.json')},indent=2))
