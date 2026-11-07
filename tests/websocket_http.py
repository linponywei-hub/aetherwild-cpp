"""Real RFC6455 socket integration, isolated fixture only. No production credentials."""
import base64,json,os,socket,struct,sys,time,urllib.request
from urllib.parse import urlsplit
base=sys.argv[1];u=urlsplit(base);checks=0
def check(value):
 global checks
 checks+=1
 if not value:raise AssertionError('WebSocket integration failed')
def api(path,body=None):
 req=urllib.request.Request(base+'/api/'+path,data=None if body is None else json.dumps(body).encode(),headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'})
 with urllib.request.urlopen(req,timeout=2) as r:return json.load(r)
class WS:
 def __init__(self,origin=None):
  self.socket=socket.create_connection((u.hostname,u.port),timeout=2);self.buffer=b''
  host=f'{u.hostname}:{u.port}'
  self.socket.sendall(f'GET /api/socket HTTP/1.1\r\nHost: {host}\r\nOrigin: {origin or base}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n'.encode())
  while b'\r\n\r\n' not in self.buffer:self.buffer+=self.socket.recv(4096)
  self.header,self.buffer=self.buffer.split(b'\r\n\r\n',1)
 def send(self,message,opcode=1,final=True,masked=True):
  payload=message.encode() if isinstance(message,str) else message
  n=len(payload);head=bytes([(128 if final else 0)|opcode,(128 if masked else 0)|(n if n<126 else 126)])
  if n>=126:head+=struct.pack('!H',n)
  mask=os.urandom(4)
  self.socket.sendall(head+(mask+bytes(b^mask[i%4] for i,b in enumerate(payload)) if masked else payload))
 def json(self,obj):self.send(json.dumps(obj))
 def read(self):
  def need(n):
   while len(self.buffer)<n:
    data=self.socket.recv(65536)
    if not data:raise EOFError('socket closed')
    self.buffer+=data
  need(2);opcode=self.buffer[0]&15;n=self.buffer[1]&127;at=2
  if n==126:need(4);n=struct.unpack('!H',self.buffer[2:4])[0];at=4
  elif n==127:need(10);n=struct.unpack('!Q',self.buffer[2:10])[0];at=10
  need(at+n);data=self.buffer[at:at+n];self.buffer=self.buffer[at+n:]
  return opcode,json.loads(data) if opcode==1 else data
 def close(self):self.socket.close()
session=api('session',{'name':'WS QA','profession':'warrior'})
s=WS()
try:
 check(b'101 Switching Protocols' in s.header)
 check(b'Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=' in s.header)
 s.json({'channel':'auth','id':'auth','token':session['token']});op,state=s.read()
 check(op==1 and state['player']['id']==session['player']['id'])
 s.send('ping-check',opcode=9);check(s.read()==(10,b'ping-check'))
 if 'content' in state:
  # The content command router and realtime channel share one authoritative world.
  topic=state['content']['relations']['windchime_merchant']['topics'][0]
  body={'request':'ws-npc-once','type':'npc_chat','target':'windchime_merchant','topic':topic}
  s.json({'channel':'command','id':'npc1','body':body});result=s.read()[1]
  check(result['content']['relations']['windchime_merchant']['score']==1)
  check('_motion' in result and '_sync' in result)
  s.json({'channel':'command','id':'npc2','body':body})
  check(s.read()[1]['content']['relations']['windchime_merchant']['score']==1)
  new=api('session',{'name':'WS tank','profession':'aw_class_tank'});tank=WS()
  try:
   tank.json({'channel':'auth','id':'auth','token':new['token']});response=tank.read()[1]
   check(response['player']['profession']=='aw_class_tank')
   item=response['player']['inventory'][0]['id']
   tank.json({'channel':'command','id':'unequip','body':{'request':'ws-tank-unequip','type':'unequip','target':item}});check(tank.read()[1]['ok'])
   tank.json({'channel':'command','id':'store','body':{'request':'ws-tank-store','type':'store_item','target':item}});response=tank.read()[1]
   check(any(i['id']==item for i in response['player']['warehouseItems']))
  finally:tank.close()

 # Fragmented text frames are reassembled, including JSON larger than 125 bytes.
 message=json.dumps({'channel':'state','id':'fragmented-'+'x'*80})
 s.send(message[:20],final=False);s.send('between-fragments',opcode=9);check(s.read()==(10,b'between-fragments'));s.send(message[20:],opcode=0)
 check(s.read()[1]['_reply'].startswith('fragmented-'))
 request={'channel':'command','id':'c1','body':{'type':'buy','target':'potion','request':'ws-once'}}
 s.json(request);coins=s.read()[1]['player']['coins'];request['id']='c2';s.json(request)
 check(s.read()[1]['player']['coins']==coins)
 start=state['player']['position']['x']
 # Send continuous movement for 1s without reading replies: input cannot wait on acknowledgments.
 for seq in range(1,21):
  s.json({'channel':'input','id':f'i{seq}','body':{'sequence':seq,'x':1,'z':0,'yaw':0}});time.sleep(.05)
 s.json({'channel':'input','id':'stop','body':{'sequence':21,'x':0,'z':0,'yaw':0}})
 s.json({'channel':'state','id':'final'})
 while True:
  result=s.read()[1]
  if result.get('_reply')=='final':break
 check(result['player']['position']['x']-start>4)
 check(result['_motion']['sequence']==21)
 check(result['_sync']['sequence']>state['_sync']['sequence'])
 s.send('',opcode=8);check(s.read()[0]==8)
finally:s.close()
for origin in ['https://wrong.example']:
 bad=WS(origin)
 try:check(b'400 Bad Request' in bad.header)
 finally:bad.close()
for payload,masked in [({'channel':'state','id':'unauthorized'},True),({'channel':'auth','token':'invalid'},True),({'channel':'state'},False)]:
 bad=WS()
 try:
  bad.send(json.dumps(payload),masked=masked);op,response=bad.read()
  check((op==1 and response['ok'] is False) if masked else op==8)
 finally:bad.close()
for payload in [b'\xff',b'x'*16385]:
 bad=WS()
 try:
  bad.send(payload);check(bad.read()[0]==8)
 finally:bad.close()
print(f'WEBSOCKET OK: {checks} checks; RFC accept, auth, origin, masked/fragmented frames, ping/close, idempotency, pipelined movement')
