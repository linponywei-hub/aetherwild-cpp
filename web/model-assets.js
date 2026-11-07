import * as THREE from '/vendor/three.module.js';

// Visual-only registry. No authoritative game rules belong in this module.
// Register GLB files here after placing them under web/assets/models/.
export const MODEL_ASSETS = Object.freeze({
  // Example (uncomment after uploading a real file):
  // 'human:warrior': { path: '/assets/models/characters/TRIPO-001.glb', scale: 1, yaw: 0 },
});

export function validateModelAsset(asset) {
  if (!asset || typeof asset !== 'object') throw new Error('Invalid model descriptor');
  if (typeof asset.path !== 'string' || !/^\/assets\/models\/[a-zA-Z0-9_./-]+\.glb$/.test(asset.path) || asset.path.includes('..')) {
    throw new Error('Model path must be a local /assets/models/*.glb URL');
  }
  if (asset.scale !== undefined && (!Number.isFinite(asset.scale) || asset.scale <= 0 || asset.scale > 100)) throw new Error('Invalid model scale');
  if (asset.yaw !== undefined && !Number.isFinite(asset.yaw)) throw new Error('Invalid model yaw');
  return asset;
}

export class ModelAssets {
  constructor(registry = MODEL_ASSETS) {
    this.registry = registry;
    this.cache = new Map();
    this.loaderPromise = null;
  }
  async loader() {
    // Keep the Three.js dependency identical to the game's existing vendor build.
    // The addon is loaded only when a registered GLB is actually requested.
    if (!this.loaderPromise) this.loaderPromise = import('/vendor/GLTFLoader.js')
      .then(module => new module.GLTFLoader())
      .catch(error => { this.loaderPromise = null; throw error; });
    return this.loaderPromise;
  }
  async template(id) {
    const asset = validateModelAsset(this.registry[id]);
    if (!this.cache.has(id)) {
      const task = this.loader().then(loader => loader.loadAsync(asset.path))
        .then(gltf => {
          const root = gltf.scene;
          root.traverse(node => {
            if (node.isMesh) { node.castShadow = true; node.receiveShadow = true; }
          });
          return { root, asset, animations: gltf.animations || [] };
        })
        .catch(error => { this.cache.delete(id); throw error; });
      this.cache.set(id, task);
    }
    return this.cache.get(id);
  }
  async instantiate(id) {
    const { root, asset, animations } = await this.template(id);
    // Cloning skinned meshes needs SkeletonUtils; defer that to the rigging stage.
    if (root.getObjectByProperty('isSkinnedMesh', true)) {
      throw new Error('Skinned GLB requires SkeletonUtils integration before instancing');
    }
    const instance = root.clone(true);
    instance.scale.multiplyScalar(asset.scale ?? 1);
    instance.rotation.y += asset.yaw ?? 0;
    return { instance, animations };
  }
  async attach(group, id) {
    if (!this.registry[id]) return false;
    try {
      const { instance } = await this.instantiate(id);
      if (!group.parent || group.userData.modelAssetId !== id) return false;
      const previous = group.userData.modelInstance;
      if (previous) group.remove(previous);
      group.add(instance);
      group.userData.modelInstance = instance;
      // Hide only the procedural body. Preserve labels, collision proxies and weapons.
      for (const part of group.userData.proceduralParts || []) part.visible = false;
      return true;
    } catch (error) {
      console.warn('[Aetherwild] GLB fallback:', id, error);
      return false;
    }
  }
  clearCache() { this.cache.clear(); }
}
