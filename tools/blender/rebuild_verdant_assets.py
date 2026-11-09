"""Cut a traversable hollow/doorways into the CC0 imported Bellroot tree.
Run: blender -b -t 2 --python tools/blender/rebuild_verdant_assets.py
All cutter objects are deleted before export; the rendered mesh is the external asset.
"""
import bpy, math
from pathlib import Path
root=Path(__file__).resolve().parents[2]
folder=root/'web/assets/models/verdant/rebuilt'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(folder/'bellroot-tree.glb'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
# Work with the authored bark mesh, omitting ornamental copper root loops.
for o in objects:
 if not any('gate-root' in m.name for m in o.data.materials if m):bpy.data.objects.remove(o,do_unlink=True)
objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
for o in objects:
 # glTF Y-up imports as Blender Z-up. Scale in world coordinates.
 from mathutils import Matrix
 o.matrix_world=Matrix.Diagonal((50,50,24,1)) @ o.matrix_world
 bpy.context.view_layer.objects.active=o;o.select_set(True)
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.select_set(False)
 # Preserve the imported topology while softening the straight bole and spreading roots.
 for vertex in o.data.vertices:
  w=o.matrix_world@vertex.co
  radius=math.hypot(w.x,w.y)
  if w.z<24 and radius>24:
   w.z*=max(.15,1-(radius-24)/50)
  if radius>11.5:
   wave=1+.035*math.sin(math.atan2(w.y,w.x)*7+w.z*.06)
   w.x*=wave;w.y*=wave
  vertex.co=o.matrix_world.inverted()@w
 for polygon in o.data.polygons:polygon.use_smooth=True
 for material in o.data.materials:
  if material:material.diffuse_color=(.24,.13,.055,1)
# A central cavity covers both the ascending spiral and the upper landing.
bpy.ops.mesh.primitive_cylinder_add(vertices=64,radius=11,depth=66,location=(0,0,32))
cavity=bpy.context.object
cutters=[cavity]
for height in [3,64]:
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,18,height))
 door=bpy.context.object;door.dimensions=(8,36,8);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);cutters.append(door)
for o in objects:
 bpy.context.view_layer.objects.active=o
 for cutter in cutters:
  modifier=o.modifiers.new('traversable hollow','BOOLEAN');modifier.operation='DIFFERENCE';modifier.solver='EXACT';modifier.object=cutter
  bpy.ops.object.modifier_apply(modifier=modifier.name)
 for m in o.data.materials:
  if m:m.use_backface_culling=False
for c in cutters:bpy.data.objects.remove(c,do_unlink=True)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(folder/'hollow-bellroot.glb'),export_format='GLB',use_selection=True,export_animations=False,export_yup=True)
print('EXPORTED_EXTERNAL_HOLLOW_TREE',len(objects),sum(len(o.data.polygons) for o in objects))
