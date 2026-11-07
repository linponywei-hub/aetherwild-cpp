"""Validate authored registries before compiling the embedded content bundle."""
import json
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'data/content'
def read(n):return json.loads((p/(n+'.json')).read_text())
expected={'monsters':441,'skills':1681,'classes':46,'equipment':660,'profiles':263,'regions':25,'releases':5}
for name,count in expected.items():
 rows=read(name);assert len(rows)==count,(name,len(rows));assert len({r['id'] for r in rows})==count,name
skills={r['id'] for r in read('skills')};classes=read('classes');ids={r['id'] for r in classes}
for row in classes:
 assert set(row['skills'])<=skills,row['id']
 assert not row.get('parent') or row['parent'] in ids,row['id']
seen=[]
for release in read('releases'):
 assert len(release['regions'])==5
 seen+=release['regions']
assert len(set(seen))==25
assert set(seen)=={r['id'] for r in read('regions')}
assert 'aw_crownreach' in read('releases')[1]['regions']
assert sum(bool(r.get('starting_class')) for r in classes)==10
print('OK content catalog counts, unique IDs, class references and five-region release plan')
