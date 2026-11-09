"""Blender batch production: six visibly distinct goblin GLBs with authored actions.
Run inside Blender. The input must contain an armature with recognizable limb bones.
"""
import bpy, sys, math, json
from pathlib import Path
from mathutils import Vector

argv=sys.argv[sys.argv.index("--")+1:]
source=Path(argv[0]); output=Path(argv[1])
roles=[
 ("scout",(.25,.46,.18),"dagger"),
 ("slinger",(.42,.57,.20),"sling"),
 ("warrior",(.18,.32,.16),"sword"),
 ("archer",(.38,.43,.16),"bow"),
 ("shaman",(.28,.31,.53),"staff"),
 ("captain",(.36,.26,.12),"axe"),
]
def clear():
 bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
def mat(name,color,metal=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
 bs=m.node_tree.nodes.get("Principled BSDF")
 if bs: bs.inputs["Base Color"].default_value=(*color,1);bs.inputs["Metallic"].default_value=metal
 return m
def primitive(name,loc,scale,material,shape="cube",parent=None):
 if shape=="sphere":bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=8,location=loc)
 elif shape=="cone":bpy.ops.mesh.primitive_cone_add(vertices=10,location=loc)
 elif shape=="cylinder":bpy.ops.mesh.primitive_cylinder_add(vertices=10,location=loc)
 else:bpy.ops.mesh.primitive_cube_add(location=loc)
 ob=bpy.context.object;ob.name=name;ob.dimensions=scale
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 ob.data.materials.append(material)
 if parent:
  w=ob.matrix_world.copy();ob.parent=parent;ob.matrix_world=w
 return ob
def mesh_bounds(meshes):
 bpy.context.view_layer.update();deps=bpy.context.evaluated_depsgraph_get();v=[]
 for ob in meshes:
  e=ob.evaluated_get(deps);me=e.to_mesh()
  try:v.extend((e.matrix_world @ x.co) for x in me.vertices)
  finally:e.to_mesh_clear()
 if not v:raise RuntimeError("No vertices")
 return Vector((min(x.x for x in v),min(x.y for x in v),min(x.z for x in v))),Vector((max(x.x for x in v),max(x.y for x in v),max(x.z for x in v)))
def find_bone(arm,*tokens):
 for bone in arm.pose.bones:
  name=bone.name.lower().replace("mixamorig:","").replace("_","")
  if any(t in name for t in tokens):return bone
 return None
def animate(arm):
 bones={
  "lthigh":find_bone(arm,"leftupleg","thigh.l","leftthigh"),
  "rthigh":find_bone(arm,"rightupleg","thigh.r","rightthigh"),
  "lknee":find_bone(arm,"leftleg","shin.l","leftshin"),
  "rknee":find_bone(arm,"rightleg","shin.r","rightshin"),
  "larm":find_bone(arm,"leftarm","upperarm.l"),
  "rarm":find_bone(arm,"rightarm","upperarm.r"),
 }
 if not all(bones.values()):raise RuntimeError("Required arm/leg bones missing: "+str({k:bool(v) for k,v in bones.items()}))
 for b in bones.values():b.rotation_mode="XYZ"
 for name,length in (("Idle",48),("Walk",32),("Attack",28)):
  if arm.animation_data:arm.animation_data_clear()
  action=bpy.data.actions.new(name);arm.animation_data_create();arm.animation_data.action=action
  for frame in range(1,length+1,4):
   t=(frame-1)/length*math.tau
   walk= name=="Walk";attack=name=="Attack"
   vals={
    "lthigh": math.sin(t)*.33 if walk else 0,
    "rthigh":-math.sin(t)*.33 if walk else 0,
    "lknee": max(0,-math.sin(t))*.48 if walk else 0,
    "rknee": max(0, math.sin(t))*.48 if walk else 0,
    "larm":-math.sin(t)*.23 if walk else 0,
    "rarm": math.sin(t)*.23 if walk else (-.85*math.sin(min(1,frame/length)*math.pi) if attack else 0),
   }
   for key,b in bones.items():
    b.rotation_euler=(vals[key],0,0);b.keyframe_insert(data_path="rotation_euler",frame=frame,group=b.name)
  for b in bones.values():b.keyframe_insert(data_path="rotation_euler",frame=length+1,group=b.name)
  action.use_fake_user=True
  # glTF exporter exports stashed NLA actions as independent clips.
  track=arm.animation_data.nla_tracks.new();track.name=name
  strip=track.strips.new(name,1,action);strip.action_frame_end=length+1
  arm.animation_data.action=None
 return list(bones)
