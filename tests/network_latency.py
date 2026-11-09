"""Regression: slow/idle HTTP connections must not pause authoritative simulation."""
import json,socket,sys,time,urllib.request
from urllib.parse import urlsplit
base=sys.argv[1];u=urlsplit(base)
def api(path,body=None,token=''):
 headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'}
 if token:headers['Authorization']='Bearer '+token
 req=urllib.request.Request(base+'/api/'+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
 with urllib.request.urlopen(req,timeout=.6) as r:return json.load(r)
s=api('session',{'name':'Slow socket QA','profession':'warrior'});held=[]
try:
 for data in [b'',b'GET /api/health HTTP/1.1\r\nHost: qa',b'POST /api/input HTTP/1.1\r\nHost: qa\r\nContent-Length: 999\r\n\r\n{']:
  c=socket.create_connection((u.hostname,u.port),timeout=1)
  if data:c.sendall(data)
  held.append(c)
 started=time.monotonic();api('health')
 assert time.monotonic()-started<.4,'Idle sockets delayed another request'
 api('input',{'sequence':1,'x':0,'z':0,'yaw':0,'jump':True},s['token'])
 a=api('state',token=s['token']);time.sleep(.22);b=api('state',token=s['token'])
 assert b['now']-a['now']>=.15,'Simulation stopped behind incomplete HTTP requests'
 assert b['player']['position']['y']>a['player']['position']['y']+.2,'Jump failed to advance while sockets idle'
 time.sleep(.7);c=api('state',token=s['token'])
 assert c['player']['grounded'],'Jump did not land during idle connections'
 slow=socket.create_connection((u.hostname,u.port),timeout=1)
 slow.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,1024)
 slow.sendall(b'GET /slow.bin HTTP/1.1\r\nHost: qa\r\n\r\n');held.append(slow);time.sleep(.1)
 started=time.monotonic();api('health')
 assert time.monotonic()-started<.4,'Slow download blocked HTTP service'
 before=api('state',token=s['token'])['now'];time.sleep(.2)
 assert api('state',token=s['token'])['now']-before>=.15,'Slow reader stopped simulation'
 print('NONBLOCKING IO OK: idle/partial sockets, jump rise/landing and slow-reader isolation')
finally:
 for c in held:c.close()
