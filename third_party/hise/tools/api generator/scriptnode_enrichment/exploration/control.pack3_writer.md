# control.pack3_writer - C++ Exploration (Variant)

**Base variant:** control.pack2_writer
**Variant parameter:** NumValues = 3

## Variant-Specific Behaviour

Identical to pack2_writer except the template parameter NumValues is 3 instead of 2. This means:
- 3 Value parameters (Value1, Value2, Value3) writing to slider indices 0-2.
- setExternalData() auto-resizes the connected SliderPack to 3 entries.
- Node ID is "pack3_writer" (generated dynamically).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
