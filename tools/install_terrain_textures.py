#!/usr/bin/env python3
"""Vendor pinned third-party terrain images at image-build time.

Uses git blob hashes to verify source bytes; never silently accepts changed
upstream assets. Fails open to procedural terrain when the network is down.
"""
import hashlib
from pathlib import Path
import urllib.request

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/"web/assets/terrain"
ASSETS={
 "grass.jpg":("terrain/grasslight-big.jpg","ed6c7f85a33abbed6f41e6e08585a594f0bd5fae"),
 "stone.jpg":("brick_diffuse.jpg","2312abd7d196b9ede4577fcb08269a0385ec7a2f"),
 "lava.jpg":("lava/lavatile.jpg","e9ac640ee9515b18ba2fa204cd05e4bbca0e8542"),
}
BASE="https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/"
def install():
 DEST.mkdir(parents=True,exist_ok=True)
 for name,(source,expected) in ASSETS.items():
  path=DEST/name
  try:
   if path.exists():data=path.read_bytes()
   else:
    with urllib.request.urlopen(BASE+source,timeout=25) as response:
     data=response.read(4_000_001)
   digest=hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
   if digest!=expected:raise ValueError("upstream image SHA mismatch")
   path.write_bytes(data)
   print("Installed verified terrain asset",name,len(data))
  except (OSError,ValueError) as exc:
   print("Terrain asset unavailable:",name,str(exc))
   if path.exists():path.unlink()
if __name__=="__main__":install()
