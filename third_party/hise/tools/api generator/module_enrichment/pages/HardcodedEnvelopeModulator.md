---
title: Hardcoded Envelope Modulator
moduleId: HardcodedEnvelopeModulator
type: Modulator
subtype: EnvelopeModulator
tags: [custom]
builderPath: b.Modulators.HardcodedEnvelopeModulator
screenshot: /images/v2/reference/audio-modules/hardcodedenvelopemodulator.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - { parameter: "Loaded network", impact: "variable", note: "CPU cost depends on the compiled network, scales with voice count. Runs at control rate." }
seeAlso:
  - { id: ScriptEnvelopeModulator, type: alternative, reason: "Loads interpreted XML scriptnode networks or HiseScript callbacks instead of compiled C++ code" }
  - { id: AHDSR, type: alternative, reason: "Built-in five-stage envelope when a custom shape is not needed" }
  - { id: HardcodedSynth, type: companion, reason: "Compiled synthesiser that benefits from a compiled envelope for consistency" }
  - { id: "LANG.cpp-dsp-nodes", type: guide, reason: "Complete callback interface and worked examples for writing custom C++ DSP nodes" }
forumReferences:
  - id: 3
    title: "Global envelope usage requires Uniform Voice Handler"
    summary: "To use a scriptnode envelope as a global modulator across multiple sound generators, enable the Uniform Voice Handler at root level with Synth.setUseUniformVoiceHandler()."
    topic: 7890
  - id: 4
    title: "Modulation source and target must both be inside midichain for sample accuracy"
    summary: "Placing the target node outside the midichain container loses sample-accurate timing; the modulation value is applied from the last MIDI message to the entire buffer rather than per-sample."
    topic: 7890
commonMistakes:
  - title: "Missing compiled DLL"
    wrong: "Adding a Hardcoded Envelope Modulator without first compiling the scriptnode networks"
    right: "Use Export > Compile DSP Networks as DLL before loading a network in the module"
    explanation: "The network selector will be empty until the project's scriptnode networks are compiled."
  - title: "Network does not signal voice death"
    wrong: "Using a compiled envelope network that never signals the voice to stop"
    right: "Ensure the network uses the voice reset mechanism to signal when the envelope reaches zero"
    explanation: "If the network never signals voice death, voices accumulate indefinitely and CPU climbs."
  - title: "Using reserved parameter names"
    wrong: "Naming a network parameter 'Monophonic', 'Retrigger', or 'Intensity'"
    right: "Choose parameter names that do not collide with the built-in envelope parameters"
    explanation: "These names are reserved for the base envelope controls."
  - title: "DLL API version mismatch after HISE update"
    wrong: "Updating HISE and expecting existing compiled DLLs to continue working"
    right: "Recompile your DSP networks after every HISE update"
    explanation: "The internal API between HISE and the compiled DLL can change between HISE versions. A stale DLL will fail to load or cause undefined behaviour."
  - title: "Network must output exactly one channel"
    wrong: "Compiling the network with a multi-channel output configuration"
    right: "Set the network's channel count to 1 before compiling"
    explanation: "Envelope modulators produce a single modulation signal. A channel count other than 1 causes initialisation failure. [1]($FORUM_REF.13185$)"
  - title: "Cannot modulate container-level parameters"
    wrong: "Placing this envelope in a container's modulation chain to modulate a filter or parameter at container level"
    right: "Place the envelope inside a sound generator's modulation chain"
    explanation: "Containers have no voice concept, so per-voice envelope modulation does not apply there. [2]($FORUM_REF.7890$)"
