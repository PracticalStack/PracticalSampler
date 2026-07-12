# control.pack5_writer - C++ Exploration (Variant)

**Base variant:** control.pack2_writer
**Variant parameter:** NumValues = 5

## Variant-Specific Behaviour

Identical to pack2_writer except the template parameter NumValues is 5 instead of 2. This means:
- 5 Value parameters (Value1 through Value5) writing to slider indices 0-4.
- setExternalData() auto-resizes the connected SliderPack to 5 entries.
- Node ID is "pack5_writer" (generated dynamically).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
