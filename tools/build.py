"""Fallback build for systems with GCC but no CMake. Python 3.10+, g++ 11+."""
import os,subprocess,sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
root=Path(__file__).resolve().parents[1]
build=root/('build-sanitize' if '--sanitize' in sys.argv else 'build');build.mkdir(exist_ok=True)
flags=['-std=c++20','-Wall','-Wextra','-Wpedantic','-I'+str(root/'include')]
flags+=['-g1','-O0','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else ['-O2']
compiler=os.environ.get('CXX','g++')
sources=sorted((root/'src').rglob('*.cpp'))
if not sources:raise RuntimeError('No game module sources found')
def compile_one(p):
    out=(build/'objects'/p.relative_to(root/'src')).with_suffix('.o')
    out.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run([compiler,*flags,'-c',str(p),'-o',str(out)],check=True)
    return str(out)
jobs=max(1,min(4,int(os.environ.get('AW_BUILD_JOBS','2' if '--sanitize' in sys.argv else '4'))))
with ThreadPoolExecutor(max_workers=jobs) as pool:
    objects=list(pool.map(compile_one,sources))
for name,p in [('aetherwild',root/'apps/cli.cpp'),('aetherwild_tests',root/'tests/core_tests.cpp'),('aetherwild_runtime_tests',root/'tests/runtime_tests.cpp'),('aetherwild_content_tests',root/'tests/content_tests.cpp'),('aetherwild_workstation',root/'apps/workstation.cpp')]:
    output=build/(name+('.exe' if os.name=='nt' else ''))
    subprocess.run([compiler,*flags,str(p),*objects,*(['-lws2_32'] if os.name=='nt' and name=='aetherwild_workstation' else []),'-o',str(output)],check=True)
subprocess.run([str(build/('aetherwild_tests.exe' if os.name=='nt' else 'aetherwild_tests'))],check=True,cwd=build)

subprocess.run([str(build/('aetherwild_runtime_tests.exe' if os.name=='nt' else 'aetherwild_runtime_tests'))],check=True,cwd=build)

subprocess.run([str(build/('aetherwild_content_tests.exe' if os.name=='nt' else 'aetherwild_content_tests'))],check=True,cwd=build)
