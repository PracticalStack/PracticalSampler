---
title: Hardcoded Polyphonic FX
moduleId: HardcodedPolyphonicFX
type: Effect
subtype: VoiceEffect
tags: [custom]
builderPath: b.Effects.HardcodedPolyphonicFX
screenshot: /images/v2/reference/audio-modules/hardcodedpolyphonicfx.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - { parameter: "Loaded network", impact: "variable", note: "CPU cost depends on the compiled network, scales with active voice count" }
seeAlso:
  - { id: PolyScriptFX, type: alternative, reason: "Loads interpreted XML scriptnode networks instead of compiled C++ code for per-voice effects" }
  - { id: HardcodedMasterFX, type: alternative, reason: "Compiled monophonic master effect when per-voice processing is not needed" }
  - { id: HardcodedSynth, type: companion, reason: "Compiled synthesiser whose voices can be processed by this compiled per-voice effect" }
  - { id: "LANG.cpp-dsp-nodes", type: guide, reason: "Complete callback interface and worked examples for writing custom C++ DSP nodes" }
commonMistakes:
  - title: "Missing compiled DLL"
    wrong: "Adding a Hardcoded Polyphonic FX without first compiling the scriptnode networks"
    right: "Use Export > Compile DSP Networks as DLL before loading a network in the module"
    explanation: "The network selector will be empty until the project's scriptnode networks are compiled."
  - title: "Channel count mismatch"
    wrong: "Loading a network whose channel count does not match the routing matrix"
    right: "Ensure the routing matrix channel count matches the compiled network's channel count"
    explanation: "A channel mismatch disables processing and shows an error."
  - title: "Using reserved parameter names"
    wrong: "Naming a network parameter 'Bypassed', 'Type', 'ID', or 'Network'"
    right: "Choose parameter names that do not collide with HISE's reserved property names"
    explanation: "Reserved names cause a conflict with internal module properties."
  - title: "DLL API version mismatch after HISE update"
    wrong: "Updating HISE and expecting existing compiled DLLs to continue working"
    right: "Recompile your DSP networks after every HISE update"
    explanation: "The internal API between HISE and the compiled DLL can change between HISE versions. A stale DLL will fail to load or cause undefined behaviour."
llmRef: |
  Hardcoded Polyphonic FX (Effect/VoiceEffect)

  Runs a compiled C++ scriptnode network or custom C++ DSP node as a per-voice effect processor. Each voice has independent state. No fixed parameters — all come from the loaded network.

  Signal flow:
    audio in (per voice) -> [compiled network per voice] -> audio out (per voice)

  Modulation: extra slots only (NUM_HARDCODED_POLY_FX_MODS, default 0), no built-in chains. See modulators parent page.
  Channels: fixed at compile time, must match routing matrix. See sound-generators parent page.
  Parameters: all from network, offset 0. See index parent page.
  Complex data: slot counts baked at compile time. See index parent page.
  Custom C++ nodes: see cpp-dsp-nodes language guide.
  Voice management: each voice gets independent state; supports voice suspension for tails.

  CPU: negligible framework overhead, actual cost depends on loaded network, polyphonic (scales with voices).

  Common mistakes:
    Must compile DSP networks before loading.
    Channel count between routing matrix and network must match.
    Avoid reserved parameter names.
    Must recompile DLL after HISE updates.

  See also:
    alternative PolyScriptFX - interpreted XML scriptnode per-voice effect
    alternative HardcodedMasterFX - compiled monophonic effect
    companion HardcodedSynth - compiled synthesiser for per-voice processing
    guide cpp-dsp-nodes - C++ DSP node callback interface and examples
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Hardcoded Polyphonic FX screenshot](/images/v2/reference/audio-modules/hardcodedpolyphonicfx.png)

The Hardcoded Polyphonic FX runs compiled C++ code as a per-voice effect. Each active voice has its own independent state in the compiled network, allowing effects like per-voice filtering, distortion, or waveshaping. It can load compiled scriptnode networks (visual graphs exported to C++) or custom C++ DSP nodes (hand-written algorithms), both delivered through the same DLL compilation pipeline. Unlike $MODULES.PolyScriptFX$ which interprets an XML scriptnode network, this module loads pre-compiled code for better performance — particularly important for per-voice effects that scale with the number of active voices.

The module has no fixed parameters. When a compiled network or custom node is loaded, its parameters, tables, slider packs, and audio files appear automatically. Modulation chains can drive network parameters, and MIDI events are forwarded to the network for voice lifecycle management.

## Signal Path

::signal-path
---
glossary:
  functions:
    selectNetwork:
      desc: "Choose which compiled C++ scriptnode network to load as the per-voice effect"
    processEffect:
      desc: "Run the compiled network on each voice's audio independently"
  modulations:
    ExtraModChains:
      desc: "Additional modulation chains driving compiled network parameters at control rate (downsampled by factor of 8)"
      scope: "per-voice"
