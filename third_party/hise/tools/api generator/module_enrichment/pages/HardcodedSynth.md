---
title: Hardcoded Synthesiser
moduleId: HardcodedSynth
type: SoundGenerator
subtype: SoundGenerator
tags: [custom]
builderPath: b.SoundGenerators.HardcodedSynth
screenshot: /images/v2/reference/audio-modules/hardcodedsynth.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - { parameter: "Loaded network", impact: "variable", note: "CPU cost depends on the compiled network, scales with voice count" }
    - { parameter: VoiceLimit, impact: "linear", note: "More voices increase CPU proportionally" }
seeAlso:
  - { id: ScriptSynth, type: alternative, reason: "Loads interpreted XML scriptnode networks instead of compiled C++ code" }
  - { id: HardcodedMasterFX, type: alternative, reason: "Compiled monophonic effect for master bus processing" }
  - { id: HardcodedEnvelopeModulator, type: companion, reason: "Compiled envelope modulator for use with compiled synthesisers" }
  - { id: "LANG.cpp-dsp-nodes", type: guide, reason: "Complete callback interface and worked examples for writing custom C++ DSP nodes" }
commonMistakes:
  - title: "Missing compiled DLL"
    wrong: "Adding a Hardcoded Synthesiser without first compiling the scriptnode networks"
    right: "Use Export > Compile DSP Networks as DLL before loading a network in the module"
    explanation: "The network selector will be empty and the module will show 'No DLL loaded' until the project's scriptnode networks are compiled."
  - title: "Using reserved parameter names in the network"
    wrong: "Naming a network parameter 'Gain', 'Balance', 'VoiceLimit', or 'KillFadeTime'"
    right: "Choose parameter names that do not collide with the built-in sound generator parameters"
    explanation: "These names are reserved for the standard output controls. The module will show an error if a network parameter uses one of them."
  - title: "No envelope for voice killing"
    wrong: "Using a Hardcoded Synthesiser without adding an envelope modulator to kill voices"
    right: "Add an envelope modulator (AHDSR, SimpleEnvelope, or HardcodedEnvelopeModulator) to control voice lifetime"
    explanation: "Without an envelope, voices will never stop and CPU will climb as notes accumulate."
  - title: "DLL API version mismatch after HISE update"
    wrong: "Updating HISE and expecting existing compiled DLLs to continue working"
    right: "Recompile your DSP networks after every HISE update"
    explanation: "The internal API between HISE and the compiled DLL can change between HISE versions. A stale DLL will fail to load or cause undefined behaviour."
llmRef: |
  Hardcoded Synthesiser (SoundGenerator/SoundGenerator)

  Runs a compiled C++ scriptnode network or custom C++ DSP node as a polyphonic sound generator. Each voice runs an independent instance of the compiled code. Inherits standard Gain, Balance, VoiceLimit, and KillFadeTime parameters, plus Gain and Pitch modulation chains.

  Signal flow:
    MIDI in -> voice allocation -> [compiled network per voice] -> gain modulation -> FX chain -> audio out

  Modulation: built-in Gain and Pitch chains, plus extra slots (NUM_HARDCODED_SYNTH_MODS, default 2). Connection mode baked at compile time. See modulators parent page.
  Channels: fixed at compile time. See sound-generators parent page.
  Parameters: network params at index 4+. Custom C++ nodes use createParameters(). See index parent page.
  Complex data: slot counts baked at compile time. See index parent page.
  Custom C++ nodes: see cpp-dsp-nodes language guide.

  CPU: negligible framework overhead, actual cost depends on loaded network, polyphonic (scales with voices).

  Parameters:
    Gain (0.0 - 1.0, default 0.25) - output volume as normalised linear gain
    Balance (-1.0 - 1.0, default 0.0) - stereo balance
    VoiceLimit (1 - 256, default 256) - maximum number of simultaneous voices
    KillFadeTime (0 - 20000 ms, default 20 ms) - fade out time when voices are killed
    + all parameters from the loaded compiled network (indices 4+)

  Common mistakes:
    Must compile DSP networks before loading.
    Cannot use Gain/Balance/VoiceLimit/KillFadeTime as network parameter names.
    Must add an envelope for voice killing.
    Must recompile DLL after HISE updates.

  See also:
    alternative ScriptSynth - interpreted XML scriptnode networks
    alternative HardcodedMasterFX - compiled monophonic effect
    companion HardcodedEnvelopeModulator - compiled envelope for voice control
    guide cpp-dsp-nodes - C++ DSP node callback interface and examples
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Hardcoded Synthesiser screenshot](/images/v2/reference/audio-modules/hardcodedsynth.png)

