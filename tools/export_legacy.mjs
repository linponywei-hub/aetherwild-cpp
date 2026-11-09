import vm from 'node:vm';
import fs from 'node:fs/promises';
import path from 'node:path';
const project=path.resolve(import.meta.dirname,'..');
if(!process.argv[2])throw new Error('Usage: node --experimental-vm-modules tools/export_legacy.mjs /absolute/path/to/extracted/Aetherwild_v0.30.1/public');
const root=path.resolve(process.argv[2]);
const context=vm.createContext({console,structuredClone,crypto:globalThis.crypto,TextEncoder,TextDecoder,URL,Math,Date, setTimeout,clearTimeout});
const cache=new Map();
async function load(file){file=file.split('?')[0];if(cache.has(file))return cache.get(file);const m=new vm.SourceTextModule(await fs.readFile(file,'utf8'),{context,identifier:file});cache.set(file,m);return m;}
// Link one graph before evaluating; the catalog has legitimate ESM cycles.
const entry=new vm.SourceTextModule(`import * as w from './world-data.js';import * as e from './equipment.js';import * as p from './progression.js';import * as l from './life-systems.js';import * as a from './activity-rules.js';import * as q from './quest-data.js';import * as b from './boss-loot.js';import * as ar from './arrow-rules.js';export {w,e,p,l,a,q,b,ar};`,{context,identifier:path.join(root,'export.mjs')});
await entry.link((s,m)=>load(path.resolve(path.dirname(m.identifier),s)));await entry.evaluate();
let out={};for(const[group,ns]of Object.entries(entry.namespace))for(const[key,v]of Object.entries(ns))if(typeof v!=='function'&&typeof v!=='undefined')out[key]=v;
await fs.writeFile(path.join(project,'data/legacy_catalog.json'),JSON.stringify(out,null,2));console.log(Object.fromEntries(Object.entries(out).map(([k,v])=>[k,Array.isArray(v)?v.length:typeof v==='object'&&v?Object.keys(v).length:typeof v])));
