import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import * as T from '../web/vendor/three.module.js';
import {GLTFLoader} from '../web/vendor/GLTFLoader.js';
import {ScannedVegetation} from '../web/modules/world/scanned-vegetation.js';
const manifest=JSON.parse(readFileSync(new URL('../web/assets/models/environment/sources.json',import.meta.url)));
test('third-party assets load through the official loader with native buffers and bounded triangles',async()=>{
 // Decode-free image stub: this checks scene/geometry parsing, not GPU rendering.
 globalThis.Image=class {set src(value){this.height=1;queueMicrotask(()=>this.onload?.());}};globalThis.self={URL:globalThis.URL};globalThis.createImageBitmap=async()=>({width:512,height:512,close(){}});
 for(const asset of manifest.models){
  const raw=readFileSync(new URL('../web'+asset.path,import.meta.url));assert.equal(createHash('sha256').update(raw).digest('hex'),asset.sha256);assert.equal(asset.license,'CC0-1.0');
  const loaded=await new GLTFLoader().parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');let count=0;
  loaded.scene.traverse(o=>{if(o.isMesh){assert.ok(o.geometry.attributes.normal);count+=(o.geometry.index?.count||o.geometry.attributes.position.count)/3;}});
  assert.equal(count,asset.triangles);assert.ok(count<13000);assert.equal(new T.Box3().setFromObject(loaded.scene).isEmpty(),false);
 }
});
test('scanned plant instances normalize translated source geometry and remain on native ground after updates',async()=>{
 const source=new T.Group(),mesh=new T.Mesh(new T.BoxGeometry(2,4,2),new T.MeshStandardMaterial());mesh.position.set(11,8,-5);source.add(mesh);
 let h=12;const parent=new T.Group(),plants=new ScannedVegetation(parent,[{x:101,z:201,variation:.1,rotation:0}],{x:100,z:200},()=>h,{loadModels:async()=>[{scene:source}]});assert.equal(await plants.promise,true);assert.equal(parent.children.length,1);
 const instance=parent.children[0],matrix=new T.Matrix4();instance.getMatrixAt(0,matrix);const bounds=new T.Box3().setFromBufferAttribute(instance.geometry.attributes.position).applyMatrix4(matrix);assert.ok(Math.abs(bounds.min.y-h)<.00001);assert.ok(Math.abs(bounds.getCenter(new T.Vector3()).x-1)<.00001);
 h=20;plants.update();instance.getMatrixAt(0,matrix);bounds.setFromBufferAttribute(instance.geometry.attributes.position).applyMatrix4(matrix);assert.ok(Math.abs(bounds.min.y-h)<.00001);
});
test('scan download failure keeps the caller fallback and never signals readiness',async()=>{let ready=false;const plants=new ScannedVegetation(new T.Group(),[],{x:0,z:0},()=>0,{loadModels:async()=>{throw Error('offline');},onReady:()=>ready=true});assert.equal(await plants.promise,false);assert.equal(ready,false);assert.equal(plants.groups.length,0);});
test('existing house 001 also parses with the official glTF reader without editing the source model',async()=>{
 globalThis.Image=class {set src(value){this.height=1;queueMicrotask(()=>this.onload?.());}};globalThis.self={URL:globalThis.URL};globalThis.createImageBitmap=async()=>({width:512,height:512,close(){}});
 const raw=readFileSync(new URL('../web/assets/models/buildings/village/village_building_01_lod0.glb',import.meta.url));const loaded=await new GLTFLoader().parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');const box=new T.Box3().setFromObject(loaded.scene);assert.ok(Number.isFinite(box.min.y)&&box.max.y>box.min.y);let meshes=0;loaded.scene.traverse(o=>{if(o.isMesh){meshes++;assert.ok(o.geometry.attributes.position.count>0);}});assert.ok(meshes>0);
});
test('dense vegetation selects nearby instances under explicit mobile draw budgets',async()=>{
 const source=new T.Group();source.add(new T.Mesh(new T.BoxGeometry(1,1,1),new T.MeshStandardMaterial()));const points=Array.from({length:200},(_,i)=>({x:i,z:0,variation:.1,rotation:0}));const plants=new ScannedVegetation(new T.Group(),points,{x:0,z:0},()=>0,{loadModels:async()=>[{scene:source}],limits:{grass:20,shrub:5}});await plants.promise;assert.ok(plants.groups[0].count<=20);plants.updateFocus({x:1000,z:1000});assert.equal(plants.groups[0].count,0);plants.updateFocus({x:100,z:0});assert.equal(plants.groups[0].count,20);
});
