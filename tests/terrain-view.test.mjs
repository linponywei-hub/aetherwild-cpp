import test from 'node:test';
import assert from 'node:assert/strict';
import * as T from '../web/vendor/three.module.js';
import {terrainMaterial,clipTerrainPatch} from '../web/modules/world/terrain-material.js';
import {terrainMesh} from '../web/modules/world/content-scene.js';
import {EditorTerrain} from '../web/modules/modeling/editor-terrain.js';

const patch=(x=0,z=0)=>({side:65,step:16,x:Math.floor(x/512)*512-256,z:Math.floor(z/512)*512-256,version:2,heights:Array.from({length:4225},(_,i)=>Math.floor(i/65)+i%65)});
test('terrain material retains slope, climate and fine detail when native patch clipping is installed',()=>{
 const material=terrainMaterial();clipTerrainPatch(material,patch());const shader={uniforms:{},vertexShader:'#include <begin_vertex>',fragmentShader:'#include <color_fragment>\n#include <normal_fragment_maps>\n#include <clipping_planes_fragment>'};material.onBeforeCompile(shader);
 for(const token of ['awRock','awIce','dFdx','discard'])assert.ok(shader.fragmentShader.includes(token),token);
 assert.ok(shader.vertexShader.includes('awClimate'));assert.ok(shader.uniforms.patchBounds);assert.ok(material.customProgramCacheKey().includes('v3'));
});
test('terrain geometry carries native sample heights and climate data without raising or moving vertices',()=>{
 const grid={side:2,step:16,x:10,z:20,heights:[1,2,3,4]};const mesh=terrainMesh(grid,'#718866');const pos=mesh.geometry.attributes.position;
 assert.deepEqual(Array.from(pos.array),[10,1,20,26,2,20,10,3,36,26,4,36]);assert.equal(mesh.geometry.attributes.awClimate.count,4);
});
test('editor placement reads and caches server heights rather than generating its own relief',async()=>{
 let calls=0;const view=new EditorTerrain(new T.Scene(),{fetcher:async(url,options)=>{calls++;assert.equal(url,'/api/terrain');assert.deepEqual(JSON.parse(options.body),{x:0,z:0});return {ok:true,json:async()=>({terrain:patch()})};}});
 view.catalog={worldLimit:48000,landscape:{version:2}};
 assert.equal(await view.sampleHeight(0,0),32);assert.equal(await view.sampleHeight(0,0),32);assert.equal(calls,1);
 await assert.rejects(()=>view.tile(48001,0));assert.equal(calls,1);
});
test('outdated tile replies cannot replace terrain after the editor moves to another region',async()=>{
 const pending=[];const view=new EditorTerrain(new T.Scene(),{fetcher:(_url,options)=>new Promise(resolve=>pending.push({body:JSON.parse(options.body),resolve}))});
 view.catalog={worldLimit:48000,landscape:{version:2,water:[],rivers:[],biomes:[]}};view.global=terrainMesh({side:2,step:10000,x:-1000,z:-1000,heights:[0,0,0,0]},'#718866');
 const a=view.ensurePatch(0,0),b=view.ensurePatch(1024,0);
 pending[1].resolve({ok:true,json:async()=>({terrain:patch(1024,0)})});await b;
 pending[0].resolve({ok:true,json:async()=>({terrain:patch(0,0)})});await a;
 assert.equal(view.patch.x,768);assert.equal(view.local.geometry.attributes.position.getX(0),768);
});
