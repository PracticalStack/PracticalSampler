---
title: Macro Modulation Source
moduleId: MacroModulationSource
type: SoundGenerator
subtype: SoundGenerator
tags: [routing]
builderPath: b.SoundGenerators.MacroModulationSource
screenshot: /images/v2/reference/audio-modules/macromodulationsource.png
cpuProfile:
  baseline: very low
  polyphonic: false
  scalingFactors: [number of active macro chains, modulator complexity]
seeAlso:
  - { id: MacroModulator, type: alternative, reason: "The receive side of the macro system - reads macro slot values and converts them to modulation signals with smoothing and table support" }
commonMistakes:
  - title: "Macro Source produces no audio"
    wrong: "Expecting the Macro Modulation Source to produce audio output"
    right: "This module produces no audio. It only drives macro control slots from its modulation chains."
    explanation: "Despite being classified as a SoundGenerator, this module does not generate sound. The SoundGenerator infrastructure is used to host the macro modulation chains within the rendering pipeline."
  - title: "Gain and Balance have no effect"
    wrong: "Adjusting the Gain or Balance knobs and expecting them to affect the macro output"
    right: "Control the macro values by adding modulators to the Macro 1-8 chains"
    explanation: "The Gain and Balance parameters are inherited from the base class but are not applied in this module's processing. The macro chain outputs are read directly."
customEquivalent:
  approach: scriptnode
  moduleType: SoundGenerator
  complexity: low
  description: "Global cables sending modulation values to macro slots can replicate this pattern in scriptnode"
llmRef: |
  Macro Modulation Source (SoundGenerator)

  A container that hosts modulation chains whose output drives the global macro control system. Does not generate audio - uses the SoundGenerator infrastructure to participate in the rendering pipeline and host modulation chains.

  Signal flow:
    Macro chains (1-8) -> render chains -> read first sample per block -> delta check -> scale to 0-127 -> macro control system

  CPU: very low base, depends on modulator complexity in macro chains. Monophonic.

  Parameters (inherited, vestigial):
    Gain (0-100%, default 25%) - NOT applied in processing
    Balance (-1 to 1, default 0) - NOT applied in processing
    VoiceLimit (1-256, default 256) - irrelevant (no audio voices)
    KillFadeTime (0-20000 ms, default 20 ms) - irrelevant (no audio voices)

  Modulation chains (active):
    Macro 1-8 - each chain's output drives the corresponding macro control slot. Updated per-block. Any modulator type accepted.

  Modulation chains (disabled):
    Gain Modulation - not applicable (no audio)
    Pitch Modulation - not applicable (no oscillator)

  When to use:
    Driving macro controls from modulation sources (LFOs, envelopes, constant modulators). Place modulators in the Macro 1-8 chains to automate macro knob values from within the module tree.

  Common mistakes:
    This module produces no audio despite being a SoundGenerator.
    Gain/Balance knobs have no effect - use the macro chains directly.

  Custom equivalent:
    scriptnode SoundGenerator with global cables to macro slots.

  See also:
    alternative MacroModulator - reads macro slot values as modulation
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Macro Modulation Source screenshot](/images/v2/reference/audio-modules/macromodulationsource.png)

The Macro Modulation Source hosts modulation chains that drive the global macro control slots. Each of its Macro chains (1 through 8 by default) feeds its output value into the corresponding macro control slot, allowing LFOs, envelopes, and other modulators to automate macro knobs from within the module tree.

This module does not generate audio. It is classified as a SoundGenerator to participate in the rendering pipeline and host its modulation chains, but its voices produce no output. The only user-facing features are the Macro modulation chain slots. The number of macro chains defaults to 8 but is configurable per-project (up to a maximum of 64).

## Signal Path

::signal-path
---
glossary:
  functions:
    renderChains:
      desc: "Renders all macro modulation chains as part of the normal synth rendering pipeline"
    readFirstSample:
      desc: "Reads the first sample of each chain's output buffer to get the current value"
    setMacroControl:
      desc: "Sends the scaled value (0-127) to the corresponding global macro control slot"
---

```
// Macro Modulation Source - monophonic, no audio
// drives macro control slots from modulation chains

// Per-block processing (in preVoiceRendering)
renderChains()

for each macroChain (1 to 8):
    if chain is empty: skip

    value = readFirstSample(macroChain)

    if value != previousValue:
        setMacroControl(slotIndex, value * 127)
        previousValue = value
```

::

## Parameters

::parameter-table
---
groups:
  - label: Output (Inherited)
    params:
      - { name: Gain, desc: "Inherited from the base class. Not applied in processing. Has no effect on macro output values.", range: "0 - 100%", default: "25%" }
      - { name: Balance, desc: "Inherited from the base class. Not applied in processing.", range: "-1 - 1", default: "0" }
  - label: Voice Management (Inherited)
    params:
      - { name: VoiceLimit, desc: "Inherited from the base class. The module has no functional voices. This parameter has no effect.", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Inherited from the base class. Not applicable since the module has no voice lifecycle.", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - name: "Macro 1"
    desc: "Drives macro control slot 1. Output value (0-1) is scaled to 0-127 and sent to the macro system per block."
    scope: "monophonic"
    constrainer: "Any"
    hints:
      - type: info
        text: "Each chain is sampled once per audio block (first sample position). For per-sample resolution on the receiving end, use a $MODULES.MacroModulator$ with smoothing enabled. [1]($FORUM_REF.4333$)"
  - { name: "Macro 2", desc: "Drives macro control slot 2.", scope: "monophonic", constrainer: "Any" }
  - { name: "Macro 3", desc: "Drives macro control slot 3.", scope: "monophonic", constrainer: "Any" }
  - { name: "Macro 4", desc: "Drives macro control slot 4.", scope: "monophonic", constrainer: "Any" }
  - { name: "Macro 5", desc: "Drives macro control slot 5.", scope: "monophonic", constrainer: "Any" }
  - { name: "Macro 6", desc: "Drives macro control slot 6.", scope: "monophonic", constrainer: "Any" }
  - { name: "Macro 7", desc: "Drives macro control slot 7.", scope: "monophonic", constrainer: "Any" }
  - { name: "Macro 8", desc: "Drives macro control slot 8.", scope: "monophonic", constrainer: "Any" }
  - { name: "Gain Modulation", desc: "Inherited from the base class. Disabled. Not applicable - this module produces no audio.", scope: "monophonic", constrainer: "Any" }
  - { name: "Pitch Modulation", desc: "Inherited from the base class. Disabled. Not applicable - this module has no oscillator.", scope: "monophonic", constrainer: "Any" }
---
::

**See also:** $MODULES.MacroModulator$ -- the receive side of the macro system, reads macro slot values and converts them to modulation signals with smoothing and table-based response curves