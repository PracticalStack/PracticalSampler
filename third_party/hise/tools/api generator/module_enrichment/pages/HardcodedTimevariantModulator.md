---
title: Hardcoded Time Variant Modulator
moduleId: HardcodedTimevariantModulator
type: Modulator
subtype: TimeVariantModulator
tags: [custom]
builderPath: b.Modulators.HardcodedTimevariantModulator
screenshot: /images/v2/reference/audio-modules/hardcodedtimevariantmodulator.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "Loaded network", impact: "variable", note: "CPU cost depends on the compiled network. Runs monophonically at control rate." }
seeAlso:
  - { id: ScriptTimeVariantModulator, type: alternative, reason: "Loads interpreted XML scriptnode networks or HiseScript callbacks instead of compiled C++ code" }
  - { id: LFO, type: alternative, reason: "Built-in LFO when a standard waveform modulator is sufficient" }
  - { id: HardcodedMasterFX, type: companion, reason: "Compiled master effect that can be modulated by this compiled modulator" }
  - { id: "LANG.cpp-dsp-nodes", type: guide, reason: "Complete callback interface and worked examples for writing custom C++ DSP nodes" }
commonMistakes:
  - title: "Missing compiled DLL"
    wrong: "Adding a Hardcoded Time Variant Modulator without first compiling the scriptnode networks"
    right: "Use Export > Compile DSP Networks as DLL before loading a network in the module"
    explanation: "The network selector will be empty until the project's scriptnode networks are compiled."
  - title: "Using reserved parameter names"
    wrong: "Naming a network parameter 'Intensity'"
    right: "Choose a parameter name that does not collide with the base modulator's Intensity parameter"
    explanation: "Intensity is reserved for the base time-variant modulator class."
  - title: "DLL API version mismatch after HISE update"
    wrong: "Updating HISE and expecting existing compiled DLLs to continue working"
    right: "Recompile your DSP networks after every HISE update"
    explanation: "The internal API between HISE and the compiled DLL can change between HISE versions. A stale DLL will fail to load or cause undefined behaviour."
llmRef: |
  Hardcoded Time Variant Modulator (Modulator/TimeVariantModulator)

  Runs a compiled C++ scriptnode network or custom C++ DSP node as a monophonic time-variant modulator at control rate. Produces a continuous modulation signal (0-1). MIDI events forwarded to network.

  Signal flow:
    [continuous control-rate processing] -> modulation out (0-1, monophonic)

  Parameters: all from network, offset 0. See index parent page.
  Complex data: slot counts baked at compile time. See index parent page.
  Custom C++ nodes: see cpp-dsp-nodes language guide.

  CPU: negligible framework overhead, runs monophonically at control rate.

  Common mistakes:
    Must compile DSP networks before loading.
    Cannot use Intensity as a network parameter name.
    Must recompile DLL after HISE updates.

  See also:
    alternative ScriptTimeVariantModulator - interpreted XML or HiseScript modulator
    alternative LFO - built-in LFO for standard modulation
    companion HardcodedMasterFX - compiled effect that can receive this modulation
    guide cpp-dsp-nodes - C++ DSP node callback interface and examples
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Hardcoded Time Variant Modulator screenshot](/images/v2/reference/audio-modules/hardcodedtimevariantmodulator.png)

The Hardcoded Time Variant Modulator runs compiled C++ code as a monophonic modulation source. It produces a continuous modulation signal at control rate, suitable for driving parameters on effects, sound generators, or other modulators. It can load compiled scriptnode networks (visual graphs exported to C++) or custom C++ DSP nodes (hand-written algorithms), both delivered through the same DLL compilation pipeline. Unlike $MODULES.ScriptTimeVariantModulator$ which interprets an XML network or runs HiseScript callbacks, this module loads pre-compiled code for better performance.

The module has no fixed parameters. When a compiled network or custom node is loaded, all of its parameters appear in the module's interface. MIDI events are forwarded to the compiled network, allowing it to respond to note activity, control changes, or other MIDI input if the network is designed to use them. This module produces no audio output — it generates monophonic modulation values only.

## Signal Path

