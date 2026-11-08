"""Verify moved web entry points and panel projections with an isolated native world."""
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request

root = Path(__file__).resolve().parents[1]
exe = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'build/aetherwild_workstation'
with socket.socket() as sock:
    sock.bind(('127.0.0.1', 0))
    port = sock.getsockname()[1]
base = f'http://127.0.0.1:{port}'
with tempfile.TemporaryDirectory(prefix='aw_modules_') as folder:
    folder = Path(folder)
    for path in (root/'web').rglob('*'):
        if path.is_file() and path.suffix in ('.js','.html','.css'):
            target = folder/'web'/path.relative_to(root/'web')
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(path,target)
    env = dict(os.environ, AETHERWILD_PUBLIC='0', AETHERWILD_DATA_DIR=str(folder/'runtime-data'))
    env.pop('GAME_OWNER_CODE',None)
    env.pop('GAME_INVITE_CODE',None)
    process = subprocess.Popen([str(exe),'--root',str(folder),'--port',str(port)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    def request(path,body=None,token=''):
        headers={'X-Aetherwild-Client':'1','Content-Type':'application/json'}
        if token:headers['Authorization']='Bearer '+token
        req=urllib.request.Request(base+path,data=None if body is None else json.dumps(body).encode(),headers=headers)
        with urllib.request.urlopen(req,timeout=10) as response:return response.read()
    try:
        for _ in range(200):
            if process.poll() is not None:raise RuntimeError(process.communicate()[1].decode())
            try:
                request('/api/health')
                break
            except OSError:time.sleep(.05)
        else:raise RuntimeError('Module HTTP fixture did not start')
        checks=0
        for path in (folder/'web').rglob('*'):
            if path.is_file():
                assert request('/'+path.relative_to(folder/'web').as_posix())==path.read_bytes(),path
                checks+=1
        catalog=json.loads(request('/api/catalog'))
        session=json.loads(request('/api/session',{'name':'模組驗證','profession':'warrior'}))
        state=json.loads(request('/api/command',{'type':'admin_supply','request':'module-fixture'},session['token']))
        fixture=folder/'panel-fixture.json'
        fixture.write_text(json.dumps({'catalog':catalog,'state':state}))
        subprocess.run(['node',str(root/'tests/panel_projection.mjs'),str(fixture)],check=True)
        print('MODULE HTTP OK:',checks,'entry points, scripts and styles served with identical bytes')
    finally:
        process.terminate()
        process.communicate(timeout=10)
