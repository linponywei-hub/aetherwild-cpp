export class RemoteTrack {
 constructor({changesOnly=false}={}){this.changesOnly=changesOnly;this.samples=[];}
 push(position,time){if(!Number.isFinite(time)||!['x','y','z'].every(k=>Number.isFinite(position[k])))return;const last=this.samples.at(-1);if(last&&time<=last.time)return;const distance=last?Math.hypot(position.x-last.x,position.y-last.y,position.z-last.z):0;if(last&&(distance>20||time-last.time>3))this.samples=[];if(this.changesOnly&&last&&distance<.0001)return;this.samples.push({...position,time});while(this.samples.length>32||this.samples.length>2&&this.samples[1].time<time-3)this.samples.shift();}
 at(time){const a=this.samples;if(!a.length)return null;if(time<=a[0].time)return a[0];for(let i=1;i<a.length;i++)if(time<=a[i].time){const p=a[i-1],q=a[i],t=(time-p.time)/(q.time-p.time);return {x:p.x+(q.x-p.x)*t,y:p.y+(q.y-p.y)*t,z:p.z+(q.z-p.z)*t};}return a.at(-1);}
}
export function compassPosition(yaw){const bearing=((-yaw*180/Math.PI)%360+360)%360;return {bearing,offset:140-24-(8+bearing/45)*48};}

export function advanceRemoteClock(current,observed,dt){if(!Number.isFinite(current)||observed-current>1)return observed;const next=current+dt+Math.max(-dt*.15,Math.min(dt*.15,observed-current-dt));return Math.max(current,Math.min(observed+.1,next));}
