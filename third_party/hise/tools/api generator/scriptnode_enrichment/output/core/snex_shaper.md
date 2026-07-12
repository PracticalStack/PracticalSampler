---
title: SNEX Shaper
description: "A custom waveshaper node using SNEX code to define the transfer function."
factoryPath: core.snex_shaper
factory: core
polyphonic: false
tags: [core, snex, waveshaping, saturation]
cpuProfile:
  baseline: variable
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "core.snex_node", type: alternative, reason: "Full SNEX callback set for general-purpose processing" }
  - { id: "math.tanh", type: alternative, reason: "Built-in tanh waveshaper without custom code" }
commonMistakes:
  - title: "Expecting MIDI event access"
    wrong: "Writing a handleHiseEvent callback in the SNEX shaper code"
    right: "Use core.snex_node if MIDI event handling is required."
    explanation: "core.snex_shaper does not forward MIDI events. It is designed purely for sample-level waveshaping. Use core.snex_node for DSP that needs MIDI input."
llmRef: |
  core.snex_shaper

  A waveshaper node where the transfer function is defined by user-written SNEX code. The node displays the shaping curve by passing test samples through the user code.

  Signal flow:
    audio in -> SNEX shaper code -> audio out

  CPU: variable (depends on user SNEX code), monophonic

  Parameters:
    All parameters are user-defined in the SNEX code.

  When to use:
    - Custom waveshaping or saturation curves
    - Transfer functions not available from built-in math nodes
    - Visualising the shaping curve in the node display

  See also:
    [alternative] core.snex_node -- full SNEX callback set
    [alternative] math.tanh -- built-in tanh waveshaper
---

The SNEX shaper wraps a user-written [SNEX]($LANG.snex$) waveshaping function and applies it to the incoming audio signal. The node displays a visual representation of the transfer curve by passing test values through the shaper code, making it straightforward to verify the shape.

The SNEX code can implement either a simple `getSample` function that processes one sample at a time, or provide full `process` and `processFrame` callbacks for more complex shaping that requires state. Parameters defined in the SNEX code are automatically exposed on the node. Unlike [core.snex_node]($SN.core.snex_node$), this node does not receive MIDI events.

## Signal Path

::signal-path
---
glossary:
  functions:
    shaperFunction:
      desc: "User-defined SNEX waveshaping function applied to each sample"
---

```
// core.snex_shaper - custom waveshaping via SNEX
// audio in -> audio out

process(input) {
    for each sample in input {
        sample = shaperFunction(sample)
    }
}
```

::

## Parameters

All parameters are defined dynamically by the user's SNEX code. The node itself has no built-in parameters.

### Setup

The `ClassId` property selects which SNEX shaper class to compile. The SNEX code can optionally implement `setExternalData` to access tables or audio files for lookup-based shaping.

### Compilation

When compiled to C++, the shaper class receives the voice count as a template argument, allowing per-voice state if needed in a polyphonic context. However, within the scriptnode interpreter the node itself operates monophonically.

**See also:** $SN.core.snex_node$ -- full SNEX callback set for general-purpose processing, $SN.math.tanh$ -- built-in tanh waveshaper without custom code
