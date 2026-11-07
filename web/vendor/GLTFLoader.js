// Optional GLB loader adapter. A locally vendored GLTFLoader module is required.
// This file intentionally fails with a clear diagnostic until the matching
// three.js examples/jsm/loaders/GLTFLoader.js is installed here.
// Do not silently load a different Three.js version from a CDN.
export class GLTFLoader {
  constructor() { throw new Error('Install the GLTFLoader addon matching web/vendor/three.module.js; see docs/MODEL_PIPELINE.md'); }
}
