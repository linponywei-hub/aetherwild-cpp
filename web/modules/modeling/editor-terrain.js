import * as T from '../../vendor/three.module.js';
import {terrainMesh} from '../world/content-scene.js';
import {gridHeight} from '../world/landscape-model.js';
import {makeLandscape} from '../world/landscape-scene.js';
import {clipTerrainPatch} from '../world/terrain-material.js';

// The editor requests native terrain tiles, never invents an independent height map.
export class EditorTerrain {
 constructor(scene,{fetcher=fetch,onStatus=()=>{},onJump=()=>{}}={}){
  this.scene=scene;this.fetcher=fetcher;this.onStatus=onStatus;this.onJump=onJump;
  this.root=new T.Group();this.root.name='native-world-terrain';scene.add(this.root);
  this.catalog=null;this.patch=null;this.local=null;this.requestId=0;this.jumpId=0;this.jumping=false;this.lastRequest='';this.lastTime=0;this.cache=new Map();
 }
 async load(){
  this.onStatus('正在載入遊戲地圖…');const response=await this.fetcher('/api/catalog');if(!response.ok)throw Error('無法載入世界資料');
  const catalog=await response.json();
  if(!catalog.terrain||catalog.landscape?.version<2||!catalog.atlas)throw Error('伺服器尚未提供新版地形');
  this.catalog=catalog;this.global=terrainMesh(catalog.terrain,'#718866',catalog.landscape);this.root.add(this.global);
  const view={catalog,root:this.root,mat:(color,extra={})=>new T.MeshStandardMaterial({color,roughness:.9,...extra})};
  view.mesh=(geometry,material,position,parent=this.root)=>{const mesh=new T.Mesh(geometry,typeof material==='string'?view.mat(material):material);if(position)mesh.position.set(...position);parent.add(mesh);return mesh;};
  view.box=(parent,w,h,d,color,x=0,y=0,z=0)=>view.mesh(new T.BoxGeometry(w,h,d),view.mat(color),[x,y,z],parent);
  view.cylinder=(parent,r1,r2,h,color,x=0,y=0,z=0,segments=12)=>view.mesh(new T.CylinderGeometry(r1,r2,h,segments),view.mat(color),[x,y,z],parent);
  makeLandscape(view);this.onStatus('已載入原世界地圖與地形。');return catalog;
 }
 key(x,z){return Math.floor(x/512)+':'+Math.floor(z/512);}
 async tile(x,z){
  if(!this.catalog)return null;const limit=this.catalog.worldLimit;
  if(!Number.isFinite(x)||!Number.isFinite(z)||Math.abs(x)>limit||Math.abs(z)>limit)throw Error('位置超出世界範圍');
  const key=this.key(x,z);if(this.cache.has(key))return this.cache.get(key);
  const response=await this.fetcher('/api/terrain',{method:'POST',headers:{'Content-Type':'application/json','X-Aetherwild-Client':'1'},body:JSON.stringify({x,z})});
  if(!response.ok)throw Error('無法讀取地形高度');const data=await response.json(),patch=data.terrain;
  if(patch?.side!==65||patch.step!==16||patch.version!==this.catalog.landscape.version||!Array.isArray(patch.heights)||patch.heights.length!==4225||!patch.heights.every(Number.isFinite)||!Number.isFinite(patch.x)||!Number.isFinite(patch.z)||gridHeight(patch,x,z)===null)throw Error('地形資料不完整或版本不符');
  if(this.cache.size>=16)this.cache.delete(this.cache.keys().next().value);this.cache.set(key,patch);return patch;
 }
 async ensurePatch(x,z){
  const id=++this.requestId,patch=await this.tile(x,z);if(!patch||id!==this.requestId)return patch;
  if(this.local){this.root.remove(this.local);this.local.geometry.dispose();this.local.material.dispose();}
  this.patch=patch;this.local=terrainMesh(patch,'#718866',this.catalog.landscape);this.local.userData.localTerrain=true;this.root.add(this.local);clipTerrainPatch(this.global.material,patch);return patch;
 }
 async sampleHeight(x,z){if(!this.catalog)return 0;const patch=await this.tile(x,z);return gridHeight(patch,x,z);}
 height(x,z){if(!this.catalog)return 0;return gridHeight(this.patch,x,z)??gridHeight(this.catalog.terrain,x,z)??0;}
 async jumpToRegion(id){const r=this.catalog?.regions.find(r=>r.id===id);if(!r)return;await this.jumpAt(r.position.x,r.position.z,r.name);}
 async jumpAt(x,z,label='地圖位置'){
  const id=++this.jumpId;this.jumping=true;this.lastRequest=this.key(x,z);
  try{const patch=await this.ensurePatch(x,z);if(!patch||id!==this.jumpId)return;this.onJump({x,y:gridHeight(patch,x,z),z});this.onStatus(label+' · 已載入實際地形');}
  finally{if(id===this.jumpId)this.jumping=false;}
 }
 updateTarget(target,now){
  if(!this.catalog||this.jumping||now-this.lastTime<700)return;const key=this.key(target.x,target.z);if(key===this.lastRequest)return;
  this.lastTime=now;this.lastRequest=key;
  this.ensurePatch(target.x,target.z).catch(e=>{if(this.lastRequest===key)this.lastRequest='';this.onStatus(e.message);});
 }
}
