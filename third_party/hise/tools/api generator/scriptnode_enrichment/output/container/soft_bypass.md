---
title: Soft Bypass
description: "A serial container with smoothed bypass crossfading to prevent clicks."
factoryPath: container.soft_bypass
factory: container
polyphonic: false
tags: [container, serial, bypass]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "container.chain", type: disambiguation, reason: "Hard bypass with no crossfade" }
  - { id: "container.branch", type: companion, reason: "Index-based switching that benefits from soft bypass wrappers" }
commonMistakes:
  - title: "Modulation output not smoothed during bypass"
    wrong: "Expecting modulation outputs from children to fade smoothly when bypassed"
    right: "Be aware that modulation output is suppressed immediately when bypassed, even though the audio crossfade takes time."
    explanation: "The audio crossfade uses a linear ramp, but modulation output is cut instantly when bypass is engaged. If downstream nodes depend on modulation from inside the soft_bypass, they will see an abrupt change."
  - title: "Series chaining produces clicks instead of smooth transitions"
    wrong: "Placing two soft_bypass containers in series and expecting both to crossfade smoothly"
    right: "Use soft_bypass containers in parallel (e.g. inside a split or via the `template.softbypass_switchN` templates). For series switching, use `container.branch` instead."
    explanation: "The double-ramp crossfade pre-multiplies the input by the ramp before processing. When two soft_bypass nodes are in series, the second node receives an already-ramped signal from the first, causing nested ramp interactions that produce clicks rather than smooth transitions."
llmRef: |
  container.soft_bypass

  A serial container with smoothed bypass crossfading. When bypass is toggled, a linear crossfade transitions between dry and wet signals over a configurable duration, preventing clicks.

  Signal flow:
    Active:     input -> children process serially -> output
    Bypassed:   input -> passthrough -> output
    Transition: input -> crossfade(dry, wet) -> output

  CPU: low, monophonic
    Zero overhead when fully active or fully bypassed. Buffer copy + blend during crossfade.

  Parameters:
    None (bypass is controlled via parameter connections or the node's bypass state)

  Properties:
    SmoothingTime: 0 - 1000 ms (default 20)

  When to use:
    Click-free bypass toggling for effect chains. The only container that accepts dynamic bypass parameter connections. Used internally by template.softbypass_switchN for click-free N-way switching.

  Key details:
    Use math.compare upstream for threshold-based bypass control.
    Series chaining two soft_bypass nodes produces clicks - use parallel arrangement instead.

  Common mistakes:
    Modulation output is suppressed immediately on bypass, not smoothed.
    Series chaining produces clicks due to nested double-ramp interactions.

  See also:
    [disambiguation] container.chain -- hard bypass with no crossfade
    [companion] container.branch -- index-based switching that benefits from soft bypass wrappers
---

The soft bypass container processes children serially (like a chain) but uses a smoothed crossfade when bypass is toggled, preventing clicks. This is the recommended container for any processing path that needs to be toggled on and off during playback.

When bypass is engaged, a linear ramp crossfades from the processed (wet) signal to the dry input over the duration set by the `SmoothingTime` property (default 20 ms). The crossfade applies a double-ramp to the wet signal: the input is pre-multiplied by the ramp before processing, and the output is multiplied again. This prevents stateful effects from seeing sudden full-amplitude input during the transition.

Soft bypass is the only container that accepts dynamic bypass parameter connections. Values of 0.5 or above activate the container; values below 0.5 bypass it. This makes it possible to automate bypass from a control cable or parameter. The `template.softbypass_switchN` nodes use soft_bypass internally for click-free N-way switching.

## Signal Path

::signal-path
---
glossary:
  functions:
    crossfade:
      desc: "Linear ramp blending between dry input and wet (processed) output"
---

```
// container.soft_bypass - smoothed bypass crossfade
// audio in -> audio out

dispatch(input) {
    if active:
        children.process(input)             // serial, in-place
    if bypassed:
        input passes through
    if transitioning:
        crossfade: blend(dry, wet, ramp)    // over SmoothingTime ms
}
```

::

The `SmoothingTime` property (default 20 ms) controls the crossfade duration. Setting it to 0 effectively creates a hard bypass. MIDI events are always forwarded to children regardless of bypass state, keeping their internal state (note tracking, CC values) up to date. Bypass state is shared across all voices in polyphonic contexts -- there is no per-voice bypass. Changing `SmoothingTime` during a crossfade cancels the transition and snaps to the current bypass state.

### Threshold-based Bypass Control

To derive bypass state from a continuous parameter (e.g. bypass a filter when its gain reaches zero), connect a `math.compare` node between the parameter and the soft_bypass. This gives precise threshold control rather than relying on the default 0.5 cutoff.

**See also:** $SN.container.chain$ -- hard bypass with no crossfade, $SN.container.branch$ -- index-based switching that benefits from soft bypass wrappers