def run():
 output.mkdir(parents=True,exist_ok=True)
 results=[]
 for role,color,weapon in roles:
  clear();bpy.ops.import_scene.gltf(filepath=str(source))
  meshes=[o for o in bpy.context.scene.objects if o.type=="MESH"]
  arms=[o for o in bpy.context.scene.objects if o.type=="ARMATURE"]
  if not meshes or not arms:raise RuntimeError("Source needs mesh and armature")
  arm=arms[0];low,high=mesh_bounds(meshes)
  # Reposition entire imported hierarchy so the actual rest mesh sole touches z=0.
  for root in [o for o in bpy.context.scene.objects if o.parent is None]:root.location.z-=low.z
  bpy.context.view_layer.update();low,high=mesh_bounds(meshes)
  h=high.z-low.z;cx=(high.x+low.x)/2;cy=(high.y+low.y)/2
  leather=mat("Leather "+role,(.27,.16,.09));metal=mat("Metal "+role,(.48,.47,.42),.65)
  fabric=mat("Cloth "+role,color);bone=mat("Bone "+role,(.78,.71,.52))
  # Actual mesh additions, not game-side primitive attachments.
  primitive("Role chest armor", (cx,cy,h*.58), (h*.36,h*.22,h*.25),fabric,"cube",arm)
  primitive("Role belt",(cx,cy,h*.42),(h*.4,h*.25,h*.075),leather,"cube",arm)
  if role in ("warrior","captain"):
   for side in (-1,1):primitive("Shoulder plate",(cx+side*h*.24,cy,h*.7),(h*.17,h*.24,h*.12),metal,"sphere",arm)
  if role=="shaman":
   primitive("Shaman headdress",(cx,cy,h*.98),(h*.3,h*.28,h*.2),fabric,"cone",arm)
  if role=="archer":
   primitive("Quiver",(cx+h*.19,cy+h*.13,h*.56),(h*.12,h*.12,h*.35),leather,"cylinder",arm)
  if role=="slinger":
   primitive("Stone pouch",(cx+h*.23,cy,h*.39),(h*.2,h*.14,h*.16),leather,"sphere",arm)
  if role=="captain":
   for side in (-1,1):primitive("Helmet horn",(cx+side*h*.15,cy,h*1.02),(h*.1,h*.1,h*.3),bone,"cone",arm)
  # Distinct held weapon silhouette, geometry is exported in each GLB.
  wx=cx+h*.34;wy=cy-h*.04
  if weapon in ("sword","dagger","axe"):
   primitive(weapon+" blade",(wx,wy,h*.51),(h*.07,h*.08,h*(.39 if weapon!="dagger" else .22)),metal,"cube",arm)
   if weapon=="axe":primitive("Axe head",(wx,wy,h*.7),(h*.24,h*.09,h*.13),metal,"cube",arm)
  elif weapon=="staff":primitive("Staff",(wx,wy,h*.59),(h*.045,h*.045,h*.75),leather,"cylinder",arm)
  elif weapon=="bow":primitive("Bow stave",(wx,wy,h*.57),(h*.05,h*.05,h*.57),leather,"cylinder",arm)
  else:primitive("Sling stone",(wx,wy,h*.48),(h*.12,h*.12,h*.12),bone,"sphere",arm)
  bones=animate(arm)
  bpy.context.scene.frame_set(1)
  target=output/(role+".glb")
  bpy.ops.export_scene.gltf(filepath=str(target),export_format="GLB",export_animations=True,export_nla_strips=True)
  results.append({"role":role,"path":str(target),"height":h,"bone_channels":bones,"actions":["Idle","Walk","Attack"]})
 (output/"manifest.json").write_text(json.dumps(results,indent=2))
 print(json.dumps(results,indent=2))
run()
