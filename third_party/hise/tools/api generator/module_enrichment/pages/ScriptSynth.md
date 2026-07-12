---
title: Scriptnode Synthesiser
moduleId: ScriptSynth
type: SoundGenerator
subtype: SoundGenerator
tags: [custom]
builderPath: b.SoundGenerators.ScriptSynth
screenshot: /images/v2/reference/audio-modules/scriptsynth.png
cpuProfile:
  baseline: "(depends on loaded network)"
  polyphonic: true
  scalingFactors:
    - { parameter: VoiceLimit, impact: "linear", note: "CPU scales with the number of active voices" }
seeAlso:
  - { id: HardcodedSynth, type: alternative, reason: "Compiled variant for exported plugins - loads the same networks without the script editor" }
  - { id: ScriptnodeVoiceKiller, type: companion, reason: "Required in the gain chain for voice management when the network does not handle voice killing internally" }
  - { id: ScriptEnvelopeModulator, type: companion, reason: "Scriptnode envelope for per-voice modulation within the same scriptnode ecosystem" }
commonMistakes:
  - title: "Missing voice killer"
    wrong: "Adding a ScriptSynth without any envelope or voice killer, causing voices to accumulate indefinitely"
    right: "Add a ScriptnodeVoiceKiller to the gain modulation chain, or ensure the network's gate signal terminates voices"
    explanation: "Without a voice killer, every note-on allocates a voice that never stops. This quickly exhausts the voice limit and wastes CPU."
  - title: "Network not compiled before export"
    wrong: "Exporting with an uncompiled scriptnode network"
    right: "Compile the network to a DLL before exporting, or switch to HardcodedSynth"
    explanation: "Uncompiled networks only work in the HISE IDE."
  - title: "Forgetting to set extra mod chain count"
    wrong: "Trying to use more than two extra modulation chain slots without changing the preprocessor definition"
    right: "Add HISE_NUM_SCRIPTNODE_SYNTH_MODS=N to the Extra Definitions field in the project settings before creating the module"
    explanation: "The default number of extra modulation slots is 2. If you need more, you must set the HISE_NUM_SCRIPTNODE_SYNTH_MODS preprocessor macro to the desired count. This must be done before compilation."
llmRef: |
  Scriptnode Synthesiser (SoundGenerator/SoundGenerator)

  Polyphonic sound generator that renders each voice through a scriptnode DSP network. Inherits standard synth parameters (Gain, Balance, VoiceLimit, KillFadeTime) and adds dynamic parameters from the loaded network.

  Signal flow:
    noteOn -> init voice in network -> extra mod chains modulate network params -> network process (per voice) -> gain modulation * Gain -> FX chain -> mix to stereo out

  CPU: depends on loaded network, polyphonic
    Scales linearly with active voices.

  Parameters:
    Output:
      Gain (0.0 - 1.0, default 0.25) - output volume as normalised linear gain
      Balance (-1.0 - 1.0, default 0.0) - stereo balance
      VoiceLimit (1 - 256, default 256) - maximum simultaneous voices
      KillFadeTime (0 - 20000 ms, default 20 ms) - fade-out when voices are killed
    Network parameters are appended dynamically starting at index 4.

  Modulation chains:
    Gain Modulation - scales output volume per voice
    Pitch Modulation - modulates pitch via pitch_mod nodes in the network
    Extra Modulation Chains (default 2, configurable via HISE_NUM_SCRIPTNODE_SYNTH_MODS) - see Audio Modules modulators reference for connection mode details

  Channel configuration:
    Standard scriptnode channel configuration (default stereo, up to 16). See Audio Modules sound-generators reference for details.

  Complex data types and parameter exposure:
    Network parameters start at index 4 (after Gain, Balance, VoiceLimit, KillFadeTime). See Audio Modules index Custom section for complex data types, parameter exposure, and configuration table.

  When to use:
    Use ScriptSynth to build custom synthesisers in scriptnode during development.

  Common mistakes:
    Missing voice killer causes voices to accumulate.
    Must compile network before export.
    Forgetting to set HISE_NUM_SCRIPTNODE_SYNTH_MODS when needing more than 2 extra mod slots.

  See also:
    alternative HardcodedSynth - compiled variant for exported plugins
    companion ScriptnodeVoiceKiller - voice management
    companion ScriptEnvelopeModulator - scriptnode envelope
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules with user-defined signal paths via scriptnode networks or HISEScript callbacks" }
---
::

![Scriptnode Synthesiser screenshot](/images/v2/reference/audio-modules/scriptsynth.png)

The Scriptnode Synthesiser generates polyphonic audio by rendering each voice through a scriptnode DSP network. It provides the full voice management infrastructure of a standard HISE sound generator — voice allocation, gain and pitch modulation chains, extra modulation chain slots, an FX chain slot — while delegating the actual sound generation to the loaded network. This makes it the primary module for building custom synthesisers in scriptnode.