llmRef: |
  Hardcoded Envelope Modulator (Modulator/EnvelopeModulator)

  Runs a compiled C++ scriptnode network or custom C++ DSP node as a per-voice envelope modulator at control rate. Produces modulation signal (0-1) per voice. Controls voice lifetime — must signal voice death.

  Signal flow:
    noteOn -> initialise per-voice state -> [compiled network at control rate] -> modulation out (0-1, per voice)
    noteOff -> forwarded for release handling -> signals voice death when finished

  Parameters: Monophonic (index 0), Retrigger (index 1), then network params at index 2+. Custom C++ nodes use createParameters(). See index parent page.
  Complex data: slot counts baked at compile time. See index parent page.
  Custom C++ nodes: see cpp-dsp-nodes language guide.

  CPU: negligible framework overhead, runs at control rate (downsampled), polyphonic.

  Common mistakes:
    Must compile DSP networks before loading.
    Network must signal voice death or voices accumulate.
    Network must output exactly one channel.
    Cannot use Monophonic/Retrigger/Intensity as network parameter names.
    Cannot modulate container-level parameters (containers have no voice concept).
    Must recompile DLL after HISE updates.
    For global envelope usage, enable Uniform Voice Handler.

  See also:
    alternative ScriptEnvelopeModulator - interpreted XML or HiseScript envelope
    alternative AHDSR - built-in five-stage envelope
    companion HardcodedSynth - compiled synthesiser
    guide cpp-dsp-nodes - C++ DSP node callback interface and examples
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Hardcoded Envelope Modulator screenshot](/images/v2/reference/audio-modules/hardcodedenvelopemodulator.png)

The Hardcoded Envelope Modulator runs compiled C++ code as a per-voice envelope. It produces a modulation signal between 0 and 1 for each active voice, running at control rate (downsampled) for efficiency. It can load compiled scriptnode networks (visual graphs exported to C++) or custom C++ DSP nodes (hand-written algorithms), both delivered through the same DLL compilation pipeline. Unlike $MODULES.ScriptEnvelopeModulator$ which interprets an XML network or runs HiseScript callbacks, this module loads pre-compiled code for better performance.

The module inherits Monophonic and Retrigger parameters from the envelope base class. When a compiled network or custom node is loaded, its parameters appear after these. The compiled network controls voice lifetime — it must signal when the envelope finishes so that voices can be killed. MIDI note-on and note-off events are forwarded to the network, allowing it to handle attack and release phases. This module produces no audio output — it generates modulation values only.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Monophonic:
      desc: "Shares one envelope across all voices instead of per-voice envelopes"
      range: "Off / On"
      default: "(dynamic)"
    Retrigger:
      desc: "Restarts the envelope on new notes in monophonic mode"
      range: "Off / On"
      default: "On"
  functions:
    selectNetwork:
      desc: "Choose which compiled C++ scriptnode network to load as the envelope"
    processEnvelope:
      desc: "Run the compiled network to generate per-voice envelope values at control rate"
---

