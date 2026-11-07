"""Require complete artwork for the original playable catalogue, including shadowstep."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
catalog = json.loads((root / 'data/legacy_catalog.json').read_text(encoding='utf-8'))
scope = json.loads((root / 'data/artwork/phase-one.json').read_text(encoding='utf-8'))
manifest = json.loads((root / 'data/artwork/manifest.json').read_text(encoding='utf-8'))['entries']
expected = {
    'equipment': set(catalog['ITEMS']),
    'material': set(catalog['MATERIALS']),
    'tool': set(catalog['TOOLS']),
    'consumable': set(catalog['ARROWS']) | {'potion', 'energy_potion', 'return_crystal'},
    'skill': set(catalog['SKILLS']),
}
for category, ids in expected.items():
    assert set(scope['groups'][category]) == ids, category
all_ids = set().union(*expected.values())
assert len(scope['ids']) == len(all_ids) == 143
assert set(scope['ids']) == all_ids
missing = [key for key in sorted(all_ids) if manifest.get(key, {}).get('status') != 'ready']
assert not missing, 'Phase one artwork missing: ' + ', '.join(missing)
for category, ids in expected.items():
    for key in ids:
        entry = manifest[key]
        assert entry['category'] == category, key
        assert (root / entry['path']).is_file(), key
print('ARTWORK PHASE ONE OK: all 143 original catalogue IDs have dedicated ready assets')