Each voice is independently processed through the network with its own state. The network receives note-on and note-off events and must produce audio output. Voice killing can be handled by the network itself (via a gate signal) or by adding a [ScriptnodeVoiceKiller]($MODULES.ScriptnodeVoiceKiller$) to the gain modulation chain.

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
      desc: "Stereo panning position"
      range: "-1.0 L - 1.0 R"
      default: "0.0 (centre)"
    VoiceLimit:
      desc: "Maximum number of simultaneous voices"
      range: "1 - 256"
      default: "256"
    KillFadeTime:
      desc: "Fade-out time when voices are killed"
      range: "0 - 20000 ms"
      default: "20 ms"
  functions:
    initVoice:
      desc: "Initialises the scriptnode network's state for the new voice"
    networkProcess:
      desc: "Renders audio through the scriptnode network for this voice"
  modulations:
    GainModulation:
      desc: "Scales the output volume per voice"
      scope: "per-voice"
    PitchModulation:
      desc: "Modulates the pitch of all voices via pitch_mod nodes in the network"
      scope: "per-voice"
    ExtraModChains:
      desc: "Extra modulation chain slots that drive extra_mod nodes inside the network. Default count is 2, configurable via HISE_NUM_SCRIPTNODE_SYNTH_MODS"
      scope: "per-voice"
---

```
// Scriptnode Synthesiser - polyphonic scriptnode sound generator
// noteOn/noteOff in -> audio L/R out (per voice)

onNoteOn() {
    initVoice()    // set up network state for this voice
}

perVoice() {
    // Extra modulation chains drive network parameters
    networkParams *= ExtraModChains

    networkProcess(voiceBuffer)

    // Apply output stage
    voiceBuffer *= Gain * GainModulation
    fxChain.process(voiceBuffer)
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
      - { name: Balance, desc: "Stereo balance position. -1.0 is fully left, 1.0 is fully right", range: "-1.0 - 1.0", default: "0.0" }
      - { name: VoiceLimit, desc: "Maximum number of simultaneous voices. Excess voices are killed with KillFadeTime", range: "1 - 256", default: "256" }
      - { name: KillFadeTime, desc: "Fade-out time in milliseconds when voices are killed by exceeding the voice limit or by a voice killer", range: "0 - 20000 ms", default: "20 ms" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: Gain Modulation, desc: "Scales the output volume per voice. Applied after network processing", scope: "per-voice", constrainer: "*" }
  - { name: Pitch Modulation, desc: "Modulates the pitch of all voices. Connected to pitch_mod nodes inside the network", scope: "per-voice", constrainer: "*" }
  - { name: Extra Modulation Chains, desc: "Additional modulation slots for driving extra_mod nodes in the network. Default count is 2; increase via HISE_NUM_SCRIPTNODE_SYNTH_MODS in project settings", scope: "per-voice", constrainer: "*" }
---
::

### Loading a Network

Create a scriptnode network in the `onInit` callback by calling `Engine.createDspNetwork("NetworkName")`. This creates a new network or loads an existing one from the `DspNetworks/Networks/` folder, where networks are stored as `.xml` files.

The network must be designed for polyphonic operation — it receives per-voice note events and maintains per-voice state. Use polyphonic containers (e.g. `container.poly`) within the network to handle voice-specific processing. Any nodes placed outside a polyphonic container will run in monophonic mode and share state across all voices.

Switching networks at runtime is possible by calling `Engine.createDspNetwork()` again with a different name. However, this is a heavyweight operation that reinitialises the entire DSP graph and should not be done during audio playback. For preset-style switching, expose network parameters and change those instead.

### Modulation Chain Configuration

This module has built-in Gain and Pitch modulation chains plus `HISE_NUM_SCRIPTNODE_SYNTH_MODS` extra slots (default: 2). See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for how extra modulation slots connect to network parameters.

### Channel Configuration

This module supports the standard scriptnode channel configuration. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels).

### Parameter Exposure and Complex Data

Network parameters are appended starting at index 4, after the fixed parameters (Gain, Balance, VoiceLimit, KillFadeTime). See [Custom module hosting](/v2/reference/audio-modules/#custom) for parameter exposure, complex data types, and the configuration table.

### Voice Management

Every sound generator in HISE needs a mechanism to stop voices. For ScriptSynth, there are two approaches:

- **Network-internal**: The network produces a gate signal that closes when the voice should stop. This requires the network to implement proper note-off handling.
- **ScriptnodeVoiceKiller**: Add a [ScriptnodeVoiceKiller]($MODULES.ScriptnodeVoiceKiller$) module to the gain modulation chain. It monitors the network's gate signal and terminates voices when the gate closes.

Without either mechanism, voices accumulate indefinitely until the voice limit is reached.

**See also:** $MODULES.HardcodedSynth$ -- compiled variant for exported plugins, $MODULES.ScriptnodeVoiceKiller$ -- voice killer for scriptnode-based sound generators, $MODULES.ScriptEnvelopeModulator$ -- scriptnode envelope for per-voice modulation
