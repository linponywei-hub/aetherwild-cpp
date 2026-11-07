"""Public-mode access, first-visitor protection and graceful checkpoint tests.
All access codes below are disposable, generated fixture values, never production secrets.
"""
import json,os,secrets,socket,subprocess,sys,tempfile,time,urllib.request,urllib.error
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build'/('aetherwild_workstation.exe' if os.name=='nt' else 'aetherwild_workstation')
checks=0
def check(condition):
 global checks
 checks+=1
 if not condition:raise AssertionError('Cloud access check failed')
with tempfile.TemporaryDirectory(prefix='aw_cloud_access_') as folder:
 env=os.environ.copy();env['AETHERWILD_PUBLIC']='1';env.pop('GAME_OWNER_CODE',None);env.pop('GAME_INVITE_CODE',None)
 failed=subprocess.run([str(exe),'--root',folder],env=env,capture_output=True)
 check(failed.returncode!=0)
 owner=secrets.token_urlsafe(32);invite=secrets.token_urlsafe(32)
 env['GAME_OWNER_CODE']=owner;env['GAME_INVITE_CODE']=invite
 with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
 base='http://127.0.0.1:'+str(port)
 def request(path,body=None,token=''):
  headers={'X-Aetherwild-Client':'1','Content-Type':'application/json'}
  if token:headers['Authorization']='Bearer '+token
  req=urllib.request.Request(base+'/api/'+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
  try:
   with urllib.request.urlopen(req,timeout=4) as r:return r.status,json.load(r)
  except urllib.error.HTTPError as e:return e.code,json.load(e)
 p=subprocess.Popen([str(exe),'--root',folder,'--port',str(port)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 try:
  for _ in range(600):
   if p.poll() is not None:raise RuntimeError("Cloud fixture exited before readiness")
   try:
    if request('health')[0]==200:break
   except Exception:time.sleep(.05)
  else:raise RuntimeError('Cloud fixture did not start within the 30-second readiness window')
  health=request('health')[1];check(health['inviteRequired'] and health['cloudTest'])
  check(request('session',{'name':'stranger','profession':'warrior'})[0]==400)
  check(request('session',{'name':'wrong','profession':'warrior','ownerCode':'x'*64})[0]==400)
  visitor=request('session',{'name':'first visitor','profession':'warrior','inviteCode':invite})[1]
  check(visitor['role']=='player')
  host=request('session',{'name':'owner','profession':'warrior','ownerCode':owner})[1]
  check(host['role']=='owner')
  bad=request('command',{'type':'admin_supply','request':'forbidden'},visitor['token'])
  check(bad[0]==400)
  good=request('command',{'type':'admin_supply','request':'allowed'},host['token'])
  check(good[0]==200 and good[1]['player']['level']==60)
  check(owner not in json.dumps(good[1]) and invite not in json.dumps(good[1]))
 finally:
  p.terminate();stdout,stderr=p.communicate(timeout=6)
 check(p.returncode==0)
 saved=json.loads((Path(folder)/'runtime-data/world-v2.json').read_text())
 check(len(saved['players'])==2)
 check(owner not in json.dumps(saved) and invite not in json.dumps(saved))
print('CLOUD ACCESS OK:',checks,'checks; first visitor cannot take ownership; secrets not saved; graceful shutdown')