The Hardcoded Synthesiser runs compiled C++ code as a polyphonic sound generator. Each voice runs its own instance of the compiled code with independent per-voice state. It can load compiled scriptnode networks (visual graphs exported to C++) or custom C++ DSP nodes (hand-written algorithms), both delivered through the same DLL compilation pipeline. Unlike $MODULES.ScriptSynth$ which interprets an XML scriptnode network at runtime, this module loads pre-compiled code for better performance.

The module inherits the standard sound generator controls (Gain, Balance, VoiceLimit, KillFadeTime) and modulation chains (Gain Modulation, Pitch Modulation). When a compiled network or custom node is loaded, its parameters appear after the built-in ones. The module also has an FX chain slot for per-voice effects. Like any sound generator, it needs an envelope modulator to control voice lifetime.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Output volume as normalised linear gain"
      range: "0.0 - 1.0"
      default: "0.25"
    Balance:
      desc: "Stereo balance"
      range: "-1.0 - 1.0"
      default: "0.0"
    VoiceLimit:
      desc: "Maximum number of simultaneous voices"
      range: "1 - 256"
      default: "256"
    KillFadeTime:
      desc: "Fade out time when voices are killed by exceeding the voice limit"
      range: "0 - 20000 ms"
      default: "20 ms"
  functions:
    selectNetwork:
      desc: "Choose which compiled network or custom C++ node to load"
    processNetwork:
      desc: "Run the compiled code's per-voice audio generation"
    applyGain:
      desc: "Multiply voice output by the gain modulation value"
    processFXChain:
      desc: "Process the per-voice FX chain"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Modulates the pitch of all voices"
      scope: "per-voice"
    ExtraModChains:
      desc: "Additional modulation chains driving compiled network parameters at control rate (downsampled by factor of 8)"
      scope: "per-voice"
---

