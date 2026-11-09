import * as T from '../../vendor/three.module.js';
import {TILE,DECK_TOP,verdantDeckTiles,verdantBridges} from './verdant-layout.js';
const BASE='/assets/models/verdant/rebuilt/';
let sources;
async function load(name){
 const {GLTFLoader}=await import('../../vendor/GLTFLoader.js');
 const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),12000);
 try{const r=await fetch(BASE+name+'.glb',{signal:controller.signal});if(!r.ok)throw Error(name+': HTTP '+r.status);const data=await r.arrayBuffer();return await new Promise((resolve,reject)=>new GLTFLoader().parse(data,BASE,resolve,reject));}finally{clearTimeout(timer);}
}
export function bakeImportedGeometry(mesh){
 const geometry=mesh.geometry.clone();
 // Quantized GLB positions cannot hold world-space coordinates in an Int16 array.
 // Decode before baking node transforms; otherwise bridge/deck vertices wrap.
 for(const name of ['position','normal','tangent']){
  const a=geometry.getAttribute(name);if(!a)continue;
  const values=[];for(let i=0;i<a.count;i++)for(let k=0;k<a.itemSize;k++)values.push(a[['getX','getY','getZ','getW'][k]](i));
  geometry.setAttribute(name,new T.Float32BufferAttribute(values,a.itemSize));
 }
 return geometry.applyMatrix4(mesh.matrixWorld);
}
function instances(source,root,transforms,filter=()=>true){
 source.scene.updateMatrixWorld(true);
 source.scene.traverse(mesh=>{
  if(!mesh.isMesh||!filter(mesh))return;
  const geometry=bakeImportedGeometry(mesh);
  const batch=new T.InstancedMesh(geometry,mesh.material,transforms.length);
  transforms.forEach((m,i)=>batch.setMatrixAt(i,m));batch.instanceMatrix.needsUpdate=true;
  batch.castShadow=false;batch.receiveShadow=true;batch.computeBoundingSphere();root.add(batch);
 });
}
function matrix(x,y,z,sx,sy,sz,yaw=0){return new T.Matrix4().compose(new T.Vector3(x,y,z),new T.Quaternion().setFromAxisAngle(new T.Vector3(0,1,0),yaw),new T.Vector3(sx,sy,sz));}
export async function attachVerdantCC0Models(root){
 if(root.userData.assetPhase==='ready')return;
 root.userData.assetPhase='loading';
 try{
  sources??=Promise.all(['hollow-bellroot','timber-deck','rope-bridge','rhododendron','jungle-canopy'].map(load)).catch(error=>{sources=null;throw error;});
  const [tree,deck,bridge,bush,canopy]=await sources;
  const staged=new T.Group();staged.name='verdant-rebuilt-assets';
  // The base is at forest ground; a single imported buttressed bole rises above all decks.
  instances(tree,staged,[matrix(0,-32,0,1,1,1)]);
  instances(deck,staged,verdantDeckTiles().map(t=>matrix(t.x,-DECK_TOP,t.z,TILE/2.4,1,TILE/2.4)));
  // Reuse the imported timber decking for all spiral treads, batched into three draws.
  const stairs=[];
  for(let i=0;i<100;i++){const a=Math.PI/2+i*8*Math.PI/100;stairs.push(matrix(Math.cos(a)*8.5,-32+i*.32-DECK_TOP*.22,Math.sin(a)*8.5,3.8/2.4,.22,3/2.4,-a));}
  instances(deck,staged,stairs);
  // Remove stone abutments. The timber deck endpoints are z=±3.57, y=1.59983338.
  instances(bridge,staged,verdantBridges.map(b=>{const mid=(b.start+b.end)/2;return matrix(Math.sin(b.angle)*mid,-1.59983338,Math.cos(b.angle)*mid,b.width/1.14965176,1,(b.end-b.start)/7.13999986,b.angle);}),m=>!m.name.endsWith('_0'));
  const foliage=[matrix(0,50,0,16,12,16),matrix(-32,31,0,11,9,11),matrix(32,31,0,11,9,11),matrix(0,35,-30,11,8,11)];
  for(let i=0;i<4;i++){const a=i*Math.PI/2;foliage.push(matrix(Math.sin(a)*54,-12,Math.cos(a)*54,11,5,11,a));}
  instances(bush,staged,foliage);
  instances(canopy,staged,[matrix(0,22,0,7,3.5,7)],m=>!m.name.endsWith('_0'));
  // Atomic replacement: a failed download never leaves half a city installed.
  root.clear();root.add(staged);root.userData.assetPhase='ready';root.userData.assetError=null;
  root.userData.externalCC0Models=5;
 }catch(error){root.userData.assetPhase='error';root.userData.assetError=String(error);throw error;}
}
