---
title: Noise Grain Player
moduleId: NoiseGrainPlayer
type: Effect
subtype: VoiceEffect
tags: [utility]
builderPath: b.Effects.NoiseGrainPlayer
screenshot: /images/v2/reference/audio-modules/noisegrainplayer.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: Polyphony, impact: high, note: "Per-voice grain rendering scales linearly with active voice count" }
seeAlso:
  - { id: PolyshapeFX, type: alternative, reason: "Another polyphonic effect with per-voice processing, but applies waveshaping rather than granular synthesis" }
  - { id: Convolution, type: disambiguation, reason: "Also loads an audio file for processing, but applies it as an impulse response rather than decomposing it into noise grains" }
  - { id: "core.granulator", type: scriptnode, reason: "Scriptnode granular synthesiser with position, density, and detune controls" }
commonMistakes:
  - title: "WhiteNoise modulates grain position"
    wrong: "Expecting the WhiteNoise parameter to mix audio-domain white noise into the output"
    right: "WhiteNoise randomises the read position within each grain, producing a noisier texture through phase scrambling"
    explanation: "Despite the name, WhiteNoise does not add white noise to the audio signal. It applies a random offset to the per-sample read position inside each grain. Higher values create a rougher, more diffuse texture."
  - title: "GrainSize change triggers reanalysis"
    wrong: "Changing GrainSize during playback and expecting an instant update"
    right: "GrainSize triggers a full offline reanalysis of the audio file, which runs asynchronously"
    explanation: "When GrainSize changes, the module re-runs the FFT-based noise decomposition on the loaded audio file. Active voices are killed during this process. Avoid automating GrainSize during performance."
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: complex
  description: "Would require FFT-based sinusoidal-transient-noise separation, a grain scheduler with overlap, and per-sample interpolation with position noise - not practical to replicate in scriptnode"
llmRef: |
  Noise Grain Player (VoiceEffect, polyphonic)

  A polyphonic granular effect that decomposes a loaded audio file into noise grains using FFT-based sinusoidal-transient-noise separation, then plays them back per voice at a modulatable position. Each voice independently schedules grains with 4x overlap.

  Signal flow:
    Offline: audio file -> SiTraNo FFT decomposition -> Hann-windowed noise grains
    Per voice: input -> dry gain (overlap fader) -> add grain output * wet gain -> output

  CPU: low baseline per voice. Scales linearly with polyphony. Offline analysis is asynchronous and one-time per file/GrainSize change.

  Parameters:
    Position (0 - 100%, default 0%) - normalised playback position selecting which grain to play
    Mix (0 - 100%, default 100%) - overlap fader crossfade between dry input and grain output
    WhiteNoise (0 - 100%, default 0%) - randomises grain read position for a noisier texture (not audio-domain noise)
    GrainSize (256/512/1024/2048/4096/8192 samples, default 8192) - FFT frame size for offline decomposition; triggers reanalysis

  Modulation chains:
    Table Index Modulation (gain, per-voice) - multiplicative scaling of Position
    Table Index Bipolar (offset, per-voice) - bipolar additive offset to Position

  When to use:
    Adding noise-based texture from an audio file to individual voices. Load a sample, and each voice plays back decomposed noise grains at the selected position with optional position randomisation.

  Common mistakes:
    WhiteNoise randomises read position, not audio noise. GrainSize changes trigger async reanalysis and kill voices.

  Custom equivalent:
    Not practical in scriptnode due to FFT decomposition and custom grain scheduling.

  See also:
    alternative PolyshapeFX - polyphonic waveshaping effect
    disambiguation Convolution - also loads audio files but uses them as impulse responses
    [scriptnode] core.granulator -- scriptnode granular synthesiser with position, density, and detune controls
---

::category-tags
---
tags:
  - { name: utility, desc: "Modules for analysis, placeholders, or structural purposes without audio processing" }
---
::

![Noise Grain Player screenshot](/images/v2/reference/audio-modules/noisegrainplayer.png)

The Noise Grain Player is a polyphonic granular effect that extracts the noise component from a loaded audio file and plays it back as overlapping grains. Each voice independently selects and renders grains at a modulatable position within the file, with 4x overlap for smooth playback.

