import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {surfaceAt,gridHeight,harborHeight} from '../web/landscape-model.js';
const model=JSON.parse(fs.readFileSync(new URL('../data/atlas_surface.json',import.meta.url)));
const world=([x,z])=>({x:(x-633)*64,z:(z-578)*64});
test('reference coast, northwest snow, western forest and southeast desert stay in authored locations',()=>{for(const [pixel,kind] of [[[1180,550],'water'],[[400,100],'snow'],[[200,500],'forest'],[[1100,950],'desert']]){const p=world(pixel);assert.equal(surfaceAt(p.x,p.z,model),kind);}});
test('harbor deck and boarding ramp heights match collision authoring',()=>{const catalog={landscape:model,regions:model.harbors.map(h=>({id:h.region,position:{x:0,z:0}}))};assert.equal(harborHeight({x:30,z:30,y:4},catalog),4);const h=model.harbors[0],z=h.ramp.z+h.ramp.depth*.25;const isolated={landscape:{harbors:[h]},regions:[{id:h.region,position:{x:0,z:0}}]};assert.ok(Math.abs(harborHeight({x:0,z,y:1},isolated)-1)<1e-9);assert.equal(harborHeight({x:20000,z:20000,y:0},catalog),-Infinity);});
test('terrain sample blends server heights and refuses extrapolation outside patch',()=>{const grid={side:2,step:10,x:0,z:0,heights:[0,10,10,20]};assert.equal(gridHeight(grid,5,5),10);assert.equal(gridHeight(grid,-1,5),null);});
