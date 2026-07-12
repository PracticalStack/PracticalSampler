# template.softbypass_switch7 - Composite Exploration (Variant)

**Base variant:** template.softbypass_switch2
**Variant parameter:** N = 7 (number of switch targets)

## Variant-Specific Behaviour

Identical structure to softbypass_switch2 with these differences:

- **Switch parameter range:** 0-6 (step 1)
- **Number of soft_bypass containers:** 7 (sb1-sb7)
- **xfader NumParameters:** 7
- **SwitchTargets:** 7 connections (sb1-sb7 Bypassed)

All other behaviour is identical to the base variant.

### Gap Answer

#### variant-difference: Is the internal logic identical except for the additional switch targets?

Yes. Generated from the same `softbypass_switch<7>` template. Only N-dependent values differ.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors:
- Same as base variant; during crossfade transitions, two paths process simultaneously
