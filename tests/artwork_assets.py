"""Validate shipped artwork against exact IDs, alpha, names and file hashes."""
import hashlib,json,pathlib
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[1]
m=json.loads((root/'data/artwork/manifest.json').read_text())
ready=[e for e in m['entries'].values() if e['status']=='ready']
assert ready
digests=set()
for e in ready:
 p=root/e['path'];assert p.name==e['id']+'.png' and e['name']
 assert hashlib.sha256(p.read_bytes()).hexdigest()==e['sha256']
 assert e['sha256'] not in digests,'Different items must not silently share the same image'
 digests.add(e['sha256'])
 im=Image.open(p);assert im.format=='PNG' and im.mode=='RGBA'
 hist=im.getchannel('A').histogram();assert hist[0]>im.width*im.height*.1
 assert sum(hist[200:])>im.width*im.height*.02
 assert im.width<=512 and im.height<=342
print('ARTWORK ASSETS OK:',len(ready),'verified distinct transparent PNGs /',len(m['entries']),'planned')
