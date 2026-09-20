# RendererLab ocean work — 2026-09-20

Active project: `S:/spacesim/spacesim-recovered-server-ready/ssss/spacesim`.
Executable: `build/Release/RendererLab.exe`. This changes RendererLab, not the original raylib game.

## Implemented in this pass

- Replaced the 512x512 uniform wave grid with a dense center and ten concentric annuli. Inner spacing is 4 metres; outer half-width is 262.144 km. Fine outer boundary vertices collapse onto the next ring's sampling lattice. Ring holes avoid overlapping grids.
- Geometry contains 183,051 allocated vertices and 278,528 triangles, versus 524,288 triangles in the original grid. This is a geometry-count reduction, not a measured whole-frame performance claim.
- Geometric wave amplitudes are filtered continuously according to spatial resolution. Shared ring vertices evaluate the same displacement. Gerstner tangent derivatives include horizontal displacement when computing normals.
- Removed the artificial 0.5-to-multiple-metres lift above the globe. The globe discards ocean coverage owned by the local grid; the local fragment path rejects land and shades only water. A local analytic sea-sphere depth correction handles the fallback sphere's chord depth while local coverage is active.
- Added `hasOcean`, physical `radiusKm`, and ocean gravity to `PlanetRenderObject`. The nearest eligible spherical ocean is selected explicitly, instead of `planets.front()`. Ocean geometry no longer requires an atmosphere instance.
- Kept procedural coastline eligibility shared between both passes. This is still a procedural mask, not final terrain/seabed intersection.
- Added filtered aperiodic metre-scale normal ripples on local water. They replace the old kilometre-scale detail field locally and blend back toward the distant material as the geometric ocean fades with altitude.
- Persisted the local grid orientation to avoid the old tangent helper-axis threshold jump. Wave phase remains anchored to planetary position rather than to the camera's grid coordinates.
- Constructed nearby geometry with a stable curvature formula and camera-relative vertex projection. This improves local precision; it does not implement a complete double-precision world system.
- Corrected the surface camera label to 50 m. Its previous near clipping distance represented about 127 m. The near plane now adapts to altitude with a metre-scale minimum.

## Verification

Release RendererLab build succeeded. Hidden runtime shader compilation and six view captures completed with OpenGL error 0 on an RTX 2080 SUPER.

The actual generated ring mesh is checked in diagnostic mode for index validity, outward winding, internal edge sharing, outer boundary ownership and exact total plane coverage. Collapsed stitching triangles are intentionally degenerate. This validates topology; it does not prove every rendered horizon/coast transition is artifact-free.

Run from `build/Release`:

```powershell
$env:SPACESIM_HIDDEN_CHECK='1'
.\RendererLab.exe --ocean-check
Remove-Item Env:SPACESIM_HIDDEN_CHECK
```

The diagnostic saves `ocean-check-0.ppm` through `ocean-check-5.ppm`: 50 m, translated surface camera, 1 km, 3.9 km, no atmosphere, and orbit. The magenta no-atmosphere background is the existing colored test cubemap (`TestEnvironment.cpp`), not a shader compilation failure. The diagnostic changes views discretely and allows only a short exposure settling period; continuous flight and exposure quality need separate testing.

The first verification caught a reserved GLSL identifier in the new ripple helper; it was corrected and the full diagnostic rerun successfully.

Original edited files are backed up in the original workspace at:
`S:/spacesim/spacesim-recovered-server-ready/spacesim-recovered-server-ready/build/ocean-rework/original`.

## Remaining work and limits

1. Validate continuous movement, changing wave strength, camera altitude through 4 km, rotated/translated planets, multi-planet selection, and coastlines. Only the current single-planet presets have been visually checked.
2. Sky reflection is still the existing approximate local-sky integration, not a fully roughness-prefiltered environment BRDF. Broad grazing highlights remain; do not call the ocean visually final.
3. The concentric mesh follows the camera with stable planetary wave phase. It is not yet a fully snapped clipmap with temporal morphing. Mesh resampling under movement can still cause shimmer.
4. Geometry waves filter into the existing far-water appearance rather than a rigorously energy-conserving common wave spectrum. Normal derivatives omit the spatial derivative of the resolution/fade envelope and small curved-basis terms.
5. The analytic fallback depth correction is for a spherical prototype. Replace that ownership/depth strategy as real terrain and ocean geometry are integrated. Test shallow water before extending this to coasts; there is no seabed, refraction, foam or underwater rendering yet.
6. A single local ocean host and single atmosphere are still the rendering model. Nonuniformly scaled spheres are excluded from local ocean selection. No terrain collision or boat buoyancy was added.
7. Some coordinates and shaders still use float world positions. Long-distance precision, reversed depth or alternative depth strategy, and complete camera-relative lighting remain integration work.

## Roadmap after this pass

Finish ocean stability and reflection validation, then bring one real adaptive coastline into RendererLab. Establish common coordinates, elevation, sea level and collision queries before extensive land-material/cloud polish. Develop terrain geometry and materials together. Clouds, advanced water features and additional planet classes follow. The atmosphere foundation is retained; its broader parameter and performance validation is still needed.
