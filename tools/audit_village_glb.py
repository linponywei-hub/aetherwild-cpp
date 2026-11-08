#!/usr/bin/env python3
"""Audit GLB mesh topology without changing original models or materials."""
import json,struct,pathlib,collections,sys
ROOT=pathlib.Path("web/assets/models/buildings/village")
def audit(path):
 data=path.read_bytes()
 if data[:4]!=b"glTF":return {"error":"not a GLB"}
 offset=12;meta=None;binary=b""
 while offset+8<=len(data):
  length,typ=struct.unpack_from("<II",data,offset);offset+=8
  chunk=data[offset:offset+length];offset+=length
  if typ==0x4E4F534A:meta=json.loads(chunk)
  elif typ==0x004E4942:binary=chunk
 if meta is None:return {"error":"missing JSON chunk"}
 accessors=meta.get("accessors",[]);views=meta.get("bufferViews",[])
 def values(index):
  acc=accessors[index];view=views[acc["bufferView"]]
  typ=acc["componentType"];fmt={5121:"B",5123:"H",5125:"I",5126:"f",5122:"h",5120:"b"}[typ]
  count=acc["count"];components={"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4}[acc["type"]]
  size=struct.calcsize("<"+fmt)*components;stride=view.get("byteStride",size)
  start=view.get("byteOffset",0)+acc.get("byteOffset",0)
  return [struct.unpack_from("<"+fmt*components,binary,start+i*stride) for i in range(count)]
 report={"file":path.name,"meshes":0,"triangles":0,"boundary_edges":0,"nonmanifold_edges":0,"degenerate_triangles":0,"materials":len(meta.get("materials",[])),"primitives":[]}
 for mesh in meta.get("meshes",[]):
  for prim in mesh.get("primitives",[]):
   if prim.get("mode",4)!=4 or "POSITION" not in prim.get("attributes",{}):continue
   positions=values(prim["attributes"]["POSITION"])
   indices=[x[0] for x in values(prim["indices"])] if "indices" in prim else list(range(len(positions)))
   # Count edges by vertex position rather than index: GLB UV seams often split vertices.
   key=lambda idx:tuple(round(float(v),5) for v in positions[idx][:3])
   edges=collections.Counter();degenerate=0
   for k in range(0,len(indices)-2,3):
    a,b,c=(key(indices[k+j]) for j in range(3))
    if len({a,b,c})!=3:degenerate+=1;continue
    for u,v in ((a,b),(b,c),(c,a)):edges[tuple(sorted((u,v)))]+=1
   boundaries=sum(n==1 for n in edges.values());nonmanifold=sum(n>2 for n in edges.values())
   report["meshes"]+=1;report["triangles"]+=len(indices)//3
   report["boundary_edges"]+=boundaries;report["nonmanifold_edges"]+=nonmanifold;report["degenerate_triangles"]+=degenerate
   report["primitives"].append({"vertices":len(positions),"triangles":len(indices)//3,"boundary_edges":boundaries,"nonmanifold_edges":nonmanifold,"degenerate_triangles":degenerate})
 return report
if __name__=="__main__":
 out={"houses":[]}
 for path in sorted(ROOT.glob("village_building_*_lod0.glb")):
  try:out["houses"].append(audit(path))
  except Exception as exc:out["houses"].append({"file":path.name,"error":str(exc)})
 pathlib.Path("village_glb_audit.json").write_text(json.dumps(out,ensure_ascii=False,indent=2))
 for x in out["houses"]:print(json.dumps({k:v for k,v in x.items() if k!="primitives"},ensure_ascii=False))
 if not out["houses"]:sys.exit("No GLBs found")
