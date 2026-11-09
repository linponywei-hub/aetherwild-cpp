#!/usr/bin/env python3
"""Install verified Quaternius GLBs from Aetherwild_Nature_GLB_23_models.zip.
Usage: python3 tools/install_nature_glbs.py /path/to/Aetherwild_Nature_GLB_23_models.zip
"""
from pathlib import Path
import sys, zipfile, struct

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/"web/assets/models/environment/trees"
NAMES=[*(f"CommonTree_{i}" for i in range(1,6)),*(f"Pine_{i}" for i in range(1,6)),*(f"TwistedTree_{i}" for i in range(1,6)),*(f"DeadTree_{i}" for i in range(1,6))]
ROCKS=[f"Rock_Medium_{i}" for i in range(1,4)]

def main(path):
    with zipfile.ZipFile(path) as archive:
        members={Path(n).name:n for n in archive.namelist() if n.endswith(".glb")}
        missing=[n for n in NAMES if n+".glb" not in members]
        if missing: raise SystemExit("Missing required tree GLBs: "+", ".join(missing))
        for name in NAMES+ROCKS:
            if name+".glb" not in members: continue
            data=archive.read(members[name+".glb"])
            if len(data)<20 or data[:4]!=b"glTF" or struct.unpack_from("<I",data,4)[0]!=2 or struct.unpack_from("<I",data,8)[0]!=len(data):
                raise SystemExit("Invalid GLB: "+name)
            DEST.mkdir(parents=True,exist_ok=True)
            (DEST/(name+".glb")).write_bytes(data)
            print(f"Installed {name}.glb ({len(data):,} bytes)")
    print("Done. Commit the GLB files to the PR branch before merging.")

if __name__=="__main__":
    if len(sys.argv)!=2: raise SystemExit("Usage: python3 tools/install_nature_glbs.py ZIP")
    main(sys.argv[1])
