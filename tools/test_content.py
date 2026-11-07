import pathlib,subprocess,concurrent.futures,sys
P=pathlib.Path(__file__).resolve().parents[1];build=P/('build-asan' if '--sanitize' in sys.argv else 'build');build.mkdir(exist_ok=True)
flags=['-std=c++20','-I'+str(P/'include'),'-Wall','-Wextra','-Wpedantic','-g']+(['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else ['-O1'])
src=[p for p in (P/'src').glob('*.cpp') if p.name!='main.cpp']
def obj(p):
 out=build/(p.stem+'.o')
 if not out.exists() or out.stat().st_mtime<p.stat().st_mtime or any(h.stat().st_mtime>out.stat().st_mtime for h in (P/'include/aetherwild').glob('*.hpp')):
  r=subprocess.run(['g++',*flags,'-c',str(p),'-o',str(out)],capture_output=True,text=True)
  if r.returncode:print(r.stderr);raise RuntimeError(p.name)
  if r.stderr:print(r.stderr[:2200])
 return str(out)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as e:objs=list(e.map(obj,src))
for name,file in [('aetherwild_tests','tests/core_tests.cpp'),('aetherwild_runtime_tests','tests/runtime_tests.cpp'),('aetherwild_content_tests','tests/content_tests.cpp'),('aetherwild_workstation','apps/workstation.cpp')]:
 if not (P/file).exists():continue
 r=subprocess.run(['g++',*flags,str(P/file),*objs,'-o',str(build/name)],capture_output=True,text=True)
 if r.returncode:print(r.stderr);raise RuntimeError(name)
 print('BUILT',name,flush=True)

for name in ['aetherwild_tests','aetherwild_runtime_tests','aetherwild_content_tests']:
 subprocess.run([str(build/name)],check=True,cwd=P)