The module first analyses the audio file offline using FFT-based sinusoidal-transient-noise separation, producing a collection of Hann-windowed noise grains. At runtime, the Position parameter (with gain and bipolar modulation chains) determines which grain each voice plays. The Mix parameter crossfades between the original voice signal and the grain output using an overlap fader. The WhiteNoise parameter adds per-sample randomisation to the grain read position, creating a rougher, more diffuse texture.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Position:
      desc: "Normalised playback position selecting which grain to play"
      range: "0 - 100%"
      default: "0%"
    Mix:
      desc: "Overlap fader crossfade between dry input and grain output"
      range: "0 - 100%"
      default: "100%"
    WhiteNoise:
      desc: "Randomises per-sample grain read position for a noisier texture"
      range: "0 - 100%"
      default: "0%"
    GrainSize:
      desc: "FFT frame size for offline noise decomposition"
      range: "256 / 512 / 1024 / 2048 / 4096 / 8192 samples"
      default: "8192 samples"
  functions:
    decompose:
      desc: "FFT-based sinusoidal-transient-noise separation; extracts the noise component as Hann-windowed grains"
    selectGrain:
      desc: "Selects a grain by index with anti-repeat jitter to avoid consecutive duplicates"
    interpolate:
      desc: "Per-sample linear interpolation within the grain buffer, with optional position noise from WhiteNoise"
  modulations:
    TableIndexModulation:
      desc: "Multiplicative scaling of Position"
      scope: "per-voice"
    TableIndexBipolar:
      desc: "Bipolar additive offset to Position"
      scope: "per-voice"
---

```
// Noise Grain Player - polyphonic granular noise effect
// voice audio in -> voice audio out (per voice)

// Offline stage (runs once per file or GrainSize change)
grains = decompose(audioFile, GrainSize)

// Per-voice render
process(left, right) {
    // Modulated position
    position = Position * TableIndexModulation + TableIndexBipolar

    // Overlap fader gains
    dryGain = clamp(0, 1, 2 - 2 * Mix)
    wetGain = clamp(0, 1, 2 * Mix)

    // Scale input by dry gain
    input *= dryGain

    // Schedule new grain every hopSize samples (4x overlap)
    grain = selectGrain(grains, position)

    // Render active grains (8 slots, per-sample)
    for each sample:
        output = interpolate(grain, WhiteNoise)
        input += output * wetGain
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Playback
    params:
      - { name: Position, desc: "Normalised playback position within the analysed audio file, selecting which noise grain to play. Modulate this to sweep through different grain regions.", range: "0 - 100%", default: "0%" }
      - { name: GrainSize, desc: "Size of each grain in samples, determined by the FFT frame size used during offline analysis. Larger grains capture lower-frequency noise detail but produce fewer total grains. Changing this triggers a full reanalysis of the audio file.", range: "256 samples, 512 samples, 1024 samples, 2048 samples, 4096 samples, 8192 samples", default: "8192 samples" }
  - label: Texture
    params:
      - { name: WhiteNoise, desc: "Amount of random offset applied to the per-sample read position within each grain. At 0%, grains play back faithfully. Higher values scramble the phase relationships, creating a rougher, more diffuse texture. This does not add audio-domain white noise.", range: "0 - 100%", default: "0%" }
  - label: Mix
    params:
      - { name: Mix, desc: "Crossfade between the dry voice signal and the grain output using an overlap fader. At 0% the input passes through unaffected. At 50% both signals are at full volume. At 100% only the grain output is heard.", range: "0 - 100%", default: "100%" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "Table Index Modulation", desc: "Multiplicative scaling of the Position parameter. Applied per block before the bipolar offset.", scope: "per-voice", constrainer: "Any" }
  - { name: "Table Index Bipolar", desc: "Bipolar additive offset to the Position parameter. Applied per block after the gain scaling.", scope: "per-voice", constrainer: "Any" }
---
::

### WhiteNoise Behaviour

The WhiteNoise parameter name is misleading - it does not add audio-domain white noise. It randomises the read position within each grain on a per-sample basis, which scrambles phase relationships and produces a noise-like effect. The intensity scales by a factor of 10 internally, so even moderate values create noticeable texture changes.

The WhiteNoise amount is captured when a voice starts. Changes to the WhiteNoise parameter after a voice has already begun do not affect that voice's noise amount.

### Mix Scaling

The overlap fader used by the Mix parameter is linear, not equal-power. Around Mix = 50%, both the dry and wet signals are at full volume, so the combined output can exceed unity gain.

### Minimum File Length

If the loaded audio file is shorter than one FFT frame at the current GrainSize, no grains are produced and the module outputs silence on the wet path.

### Grain Playback Speed

Grain playback speed is determined by the ratio between the host sample rate and the file's sample rate. It is not affected by voice pitch - all voices play grains at the same rate regardless of note number.

**See also:** $MODULES.PolyshapeFX$ -- Another polyphonic effect with per-voice processing, but applies waveshaping rather than granular synthesis, $MODULES.Convolution$ -- Also loads an audio file for processing, but applies it as an impulse response rather than decomposing it into noise grains, $SN.core.granulator$ -- scriptnode granular synthesiser with position, density, and detune controls
