import {harborHeight} from './landscape-model.js';
// Presentation only. Never writes to snapshots or sends a position to the server.
export class MovementPreview {
 constructor(catalog){this.catalog=catalog;this.pose=null;this.history=[];this.offset={x:0,y:0,z:0};this.pendingJump=null;this.sent=new Map();}
 noteInput(sequence,now){this.sent.set(sequence,now);for(const [key,at] of this.sent)if(at<now-2)this.sent.delete(key);}
 pressJump(now){this.pendingJump={at:now,sequence:Infinity};this.jumpRequested=true;}
 jumpSent(sequence){if(this.pendingJump&&this.pendingJump.sequence===Infinity)this.pendingJump.sequence=sequence;}
 ground(p){
  if(this.scene!=='world')return 0;
  let y=0;
  const patch=this.terrain;
  if(patch){const gx=(p.x-patch.x)/patch.step,gz=(p.z-patch.z)/patch.step,n=patch.side;if(gx>=0&&gz>=0&&gx<n-1&&gz<n-1){const x=Math.floor(gx),z=Math.floor(gz),tx=gx-x,tz=gz-z,h=patch.heights;const a=h[z*n+x]*(1-tx)+h[z*n+x+1]*tx,b=h[(z+1)*n+x]*(1-tx)+h[(z+1)*n+x+1]*tx;y=a*(1-tz)+b*tz;}}

  for(const r of this.catalog.regions){
   const h=r.elevation||0;if(h===(r.position.y||0))continue;if(!h)continue;
   const dx=p.x-r.position.x,dz=p.z-r.position.z;
   if(r.id==='verdant'&&Math.abs(dx)<7&&dz>=30&&dz<=150){const ramp=(150-dz)*h/120;if(ramp<=p.y+1.4)y=Math.max(y,ramp);}
   const disc=Math.hypot(dx,dz)<=94&&!(r.id==='verdant'&&Math.abs(dx)<8&&dz>30);
   const bridge=r.id==='astral'&&Math.abs(dx)<7&&dz>=85&&dz<=116;
   if((disc||bridge)&&h<=p.y+1.4)y=Math.max(y,h);
  }
  return Math.max(y,harborHeight(p,this.catalog));
 }
 blocked(p){
  if(this.scene!=='world')return Math.abs(p.x)>24||Math.abs(p.z)>29;
  return this.catalog.buildings.some(b=>p.y+1.7>b.position.y&&p.y<b.position.y+b.height&&Math.abs(p.x-b.position.x)<b.width/2+.45&&Math.abs(p.z-b.position.z)<b.depth/2+.45);
 }
 integrate(p,input,dt,jump=false){
  const length=Math.max(1,Math.hypot(input.x,input.z));
  const speed=this.hints.speed*(input.sprint&&this.hints.sprintAllowed?1.6:1);
  const x=input.x/length,z=input.z/length,c=Math.cos(input.yaw),s=Math.sin(input.yaw);
  const next={...p,x:Math.max(-this.limit,Math.min(this.limit,p.x+(x*c-z*s)*speed*dt))};
  if(this.blocked(next))next.x=p.x;
  next.z=Math.max(-this.limit,Math.min(this.limit,p.z+(-x*s-z*c)*speed*dt));
  if(this.blocked(next))next.z=p.z;
  const ground=this.ground(next);
  if(jump&&next.grounded){next.vy=this.hints.jumpSpeed;next.grounded=false;}
  if(next.grounded&&next.vy<=0&&Math.abs(p.y-ground)<=1.4)next.y=ground;
  else{next.grounded=false;next.vy-=this.hints.gravity*dt;next.y=p.y+next.vy*dt;if(next.y<=ground){next.y=ground;next.vy=0;next.grounded=true;}}
  return next;
 }
 reconcile(snapshot,now,rtt=0){
  const p=snapshot.player,m=snapshot._motion;
  if(!m){this.pose=null;this.history=[];this.pendingJump=null;this.jumpRequested=false;return;}
  this.limit=(snapshot.content?.worldLimit||this.catalog.worldLimit||1800)-10;this.terrain=snapshot.content?.terrainPatch||null;
  const old=this.position();
  const reset=!this.pose||this.id!==p.id||this.scene!==p.scene||this.epoch!==snapshot._sync?.epoch||Math.hypot(this.pose.x-p.position.x,this.pose.y-p.position.y,this.pose.z-p.position.z)>6;
  this.id=p.id;this.scene=p.scene;this.epoch=snapshot._sync?.epoch;this.hints=m;this.rtt=rtt;this.lastAuthority={...p.position};this.lastSnapshot=now;
  this.enabled=p.hp>0&&!m.lift;
  this.pose={...p.position,vy:m.velocityY,grounded:p.grounded};
  if(reset){this.history=[];this.offset={x:0,y:0,z:0};this.pendingJump=null;this.jumpRequested=false;return;}
  // Replay from the acknowledged input timestamp, including both network legs.
  // Older servers without timestamp mapping retain the bounded RTT fallback.
  const anchor=this.sent.get(m.sequence)??(now-Math.min(.3,Math.max(0,rtt/2)));
  const unacked=this.pendingJump&&this.pendingJump.sequence>m.sequence;
  let jumpAt=unacked?Math.max(anchor,this.pendingJump.at):Infinity;
  let serverJump=m.jumpPending;
  if(this.enabled)for(const h of this.history){
   const start=Math.max(anchor,h.start),end=Math.min(now,h.end);
   if(end<=start)continue;
   const jump=serverJump||(jumpAt<=end);serverJump=false;if(jump)jumpAt=Infinity;
   this.pose=this.integrate(this.pose,h.input,end-start,jump);
  }
  if(this.pendingJump&&!unacked)this.pendingJump=null;
  this.lastCorrection=old?Math.hypot(old.x-this.pose.x,old.y-this.pose.y,old.z-this.pose.z):0;if(this.lastCorrection>1)this.largeCorrections=(this.largeCorrections||0)+1;
  if(old&&this.lastCorrection<3&&this.enabled)this.offset={x:old.x-this.pose.x,y:old.y-this.pose.y,z:old.z-this.pose.z};
  else this.offset={x:0,y:0,z:0};
 }
 advance(dt,now,input){
  if(!this.pose)return null;
  dt=Math.min(.1,Math.max(0,dt));
  if(this.enabled&&now-this.lastSnapshot<=.75){
   this.history.push({start:now-dt,end:now,input:{...input}});
   this.history=this.history.filter(h=>h.end>now-1);
   const steps=Math.max(1,Math.ceil(dt/(1/60))),step=dt/steps;
   for(let i=0;i<steps;i++)this.pose=this.integrate(this.pose,input,step,i===0&&this.jumpRequested);
   // Bound visual drift on a bad connection. This is never an authority position.
   const dx=this.pose.x-this.lastAuthority.x,dz=this.pose.z-this.lastAuthority.z,d=Math.hypot(dx,dz);
   const drift=Math.min(8,Math.max(3,this.hints.speed*1.6*((this.rtt||0)/2+.2)));if(d>drift){this.pose.x=this.lastAuthority.x+dx*drift/d;this.pose.z=this.lastAuthority.z+dz*drift/d;}
  }
  this.jumpRequested=false;
  const decay=Math.exp(-dt*14);
  for(const k of ['x','y','z'])this.offset[k]*=decay;
  return this.position();
 }
 position(){return this.pose?{x:this.pose.x+this.offset.x,y:this.pose.y+this.offset.y,z:this.pose.z+this.offset.z}:null;}
}
