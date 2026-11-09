import test from 'node:test';
import assert from 'node:assert/strict';
import {renderFrame} from '../web/modules/rendering/render-passes.js';
test('world trees finish before depth is cleared and weapon draws last without clearing color',()=>{const calls=[],renderer={autoClear:true,render(scene){calls.push([scene,this.autoClear]);},clearDepth(){calls.push(['clear depth']);}};renderFrame(renderer,'world','camera','weapon','weapon camera');assert.deepEqual(calls,[['world',true],['clear depth'],['weapon',false]]);assert.equal(renderer.autoClear,true);});
test('software draws a single world pass and failed overlay restores renderer settings',()=>{const calls=[],soft={software:true,render:s=>calls.push(s)};renderFrame(soft,'world');assert.deepEqual(calls,['world']);const bad={autoClear:true,clearDepth(){},render(s){if(s==='weapon')throw Error('GPU lost');}};assert.throws(()=>renderFrame(bad,'world',null,'weapon'));assert.equal(bad.autoClear,true);});
