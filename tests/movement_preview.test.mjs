import test from 'node:test';
import assert from 'node:assert/strict';
import {MovementPreview} from '../web/movement-preview.js';
const catalog={regions:[],buildings:[]};
const motion={sequence:0,velocityY:0,jumpPending:false,lift:'',speed:5,sprintAllowed:true,gravity:22,jumpSpeed:8};
const snapshot=(x=0,extra={})=>({player:{id:'p',hp:100,scene:'world',grounded:true,position:{x,y:0,z:0}},_sync:{epoch:'a'},_motion:{...motion},...extra});
const right={x:1,z:0,yaw:0,sprint:false},idle={...right,x:0};
test('walking responds on first frame while a 300ms server reply is still pending',()=>{
 const p=new MovementPreview(catalog),s=snapshot();p.reconcile(s,0,.3);
 const before=JSON.stringify(s);
 const first=p.advance(1/60,1/60,right);
 assert.ok(first.x>.07&&first.x<.1);
 for(let i=2;i<=17;i++)p.advance(1/60,i/60,right);
 assert.ok(p.position().x>1.3,'motion must not wait 300ms for a reply');
 assert.equal(JSON.stringify(s),before,'prediction cannot write into authoritative data');
});
test('300ms delayed snapshots correct smoothly instead of resetting every frame',()=>{
 const p=new MovementPreview(catalog);p.reconcile(snapshot(),0,.3);
 let last=0,maxStep=0;
 for(let i=1;i<=120;i++){
  const now=i/60;
  if(i%6===0)p.reconcile(snapshot(5*Math.max(0,now-.3)),now,.3);
  const v=p.advance(1/60,now,right);
  maxStep=Math.max(maxStep,Math.abs(v.x-last));last=v.x;
 }
 assert.ok(last>7,'walking continues under 300ms delay');
 assert.ok(maxStep<.3,'no large per-frame reconciliation snap');
});
test('local jump responds before acknowledgement and authoritative denial reconciles',()=>{
 const p=new MovementPreview(catalog);p.reconcile(snapshot(),0,.3);
 p.pressJump(.001);p.jumpSent(10);
 assert.ok(p.advance(1/60,1/60,idle).y>.1);
 p.reconcile(snapshot(0,{_motion:{...motion,sequence:10}}),.2,.3);
 for(let i=1;i<=40;i++)p.advance(1/60,.2+i/60,idle);
 assert.ok(Math.abs(p.position().y)<.03,'denied jump must return to server ground');
});
test('preview preserves walls, diagonal speed, teleports, death and disconnect bounds',()=>{
 const p=new MovementPreview({...catalog,buildings:[{position:{x:1,y:0,z:0},width:.2,depth:10,height:10}]});p.reconcile(snapshot(),0);
 for(let i=1;i<=30;i++)p.advance(1/60,i/60,right);
 assert.ok(p.position().x<.5);
 p.reconcile(snapshot(100),.6);assert.equal(p.position().x,100);
 const d=new MovementPreview(catalog);d.reconcile(snapshot(),0);
 d.advance(.1,.1,{...right,z:1});assert.ok(Math.hypot(d.position().x,d.position().z)<=.501);
 for(let i=1;i<=120;i++)d.advance(1/60,.1+i/60,right);
 const stopped=d.position().x;d.advance(.1,3,right);assert.equal(d.position().x,stopped);
 d.reconcile(snapshot(2,{player:{...snapshot().player,hp:0,position:{x:2,y:0,z:0}}}),4);
 d.advance(.1,4.1,right);assert.equal(d.position().x,2);
});
test('old server without movement hints uses authoritative rendering',()=>{
 const p=new MovementPreview(catalog);p.reconcile({...snapshot(),_motion:undefined},0);assert.equal(p.advance(.02,.02,right),null);
});
