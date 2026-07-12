---
title: SNEX Node
description: "A generic SNEX node with the complete callback set for custom audio processing."
factoryPath: core.snex_node
factory: core
polyphonic: false
tags: [core, snex, custom-dsp]
cpuProfile:
  baseline: variable
  polyphonic: false
  scalingFactors: []
forumReferences:
  - { tid: 4364, reason: "SNEX export workflow: wrap into DSP network, compile, freeze" }
seeAlso:
  - { id: "core.snex_shaper", type: alternative, reason: "Simpler SNEX interface for waveshaping only" }
  - { id: "core.snex_osc", type: alternative, reason: "SNEX interface specialised for oscillators with built-in frequency tracking" }
  - { id: "core.faust", type: alternative, reason: "Alternative custom DSP using Faust language instead of SNEX" }
commonMistakes:
  - title: "Expecting polyphonic voice support"
    wrong: "Using core.snex_node for per-voice synthesis"
    right: "Use core.snex_osc for polyphonic oscillators, or compile to a C++ node for full polyphonic support."
    explanation: "core.snex_node is monophonic. It does not support per-voice state. For polyphonic use cases, core.snex_osc provides voice management for oscillators."
  - title: "SNEX JIT compiler is not included in exported plugins"
    wrong: "Exporting a plugin that uses core.snex_node (or cable_expr, math.expr) without compiling to a C++ DLL first"
    right: "Wrap the SNEX node into a sub-network using the 'Wrap into DSP Network' toolbar action, then compile it with 'Compile DSP Networks'. The frozen (snowflake) icon confirms the compiled version is active."
    explanation: "The SNEX JIT compiler is not part of the exported plugin binary. Any node that uses SNEX (including cable_expr and math.expr) must be compiled to a C++ DLL before export, or it will be replaced by a silent dummy in the shipped plugin."
llmRef: |
  core.snex_node

  A generic SNEX node that delegates all audio processing to user-written SNEX code compiled at runtime. Supports the full callback set: prepare, reset, process, processFrame, and handleHiseEvent. Parameters and complex data slots are defined dynamically by the SNEX code.

  Signal flow:
    audio in + MIDI in -> SNEX process callbacks -> audio out
    (optional) -> modulation out (normalised 0-1)

  CPU: variable (depends on user SNEX code), monophonic

  Parameters:
    All parameters are user-defined in the SNEX code (up to 16).

  When to use:
    - Custom audio processing that cannot be built from existing nodes
    - Prototyping DSP algorithms with JIT compilation
    - Monophonic effects or utilities requiring MIDI access

  Common mistakes:
    - Monophonic only -- use core.snex_osc for polyphonic oscillators
    - SNEX JIT not in exported plugins -- must wrap and compile to C++ DLL first

  Forum references: tid:4364 (export workflow: wrap, compile, freeze)

  See also:
    [alternative] core.snex_shaper -- waveshaping-only SNEX interface
    [alternative] core.snex_osc -- oscillator SNEX interface with frequency tracking
    [alternative] core.faust -- custom DSP via Faust language
---

The SNEX node is a general-purpose container for custom DSP written in [SNEX]($LANG.snex$). It provides the complete set of processing callbacks, giving full control over audio processing, MIDI event handling, and optionally modulation output. The SNEX code is compiled at runtime, so changes take effect immediately without restarting.

Unlike the more specialised [core.snex_shaper]($SN.core.snex_shaper$) and [core.snex_osc]($SN.core.snex_osc$), this node imposes no constraints on what the SNEX code does. It receives audio input, MIDI events, and any user-defined parameters, then delegates everything to the compiled SNEX callbacks. The node is monophonic -- for polyphonic oscillators, use [core.snex_osc]($SN.core.snex_osc$) instead.

## Signal Path

::signal-path
---
glossary:
  functions:
    snexProcess:
      desc: "Delegates to the user's compiled SNEX process callback"
    handleModulation:
      desc: "Optional callback that produces a normalised modulation output (0-1)"
---

```
// core.snex_node - custom DSP via SNEX code
// audio + MIDI in -> audio out (+ optional mod out)

process(input) {
    // All processing defined by user SNEX code
    output = snexProcess(input)

    // If handleModulation() is defined in the SNEX code:
    modOutput = handleModulation()
}
```

::

## Parameters

All parameters are defined dynamically by the user's SNEX code. The node itself has no built-in parameters. Up to 16 parameters can be declared in the SNEX class.

### Setup

The SNEX code must implement five required callbacks: `prepare`, `reset`, `handleHiseEvent`, `process`, and `processFrame`. If any of these is missing, compilation fails. Two additional callbacks are optional: `handleModulation` (enables the node to act as a modulation source) and `setExternalData` (enables access to tables, audio files, and other complex data).

The `ClassId` property selects which SNEX class to compile. Parameters, complex data slots, and modulation output are all discovered from the compiled SNEX code -- nothing is configured on the node itself.

### Export workflow

The SNEX JIT compiler is not included in exported plugin binaries. To use a SNEX node in a shipping plugin:

1. Use the **Wrap into DSP Network** action in the scriptnode toolbar to create an encapsulated sub-network.
2. Run **Compile DSP Networks** to generate the C++ DLL.
3. After reloading, the frozen (snowflake) icon confirms the compiled version is active.

This applies to all SNEX-based nodes, including `cable_expr`, `math.expr`, and parameter expression nodes.

**See also:** $SN.core.snex_shaper$ -- simpler SNEX interface for waveshaping only, $SN.core.snex_osc$ -- SNEX interface specialised for oscillators with built-in frequency tracking, $SN.core.faust$ -- alternative custom DSP using Faust language instead of SNEX
