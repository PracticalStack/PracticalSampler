---
title: Faust
description: "A node that compiles and runs Faust DSP code with automatic parameter and modulation output mapping."
factoryPath: core.faust
factory: core
polyphonic: true
tags: [core, faust, custom-dsp]
cpuProfile:
  baseline: variable
  polyphonic: true
  scalingFactors: []
forumReferences:
  - { tid: 6505, reason: "JIT compiler not in exported plugins; polyphony support" }
  - { tid: 11160, reason: "vgroup parameters silently suppressed" }
  - { tid: 7734, reason: "MIDI synthesis freq/gain/gate convention" }
seeAlso:
  - { id: "core.snex_node", type: alternative, reason: "Custom DSP using SNEX instead of Faust" }
commonMistakes:
  - title: "Mismatched channel count causes error"
    wrong: "Writing Faust code with a different number of output channels than the scriptnode context provides"
    right: "Ensure the Faust code's output channel count matches the current scriptnode channel configuration. For a mono process in a stereo network, duplicate it: process = myProcess, myProcess;"
    explanation: "The node validates that the Faust output channel count matches the HISE channel count. A mismatch produces an error. Faust is mono-first, so a single-channel process must be explicitly duplicated with a comma separator for stereo."
  - title: "Faust JIT compiler is not included in exported plugins"
    wrong: "Exporting a plugin that uses a core.faust node without compiling to C++ first"
    right: "Compile the DSP network to a C++ DLL before exporting. The Faust node is replaced by a dummy in the exported binary if not compiled."
    explanation: "The Faust JIT compiler (libfaust) is not part of the plugin binary. An uncompiled Faust node produces silence in the exported plugin. The workflow is: write and test Faust code in HISE, then export the network as a compiled C++ DLL."
  - title: "vgroup/hgroup parameters are not visible in scriptnode"
    wrong: "Wrapping Faust sliders in vgroup or hgroup and expecting them to appear as node parameters"
    right: "Declare all hslider and nentry items at the top level of the Faust program without group wrappers."
    explanation: "HISE does not interpret the Faust group hierarchy. Parameters wrapped in vgroup or hgroup are silently suppressed from the scriptnode parameter list."
  - title: "Polyphony requires a Scriptnode Synthesiser module"
    wrong: "Placing a polyphonic Faust program inside a Script FX and expecting MIDI note response"
    right: "Use a Scriptnode Synthesiser module for polyphonic Faust synthesis. Script FX is for audio effects only."
    explanation: "MIDI polyphony (freq/gain/gate convention) only works when the core.faust node is inside a Scriptnode Synthesiser. A Script FX does not provide the voice management needed for polyphonic operation."
llmRef: |
  core.faust

  Compiles and runs Faust DSP code at runtime. Parameters are created automatically from Faust UI elements (sliders, buttons, nentry). Modulation outputs are created from bargraph elements. Supports polyphony with one DSP instance per voice.

  Signal flow:
    audio in -> Faust compute() -> audio out
    (optional) MIDI in -> Faust UI zones
    (optional) bargraph elements -> modulation outputs

  CPU: variable (depends on Faust code), polyphonic

  Parameters:
    All parameters are defined by the Faust code's UI elements.

  When to use:
    - Porting existing Faust algorithms into HISE
    - Writing DSP in the Faust functional language
    - Algorithms that benefit from Faust's automatic optimisation

  Common mistakes:
    - Faust output channel count must match HISE channel count (duplicate mono process with comma for stereo)
    - Faust JIT compiler not in exported plugins -- must compile to C++ DLL first
    - vgroup/hgroup parameters are silently hidden -- declare sliders at top level
    - Polyphony needs Scriptnode Synthesiser, not Script FX

  Forum references: tid:6505 (JIT not in export, polyphony), tid:11160 (vgroup suppression), tid:7734 (MIDI freq/gain/gate convention)

  See also:
    [alternative] core.snex_node -- custom DSP via SNEX
---

The Faust node compiles and runs [Faust](https://faust.grame.fr/) DSP code within scriptnode. Faust is a functional DSP programming language -- the node handles compilation, parameter discovery, and audio routing automatically. Parameters are created from Faust UI elements (sliders, buttons, numeric entries) and modulation outputs are created from bargraph elements, so the node's interface adapts entirely to the Faust code.

The node supports polyphony, with one independent Faust DSP instance per voice. MIDI events are forwarded to the Faust code if it defines MIDI zones. The `classId` property selects which Faust source file to compile.

## Signal Path

::signal-path
---
glossary:
  functions:
    faustCompute:
      desc: "Runs the compiled Faust DSP algorithm on a block of audio"
---

```
// core.faust - Faust DSP code execution
// audio in -> audio out (+ optional MIDI, mod outputs)

process(input) {
    // Parameters mapped from Faust UI elements
    // (sliders, buttons, nentry)

    output = faustCompute(input)

    // bargraph elements become modulation outputs
}
```

::

## Parameters

All parameters are defined dynamically by the Faust code's UI elements. The node itself has no built-in parameters. When the Faust code is recompiled, parameters are remapped automatically and their values are preserved across recompilation.

### MIDI synthesis

For a Faust program to respond to MIDI note events, it must define parameters named exactly `freq`, `gain`, and `gate` (using `hslider` or `button` primitives). These are the only MIDI convention keywords recognised by the HISE Faust integration. The node must be placed inside a Scriptnode Synthesiser module (not a Script FX) for polyphonic voice management to be active. For additional MIDI control (CC, pitch bend), use a scriptnode MIDI node and modulate a custom Faust parameter from it.

### Export workflow

The Faust JIT compiler is not included in exported plugin binaries. To use a Faust node in a shipping plugin, compile the DSP network to a C++ DLL using the scriptnode toolbar. The compiled version replaces the Faust node at runtime. An uncompiled Faust node produces silence in the exported plugin.

### Limitations

HISE must be built with Faust support enabled for this node to be available. The Faust code can be compiled via either an LLVM backend (faster execution) or an interpreter backend (no LLVM dependency).

The Faust output channel count must match the scriptnode channel configuration. The input channel count may be smaller -- unused input channels are simply not passed to the Faust code. Since Faust is a mono-first language, a mono process in a stereo network must be duplicated with a comma separator: `process = myProcess, myProcess;`.

Parameters wrapped in `vgroup` or `hgroup` constructs are not visible in the scriptnode parameter list. Declare all UI elements at the top level of the Faust program.

**See also:** $SN.core.snex_node$ -- custom DSP using SNEX instead of Faust
