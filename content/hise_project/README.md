# HISE Project Content

This directory is the product-owned HISE authoring content root for Decent Rhapsody Studio.

It is intentionally separate from `third_party/hise/` so presets, sample maps, images, scripts,
and other project assets stay under first-party control while HISE itself remains an external
vendored dependency.

The initial Phase 0 layout mirrors the core HISE project folder conventions:

- `Images/`
- `SampleMaps/`
- `UserPresets/`
- `AudioFiles/`
- `Samples/`
- `Expansions/`
- `Scripts/`
- `DspNetworks/`
- `XmlPresetBackups/`

These folders are scaffolded now so the `engine_adapter` can validate concrete path resolution
and asset discovery before deeper runtime integration.
