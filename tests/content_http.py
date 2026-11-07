"""Native content commands and restart; isolated test world only."""
import json,os,socket,subprocess,sys,tempfile,time,urllib.request,urllib.error,uuid
from pathlib import Path
root=Path(__file__).resolve().parents[1];exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build/aetherwild_workstation'
checks=0
with socket.socket() as sock:sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
base=f'http://127.0.0.1:{port}/api/'
def check(c):
 global checks
 checks+=1
 if not c:raise AssertionError('content HTTP check '+str(checks))
def req(path,data=None,token=''):
 headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'}
 if token:headers['Authorization']='Bearer '+token
 r=urllib.request.Request(base+path,headers=headers,data=None if data is None else json.dumps(data).encode())
 try:
  with urllib.request.urlopen(r,timeout=15) as f:return f.status,json.load(f)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
def cmd(token,kind,**extra):return req('command',{'request':uuid.uuid4().hex,'type':kind,**extra},token)
with tempfile.TemporaryDirectory(prefix='aw_content_http_') as folder:
 def launch():
  env=os.environ.copy()
  for key in ['AETHERWILD_PUBLIC','GAME_OWNER_CODE','GAME_INVITE_CODE','AETHERWILD_DATA_DIR']:env.pop(key,None)
  p=subprocess.Popen([str(exe),'--root',folder,'--port',str(port)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
  for _ in range(600):
   if p.poll() is not None:raise RuntimeError(p.communicate()[1].decode())
   try:
    if req('health')[0]==200:return p
   except OSError:pass
   time.sleep(.05)
  p.terminate();raise RuntimeError('content server did not start')
 p=launch()
 try:
  code,catalog=req('catalog');check(code==200);check(len(catalog['contentIndex']['monsters'])==441);check(len(catalog['contentIndex']['skills'])==1681);check(len([c for c in catalog['classes'] if c['level']==1 and not c['parent']])==10)
  a=req('session',{'name':'內容HTTP A','profession':'aw_class_tank'})[1];b=req('session',{'name':'內容HTTP B','profession':'aw_class_assassin'})[1];ta,tb=a['token'],b['token']
  check(a['player']['profession']=='aw_class_tank');check(b['player']['profession']=='aw_class_assassin')
  snap=req('state',token=ta)[1];check(snap['content']['relations']['windchime_merchant']['score']==0);topic=snap['content']['relations']['windchime_merchant']['topics'][0]
  receipt={'request':'one-chat','type':'npc_chat','target':'windchime_merchant','topic':topic};check(req('command',receipt,ta)[0]==200);again=req('command',receipt,ta)[1];check(again['content']['relations']['windchime_merchant']['score']==1)
  check(req('state',token=tb)[1]['content']['relations']['windchime_merchant']['score']==0)
  check(cmd(ta,'npc_chat',target='windchime_merchant',topic='forged',score=100)[0]==400)
  check(cmd(tb,'content_release',release=2)[0]==400);check(cmd(ta,'npc_gift',target='windchime_merchant')[0]==400)
  # A new-class weapon must survive warehouse storage and the expanded-world migration.
  stored_id=a['player']['inventory'][0]['id']
  check(cmd(ta,'unequip',target=stored_id)[0]==200)
  code,stored=cmd(ta,'store_item',target=stored_id);check(code==200);check(any(i['id']==stored_id for i in stored['player']['warehouseItems']))
  check(all(i['id']!=stored_id for i in req('state',token=tb)[1]['player']['warehouseItems']))
  seq=100
  for _ in range(3):seq+=1;req('input',{'sequence':seq,'x':1,'z':0,'yaw':0},ta);time.sleep(.2)
  seq+=1;req('input',{'sequence':seq,'x':0,'z':0,'yaw':0},ta)
  code,hit=cmd(ta,'npc_attack',target='windchime_merchant');check(code==200);check(hit['content']['relations']['windchime_merchant']['restricted']);check(hit['content']['relations']['windchime_merchant']['score']==-24)
  check(req('state',token=tb)[1]['content']['relations']['windchime_merchant']['score']==0)
  code,wide=cmd(ta,'expansion_start');check(code==200);check(wide['content']['expanded']);check(wide['content']['worldLimit']==32000);check(wide['content']['release']==1)
  check(cmd(ta,'admin_travel',target='aw_crownreach')[0]==400);check(cmd(ta,'content_release',release=4)[0]==400)
  check(cmd(ta,'content_release',release=2)[0]==200);code,royal=cmd(ta,'admin_travel',target='aw_crownreach');check(code==200);check(abs(royal['player']['position']['x'])<1 and abs(royal['player']['position']['z'])<1)
  check('terrain' in req('catalog')[1]);check(cmd(ta,'save')[0]==200)
 finally:p.terminate();p.communicate(timeout=10)
 p=launch()
 try:
  code,restored=req('state',token=ta);check(code==200);check(restored['content']['expanded']);check(restored['content']['release']==2);check(restored['player']['profession']=='aw_class_tank')
  check(any(i['id']==stored_id for i in restored['player']['warehouseItems']))
  other=req('state',token=tb)[1];check(other['player']['profession']=='aw_class_assassin');check(other['content']['relations']['windchime_merchant']['score']==0)
 finally:p.terminate();p.communicate(timeout=10)
print('CONTENT HTTP OK:',checks,'checks including actual process restart')
