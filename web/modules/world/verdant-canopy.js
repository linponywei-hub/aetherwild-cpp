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
  const add = (geometry, material, x,y,z) => {
    const mesh = new T.Mesh(geometry,material);
    mesh.position.set(x,y,z); mesh.castShadow=true;mesh.receiveShadow=true;
    root.add(mesh); return mesh;
  };
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
    add(new T.ConeGeometry(16,18,9),leaves,x,16,z);
    add(new T.ConeGeometry(12,14,9),leaves,x,26,z);
    // Narrow decorative connecting span, kept above the existing city surface.
    const next=(i+1)*Math.PI/3;
    const nx=Math.cos(next)*radius,nz=Math.sin(next)*radius;
    const dx=nx-x,dz=nz-z,dist=Math.hypot(dx,dz);
    const bridge=add(new T.BoxGeometry(4,.55,dist),wood,(x+nx)/2,.4,(z+nz)/2);
    bridge.rotation.y=Math.atan2(dx,dz);
  }
  root.userData.visualOnly = true;
  return root;
}
