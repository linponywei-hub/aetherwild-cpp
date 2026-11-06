"""Integration checks against an already running private test workstation."""
import json,math,time,urllib.request,urllib.error,uuid,sys
base=sys.argv[1] if len(sys.argv)>1 else 'http://127.0.0.1:4173'
checks=0
def check(c):
 global checks
 checks+=1
 if not c:raise AssertionError('HTTP integration check failed')
def request(path,body=None,token='',header=True):
 headers={'Content-Type':'application/json'}
 if header:headers['X-Aetherwild-Client']='1'
 if token:headers['Authorization']='Bearer '+token
 req=urllib.request.Request(base+'/api/'+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
 try:
  with urllib.request.urlopen(req,timeout=10) as r:return r.status,json.load(r)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
def command(session,kind,**extra):
 return request('command',{'request':uuid.uuid4().hex,'type':kind,**extra},session['token'])
check(request('health')[1]['engine']=='C++20 native')
check(request('state')[0]==401)
check(request('session',{'name':'blocked','profession':'warrior'},header=False)[0]==400)
a=request('session',{'name':'QA 連線A','profession':'warrior'})[1]
b=request('session',{'name':'QA 連線B','profession':'ranger'})[1]
check('token' in a and 'token' in b)
rid=uuid.uuid4().hex
code,r=request('command',{'request':rid,'type':'buy','target':'potion'},a['token']);check(code==200)
coins=r['player']['coins'];check(coins==40)
r2=request('command',{'request':rid,'type':'buy','target':'potion'},a['token'])[1]
check(r2['player']['coins']==coins and r2['player']['materials']['potion']==4)
check(command(b,'admin_supply')[0]==400)
party=command(a,'party_create')[1]['party']['id']
check(command(a,'party_invite',target=b['player']['id'])[0]==200)
check(command(b,'party_join',target=party)[1]['party']['id']==party)
# Equipment warehouse uses unique instances, not catalog item codes.
item=a['player']['inventory'][0]['id']
check(command(a,'store_item',target=item)[0]==400)
check(command(a,'unequip',target=item)[0]==200)
rid=uuid.uuid4().hex
body={'request':rid,'type':'store_item','target':item}
code,stored=request('command',body,a['token'])
check(code==200 and stored['player']['warehouseItems'][0]['id']==item)
check(request('command',body,a['token'])[1]['player']['warehouseItems']==stored['player']['warehouseItems'])
check(command(a,'equip',target=item)[0]==400)
check(command(b,'withdraw_item',target=item)[0]==400)
check(command(a,'withdraw_item',target=item)[1]['player']['warehouseItems']==[])
check(command(a,'store_item',target=item)[0]==200)
start=request('state',token=a['token'])[1]['player']['position']
check(request('input',{'sequence':1,'x':1,'z':1,'yaw':0},a['token'])[0]==200)
time.sleep(.25)
end=request('state',token=a['token'])[1]['player']['position']
length=math.dist([start[k] for k in ['x','y','z']],[end[k] for k in ['x','y','z']])
check(0<length<2)
check(request('input',{'sequence':2,'x':100,'z':0,'yaw':0},a['token'])[0]==400)
check(command(a,'save')[0]==200)
snapshot=request('state',token=b['token'])[1]
check(any(p['id']==a['player']['id'] for p in snapshot['players']))
check('profiles' not in snapshot and 'token' not in snapshot)
check(command(a,'party_leave')[0]==200)
check(command(b,'party_leave')[0]==200)
print('HTTP WORKSTATION OK:',checks,'checks; native engine, identity, idempotency, multiplayer, movement, save')