---

```
// Hardcoded Polyphonic FX - compiled per-voice effect
// audio in (per voice) -> audio out (per voice)

network = selectNetwork("NetworkName")

onVoiceStart() {
    // Initialise per-voice state in the compiled network
}

process(voiceBuffer) {
    // Extra mod chains modulate network parameters per chunk
    networkParams *= ExtraModChains
    processEffect(voiceBuffer)  // independent state per voice
}
```

::

### What Can Be Loaded

The Hardcoded Polyphonic FX can load two kinds of content, both delivered through the same DLL compilation pipeline:

**Compiled scriptnode networks** are visual graphs designed in the scriptnode editor, then exported to C++. This is the standard workflow: design and iterate visually, then compile for production performance. The compilation step collapses the entire node graph into a single optimised function, eliminating per-node overhead.

**Custom C++ DSP nodes** are hand-written algorithms placed in `DspNetworks/ThirdParty/*.h` files, following the scriptnode node callback interface. This is the path for per-voice effect algorithms with no stock equivalent — custom per-voice filters, waveshapers, or spatial effects. See [Custom C++ Nodes](#custom-c-nodes) below.

Both types use the same internal dispatch mechanism and appear in the same module dropdown.

### Compiled vs Interpreted

The Hardcoded Polyphonic FX and $MODULES.PolyScriptFX$ both host scriptnode networks for per-voice effect processing. PolyScriptFX interprets the network from XML. The Hardcoded version loads compiled C++ code, eliminating per-node overhead. The performance benefit is especially significant for per-voice effects because the overhead is multiplied by the number of active voices.

### Loading a Compiled Network

Compile your scriptnode networks or custom C++ nodes using **Export > Compile DSP Networks as DLL**. Then select the network from the module's dropdown — network parameters, tables, slider packs, and audio files appear automatically based on what the compiled code declares. The network's channel count must match the routing matrix configuration.

Custom C++ nodes from the `DspNetworks/ThirdParty/` folder appear in the same dropdown alongside compiled scriptnode networks. Both load identically from the module's perspective.

During development, the DLL hot-loads when recompiled — restart HISE or recompile to pick up changes. In exported plugins, compiled networks and custom nodes are built directly into the binary (no DLL needed).

### Modulation Chain Configuration

This module has no built-in Gain or Pitch modulation chains. Extra modulation slots are configured via `NUM_HARDCODED_POLY_FX_MODS` (default: 0). Modulation runs per voice. See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for how extra modulation slots connect to network parameters.

### Channel Configuration

This module supports the standard scriptnode channel configuration. Channel count is fixed at compile time and must match the routing matrix — a mismatch disables processing and shows an error. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels).

### Parameter Exposure and Complex Data

All parameters come from the compiled network, starting at index 0. Parameter names must not collide with reserved names (Bypassed, Type, ID, Network). For custom C++ nodes, parameters are registered via the `createParameters()` callback. Complex data slot counts are baked into the compiled code at compile time. See [Custom module hosting](/v2/reference/audio-modules/#custom) for the full parameter and complex data reference.

### Voice Management

Each voice gets independent state in the compiled network. When a voice starts, per-voice state is initialised. When a voice is reset, the per-voice state is cleared. The network can handle voice suspension — allowing the effect to continue processing after the sound generator has stopped a voice. This is useful for effects with reverb tails or delay feedback that need to ring out beyond the voice's note-off.

### Custom C++ Nodes

This module can load custom C++ DSP nodes from `DspNetworks/ThirdParty/*.h` in addition to compiled scriptnode networks. See the [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) guide for the complete callback interface, workflow, and worked examples.

### Export Workflow

There are two workflows depending on what you are loading:

**Compiled scriptnode network**: Design your per-voice effect network in the scriptnode editor. When ready for production, use **Export > Compile DSP Networks as DLL** to compile it. Add a Hardcoded Polyphonic FX to a sound generator's FX chain, select the network from the dropdown, and configure the routing matrix. On plugin export, the network is baked directly into the binary.

**Custom C++ node**: Write your `.h` file in `DspNetworks/ThirdParty/`. Compile the DLL — the node auto-loads in HISE and appears in the module dropdown. On plugin export, the C++ code is compiled directly into the binary alongside the rest of the plugin.

In both cases, the exported plugin contains no DLL and no XML — everything runs as native compiled code.

**See also:** $MODULES.PolyScriptFX$ -- loads interpreted XML scriptnode networks for per-voice effects, $MODULES.HardcodedMasterFX$ -- compiled monophonic master effect, $MODULES.HardcodedSynth$ -- compiled synthesiser whose voices can be processed by this effect, [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) -- complete callback interface and worked examples for writing custom C++ DSP nodes
