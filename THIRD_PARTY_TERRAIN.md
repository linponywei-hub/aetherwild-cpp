# Third-party terrain texture integration

World terrain uses three real external image textures from the Three.js example assets, sampled in world coordinates with biome masks. This is a first pass, not a complete replacement of all regional terrain textures.

- Grass: `https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/terrain/grasslight-big.jpg` — Three.js terrain examples. The terrain folder's readme attributes OpenGameArt sources and specifies CC BY 3.0; see `https://github.com/mrdoob/three.js/blob/dev/examples/textures/terrain/readme.txt`.
- Stone/brick: `https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/brick_diffuse.jpg` — Three.js examples asset. Check source-specific attribution before redistributing.
- Lava: `https://raw.githubusercontent.com/mrdoob/three.js/dev/examples/textures/lava/lavatile.jpg` — Three.js examples asset. Check source-specific attribution before redistributing.

All three are fetched at runtime, not yet vendored into this repository. If a request fails, the procedural shader remains active; no third-party imagery is falsely reported as deployed. Runtime network and CORS access are required. Images are cached once per page and shared across terrain tiles. Before production, replace with vetted, locally hosted, optimized CC0/compatible textures for sand, snow, rock, marsh, and forest; verify licenses and use dedicated texture channels (albedo, normal, roughness) where available.

Server-authoritative height and collision are unchanged.
