import assert from 'node:assert/strict';
import {verdantDeckTiles,verdantSurface,verdantBridges} from '../web/modules/world/verdant-layout.js';
// Continuous walking paths over each deck/bridge join, and real open air between islands.
for(const b of verdantBridges){for(let distance=12;distance<68;distance+=.1){const h=verdantSurface(Math.sin(b.angle)*distance,Math.cos(b.angle)*distance,1);assert.notEqual(h,null,`gap on bridge arm ${b.angle} at ${distance}`);assert.ok(h>=-.501&&h<=0);}}
assert.equal(verdantSurface(28,28,1),null,'Old monolithic city floor must be gone');
assert.equal(verdantSurface(0,0,1),null,'Hollow-tree stairwell must stay open');
assert.equal(verdantSurface(0,12,-32),null,'Walking under the city must not snap to the deck');
const tiles=verdantDeckTiles();assert.equal(tiles.length,196);
assert.equal(new Set(tiles.map(t=>`${t.x.toFixed(2)},${t.z.toFixed(2)}`)).size,196,'No doubled deck panels');
console.log('Verdant: four continuous bridge routes, separated islands and open stairwell passed');
