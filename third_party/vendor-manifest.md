# Vendor Manifest

This manifest records the current vendored dependency snapshots imported for Phase 0.

## JUCE

- Path: `third_party/juce/`
- Upstream repository: `https://github.com/juce-framework/JUCE.git`
- Upstream branch: `master`
- Pinned commit: `2cdfca8feb300fb424002ba2c2751569e5bacb64`
- Import date: `2026-07-11`
- Local modifications: none

## HISE

- Path: `third_party/hise/`
- Upstream repository: `https://github.com/christophhart/HISE.git`
- Upstream branch: `develop`
- Pinned commit: `6446c4ab64ba27c189f5d1ad31ecace25d02a292`
- Import date: `2026-07-11`
- Local modifications: none

### HISE vendored submodules included in snapshot

- `third_party/hise/JUCE/`
  - Upstream repository: `https://github.com/christophhart/JUCE_customized.git`
  - Pinned commit: `b19c001a3553478723d8edb8e2fd74fd345c5154`
- `third_party/hise/tools/api generator/`
  - Upstream repository: `https://github.com/christoph-hart/hise_api_generator.git`
  - Pinned commit: `47febaa6f6d171e027765c2da22619f8e44e66a1`
- `third_party/hise/tools/hise_lsp_server/`
  - Upstream repository: `https://github.com/christoph-hart/hise_lsp_server.git`
  - Pinned commit: `1b30f026d4eda9b24fc2f451ac389292dc21f88f`
- `third_party/hise/tools/mcp_server/`
  - Upstream repository: `https://github.com/christoph-hart/hise_mcp_server.git`
  - Pinned commit: `e074eb4387498769fa3c5a77913303e943fbaf58`
