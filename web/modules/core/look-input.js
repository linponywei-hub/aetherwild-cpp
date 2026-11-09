// Pointer ownership only; no inertia, timers or autonomous camera movement.
export class LookInput {
 constructor(){this.reset();}
 reset(){this.pointer=null;this.firstMouse=true;}
 begin(id,x,y,time){if(this.pointer!==null||![x,y,time].every(Number.isFinite))return false;this.pointer={id,x,y,time};return true;}
 move(id,x,y,time,enabled=true){if(!enabled){this.reset();return null;}const p=this.pointer;if(!p||p.id!==id)return null;const dx=x-p.x,dy=y-p.y,gap=time-p.time;p.time=time;if(![dx,dy,gap].every(Number.isFinite)||gap<0||gap>300||Math.abs(dx)>100||Math.abs(dy)>100){p.x=x;p.y=y;return null;}if(Math.hypot(dx,dy)<.5)return null;p.x=x;p.y=y;return {x:dx,y:dy};}
 end(id){if(this.pointer?.id===id)this.pointer=null;}
 mouse(x,y){if(this.pointer!==null)return null;if(this.firstMouse){this.firstMouse=false;return null;}if(!Number.isFinite(x)||!Number.isFinite(y)||Math.abs(x)>120||Math.abs(y)>120)return null;return {x,y};}
}
export function lookAngles(yaw,pitch,delta,sensitivity=1,touch=false){if(!delta)return {yaw,pitch};const s=Number.isFinite(sensitivity)?Math.max(.1,Math.min(3,sensitivity)):1,rate=touch?.0024:.0022;const next=yaw-delta.x*rate*s;return {yaw:Math.atan2(Math.sin(next),Math.cos(next)),pitch:Math.max(-1.3,Math.min(1.3,pitch-delta.y*rate*s))};}
