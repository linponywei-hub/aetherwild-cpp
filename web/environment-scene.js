import * as T from '/vendor/three.module.js';
import {EnvironmentClock} from '/day-night.js';
const color=a=>new T.Color(`rgb(${a.join(',')})`);
export class EnvironmentVisual {
 constructor(scene){this.scene=scene;this.clock=new EnvironmentClock();this.moon=new T.DirectionalLight('#9fb9ed',.4);scene.world.add(this.moon,this.moon.target);
  const vertices=[];for(let i=0;i<110;i++){const y=.1+(i*37%101)/120,a=i*2.39996,r=700*Math.sqrt(1-y*y);vertices.push(Math.cos(a)*r,y*700,Math.sin(a)*r);}
  const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(vertices,3));this.stars=new T.Points(g,new T.PointsMaterial({color:'#dce9ff',size:1.6,transparent:true,depthWrite:false}));scene.world.add(this.stars);
  this.sunDisc=new T.Mesh(new T.SphereGeometry(7,12,8),new T.MeshBasicMaterial({color:'#ffe5a0'}));this.moonDisc=new T.Mesh(new T.SphereGeometry(5,12,8),new T.MeshBasicMaterial({color:'#ceddff'}));scene.world.add(this.sunDisc,this.moonDisc);
 }
 sync(state){this.clock.sync(state.environment);}
 update(position,world=true){const s=this.scene,l=this.clock.light();s.lighting=l;this.stars.visible=world;this.sunDisc.visible=world&&l.elevation>-.08;this.moonDisc.visible=world&&l.elevation<.08;
  const x=Math.cos(l.angle)*300,z=Math.cos(l.angle)*100,y=l.elevation*320;
  s.sun.position.set(position.x+x,position.y+y,position.z+z);s.sun.target.position.set(position.x,position.y,position.z);s.sun.intensity=world?l.sunIntensity:1.5;s.sun.color.set(l.elevation<.3?'#ffd29b':'#fff4d9');s.sun.castShadow=world&&l.elevation>.04;
  this.moon.position.set(position.x-x,position.y-y,position.z-z);this.moon.target.position.set(position.x,position.y,position.z);this.moon.intensity=world?l.moonIntensity:0;
  s.hemisphere.intensity=world?l.ambient:1.4;s.hemisphere.color.copy(color(l.horizon));s.hemisphere.groundColor.copy(color(l.ground));
  s.world.background.copy(color(world?l.horizon:[24,39,34]));if(s.world.fog)s.world.fog.color.copy(s.world.background);
  s.sky.material.uniforms.top.value.copy(color(l.top));s.sky.material.uniforms.bottom.value.copy(color(l.horizon));this.stars.position.copy(s.camera.position);this.stars.material.opacity=(1-l.daylight)*.85;
  this.sunDisc.position.set(s.camera.position.x+x*1.7,s.camera.position.y+y*1.7,s.camera.position.z+z*1.7);this.moonDisc.position.set(s.camera.position.x-x*1.7,s.camera.position.y-y*1.7,s.camera.position.z-z*1.7);
  s.renderer.environment=world?l:null;
  const clock=document.querySelector('#world-clock');if(clock)clock.textContent=l.label+' '+l.clock;
 }
}
