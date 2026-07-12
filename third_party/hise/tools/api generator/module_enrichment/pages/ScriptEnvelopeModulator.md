---
title: Script Envelope Modulator
moduleId: ScriptEnvelopeModulator
type: Modulator
subtype: EnvelopeModulator
tags: [custom]
builderPath: b.Modulators.ScriptEnvelopeModulator
screenshot: /images/v2/reference/audio-modules/scriptenvelopemodulator.png
cpuProfile:
  baseline: "(depends on loaded network)"
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: HardcodedEnvelopeModulator, type: alternative, reason: "Compiled variant for exported plugins" }
  - { id: AHDSR, type: alternative, reason: "Built-in five-stage envelope when a custom shape is not needed" }
  - { id: ScriptnodeVoiceKiller, type: companion, reason: "Dedicated voice killer module - an alternative to using this envelope as the voice killer" }
  - { id: ScriptSynth, type: companion, reason: "Scriptnode synthesiser that this envelope typically modulates" }
commonMistakes:
  - title: "Network does not signal voice death"
    wrong: "Building an envelope network that never closes the gate signal"
    right: "Ensure the network produces a gate-close signal after the release phase completes"
    explanation: "The Script Envelope Modulator acts as the voice killer. If the network never signals that the envelope is finished, voices accumulate indefinitely."
  - title: "Expecting audio-rate processing"
    wrong: "Designing a network that relies on audio-rate sample accuracy"
    right: "Design the network for control-rate operation - it runs at a downsampled rate"
    explanation: "The envelope processes at control rate (audio rate divided by the event raster), not full audio rate. Per-sample precision is not available."
llmRef: |
  Script Envelope Modulator (Modulator/EnvelopeModulator)

  Per-voice envelope modulator that generates a modulation signal from a scriptnode DSP network. Runs at control rate. Acts as the voice killer - the network must signal voice death via the gate mechanism.

  Signal flow:
    noteOn -> init voice in network -> network process (control rate, 1 channel) -> modulation out (0-1, per voice)
    noteOff -> mark ringing off -> network continues until gate closes -> voice killed

  CPU: depends on loaded network, polyphonic
    Runs at control rate, significantly cheaper than audio-rate processing.

  Parameters:
    Voice Mode:
      Monophonic (Off/On, default dynamic) - shares one envelope across all voices
      Retrigger (Off/On, default On) - restarts envelope on new notes in monophonic mode
    Network parameters are appended dynamically starting at index 2 (offset = 2).

  Modulation chains:
    None. This module IS a modulator - it does not have parent Gain/Pitch chains or extra modulation slots.

  Channel configuration:
    Not applicable - produces a single-channel modulation signal, not audio output.

  Complex data types and parameter exposure:
    Network parameters start at index 2 (after Monophonic, Retrigger). See Audio Modules index Custom section for complex data types, parameter exposure, and configuration table.

  When to use:
    Use when you need a custom envelope shape that cannot be achieved with AHDSR or TableEnvelope.

  Common mistakes:
    Network must close the gate signal or voices accumulate.
    Runs at control rate, not audio rate.

  See also:
    alternative HardcodedEnvelopeModulator - compiled variant
    alternative AHDSR - built-in five-stage envelope
    companion ScriptnodeVoiceKiller - alternative voice killer
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules with user-defined signal paths via scriptnode networks or HISEScript callbacks" }
---
::

![Script Envelope Modulator screenshot](/images/v2/reference/audio-modules/scriptenvelopemodulator.png)

The Script Envelope Modulator generates a per-voice envelope signal from a scriptnode DSP network. It operates at control rate (downsampled from the audio rate) and produces a 0-1 modulation signal for each voice. This module is the go-to choice when you need an envelope shape that goes beyond what the built-in [AHDSR]($MODULES.AHDSR$) or [TableEnvelope]($MODULES.TableEnvelope$) can provide.

The module acts as the voice killer for its parent sound generator. The network must close a gate signal when the envelope finishes to terminate voices. Without this, voices will accumulate indefinitely.

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
    initVoice:
      desc: "Initialises the per-voice state and network voice for the new note"
    networkProcess:
      desc: "Processes the modulation signal through the scriptnode network at control rate"
    voiceKill:
      desc: "Terminates the voice when the network's gate signal closes"
---

```
// Script Envelope Modulator - per-voice scriptnode envelope
// noteOn/noteOff in -> modulation out (0-1, per voice)

onNoteOn() {
    initVoice()    // reset state, start network voice
    // Network produces modulation signal at control rate
}

perVoice() {
    networkProcess(modulationBuffer)
    // Output: 0-1 modulation signal
}

onNoteOff() {
    // Voice marked as ringing off
    // Network continues processing (release phase)
    // When gate closes -> voiceKill()
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
        hints:
          - type: warning
            text: "Monophonic mode may affect voice killing behaviour. Test voice management carefully when enabling this."
      - { name: Retrigger, desc: "Restarts the envelope from its current value when a new note arrives in monophonic mode. Has no effect in polyphonic mode", range: "Off / On", default: "On" }
---
::

### Loading a Network

Create a scriptnode network in the `onInit` callback by calling `Engine.createDspNetwork("NetworkName")`. This creates a new network or loads an existing one from the `DspNetworks/Networks/` folder, where networks are stored as `.xml` files.

The network must be designed as a polyphonic modulation source that produces a single-channel output between 0 and 1. It operates at **control rate** (the audio sample rate divided by the event raster), not at the full audio rate. Design the network accordingly — per-sample precision is not available, and audio-rate nodes will not behave as expected.

The network must implement proper gate handling: after receiving a note-off event, it should continue processing its release phase and then close the gate to signal voice death. Without this, voices accumulate indefinitely.

Switching networks at runtime is possible by calling `Engine.createDspNetwork()` again with a different name. However, this is a heavyweight operation that reinitialises the entire DSP graph and should not be done during audio playback.

### Modulation Chain Configuration

Not applicable. This module **is** a modulator — it does not have parent Gain or Pitch chains, nor extra modulation chain slots. All modulation within the network must be handled internally using `container.modchain`, modulation cables, and `control.*` nodes.

### Parameter Exposure and Complex Data

Network parameters are appended starting at index 2, after the fixed parameters (Monophonic, Retrigger). See [Custom module hosting](/v2/reference/audio-modules/#custom) for parameter exposure, complex data types, and the configuration table.

### Voice Management

This module registers itself as the voice killer for its parent sound generator. The voice lifecycle is:

1. **Note on**: Per-voice state is initialised (playing = true, ringing off = false). The network voice is started.
2. **Processing**: The network produces a modulation signal at control rate.
3. **Note off**: The voice is marked as ringing off. The network continues processing its release phase.
4. **Gate close**: When the network signals that the envelope is finished, the voice is marked as no longer playing. The parent synth then terminates it.

If the network never closes the gate, the voice remains alive indefinitely.

**See also:** $MODULES.HardcodedEnvelopeModulator$ -- compiled variant for exported plugins, $MODULES.AHDSR$ -- built-in five-stage envelope when a custom shape is not needed
