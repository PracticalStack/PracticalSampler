# template.softbypass_switch6 - Composite Exploration (Variant)

**Base variant:** template.softbypass_switch2
**Variant parameter:** N = 6 (number of switch targets)

## Variant-Specific Behaviour

Identical structure to softbypass_switch2 with these differences:

- **Switch parameter range:** 0-5 (step 1)
- **Number of soft_bypass containers:** 6 (sb1-sb6)
- **xfader NumParameters:** 6
- **SwitchTargets:** 6 connections (sb1-sb6 Bypassed)

All other behaviour is identical to the base variant.

### Gap Answer

#### variant-difference: Is the internal logic identical except for the additional switch targets?

Yes. Generated from the same `softbypass_switch<6>` template. Only N-dependent values differ.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors:
- Same as base variant; during crossfade transitions, two paths process simultaneously
