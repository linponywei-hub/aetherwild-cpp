import test from 'node:test';
import assert from 'node:assert/strict';
import {registerHooks} from 'node:module';
import * as T from '../web/vendor/three.module.js';

// Resolve browser-root imports when checking the actual browser module graph in Node.
const webRoot=new URL('../web/',import.meta.url);
registerHooks({resolve(specifier,context,next){
 if(specifier.startsWith('/')&&context.parentURL?.startsWith(webRoot.href))return next(new URL(specifier.slice(1),webRoot).href,context);
 return next(specifier,context);
}});
const {RealmScene}=await import('../web/modules/core/scene.js');
const {makeNpcs}=await import('../web/modules/npc/scene.js');

test('scene composition exposes modeling, actor and monster adapters through the existing API',()=>{
 for(const name of ['loadVillageHouse','refreshVillageCollision','human','makeGoblinPreview','goblinTest','wolf','updateGoblinAI'])assert.equal(typeof RealmScene.prototype[name],'function',name);
});
test('extracted building collision adapter still updates movement prediction after transformed bounds change',()=>{
 const g=new T.Group(),model=new T.Mesh(new T.BoxGeometry(4,6,8));g.add(model);g.position.set(10,2,20);g.userData={villageGLB:model,villageIndex:1};
 const view={preview:{}};RealmScene.prototype.refreshVillageCollision.call(view,g);
 assert.equal(view.preview.villageCollisionBounds.length,1);
 assert.deepEqual(view.preview.villageCollisionBounds[0].min.toArray(),[8,-1,16]);
 g.position.x=30;RealmScene.prototype.refreshVillageCollision.call(view,g);
 assert.equal(view.preview.villageCollisionBounds.length,1);
 assert.equal(view.preview.villageCollisionBounds[0].min.x,28);
});
test('NPC module retains identity, world anchors, nameplates and remote tracks',()=>{
 const registered=[];const view={catalog:{npcs:[{id:'guild',name:'艾琳',role:'guild',region:'windchime',position:{x:4,y:2,z:6}}]},root:new T.Group(),npcObjects:new Map(),human:()=>new T.Group(),nameplates:{register:(...args)=>registered.push(args)}};
 makeNpcs(view);const npc=view.npcObjects.get('guild');assert.deepEqual(npc.position.toArray(),[4,2,6]);assert.equal(npc.userData.data.id,'guild');assert.equal(npc.userData.externalVillager,true);assert.ok(npc.userData.track);assert.equal(registered[0][0],'npc:guild');
});
