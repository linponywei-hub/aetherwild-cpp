"""Compatibility entry point; the shared builder runs core/runtime/content tests."""
import subprocess
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, str(root / 'tools/build.py'), *sys.argv[1:]], check=True, cwd=root)
