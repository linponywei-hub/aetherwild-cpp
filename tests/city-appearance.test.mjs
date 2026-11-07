import test from 'node:test';import assert from 'node:assert/strict';
import {buildingParts,cityTheme,desiredCells} from '../web/city-architecture.js';
import {weaponParts,humanoidParts,nameplateVisible} from '../web/actor-model.js';
test('city themes and palace/lamp recipes are distinct, finite and bounded',()=>{
 const themes=new Set(['windchime','verdant','cinder','frosthold','astral','sundune','tidehaven','aw_crownreach'].map(cityTheme));assert.equal(themes.size,8);
 for(const [kind,w,d,h]of [['palace',170,105,78],['house',14,14,16],['lamp',.45,.45,5],['tower',24,24,38]]){const parts=buildingParts({region:'aw_crownreach',style:'aw_crownreach|'+kind,width:w,depth:d,height:h});assert.ok(parts.length>1);assert.ok(parts.every(p=>['x','y','z','sx','sy','sz'].every(k=>Number.isFinite(p[k]))));if(kind==='lamp')assert.ok(parts.some(p=>p.glow));}
});
test('weapons are visible geometry for every basic weapon family, unarmed stays empty',()=>{for(const type of ['sword','bow','staff','daggers','shieldblade','cannon','spear','hammer'])assert.ok(weaponParts(type).length>0,type);assert.equal(weaponParts('unarmed').length,0);assert.notDeepEqual(humanoidParts('merchant','a').body,humanoidParts('healer','b').body);});
test('scenery streaming stays at nine cells, nearest cell first, and nameplates use nearby range',()=>{for(const x of [0,160,-160,30000]){const cells=desiredCells(x,-9200);assert.equal(cells.length,9);assert.equal(cells[0].d,0);assert.equal(new Set(cells.map(c=>c.x+':'+c.z)).size,9);}assert.equal(nameplateVisible({x:50,y:0,z:0},{x:0,y:0,z:0}),true);assert.equal(nameplateVisible({x:100,y:0,z:0},{x:0,y:0,z:0}),false);});
