// Visual lighting only. Epoch/cycle are supplied by C++; no gameplay rules here.
export const CYCLE_SECONDS=1800;
const clamp=x=>Math.max(0,Math.min(1,x));
const smooth=(a,b,x)=>{const t=clamp((x-a)/(b-a));return t*t*(3-2*t);};
const blend=(a,b,t)=>a.map((v,i)=>Math.round(v+(b[i]-v)*t));
export function lightingAt(seconds,cycle=CYCLE_SECONDS){
 const phase=((seconds%cycle)+cycle)%cycle/cycle,angle=phase*Math.PI*2-Math.PI/2,elevation=Math.sin(angle),daylight=smooth(-.12,.23,elevation),twilight=(1-smooth(0,.35,Math.abs(elevation)))*(1-Math.abs(daylight-.5));
 const rgb=(night,day)=>blend(night,day,daylight);
 const top=rgb([9,17,37],[112,173,209]),horizon=blend(rgb([24,34,57],[224,222,190]),[224,139,94],twilight*.55);
 const hour=phase*24,minutes=Math.floor(hour*60);
 return {phase,hour,daylight,elevation,angle,top,horizon,ground:rgb([27,39,44],[110,137,90]),sunIntensity:3.2*daylight,moonIntensity:.46*(1-daylight),ambient:.5+1.3*daylight,
  label:elevation<-.12?'夜晚':elevation<.23?'晨昏':'白晝',clock:String(Math.floor(minutes/60)).padStart(2,'0')+':'+String(minutes%60).padStart(2,'0')};
}
export class EnvironmentClock {
 constructor(){this.anchor=Date.now()/1000;this.at=performance.now();this.cycle=CYCLE_SECONDS;}
 sync(environment,at=performance.now()){if(!environment||!Number.isFinite(environment.unixSeconds))return;this.anchor=environment.unixSeconds;this.at=at;this.cycle=environment.cycleSeconds||CYCLE_SECONDS;}
 now(at=performance.now()){return this.anchor+Math.max(0,(at-this.at)/1000);}
 light(at=performance.now()){return lightingAt(this.now(at),this.cycle);}
}
