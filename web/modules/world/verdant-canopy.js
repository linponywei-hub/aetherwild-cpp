import * as T from '../../vendor/three.module.js';

// Visual-only enhancements for Verdant City. Collision remains authoritative in C++.
// Keep walkable structures inside the existing elevated city footprint.
export function addVerdantCanopy(scene) {
  const region = scene.catalog.regions.find(r => r.id === 'verdant');
  if (!region) return null;
  const root = new T.Group();
  root.name = 'verdant-canopy-visuals';
  root.position.set(region.position.x, region.elevation, region.position.z);
  scene.root.add(root);
  const wood = new T.MeshStandardMaterial({color:'#67513a',roughness:1});
  const rail = new T.MeshStandardMaterial({color:'#9b8054',roughness:1});
  const leaves = new T.MeshStandardMaterial({color:'#54764b',roughness:1,side:T.DoubleSide});
  const plaster = new T.MeshStandardMaterial({color:'#b6aa7f',roughness:1});
  const roofMat = new T.MeshStandardMaterial({color:'#435e3e',roughness:1});
  const windowMat = new T.MeshStandardMaterial({color:'#d9bd75',emissive:'#7c5c29',emissiveIntensity:.2});
  const add = (geometry, material, x,y,z) => {
    const mesh = new T.Mesh(geometry,material);
    mesh.position.set(x,y,z); mesh.castShadow=true;mesh.receiveShadow=true;
    root.add(mesh); return mesh;
  };
  // Hollow trunk concept: visible entrance, interior stair flights and lanterns.
  // Stair treads match the C++ groundBelow spiral; trunk walls remain visual-only.
  const barkCanvas=document.createElement('canvas');
  barkCanvas.width=256;barkCanvas.height=512;
  const ctx=barkCanvas.getContext('2d');
  ctx.fillStyle='#493322';ctx.fillRect(0,0,256,512);
  let seed=71591;const random=()=>{seed=(seed*1664525+1013904223)>>>0;return seed/4294967296;};
  for(let i=0;i<950;i++){
    const x=random()*256,y=random()*512,w=1+random()*9,h=12+random()*110;
    ctx.fillStyle=['#291e16','#73513a','#96704b','#38291e','#5c422d'][Math.floor(random()*5)];
    ctx.fillRect(x,y,w,h);
  }
  const barkTexture=new T.CanvasTexture(barkCanvas);
  barkTexture.wrapS=T.RepeatWrapping;barkTexture.wrapT=T.RepeatWrapping;
  barkTexture.repeat.set(3,2);
  const bark = new T.MeshStandardMaterial({map:barkTexture,color:'#d8c0a1',roughness:1,side:T.DoubleSide});
  const innerRadius=8.5;
  // Entry faces +Z; the first tread must be reachable from the doorway.
  for(let j=0;j<20;j++) {
    const theta=j*Math.PI/10;
    // Leave a wide entrance opening on the south side of the trunk.
    if(j>=4&&j<=6)continue;
    const segment=add(new T.CylinderGeometry(11,12,35,32,1,true,theta,Math.PI/10),bark,0,-17,0);
    segment.userData.decorativeTrunk=true;
  }
  // Massive asymmetrical roots give the central trunk a natural silhouette.
  for(let k=0;k<13;k++){
    const a=k*Math.PI*2/13, x=Math.cos(a),z=Math.sin(a);
    const rootMesh=add(new T.CylinderGeometry(1.3,4.5,21,7),bark,x*12,-25,z*12);
    rootMesh.rotation.z=z*.46;rootMesh.rotation.x=-x*.46;
  }
  // Branching crown above the walkable hollow trunk.
  for(let k=0;k<17;k++){
    const a=k*2.399963229728653, height=14+(k%5)*4;
    const x=Math.cos(a),z=Math.sin(a);
    const branch=add(new T.CylinderGeometry(.65,2.5,26,7),bark,x*12,height,z*12);
    branch.rotation.z=z*.8;branch.rotation.x=-x*.8;
    const crown=add(new T.IcosahedronGeometry(11+(k%3)*2,2),leaves,x*23,height+12,z*23);
    crown.scale.set(1.3,.75,1.15);
  }
  for(let level=0;level<100;level++) {
    const a=Math.PI/2+level*8*Math.PI/100, y=-32+level*.32;
    const step=add(new T.BoxGeometry(4,.24,2.7),wood,Math.cos(a)*innerRadius,y,Math.sin(a)*innerRadius);
    step.rotation.y=-a;
    if(level%5===0)add(new T.BoxGeometry(.5,.65,.5),windowMat,Math.cos(a)*6.8,y+.55,Math.sin(a)*6.8);
  }
  // Raised decks and walkways are decorative only: never advertise them as
  // navigable until their matching server collision geometry is implemented.
  for (let i=0;i<6;i++) {
    const a=i*Math.PI/3, radius=49;
    const x=Math.cos(a)*radius,z=Math.sin(a)*radius;
    add(new T.CylinderGeometry(3.3,5.5,33,9),wood,x,-16,z);
    add(new T.CylinderGeometry(13,13,0.65,12),wood,x,0.3,z);
    for (let k=0;k<10;k++) {
      const t=k*Math.PI/5;
      add(new T.CylinderGeometry(.16,.16,2.4,5),rail,x+Math.cos(t)*11,1.65,z+Math.sin(t)*11);
    }
    const crown=add(new T.IcosahedronGeometry(16,2),leaves,x,23,z);
    crown.scale.set(1.2,.8,1.2);
    crown.userData.proceduralFoliage=true;
    // Compact tree dwelling: timber frame, pitched roof, warm window and porch.
    // Offset from the trunk; all geometry is visual-only until C++ paths are defined.
    const hx=x+Math.cos(a)*6,hz=z+Math.sin(a)*6;
    add(new T.BoxGeometry(7,5,6),plaster,hx,3.15,hz);
    const roof=add(new T.ConeGeometry(5.4,3.4,4),roofMat,hx,7.2,hz);
    roof.rotation.y=Math.PI/4;
    add(new T.BoxGeometry(1.5,2.8,.12),wood,hx,1.9,hz+3.07);
    add(new T.BoxGeometry(1.15,1.15,.15),windowMat,hx-2,4.1,hz+3.09);
    add(new T.BoxGeometry(1.15,1.15,.15),windowMat,hx+2,4.1,hz+3.09);
    for (const side of [-1,1]) {
      add(new T.CylinderGeometry(.17,.17,3.5,6),wood,hx+side*3.3,3.1,hz+2.8);
    }
    // Narrow decorative connecting span, kept above the existing city surface.
    const next=(i+1)*Math.PI/3;
    const nx=Math.cos(next)*radius,nz=Math.sin(next)*radius;
    const dx=nx-x,dz=nz-z,dist=Math.hypot(dx,dz);
    const bridge=add(new T.BoxGeometry(4,.55,dist),wood,(x+nx)/2,.4,(z+nz)/2);
    bridge.rotation.y=Math.atan2(dx,dz);
  }
  root.userData.visualOnly = true;
  root.userData.regionId = 'verdant';
  root.userData.assetPhase = 'procedural-phase1';
  return root;
}
