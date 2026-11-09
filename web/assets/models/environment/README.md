# Scanned meadow vegetation

Powered by Poly Haven, CC0 1.0. `sources.json` identifies the source assets and exact shipped GLB hashes.

Grass Medium 01 uses the first small grass variant. Shrub 01 is simplified offline using glTF Transform and meshoptimizer. Normals and ARM textures use JPEG 512 at quality 90; alpha-tested base color textures retain their source image when conversion would discard alpha semantics. Output uses ordinary glTF 2.0 with embedded images and no required mesh compression extension.

The renderer normalizes each model's bounds, selects nearby instances from a dense candidate pool, with at most 1500 grass clumps and 40 shrubs (750 / 20 on devices reporting four or fewer CPU threads), and samples native ground height for placement. These are decorative, non-colliding plants. Download failure retains the previous groundcover. Grass is shown within 85 meters and shrubs within 120 meters, with matrix updates after moving eight meters. Performance and actual GPU image quality require device testing.

Modular Fort 01 contributes the actual thin straight wall model, selected and optimized offline; it is fitted to C++-authored short field walls. Its source and hash are also in sources.json. Native collision does not depend on the downloaded mesh.
