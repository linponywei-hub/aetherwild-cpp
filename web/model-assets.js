import * as T from '/vendor/three.module.js';
import {BASE_MODELS, MODEL_VARIANTS, resolveVariant} from '/model-catalog.js';
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
    const cacheKey = asset.baseId || id;
    if (!this.cache.has(cacheKey)) {
      const task = this.loader().then(loader => loader.loadAsync(asset.path))
        .then(gltf => {
          const root = gltf.scene;
          root.traverse(node => {
            if (node.isMesh) { node.castShadow = true; node.receiveShadow = true; }
          });
          return { root, asset, animations: gltf.animations || [] };
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
    // Size the imported character in world units without touching its bone transforms.
    // A neutral pose is used for bounds; GLB authoring units may be centimetres.
    if (id === 'HUM_EILEEN_TEST_001') {
      instance.updateMatrixWorld(true);
      const box = new T.Box3().setFromObject(instance);
      const height = box.max.y - box.min.y;
      if (Number.isFinite(height) && height > 0.0001) {
        const targetHeight = 2.795; // player camera is at 1.68 world units
        const scale = Math.min(10000, Math.max(0.0001, targetHeight / height));
        instance.scale.multiplyScalar(scale);
        instance.updateMatrixWorld(true);
        const grounded = new T.Box3().setFromObject(instance);
        instance.position.y -= grounded.min.y;
      } else console.warn('[Aetherwild] Eileen has invalid bounds; retaining original size');
    }
    instance.scale.multiplyScalar(asset.scale ?? 1);
    instance.rotation.y += asset.yaw ?? 0;
    if (asset.tint && asset.tint !== '#ffffff') instance.traverse(node => {
      if (!node.isMesh) return;
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
      // The supplied GLB animation clips do not keyframe either upper arm.
      // Add an explicit shoulder-down pose on the existing skin (not the mesh root).
      if (id === 'HUM_EILEEN_TEST_001') {
        const joints = {};
        instance.traverse(node => { if (node.isBone) joints[node.name] = node; });
        group.userData.eileenShoulders = [
          { bone: joints.Eileen_LeftUpperArm, angle: 0.88 },
          { bone: joints.Eileen_RightUpperArm, angle: -0.88 }
        ].filter(entry => !!entry.bone);
      }
      group.userData.modelHealth = {status: id === 'HUM_EILEEN_TEST_001' ? 'proxy-rig-animation-test' : 'loaded', animationCount: animations.length};
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
