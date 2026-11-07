"""Embed the reference terrain authoring model independently of large content packs."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
text=json.dumps(json.loads((root/'data/atlas_surface.json').read_text()),ensure_ascii=False,separators=(',',':'))
(root/'src/atlas_data.cpp').write_text('#include "aetherwild/content.hpp"\nnamespace aw {const Json& atlasSurface(){static const Json data=Json::parse(R"ATLAS('+text+')ATLAS");return data;}}\n')
