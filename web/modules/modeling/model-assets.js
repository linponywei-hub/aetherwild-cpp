import * as T from '/vendor/three.module.js';
import {BASE_MODELS, MODEL_VARIANTS, resolveVariant} from '/modules/modeling/model-catalog.js';
// Visual-only registry. No authoritative game rules belong in this module.
// Register GLB files here after placing them under web/assets/models/.
export const MODEL_ASSETS = Object.freeze(Object.fromEntries(Object.entries(MODEL_VARIANTS).map(([id]) => [id, resolveVariant(id)])));

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
    this.animationCache = new Map();
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
    const cacheKey = asset.path;
    if (!this.cache.has(cacheKey)) {
      const task = this.loader().then(loader => loader.loadAsync(asset.path))
        .then(async gltf => {
          const root = gltf.scene;
          root.traverse(node => {
            if (node.isMesh) { node.castShadow = true; node.receiveShadow = true; }
          });
          let animations = gltf.animations || [];
          if (asset.animationPath) {
            if (!this.animationCache.has(asset.animationPath)) {
              this.animationCache.set(asset.animationPath, this.loader().then(loader => loader.loadAsync(asset.animationPath)).catch(error => {this.animationCache.delete(asset.animationPath);throw error;}));
            }
            const library = await this.animationCache.get(asset.animationPath);
            animations = library.animations;
            const names = new Set();root.traverse(node => names.add(node.name));
            for (const clip of animations) for (const track of clip.tracks) {
              const target = T.PropertyBinding.parseTrackName(track.name).nodeName;
              if (!names.has(target)) throw Error('Animation joint missing: '+target);
            }
          }
          return { root, asset, animations };
        })
        .catch(error => { this.cache.delete(cacheKey); throw error; });
      this.cache.set(cacheKey, task);
    }
    return this.cache.get(cacheKey);
  }
  async instantiate(id) {
    const { root, animations } = await this.template(id);
    const asset = validateModelAsset(this.registry[id]);
    // Rebuild the skeleton against cloned bones: Object3D.clone() otherwise
    // retains bone references from the cached template and breaks animation.
    const originals = [], copies = [];
    root.traverse(node => originals.push(node));
    const instance = root.clone(true);
    instance.traverse(node => copies.push(node));
    const lookup = new Map(originals.map((node, i) => [node, copies[i]]));
    for (const source of originals) {
      if (!source.isSkinnedMesh) continue;
      const target = lookup.get(source);
      const skeleton = new T.Skeleton(
        source.skeleton.bones.map(bone => { const cloned = lookup.get(bone); if (!cloned) throw new Error('Missing cloned GLB joint'); return cloned; }),
        source.skeleton.boneInverses.map(matrix => matrix.clone())
      );
      target.bind(skeleton, source.bindMatrix.clone());
    }
    // Measure the actual skinned neutral pose; normalize the whole asset, never bones.
    if (asset.height) {
      instance.updateMatrixWorld(true);
      instance.traverse(node => { if (node.isSkinnedMesh) node.computeBoundingBox(); });
      const bounds = new T.Box3().setFromObject(instance);
      const height = bounds.max.y - bounds.min.y;
      if (!Number.isFinite(height) || height < .001) throw Error('Invalid character bounds');
      instance.scale.multiplyScalar(asset.height / height);
      instance.updateMatrixWorld(true);
      const grounded = new T.Box3().setFromObject(instance);
      instance.position.x -= (grounded.min.x + grounded.max.x) / 2;
      instance.position.z -= (grounded.min.z + grounded.max.z) / 2;
      instance.position.y -= grounded.min.y;
    }
    instance.traverse(node => {if (node.name === 'Hair_Beard') node.visible=!!asset.beard;});
    instance.scale.multiplyScalar(asset.scale ?? 1);
    instance.rotation.y += asset.yaw ?? 0;
    if (asset.tint && asset.tint !== '#ffffff') instance.traverse(node => {
      if (!node.isMesh || /head|hair|eye|brow|beard/i.test(node.name)) return;
      node.material = Array.isArray(node.material) ? node.material.map(m => m.clone()) : node.material.clone();
      for (const material of Array.isArray(node.material) ? node.material : [node.material]) material.color?.multiply(new (material.color.constructor)(asset.tint));
    });
    return { instance, animations };
  }
  async attach(group, id) {
    if (!this.registry[id]) return false;
    try {
      const { instance, animations } = await this.instantiate(id);
      if (!group.parent || group.userData.modelAssetId !== id) return false;
      const previous = group.userData.modelInstance;
      if (previous) group.remove(previous);
      group.add(instance);
      group.userData.modelInstance = instance;
      if (group.userData.modelMixer) group.userData.modelMixer.stopAllAction();
      group.userData.modelMixer = null;
      // Drive the supplied GLB clips using the cloned skeleton, without forced joint offsets.
      if (animations.length) {
        const mixer = new T.AnimationMixer(instance);
        const idle = animations.find(clip => /idle|breathe|待機/i.test(clip.name)) || animations[0];
        mixer.clipAction(idle).play();
        group.userData.modelMixer = mixer;
        group.userData.modelClips = animations;
      }
      group.userData.modelHealth = {status: 'loaded', animationCount: animations.length};
      // Hide only the procedural body. Preserve labels, collision proxies and weapons.
      for (const part of group.userData.proceduralParts || []) part.visible = false;
      return true;
    } catch (error) {
      console.warn('[Aetherwild] GLB fallback:', id, error);
      return false;
    }
  }
  clearCache() { this.cache.clear(); }
  registeredIds() { return Object.keys(this.registry); }
}
