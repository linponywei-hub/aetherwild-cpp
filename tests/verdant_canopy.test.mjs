import test from 'node:test';
import assert from 'node:assert/strict';
import {registerHooks} from 'node:module';
import * as T from '../web/vendor/three.module.js';
const webRoot=new URL('../web/',import.meta.url);
registerHooks({resolve(s,c,n){if(s.startsWith('/')&&c.parentURL?.startsWith(webRoot.href))return n(new URL(s.slice(1),webRoot).href,c);return n(s,c);}});
const {addVerdantCanopy}=await import('../web/modules/world/verdant-canopy.js');
test('external Verdant visual root preserves its city anchor without recreating removed floating bridges',()=>{
 const scene={root:new T.Group(),catalog:{regions:[{id:'verdant',position:{x:100,z:200},elevation:32}]}};
 const root=addVerdantCanopy(scene);assert.equal(root.parent,scene.root);
 assert.deepEqual(root.position.toArray(),[100,32,200]);assert.equal(root.userData.visualOnly,true);assert.equal(root.userData.assetPhase,'external-only');
 assert.equal(root.children.length,0,'obsolete procedural bridge/tree/stair meshes must stay removed');
});
test('Verdant root does not create objects when the region is absent',()=>{
 const scene={root:new T.Group(),catalog:{regions:[]}};assert.equal(addVerdantCanopy(scene),null);assert.equal(scene.root.children.length,0);
});
