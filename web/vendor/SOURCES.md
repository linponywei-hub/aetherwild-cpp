# Third-party runtime components

Three.js core, GLTFLoader, OrbitControls, TransformControls and BufferGeometryUtils are pinned to upstream r160 (MIT). See `three-LICENSE.txt`.

Sources:
- https://github.com/mrdoob/three.js/tree/r160
- https://github.com/mrdoob/three.js/blob/r160/examples/jsm/loaders/GLTFLoader.js
- https://github.com/mrdoob/three.js/blob/r160/examples/jsm/controls/OrbitControls.js
- https://github.com/mrdoob/three.js/blob/r160/examples/jsm/controls/TransformControls.js
- https://github.com/mrdoob/three.js/blob/r160/examples/jsm/utils/BufferGeometryUtils.js

The only vendoring edits change package/relative imports to the local `three.module.js` and `BufferGeometryUtils.js`. No runtime CDN required. The previous bespoke GLB reader is replaced by the official loader, not retained as a second implementation.
