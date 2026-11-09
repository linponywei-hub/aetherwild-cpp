// Node.js built-in regression check: run with node tests/verdant_canopy.test.mjs
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
const canopy=readFileSync(new URL('../web/modules/world/verdant-canopy.js',import.meta.url),'utf8');
const city=readFileSync(new URL('../web/modules/world/city-scene.js',import.meta.url),'utf8');
const scene=readFileSync(new URL('../web/modules/core/scene.js',import.meta.url),'utf8');
assert.match(canopy,/export function addVerdantCanopy\(scene\)/);
assert.match(canopy,/r\.id === 'verdant'/);
assert.match(canopy,/for \(let i=0;i<6;i\+\+\)/);
assert.match(canopy,/visualOnly = true/);
assert.match(canopy,/assetPhase = 'procedural-phase1'/);
assert.match(city,/import \{addVerdantCanopy\} from '\.\/verdant-canopy\.js'/);
assert.match(city,/this\.verdantCanopy=addVerdantCanopy\(scene\)/);
assert.match(scene,/this\.cityScene=new CityScene\(this\)/);
console.log('Verdant canopy integration source checks passed (not a rendering or physics test).');
