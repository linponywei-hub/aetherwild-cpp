import bpy, sys, json
from pathlib import Path
from mathutils import Vector

argv=sys.argv[sys.argv.index("--")+1:]
source=Path(argv[0]); target=Path(argv[1]); report=Path(argv[2])
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(source))
objects=list(bpy.context.scene.objects)
meshes=[o for o in objects if o.type=='MESH']
if not meshes: raise RuntimeError("GLB contains no mesh")
bpy.context.view_layer.update()

def bounds():
    deps=bpy.context.evaluated_depsgraph_get()
    ys=[]
    for obj in meshes:
        evaluated=obj.evaluated_get(deps)
        mesh=evaluated.to_mesh()
        try:
            world=evaluated.matrix_world
            ys.extend((world @ v.co).z for v in mesh.vertices)
        finally: evaluated.to_mesh_clear()
    if not ys: raise RuntimeError("No mesh vertices")
    return min(ys),max(ys)

before=bounds()
# Move the whole imported hierarchy, preserving armature-to-mesh transforms.
roots=[o for o in objects if o.parent is None]
for root in roots: root.location.z-=before[0]
bpy.context.view_layer.update()
after=bounds()
if abs(after[0])>0.005: raise RuntimeError(f"Ground correction failed: {after}")
target.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.export_scene.gltf(filepath=str(target),export_format='GLB',export_apply=False)
report.parent.mkdir(parents=True,exist_ok=True)
report.write_text(json.dumps({"source":str(source),"output":str(target),"mesh_count":len(meshes),"armatures":sum(o.type=='ARMATURE' for o in objects),"before_z":before,"after_z":after,"note":"Grounded evaluated mesh at rest pose. Animation and gait not repaired."},indent=2))
print(report.read_text())
