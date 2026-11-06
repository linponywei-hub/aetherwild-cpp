"""Launches the real C++ workstation; no Python or Node gameplay server."""
import os,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
exe=root/'build'/('aetherwild_workstation.exe' if os.name=='nt' else 'aetherwild_workstation')
if not exe.exists():subprocess.run([sys.executable,str(root/'tools/build.py')],check=True,cwd=root)
os.chdir(root)
os.execv(str(exe),[str(exe),'--root',str(root)])
