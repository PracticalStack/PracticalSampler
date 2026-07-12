---
title: JDSP Nodes
factory: jdsp
---

DSP algorithm wrappers from JUCE `dsp` module. Nodes expose standard JUCE effects/utilities as scriptnode nodes, accessing well-tested common audio processing implementations.

JDSP nodes process audio in blocks using JUCE `AudioBlock` interface internally. Delay variants differ in interpolation algorithms with trade-offs between CPU, frequency response, modulation suitability.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.jdsp.jchorus$]($SN.jdsp.jchorus$) | Chorus effect with LFO-modulated delay, feedback, and wet/dry mix |
| [$SN.jdsp.jcompressor$]($SN.jdsp.jcompressor$) | Simple compressor with gain reduction modulation output |
| [$SN.jdsp.jdelay$]($SN.jdsp.jdelay$) | Delay line with linear interpolation (cheapest, mild high-frequency roll-off) |
| [$SN.jdsp.jdelay_cubic$]($SN.jdsp.jdelay_cubic$) | Delay line with cubic Lagrange interpolation (flat response, highest CPU) |
| [$SN.jdsp.jdelay_thiran$]($SN.jdsp.jdelay_thiran$) | Delay line with Thiran allpass interpolation (flat response, not for fast modulation) |
| [$SN.jdsp.jlinkwitzriley$]($SN.jdsp.jlinkwitzriley$) | 4th-order Linkwitz-Riley crossover filter (LP/HP/AP) |
| [$SN.jdsp.jpanner$]($SN.jdsp.jpanner$) | Stereo panner with seven selectable panning laws |