```
// Hardcoded Envelope Modulator - compiled scriptnode envelope
// noteOn/noteOff in -> modulation out (0-1, per voice, control rate)

network = selectNetwork("NetworkName")

onNoteOn() {
    // Initialise per-voice state in compiled network
    // Process one frame to get initial envelope value
    initialValue = processEnvelope(voiceIndex)

    // Continue generating envelope values at control rate
    if (Monophonic && Retrigger)
        restart from current value
}

onNoteOff() {
    // Note-off forwarded to compiled network
    // Network handles release phase internally
    // Signals voice death when envelope reaches zero
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Voice Mode
    params:
      - name: Monophonic
        desc: "Shares a single envelope across all voices instead of running one per voice"
        range: "Off / On"
        default: "(dynamic)"
      - { name: Retrigger, desc: "Restarts the envelope from its current value when a new note arrives in monophonic mode. Has no effect in polyphonic mode", range: "Off / On", default: "On" }
---
::

### What Can Be Loaded

The Hardcoded Envelope Modulator can load two kinds of content, both delivered through the same DLL compilation pipeline:

**Compiled scriptnode networks** are visual graphs designed in the scriptnode editor, then exported to C++. This is the standard workflow: design and iterate visually, then compile for production performance. The compilation step collapses the entire node graph into a single optimised function, eliminating per-node overhead.

**Custom C++ DSP nodes** are hand-written algorithms placed in `DspNetworks/ThirdParty/*.h` files, following the scriptnode node callback interface. This is the path for envelope algorithms with no stock equivalent — custom multi-stage shapes, retriggerable patterns, or unconventional modulation curves. See [Custom C++ Nodes](#custom-c-nodes) below.

Both types use the same internal dispatch mechanism and appear in the same module dropdown.

### Compiled vs Interpreted

The Hardcoded Envelope Modulator and $MODULES.ScriptEnvelopeModulator$ both host custom envelope logic. ScriptEnvelopeModulator can run either an XML scriptnode network or HiseScript callbacks. The Hardcoded version loads compiled C++ code, eliminating interpretation overhead. Since envelopes run per voice, the performance benefit multiplies with the voice count.

### Loading a Compiled Network

Compile your scriptnode networks or custom C++ nodes using **Export > Compile DSP Networks as DLL**. Then select the network from the module's dropdown — network parameters and complex data objects appear automatically based on what the compiled code declares. The network must output a single channel of modulation values. On note-on, the network's per-voice state is initialised and one sample is processed immediately to provide the initial envelope value.

Custom C++ nodes from the `DspNetworks/ThirdParty/` folder appear in the same dropdown alongside compiled scriptnode networks. Both load identically from the module's perspective.

During development, the DLL hot-loads when recompiled — restart HISE or recompile to pick up changes. In exported plugins, compiled networks and custom nodes are built directly into the binary (no DLL needed).

### Parameter Exposure and Complex Data

The first two parameters (Monophonic at index 0, Retrigger at index 1) are fixed and inherited from the envelope base class. Network parameters appear after these with indices starting from 2. Network parameter names must not collide with Monophonic, Retrigger, or Intensity. For custom C++ nodes, parameters are registered via the `createParameters()` callback. Complex data slot counts are baked into the compiled code at compile time. See [Custom module hosting](/v2/reference/audio-modules/#custom) for the full parameter and complex data reference.

### Voice Lifetime

The compiled network controls when voices are killed. It must use the voice reset mechanism to signal that the envelope has finished (typically when the release phase reaches zero). If the network never signals voice death, voices will accumulate and CPU usage will increase indefinitely. This is the primary role of an envelope modulator — it acts as the voice killer for its parent sound generator.

### Custom C++ Nodes

This module can load custom C++ DSP nodes from `DspNetworks/ThirdParty/*.h` in addition to compiled scriptnode networks. See the [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) guide for the complete callback interface, workflow, and worked examples.

### Export Workflow

There are two workflows depending on what you are loading:

**Compiled scriptnode network**: Design your envelope network in the scriptnode editor. When ready for production, use **Export > Compile DSP Networks as DLL** to compile it. Add a Hardcoded Envelope Modulator to your sound generator's Gain Modulation chain, and select the network from the dropdown. On plugin export, the network is baked directly into the binary.

**Custom C++ node**: Write your `.h` file in `DspNetworks/ThirdParty/`. Compile the DLL — the node auto-loads in HISE and appears in the module dropdown. On plugin export, the C++ code is compiled directly into the binary alongside the rest of the plugin.

In both cases, the exported plugin contains no DLL and no XML — everything runs as native compiled code.

### Global Envelope Usage

To use a compiled envelope as a global modulator across multiple sound generators, enable the Uniform Voice Handler at the root level with `Synth.setUseUniformVoiceHandler()`. This ensures all sound generators allocate voices synchronously, which is required for envelope-based global modulation. [3]($FORUM_REF.7890$)

When connecting envelope modulation within a scriptnode network, ensure both the modulation source node and the target node are placed inside the `midichain` container. Placing the target outside the `midichain` loses sample-accurate timing — the modulation value is applied from the last incoming MIDI message to the entire buffer rather than per-sample. [4]($FORUM_REF.7890$)

**See also:** $MODULES.ScriptEnvelopeModulator$ -- loads interpreted XML scriptnode networks or HiseScript callbacks, $MODULES.AHDSR$ -- built-in five-stage envelope for standard use cases, $MODULES.HardcodedSynth$ -- compiled synthesiser that pairs well with a compiled envelope, [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) -- complete callback interface and worked examples for writing custom C++ DSP nodes
