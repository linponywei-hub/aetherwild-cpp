#!/usr/bin/env python3
"""Dependency-free structural smoke test for the first humanoid GLB."""
import json
import struct
import sys
from pathlib import Path

COMPONENT_BYTES = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
ITEM_SIZES = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def check(path: Path):
    data = path.read_bytes()
    assert len(data) >= 20, "File too short"
    magic, version, total = struct.unpack_from("<III", data)
    assert magic == 0x46546C67 and version == 2 and total == len(data), "Invalid GLB header"
    offset, doc, binary = 12, None, None
    while offset < len(data):
        assert offset + 8 <= len(data), "Truncated chunk header"
        size, kind = struct.unpack_from("<II", data, offset)
        offset += 8
        assert offset + size <= len(data), "Chunk exceeds file"
        if kind == 0x4E4F534A:
            doc = json.loads(data[offset:offset + size])
        elif kind == 0x004E4942:
            binary = data[offset:offset + size]
        offset += size
    assert doc is not None and binary is not None, "Missing JSON/BIN chunk"
    assert not doc.get("extensionsRequired"), "Required extensions unsupported by lightweight loader"
    assert not doc.get("skins") and not doc.get("animations"), "Only static GLB supported"
    assert doc.get("scenes"), "Missing scene"
    for item in doc.get("accessors", []):
        assert not item.get("sparse"), "Sparse accessors unsupported"
        view = doc["bufferViews"][item["bufferView"]]
        assert not view.get("byteStride"), "Interleaved accessors unsupported"
        size = COMPONENT_BYTES[item["componentType"]] * ITEM_SIZES[item["type"]] * item["count"]
        start = view.get("byteOffset", 0) + item.get("byteOffset", 0)
        assert start + size <= len(binary), "Accessor exceeds binary chunk"
    for item in doc.get("images", []):
        assert "bufferView" in item, "External images unsupported"
        view = doc["bufferViews"][item["bufferView"]]
        start = view.get("byteOffset", 0)
        assert start + view["byteLength"] <= len(binary), "Image exceeds binary chunk"
    triangles = 0
    for mesh in doc.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            assert primitive.get("mode", 4) == 4, "Non-triangle primitive unsupported"
            if "indices" in primitive:
                triangles += doc["accessors"][primitive["indices"]]["count"] // 3
            else:
                triangles += doc["accessors"][primitive["attributes"]["POSITION"]]["count"] // 3
    print(json.dumps({"ok": True, "path": str(path), "triangles": triangles, "meshes": len(doc.get("meshes", [])), "materials": len(doc.get("materials", [])), "textures": len(doc.get("textures", []))}, ensure_ascii=False))


if __name__ == "__main__":
    check(Path(sys.argv[1] if len(sys.argv) > 1 else "web/assets/models/characters/TRIPO-001.glb"))
