import {surfaceAt} from './landscape-model.js';
// Meshes here only display server-authored terrain and access boundaries.
import * as T from '../../vendor/three.module.js';
import {terrainMaterial,clipTerrainPatch} from './terrain-material.js';
export function terrainMesh(data,color,landscape){
 const n=data.side,vertices=[],indices=[];for(let z=0;z<n;z++)for(let x=0;x<n;x++)vertices.push(data.x+x*data.step,data.heights[z*n+x],data.z+z*data.step);
 for(let z=0;z<n-1;z++)for(let x=0;x<n-1;x++){const a=z*n+x;indices.push(a,a+n,a+1,a+1,a+n,a+n+1);}
 const colors=[];if(landscape){const palette={plain:'#79925f',water:'#3c889c',snow:'#d7e2e5',volcanic:'#68574b',forest:'#355f3a',desert:'#d5b57e',mesa:'#b78054',marsh:'#6f9468'};for(let i=0;i<vertices.length;i+=3){const kind=surfaceAt(vertices[i],vertices[i+2],landscape),color=new T.Color(palette[kind]||palette.plain);colors.push(color.r,color.g,color.b);}}
 const climate=[];for(let i=0;i<vertices.length;i+=3)climate.push(landscape&&surfaceAt(vertices[i],vertices[i+2],landscape)==='snow'?1:0);
 const geometry=new T.BufferGeometry();geometry.setAttribute('position',new T.Float32BufferAttribute(vertices,3));geometry.setAttribute('awClimate',new T.Float32BufferAttribute(climate,1));geometry.setIndex(indices);if(colors.length)geometry.setAttribute('color',new T.Float32BufferAttribute(colors,3));geometry.computeVertexNormals();
 const mesh=new T.Mesh(geometry,terrainMaterial({color:landscape?'#ffffff':color,vertexColors:!!landscape,starter:landscape?.starterVillage}));mesh.receiveShadow=true;mesh.userData.fullIndices=indices;return mesh;
}
export function updateContentScene(scene,state){
 const c=state.content;if(!c)return;
 if(!scene.contentRoot){scene.contentRoot=new T.Group();scene.root.add(scene.contentRoot);scene.contentProps=new T.Group();scene.root.add(scene.contentProps);}
 for(const live of c.npcs||[]){const obj=scene.npcObjects.get(live.id);if(obj)obj.userData.track.push(live.position,state.now);}
 const patch=c.terrainPatch;
 if(patch&&scene.patchKey!==patch.x+':'+patch.z){
  scene.sceneryCell='';scene.patchKey=patch.x+':'+patch.z;scene.starterLandscape?.rebuild();if(scene.terrainPatch){scene.contentRoot.remove(scene.terrainPatch);scene.terrainPatch.geometry.dispose();scene.terrainPatch.material.dispose();}
  scene.terrainPatch=terrainMesh(patch,'#708766',scene.catalog.landscape);scene.terrainPatch.userData.localTerrain=true;scene.contentRoot.add(scene.terrainPatch);
  if(scene.globalTerrain)clipTerrainPatch(scene.globalTerrain.material,patch);
 }
 const propsKey=Math.floor(state.player.position.x/32)+':'+Math.floor(state.player.position.z/32)+':'+c.release;
 if(scene.propsKey!==propsKey){scene.propsKey=propsKey;for(const child of [...scene.contentProps.children]){scene.contentProps.remove(child);child.traverse(o=>{o.geometry?.dispose();o.material?.dispose?.();});}
  for(const b of c.blockers||[]){const pos=b.position;scene.box(scene.contentProps,10,8,3,'#69746c',pos.x,pos.y+4,pos.z);scene.cylinder(scene.contentProps,.25,.25,6,'#a6bda5',pos.x,pos.y+3,pos.z,6);const l=scene.label('前方封鎖 · 請循驛路','#ecd79c',2);l.position.set(pos.x,pos.y+9,pos.z);scene.contentProps.add(l);}
  for(const b of c.books||[]){scene.box(scene.contentProps,1.5,1.1,.9,'#7d6d50',b.position.x,b.position.y+.55,b.position.z);const label=scene.label('藏記架 · 終端→故事','#e8d4a3',1.1);label.position.set(b.position.x,b.position.y+2,b.position.z);scene.contentProps.add(label);}
  const seen=new Set();for(const s of c.stories||[]){const key=s.position.x+':'+s.position.z;if(seen.has(key))continue;seen.add(key);scene.box(scene.contentProps,2,.9,1.2,'#796b52',s.position.x,s.position.y+.45,s.position.z);const label=scene.label('地方調查桌','#e8d4a3',1);label.position.set(s.position.x,s.position.y+1.7,s.position.z);scene.contentProps.add(label);}
 }
}

export function updateCastTelegraphs(scene,state){
 if(!scene.castRoot){scene.castRoot=new T.Group();scene.world.add(scene.castRoot);scene.castMeshes=new Map();}
 const needed=new Set();for(const m of state.monsters||[]){if(!m.cast||m.hp<=0)continue;const key=m.id+':'+m.cast.resolveAt;needed.add(key);if(scene.castMeshes.has(key))continue;
  const c=m.cast,mat=new T.MeshBasicMaterial({color:'#e17d51',transparent:true,opacity:.32,side:T.DoubleSide,depthWrite:false});let mesh,center=c.position;
  if(c.shape==='annulus'){mesh=new T.Mesh(new T.RingGeometry(c.innerRadius,c.radius,64),mat);center=c.origin;mesh.rotation.x=-Math.PI/2;}
  else if(c.shape==='cone'||c.shape==='melee_arc'){const angle=Math.atan2(c.position.z-c.origin.z,c.position.x-c.origin.x);mesh=new T.Mesh(new T.CircleGeometry(c.radius,40,-angle-Math.PI/3,Math.PI*2/3),mat);mesh.rotation.x=-Math.PI/2;center=c.origin;}
  else if(c.shape==='capsule'){const dx=c.position.x-c.origin.x,dz=c.position.z-c.origin.z;mesh=new T.Mesh(new T.BoxGeometry(c.width,.06,Math.min(24,Math.hypot(dx,dz))),mat);mesh.rotation.y=Math.atan2(dx,dz);center={x:(c.position.x+c.origin.x)/2,y:c.position.y,z:(c.position.z+c.origin.z)/2};}
  else{mesh=new T.Mesh(new T.CircleGeometry(c.radius,48),mat);mesh.rotation.x=-Math.PI/2;}
  mesh.position.set(center.x,center.y+.12,center.z);scene.castRoot.add(mesh);scene.castMeshes.set(key,mesh);
 }
 for(const[key,mesh]of scene.castMeshes)if(!needed.has(key)){scene.castRoot.remove(mesh);mesh.geometry.dispose();mesh.material.dispose();scene.castMeshes.delete(key);}
}
