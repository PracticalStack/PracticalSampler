---
title: Silent Synth
moduleId: SilentSynth
type: SoundGenerator
subtype: SoundGenerator
tags: [custom]
builderPath: b.SoundGenerators.SilentSynth
screenshot: /images/v2/reference/audio-modules/silentsynth.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors:
    - { parameter: "FX Chain contents", impact: "variable", note: "CPU depends entirely on loaded effects" }
seeAlso:
  - { id: SendContainer, type: companion, reason: "Receives audio from a SendFX - often hosted in a SilentSynth's FX chain for parallel processing" }
  - { id: SynthChain, type: disambiguation, reason: "Container for multiple sound generators - SilentSynth is a single generator that hosts effects without producing audio" }
commonMistakes:
  - title: "Adding gain or pitch modulators to a SilentSynth"
    wrong: "Adding modulators to the Gain or Pitch modulation chains"
    right: "Both chains are disabled - modulators added to them will have no effect"
    explanation: "SilentSynth produces no audio of its own, so gain and pitch modulation are meaningless. The chains are explicitly disabled."
  - title: "Expecting effects to process audio without external routing"
    wrong: "Adding a filter or delay to the FX chain and expecting it to produce sound"
    right: "Route audio into the SilentSynth via the routing matrix or use send effects to feed signal into it"
    explanation: "The FX chain receives a silent buffer. Effects that only process their input will output silence. Use effects that receive signal from elsewhere (routing matrix, send buses) or generate their own output."
llmRef: |
  Silent Synth (SoundGenerator/SoundGenerator)

  A sound generator that produces no audio but runs its effect chain on each voice. Used to host effects without a sound source, route external signals through an FX chain, or create FX bus configurations.

  Signal flow:
    noteOn -> clear buffer (silence) -> FX chain -> Gain/Balance -> output

  CPU: negligible baseline (buffer clear only). Actual cost depends on loaded effects.

  Parameters:
    Gain (0 - 100%, default 25%) - output volume as normalised linear gain
    Balance (-1.0 - 1.0, default 0.0) - stereo balance
    VoiceLimit (1 - 256, default 256) - maximum simultaneous voices
    KillFadeTime (0 - 20000 ms, default 20 ms) - fade out time when voices are killed

  Disabled chains:
    Gain Modulation - disabled (no audio to modulate)
    Pitch Modulation - disabled (no pitched audio)

  When to use:
    - Host effects that process externally-routed audio
    - Create FX send/return buses alongside SendFX and SendContainer
    - Run effects that generate their own audio (synthesiser effects, noise generators)
    - Multi-channel effect processing via the resizable routing matrix

  Does not require an envelope. Voices are released when the FX chain has no tailing effects.

  Common mistakes:
    Adding gain/pitch modulators has no effect (chains are disabled).
    Effects processing only input will output silence unless signal is routed in externally.

  See also:
    companion SendContainer - receives send bus audio, often paired with SilentSynth
    disambiguation SynthChain - container for multiple generators (different purpose)
---

::category-tags
---
tags:
  - { name: custom, desc: "Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks" }
---
::

![Silent Synth screenshot](/images/v2/reference/audio-modules/silentsynth.png)

The Silent Synth is a sound generator that produces no audio of its own. Each voice clears its buffer to silence and then passes it through the FX chain. This makes it useful for hosting effects that receive their signal from elsewhere - via the routing matrix, send buses, or sidechain inputs - without needing an actual sound source.

Unlike other sound generators, the Silent Synth does not require an envelope modulator. Both the Gain Modulation and Pitch Modulation chains are disabled since there is no audio to shape or pitch to shift. The routing matrix supports multi-channel configurations, allowing the module to act as a flexible FX host for complex routing setups. Voices are automatically released once the FX chain has no remaining tailing effects (such as reverb or delay tails).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gain:
      desc: "Output volume as normalised linear gain"
      range: "0 - 100%"
      default: "25%"
    Balance:
      desc: "Stereo balance of the output"
      range: "-1.0 - 1.0"
      default: "0.0"
  functions:
    clearBuffer:
      desc: "Fills the voice buffer with silence - no audio is generated"
    renderFXChain:
      desc: "Processes the voice buffer through all loaded effects"
---

```
// Silent Synth - silent generator with FX chain
// noteOn in -> audio out (from FX chain only)

process(left, right) {
    clearBuffer(left, right)       // silence - no audio generated

    renderFXChain(left, right)     // effects process the silent buffer
                                   // (or receive signal via routing/sends)

    left  *= Gain
    right *= Gain
    applyBalance(left, right, Balance)
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
        desc: "The output volume as normalised linear gain. Since the module generates silence, this only affects signal produced by the FX chain"
        range: "0 - 100%"
        default: "25%"
      - name: Balance
        desc: "The stereo balance of the output signal"
        range: "-1.0 L - 1.0 R"
        default: "0.0 (centre)"
  - label: Voice Management
    params:
      - name: VoiceLimit
        desc: "The maximum number of simultaneous voices. Each voice runs its own instance of the FX chain"
        range: "1 - 256"
        default: "256"
      - name: KillFadeTime
        desc: "The fade out time in milliseconds when voices are killed by exceeding the voice limit or by a voice killer"
        range: "0 - 20000 ms"
        default: "20 ms"
---
::

### No Envelope Required

Unlike most sound generators, the Silent Synth does not need an envelope modulator. Adding one will not cause errors, but it serves no purpose since there is no audio amplitude to shape. HISE will not display a warning about a missing envelope for this module.

### Voice Release and Effect Tails

When a note is released, the voice checks whether any polyphonic effects in the FX chain still have audio tailing (for example, a reverb tail or delay feedback). If tailing effects are active, the voice stays alive until they finish. If no effects are tailing, the voice is released immediately. This ensures that effect tails are not cut short.

### Multi-Channel Routing

The routing matrix supports resizing, allowing the Silent Synth to work with more than the standard stereo pair. When the channel count changes, the module automatically resizes its internal buffers and propagates the new channel configuration to all effects in the FX chain. This is useful for surround or multi-bus effect configurations.

**See also:** $MODULES.SendContainer$ -- receives audio from a SendFX for parallel processing, $MODULES.SynthChain$ -- container for multiple sound generators (different purpose from SilentSynth)
