"""Owner-only shared visual house poses, revision safety, assets and restart."""
import hashlib,json,os,socket,subprocess,sys,tempfile,time,urllib.request,urllib.error
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build/aetherwild_workstation'
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
base=f'http://127.0.0.1:{port}'
def req(path,body=None,token=''):
 headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'}
 if token:headers['Authorization']='Bearer '+token
 request=urllib.request.Request(base+'/api/'+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
 try:
  with urllib.request.urlopen(request,timeout=15) as response:return response.status,json.load(response)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
with tempfile.TemporaryDirectory(prefix='aw_pose_') as folder:
 env=dict(os.environ,AETHERWILD_DATA_DIR=folder,AETHERWILD_PUBLIC='0');env.pop('GAME_OWNER_CODE',None);env.pop('GAME_INVITE_CODE',None)
 def start():
  process=subprocess.Popen([str(exe),'--root',str(root),'--port',str(port)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
  for _ in range(200):
   if process.poll() is not None:raise RuntimeError(process.communicate()[1].decode())
   try:
    if req('health')[0]==200:return process
   except OSError:time.sleep(.05)
  raise RuntimeError('Fixture did not start')
 process=start()
 try:
  owner=req('session',{'name':'姿態擁有者','profession':'warrior'})[1]['token'];guest=req('session',{'name':'姿態訪客','profession':'warrior'})[1]['token']
  pose={'position':[-6272,0,-14592],'modelPosition':[1.13,-.73,2.08],'rotation':[.234,-.81,.12],'rotationOrder':'XYZ','scale':[.82,.82,.82]}
  body={'type':'village_pose_save','index':1,'revision':0,'pose':pose,'request':'pose-first'}
  assert req('command',body)[0]==401
  assert req('command',body,guest)[0]>=400
  assert req('command',body,owner)[0]==200
  assert req('command',body,owner)[1]['content']['villageLayout']['revision']==1
  assert req('catalog')[1]['villageLayout']=={'revision':1,'poses':{'1':pose}}
  assert req('command',{**body,'request':'stale'},owner)[0]>=400
  assert req('catalog')[1]['villageLayout']['poses']['1']==pose
  sources=json.loads((root/'web/assets/models/environment/sources.json').read_text())
  for asset in sources['models']:
   with urllib.request.urlopen(base+asset['path'],timeout=15) as r:assert hashlib.sha256(r.read()).hexdigest()==asset['sha256']
  process.terminate();process.communicate(timeout=10);process=start()
  assert req('state',token=owner)[1]['content']['villageLayout']=={'revision':1,'poses':{'1':pose}}
  print('VILLAGE POSE HTTP OK: owner-only publish; exact full pose; stale and duplicate requests; scanned assets; genuine restart')
 finally:process.terminate();process.communicate(timeout=10)
