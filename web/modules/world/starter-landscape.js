import {ScannedVegetation} from './scanned-vegetation.js';
import * as T from '../../vendor/three.module.js';

export function starterConfig(catalog){return catalog.landscape?.starterVillage;}
export function starterCenter(config){return {x:(config.pixel[0]-633)*64,z:(config.pixel[1]-578)*64};}
export function villageRoadDistance(x,z){return Math.min(Math.abs(x+.3*Math.sin(z*.075)),Math.abs(z+.3*Math.sin(x*.075)));}
export function starterHouses(catalog){return (catalog.buildings||[]).filter(b=>b.region==='windchime'&&b.style?.split('|')[1]==='house').slice(0,5);}
export function decorativePointAllowed(x,z,config,houses,center){
 const dx=x-center.x,dz=z-center.z;
 if(Math.hypot(dx,dz)<config.plazaRadius+2||villageRoadDistance(dx,dz)<config.roadHalfWidth+1.7)return false;
 return !houses.some(b=>Math.abs(x-b.position.x)<b.width/2+3&&Math.abs(z-b.position.z)<b.depth/2+3);
}
export function starterScatter(catalog,count=3200){
 const config=starterConfig(catalog);if(!config)return [];
 const center=starterCenter(config),houses=starterHouses(catalog),points=[];let seed=0x51a7;
 const rand=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
 for(let i=0;i<count;i++){
  // Concentrate detail within walking distance; only sparse edge tufts farther away.
  const radius=12+Math.pow(rand(),1.7)*180,angle=rand()*Math.PI*2;
  const x=center.x+Math.cos(angle)*radius,z=center.z+Math.sin(angle)*radius;
  const variation=rand(),rotation=rand()*Math.PI*2;
  if(decorativePointAllowed(x,z,config,houses,center))points.push({x,z,variation,rotation});
 }
 return points;
}
export class StarterLandscape {
 constructor(parent,catalog,height,{software=false,count=3200}={}){
  this.catalog=catalog;this.height=height;this.root=new T.Group();this.root.name='starter-village-groundcover';this.config=starterConfig(catalog);
  if(!this.config)return;
  this.center=starterCenter(this.config);this.root.position.set(this.center.x,0,this.center.z);this.root.userData.cullRadius=220;parent.add(this.root);
  this.points=software?[]:starterScatter(catalog,count);this.rebuild();if(!software&&typeof document!=='undefined')this.scanned=new ScannedVegetation(this.root,this.points,this.center,height,{onReady:()=>{const grass=this.root.children.find(m=>m.name==='starter-grass');if(grass)grass.visible=false;}});
 }
 rebuild(){
  if(!this.config)return;
  for(const child of [...this.root.children]){if(child.isInstancedMesh)continue;this.root.remove(child);child.geometry.dispose();child.material.dispose();}
  const positions=[],colors=[],rockPositions=[],rockColors=[];const leaf=new T.Color(),stone=new T.Color();
  const triangle=(a,b,c,color,target=positions,tints=colors)=>{for(const p of [a,b,c]){target.push(...p);tints.push(color.r,color.g,color.b);}};
  for(const p of this.points){
   const y=this.height(p.x,p.z);if(!Number.isFinite(y))continue;const x=p.x-this.center.x,z=p.z-this.center.z;
   leaf.setHSL(.21+p.variation*.06,.25+p.variation*.16,.20+p.variation*.12);
   for(let blade=0;blade<5;blade++){
    const a=p.rotation+blade*2.39996,w=.028+p.variation*.035,h=.18+((p.variation+blade*.173)%1)*.42;
    const dx=Math.cos(a),dz=Math.sin(a),ox=x+Math.sin(a*3)*.16,oz=z+Math.cos(a*2)*.16;
    const l=[ox-dx*w,y,oz-dz*w],r=[ox+dx*w,y,oz+dz*w],m=[ox+dz*h*.18,y+h*.6,oz-dx*h*.18],tip=[ox+dz*h*.43,y+h,oz-dx*h*.43];
    triangle(l,r,m,leaf);triangle(l,m,tip,leaf);
   }
   if(p.variation>.91){
    const r=.10+p.variation*.09,h=.06+p.variation*.07;stone.setHSL(.1,.05,.28+p.variation*.1);
    const a=[x-r,y,z-r*.6],b=[x+r,y,z-r*.5],c=[x+r*.8,y,z+r*.8],d=[x-r*.7,y,z+r],top=[x+r*.2,y+h,z];
    triangle(a,top,b,stone,rockPositions,rockColors);triangle(b,top,c,stone,rockPositions,rockColors);triangle(c,top,d,stone,rockPositions,rockColors);triangle(d,top,a,stone,rockPositions,rockColors);
   }
  }
  for(const [vertices,tints,name] of [[positions,colors,'grass'],[rockPositions,rockColors,'pebbles']]){
   if(!vertices.length)continue;const geometry=new T.BufferGeometry();geometry.setAttribute('position',new T.Float32BufferAttribute(vertices,3));geometry.setAttribute('color',new T.Float32BufferAttribute(tints,3));geometry.computeVertexNormals();geometry.computeBoundingSphere();
   const material=new T.MeshStandardMaterial({vertexColors:true,roughness:.95,side:name==='grass'?T.DoubleSide:T.FrontSide});
   const mesh=new T.Mesh(geometry,material);mesh.name='starter-'+name;if(name==='grass'&&this.scanned?.ready)mesh.visible=false;mesh.receiveShadow=true;mesh.castShadow=name==='pebbles';this.root.add(mesh);
  }
  this.scanned?.update();
 }
}
