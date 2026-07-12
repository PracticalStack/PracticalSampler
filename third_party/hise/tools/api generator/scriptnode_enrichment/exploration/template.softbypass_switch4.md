# template.softbypass_switch4 - Composite Exploration (Variant)

**Base variant:** template.softbypass_switch2
**Variant parameter:** N = 4 (number of switch targets)

## Variant-Specific Behaviour

Identical structure to softbypass_switch2 with these differences:

- **Switch parameter range:** 0-3 (step 1)
- **Number of soft_bypass containers:** 4 (sb1, sb2, sb3, sb4)
- **xfader NumParameters:** 4
- **SwitchTargets:** 4 connections (sb1-sb4 Bypassed)

All other behaviour is identical to the base variant.

### Gap Answer

#### variant-difference: Is the internal logic identical except for the additional switch targets?

Yes. Generated from the same `softbypass_switch<4>` template. Only N-dependent values differ.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors:
- Same as base variant; during crossfade transitions, two paths process simultaneously
