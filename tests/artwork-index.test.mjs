import {test} from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {artwork} from '../web/artwork-index.js';
import {artworkNames} from '../web/artwork-names.js';
test('artwork registry only maps ready exact item IDs with their actual names',()=>{
 const m=JSON.parse(fs.readFileSync(new URL('../data/artwork/manifest.json',import.meta.url)));
 assert.ok(Object.keys(artwork).length);
 for(const [id,url] of Object.entries(artwork)){assert.equal(url,'/art/items/'+id+'.png');assert.equal(m.entries[id].status,'ready');assert.equal(artworkNames[id],m.entries[id].name);assert.ok(fs.existsSync(new URL('../'+m.entries[id].path,import.meta.url)));}
 assert.equal(Object.keys(artwork).length,Object.values(m.entries).filter(e=>e.status==='ready').length);
});
