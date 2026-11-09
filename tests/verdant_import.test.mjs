import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {GLTFLoader} from '../web/vendor/GLTFLoader.js';
import * as T from '../web/vendor/three.module.js';
import {bakeImportedGeometry} from '../web/modules/world/verdant-cc0-models.js';
for(const name of ['timber-deck','rope-bridge']){
 const bytes=readFileSync(new URL(`../web/assets/models/verdant/rebuilt/${name}.glb`,import.meta.url));
 const source=await new Promise((resolve,reject)=>new GLTFLoader().parse(bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength),'',resolve,reject));
 source.scene.updateMatrixWorld(true);
 source.scene.traverse(mesh=>{
  if(!mesh.isMesh)return;
  const expected=new T.Box3().setFromObject(mesh),geometry=bakeImportedGeometry(mesh);geometry.computeBoundingBox();
  assert.ok(geometry.attributes.position.array instanceof Float32Array);
  for(const axis of ['x','y','z']){
   assert.ok(Math.abs(geometry.boundingBox.min[axis]-expected.min[axis])<.0001,`${name} min ${axis}`);
   assert.ok(Math.abs(geometry.boundingBox.max[axis]-expected.max[axis])<.0001,`${name} max ${axis}`);
  }
 });
}
console.log('Imported quantized GLB deck/bridge node transforms preserve actual metre bounds');
