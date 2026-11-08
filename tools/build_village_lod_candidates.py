#!/usr/bin/env python3
"""Generate conservative, UV-seam-aware LOD candidates. Originals remain untouched."""
import bpy,bmesh,json,pathlib,sys
ROOT=pathlib.Path("web/assets/models/buildings/village")
OUT=pathlib.Path("village-lod-candidates")
OUT.mkdir(exist_ok=True)
REPORT=[]
for path in sorted(ROOT.glob("village_building_*_lod0.glb")):
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.gltf(filepath=str(path.resolve()))
 meshes=[obj for obj in bpy.context.scene.objects if obj.type=="MESH"]
 for obj in meshes:
  # Protect material boundaries, UV islands, sharp corners and boundaries.
  if obj.data.users>1:obj.data=obj.data.copy()
  mesh=obj.data
  if not mesh.uv_layers:continue
  bm=bmesh.new();bm.from_mesh(mesh);uv=bm.loops.layers.uv.active
  for edge in bm.edges:
   if not edge.is_manifold:
    edge.seam=True;continue
   loops=[]
   for face in edge.link_faces:
    loops.append({tuple(round(v[uv].uv[k],5) for k in (0,1)) for v in face.loops if v.vert in edge.verts})
   if len(loops)==2 and loops[0]!=loops[1]:edge.seam=True
  bm.to_mesh(mesh);bm.free();mesh.update()
 original_faces=sum(len(obj.data.polygons) for obj in meshes)
 for ratio in (0.85,0.65):
  # Re-import for every candidate, never compound destructive reductions.
  bpy.ops.wm.read_factory_settings(use_empty=True)
  bpy.ops.import_scene.gltf(filepath=str(path.resolve()))
  for obj in [o for o in bpy.context.scene.objects if o.type=="MESH"]:
   if obj.data.users>1:obj.data=obj.data.copy()
   mod=obj.modifiers.new("Conservative LOD","DECIMATE")
   mod.decimate_type="COLLAPSE";mod.ratio=ratio
   mod.use_collapse_triangulate=False
   mod.delimit={"UV","SEAM","MATERIAL","SHARP","NORMAL"}
   bpy.context.view_layer.objects.active=obj
   bpy.ops.object.modifier_apply(modifier=mod.name)
  output=OUT/(path.stem+f"_ratio_{int(ratio*100)}.glb")
  bpy.ops.export_scene.gltf(filepath=str(output.resolve()),export_format="GLB",export_apply=False)
  faces=sum(len(o.data.polygons) for o in bpy.context.scene.objects if o.type=="MESH")
  REPORT.append({"source":path.name,"ratio":ratio,"source_faces":original_faces,"output_faces":faces,"output":output.name})
  print(REPORT[-1],flush=True)
(OUT/"lod_report.json").write_text(json.dumps(REPORT,indent=2))
assert len(REPORT)==10, f"Expected ten candidate outputs, got {len(REPORT)}"
