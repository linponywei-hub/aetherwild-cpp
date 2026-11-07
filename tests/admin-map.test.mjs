import {test} from 'node:test';
import assert from 'node:assert/strict';
import {MapViewport} from '../web/admin-map.js';
const regions=[{center:{x:-25000,z:-20000},radius:2800},{center:{x:24000,z:18000},radius:850}];
test('fit includes full region bounds and preserves world/screen round trip',()=>{const v=new MapViewport();v.fit(regions,1000,700);for(const r of regions){const p=v.screen(r.center,1000,700);assert(p.x>=0&&p.x<=1000&&p.y>=0&&p.y<=700);const q=v.world(p.x,p.y,1000,700);assert(Math.abs(q.x-r.center.x)<1e-8);assert(Math.abs(q.z-r.center.z)<1e-8);}});
test('zoom stays anchored to cursor, clamps scale, and pan follows pointer',()=>{const v=new MapViewport();v.fit(regions,1000,700);const a=v.world(150,180,1000,700);v.zoom(2,150,180,1000,700);const b=v.world(150,180,1000,700);assert(Math.abs(a.x-b.x)<1e-8);assert(Math.abs(a.z-b.z)<1e-8);const p=v.screen(a,1000,700);v.pan(40,-20);const q=v.screen(a,1000,700);assert(Math.abs(q.x-p.x-40)<1e-8);assert(Math.abs(q.y-p.y+20)<1e-8);v.zoom(1e9,0,0,1000,700);assert.equal(v.scale,v.base*64);v.zoom(1e-20,0,0,1000,700);assert.equal(v.scale,v.base*.5);});
