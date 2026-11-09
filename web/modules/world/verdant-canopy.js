import * as T from '../../vendor/three.module.js';

// External-asset-only Verdant visual root. Gameplay collision stays on the server.
// No procedural tree, hut, stair, railing or bridge mesh is created here.
export function addVerdantCanopy(scene) {
  const region = scene.catalog.regions.find(r => r.id === 'verdant');
  if (!region) return null;
  const root = new T.Group();
  root.name = 'verdant-external-city';
  root.position.set(region.position.x, region.elevation, region.position.z);
  root.userData.regionId = 'verdant';
  root.userData.assetPhase = 'external-only';
  root.userData.visualOnly = true;
  scene.root.add(root);
  return root;
}
