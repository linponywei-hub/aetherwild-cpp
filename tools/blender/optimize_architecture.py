import bpy,sys,json
from pathlib import Path
args=sys.argv[sys.argv.index('--')+1:]
src=Path(args[0]);out=Path(args[1]);report=Path(args[2])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(src))
meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
def tris(obj):return sum(max(0,len(p.vertices)-2) for p in obj.data.polygons)
before=sum(tris(o) for o in meshes)
if before<1:raise RuntimeError('No triangles')
# Reduce each independent mesh proportionally. Never overwrite the source GLB.
target=180000
for obj in meshes:
 n=tris(obj)
 if n<1500:continue
 mod=obj.modifiers.new('Aetherwild LOD0 Decimate','DECIMATE')
 mod.ratio=min(1,max(.02,target/before))
 bpy.context.view_layer.objects.active=obj
 bpy.ops.object.modifier_apply(modifier=mod.name)
after=sum(tris(o) for o in meshes)
out.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.export_scene.gltf(filepath=str(out),export_format='GLB',export_texcoords=True,export_normals=True,export_materials='EXPORT')
report.parent.mkdir(parents=True,exist_ok=True)
report.write_text(json.dumps({'input':str(src),'output':str(out),'meshes':len(meshes),'triangles_before':before,'triangles_after':after,'reduction_pct':round(100*(1-after/before),2),'warning':'Decimation can damage silhouette, UVs, and details; visual inspection and collision mesh authoring required. Original units are not verified.'},indent=2))
print(report.read_text())
