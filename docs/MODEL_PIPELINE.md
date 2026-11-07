# Aetherwild GLB asset pipeline (stage 1)

The C++ server remains authoritative. This feature adds a browser-side visual model registry and a safe procedural fallback. It does **not** implement rigging, decimation, retargeting, animation or character variants.

## Importing a static GLB

1. Place a GLB in `web/assets/models/characters/` (or another directory under `web/assets/models/`).
2. Install the **matching-version** Three.js `examples/jsm/loaders/GLTFLoader.js` and its imports under `web/vendor/`. The repository currently does not contain the loader. The checked-in adapter throws an actionable error instead of loading an incompatible version from a CDN.
3. Add a registry entry in `web/model-assets.js`, e.g.:

```js
'human:warrior': {
  path: '/assets/models/characters/TRIPO-001.glb',
  scale: 1,
  yaw: 0
}
```

4. Launch the workstation. Player and NPC humanoids with the matching role request the registered asset asynchronously. Until the GLB is loaded, or on any error, the original procedural model remains visible.

## Supported in this change

- Strict local GLB path validation; registry IDs; lazy loading and promise cache.
- Static mesh cloning, transform offsets, shadows and graceful fallback.
- Visual-only integration at `RealmScene.human`; existing C++ rules unchanged.

## Not yet supported

- GLTFLoader addon is **not bundled**: the adapter must be replaced with a compatible upstream loader before GLB imports work.
- Skinned mesh cloning, skeleton binding, animation mixers, retargeting, material variants, LOD, equipment sockets.
- Model preview UI, bulk ingestion, binary asset upload, GPU instancing and automated rendering QA.

Do not declare a GLB playable until the compatible loader is installed and an actual model is tested. Preserve source asset files and use a reviewable branch/PR.
