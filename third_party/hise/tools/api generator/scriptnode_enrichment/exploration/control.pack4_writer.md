# control.pack4_writer - C++ Exploration (Variant)

**Base variant:** control.pack2_writer
**Variant parameter:** NumValues = 4

## Variant-Specific Behaviour

Identical to pack2_writer except the template parameter NumValues is 4 instead of 2. This means:
- 4 Value parameters (Value1 through Value4) writing to slider indices 0-3.
- setExternalData() auto-resizes the connected SliderPack to 4 entries.
- Node ID is "pack4_writer" (generated dynamically).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
