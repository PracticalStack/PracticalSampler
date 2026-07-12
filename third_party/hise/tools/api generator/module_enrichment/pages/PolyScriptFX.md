---
title: Polyphonic Script FX
moduleId: PolyScriptFX
type: Effect
subtype: VoiceEffect
tags: [custom]
builderPath: b.Effects.PolyScriptFX
screenshot: /images/v2/reference/audio-modules/polyscriptfx.png
cpuProfile:
  baseline: "(depends on loaded network)"
  polyphonic: true
  scalingFactors:
    - { parameter: "(active voices)", impact: "linear", note: "CPU scales with the number of active voices" }
seeAlso:
  - { id: HardcodedPolyphonicFX, type: alternative, reason: "Compiled variant for exported plugins" }
  - { id: ScriptFX, type: alternative, reason: "Monophonic variant for master effects that do not need per-voice processing" }
  - { id: ScriptSynth, type: companion, reason: "Scriptnode synthesiser - PolyScriptFX is typically placed in its FX chain for per-voice effects" }
commonMistakes:
  - title: "Using PolyScriptFX for monophonic effects"
    wrong: "Adding a PolyScriptFX for an effect that does not need per-voice state (e.g. master reverb)"
    right: "Use ScriptFX for monophonic effects - it processes once per buffer instead of once per voice"
    explanation: "PolyScriptFX processes the network independently for every active voice. For effects that should be shared across all voices, use ScriptFX instead."
  - title: "Network not compiled before export"
    wrong: "Exporting with an uncompiled network"
    right: "Compile the network to a DLL before exporting, or switch to HardcodedPolyphonicFX"
    explanation: "Uncompiled networks only work in the HISE IDE."
  - title: "Forgetting to enable extra mod chains"
    wrong: "Trying to add modulation chains to PolyScriptFX without setting the preprocessor definition"
    right: "Add HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS=N to the Extra Definitions field in the project settings"
    explanation: "By default, PolyScriptFX has zero extra modulation chain slots. You must set the HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS preprocessor macro to the desired count before compilation."
llmRef: |
  Polyphonic Script FX (Effect/VoiceEffect)

  Per-voice effect that processes each voice independently through a scriptnode DSP network. Each voice maintains its own state in the network.

  Signal flow:
    voice audio in -> [voice active?] -> ExtraModChains modulate network params -> init voice on note-on -> network process (per voice) -> voice audio out
                                      -> bypass (voice silent)

  CPU: depends on loaded network, polyphonic
    Scales linearly with active voices. Includes voice suspension for silent voices.

  Parameters:
    No fixed parameters (offset = 0). All parameters come from the loaded network.

  Modulation chains:
    Extra Modulation Chains (default 0, configurable via HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS) - see Audio Modules modulators reference for connection mode details

  Channel configuration:
    Standard scriptnode channel configuration (default stereo, up to 16). See Audio Modules sound-generators reference for details.

  Complex data types and parameter exposure:
    No fixed parameters (offset = 0). See Audio Modules index Custom section for complex data types, parameter exposure, and configuration table.

  When to use:
    Use for per-voice effects like distortion, filtering, or waveshaping that need independent state per voice. For monophonic effects, use ScriptFX instead.

  Common mistakes:
    Do not use for monophonic effects - wastes CPU on per-voice processing.
    Must compile network before export.
    Forgetting to set HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS when needing extra mod slots.

  See also:
    alternative HardcodedPolyphonicFX - compiled variant for exported plugins
    alternative ScriptFX - monophonic variant for master effects
    companion ScriptSynth - scriptnode synthesiser
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules with user-defined signal paths via scriptnode networks or HISEScript callbacks" }
---
::

![Polyphonic Script FX screenshot](/images/v2/reference/audio-modules/polyscriptfx.png)

The Polyphonic Script FX processes each voice independently through a scriptnode DSP network. Every active voice has its own state in the network, making it suitable for per-voice effects like distortion, filtering, waveshaping, or any effect that should sound different depending on the note being played. Place it in a sound generator's FX chain for per-voice processing.

Unlike [ScriptFX]($MODULES.ScriptFX$), which processes the entire mixed output once per buffer, PolyScriptFX runs the network separately for every active voice. This provides true polyphonic processing but costs proportionally more CPU. For effects that do not need per-voice state, use ScriptFX instead.

## Signal Path

::signal-path
---
glossary:
  functions:
    initVoice:
      desc: "Initialises the scriptnode network's state for the new voice on note-on"
    networkProcess:
      desc: "Processes audio through the scriptnode network for this voice"
  modulations:
    ExtraModChains:
      desc: "Extra modulation chain slots that drive extra_mod nodes inside the network. Default count is 0, configurable via HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS"
      scope: "per-voice"
---

```
// Polyphonic Script FX - per-voice scriptnode effect
// voice audio in -> voice audio out

onNoteOn() {
    initVoice()    // set up network state for this voice
}

perVoice() {
    if (voiceSilent)
        return    // skip processing for suspended voices

    // Extra modulation chains drive network parameters
    networkParams *= ExtraModChains

    networkProcess(voiceBuffer)
}
```

::

### Loading a Network

Create a scriptnode network in the `onInit` callback by calling `Engine.createDspNetwork("NetworkName")`. This creates a new network or loads an existing one from the `DspNetworks/Networks/` folder, where networks are stored as `.xml` files.

The network must be designed for polyphonic operation — each voice runs an independent instance of the network with its own state. Use polyphonic containers (e.g. `container.poly`) within the network to handle voice-specific processing. Only `onInit` and `onControl` callbacks are available — there is no HISEScript audio processing fallback. All audio processing must come from the scriptnode network.

Switching networks at runtime is possible by calling `Engine.createDspNetwork()` again with a different name. However, this is a heavyweight operation that reinitialises the entire DSP graph and should not be done during audio playback. For preset-style switching, expose network parameters and change those instead.

### Modulation Chain Configuration

This module has no built-in Gain or Pitch modulation chains. Extra modulation slots are controlled by `HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS` (default: 0). The parent synth's pitch chain is connected to the network's runtime targets, allowing pitch modulation to affect `pitch_mod` nodes. See [Scriptnode Modulation Bridge](/v2/reference/audio-modules/modulators/#scriptnode-modulation-bridge) for how extra modulation slots connect to network parameters.

### Channel Configuration

This module supports the standard scriptnode channel configuration. See [Scriptnode and Hardcoded Module Channels](/v2/reference/audio-modules/sound-generators/#scriptnode-and-hardcoded-module-channels).

### Parameter Exposure and Complex Data

This module has no fixed parameters — all network parameters start at index 0. See [Custom module hosting](/v2/reference/audio-modules/#custom) for parameter exposure, complex data types, and the configuration table.

### Voice Management

PolyScriptFX initialises a new voice in the network on each note-on event. The network maintains per-voice state via the polyphonic handler. Voice suspension is supported — if a voice's audio buffer is silent, processing can be skipped to save CPU. The module performs pre- and post-silence checks to determine when a voice can be suspended or safely stopped.

Non-noteOn MIDI events (note-off, controllers, pitch bend) are forwarded to the network for all active voices.

**See also:** $MODULES.HardcodedPolyphonicFX$ -- compiled variant for exported plugins, $MODULES.ScriptFX$ -- monophonic variant for effects that do not need per-voice processing
