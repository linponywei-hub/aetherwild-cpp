#!/bin/sh
# Run from repository root after downloading Aetherwild_Eileen_Bound.zip.
set -eu
ZIP="${1:-$HOME/Downloads/Aetherwild_Eileen_Bound.zip}"
test -f "$ZIP" || { echo "Missing model ZIP: $ZIP" >&2; exit 1; }
test -d web || { echo "Run from aetherwild-cpp repository root" >&2; exit 1; }
mkdir -p web/assets/models/characters
unzip -p "$ZIP" web/assets/models/characters/TRIPO-001.glb > web/assets/models/characters/TRIPO-001.glb
python3 - <<'PY'
import json,struct
p='web/assets/models/characters/TRIPO-001.glb'
with open(p,'rb') as f: data=f.read()
assert data[:4]==b'glTF' and struct.unpack_from('<I',data,8)[0]==len(data)
length=struct.unpack_from('<I',data,12)[0]
j=json.loads(data[20:20+length])
assert j.get('skins') and len(j.get('animations',[]))>=4
print('Validated:',len(j['skins']),'skin(s),',len(j['animations']),'animations')
PY
echo "Model installed locally. Commit and push this GLB to the test branch before browser testing."
