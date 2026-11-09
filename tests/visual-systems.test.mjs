import test from 'node:test';import assert from 'node:assert/strict';
import {lightingAt,EnvironmentClock,CYCLE_SECONDS} from '../web/modules/world/day-night.js';
import {mapPoint,headingTriangle} from '../web/modules/ui/minimap.js';
import {weaponPose,BowInput} from '../web/modules/combat/weapon-pose.js';
test('1800 real seconds is one cycle, noon and midnight differ, server sync is shared',()=>{
 assert.equal(CYCLE_SECONDS,1800);assert.equal(lightingAt(0).clock,'00:00');assert.equal(lightingAt(900).clock,'12:00');
 assert.equal(lightingAt(900).daylight,1);assert.equal(lightingAt(0).daylight,0);assert.equal(lightingAt(1800).phase,0);assert.equal(lightingAt(450).clock,'06:00');
 const a=new EnvironmentClock(),b=new EnvironmentClock();a.sync({unixSeconds:123,cycleSeconds:1800},1000);b.sync({unixSeconds:123,cycleSeconds:1800},1000);assert.equal(a.now(2000),124);assert.equal(a.light(2000).phase,b.light(2000).phase);
});
test('minimap player stays exactly centered after long moves and camera rotation matches world yaw',()=>{
 for(const center of [{x:0,z:0},{x:20000,z:-8000},{x:-123,z:500}])assert.deepEqual(mapPoint(center,center,216,180),{x:108,y:90});
 assert.equal(mapPoint({x:100,z:0},{x:0,z:0},216,180).x,163);
 const north=headingTriangle(0)[0],west=headingTriangle(Math.PI/2)[0];assert.equal(north[1],-6);assert.ok(Math.abs(west[0]+6)<1e-8);
});
test('dual daggers occupy opposite lower corners even on a narrow display',()=>{
 for(const aspect of [1,16/9,21/9]){const p=weaponPose('daggers',aspect);assert.ok(p.left[0]<-.3&&p.right[0]>.3);assert.equal(p.left[1],p.right[1]);assert.ok(p.left[1]<-.3);}
});
test('release waits for draw ACK; blur cancellation cannot fire an arrow',async()=>{
 const sent=[];let ready;const c=new BowInput((type)=>{sent.push(type);return type==='bow_draw'?new Promise(r=>ready=r):true;});
 c.begin();const fired=c.release();await new Promise(r=>setTimeout(r,0));assert.deepEqual(sent,['bow_draw']);ready(true);await fired;assert.deepEqual(sent,['bow_draw','bow_release']);
 const d=new BowInput(async t=>{sent.push(t);return true;});await d.begin();await d.cancel();assert.equal(await d.release(),undefined);assert.equal(sent.at(-1),'bow_cancel');
});
test('cancelling while a draw ACK is pending suppresses queued release and cancels server draw',async()=>{
 const sent=[];let ready;const c=new BowInput(t=>{sent.push(t);return t==='bow_draw'?new Promise(r=>ready=r):true;});c.begin();const release=c.release();const cancel=c.cancel();await new Promise(r=>setTimeout(r,0));ready(true);await Promise.all([release,cancel]);assert.deepEqual(sent,['bow_draw','bow_cancel']);
});
