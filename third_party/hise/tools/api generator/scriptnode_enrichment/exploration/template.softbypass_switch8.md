# template.softbypass_switch8 - Composite Exploration (Variant)

**Base variant:** template.softbypass_switch2
**Variant parameter:** N = 8 (number of switch targets, maximum variant)

## Variant-Specific Behaviour

Identical structure to softbypass_switch2 with these differences:

- **Switch parameter range:** 0-7 (step 1)
- **Number of soft_bypass containers:** 8 (sb1-sb8)
- **xfader NumParameters:** 8
- **SwitchTargets:** 8 connections (sb1-sb8 Bypassed)

All other behaviour is identical to the base variant.

### Gap Answer

#### variant-difference: Is the internal logic identical except for the additional switch targets? Is 8 the practical maximum?

Yes, the logic is identical. The C++ template `softbypass_switch<NumSwitches>` imposes no hard maximum -- it is a template parameter. However, the pre-registered variants only go up to 8 (registered in StaticNodeWrappers.cpp). The control.xfader supports up to 16 switch targets (matching the compiled container.branch maximum), so 8 is a design choice rather than a technical limitation. Users needing more than 8 paths would need to build the structure manually.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors:
- Same as base variant; during crossfade transitions, two paths process simultaneously
- With 8 soft_bypass containers in a serial chain, all 8 are traversed per block (7 passing through as dry, 1 processing). The overhead of 7 inactive soft_bypass pass-throughs is negligible.
