import * as T from '/vendor/three.module.js';
export function makeLandscape(scene){const model=scene.catalog.landscape;if(!model)return;const point=([x,z])=>new T.Vector2((x-633)*64,-(z-578)*64);
 // Water is geometry along authored coastlines and rivers, never a city-sized billboard.
 for(const polygon of model.water){const shape=new T.Shape(polygon.map(point)),g=new T.ShapeGeometry(shape);g.rotateX(-Math.PI/2);const mesh=scene.mesh(g,scene.mat('#3c889c',{roughness:.35,side:T.DoubleSide}),[0,.08,0]);mesh.castShadow=mesh.receiveShadow=false;mesh.userData.cullRadius=100000;}
 for(const river of model.rivers){const v=[];for(let i=1;i<river.points.length;i++){const a=point(river.points[i-1]),b=point(river.points[i]),dx=b.x-a.x,dz=b.y-a.y,length=Math.hypot(dx,dz),nx=-dz/length*river.width/2,nz=dx/length*river.width/2;v.push(a.x+nx,.1,-a.y-nz,a.x-nx,.1,-a.y+nz,b.x+nx,.1,-b.y-nz,b.x+nx,.1,-b.y-nz,a.x-nx,.1,-a.y+nz,b.x-nx,.1,-b.y+nz);}const geo=new T.BufferGeometry();geo.setAttribute('position',new T.Float32BufferAttribute(v,3));geo.computeVertexNormals();const m=scene.mesh(geo,scene.mat('#4f9aa5',{side:T.DoubleSide}),[0,0,0]);m.castShadow=m.receiveShadow=false;m.userData.cullRadius=100000;}
 for(const port of model.harbors){const region=scene.catalog.regions.find(r=>r.id===port.region);if(!region)continue;const root=new T.Group();root.position.set(region.position.x,0,region.position.z);root.userData.cullRadius=port.extent;scene.root.add(root);
  for(const b of port.platforms){const deck=scene.box(root,b.width,.5,b.depth,b.kind==='quay'?'#b0aa8c':'#a18c68',b.x,b.height-.25,b.z);deck.castShadow=false;
   if(b.kind==='ship'){const hull=scene.box(root,b.width*.93,9,b.depth*.92,'#554b3b',b.x,b.height-5,b.z);hull.castShadow=false;const mast=scene.cylinder(root,.6,1.2,b.depth*.35,'#756342',b.x,b.height+b.depth*.175,b.z,8);mast.castShadow=false;const sail=scene.mesh(new T.PlaneGeometry(b.width*.68,b.depth*.22),scene.mat('#dfd9bf',{side:T.DoubleSide}),[b.x,b.height+b.depth*.23,b.z],root);sail.castShadow=false;}
   else if(b.kind==='quay'){scene.box(root,b.width,4,b.depth,'#98917b',b.x,1.75,b.z);}else{for(let x=-b.width/2+10;x<b.width/2;x+=Math.max(60,b.width/12))scene.cylinder(root,.7,1,6,'#61563f',b.x+x,1,b.z,6);}
  }
  const r=port.ramp,ramp=scene.box(root,r.width,.5,Math.hypot(r.depth,r.height),'#a28f6e',r.x,r.height/2-.25,r.z);ramp.rotation.x=Math.atan2(r.height,r.depth);ramp.castShadow=false;
 }
}
