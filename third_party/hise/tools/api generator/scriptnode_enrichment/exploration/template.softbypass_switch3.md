# template.softbypass_switch3 - Composite Exploration (Variant)

**Base variant:** template.softbypass_switch2
**Variant parameter:** N = 3 (number of switch targets)

## Variant-Specific Behaviour

Identical structure to softbypass_switch2 with these differences:

- **Switch parameter range:** 0-2 (step 1) instead of 0-1
- **Number of soft_bypass containers:** 3 (sb1, sb2, sb3) instead of 2
- **xfader NumParameters:** 3 instead of 2
- **SwitchTargets:** 3 connections (sb1.Bypassed, sb2.Bypassed, sb3.Bypassed)

All other behaviour (crossfade mechanism, smoothing time default of 20ms, dummy placeholders, serial signal flow through sb_container) is identical to the base variant.

### Gap Answer

#### variant-difference: Is the internal logic identical except for the additional switch target?

Yes. The C++ template `softbypass_switch<NumSwitches>` (StaticNodeWrappers.cpp:756) generates the structure procedurally using the NumSwitches template parameter. The only differences are: the Switch parameter max value (N-1 = 2), the number of soft_bypass children in sb_container (3), and the xfader's NumParameters property (3). All wiring and behaviour is identical.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors:
- Same as base variant; during crossfade transitions, two paths process simultaneously
