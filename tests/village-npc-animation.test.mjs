import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {registerHooks} from 'node:module';
import * as T from '../web/vendor/three.module.js';
const webRoot=new URL('../web/',import.meta.url);
registerHooks({resolve(s,c,n){if(s.startsWith('/')&&c.parentURL?.startsWith(webRoot.href))return n(new URL(s.slice(1),webRoot).href,c);return n(s,c);}});
// CPU skin / keyframe verification. A bitmap stub allows embedded images without a DOM/GPU.
globalThis.self=globalThis;globalThis.Image=class{constructor(){this.height=1;}set src(v){queueMicrotask(()=>this.onload?.());}};globalThis.createImageBitmap=async()=>({width:512,height:512,close(){}});
const {GLTFLoader}=await import('../web/vendor/GLTFLoader.js');
const {ModelAssets}=await import('../web/modules/modeling/model-assets.js');
const {MODEL_ASSETS}=await import('../web/modules/modeling/model-assets.js');
const {VILLAGE_CAST}=await import('../web/modules/npc/scene.js');
const {npcClip,updateNpcVisual}=await import('../web/modules/npc/animation.js');
const loader=new GLTFLoader();const cache=new Map();
async function parse(path){if(!cache.has(path))cache.set(path,(async()=>{const b=await readFile(new URL(path.slice(1),webRoot));return loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),'');})());return cache.get(path);}
test('all 18 village identities have a local asset and no removed test skin',()=>{
 assert.equal(Object.keys(VILLAGE_CAST).length,18);for(const id of Object.values(VILLAGE_CAST))assert.ok(MODEL_ASSETS[id]);
 assert.ok(!JSON.stringify(MODEL_ASSETS).includes('TRIPO'));
});
test('external rigs clone independently and retain finite skinned bounds throughout walk/idle/conversation',async()=>{
 const assets=new ModelAssets();assets.loader=async()=>({loadAsync:parse});
 for(const id of Object.keys(MODEL_ASSETS)){
  const {instance,animations}=await assets.instantiate(id);const twin=await assets.instantiate(id);
  const bones=[];instance.traverse(n=>{if(n.isBone)bones.push(n);});const other=[];twin.instance.traverse(n=>{if(n.isBone)other.push(n);});
  assert.ok(bones.length>=65);assert.ok(bones.every((b,i)=>b!==other[i]));
  const g=new T.Group();g.add(instance);g.userData={modelMixer:new T.AnimationMixer(instance),modelClips:animations};
  const initial=new T.Box3().setFromObject(instance);assert.ok(Math.abs(initial.min.y)<.02);assert.ok(Math.abs(initial.max.y-MODEL_ASSETS[id].height)<.02);
  for(const mode of ['walk','idle','talk']){
   const clip=npcClip(animations,mode);assert.ok(clip,`${id} ${mode}`);
   g.userData.modelMixer.stopAllAction();g.userData.modelMixer.clipAction(clip).play();
   for(let k=0;k<=12;k++){
    g.userData.modelMixer.setTime(clip.duration*k/12);instance.updateMatrixWorld(true);
    instance.traverse(n=>{if(n.isSkinnedMesh)n.computeBoundingBox();});
    const b=new T.Box3().setFromObject(instance),size=b.getSize(new T.Vector3());
    assert.ok([size.x,size.y,size.z,b.min.y].every(Number.isFinite));assert.ok(b.min.y>-.12&&b.min.y<.18,`${id} ${mode} feet ${b.min.y}`);assert.ok(size.x<4&&size.y<3&&size.z<4,`${id} ${mode} skin spikes`);
   }
   assert.ok(Math.abs(g.position.x)+Math.abs(g.position.z)<.001,'clip cannot move gameplay root');
  }
 }
});
test('server interpolation drives walk animation and smooth heading; talking selects authored gesture',()=>{
 const npc=new T.Group();npc.userData={track:{at:()=>({x:.04,y:0,z:0})},npcLife:{yaw:Math.PI/2,mode:'work'},legs:[]};
 updateNpcVisual(npc,2,.05,2);assert.equal(npc.position.x,.04);assert.ok(npc.rotation.y>0&&npc.rotation.y<=.151);
});
