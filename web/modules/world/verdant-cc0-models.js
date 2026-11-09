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
  for(let i=0;i<6;i++){
    const angle=i*Math.PI/3;
    placeModel(trees[i%trees.length],root,Math.cos(angle)*49,-32,Math.sin(angle)*49,68+(i%3)*7);
    loaded++;
  }
  // Keep the doorway clear: the central external crown is suspended above the
  // existing server-controlled hollow-trunk entrance and spiral stair region.
  for(let i=0;i<5;i++){
    const a=i*Math.PI*2/5;
    placeModel(trees[i%trees.length],root,Math.cos(a)*14,10,Math.sin(a)*14,66);
    loaded++;
  }
  // Reuse the game's already bundled authored GLB buildings, not primitive huts.
  // These are temporary architecture assets until licensed canopy-house GLBs are sourced.
  const houseURLs=Array.from({length:5},(_,i)=>'/assets/models/buildings/village/village_building_'+String(i+1).padStart(2,'0')+'_lod0.glb');
  const houses=await Promise.allSettled(houseURLs.map(fetchModel));
  const available=houses.filter(r=>r.status==='fulfilled').map(r=>r.value);
  for(let i=0;i<6&&available.length;i++){
    const a=i*Math.PI/3;
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
