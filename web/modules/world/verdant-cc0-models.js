import * as T from '../../vendor/three.module.js';

// Load an externally hosted Quaternius CC0 tree model; keep local bridge placeholder. Missing assets leave the procedural city intact.
// The game server, not these GLBs, owns all collision.
const SOURCES = Object.freeze({
  canopy: 'https://raw.githubusercontent.com/anshaneja5/skyline-run/main/public/assets/models/tree1.glb',
  bridge: '/assets/models/verdant/aetherwild-rope-bridge.glb'
});
let loaderPromise;
function loader() {
  return loaderPromise ??= import('../../vendor/GLTFLoader.js').then(({GLTFLoader}) => new GLTFLoader());
}
async function fetchModel(url) {
  const response = await fetch(url);
  if (!response.ok) throw new Error('Missing model: '+url+' ('+response.status+')');
  const data = await response.arrayBuffer();
  const gltfLoader = await loader();
  return await new Promise((resolve,reject) => gltfLoader.parse(data,url.slice(0,url.lastIndexOf('/')+1),resolve,reject));
}
function normalize(mesh, target) {
  mesh.updateMatrixWorld(true);
  const bounds = new T.Box3().setFromObject(mesh);
  const size = bounds.getSize(new T.Vector3());
  if (!(size.x>0 && size.y>0 && size.z>0)) throw new Error('Invalid model bounds');
  mesh.scale.multiplyScalar(target/Math.max(size.x,size.y,size.z));
  mesh.updateMatrixWorld(true);
  const resized = new T.Box3().setFromObject(mesh);
  mesh.position.sub(resized.getCenter(new T.Vector3()));
  return mesh;
}
export async function attachVerdantCC0Models(canopyRoot) {
  if (!canopyRoot) return {loaded:0};
  const treePositions = Array.from({length:6},(_,i)=>{
    const a=i*Math.PI/3;return [Math.cos(a)*49,Math.sin(a)*49];
  });
  let loaded=0;
  // Only decorative crowns: keep the hollow central trunk and doorway open.
  try {
    const gltf=await fetchModel(SOURCES.canopy);
    // Hide procedural foliage only when a real tree asset loads successfully.
    canopyRoot.children.filter(node=>node.isMesh && node.material?.color?.getHexString && ['54764b'].includes(node.material.color.getHexString())).forEach(node=>{node.visible=false;});
    for(const [x,z] of treePositions) {
      const model=normalize(gltf.scene.clone(true),30);
      model.position.add(new T.Vector3(x,17,z));
      canopyRoot.add(model);
      loaded++;
    }
  } catch(error) { console.info('[Verdant] CC0 tree model unavailable; using procedural foliage:',error.message); }
  try {
    const gltf=await fetchModel(SOURCES.bridge);
    for(let i=0;i<6;i++) {
      const [x,z]=treePositions[i], [nx,nz]=treePositions[(i+1)%6];
      const dx=nx-x,dz=nz-z;
      const bridge=normalize(gltf.scene.clone(true),Math.hypot(dx,dz));
      // Source bridge longitudinal axis is X; face the next platform.
      bridge.rotation.y=-Math.atan2(dz,dx);
      bridge.position.add(new T.Vector3((x+nx)/2,2,(z+nz)/2));
      canopyRoot.add(bridge);
      loaded++;
    }
  } catch(error) { console.info('[Verdant] CC0 bridge model unavailable; using procedural spans:',error.message); }
  canopyRoot.userData.externalCC0Models=loaded;
  return {loaded};
}
