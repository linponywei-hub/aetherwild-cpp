// Event-driven canvas: viewing the atlas never starts a game render loop.
export class MapViewport {
 constructor(){this.x=0;this.z=0;this.scale=1;this.base=1;}
 fit(regions,w,h){const xs=regions.flatMap(r=>[r.center.x-r.radius,r.center.x+r.radius]),zs=regions.flatMap(r=>[r.center.z-r.radius,r.center.z+r.radius]);const loX=Math.min(...xs),hiX=Math.max(...xs),loZ=Math.min(...zs),hiZ=Math.max(...zs);this.x=(loX+hiX)/2;this.z=(loZ+hiZ)/2;this.base=this.scale=Math.min(w/Math.max(1,hiX-loX),h/Math.max(1,hiZ-loZ))*.82;}
 world(px,py,w,h){return {x:this.x+(px-w/2)/this.scale,z:this.z+(py-h/2)/this.scale};}
 screen(p,w,h){return {x:w/2+(p.x-this.x)*this.scale,y:h/2+(p.z-this.z)*this.scale};}
 zoom(factor,px,py,w,h){const anchor=this.world(px,py,w,h);this.scale=Math.max(this.base*.5,Math.min(this.base*64,this.scale*factor));this.x=anchor.x-(px-w/2)/this.scale;this.z=anchor.z-(py-h/2)/this.scale;}
 pan(dx,dy){this.x-=dx/this.scale;this.z-=dy/this.scale;}
}
export class AdminMap {
 constructor(){this.isOpen=false;this.viewport=new MapViewport();}
 open(data){
  if(!this.root)this.mount();this.data=data;this.root.hidden=false;this.isOpen=true;this.select.value='planned';this.image ||= new Image();this.image.onload=()=>this.draw();this.image.src=data.atlas.image;this.setLayer();this.root.querySelector('[data-close]').focus();
 }
 close(){this.isOpen=false;if(this.root)this.root.hidden=true;document.querySelector('#admin-map-open')?.focus();}
 mount(){
  const style=document.createElement('style');style.textContent=`.admin-atlas[hidden]{display:none}.admin-atlas{position:fixed;inset:3vh 3vw;z-index:90;background:#101c28;color:#edf4fa;border:1px solid #829aae;border-radius:12px;padding:16px;display:flex;flex-direction:column;box-shadow:0 0 0 4vw #000a;font:15px sans-serif}.admin-atlas header{display:flex;flex-wrap:wrap;gap:8px;align-items:center}.admin-atlas h2{margin:0 auto 0 0;font-size:20px}.admin-atlas button,.admin-atlas select{padding:8px;background:#253a4d;color:white;border:1px solid #7a96ae;border-radius:5px}.admin-atlas canvas{width:100%;flex:1;min-height:80px;touch-action:none;cursor:grab;margin:12px 0;background:#09131f}.admin-atlas p{margin:3px 0;font-size:13px}.admin-atlas [data-info]{min-height:32px}`;document.head.append(style);
  this.root=document.createElement('section');this.root.className='admin-atlas';this.root.setAttribute('role','dialog');this.root.setAttribute('aria-modal','true');this.root.setAttribute('aria-label','管理員完整世界地圖');this.root.innerHTML=`<header><h2>管理員完整世界地圖</h2><button data-minus aria-label="縮小">−</button><button data-plus aria-label="放大">＋</button><button data-fit>全圖</button><button data-close>關閉</button></header><p data-status></p><canvas aria-label="可縮放拖曳的世界地圖"></canvas><p data-info>點選區域查看資訊</p><p>滾輪放大縮小／拖曳平移／雙擊放大。底圖依參考原圖定位；標記顯示區域中心。</p>`;document.body.append(this.root);
  this.canvas=this.root.querySelector('canvas');this.select={value:'planned'};this.root.querySelector('[data-close]').onclick=()=>this.close();this.root.querySelector('[data-fit]').onclick=()=>this.fit();
  for(const [key,f] of [['plus',1.5],['minus',1/1.5]])this.root.querySelector('[data-'+key+']').onclick=()=>{this.viewport.zoom(f,this.w/2,this.h/2,this.w,this.h);this.draw();};
  const point=e=>{const b=this.canvas.getBoundingClientRect();return {x:e.clientX-b.left,y:e.clientY-b.top};};
  this.canvas.onwheel=e=>{e.preventDefault();const p=point(e);this.viewport.zoom(Math.exp(-Math.max(-120,Math.min(120,e.deltaY))*.006),p.x,p.y,this.w,this.h);this.draw();};
  this.canvas.onpointerdown=e=>{if(e.button!==0)return;this.drag={...point(e),start:point(e),moved:false};this.canvas.setPointerCapture(e.pointerId);};
  this.canvas.onpointermove=e=>{if(!this.drag)return;const p=point(e);this.drag.moved ||= Math.hypot(p.x-this.drag.start.x,p.y-this.drag.start.y)>4;this.viewport.pan(p.x-this.drag.x,p.y-this.drag.y);Object.assign(this.drag,p);this.draw();};
  this.canvas.onpointerup=e=>{if(!this.drag)return;if(!this.drag.moved)this.pick(point(e));this.drag=null;};this.canvas.onpointercancel=()=>{this.drag=null;};
  this.canvas.ondblclick=e=>{const p=point(e);this.viewport.zoom(2,p.x,p.y,this.w,this.h);this.draw();};
  this.root.addEventListener('keydown',e=>{if(e.key==='Tab'){const list=[...this.root.querySelectorAll('button,select')];if(e.shiftKey&&document.activeElement===list[0]){e.preventDefault();list.at(-1).focus();}else if(!e.shiftKey&&document.activeElement===list.at(-1)){e.preventDefault();list[0].focus();}}});
  new ResizeObserver(()=>{if(this.isOpen){this.resize();this.draw();}}).observe(this.canvas);
 }
 resize(){const b=this.canvas.getBoundingClientRect();this.w=Math.max(1,b.width);this.h=Math.max(1,b.height);const d=Math.min(devicePixelRatio||1,2);this.canvas.width=Math.round(this.w*d);this.canvas.height=Math.round(this.h*d);this.canvas.getContext('2d').setTransform(d,0,0,d,0,0);}
 setLayer(){this.regions=this.data[this.select.value];this.selected=null;this.root.querySelector('[data-info]').textContent='點選區域查看資訊';this.root.querySelector('[data-status]').textContent=`${this.regions.length} 個區域｜完整世界｜原圖 64 m/px｜世界範圍 96 × 96 km`;this.fit();}
 fit(){this.resize();const b=this.data.atlas.bounds;this.viewport.fit([{center:{x:(b.minX+b.maxX)/2,z:(b.minZ+b.maxZ)/2},radius:Math.max(b.maxX-b.minX,b.maxZ-b.minZ)/2}],this.w,this.h);this.draw();}
 pick(p){const hit=this.regions.map(r=>({r,p:this.viewport.screen(r.center,this.w,this.h)})).filter(v=>Math.hypot(p.x-v.p.x,p.y-v.p.y)<=Math.max(12,v.r.radius*this.viewport.scale)).sort((a,b)=>Math.hypot(p.x-a.p.x,p.y-a.p.y)-Math.hypot(p.x-b.p.x,p.y-b.p.y))[0];this.selected=hit?.r.id;this.root.querySelector('[data-info]').textContent=hit?`${hit.r.name}（${hit.r.id}）｜完整世界｜${hit.r.layer||'surface'}｜城域半徑 ${hit.r.radius}m／建成區 ${hit.r.urbanRadius||0}m｜X ${hit.r.center.x} / Z ${hit.r.center.z}`:'點選區域查看資訊';this.draw();}
 draw(){if(!this.isOpen)return;const c=this.canvas.getContext('2d'),v=this.viewport,w=this.w,h=this.h;c.clearRect(0,0,w,h);const project=p=>v.screen(p,w,h);c.strokeStyle='#203648';c.lineWidth=1;const step=10**Math.ceil(Math.log10(75/v.scale));const top=v.world(0,0,w,h),bottom=v.world(w,h,w,h);for(let x=Math.ceil(top.x/step)*step;x<bottom.x;x+=step){const p=project({x,z:0});c.beginPath();c.moveTo(p.x,0);c.lineTo(p.x,h);c.stroke();}for(let z=Math.ceil(top.z/step)*step;z<bottom.z;z+=step){const p=project({x:0,z});c.beginPath();c.moveTo(0,p.y);c.lineTo(w,p.y);c.stroke();}
  const bounds=this.data.atlas.bounds,a=project({x:bounds.minX,z:bounds.minZ}),b=project({x:bounds.maxX,z:bounds.maxZ});if(this.image?.complete&&this.image.naturalWidth)c.drawImage(this.image,a.x,a.y,b.x-a.x,b.y-a.y);
  for(const r of this.regions){const p=project(r.center),radius=r.city||r.id===this.selected?Math.max(6,r.radius*v.scale):5;if(p.x+radius<0||p.y+radius<0||p.x-radius>w||p.y-radius>h)continue;c.setLineDash(r.open?[]:[5,4]);c.strokeStyle=r.id===this.selected?'white':r.open?'#67dca4':'#efb86c';c.fillStyle=r.open?'#245c4855':'#78542b44';c.beginPath();c.arc(p.x,p.y,radius,0,Math.PI*2);c.fill();c.stroke();if(r.city&&r.urbanRadius){c.setLineDash([]);c.strokeStyle='#ffe6a1';c.lineWidth=2;c.beginPath();c.arc(p.x,p.y,Math.max(3,r.urbanRadius*v.scale),0,Math.PI*2);c.stroke();}c.setLineDash([]);c.lineWidth=1;c.font='12px sans-serif';c.fillStyle='#f2f6ff';c.textAlign='center';c.lineWidth=3;c.strokeStyle='#102030';c.strokeText(r.name,p.x,p.y-radius-5);c.fillText(r.name,p.x,p.y-radius-5);}
  c.textAlign='left';c.fillStyle='#dce8f2';c.fillText(`N ↑  ｜ ${(v.scale/v.base).toFixed(1)}× ｜格線 ${step.toLocaleString()} m｜外圈：城域；金圈：建成區`,12,h-12);
 }
}
