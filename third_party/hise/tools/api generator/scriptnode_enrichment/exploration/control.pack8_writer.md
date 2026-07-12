# control.pack8_writer - C++ Exploration (Variant)

**Base variant:** control.pack2_writer
**Variant parameter:** NumValues = 8

## Variant-Specific Behaviour

Identical to pack2_writer except the template parameter NumValues is 8 instead of 2. This means:
- 8 Value parameters (Value1 through Value8) writing to slider indices 0-7.
- setExternalData() auto-resizes the connected SliderPack to 8 entries.
- Node ID is "pack8_writer" (generated dynamically).
- This is the largest variant in the group. The createParameters macro loop supports up to 16 parameters (indices 0-15), so 8 is well within the supported range.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
