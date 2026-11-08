#!/usr/bin/env python3
"""Non-destructive house 001 small-hole candidate. Never overwrites originals."""
import bpy,bmesh,json,pathlib,collections,math
from mathutils import Vector
src=pathlib.Path("web/assets/models/buildings/village/village_building_01_lod0.glb")
out=pathlib.Path("village-hole-fill-candidate");out.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(src.resolve()))
report={"source":src.name,"meshes":[],"filled_loops":0,"new_faces":0,"skipped_loops":0}
for obj in [o for o in bpy.context.scene.objects if o.type=="MESH"]:
 if obj.data.users>1:obj.data=obj.data.copy()
 mesh=obj.data;bm=bmesh.new();bm.from_mesh(mesh);bm.verts.ensure_lookup_table()
 if not bm.verts:bm.free();continue
 bb=[v.co for v in bm.verts]
 diag=(Vector(tuple(max(v.co[i] for v in bm.verts) for i in range(3)))-Vector(tuple(min(v.co[i] for v in bm.verts) for i in range(3)))).length
 max_diameter=diag*.012
 boundary={e for e in bm.edges if e.is_boundary}
 adjacency=collections.defaultdict(list)
 for e in boundary:
  for v in e.verts:adjacency[v].append(e)
 visited=set();count=0;newfaces=0;skipped=0
 # Only closed, non-branching tiny loops: never bridge unrelated open borders.
 for seed in list(boundary):
  if seed in visited:continue
  component=set();stack=[seed]
  while stack:
   edge=stack.pop()
   if edge in component:continue
   component.add(edge)
   for v in edge.verts:stack.extend(e for e in adjacency[v] if e not in component)
  visited.update(component)
  verts={v for e in component for v in e.verts}
  if len(component)<3 or len(component)>8 or any(len(adjacency[v])!=2 for v in verts):
   skipped+=1;continue
  coords=[v.co for v in verts]
  diameter=max((a-b).length for a in coords for b in coords)
  if diameter>max_diameter:
   skipped+=1;continue
  surrounding=[face for edge in component for face in edge.link_faces]
  if not surrounding:skipped+=1;continue
  # Avoid bridging strongly bent corners or architectural openings.
  normal=surrounding[0].normal
  if any(face.normal.dot(normal)<.85 for face in surrounding):
   skipped+=1;continue
  materials=collections.Counter(face.material_index for face in surrounding)
  material=materials.most_common(1)[0][0]
  # Fill only the selected closed boundary; preserve all original faces.
  result=bmesh.ops.holes_fill(bm,edges=list(component),sides=0)
  faces=list(result.get("faces",[]))
  if not faces:skipped+=1;continue
  uv_layer=bm.loops.layers.uv.active
  # Reuse the UV at each border vertex from its adjacent original face.
  # This keeps the original texture and makes the patch match its border.
  uv_by_vertex={}
  if uv_layer:
   for face in surrounding:
    for loop in face.loops:
     if loop.vert in verts and loop.vert not in uv_by_vertex:
      uv_by_vertex[loop.vert]=loop[uv_layer].uv.copy()
  for face in faces:
   face.material_index=material
   if uv_layer:
    for loop in face.loops:
     if loop.vert in uv_by_vertex:loop[uv_layer].uv=uv_by_vertex[loop.vert]
  count+=1;newfaces+=len(faces)
 bm.to_mesh(mesh);bm.free();mesh.update()
 report["meshes"].append({"name":obj.name,"filled_loops":count,"new_faces":newfaces,"skipped_loops":skipped})
 report["filled_loops"]+=count;report["new_faces"]+=newfaces;report["skipped_loops"]+=skipped
dest=out/"village_building_01_lod0_patch_test.glb"
bpy.ops.export_scene.gltf(filepath=str(dest.resolve()),export_format="GLB",export_apply=False)
report["output"]=str(dest);(out/"report.json").write_text(json.dumps(report,ensure_ascii=False,indent=2))
print(json.dumps(report,ensure_ascii=False),flush=True)
assert dest.is_file() and dest.stat().st_size>1024
