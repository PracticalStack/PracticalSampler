# EmptyFX - C++ Exploration

**Source:** `hi_core/hi_modules/effects/fx/GainEffect.h` (lines 38-87), `GainEffect.cpp` (lines 385-430)
**Base class:** `MasterEffectProcessor`

## Signal Path

Pure passthrough. `applyEffect()` (GainEffect.h:82-85) has an empty body - it does nothing to the audio buffer. The input passes through to the output unchanged.

audio in -> audio out (unchanged)

## Gap Answers

No gaps were identified in Steps 1-2 because the module is trivially simple.

## Processing Chain Detail

No processing stages. The module has:
- No parameters (`setInternalAttribute` and `getAttribute` are no-ops)
- No modulation chains (`getNumInternalChains() = 0`)
- No child processors (`getNumChildProcessors() = 0`)
- No tail (`hasTail() = false`)
- No soft bypass behaviour (`setSoftBypass` is a no-op, `isFadeOutPending` returns false)

## CPU Assessment

- **Overall baseline**: negligible (literally zero processing)
- **No scaling factors**

## UI Components

- Backend editor: `EmptyProcessorEditorBody` (GainEffect.cpp:421) - a generic empty editor body

## Notes

EmptyFX serves as the default child processor for SlotFX when no effect is loaded. It is also available as a standalone effect for structural purposes (e.g. as a routing placeholder). The metadata description is "A placeholder effect that passes audio through unchanged, useful for routing or as a template."
