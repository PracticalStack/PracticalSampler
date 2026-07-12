---
title: Empty
moduleId: EmptyFX
type: Effect
subtype: MasterEffect
tags: [utility]
builderPath: b.Effects.EmptyFX
screenshot: /images/v2/reference/audio-modules/emptyfx.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: SlotFX, type: companion, reason: "Uses Empty as the default placeholder when no effect is loaded in the slot" }
commonMistakes: []
llmRef: |
  Empty (MasterEffect)

  A placeholder effect that passes audio through unchanged. Has no parameters, no modulation chains, and no processing. Used as a structural placeholder in effect chains and as the default child of the Effect Slot when no effect is loaded.

  Signal flow:
    audio in -> audio out (passthrough)

  CPU: negligible (zero processing), monophonic.

  Parameters: none.

  When to use:
    As a placeholder in an effect chain for routing purposes, or when you need a slot that does nothing. Typically not added manually - it is the default state of an empty Effect Slot.

  See also:
    companion Effect Slot - uses Empty as its default placeholder
---

::category-tags
---
tags:
  - { name: utility, desc: "Modules for analysis, placeholders, or structural purposes without audio processing" }
---
::

![Empty screenshot](/images/v2/reference/audio-modules/emptyfx.png)

A placeholder effect that passes audio through unchanged. It has no parameters, no modulation chains, and performs no processing whatsoever.

The Empty effect is primarily used internally as the default child of the Effect Slot when no effect is loaded. It can also be added manually as a structural placeholder in an effect chain for routing purposes.

**See also:** $MODULES.SlotFX$ -- Uses Empty as the default placeholder when no effect is loaded in the slot
