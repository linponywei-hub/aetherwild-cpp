"""Isolated integration: urban metadata, public weapons and five persisted skill slots."""
import json,os,pathlib,socket,subprocess,sys,tempfile,time,urllib.request,urllib.error,uuid
exe=str(pathlib.Path(sys.argv[1]).resolve());checks=0
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
base=f'http://127.0.0.1:{port}/api/'
def check(value):
 global checks
 checks+=1
 assert value,'city/skills HTTP check '+str(checks)
def request(path,body=None,token=''):
 headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'}
 if token:headers['Authorization']='Bearer '+token
 req=urllib.request.Request(base+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
 try:
  with urllib.request.urlopen(req,timeout=15) as r:return r.status,json.load(r)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
def command(token,kind,**data):return request('command',{'type':kind,'request':uuid.uuid4().hex,**data},token)
with tempfile.TemporaryDirectory(prefix='aw_city_skills_') as folder:
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
  catalog=request('catalog')[1];cities=[r for r in catalog['regions'] if not r['wilderness']];check(len(cities)>=10);check(all(0<r['urbanRadius']<=r['radius'] for r in cities))
  check(any(b['style']=='aw_crownreach|palace' for b in catalog['buildings']));check(any(b['style']=='aw_crownreach|wall' for b in catalog['buildings']));check(sum(b['style'].endswith('|lamp') for b in catalog['buildings'])>60)
  a=request('session',{'name':'城市技能甲','profession':'warrior'})[1];b=request('session',{'name':'城市技能乙','profession':'ranger'})[1];ta,tb=a['token'],b['token']
  snap=request('state',token=ta)[1];slots=snap['player']['skillSlots'];first=slots[0];check(len(slots)==5 and bool(first));other=request('state',token=tb)[1];check(len(other['player']['skillSlots'])==5)
  visible=next(v for v in snap['players'] if v['id']==b['player']['id']);check(visible['name']=='城市技能乙');check(visible['weaponType']=='bow');check(visible['weapon']['type']=='bow');check('inventory' not in visible and 'warehouseItems' not in visible and 'skillSlots' not in visible)
  atlas=request('admin/map',token=ta)[1];check(all('urbanRadius' in r and 'city' in r for r in atlas['current']));check(request('admin/map',token=tb)[0]==400)
  check(command(ta,'skill_slot',slot=5,skill=first)[0]==400);check(command(ta,'skill_slot',slot=1,skill=first)[0]==400);check(command(ta,'skill_slot',slot=0,skill='forged')[0]==400)
  check(command(ta,'skill_slot',slot=0,skill=other['player']['skillSlots'][0])[0]==400)
  check(command(ta,'skill_slot',slot=0,skill='')[0]==200);code,changed=command(ta,'skill_slot',slot=1,skill=first,target=b['player']['id']);check(code==200);check(changed['player']['skillSlots'][1]==first and changed['player']['skillSlots'][0]=='')
  check(request('state',token=tb)[1]['player']['skillSlots']==other['player']['skillSlots'])
  weapon=b['player']['equipped']['0'];check(command(tb,'unequip',target=weapon)[0]==200);near=request('state',token=ta)[1];visible=next(v for v in near['players'] if v['id']==b['player']['id']);check(visible['weaponType']=='unarmed' and visible['weapon'] is None)
  check(command(ta,'save')[0]==200)
 finally:p.terminate();p.communicate(timeout=15)
 p=launch()
 try:
  snap=request('state',token=ta)[1];check(snap['player']['skillSlots'][0]=='');check(snap['player']['skillSlots'][1]==first);check(request('state',token=tb)[1]['player']['skillSlots']==other['player']['skillSlots'])
 finally:p.terminate();p.communicate(timeout=15)
 print('CITY SKILLS HTTP OK:',checks,'checks including real restart and public appearance privacy')
