"""Actual native static delivery of registered PNG icons, isolated test world."""
import json,pathlib,socket,subprocess,tempfile,time,urllib.request
root=pathlib.Path(__file__).resolve().parents[1]
manifest=json.loads((root/'data/artwork/manifest.json').read_text())
with tempfile.TemporaryDirectory(prefix='aw_artwork_http_') as tmp:
 folder=pathlib.Path(tmp)
 for name in ['web','assets']:(folder/name).symlink_to(root/name,target_is_directory=True)
 with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
 p=subprocess.Popen([str(root/'build/aetherwild_workstation'),'--root',tmp,'--port',str(port)],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 try:
  for _ in range(200):
   try:
    with urllib.request.urlopen(f'http://127.0.0.1:{port}/api/health',timeout=1) as r:assert json.load(r)['ok']
    break
   except OSError:time.sleep(.1)
  count=0
  for e in manifest['entries'].values():
   if e['status']!='ready':continue
   with urllib.request.urlopen(f'http://127.0.0.1:{port}/art/items/'+e['id']+'.png',timeout=5) as r:
    assert r.headers['Content-Type']=='image/png';assert r.read()==(root/e['path']).read_bytes();count+=1
  for path in ['artwork-index.js','artwork-names.js']:
   with urllib.request.urlopen(f'http://127.0.0.1:{port}/'+path,timeout=5) as r:assert r.status==200 and 'export const' in r.read().decode()
  print('ARTWORK HTTP OK:',count,'exact PNG files and two modules served by C++')
 finally:p.terminate();p.communicate(timeout=15)