```
// Hardcoded Synthesiser - compiled network as polyphonic synth
// MIDI in -> audio L/R out (per voice)

network = selectNetwork("CompiledNetworkName")

onNoteOn() {
    // Allocate voice, initialise per-voice state
}

perVoice() {
    // Extra mod chains modulate network parameters (control rate)
    networkParams *= ExtraModChains

    processNetwork(voiceBuffer)   // compiled C++ per voice

    applyGain(voiceBuffer, Gain * GainModulation)
    processFXChain(voiceBuffer)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Output
    params:
      - name: Gain
        desc: "Output volume as normalised linear gain. Use a SimpleGain in the FX chain for decibel-scaled control"
        range: "0.0 - 1.0"
        default: "0.25"
      - { name: Balance, desc: "Stereo balance. -1.0 is fully left, 1.0 is fully right", range: "-1.0 - 1.0", default: "0.0" }
  - label: Voice Management
    params:
      - { name: VoiceLimit, desc: "Maximum number of simultaneous voices. Excess voices are killed with the fade time below", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Fade out time in milliseconds when voices are killed by exceeding the voice limit or by a voice killer", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: Gain Modulation, desc: "Scales the output volume per voice as a gain multiplier", scope: "per-voice", constrainer: "*" }
  - { name: Pitch Modulation, desc: "Modulates the pitch of all voices. Connected to the compiled network via runtime targets", scope: "per-voice", constrainer: "*" }
  - { name: "Extra1, Extra2, ...", desc: "Additional modulation chains that drive compiled network parameters. The number of slots is set by NUM_HARDCODED_SYNTH_MODS (default 2). Each slot maps to a network parameter via extra_mod nodes", scope: "per-voice", constrainer: "*" }
---
::

### What Can Be Loaded

The Hardcoded Synthesiser can load three kinds of content, all delivered through the same DLL compilation pipeline:

**Compiled scriptnode networks** are visual graphs designed in the scriptnode editor, then exported to C++. This is the standard workflow for most projects: design and iterate visually, then compile for production performance. The compilation step collapses the entire node graph into a single optimised function, eliminating per-node overhead.

**Custom C++ DSP nodes** are hand-written algorithms placed in `DspNetworks/ThirdParty/*.h` files, following the scriptnode node callback interface. This is the path for algorithms with no stock equivalent — granular engines, FDN reverbs, analogue filter models, custom saturation algorithms, and other DSP that goes beyond what the stock node library provides. See [Custom C++ Nodes](#custom-c-nodes) below.

Both types use the same internal dispatch mechanism and appear in the same module dropdown. Note that $MODULES.ScriptSynth$ (interpreted) can also host custom C++ nodes via the project DLL — the difference is that the Hardcoded Synthesiser eliminates the XML interpretation overhead on top.

### Loading a Compiled Network

Compile your scriptnode networks or custom C++ nodes using **Export > Compile DSP Networks as DLL**. Then select the network from the module's dropdown — network parameters, tables, slider packs, and audio files appear automatically based on what the compiled code declares.

Custom C++ nodes from the `DspNetworks/ThirdParty/` folder appear in the same dropdown alongside compiled scriptnode networks. Both load identically from the module's perspective.

During development, the DLL hot-loads when recompiled — restart HISE or recompile to pick up changes. In exported plugins, compiled networks and custom nodes are built directly into the binary (no DLL needed).

### Modulation Chain Configuration

This module uses `NUM_HARDCODED_SYNTH_MODS` extra modulation slots (default: 2), with built-in Gain and Pitch chains. The connection mode for each slot is configured in the scriptnode editor before compilation and baked into the compiled code. See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for how extra modulation slots connect to network parameters.

### Channel Configuration

This module supports the standard scriptnode channel configuration. Channel count is fixed at compile time. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels).

### Parameter Exposure and Complex Data

Network parameters are appended starting at index 4, after the fixed parameters (Gain, Balance, VoiceLimit, KillFadeTime). For custom C++ nodes, parameters are registered via the `createParameters()` callback. Complex data slot counts are baked into the compiled code at compile time. See [Custom module hosting](/v2/reference/audio-modules/#custom) for the full parameter and complex data reference.

### Custom C++ Nodes

This module can load custom C++ DSP nodes from `DspNetworks/ThirdParty/*.h` in addition to compiled scriptnode networks. See the [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) guide for the complete callback interface, workflow, and worked examples.

### Export Workflow

There are two workflows depending on what you are loading:

**Compiled scriptnode network**: Design your synthesis network in the scriptnode editor. When ready for production, use **Export > Compile DSP Networks as DLL** to compile it. Add a Hardcoded Synthesiser, select the network from the dropdown, add an envelope modulator for voice killing, and add effects to the FX chain as needed. On plugin export, the network is baked directly into the binary.

**Custom C++ node**: Write your `.h` file in `DspNetworks/ThirdParty/`. Compile the DLL — the node auto-loads in HISE and appears in the module dropdown. On plugin export, the C++ code is compiled directly into the binary alongside the rest of the plugin.

In both cases, the exported plugin contains no DLL and no XML — everything runs as native compiled code.

**See also:** $MODULES.ScriptSynth$ -- loads interpreted XML scriptnode networks, $MODULES.HardcodedMasterFX$ -- compiled monophonic master effect, $MODULES.HardcodedEnvelopeModulator$ -- compiled envelope modulator for use with compiled synthesisers, [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) -- complete callback interface and worked examples for writing custom C++ DSP nodes
