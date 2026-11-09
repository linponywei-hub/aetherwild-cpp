import * as T from '../../vendor/three.module.js';

// Quaternius CC0 nature meshes hosted by a public GitHub project.
// Source: https://quaternius.com/packs/stylizednaturemegakit
// Do not fall back to procedural geometric buildings or bridges.
const TREE_URLS = [
  'https://raw.githubusercontent.com/anshaneja5/skyline-run/main/public/assets/models/tree1.glb',
  'https://raw.githubusercontent.com/anshaneja5/skyline-run/main/public/assets/models/tree2.glb',
  'https://raw.githubusercontent.com/anshaneja5/skyline-run/main/public/assets/models/tree3.glb'
];
let loaderPromise;
function loader() {
  return loaderPromise ??= import('../../vendor/GLTFLoader.js').then(({GLTFLoader})=>new GLTFLoader());
}
async function fetchModel(url) {
  const response = await fetch(url, {mode:'cors'});
  if (!response.ok) throw new Error('HTTP '+response.status+' '+url);
  const data = await response.arrayBuffer();
  const gltfLoader = await loader();
  return new Promise((resolve,reject)=>gltfLoader.parse(data,url.slice(0,url.lastIndexOf('/')+1),resolve,reject));
}
function placeModel(source, root, x, y, z, height) {
  const model = source.scene.clone(true);
  model.updateMatrixWorld(true);
  const bounds = new T.Box3().setFromObject(model);
  const size = bounds.getSize(new T.Vector3());
  if (!(size.y>0)) throw new Error('Tree model has invalid bounds');
  model.scale.multiplyScalar(height/size.y);
  model.updateMatrixWorld(true);
  const resized = new T.Box3().setFromObject(model);
  const bottom = resized.min.y;
  const center = resized.getCenter(new T.Vector3());
  model.position.set(x-center.x,y-bottom,z-center.z);
  model.traverse(o=>{if(o.isMesh){o.castShadow=true;o.receiveShadow=true;}});
  root.add(model);
}
export async function attachVerdantCC0Models(root) {
  if (!root) return {loaded:0};
  const results = await Promise.allSettled(TREE_URLS.map(fetchModel));
  const trees = results.filter(r=>r.status==='fulfilled').map(r=>r.value);
  if (!trees.length) {
    console.error('[Verdant] All external GLB requests failed',results.map(r=>String(r.reason)));
    root.userData.assetError='External GLB trees could not be loaded';
    return {loaded:0};
  }
  let loaded=0;
  for(let i=0;i<4;i++){
    const angle=i*Math.PI/2;
    placeModel(trees[i%trees.length],root,Math.cos(angle)*49,-32,Math.sin(angle)*49,68+(i%3)*7);
    loaded++;
  }
  // One dominant imported tree, with the base lifted to preserve the central
  // entrance. Satellite crowns stay separated so the skyline is not one slab.
  placeModel(trees[0],root,0,-19,0,82);
  loaded++;
  // Imported bridge GLB spans between the individual tree platforms.
  try {
    const bridgeSource=await fetchModel('/assets/models/verdant/aetherwild-rope-bridge.glb');
    const bridgeBase=bridgeSource.scene;
    for(let i=0;i<4;i++){
      const a=i*Math.PI/2,b=(i+1)*Math.PI/2;
      const x=Math.cos(a)*49,z=Math.sin(a)*49;
      const nx=Math.cos(b)*49,nz=Math.sin(b)*49;
      const dx=nx-x,dz=nz-z,length=Math.hypot(dx,dz);
      const bridge=bridgeBase.clone(true);
      bridge.updateMatrixWorld(true);
      const box=new T.Box3().setFromObject(bridge);
      const size=box.getSize(new T.Vector3());
      const longitudinalX=size.x>=size.z;
      const factor=length/Math.max(size.x,size.z);
      bridge.scale.multiplyScalar(factor);
      bridge.updateMatrixWorld(true);
      const centered=new T.Box3().setFromObject(bridge);
      const center=centered.getCenter(new T.Vector3());
      bridge.position.set(-center.x,-center.y,-center.z);
      bridge.rotation.y=(longitudinalX?-Math.atan2(dz,dx):Math.atan2(dx,dz));
      const anchor=new T.Group();
      anchor.position.set((x+nx)/2,1.2,(z+nz)/2);
      anchor.add(bridge);root.add(anchor);loaded++;
    }
  } catch(error) {console.warn('[Verdant] bridge GLB unavailable',error);}
  // Performance: do not clone the bridge GLB dozens of times for every stair.
  // A dedicated low-draw-call stair asset will replace this in a later pass.
  // Reuse the game's already bundled authored GLB buildings, not primitive huts.
  // These are temporary architecture assets until licensed canopy-house GLBs are sourced.
  const houseURLs=Array.from({length:5},(_,i)=>'/assets/models/buildings/village/village_building_'+String(i+1).padStart(2,'0')+'_lod0.glb');
  const houses=await Promise.allSettled(houseURLs.map(fetchModel));
  const available=houses.filter(r=>r.status==='fulfilled').map(r=>r.value);
  for(let i=0;i<4&&available.length;i++){
    const a=i*Math.PI/2;
    try {
      const source=available[i%available.length], house=source.scene.clone(true);
      house.updateMatrixWorld(true);
      const bounds=new T.Box3().setFromObject(house);
      const size=bounds.getSize(new T.Vector3());
      const scale=12/Math.max(size.x,size.y,size.z);
      if(!Number.isFinite(scale)||scale<=0)continue;
      house.scale.multiplyScalar(scale);
      house.updateMatrixWorld(true);
      const placed=new T.Box3().setFromObject(house);
      const center=placed.getCenter(new T.Vector3());
      house.position.set(Math.cos(a)*49-center.x,-placed.min.y,Math.sin(a)*49-center.z);
      house.traverse(o=>{if(o.isMesh){o.castShadow=true;o.receiveShadow=true;}});
      root.add(house);
      loaded++;
    } catch(error) { console.warn('[Verdant] External house placement failed:',error); }
  }
  root.userData.externalCC0Models=loaded;
  root.userData.assetError=results.some(r=>r.status==='rejected')?'Some GLB variants failed to load':null;
  console.info('[Verdant] External-only 3D tree models loaded:',loaded);
  return {loaded};
}
