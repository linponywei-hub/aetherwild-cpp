"""Convert split village GLBs into independent game assets with visual-QA reports.
Usage: blender -b -P tools/blender/build_village_game_assets.py -- INPUT_DIR OUTPUT_DIR
Requires Blender 3.6+. Does not overwrite sources.
"""
import bpy, sys, json, math
from pathlib import Path
args=sys.argv[sys.argv.index("--")+1:]
source,out=map(Path,args[:2]);out.mkdir(parents=True,exist_ok=True)
report=[]
for f in sorted(source.glob("village_building_*.glb")):
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.gltf(filepath=str(f))
 objs=[o for o in bpy.context.scene.objects if o.type=="MESH"]
 bpy.context.view_layer.update()
 before=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in objs)
 # Original model uses Y-up in GLB; Blender import converts it to Z-up.
 # Avoid guessing real-world scale; report dimensions for human review.
 from mathutils import Vector
 corners=[o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
 mins=[min(v[i] for v in corners) for i in range(3)]
 maxs=[max(v[i] for v in corners) for i in range(3)]
 center=((mins[0]+maxs[0])/2,(mins[1]+maxs[1])/2,mins[2])
 for o in objs:
  o.location.x-=center[0];o.location.y-=center[1];o.location.z-=center[2]
 # Preserve high-quality source geometry; make a separate reduced version.
 high=out/(f.stem+"_source.glb")
 bpy.ops.export_scene.gltf(filepath=str(high),export_format="GLB")
 for o in objs:
  n=sum(len(p.vertices)-2 for p in o.data.polygons)
  if n<1000:continue
  mod=o.modifiers.new("LOD0_reduction","DECIMATE")
  mod.ratio=max(.02,min(1.0,85000/max(before,1)))
  bpy.context.view_layer.objects.active=o
  bpy.ops.object.modifier_apply(modifier=mod.name)
 after=sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in objs)
 dest=out/(f.stem+"_lod0.glb")
 bpy.ops.export_scene.gltf(filepath=str(dest),export_format="GLB")
 report.append(dict(name=f.stem,source_triangles=before,lod0_triangles=after,source_dimensions=[round(maxs[i]-mins[i],5) for i in range(3)],scale_requires_review=True,source_file=high.name,lod0_file=dest.name,qa="Needs visual check of roofs, textures and detached props; collision not yet authored"))
(out/"conversion_report.json").write_text(json.dumps(report,ensure_ascii=False,indent=2))
print("Processed",len(report),"buildings")
