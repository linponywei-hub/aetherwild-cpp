"""Isolated native HTTP integration fixture; does not alter workstation player data."""
import json,os,socket,subprocess,sys,tempfile,time,urllib.request
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exe=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build'/('aetherwild_workstation.exe' if os.name=='nt' else 'aetherwild_workstation')
with socket.socket() as s:
 s.bind(('127.0.0.1',0));port=s.getsockname()[1]
base='http://127.0.0.1:'+str(port)
with tempfile.TemporaryDirectory(prefix='aw_http_') as folder:
 def launch():
  p=subprocess.Popen([str(exe),'--root',folder,'--port',str(port)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
  for _ in range(100):
   if p.poll() is not None:raise RuntimeError(p.communicate()[1].decode())
   try:
    with urllib.request.urlopen(base+'/api/health',timeout=.5) as r:
     if json.load(r)['ok']:return p
   except Exception:time.sleep(.05)
  p.terminate();raise RuntimeError('Native HTTP fixture not ready')
 (Path(folder)/'web').mkdir()
 (Path(folder)/'web/slow.bin').write_bytes(b'x'*(8*1024*1024))
 p=launch()
 try:
  subprocess.run([sys.executable,str(root/'tests/network_latency.py'),base],check=True)
  subprocess.run([sys.executable,str(root/'tests/workstation_http.py'),base],check=True)
 finally:p.terminate();p.communicate(timeout=5)
 data=json.loads((Path(folder)/'runtime-data/world-v2.json').read_text())
 profile=data['profiles'][0];saved=next(p for p in data['players'] if p['id']==profile['player']);expected=saved['coins'];token=profile['token']
 p=launch()
 try:
  req=urllib.request.Request(base+'/api/state',headers={'Authorization':'Bearer '+token})
  with urllib.request.urlopen(req,timeout=5) as r:restored=json.load(r)
  if restored['player']['coins']!=expected:raise AssertionError('Restart changed wallet')
  if restored['player']['warehouseItems']!=saved['warehouseItems']:raise AssertionError('Restart changed equipment warehouse')
  print('HTTP RESTART OK: durable world, equipment warehouse and session restored')
 finally:p.terminate();p.communicate(timeout=5)
