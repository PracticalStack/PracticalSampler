---
title: Macro Modulator
moduleId: MacroModulator
type: Modulator
subtype: TimeVariantModulator
tags: [routing]
builderPath: b.Modulators.MacroModulator
screenshot: /images/v2/reference/audio-modules/macromodulator.png
cpuProfile:
  baseline: very low
  polyphonic: false
  scalingFactors: [smoothing active]
seeAlso:
  - { id: MacroModulationSource, type: alternative, reason: "The source side of the macro system - hosts modulation chains that drive macro slots, which this modulator then reads" }
commonMistakes:
  - title: "Macro slot must be assigned"
    wrong: "Adding a Macro Modulator and expecting it to produce output without assigning a macro slot"
    right: "Set MacroIndex to the desired macro slot (1-8) before using"
    explanation: "The default MacroIndex is -1 (disconnected). When disconnected, the modulator outputs a constant 1.0 and does not respond to any macro changes."
  - title: "Table lookup happens before smoothing"
    wrong: "Expecting the response curve table to update in real-time when the macro value changes continuously"
    right: "The table is applied once when the macro value changes, before smoothing"
    explanation: "The table lookup transforms the incoming macro value into a target value. The smoother then interpolates towards that target. The table is not re-evaluated during the smoothing ramp."
forumReferences:
  - id: 1
    title: "Macro modulation is not sample-accurate and is designed for slow movements only"
    summary: "Christoph Hart confirmed that macro modulation is not sample-accurate and never will be; it moves the target parameter's knob value at block rate, not via the modulation chain."
    topic: 4333
customEquivalent:
  approach: scriptnode
  moduleType: Modulator
  complexity: low
  description: "A global cable receiving macro values with optional table lookup and smoothing node"
llmRef: |
  Macro Modulator (TimeVariantModulator)

  Bridges the macro control system to the modulation system. Listens to a macro control slot and outputs its value as a time-variant modulation signal with optional response curve shaping and smoothing.

  Signal flow:
    Macro slot value (0-1) -> table lookup (optional) -> smoother (per-sample at control rate) -> modulation output

  CPU: very low. Monophonic. Per-sample processing only when smoothing is active; constant-fill when converged.

  Parameters:
    MacroIndex (-1 to 7, default -1) - which macro slot to listen to. -1 = disconnected.
    SmoothTime (0-1000 ms, default 200 ms) - smoothing time for value transitions
    UseTable (Off/On, default Off) - enables a lookup table for custom response curves
    MacroValue (0-1, default 1.0) - internal write-only parameter driven by the macro system

  When to use:
    When you need smoothed macro control with custom response curves. The macro system is designed for slow control movements, not precision modulation. This modulator adds per-sample interpolation and table-based reshaping on top of the block-rate macro protocol.

  Common mistakes:
    Default MacroIndex is -1 (disconnected) - must assign a slot.
    Table lookup happens before smoothing, not after.

  Custom equivalent:
    scriptnode Modulator with global cable and smoothing node.

  See also:
    alternative MacroModulationSource - drives macro slots from modulation chains
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Macro Modulator screenshot](/images/v2/reference/audio-modules/macromodulator.png)

The Macro Modulator listens to a macro control slot and converts its value into a time-variant modulation signal. It provides two features beyond what the standard macro control protocol offers: per-sample smoothing to prevent zipper noise on fast value changes, and an optional lookup table for custom response curves.

Place this modulator in any modulation chain where you want a macro knob to control a parameter with smooth, click-free transitions. The table allows non-linear mappings - for example, an exponential curve for volume fades or a stepped curve for discrete value jumps. The macro control system is designed for slow control movements (pad filter sweeps, randomised jitter), not precision sample-accurate modulation - use dedicated modulators for fast or exact modulation tasks. [2]($FORUM_REF.4333$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    MacroIndex:
      desc: "Which macro slot to listen to (0-7, or -1 for disconnected)"
      range: "-1 - 7"
      default: "-1"
    UseTable:
      desc: "Enables the lookup table for response curve shaping"
      range: "Off / On"
      default: "Off"
    SmoothTime:
      desc: "Smoothing time for value transitions"
      range: "0 - 1000 ms"
      default: "200 ms"
  functions:
    tableLookup:
      desc: "Reads the response curve table at the macro value position (0-1 input, 0-1 output)"
    smooth:
      desc: "Per-sample interpolation towards the target value at the control rate"
---

```
// Macro Modulator - monophonic, time-variant
// no audio I/O, modulation output only

// Value update (event-driven, when macro slot changes)
macroValue = clamp(incomingValue, 0, 1)

if UseTable:
    targetValue = tableLookup(macroValue)
else:
    targetValue = macroValue

// Per-block processing
if targetValue != currentValue:
    // Per-sample smoothing (control rate)
    for each sample:
        currentValue = smooth(targetValue, SmoothTime)
        output[sample] = currentValue
else:
    // Constant fill (CPU optimization)
    fill(output, currentValue)
```

::

## Parameters

::parameter-table
---
groups:
  - label: Macro Connection
    params:
      - name: MacroIndex
        desc: "Which macro control slot to listen to. Values 0-7 correspond to Macro 1-8. A value of -1 means disconnected - the modulator outputs a constant 1.0."
        range: "-1 - 7"
        default: "-1"
        hints:
          - type: info
            text: "After disconnecting from a macro slot, the modulator retains whatever value it had at the moment of disconnection. Only a newly created instance starts at 1.0."
  - label: Value Processing
    params:
      - { name: UseTable, desc: "Enables a lookup table for custom response curves. The table maps the incoming macro value (0-1) to an output value (0-1) before smoothing is applied.", range: "Off / On", default: "Off" }
      - name: SmoothTime
        desc: "Time in milliseconds for the output to reach a new target value. Higher values produce smoother but slower transitions. At 0 ms the output changes instantly."
        range: "0 - 1000 ms"
        default: "200 ms"
        hints:
          - type: tip
            text: "For fast automation, a small smoothing time (10-50 ms) prevents audible artefacts without adding noticeable latency. [1]($FORUM_REF.4333$)"
  - label: Internal
    params:
      - { name: MacroValue, desc: "The current macro value. This is a system-driven parameter used internally by the macro control system. Not user-editable.", range: "0 - 100%", default: "100%" }
---
::

**See also:** $MODULES.MacroModulationSource$ -- the source side of the macro system, hosts modulation chains that drive the macro slots this modulator reads from