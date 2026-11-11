"""Generate immutable CE fallback rules; runtime snapshots override live fields."""
from pathlib import Path
import json
root=Path(__file__).resolve().parent
rows=['/* Generated from the bundled original 219-trait DB, not officer names. */',
      'static const GTraitRule original_trait_rules[MAX_TRAITS]={']
for item in json.loads((root.parent/'data/SAN14CE_개성마스터219_DB.json').read_text()):
    b=bytes.fromhex(item['raw_param_hex'])
    rows.append('[%d]={%d,{%d,%d},%d,{%d,%d},0,1,0},' % (
        item['trait_id'],int.from_bytes(b[2:4],'little'),
        int.from_bytes(b[5:7],'little',signed=True),int.from_bytes(b[8:10],'little',signed=True),b[0],b[4],b[7]))
rows.append('};')
(root/'trait_rules_db.h').write_text('\n'.join(rows)+'\n')
