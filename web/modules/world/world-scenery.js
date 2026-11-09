import * as T from '../../vendor/three.module.js';
import {surfaceAt} from './landscape-model.js';
import {speciesFor} from './tree-species.js';
import {GLTFLoader} from '../../vendor/GLTFLoader.js';

// Deterministic, bounded scenery tiles. Server terrain remains authoritative.
const CELL=128, RADIUS=2, MAX_TILES=25;
const hash=(x,z,s=0)=>{let n=Math.imul(x,374761393)^Math.imul(z,668265263)^Math.imul(s+1,2246822519);n=Math.imul(n^(n>>>13),1274126177);return ((n^(n>>>16))>>>0)/4294967296;};
const treeGeometry=()=>{const g=new T.ConeGeometry(1,3,7);g.translate(0,1.5,0);return g;};
export class WorldScenery {
 constructor(scene){
  this.scene=scene;this.root=new T.Group();this.root.name='streamed-world-scenery';scene.root.add(this.root);
  this.tiles=new Map();this.treeGeo=treeGeometry();this.rockGeo=new T.DodecahedronGeometry(1,0);
  this.treeMat=new T.MeshStandardMaterial({color:'#426c39',roughness:1});
  this.rockMat=new T.MeshStandardMaterial({color:'#858478',roughness:1});
  this.dummy=new T.Object3D();this.treeTemplates=new Map();this.missingTrees=new Set();
 }
 update(x,z){
  const cx=Math.floor(x/CELL),cz=Math.floor(z/CELL),wanted=new Set();
  for(const [key,group] of this.tiles){const [tx,tz]=key.split(':').map(Number);if(Math.abs(tx-cx)>RADIUS||Math.abs(tz-cz)>RADIUS){this.root.remove(group);this.tiles.delete(key);}}
  for(let dz=-RADIUS;dz<=RADIUS;dz++)for(let dx=-RADIUS;dx<=RADIUS;dx++){
   const tx=cx+dx,tz=cz+dz,key=tx+':'+tz;wanted.add(key);
   if(!this.tiles.has(key))this.create(tx,tz,key);
  }
  for(const [key,group] of this.tiles)if(!wanted.has(key)){this.root.remove(group);this.tiles.delete(key);}
 }
 create(tx,tz,key){
  const scene=this.scene,group=new T.Group();group.name='scenery-'+key;group.userData.cullRadius=CELL*1.5;
  const trees=[],rocks=[],landscape=scene.catalog.landscape;
  for(let i=0;i<115;i++){
   const x=(tx+hash(tx,tz,i*2))*CELL,z=(tz+hash(tx,tz,i*2+1))*CELL;
   const biome=surfaceAt(x,z,landscape);
   if(biome==='water')continue;
   // Do not obstruct towns or the detailed starter village.
   if(scene.catalog.regions.some(r=>Math.hypot(x-r.position.x,z-r.position.z)<Math.max(125,r.extent||0)))continue;
   const y=scene.landHeight(x,z);if(!Number.isFinite(y))continue;
   const species=speciesFor(biome,y,hash(tx,tz,i+1700));
   const density=biome==='forest'?.78:biome==='marsh'?.23:biome==='snow'?.18:biome==='desert'?.025:biome==='volcanic'?.018:.12;
   if(species&&hash(tx,tz,i+400)<density)trees.push({x,y,z,s:species.meters*(.8+hash(tx,tz,i+800)*.4),species});
   else if(hash(tx,tz,i+900)<.24)rocks.push({x,y,z,s:.45+hash(tx,tz,i+1200)*1.5});
  }
  for(const [items,geo,mat,name] of [[trees,this.treeGeo,this.treeMat,'trees'],[rocks,this.rockGeo,this.rockMat,'rocks']]){
   if(!items.length)continue;
   const mesh=new T.InstancedMesh(geo,mat,items.length);mesh.name=name;mesh.castShadow=false;mesh.receiveShadow=true;
   items.forEach((p,i)=>{this.dummy.position.set(p.x,p.y+(name==='rocks'?p.s*.35:0),p.z);this.dummy.rotation.set(0,hash(tx,tz,i+1500)*Math.PI*2,0);this.dummy.scale.set(name==='trees'?p.s/3:p.s,name==='trees'?p.s/3:p.s,name==='trees'?p.s/3:p.s);this.dummy.updateMatrix();mesh.setMatrixAt(i,this.dummy.matrix);});
   mesh.instanceMatrix.needsUpdate=true;mesh.frustumCulled=false;group.add(mesh);
   if(name==='trees')this.replaceTrees(group,mesh,items,key);
  }
  this.root.add(group);this.tiles.set(key,group);
  if(this.tiles.size>MAX_TILES)throw Error('Scenery tile budget exceeded');
 }
 async loadTree(species){
  if(this.missingTrees.has(species.id))return null;
  if(!this.treeTemplates.has(species.id))this.treeTemplates.set(species.id,new GLTFLoader().loadAsync(species.path).then(gltf=>{
   const source=gltf.scene;source.updateMatrixWorld(true);const bounds=new T.Box3().setFromObject(source);const height=bounds.max.y-bounds.min.y;
   if(!Number.isFinite(height)||height<=.001)throw Error('Invalid tree GLB bounds');
   const meshes=[];source.traverse(n=>{if(n.isMesh)meshes.push({geometry:n.geometry,material:n.material,matrix:n.matrixWorld.clone()});});
   if(!meshes.length)throw Error('Tree GLB has no mesh');return {meshes,height,center:bounds.getCenter(new T.Vector3()),floor:bounds.min.y};
  }).catch(()=>{this.missingTrees.add(species.id);return null;}));
  return this.treeTemplates.get(species.id);
 }
 async replaceTrees(group,fallback,items,key){
  const buckets=new Map();for(const p of items){if(!buckets.has(p.species.id))buckets.set(p.species.id,[]);buckets.get(p.species.id).push(p);}
  const replacements=[];let loaded=0;
  for(const points of buckets.values()){
   const asset=await this.loadTree(points[0].species);
   if(this.tiles.get(key)!==group)return;
   if(!asset)continue;
   const pivot=new T.Matrix4().makeTranslation(-asset.center.x,-asset.floor,-asset.center.z);
   for(const part of asset.meshes){const mesh=new T.InstancedMesh(part.geometry,part.material,points.length);mesh.name='external-tree-'+points[0].species.id;
    points.forEach((p,i)=>{const transform=new T.Matrix4().compose(new T.Vector3(p.x,p.y,p.z),new T.Quaternion().setFromAxisAngle(new T.Vector3(0,1,0),hash(Math.floor(p.x),Math.floor(p.z),17)*Math.PI*2),new T.Vector3(1,1,1).multiplyScalar(p.s/asset.height));mesh.setMatrixAt(i,transform.multiply(pivot).multiply(part.matrix));});
    mesh.instanceMatrix.needsUpdate=true;mesh.frustumCulled=false;mesh.receiveShadow=true;group.add(mesh);replacements.push(mesh);
   }
   loaded+=points.length;
  }
  // Keep fallback for missing species; avoid hiding all trees on partial asset failure.
  if(loaded===items.length){fallback.visible=false;}else if(loaded>0){for(const mesh of replacements)mesh.visible=false;}
 }
 dispose(){for(const g of this.tiles.values())this.root.remove(g);this.tiles.clear();this.treeGeo.dispose();this.rockGeo.dispose();this.treeMat.dispose();this.rockMat.dispose();}
}
