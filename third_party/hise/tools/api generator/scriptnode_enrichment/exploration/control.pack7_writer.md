# control.pack7_writer - C++ Exploration (Variant)

**Base variant:** control.pack2_writer
**Variant parameter:** NumValues = 7

## Variant-Specific Behaviour

Identical to pack2_writer except the template parameter NumValues is 7 instead of 2. This means:
- 7 Value parameters (Value1 through Value7) writing to slider indices 0-6.
- setExternalData() auto-resizes the connected SliderPack to 7 entries.
- Node ID is "pack7_writer" (generated dynamically).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
