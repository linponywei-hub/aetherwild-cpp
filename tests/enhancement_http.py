"""Isolated integration: blacksmith enhancement, transactions and real restart."""
import json,os,pathlib,socket,subprocess,sys,tempfile,time,urllib.request,urllib.error,uuid
exe=str(pathlib.Path(sys.argv[1]).resolve());checks=0
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
base=f'http://127.0.0.1:{port}/api/'
def check(value):
 global checks
 checks+=1
 assert value,'enhancement HTTP check '+str(checks)
def request(path,body=None,token=''):
 headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'}
 if token:headers['Authorization']='Bearer '+token
 req=urllib.request.Request(base+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
 try:
  with urllib.request.urlopen(req,timeout=15) as r:return r.status,json.load(r)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
def command(token,kind,**data):return request('command',{'type':kind,'request':uuid.uuid4().hex,**data},token)
with tempfile.TemporaryDirectory(prefix='aw_forge_') as folder:
 env=os.environ.copy()
 for key in ['AETHERWILD_PUBLIC','AETHERWILD_DATA_DIR','GAME_OWNER_CODE','GAME_INVITE_CODE']:env.pop(key,None)
 def launch():
  p=subprocess.Popen([exe,'--root',folder,'--port',str(port)],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
  for _ in range(600):
   if p.poll() is not None:raise RuntimeError(p.communicate()[1].decode())
   try:
    if request('health')[0]==200:return p
   except OSError:pass
   time.sleep(.05)
  p.terminate();p.wait();raise RuntimeError('fixture not ready')
 p=launch()
 try:
  a=request('session',{'name':'鍛造驗收','profession':'warrior'})[1];b=request('session',{'name':'其他玩家','profession':'ranger'})[1];ta,tb=a['token'],b['token'];aid=a['player']['id'];item=a['player']['equipped']['0'];smith='windchime_smith'
  snap=command(ta,'admin_supply')[1];check(snap['enhancement']['npc']==smith);q=snap['enhancement']['quotes'][item];check(q['coins']==220 and q['ore']==2 and q['chance']==1);before=snap['player'];base_attack=snap['equipmentStats'][item]['stats']['attack']
  for extra in [{'npc':'windchime_merchant','expected':0},{'npc':'cinder_smith','expected':0},{'npc':smith,'expected':1},{'npc':smith,'expected':0.5}]:
   check(command(ta,'enhance',target=item,**extra)[0]==400)
  check(command(ta,'enhance',target=item,npc=smith)[0]==400)
  check(command(tb,'enhance',target=item,npc=smith,expected=0)[0]==400)
  same=request('state',token=ta)[1];check(same['player']['coins']==before['coins'] and same['player']['materials']['ore']==before['materials']['ore'])
  for level in range(6):
   prior=request('state',token=ta)[1];q=prior['enhancement']['quotes'][item];body={'type':'enhance','target':item,'npc':smith,'expected':level,'request':'enhance-'+str(level),'chance':1,'coins':0}
   code,snap=request('command',body,ta);check(code==200);gear=next(i for i in snap['player']['inventory'] if i['id']==item);check(gear['upgrade']==level+1);check(snap['player']['coins']==prior['player']['coins']-q['coins']);check(snap['player']['materials']['ore']==prior['player']['materials']['ore']-2)
   code,duplicate=request('command',body,ta);check(code==200 and duplicate['player']['coins']==snap['player']['coins']);check(next(i for i in duplicate['player']['inventory'] if i['id']==item)['upgrade']==level+1)
  check(snap['equipmentStats'][item]['stats']['attack']>base_attack);check(snap['enhancement']['quotes'][item]['chance']==.75);coins=snap['player']['coins'];check(command(ta,'enhance',target=item,npc=smith,expected=6)[0]==400)
  observer=request('state',token=tb)[1];visible=next(v for v in observer['players'] if v['id']==aid);check(visible['weapon']['upgrade']==6);check(item not in observer['enhancement']['quotes'] and item not in observer['equipmentStats']);check('inventory' not in visible)
  check(command(ta,'save')[0]==200)
 finally:p.terminate();p.communicate(timeout=15)
 p=launch()
 try:
  restored=request('state',token=ta)[1];check(next(i for i in restored['player']['inventory'] if i['id']==item)['upgrade']==6);check(restored['player']['coins']==coins);check(request('command',body,ta)[1]['player']['coins']==coins)
 finally:p.terminate();p.communicate(timeout=15)
 print('ENHANCEMENT HTTP OK:',checks,'checks including retries, privacy and true restart')
