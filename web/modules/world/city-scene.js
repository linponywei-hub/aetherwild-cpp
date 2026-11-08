import * as T from '../../vendor/three.module.js';
import {buildingParts,chunkKey,desiredCells} from './city-architecture.js';
import {batchParts} from '../rendering/mesh-batch.js';
import {surfaceAt} from './landscape-model.js';
export class CityScene {
 constructor(scene){this.scene=scene;this.chunks=[];this.glows=[];this.lamps=[];this.scenery=new Map();this.lights=[];this.pending=[];
  const batches=new Map();let starterHouseIndex=0;for(const b of scene.catalog.buildings){if(!b.style.includes('|'))continue;
   // Hide all old procedural buildings in the starter village. Only the five uploaded GLBs remain.
   if(b.region==='windchime'){
    if(b.style.split('|')[1]==='house'&&starterHouseIndex<5){
     const house=new T.Group();house.position.set(b.position.x,scene.landHeight(b.position.x,b.position.z),b.position.z);
     scene.root.add(house);scene.loadVillageHouse(house,++starterHouseIndex,b);
    }
    continue;
   }const key=chunkKey(b.position.x,b.position.z);if(!batches.has(key))batches.set(key,{x:Math.floor(b.position.x/128)*128,z:Math.floor(b.position.z/128)*128,parts:[]});const batch=batches.get(key);for(const p of buildingParts(b))batch.parts.push({...p,x:p.x+b.position.x-batch.x,y:p.y+b.position.y,z:p.z+b.position.z-batch.z});if(b.style.endsWith('|lamp'))this.lamps.push({x:b.position.x,y:b.position.y+b.height,z:b.position.z});}
  for(const r of scene.catalog.regions){if(r.wilderness||['tidehaven','aw_westport','aw_southmill'].includes(r.id))continue;const u=r.urbanRadius||80;for(const axis of [0,1])for(let along=-u*.6;along<u*.6;along+=48){const length=Math.min(48,u*.6-along),x=r.position.x+(axis?0:along+length/2),z=r.position.z+(axis?along+length/2:0),key=chunkKey(x,z);if(!batches.has(key))batches.set(key,{x:Math.floor(x/128)*128,z:Math.floor(z/128)*128,parts:[]});const b=batches.get(key);b.parts.push({shape:'box',x:x-b.x,y:r.elevation+.035,z:z-b.z,sx:axis?8:length,sy:.06,sz:axis?length:8,color:'#b2a682'});}}
  for(const batch of batches.values()){const root=new T.Group();root.position.set(batch.x,0,batch.z);root.userData.cullRadius=128;for(const glow of [false,true]){const parts=batch.parts.filter(p=>!!p.glow===glow);if(!parts.length)continue;const mesh=batchParts(parts,{glow});root.add(mesh);if(glow)this.glows.push(mesh.material);}scene.root.add(root);this.chunks.push(root);}
  for(let i=0;i<6;i++){const light=new T.PointLight('#ffd391',0,18,2);scene.world.add(light);this.lights.push(light);}
 }
 update(p,night,world=true){if(this.lastCell!==chunkKey(p.x,p.z,160)){this.lastCell=chunkKey(p.x,p.z,160);const cells=desiredCells(p.x,p.z);const keep=new Set(cells.map(c=>c.x+':'+c.z));for(const[key,g]of this.scenery)if(!keep.has(key)){this.scene.ambient.remove(g);g.traverse(o=>{o.geometry?.dispose();o.material?.dispose();});this.scenery.delete(key);}this.pending=cells.filter(c=>!this.scenery.has(c.x+':'+c.z));}
  // One small terrain/scenery batch per frame, instead of rebuilding an entire forest at a cell edge.
  if(world&&this.pending.length)this.makeCell(this.pending.shift());
  if(!this.at||performance.now()-this.at>250){this.at=performance.now();const sorted=this.lamps.filter(l=>Math.hypot(l.x-p.x,l.z-p.z)<70).sort((a,b)=>Math.hypot(a.x-p.x,a.z-p.z)-Math.hypot(b.x-p.x,b.z-p.z));this.lights.forEach((l,i)=>{if(sorted[i])l.position.set(sorted[i].x,sorted[i].y,sorted[i].z);l.intensity=world&&sorted[i]?night*18:0;});for(const m of this.glows)m.emissiveIntensity=.08+night*.8;}
 }
 makeCell(cell){const scene=this.scene,parts=[],key=cell.x+':'+cell.z;let seed=(cell.x*73856093^cell.z*19349663)>>>0;const rand=()=>{seed=(seed*1664525+1013904223)>>>0;return seed/4294967296;};const root=new T.Group();root.position.set(cell.x*160,0,cell.z*160);
  for(let i=0;i<(scene.renderer.software?10:20);i++){const x=root.position.x+rand()*160,z=root.position.z+rand()*160;
   if(scene.catalog.regions.some(r=>!r.wilderness&&Math.hypot(x-r.position.x,z-r.position.z)<(r.urbanRadius||100)+15)||surfaceAt(x,z,scene.catalog.landscape)==='water')continue;
   const h=6+rand()*6,y=scene.landHeight(x,z),dx=x-root.position.x,dz=z-root.position.z;
   parts.push({shape:'cylinder',x:dx,y:y+h*.28,z:dz,sx:.5,sy:h*.56,sz:.5,color:'#66513c'},{shape:'cone',x:dx,y:y+h*.68,z:dz,sx:h*.55,sy:h*.8,sz:h*.55,color:'#5d7850'});
  }
  if(parts.length)root.add(batchParts(parts));scene.ambient.add(root);this.scenery.set(key,root);
 }
}
