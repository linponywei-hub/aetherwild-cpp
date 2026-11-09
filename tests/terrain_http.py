"""Public terrain authoring tiles are bounded, read-only and match gameplay."""
import json,os,socket,subprocess,sys,tempfile,time,urllib.request,urllib.error
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build/aetherwild_workstation'
with socket.socket() as sock:sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
base=f'http://127.0.0.1:{port}/api/'
def request(path,body=None,token=''):
 headers={'X-Aetherwild-Client':'1','Content-Type':'application/json'}
 if token:headers['Authorization']='Bearer '+token
 req=urllib.request.Request(base+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
 try:
  with urllib.request.urlopen(req,timeout=15) as r:return r.status,json.load(r)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
with tempfile.TemporaryDirectory(prefix='aw_terrain_') as folder:
 env=dict(os.environ,AETHERWILD_DATA_DIR=folder,AETHERWILD_PUBLIC='0');env.pop('GAME_OWNER_CODE',None);env.pop('GAME_INVITE_CODE',None)
 process=subprocess.Popen([str(exe),'--root',str(root),'--port',str(port)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 try:
  for _ in range(200):
   if process.poll() is not None:raise RuntimeError(process.communicate()[1].decode())
   try:
    if request('health')[0]==200:break
   except OSError:time.sleep(.05)
  else:raise RuntimeError('Terrain fixture did not start')
  # Real deployed asset route, bytes and MIME, not just JS import strings.
  for name in ['grass-color','grass-normal','grass-arm','gravel-color','gravel-normal','gravel-arm']:
   with urllib.request.urlopen(base.replace('/api/','/art/terrain/')+name+'.webp',timeout=15) as asset:
    assert asset.headers['Content-Type']=='image/webp'
    assert asset.read()==(root/'assets/reference/terrain'/(name+'.webp')).read_bytes()
  initial=request('health')[1]['profiles']
  code,catalog=request('catalog');assert code==200
  for r in catalog['regions']:
   if r['id']=='aw_underlost':continue
   code,result=request('terrain',{'x':r['position']['x'],'z':r['position']['z']})
   assert code==200 and set(result)=={'ok','terrain','worldLimit'}
   tile=result['terrain'];assert tile['side']==65 and tile['step']==16 and tile['version']==3 and len(tile['heights'])==4225
  assert request('health')[1]['profiles']==initial
  assert request('terrain',{'x':48001,'z':0})[0]==400
  assert request('terrain',{'x':'bad','z':0})[0]==400
  assert request('terrain',{'x':0})[0]==400
  assert request('state')[0]==401
  code,session=request('session',{'name':'地形測試','profession':'warrior'});assert code==200
  code,state=request('state',token=session['token']);assert code==200
  p=state['player']['position'];tile=request('terrain',{'x':p['x'],'z':p['z']})[1]['terrain']
  assert tile==state['content']['terrainPatch']
  if len(sys.argv)>2:
   data={'catalog':catalog,'ridge':request('terrain',{'x':(425-633)*64,'z':(130-578)*64})[1]['terrain']}
   Path(sys.argv[2]).write_text(json.dumps(data))
  print('TERRAIN HTTP OK: 24 region anchors; bounds/types/auth/read-only invariants; editor and gameplay tiles identical')
 finally:process.terminate();process.communicate(timeout=10)
