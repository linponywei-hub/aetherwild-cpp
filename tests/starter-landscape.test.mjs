import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
import * as T from '../web/vendor/three.module.js';
import {starterCenter,starterScatter,decorativePointAllowed,StarterLandscape} from '../web/modules/world/starter-landscape.js';
import {createStarterTextures} from '../web/modules/world/starter-surface.js';
import {terrainMaterial,clipTerrainPatch} from '../web/modules/world/terrain-material.js';
const landscape=JSON.parse(readFileSync(new URL('../data/atlas_surface.json',import.meta.url))),config=landscape.starterVillage,center=starterCenter(config);
const houses=[{region:'windchime',style:'windchime|house',position:{x:center.x+24,z:center.z+24},width:16,depth:14}];
const catalog={landscape,buildings:houses};
test('village dressing is deterministic, bounded and excludes plaza, roads and actual house footprints',()=>{
 const points=starterScatter(catalog);assert.deepEqual(points,starterScatter(catalog));assert.ok(points.length>1200&&points.length<=3200);
 for(const p of points){assert.ok(Math.hypot(p.x-center.x,p.z-center.z)<193);assert.ok(decorativePointAllowed(p.x,p.z,config,houses,center));}
 assert.equal(decorativePointAllowed(center.x,center.z,config,houses,center),false);
 assert.equal(decorativePointAllowed(center.x+24,center.z+24,config,houses,center),false);
});
test('groundcover rests on supplied native height, fits two draw calls and does not rebuild per frame',()=>{
 const root=new T.Group(),view=new StarterLandscape(root,catalog,()=>17.25);
 assert.equal(view.root.children.length,2);const grass=view.root.children.find(m=>m.name==='starter-grass');
 const pos=grass.geometry.attributes.position;let min=Infinity;for(let i=0;i<pos.count;i++)min=Math.min(min,pos.getY(i));assert.equal(min,17.25);
 assert.ok(pos.count<=3200*5*6);assert.equal(grass.castShadow,false);assert.ok(grass.geometry.boundingSphere.radius<230);
 const pebbles=view.root.children.find(m=>m.name==='starter-pebbles');const normals=pebbles.geometry.attributes.normal;for(let i=0;i<normals.count;i++)assert.ok(normals.getY(i)>0,'pebbles must face the camera above ground');
 const fallback=new StarterLandscape(new T.Group(),catalog,()=>0,{software:true});assert.equal(fallback.root.children.length,0);
});
test('PBR terrain becomes active only when all six local maps succeed, and retains fallback after any failure',()=>{
 const pending=[];const loader={load:(path,ok,_progress,fail)=>{assert.ok(path.startsWith('/art/terrain/'));pending.push({ok,fail});return new T.Texture();}};
 const loaded=createStarterTextures(loader);for(const p of pending.slice(0,5))p.ok();assert.equal(loaded.ready.value,0);pending[5].ok();assert.equal(loaded.ready.value,1);
 pending.length=0;const failed=createStarterTextures(loader);pending[0].fail();for(const p of pending.slice(1))p.ok();assert.equal(failed.ready.value,0);
 assert.equal(loaded.maps.grassColor.colorSpace,T.SRGBColorSpace);assert.equal(loaded.maps.grassNormal.colorSpace,T.NoColorSpace);
});
test('PBR color, normals, roughness and AO survive native terrain clipping',()=>{
 const material=terrainMaterial({starter:config});clipTerrainPatch(material,{x:0,z:0,side:65,step:16});
 const shader={uniforms:{},vertexShader:'#include <begin_vertex>',fragmentShader:'#include <color_fragment>\n#include <roughnessmap_fragment>\n#include <normal_fragment_maps>\n#include <aomap_fragment>\n#include <clipping_planes_fragment>'};material.onBeforeCompile(shader);
 for(const part of ['awGroundColor','awGroundNormal','roughnessFactor','indirectDiffuse','discard'])assert.ok(shader.fragmentShader.includes(part));
 assert.equal(shader.uniforms.awVillage.value.x,center.x);assert.ok(material.customProgramCacheKey().includes('starter'));
});
test('all shipped photographic textures match the source manifest and have a bounded total payload',()=>{
 const manifest=JSON.parse(readFileSync(new URL('../assets/reference/terrain/sources.json',import.meta.url)));let bytes=0;assert.equal(manifest.maps.length,6);
 for(const map of manifest.maps){const raw=readFileSync(new URL('../assets/reference/terrain/'+map.file,import.meta.url));assert.equal(createHash('sha256').update(raw).digest('hex'),map.sha256);assert.equal(map.license,'CC0-1.0');bytes+=raw.length;}
 assert.ok(bytes<3_000_000);
});
