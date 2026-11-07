import test from 'node:test';
import assert from 'node:assert/strict';
import {ControlBindings,DEFAULT_BINDINGS} from '../web/controls.js';
const storage=()=>{const values=new Map();return {getItem:k=>values.get(k),setItem:(k,v)=>values.set(k,v)};};
test('rebound movement persists across reload and old key no longer moves',()=>{const s=storage(),a=new ControlBindings(s);a.bind('forward','KeyI');assert.equal(a.action('KeyI'),'forward');assert.equal(a.action('KeyW'),null);const b=new ControlBindings(s);assert.equal(b.action('KeyI'),'forward');b.reset();assert.deepEqual(b.bindings,DEFAULT_BINDINGS);});
test('duplicate or reserved key fails without changing bindings',()=>{const a=new ControlBindings(storage()),before={...a.bindings};assert.throws(()=>a.bind('forward','KeyS'));assert.throws(()=>a.bind('forward','Escape'));assert.deepEqual(a.bindings,before);});
test('malformed saved bindings fall back and arrows retain default movement aliases',()=>{const s=storage();s.setItem('aw-keybindings','not json');const a=new ControlBindings(s);assert.equal(a.action('ArrowUp'),'forward');a.bind('jump','ArrowUp');assert.equal(a.action('ArrowUp'),'jump');});
