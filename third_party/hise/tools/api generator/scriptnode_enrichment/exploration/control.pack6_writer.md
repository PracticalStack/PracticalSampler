# control.pack6_writer - C++ Exploration (Variant)

**Base variant:** control.pack2_writer
**Variant parameter:** NumValues = 6

## Variant-Specific Behaviour

Identical to pack2_writer except the template parameter NumValues is 6 instead of 2. This means:
- 6 Value parameters (Value1 through Value6) writing to slider indices 0-5.
- setExternalData() auto-resizes the connected SliderPack to 6 entries.
- Node ID is "pack6_writer" (generated dynamically).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
