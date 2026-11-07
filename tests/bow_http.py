"""Isolated HTTP contract for server-timed draw/release and visual environment metadata."""
import pathlib,subprocess,tempfile,socket,time,json,urllib.request,urllib.error,sys,uuid,os
exe=str(pathlib.Path(sys.argv[1]).resolve());checks=0
with socket.socket() as sock:sock.bind(('127.0.0.1',0));port=sock.getsockname()[1]
base=f'http://127.0.0.1:{port}/api/'
def check(value):
 global checks
 checks+=1
 assert value,'bow check '+str(checks)
def request(path,body=None,token=''):
 headers={'Content-Type':'application/json','X-Aetherwild-Client':'1'}
 if token:headers['Authorization']='Bearer '+token
 r=urllib.request.Request(base+path,data=json.dumps(body).encode() if body is not None else None,headers=headers)
 try:
  with urllib.request.urlopen(r,timeout=15) as response:return response.status,json.load(response)
 except urllib.error.HTTPError as e:return e.code,json.load(e)
def command(token,kind,**extra):return request('command',{'type':kind,'request':uuid.uuid4().hex,**extra},token)
with tempfile.TemporaryDirectory(prefix='aw_bow_http_') as folder:
 env=os.environ.copy()
 for k in ['AETHERWILD_PUBLIC','AETHERWILD_DATA_DIR','GAME_OWNER_CODE','GAME_INVITE_CODE']:env.pop(k,None)
 p=subprocess.Popen([exe,'--root',folder,'--port',str(port)],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
 try:
  for _ in range(600):
   if p.poll() is not None:raise RuntimeError(p.communicate()[1].decode())
   try:
    if request('health')[0]==200:break
   except OSError:pass
   time.sleep(.05)
  a=request('session',{'name':'Bow HTTP','profession':'ranger'})[1];b=request('session',{'name':'Dagger HTTP','profession':'aw_class_assassin'})[1];ta,tb=a['token'],b['token']
  snap=request('state',token=ta)[1];check(snap['environment']['cycleSeconds']==1800);check(snap['bow']['fullDrawSeconds']==1.2)
  other=request('state',token=tb)[1];check(abs(other['environment']['unixSeconds']-snap['environment']['unixSeconds'])<20)
  check(command(tb,'bow_draw')[0]==400);check(command(ta,'bow_release',charge=999999)[0]==400);check(command(ta,'attack')[0]==400)
  catalog=request('catalog')[1];entrance=next(d['id'] for d in catalog['dungeons'] if d['region']=='windchime');check(command(ta,'admin_entrance',target=entrance)[0]==200)
  check(command(ta,'bow_draw')[0]==200);cancel=command(ta,'bow_cancel')[1];check(not cancel['bow']['drawing']);check(command(ta,'bow_release')[0]==400)
  before=request('state',token=ta)[1]['player']['materials']['woodarrow'];check(command(ta,'bow_draw')[0]==200)
  receipt={'type':'bow_release','request':'one-shot-only','charge':999999,'drawSeconds':999999}
  code,shot=request('command',receipt,ta);check(code==200);check(shot['player']['materials']['woodarrow']==before-1)
  releases=[e for e in shot['events'] if e['type']=='ArrowReleased'];check(len(releases)==1);check(.55<=releases[-1]['value']<=1.6)
  again=request('command',receipt,ta)[1];check(again['player']['materials']['woodarrow']==before-1);check(not again['bow']['drawing'])
  time.sleep(.85);check(command(ta,'bow_draw')[0]==200);time.sleep(1.25);code,full=command(ta,'bow_release');check(code==200)
  last=[e for e in full['events'] if e['type']=='ArrowReleased'][-1];check(last['value']>=1.55)
  print('BOW HTTP OK:',checks,'checks; authority, forged charge, cancel, full draw and duplicate release')
 finally:p.terminate();p.communicate(timeout=15)