::signal-path
---
glossary:
  functions:
    selectNetwork:
      desc: "Choose which compiled C++ scriptnode network to load as the modulator"
    processModulation:
      desc: "Run the compiled network to generate modulation values at control rate"
---

```
// Hardcoded Time Variant Modulator - compiled monophonic modulator
// -> modulation out (control rate)

network = selectNetwork("NetworkName")

// Continuous processing at control rate
processModulation(modulationBuffer)
// Network fills the buffer with modulation values each block
```

::

### What Can Be Loaded

The Hardcoded Time Variant Modulator can load two kinds of content, both delivered through the same DLL compilation pipeline:

**Compiled scriptnode networks** are visual graphs designed in the scriptnode editor, then exported to C++. This is the standard workflow: design and iterate visually, then compile for production performance. The compilation step collapses the entire node graph into a single optimised function, eliminating per-node overhead.

**Custom C++ DSP nodes** are hand-written algorithms placed in `DspNetworks/ThirdParty/*.h` files, following the scriptnode node callback interface. This is the path for modulation algorithms with no stock equivalent — custom waveform generators, complex MIDI-reactive modulators, or unconventional modulation curves. See [Custom C++ Nodes](#custom-c-nodes) below.

Both types use the same internal dispatch mechanism and appear in the same module dropdown.

### Compiled vs Interpreted

The Hardcoded Time Variant Modulator and $MODULES.ScriptTimeVariantModulator$ both host custom modulation logic. ScriptTimeVariantModulator can run an XML scriptnode network or HiseScript callbacks. The Hardcoded version loads compiled C++ code for lower overhead. Since time-variant modulators run monophonically, the performance difference is less dramatic than with polyphonic modules, but still beneficial for complex modulation algorithms.

### Loading a Compiled Network

Compile your scriptnode networks or custom C++ nodes using **Export > Compile DSP Networks as DLL**. Then select the network from the module's dropdown — network parameters and complex data objects appear automatically based on what the compiled code declares. The network must output a single channel of modulation values.

Custom C++ nodes from the `DspNetworks/ThirdParty/` folder appear in the same dropdown alongside compiled scriptnode networks. Both load identically from the module's perspective.

During development, the DLL hot-loads when recompiled — restart HISE or recompile to pick up changes. In exported plugins, compiled networks and custom nodes are built directly into the binary (no DLL needed).

### Parameter Exposure and Complex Data

All parameters come from the compiled network, starting at index 0. Parameter names must not collide with reserved names (Intensity, Type, Bypassed, ID, Network). For custom C++ nodes, parameters are registered via the `createParameters()` callback. Complex data slot counts are baked into the compiled code at compile time. See [Custom module hosting](/v2/reference/audio-modules/#custom) for the full parameter and complex data reference.

### MIDI Event Forwarding

Unlike some time-variant modulators, this module forwards MIDI events to the compiled network. This allows you to build MIDI-reactive modulation sources — for example, a modulator that responds to note velocity, control changes, or pitch bend.

### Custom C++ Nodes

This module can load custom C++ DSP nodes from `DspNetworks/ThirdParty/*.h` in addition to compiled scriptnode networks. See the [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) guide for the complete callback interface, workflow, and worked examples.

### Export Workflow

There are two workflows depending on what you are loading:

**Compiled scriptnode network**: Design your modulation network in the scriptnode editor. When ready for production, use **Export > Compile DSP Networks as DLL** to compile it. Add a Hardcoded Time Variant Modulator to the appropriate modulation chain, and select the network from the dropdown. On plugin export, the network is baked directly into the binary.

**Custom C++ node**: Write your `.h` file in `DspNetworks/ThirdParty/`. Compile the DLL — the node auto-loads in HISE and appears in the module dropdown. On plugin export, the C++ code is compiled directly into the binary alongside the rest of the plugin.

In both cases, the exported plugin contains no DLL and no XML — everything runs as native compiled code.

**See also:** $MODULES.ScriptTimeVariantModulator$ -- loads interpreted XML scriptnode networks or HiseScript callbacks, $MODULES.LFO$ -- built-in LFO for standard periodic modulation, $MODULES.HardcodedMasterFX$ -- compiled master effect that can be modulated by this module, [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) -- complete callback interface and worked examples for writing custom C++ DSP nodes
