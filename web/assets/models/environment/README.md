# Scanned meadow vegetation

Powered by Poly Haven, CC0 1.0. `sources.json` identifies the source assets and exact shipped GLB hashes.

Grass Medium 01 uses the first small grass variant. Shrub 01 is simplified offline using glTF Transform and meshoptimizer. Normals and ARM textures use JPEG 512 at quality 90; alpha-tested base color textures retain their source image when conversion would discard alpha semantics. Output uses ordinary glTF 2.0 with embedded images and no required mesh compression extension.

The renderer normalizes each model's bounds, instances only 180 grass clumps and 14 shrubs, and samples native ground height for placement. These are decorative, non-colliding plants. Download failure retains the previous groundcover. Performance and actual GPU image quality require device testing.
