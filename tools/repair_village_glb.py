#!/usr/bin/env python3
"""Conservative, non-destructive village GLB repair in Blender.
Outputs repaired candidates separately; never overwrites source assets.
"""
import bpy,bmesh,json,math,pathlib,sys
from mathutils import Vector

SOURCE=pathlib.Path("web/assets/models/buildings/village")
OUTPUT=pathlib.Path("repaired-village-glb")
OUTPUT.mkdir(exist_ok=True)
reports=[]
def stats(mesh):
 bm=bmesh.new();bm.from_mesh(mesh)
 result={"vertices":len(bm.verts),"faces":len(bm.faces),"open_edges":sum(e.is_boundary for e in bm.edges),"nonmanifold_edges":sum(not e.is_manifold and not e.is_boundary for e in bm.edges)}
 bm.free();return result

for file in sorted(SOURCE.glob("village_building_*_lod0.glb")):
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.gltf(filepath=str(file.resolve()))
 objects=[o for o in bpy.context.scene.objects if o.type=="MESH"]
 report={"file":file.name,"objects":len(objects),"before":[],"after":[],"changes":[]}
 for obj in objects:
  # Avoid changing other objects which share the same mesh datablock.
  if obj.data.users>1:obj.data=obj.data.copy()
  mesh=obj.data
  report["before"].append(stats(mesh))
  bm=bmesh.new();bm.from_mesh(mesh)
  bm.verts.ensure_lookup_table()
  bounds=[Vector(v.co) for v in bm.verts]
  diag=(Vector(tuple(max(v[i] for v in bounds) for i in range(3)))-Vector(tuple(min(v[i] for v in bounds) for i in range(3)))).length if bounds else 0
  # Very small tolerance: only genuinely coincident positions. UV seams are
  # protected because merging coincident vertices across material boundaries
  # can otherwise damage baked Tripo textures.
  threshold=min(max(diag*0.000001,0.0000001),0.00002)
  # Remove loose vertices/edges, but NEVER cap door/window openings.
  loose_verts=[v for v in bm.verts if not v.link_edges and not v.link_faces]
  if loose_verts:bmesh.ops.delete(bm,geom=loose_verts,context="VERTS")
  loose_edges=[e for e in bm.edges if not e.link_faces]
  if loose_edges:bmesh.ops.delete(bm,geom=loose_edges,context="EDGES")
  # Repair zero-area faces; do not use aggressive remesh/decimation.
  bmesh.ops.dissolve_degenerate(bm,edges=list(bm.edges),dist=threshold)
  # Recalculate connected face orientation, preserving original UV maps.
  bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
  bm.to_mesh(mesh);bm.free();mesh.update()
  report["after"].append(stats(mesh))
  report["changes"].append({"tolerance":threshold,"loose_vertices_removed":len(loose_verts),"loose_edges_removed":len(loose_edges)})
 # Preserve transforms, textures and original vertex coordinates.
 dest=OUTPUT/file.name
 bpy.ops.export_scene.gltf(filepath=str(dest.resolve()),export_format="GLB",export_apply=False,export_yup=True)
 report["output"]=str(dest)
 reports.append(report)
 print(json.dumps(report,ensure_ascii=False),flush=True)
(OUTPUT/"repair_report.json").write_text(json.dumps(reports,ensure_ascii=False,indent=2))
