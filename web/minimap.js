import {gridHeight} from './landscape-model.js';
export const MAP_SCALE=.55;
export function mapPoint(position,center,width,height,scale=MAP_SCALE){return {x:width/2+(position.x-center.x)*scale,y:height/2+(position.z-center.z)*scale};}
export function headingTriangle(yaw,size=6){const c=Math.cos(yaw),s=Math.sin(yaw);return [[0,-size],[-size*.65,size*.7],[size*.65,size*.7]].map(([x,y])=>[x*c+y*s,-x*s+y*c]);}
const biomeColor=id=>/snow|frost|ice/.test(id)?'#c5d6d4':/sand|desert|dune/.test(id)?'#cbb17b':/ash|cinder/.test(id)?'#877d6a':/dark|swamp/.test(id)?'#435e53':/forest|verdant/.test(id)?'#64835d':'#879f70';
export class MiniMap {
 constructor(canvas,label){this.canvas=canvas;this.label=label;this.texture=new Image();this.texture.src='/art/minimap-ground-v2.webp';this.pattern=null;this.last='';}
 draw(state,catalog,yaw,unlocked){
  const c=this.canvas,w=c.clientWidth||216,h=c.clientHeight||180,dpr=Math.min(2,globalThis.devicePixelRatio||1),p=state.player,region=catalog.regions.reduce((best,r)=>!best||Math.hypot(r.position.x-p.position.x,r.position.z-p.position.z)<Math.hypot(best.position.x-p.position.x,best.position.z-p.position.z)?r:best,null);
  if(c.width!==Math.round(w*dpr)||c.height!==Math.round(h*dpr)){c.width=Math.round(w*dpr);c.height=Math.round(h*dpr);}
  const ctx=c.getContext('2d');ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,w,h);ctx.fillStyle='#879b70';ctx.fillRect(0,0,w,h);
  const scale=p.scene==='world'?MAP_SCALE:2.2,point=v=>mapPoint(v,p.position,w,h,scale);
  ctx.save();ctx.beginPath();ctx.rect(0,0,w,h);ctx.clip();ctx.translate(w/2-p.position.x*scale,h/2-p.position.z*scale);ctx.scale(scale,scale);
  if(unlocked&&p.scene==='world'){
   const model=catalog.landscape;
   const polygon=points=>{ctx.beginPath();points.forEach(([x,z],i)=>{x=(x-633)*64;z=(z-578)*64;i?ctx.lineTo(x,z):ctx.moveTo(x,z);});ctx.closePath();};
   for(const b of model?.biomes||[]){polygon(b.polygon);ctx.fillStyle=biomeColor(b.id);ctx.fill();}
   // A new repeating material stays at a fixed world scale. Never magnify the full-world atlas.
   if(this.texture.complete&&this.texture.naturalWidth){this.pattern??=ctx.createPattern(this.texture,'repeat');ctx.save();ctx.globalAlpha=.2;ctx.scale(.22,.22);ctx.fillStyle=this.pattern;ctx.fillRect((p.position.x-w/scale)/.22,(p.position.z-h/scale)/.22,2*w/scale/.22,2*h/scale/.22);ctx.restore();}
   const grid=state.content?.terrainPatch||catalog.terrain;
   for(let z=Math.floor((p.position.z-h/scale)/24)*24;z<p.position.z+h/scale;z+=24)for(let x=Math.floor((p.position.x-w/scale)/24)*24;x<p.position.x+w/scale;x+=24){const a=gridHeight(grid,x,z),b=gridHeight(grid,x+24,z+24);if(a===null||b===null)continue;const slope=Math.max(-.14,Math.min(.16,(a-b)/100));ctx.fillStyle=slope>0?'rgba(255,248,211,'+slope+')':'rgba(16,39,33,'+-slope+')';ctx.fillRect(x,z,24,24);}
   ctx.fillStyle='#4c879b';for(const water of model?.water||[]){polygon(water);ctx.fill();ctx.strokeStyle='#a3c4bc';ctx.lineWidth=2;ctx.stroke();}
   ctx.strokeStyle='#5895a6';ctx.lineCap='round';for(const river of model?.rivers||[]){ctx.beginPath();river.points.forEach(([x,z],i)=>{x=(x-633)*64;z=(z-578)*64;i?ctx.lineTo(x,z):ctx.moveTo(x,z);});ctx.lineWidth=river.width;ctx.stroke();}
   // Building footprints and existing walkways use actual game coordinates.
   for(const r of catalog.regions){if(r.wilderness)continue;ctx.strokeStyle='#d3c497';ctx.lineWidth=8;ctx.beginPath();ctx.moveTo(r.position.x-75,r.position.z);ctx.lineTo(r.position.x+75,r.position.z);ctx.moveTo(r.position.x,r.position.z-75);ctx.lineTo(r.position.x,r.position.z+75);ctx.stroke();}
   for(const harbor of model?.harbors||[]){const r=catalog.regions.find(r=>r.id===harbor.region);if(!r)continue;for(const b of harbor.platforms){ctx.fillStyle=b.kind==='ship'?'#8e7450':'#b9af8d';ctx.fillRect(r.position.x+b.x-b.width/2,r.position.z+b.z-b.depth/2,b.width,b.depth);}}
   for(const r of catalog.regions){if(r.wilderness)continue;ctx.setLineDash([5/scale,4/scale]);ctx.strokeStyle='#72b9a7';ctx.lineWidth=1/scale;ctx.beginPath();ctx.arc(r.position.x,r.position.z,r.radius,0,Math.PI*2);ctx.stroke();ctx.strokeStyle='#f6dca2';ctx.lineWidth=1.5/scale;ctx.beginPath();ctx.arc(r.position.x,r.position.z,r.urbanRadius||r.radius,0,Math.PI*2);ctx.stroke();ctx.setLineDash([]);}
   for(const b of catalog.buildings){ctx.fillStyle='#263d3655';ctx.fillRect(b.position.x-b.width/2+3,b.position.z-b.depth/2+4,b.width,b.depth);ctx.fillStyle='#dbceb0';ctx.strokeStyle='#4b5d4b';ctx.lineWidth=1.5;ctx.fillRect(b.position.x-b.width/2,b.position.z-b.depth/2,b.width,b.depth);ctx.strokeRect(b.position.x-b.width/2,b.position.z-b.depth/2,b.width,b.depth);}
  }else if(p.scene!=='world'){ctx.fillStyle='#4d635d';ctx.fillRect(-25,-30,50,60);ctx.strokeStyle='#c5cfb1';ctx.lineWidth=3;ctx.strokeRect(-25,-30,50,60);}
  ctx.restore();
  if(!unlocked&&p.scene==='world'){ctx.fillStyle='#152b24db';ctx.fillRect(0,0,w,h);this.label.textContent='向製圖師購買地圖';}
  else{this.label.textContent=p.scene==='world'?(region?.name||'無界原野'):'副本';
   for(const n of catalog.npcs||[]){const v=point(n.position);ctx.fillStyle='#ffdc81';ctx.strokeStyle='#263e34';ctx.lineWidth=1.5;ctx.beginPath();ctx.arc(v.x,v.y,3,0,Math.PI*2);ctx.fill();ctx.stroke();}
   for(const m of state.monsters||[]){if(m.hp<=0)continue;const v=point(m.position);ctx.fillStyle=m.boss?'#ec9679':'#a64639';ctx.beginPath();ctx.arc(v.x,v.y,m.boss?4:2,0,Math.PI*2);ctx.fill();}
  }
  // The player is always exactly centered, including while the map is locked.
  ctx.save();ctx.translate(w/2,h/2);ctx.shadowColor='#081c16';ctx.shadowBlur=6;ctx.fillStyle='#fff4c9';ctx.strokeStyle='#182b24';ctx.lineWidth=2;const triangle=headingTriangle(yaw,7);ctx.beginPath();triangle.forEach(([x,y],i)=>i?ctx.lineTo(x,y):ctx.moveTo(x,y));ctx.closePath();ctx.fill();ctx.stroke();ctx.restore();
  ctx.font='bold 11px system-ui';ctx.fillStyle='#fff2c8';ctx.fillText('N',w/2-4,15);ctx.strokeStyle='#f5e3b1';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(12,h-16);ctx.lineTo(12+50*scale,h-16);ctx.stroke();ctx.font='9px system-ui';ctx.fillText('50m',12,h-22);
 }
}
